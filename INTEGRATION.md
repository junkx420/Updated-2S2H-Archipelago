# 2S2H Archipelago — Integration Status

Goal: make 2S2H a pure Archipelago client for the existing
"Majora's Mask Recompiled" world (MMRecompRando's apworld), keeping full
yaml/ID/tracker compatibility. Generation stays in Python; the game maps AP
ids onto its native Rando enums and reuses `Rando::GiveItem` / check hooks.

## Layout

    mm/CMake/Archipelago.cmake                               dependency stack (apclientpp/wswrap/asio/websocketpp/OpenSSL)
    mm/2s2h/Network/Archipelago/Archipelago.{h,cpp}          ArchipelagoClient singleton (connect/poll/handlers)
    mm/2s2h/Network/Archipelago/ArchipelagoConsoleWindow.*   ImGui console (connect form + log)
    mm/2s2h/Network/Archipelago/ArchipelagoMenu.cpp          BenMenu Dev Tools -> Archipelago section
    mm/2s2h/Network/Archipelago/ArchipelagoMapping.{h,cpp}   GENERATED AP id <-> RandoCheckId/RandoItemId tables
    mm/2s2h/Network/Archipelago/tools/gen_ap_mapping.py      mapping generator (edit overrides HERE, not the .cpp)

Existing-file edits, kept deliberately tiny:
- mm/CMakeLists.txt: one `include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Archipelago.cmake)`
  after the main `target_include_directories` block (~line 246).
- CMakeLists.txt (root): `openssl` added to the vcpkg package list, gated on
  `BUILD_ARCHIPELAGO` (default ON; `-DBUILD_ARCHIPELAGO=OFF` builds vanilla).

Everything else is new files: the build globs `2s2h/**` recursively and the
window/menu self-register via `ShipInit` / `RegisterMenuInitFunc`.

## Build (Windows)

    cmake -S . -B build/x64 -G "Visual Studio 17 2022" -A x64 -DBUILD_ARCHIPELAGO=ON
    cmake --build build/x64 --target 2ship
    # once per run-dir: copy the CA bundle for wss://
    mkdir x64/Debug/networking
    copy build\x64\mm\networking\cacert.pem x64\Debug\networking\

Build pitfalls already solved (do not regress):
- `SKIP_PRECOMPILE_HEADERS` + winsock2 prologue on Archipelago.cpp
  (PCH pulls WinSock 1 before asio otherwise).
- `_WEBSOCKETPP_CPP11_STL_` + `ASIO_STANDALONE` defines (websocketpp falls
  back to Boost without them).
- OpenSSL comes from the project's own vcpkg (3.6.x, static CRT). System
  OpenSSL 4.x installs (e.g. slproweb in Program Files) break asio's ssl
  layer AND mismatch the CRT — if `asn1_string_st` errors ever reappear,
  CMake re-found the wrong OpenSSL.
- `/bigobj` on Archipelago.cpp (whole apclientpp stack in one TU).

## In-game

ESC menu -> Dev Tools -> Archipelago (console embeds inline; popout button
available). Address/slot/password persist in CVars `gNetwork.Archipelago.*`.
"TLS handshake failed" against a local ws:// server is the normal secure->
plain fallback and is logged as such.

## Status

- [x] Step 1 — link layer: connects to a hosted mm_recomp seed, receives
      slot_data + items, server chat streams to the console. Verified live.
- [x] Step 2 (core) — ID mapping: ArchipelagoMapping.cpp generated from the
      apworld + 2S2H static data. 383/419 locations, 111/111 items.
      * Matching is mostly structural (AP ids embed scene+flag / GetItem id:
        chests 0x06SSFF, collectibles 0x05, stray fairies 0x01, GI 0x00GGG,
        songs 0x04), so it is robust against name drift.
      * ~29 AP locations intentionally unmapped (None in the script): night/
        spring shop slots, starting-inventory slots, curiosity rupee trades —
        2S2H has no equivalent checks yet.
      * 6 open: Powder Keg Goron, Deku Palace Bean Seller, Zora Cape Pot
        Game, Lulu Good/Bad Pictures, Ikana Spirit House.
      * Overrides marked `# VERIFY` (cow left/right order, granny story
        order, Twin Islands spring chests) need in-game confirmation.
      * Known design debts: Tingle maps are per-map in AP but per-vendor in
        2S2H (alternate vendor needs game-side aliasing); "Glitched" AP
        locations alias the same physical chest (reverse map picks the
        canonical id); Goron Village HP exists per-season in AP but is one
        check in 2S2H; Progressive Shield / Bombchu Bag approximated
        (RI_SHIELD_HERO / RI_BOMBCHU_10).
- [x] Step 2 (slot_data) — ArchipelagoSlotData.{h,cpp}: typed `ApSlot::SlotData`
      (enums mirror the apworld's Options.py) parsed in the slot_connected
      handler; tolerant of missing/mistyped fields (warnings to the console),
      handles the apworld's "curiostity_shop_trades" typo, reset on
      disconnect. Summary printed on connect; access via
      `ArchipelagoClient::GetSlotOptions()` (check `.valid`).
      `shopPrices[i]` is indexed by the recomp SHOP_ID_* slot id (0-35).
- [x] Step 3 (MVP) — gameplay glue in ArchipelagoGameplay.cpp, running on the
      same per-frame hook as Rando's CheckQueue (OnActorUpdate ACTOR_PLAYER,
      i.e. only during actual gameplay on a rando save while connected):
      * OUT: checks with `obtained` in RANDO_SAVE_CHECKS are diffed against a
        session sent-set and reported via LocationChecks; the set resets on
        reconnect so the whole save resyncs (server dedups).
      * IN: items_received are queued (with sender alias) and applied one per
        frame through ConvertItem + Rando::GiveItem + a notification toast.
      * Resync: `apItemsReceived` high-water mark added to RandoSaveInfo
        (z64save.h + BenJsonConversions.hpp, tolerant of older saves) makes
        grants idempotent across reconnects/reloads.
- [x] Step 4 — scouted placements (chest contents match the multiworld):
      * On connect, LocationScouts for all missing locations; location_info
        stores {item, owner, names} per AP location (names resolved from the
        server datapackage at receive time).
      * On the first gameplay frame (and after every save load/reconnect),
        ApplyScoutedPlacements rewrites RANDO_SAVE_CHECKS: own items become
        the real RandoItemId (native textbox/model/grant), foreign items
        become RI_ARCHIPELAGO_ITEM (new enum value, appended at the END of
        RandoItemId — values persist in saves; give is a no-op, drawn as a
        Moon's Tear). Reporting a foreign location emits a
        "Sent <item> to <player>" toast.
      * items_handling switched 0b111 -> 0b101 (LOCAL-GRANT model): the
        server no longer echoes our own items, chests grant them natively;
        only remote items + starting inventory arrive via items_received.
      * Testing: use a FRESH native rando save (old test saves have stale
        dual-grant state). Expected: "Scouted N locations" on connect, then
        "Applied N multiworld placements" once in-game.
- [x] Step 5 — goal, DeathLink, option translation:
      * OnGameCompletion (Majora defeat / triforce hunt) -> StatusUpdate(GOAL).
      * DeathLink: tag added via ConnectUpdate after slot_data confirms it;
        incoming Bounced deaths kill Link (health=0) with a toast; own deaths
        detected by health transition and Bounced out. Self-echo filtered by
        source name; received deaths never re-broadcast (no ping-pong).
      * ApplySlotOptions (runs with placements): moon/majora remains ->
        RANDO_SAVE_OPTIONS[RO_ACCESS_*_REMAINS_COUNT], RO_SHUFFLE_SHOPS under
        shopsanity, and per-slot shop prices stamped onto shop checks via the
        SHOP_PRICE_INDEX table (RC_* -> Constants.py SHOP_ID_*).
- [x] Step 6 — file flow, AP model, aliasing, more parity:
      * AP file flow (OnApFileCreate, hooked on OnSaveInit): creating a file
        while CONNECTED (and native "Enable Rando" OFF) makes an AP rando
        save directly — native intro-skip state, RO defaults + slot options,
        finalSeed from the multiworld; no generator/seed/spoiler involved.
      * RI_ARCHIPELAGO_ITEM draws a runtime-built AP logo (six vertex-colored
        discs) in DrawItem.cpp — no custom asset/texture required.
      * Tingle aliases: alternate vendor slots mirror the primary's
        item/price and obtained state syncs per frame (either purchase
        reports the shared AP location; no double buys).
      * ApplySlotOptions also covers RO_SHUFFLE_GOLD_SKULLTULAS +
        RO_SKULLTULA_TOKENS_REQUIRED, RO_STRAY_FAIRIES_REQUIRED,
        RO_SHUFFLE_COWS, RO_SHUFFLE_TINGLE_SHOPS (maps=anywhere).
- [ ] Known deltas: magic_is_a_trap not honored (conflicts with the native
      VB_GRANT_MAGIC_UPON_REQUEST fix in MiscBehavior; needs a coordinated
      change); CAMC/link_tunic_color cosmetic options unwired; the ~35 AP
      locations without 2S2H checks (see mapping report).

Regenerating the mapping after apworld or Checks.cpp changes:

    python mm/2s2h/Network/Archipelago/tools/gen_ap_mapping.py \
        --apworld build/apworld_extract/mm_recomp --report build/ap_mapping_report.md
