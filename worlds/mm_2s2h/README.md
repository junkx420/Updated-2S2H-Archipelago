# mm_2s2h – Majora's Mask (2 Ship 2 Harkinian)

Unofficial fan project, see the `main` branch README of `junkx420/Updated-2S2H-Archipelago`.

## Downloads

The regular release is **0.1.5** (bundled apworld **0.1.4**). The **0.4.0 pre-release** packages NPC souls,
temple entrances and boss entrances with a matching client. The current source is **apworld 0.15.0**.
The new mechanics are **not tested in game**. Platform builds and preview publication run on GitHub Actions.
See the [download overview](https://github.com/junkx420/Updated-2S2H-Archipelago#choose-your-download),
[changelog](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/main/CHANGELOG.md) and
[publication progress](https://github.com/junkx420/Updated-2S2H-Archipelago/actions/workflows/publish-preview.yml).
Use the client and apworld from the same release; the release manifest records their exact commits.

## In-game Check Tracker

The client applies NPC souls, door keys, Epona riding and bean planting to check reachability. It shows shuffled rocks
and signs with their region, time and layout rules. For AP saves with fork rules enabled, it starts from the same root as the
apworld and follows the saved entrance pairings, including coupled returns and blue boss warps. With the fork rules
off, the current region remains an additional starting point. See the
[tracker comparison and in-game checklist](docs/testing.md#check-tracker-saved-entrance-pairs-part-b).

## Files

| File | Origin |
|---|---|
| `regions_generated.py`, `macros_generated.py`, `locations_generated.py`, `items_generated.py`, `enums_generated.py`, `UNTRANSLATED.md` | **generated** by `tools/gen_from_2s2h.py`, do not edit |
| `primitives.py` | hand-written save-state layer (port of `GiveItem.cpp` / `ConvertItem.cpp`) and the Logic.h primitives |
| `time_logic.py` | time operators (see `docs/time-logic.md`) |
| `options.py` | the `RO_*` options of the 2S2H randomizer, same defaults |
| `__init__.py` | AP world: regions, events, pool construction (`GeneratePools.cpp`, `StartingItems.cpp`), slot data |
| `tools/parity/` | compiles 2S2H's real logic with a mocked save and compares it with this world (`docs/testing.md`) |
| `tools/time_spike/` | Phase 0 check of the time propagation against the C++ code |
| `tools/random_seeds.py` | generates N seeds with random options |

## Additions that are not in 2S2H

The generator has to run on the **client fork** (branch `client`), not on upstream 2S2H, because these live there:

- **Shuffle Owl Warps** (`shuffle_owl_warps`, apworld 0.15.0, default off) sends each Song of Soaring selection to
  a different owl for the whole seed. Activations, owl checks and owl saves keep their original statues. Some
  flights need equipment for the way out or a playable Song of Time. The Entrance Tracker reveals each flight
  after arrival. [Escape rules and in-game checklist](docs/testing.md#stage-11a--shuffle-owl-warps).

- **Shuffle Signs** (`shuffle_signs`, apworld 0.14.0, default off) adds 87 checks to the `Signs` location group.
  Read a sign to collect its item after the text. Open wooden signs remain readable when chopped. Existing sign hints
  still work, including the West Clock Town Bank's Archipelago hint. The client and apworld use the same access rules
  from `Rando/Logic/ExtraChecks.inc`. [Inventory, exclusions and in-game checklist](docs/testing.md#stage-10--shuffle-signs-apworld-0140).

- **Clock Shuffle startup** for a newly created Archipelago save selects the earliest half-day owned from the
  initial server inventory. The AP world version stays at 0.13.0. [In-game checklist](docs/testing.md#clock-shuffle-startup).

- **Shuffle Rocks** (`shuffle_rocks`, apworld 0.13.0, default off) adds 122 checks to the `Rocks` location group.
  Lift a small rock or break a boulder to find its item at the rock's position. Open checks use the object marker.
  The client binds each rock by scene, room and actor-list index, with aliases for alternate room headers and one
  check for each stone in a ring. `Rando/Logic/ExtraChecks.inc` supplies the name, region, access rule and availability
  for the client and apworld. See [the exclusions and in-game checklist](docs/testing.md#stage-9--shuffle-rocks-apworld-0130).

- **NPC souls** (`shuffle_npc_souls`, apworld 0.2): 30 items `RI_SOUL_NPC_*`, appended to the enums so no existing
  id moves. The table `npcSouls` in `Rando/ActorBehavior/Souls.cpp` names, per soul, the actors it brings to life and
  the checks and logic events that need it. The generator writes it to `NPC_SOULS` and `__init__.py` adds the soul to
  those rules. To add or change a soul, edit that table and regenerate.
  Since apworld 0.12 the option is a choice: `off` (0), `classic` (1, the 30 souls, also `true`) and `all` (2, 65
  souls). Rows have two optional fields: `level` (2 = only with `all`) and `sceneId` (the soul owns its actors only
  in that scene; EnSob1 runs as `ACTOR_EN_OSSAN` in three shops). Three small tables next to it: `npcSoulExceptions`
  (actor plus params mask, never hidden: the Goron Shrine gatekeeper), `npcSoulCompanions` (a child follows its
  parent's soul: Romani's balloon) and `npcSoulDynaActors` (DynaPoly collision is switched off while hidden: OBJ_UM).
  The new souls are a second block of `RI_SOUL_NPC_*` and `RANDO_INF_OBTAINED_SOUL_OF_NPC_*` (`Souls.h`). Souls
  without checks or events are `useful` and the first to move to the start inventory when the pool is too full.
  How a soul looks is client-only: `Rando/DrawNpcSouls.cpp` draws the NPC's model in a blue light (stage 8c); a new
  soul needs a row there too, otherwise it shows only the blue flame. Door keys show Anju's room key, riding Epona
  Gorman's horse (`DrawItem.cpp`).
- In stage 8b the client marks **open object checks**. Shuffled snowballs, trees, beehives, the Swordsman's School
  gong and owl statues are tinted while their checks are open. The Song of Soaring engraving and the Termina Field
  music wall sparkle. `Rando/ObjectMarker.cpp` supplies the yellow default and the Chest Size Matches Contents
  palette based on the pot categories. Objects with several checks use the fixed yellow marker until all are collected.
  See [the in-game checklist](docs/testing.md#stage-8b--open-object-check-markers).
- **Dungeon entrance shuffle** (`shuffle_dungeon_entrances`, apworld 0.3): the doors of the four temples lead into a
  random one of them, coupled (the temple's exit and its boss's blue warp lead back out of the door used). The table
  `shuffleEntrances` in `Rando/MiscBehavior/EntranceShuffle.cpp` has one row per door (option, way in, way out,
  scenes, boss room and where its warp lands); the generator checks every row against the region graph and writes it
  to `ENTRANCE_SHUFFLE`, `connect_entrances` shuffles the rows of each enabled option with Archipelago's
  `entrance_rando`, and the pairing travels as `entrance_pairs` in the slot data into the save
  (`RandoSaveInfo::entrancePairs`). The client redirects at one place, the new `BeforeTransitionSetup` hook at the
  start of `TRANS_MODE_SETUP` in `z_play.c`. A further group of doors is new rows plus a new `RO_*` option. The
  inverted Stone Tower exterior joins when dungeons are selected for mixing in 0.10. The Woodfall prison door
  stays fixed. Boss rooms have their own option below.
  Since 0.3.1 a door with another temple behind it also needs that temple's **area rule** (`shuffleAreaRules` in the
  same file, generated to `ENTRANCE_AREA_RULES`): what the vanilla way to that temple's own door requires, because
  the 2S2H temple logic may rely on it silently. Woodfall: Deku Mask; Great Bay: Zora Mask and Hookshot; Stone Tower:
  Hookshot, Elegy of Emptiness, Goron and Zora Mask; Snowhead: none (its Goron Mask block comes before every check
  but an enemy drop). A temple behind its own door keeps its vanilla rule. The rule is logic only: the in-game check
  tracker does not know it and may show a temple's first rooms as reachable earlier.
- **Boss entrance shuffle** (`shuffle_boss_entrances`, apworld 0.4.0, default off): Odolwa, Goht, Gyorg and Twinmold
  are shuffled among the four boss doors, coupled, independently of dungeon entrance shuffle. Majora and the Moon
  stay unchanged. Four rows are appended to `shuffleEntrances`; boss rows also specify `templeExit`, the containing
  temple's exterior exit. The generator validates the one-way boss entrance/warp and the exterior against the original
  region graph. Boss keys and door rules remain at the source door; boss fights, hearts, warp checks and existing
  clear events remain in their boss regions. Blue warps compose the inverse boss and temple pairings to return outside
  the temple containing the door used, including the inverted temple when it is mixed in 0.10. If neither the boss
  nor its containing temple moved, the original warp and cutscene remain intact. Otherwise the cutscene is skipped,
  its clear flags are set by boss scene, and both death/void return records are renewed at the exterior spawn, including
  the first warp through the Giants' Chamber skip (which still processes the giant and Oath check).
  Foreign boss rooms add conservative `shuffleAreaRules`: Odolwa Deku Mask + Bow; Goht Goron Mask; Gyorg Zora Mask;
  Twinmold Bow + Light Arrows + magic. The original boss combat rules still apply. Giant's Mask use follows Twinmold's
  scene/actor and therefore follows the shuffled room. The original region files and `Logic.h` are unchanged.
  Both pairings use the existing save/slot-data format and are rebuilt by Universal Tracker; 0.1–0.3 seeds/saves remain
  supported. See `docs/testing.md` for code evidence, validation and the in-game checklist. **Not tested in game.**
- **Door keys** (`shuffle_door_keys`, apworld 0.5.0, default off): 25 buildings in Termina (Clock Town shops and
  houses, Stock Pot Inn, swamp and mountain shops, ranch buildings, Marine Lab, Fisherman's Hut, Music Box House,
  Ghost Hut, Dampé's House) are locked until their key `RI_DOOR_KEY_*` ("<building> Key", item group "Door Keys") is
  found. One key per building opens all of its ways in. The table `doorKeys` in `Rando/MiscBehavior/DoorKeys.cpp`
  lists key, building and the "into" entrances; the generator checks each against the region graph and writes
  `DOOR_KEYS`, and `__init__.py` adds the key to the rule of those exits (Glitchless only; the rule stays at the
  door). In game the door actor (`En_Door`, hook `VB_OPEN_KNOB_DOOR`) stays shut and names the missing key; only the
  entrance behind the door decides, so ways out are never locked. Like `shuffleAreaRules`, the in-game check tracker
  does not know the keys. 0.1–0.4 seeds/saves remain supported. Since 0.7.0 a 26th key, the Zora Hall Rooms Key,
  opens the five Zora Hall doors (see Interiors below). **Not tested in game.**
- **Grottos** (`shuffle_grottos`, apworld 0.6.0, default off): the 26 grottos of the logic (holes and the two
  scene-exit grottos, Deku Palace bean salesman and Lone Peak Shrine) are shuffled among each other and coupled:
  through hole A into grotto X, and out of X back at A. They are appended rows of `shuffleEntrances`; the game side
  is `grottoEntrances` (`EntranceShuffle.cpp`): real entrance, grotto data and kind. A grotto is the pair of real
  entrance and grotto data (`respawn[RESPAWN_MODE_UNK_3].data`), since 15 grottos share `GROTTOS:4`/`GROTTOS:10`;
  the client sets both to X's values and keeps A's respawn point as the way back. The generator proves each shared
  pair against the chest flag and `chestGrottoMap`/`cowGrottoMap`. Hole rules (bombs, songs, day, season) stay at
  the hole; no grotto needs an area rule. With the option on, grotto 27's way back leads to its hole's region (Great
  Bay Coast cliffside), as in game; off, the logic is exactly 2S2H's. The in-game check tracker shows the grotto you
  are in, but its reachability does not know shuffled entrances (also for temples and boss rooms). The save field
  `entrancePairs` is now `u16[512]` (was `u8[32]`, loaded unchanged). 0.1–0.5 seeds/saves remain supported.
  See `docs/testing.md`. **Not tested in game.**
- **Interiors** (`shuffle_interiors`, apworld 0.7.0, default off): the knob doors from outside into 32 building doors
  (Clock Town shops and houses including both Stock Pot Inn doors, swamp, mountain and ranch buildings, Marine Lab,
  Fisherman's Hut, the five Zora Hall doors, Music Box House, Ghost Hut) are shuffled among each other and coupled:
  through door A into building X, out of X in front of A. 32 rows appended to `shuffleEntrances`, with an hours class
  each in `interiorEntrances`. The time logic gives every region the same mask, so checks inside a building rely on
  their own door's hours (e.g. the shooting galleries only test the bow, their doors close at 22); doors therefore mix
  only within a class, "always open" (17) or "daytime" (9), and the 6 doors with hours of their own (Curiosity Shop,
  Kafei's hideout, Post Office, Milk Bar, Inn ground floor, barn) stay. `test_interior_shuffle.py` proves both classes
  against the logic's door rules. A door keeps its rule, hours and key; a foreign Music Box House adds Sharp's curse,
  a foreign Evan's room Zora (`shuffleAreaRules`). The client redirects without the entrance layer bits and without
  testing the exterior scene (cleared swamp, spring village). Door keys gain the Zora Hall Rooms Key (five doors); the
  client locks those doors only for seeds of 0.7 and newer (`RANDO_INF_DOOR_KEYS_ZORA_HALL`), so 0.5/0.6 saves keep
  them open. 0.1–0.6 seeds/saves remain supported. See `docs/testing.md`. **Not tested in game.**
- **Overworld** (`shuffle_overworld`, apworld 0.8.0, default off): 34 transitions between outdoor areas (Clock Town
  districts and gates, Termina Field and its roads, swamp, Milk Road, mountains, Great Bay Coast–Zora Cape, Ikana) are
  shuffled and coupled. Through exit A you arrive at the place of exit X, and X leads back to A.
  68 rows (one per exit) appended to `shuffleEntrances`. Rows name places, not scene variants: the client redirects
  to the base entrance and the game picks spring/cleared scene and layer (`Play_Init`). One-way ways, water arrivals,
  the Stone Tower inversion, Clock Tower rooftop, owls and songs stay fixed. The four Clock Town gates always
  lead out of Clock Town. Riding Epona
  through a shuffled exit, Link gets off and Epona waits there. Area rules for Woodfall, the Deku Palace treetop, the
  Goron Racetrack (both sides of its boulder) and Milk Road past the boulder (`shuffleAreaRules`). 0.1–0.7
  seeds/saves remain supported. See `docs/testing.md`. **Not tested in game.**
- **Stage 6b** (apworld 0.9.0): the remaining two-way transitions of the logic, 23 rows appended to
  `shuffleEntrances` (rows 134–156; older row numbers stay valid).
  - `shuffle_overworld` gains the Astral Observatory (Bombers' passage from East Clock Town, now a fifth "gate", and
    the door to Termina Field) and the underwater ways into Pirates' Fortress and Zora Hall. These are listed in
    `overworldWaterEntrances` and pair only among each other, always a coast's exit with a dead end's (Zora Hall's
    door by land has no region in the logic): a land exit never leads under water. They are paired after GER; a
    crossed pairing that would lose a region reachable with all items stays vanilla. Pinnacle Rock and Waterfall Rapids
    stay fixed, a Human can reach them (Human Link dives too).
  - `shuffle_interiors` gains 9 "always open" rows: the five Great Fairy fountains, the Goron Shop, the Goron
    Graveyard, the Deku King's chamber and the monkey's cell (area rule Deku for the last two).
  - New `shuffle_special_areas` (`RO_SHUFFLE_SPECIAL_AREAS`, default off): Swamp and Oceanside Spider Houses,
    Beneath the Well and Ikana Castle (front doors; the well's back exit stays joined to the castle courtyard), Secret
    Shrine and Deku Shrine, coupled among each other. The client compares them as places (cleared swamp, layers).
    Area rule only for the Swamp Spider House (its own door's guarantee).
  - Seeds of 0.8 or older keep every 0.9 row vanilla, in the client (no saved pair) and in the Universal Tracker,
    also with the overworld or interior option on (`test_special_areas.py`, real 0.8 slot data in `test/data`).
  - Correction of the 0.8 code: with the overworld and temples or bosses shuffled, blue warps lead back into their own
    boss room while GER runs, so GER never counts a place as reached through a warp that is redirected afterwards.
  - Stage 6b left 62 two-way exits fixed. The current inventory after stage 6c is in
    [testing.md](docs/testing.md#exits-that-stay-fixed-60). **Not tested in game.**
- **apworld 0.10.0 adds mixed entrance pools.** `mix_entrance_pools` takes any two or more of `dungeons`, `interiors`,
  `overworld` and `special_areas` whose shuffle switches are on. The empty list keeps the 0.9 pairings and generation
  behavior. Inverted Stone Tower Temple joins when `dungeons` is selected. Its exit and Twinmold's blue warp use the
  coupled return point, including when the temple is reached through a shop or a land exit.
  - The [copyable YAML block](docs/example.yaml) enables every entrance shuffle and mixes all eligible pools.
    Boss rooms and grottos each keep their own pool. Water pairs only with water. Of the interiors, 24 join the
    mixed pool, nine daytime entrances shuffle within their hours class and six with their own hours stay fixed.
    Trading Post and Swordsman's School also stay fixed when interiors are mixed.
  - Clock Town's gates (including the Bombers' passage) stay coupled to land exits outside town. Milk Road and
    Romani Ranch stay connected when the overworld is mixed. The other 60 fixed exits and their reasons are in the
    [inventory in testing.md](docs/testing.md#exits-that-stay-fixed-60).
  - Universal Tracker rebuilds the coupled endpoints from optional `mixed_entrance_pairs` slot data. The client
    saves the endpoints separately from the existing row-indexed pairs, and loads 0.9 slot data and saves with
    their original pairs. The C++ test writes a save with the original 0.9 codec and loads it with 0.10.
    [Test details](docs/testing.md#stage-6c--mixed-entrance-pools-apworld-0100). **Not tested in game.**
- **Ability shuffle in apworld 0.11.0.** `shuffle_epona_riding` and `shuffle_bean_planting` are independent, default-off
  options. Each adds a progression item. Epona's ability gates mounting, carrying a mount across scenes and entering
  the Gorman race. Playing the song, calling Epona, cow checks, Romani's practice and the Night 2 track route are
  available without the ability. The bean ability gates planting. An existing plant can still be watered and ridden.
  The generated logic keeps the original item and water requirements, including the separate Day 2 rain path.
  No Logic and Nearly No Logic keep the in-game locks and generate without the access rules. The 0.11 client and
  Universal Tracker support existing 0.10 seeds with both abilities off. See [testing.md](docs/testing.md#stage-4a--abilities-apworld-0110).
  **Not tested in game.**
- **Entrance tracker in the client** (apworld 0.11.1, no new ids or options). The ImGui window *Entrance Tracker*
  (Rando menu, next to the Check Tracker) lists the shuffled entrances of the loaded save as "from -> to", built only
  from the save and the tables in `EntranceShuffle.cpp`, so it works offline. Unshuffled rows are left out. A line is
  discovered when Link goes through either side of its coupled pairing. The redirect that decides the transition
  marks both endpoints in the new save field `entrancesDiscovered`, one bit per endpoint. Blue warps, void outs,
  deaths, songs and loading a save mark nothing. Undiscovered targets show as `???` unless *Show All (Spoilers)* is on.
  Search, tooltips, grouping (by area or by pool) and sorting use only the side the line starts from. The names are
  the spoiler log's: the new table `entranceNames` holds them, the generator checks them against the formulas of
  0.11.0 and writes `ENTRANCE_NAMES`, and `_door_name`/`_place_name` read that. A test compares the spoiler entrances
  of 10 seeds with those of 0.11.0 word for word. Older saves load without discoveries.
  See [testing.md](docs/testing.md#stage-7--entrance-tracker-apworld-0111). **Not tested in game.**
- **Pool trimming** (apworld 0.7.1, no new ids or options, client unchanged): when the item pool is larger than the
  shuffled locations, `create_items` removes junk (as before), then traps, then extra Plentiful Items copies (picked
  at random; every original item stays). Each trimmed trap/copy count is logged as a warning. Only if the pool is
  still too large without all of these does it raise `OptionError`, naming the counts and the options that help.
  This path is only reached where 0.7.0 raised `OptionError`, so every seed that generated before is unchanged
  (`test/test_pool_trim.py` pinned 38 fills from 0.7.0; recomputed for 0.9.0 because those rolls have Shuffle
  Interiors on, plus 40 fills of 0.8.0 without it that 0.9.0 keeps exactly).

## Updating to a new 2S2H release

```
python tools/gen_from_2s2h.py <2s2h-checkout>          # regenerate, then review the diff and UNTRANSLATED.md
python -m pytest worlds/mm_2s2h/test                   # from the archipelago root
python worlds/mm_2s2h/tools/parity/gen_shim.py <2s2h-checkout> worlds/mm_2s2h/tools/parity/build
worlds\mm_2s2h\tools\parity\build.bat
python worlds/mm_2s2h/tools/parity/compare.py 150      # must report 0 mismatching
```

Bump `world_version` in `archipelago.json` and `AP_WORLD_VERSION` in the client together when ids, options or
slot data change.
