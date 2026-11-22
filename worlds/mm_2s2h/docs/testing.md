# Testing mm_2s2h (Phase 3 results, 2S2H 5.0.1)

Release channels (2026-11-17): **0.1.5** is the regular release (apworld **0.1.4**); **0.4.0** is the experimental
pre-release. The results below describe the relevant source versions, not additional in-game testing.
Preview packaging waits for the exact client commit's Windows/Linux/macOS builds and checks ZIP integrity,
apworld version and package contents. Its manifest records source commits and SHA-256 hashes.
[Publication progress](https://github.com/junkx420/Updated-2S2H-Archipelago/actions/workflows/publish-preview.yml).

## 1. Logic parity with 2S2H (`tools/parity/`)

The apworld's logic is compared against 2S2H's **own C++ logic**, compiled outside the game:

* `gen_shim.py` copies the unmodified `Logic/Regions/*.cpp`, the body of `Logic.h` (macros, `canPlaySong`,
  `CanAccessDungeon`, `CanKillEnemy`, time operators), `Logic.cpp` (`GetRegionIdFromEntrance`,
  `FindReachableRegions`, `RR_MAX`), `TimeLogic.cpp` and the enemy-soul map out of a 2S2H checkout. Only the save
  file reads are mocked (`shim/mock_env.h`).
* `harness_main.cpp` runs the outer loop of `ApplyGlitchlessLogicToSaveContext` for a fixed save state (regions,
  counted events, checks) and prints what is reachable. Build: `tools/parity/build.bat` (MSVC).
* `compare.py` builds the real AP world (regions, entrances, event locations, rules) for fixed option sets, rolls
  random inventories, feeds the same save state to both sides and compares reachable **regions**, **checks** and
  **event counts**.

Not covered by design: the RI item -> save state step (`SaveView` in `primitives.py`, port of `GiveItem.cpp`). Both
sides start from its output. It is covered by review against `GiveItem.cpp`/`ConvertItem.cpp` and by the in-game test.

| Option set | Scenarios | Checks in domain |
|---|---|---|
| default | 150 | 376 |
| all check types on | 150 | 2639 |
| all on + boss/enemy souls + ocarina buttons + swim | 150 | 2639 |
| all on + Clock Shuffle any order / ascending / descending | 3 × 150 | 2639 |
| all on + dungeons "form or song", Moon 2 remains / 8 masks | 150 | 2639 |
| all on + everything open, 7 fairies, 12 tokens | 150 | 2639 |

**Result: 1200 / 1200 scenarios identical** (avg. 1060 of 2356 checks reachable, inventories from empty to full).

The comparison found one real bug, now fixed: several regions declare the same return entrance (e.g.
`GREAT_BAY_COAST:0`). 2S2H resolves that by `RandoRegionId` order (last writer in `std::map` iteration), the
generator had used file order. The generator now mirrors C++ and lists every such entrance in `UNTRANSLATED.md`.

Reproduce (archipelago root, venv):

```
python worlds/mm_2s2h/tools/parity/gen_shim.py <2s2h-checkout> worlds/mm_2s2h/tools/parity/build
worlds\mm_2s2h\tools\parity\build.bat
python worlds/mm_2s2h/tools/parity/compare.py 150
```

### Check Tracker fork rules (part A)

`TrackerLogic.cpp` is called by the in-game tracker and the C++ harness. The tracker uses the NPC soul lists for
checks and events, the door key on the source exit, and the ability requirements on the original branches. Rocks
use the existing registered conditions from `ExtraChecks.inc`. The current region remains an additional starting
point. This comparison starts in South Clock Town and uses the original entrance graph.

`python worlds/mm_2s2h/tools/parity/compare_tracker.py 100` compares the open shuffled check IDs for 100 seeded
worlds and eight inventories each. The focused cases cover souls and events, door keys including the old Zora Hall
exception, each ability and its alternatives, rock hours and layouts, and obtained checks. The harness evaluates
the native loop with both ability options on and off at the same inventory. The ability header invokes the original
expressions in native evaluation because only the tracker enters an ability rule context. The 1,200-scenario parity
run compares the native loop and the tracker with the fork rules off against the apworld.

The comparison passed **800/800** inventory snapshots and **18/18** focused cases, with identical native output
for both ability settings. Parity passed **1,200/1,200**. The local pytest suite passed **861 tests** and
**159,983 subtests** after setting `TEMP` and `TMP` to a workspace directory. The first run hit
`PermissionError: [Errno 13] Permission denied` writing `host.yaml.tmp` in the sandbox's default temp directory.
The Windows x64 Release build and clang-format check passed. Initial builds hit MSVC `C1902` in the sandbox and
`C2065`/`C3861` after the interrupted dependency download left an empty `stb_image.h` in the build cache. The
successful build used the restored pinned header and ran outside the sandbox.

#### In-game checklist

- In **Out of Logic Checks**, select a mode that distinguishes out-of-logic checks. At the bank, the otherwise
  accessible Adult Wallet should be out of logic without the Banker Soul. It should be in logic after receiving
  the soul.
- With Shuffle Rocks on, check that a rock requiring the spring layout appears in the list and becomes in logic
  once the route, form and Snowhead clear requirements are satisfied.
- After part B, compare open checks in the in-game Check Tracker and Universal Tracker with the same inventory and
  shuffled entrance pairs.

These checks have **not been tested in game**.

### Check Tracker saved entrance pairs (part B)

The in-game tracker follows `entrancePairs` and `mixedEntrancePairs` from the save. The source exit retains its
condition and door key. A foreign arrival applies its `shuffleAreaRules` condition. Grotto returns lead to the
region of the paired hole. Blue boss warps compose the boss and temple pairings through the same helper used for
the actual scene transition. With AP fork rules enabled, the tracker starts at `RR_MAX`. With them off, it also
starts in the current region as before. The check list still refreshes at most once every 20 frames while its
out-of-logic display is enabled.

Run `python worlds/mm_2s2h/tools/parity/compare_tracker.py 50 --entrances` after rebuilding the parity shim. Each
seed sends the apworld's pairs through the client's real `SetPairs`. The harness compares the target of every exit
and the complete set of open shuffled checks for each inventory. The timed section includes graph construction and
check evaluation. It excludes process startup, scenario parsing and result printing.

All **100 entrance graphs** and **800 inventory snapshots** agreed, with 50 separate and 50 mixed seeds. The
100-seed run with entrance shuffle off also passed all **800 inventory snapshots**. All **19 focused cases**
passed, including a save physically inside a locked building. The shim measured a **0.552 ms**
median, **1.516 ms** p95 and **1.630 ms** maximum for separate pools, and a **0.586 ms** median, **1.488 ms** p95
and **1.709 ms** maximum for mixed pools. Parity passed **1,200/1,200**. The Windows x64 Release build and
clang-format passed. Pytest passed **861 tests** and **159,979 subtests**. The native entrance harness passed,
including **92,160** boss warps, **110** mixed boss warps and the 0.9 and 0.10 save tests.

The first four-seed smoke test reported `14 differences` for separate pools and `13 differences` for mixed pools.
The AP side had precollected starting items, so the harness now includes the same items in the mocked save.
The native entrance harness initially reported `C2061: syntax error: identifier 'RandoCheckId'` and then
`Assertion failed: slot.apworldVersion == "0.11.1"`. Its mock now includes the real check enum, and its generated
fixtures take the expected version from `archipelago.json`.

Use the in-game checklist above. For the entrance test, use the same inventory and the same seed in the in-game
Check Tracker and the Universal Tracker. Check a route through a swapped entrance and a return through a mixed
pool. **Not tested in game.**

## 2. Archipelago test suites

`pytest test/general worlds/mm_2s2h/test`: **433 passed, 0 failed** (all worlds, includes ours).
The general suite caught two issues, both fixed: Toggle options created with `type()` were not picklable, and
`tools/` lacked `__init__.py`.

## 3. Random seeds (`tools/random_seeds.py`)

200 seeds, every option randomized (check-type density varies per seed from sparse to allsanity), full output
including playthrough:

* **199 / 200 generated**, avg. 2.8 s, max 3.6 s per seed.
* The 1 failure is an intentional refusal: plentiful items + triforce hunt (40 pieces) + traps with only 468
  locations means 502 items for 468 locations. 2S2H refuses the same pool ("can't balance"). The apworld raises a
  clear `OptionError` naming the counts. Since 0.7.1 such a pool is trimmed first (traps, then extra Plentiful
  copies), see "Pool trimming" below.

### Own-dungeon items (apworld 0.1.2)

Same tool, 100 seeds with small keys, boss keys and stray fairies all on "own dungeon"
(`random_seeds.py 100 <dir> small_keys=own_dungeon boss_keys=own_dungeon stray_fairies=own_dungeon`):

* item rules only (0.1.1): 97 / 100, two real `FillError`s (dungeon locations taken by other items first)
* `pre_fill` placement (0.1.2): **99 / 100**, the remaining one is the oversized pool above.

`test/test_dungeon_items.py` checks that every confined item lands in its own dungeon and the seed is beatable.

### Logic modes (apworld 0.1.3)

Before 0.1.3, `logic: no_logic` was only written into the save; the apworld still applied every glitchless rule.
Now No Logic and Nearly No Logic drop all access rules, and Nearly No Logic adds the item blacklists of
`NearlyNoLogic.cpp`. `test/test_logic_modes.py` checks that every location is reachable with nothing collected and
that no blacklisted item lands in a blacklisted scene or on a cow. 50 random seeds per mode: **49 / 50** each, the
remaining one is the oversized pool above.

## 4. In-game (Phase 2)

Windows CI build of the client, local server, seed with pots + grass: start inventory received, Happy Mask Salesman
checks give their seed items, grass/pots/chests are reported, own items (stray fairies, Epona's Song) are given from
grass.

Two-slot multiworld (both mm_2s2h, one played, the other released with `/release`): items for the other world show
the Archipelago model and are reported, ~900 released items arrive in order, and reloading the save grants nothing
twice.

### Goal (apworld 0.1.5)

2S2H's logic has no rule for beating Majora, so until 0.1.4 the goal was in logic as soon as the lair was reachable,
with or without a weapon. The goal now needs a sword (any human sword, Great Fairy's Sword or Fierce Deity) in
Glitchless logic. `test/test_goal.py` checks both directions. 20 random seeds: **20 / 20**.

### NPC souls (apworld 0.2.0)

Not part of 2S2H: 30 NPC souls live in the client fork (`Rando/ActorBehavior/Souls.cpp`, table `npcSouls`). The
generator reads that table (item, actors, checks, events) and the world adds the soul to the rule of every listed
check and logic event. 2S2H's own logic does not know the souls, so the parity comparison runs with the option off
and still has to match.

* `test/test_npc_souls.py`: for each soul, every one of its checks is out of logic with everything but the soul
  and in logic with it (70 locations with cows, shops and Tingle maps shuffled); all 30 souls are in the pool as
  progression; the option is off by default and No Logic drops the soul rules.
* 30 random seeds with `shuffle_npc_souls=true`: **30 / 30**.
* The client builds locally (MSVC, Release). **Not tested in game yet**: whether each NPC really cannot give its
  checks without the soul, and that nothing else breaks while an NPC is missing.
* Parity harness rebuilt from the fork with the souls in it, option off: **1200 / 1200 scenarios identical**.
  `pytest test/general worlds/mm_2s2h/test`: 467 passed.

### Dungeon entrance shuffle (apworld 0.3.0)

Not part of 2S2H, so there is no reference for the option on; the parity comparison runs with it off. The table
`shuffleEntrances` (`Rando/MiscBehavior/EntranceShuffle.cpp`) is read by the generator, which stops unless every
door's way in and way out is the exit of exactly one region in the stated scene and the two are each other's way
back, and unless every boss row names a warp exit of its boss room.

* `test/test_entrance_shuffle.py`: option off: slot data has no pairing and every door, way back and boss warp is
  as generated. Option on, 12 seeds: the pairing is a permutation of the four temples, coupled (door → temple,
  temple exit → the same door), each moved boss warps out of the door used, the game is beatable with all items;
  more than one pairing occurs. The base class's default tests add: every location reachable with all items, fill
  works. Universal Tracker rebuilds the same pairing and boss warps from slot data. Own-dungeon keys and fairies
  stay in their own temple with the option on.
* 100 random seeds with `shuffle_dungeon_entrances=true` (all other options random): **100 / 100**, about 2.5 s
  per seed.
* The client builds locally (MSVC, Release). Parity harness rebuilt from the fork, option off: **1200 / 1200
  scenarios identical**. `pytest test/general worlds/mm_2s2h/test`: 487 passed.
* **Not tested in game**: walking in and out of each temple, voiding out inside, the boss warps (first and repeat),
  the Great Bay turtle ride into another temple, save and reload, and the option off.

#### Area rules (apworld 0.3.1)

`shuffleAreaRules` (`EntranceShuffle.cpp`, generated to `ENTRANCE_AREA_RULES`): a door with another temple behind
it also needs what the vanilla way to that temple's door requires (read from `South.cpp`, `West.cpp`, `East.cpp`:
Woodfall Deku Mask; Great Bay Zora Mask and Hookshot; Stone Tower Hookshot, Elegy, Goron and Zora Mask; Snowhead
none, its Goron Mask block in `SnowheadTemple.cpp` comes before every check but an enemy drop). The generator
translates each rule like a CONNECTION rule and stops on an unknown or duplicate row or an untranslatable rule.
`CanAccessDungeon` (the door's own song/form condition) stays at the door and is not part of an area rule.

* `test/test_entrance_shuffle.py`, 12 seeds: without one of the area's items a door with that temple behind it is
  closed (also where its own rule would let you through); every other door evaluates exactly as its own rule;
  Universal Tracker evaluates every door the same; with No Logic every door is open.
* 100 random seeds with `shuffle_dungeon_entrances=true`: **100 / 100**, about 2.6 s per seed.
* Client builds locally. Parity harness rebuilt, option off: **1200 / 1200**. `pytest test/general
  worlds/mm_2s2h/test`: 496 passed.
* The in-game check tracker does not know the area rules: with the option on it can show a temple's first rooms
  as reachable before the logic counts them. **Not tested in game.**

### Boss entrance shuffle (apworld 0.4.0, 2026-11-17)

`shuffle_boss_entrances` / `RO_SHUFFLE_BOSS_ENTRANCES`, default off. Four appended `shuffleEntrances` rows cover
Odolwa, Goht, Gyorg and Twinmold; existing row indices and item/check ids are unchanged. The inverted Stone Tower
boss door has its own row, its exterior is not shuffled. Majora and the Moon are unchanged. Boss keys and source-door
rules remain in the temple; boss fights, hearts, warp checks and existing `RE_CLEARED_*` stay with their boss region.
There is no upstream `RE_CLEARED_STONE_TOWER_TEMPLE`; no new logic event is invented. The game does have the
Stone Tower week-event clear flag, which is preserved below.

#### Entrances and warp composition

Client source paths below are relative to `2s2h/mm/`; the region sources and `Logic.h` were not edited.
All entrance names mean `ENTRANCE(name, spawn)`.

| Door / region source | Boss entrance | Vanilla logical warp | Exterior when redirected |
|---|---|---|---|
| `Rando/Logic/Regions/WoodfallTemple.cpp:228`, warp at 48 | `ODOLWAS_LAIR,0` | `WOODFALL_TEMPLE,1` | `WOODFALL,1` |
| `Rando/Logic/Regions/SnowheadTemple.cpp:187`, warp at 52 | `GOHTS_LAIR,0` | `MOUNTAIN_VILLAGE_SPRING,7` | `SNOWHEAD,1` |
| `Rando/Logic/Regions/GreatBayTemple.cpp:273`, warp at 57 | `GYORGS_LAIR,0` | `ZORA_CAPE,9` | `ZORA_CAPE,7` |
| `Rando/Logic/Regions/StoneTowerTemple.cpp:422`, warp at 435, exterior at 241 | `TWINMOLDS_LAIR,0` | `IKANA_CANYON,15` | `STONE_TOWER_INVERTED,1` |

Resolve the inverse boss pairing to find the containing temple, then resolve that temple's inverse entrance pairing.
The inverted exterior has no dungeon pairing. If the boss and containing temple are both fixed points, preserve the
original warp, including its original scene (especially Woodfall's prison and Twinmold's canyon). Otherwise go directly
to the resolved exterior and skip the completion cutscene. Runtime variants `WOODFALL,0` and `ZORA_CAPE,8` are also
recognized. In the AP graph the boss warp is only temporarily a reverse edge for Archipelago's coupled mixer; after
mixing, `_connect_boss_warps` restores vanilla fixed points or connects the final exterior. Universal Tracker uses the
same final composition after restoring both permutations from slot data.

#### Completion flags and first-warp evidence

These effects were read from code, not inferred from boss/temple names:

| Boss | Preserved week-event flags | Original code evidence |
|---|---|---|
| Odolwa | `CLEARED_WOODFALL_TEMPLE`, `ENTERED_WOODFALL_TEMPLE_PRISON` | `Enhancements/Cutscenes/StoryCutscenes/SkipGiantsChamber.cpp:24`, repeat skip at 149; `src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c:723` |
| Goht | `CLEARED_SNOWHEAD_TEMPLE` (spring) | `SkipGiantsChamber.cpp:30`; `src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c:738` |
| Gyorg | `CLEARED_GREAT_BAY_TEMPLE` | `SkipGiantsChamber.cpp:35`; `src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c:753`, `src/code/z_demo.c:668` |
| Twinmold | `CLEARED_STONE_TOWER_TEMPLE` | `SkipGiantsChamber.cpp:40`; repeat warp `src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c:763`; Giants' Chamber destination `src/code/z_demo.c:676` |

The prefixes in the flag column are `WEEKEVENTREG_`. `EntranceShuffle.cpp:99` (`MarkCleared`) applies those flags by
the actual boss scene for redirected repeat warps. Thus Goht brings spring even behind another temple's door.

For the **first warp**, `Rando/ActorBehavior/DoorWarp1.cpp:51` selects the actual boss scene's warp check, line 63
sets its `eligible`, and line 74 calls `HandleGiantsCutsceneSkip`. That function sets the four bosses' clear flags
(`SkipGiantsChamber.cpp:20`) and queues the destination tagged with the actual boss scene at line 51. The entrance
hook changes only that queued destination/cutscene (`Rando/MiscBehavior/EntranceShuffle.cpp:192`), leaving the
Giants' Chamber transition and boss tag intact. The skip consumes the event, updates `save.entrance`, and calls
`handleGiantsCheck` (`SkipGiantsChamber.cpp:142`). That function records the correct freed giant at lines 65–90,
increments its count at 94, and marks `RC_GIANTS_CHAMBER_OATH_TO_ORDER` eligible at 100–101 if needed.
The check queue grants eligible checks and sets `cycleObtained`/`obtained`
(`Rando/MiscBehavior/CheckQueue.cpp:98`); `Network/Archipelago/ArchipelagoGameplay.cpp:149` reports obtained checks.
Neither the warp check nor Oath depends on visiting the skipped completion scene.

#### Respawns and Giant's Mask

`src/code/z_play.c:2050` saves the current entrance, room, position and temporary scene flags for a respawn.
Normal `Player_Init` calls `func_801226E0` (`src/overlays/actors/ovl_player_actor/z_player.c:11460`), which saves
DOWN (`src/code/z_player_lib.c:93`), then copies DOWN to TOP for a normal arrival (`z_player.c:11466`). The first
warp destination is already replaced before spawning actors (`src/code/z_play.c:2365`, then scene spawn at 2384).
Additionally, a redirected first or repeat warp clears the old respawn mode and marks its exterior target pending;
`EntranceShuffle.cpp:123` renews DOWN from the **new exterior spawn** and copies it to TOP after scene initialization.
This includes position/room/temporary flags, not just the entrance number.
`Play_SetRespawnData` normalizes the spawn index to zero (`z_play.c:2040`), so the hook explicitly restores the
resolved exterior entrance at `EntranceShuffle.cpp:133` before copying DOWN to TOP.
Void-out reads DOWN at `z_play.c:2073`; death reads TOP at 2083. Result from code and mocked-client checks:
redirected warps leave both return records at
the exterior, not at the boss or its home area. Death/void inside a boss room does not get shuffled again, because
entry redirection also requires the source temple scene.

Giant's Mask availability checks `SCENE_INISIE_BS` (`src/code/z_parameter.c:3389`, D-pad at 3536). Its transformation
is handled by the Twinmold actor; the boss-soul restriction also checks that scene
(`Rando/ActorBehavior/Souls.cpp:282`). Both travel with the shuffled boss room; no mask restriction change is needed.
The actual transformation handler is `src/overlays/actors/ovl_Boss_02/z_boss_02.c:1688`, called at line 1364.

#### Supplemental rules

Only a foreign boss room adds its item-only `shuffleAreaRules` condition. Existing fight/soul rules still apply;
source keys/events are not imported from the boss's original temple.

| Foreign room | Extra items | Source / reason |
|---|---|---|
| Odolwa | Deku Mask + Bow | Original boss-door rule, `WoodfallTemple.cpp:228`; conservative preservation of guaranteed items |
| Goht | Goron Mask | Temple block `SnowheadTemple.cpp:260`; combat rule `Logic.h:637` tests Fire Arrows but not Goron |
| Gyorg | Zora Mask | Original approach `GreatBayTemple.cpp:276` / `West.cpp`; `Logic.h:631` also allows Fierce Deity |
| Twinmold | Bow + Light Arrows + magic | Original inverted approach `East.cpp:545`; `Logic.h:627` otherwise accepts Bow alone or Giant's Mask + magic + human sword |

#### Verification and known failed attempts

* `pytest test/general worlds/mm_2s2h/test -q`: **531 passed**. Final full run used `TEMP` and `TMP` set to
  `C:\Users\apexios\Documents\2S2H-AP\.tmp-tests`, outside both Git checkouts; the temporary directory was removed
  afterwards. Existing upstream deprecation warnings remain (1,376,756 warnings reported).
* Tests cover disabled options, per-group coupled permutations over 12 seeds, correct composed warp targets,
  all locations reachable with all items, source-only key rules plus foreign-only supplemental rules, both options
  separately/together, No Logic, own-dungeon items, identical Universal Tracker pairings/warps/rules, and old
  0.1/0.2/0.3 slot data. A dedicated boss-only test observes a fixed point for each boss and checks its vanilla warp.
* 100 random seeds with `shuffle_boss_entrances=true shuffle_dungeon_entrances=false`: **100/100**, 371 s.
* 100 with both options true: **100/100**, 371 s. Other options randomized; no failed seeds in either series.
* Parity harness regenerated and rebuilt: both entrance options off, **1200/1200 identical**.
* `python worlds/mm_2s2h/tools/entrance_client/run.py ../2s2h` compiles the **real** `EntranceShuffle.cpp` with
  MSVC and mocked game state/API boundaries: **2304 option/permutation cases, 18432 entries, 23040 first/repeat
  warps, 44352 respawn checks passed**. It covers all 24 × 24 stored permutations under each option combination,
  source-scene guards, disabled options, fixed points, completion flags on repeat warps, first-warp event destination
  and retained boss tag, post-arrival return points, and legacy row storage. Area-rule evaluation is covered by AP
  tests, not by these mocks. This does not execute the full scene loader, actor/cutscene queue or game.
* Generator reviewed: no region/combat/macro changes; generated region diff only extends `ENTRANCE_AREA_RULES`.
  Option enum appended, existing 32-byte pairing storage retained, tolerant loading stays in
  `2s2h/BenJsonConversions.hpp:90`; client accepts 0.1, 0.2, 0.3 and 0.4. Changed client files pass clang-format.
* Initial full test: `1 failed, 524 passed`, `TestSettingsSave.test_save`,
  `PermissionError: [Errno 13] ... host.yaml.tmp` in the sandbox temporary directory. User approved moving TEMP/TMP.
  Next full run found the new tool package lacked `__init__.py` (`1 failed, 525 passed`); corrected.
  Two initial area-rule tests failed with `True is not false (..., 'RI_BOW')`: the test inventory still held
  Progressive Bow; both bow forms are now removed. The native harness initially needed mock macros, a corrected
  `/Fo` output path, and a fixture reset that preserves the stored pairing; those fixture errors are corrected.
* Client builds locally with the standard command `cmake --build build/x64 --config Release --target 2ship
  --parallel 12` (MSVC, Release), result `2s2h/x64/Release/2ship.exe`. Earlier sandbox attempts hit
  `C1902: Program database manager mismatch`; changing debug format then hit `C2855: '/Z7' inconsistent with
  precompiled header`, followed by `LNK1103: debugging information corrupt` / `LNK1318: Unexpected PDB error`.
  The generated target objects/PDB were removed and the standard Release build succeeded outside the sandbox.
  After the final spawn-index correction the same standard command succeeded again. No build-setting changes
  are committed; existing upstream compiler warnings remain.

#### In-game checklist — not tested in game

1. Test boss-only and combined shuffles: enter all four boss doors, including the inverted Stone Tower door;
   verify the source temple's boss key, its fairy checks and the actual boss/heart/warp rewards.
2. Die and void in every shuffled boss room; confirm the same assigned room reloads without a second shuffle.
3. Take the first blue warp: correct exterior, warp check and Oath check reported, correct giant and clear effect
   (especially Goht -> spring, Twinmold -> Stone Tower clear). Repeat after a cycle reset and after an already-used warp.
4. Immediately after each redirected warp die/void: both return to the exterior spawn, not the boss/home area.
5. Test bosses behind their own doors, with and without their containing temple moved: preserve vanilla scenes only
   if neither moved. Include Odolwa's prison and Twinmold's canyon. Test first/repeat warps and cancellation of the
   two permutations (same final outside area still skips the completion scene if either pairing moved).
6. Use Giant's Mask in Twinmold's room behind a foreign door; verify it is unavailable in another boss room inside
   Stone Tower. With shuffled boss souls, verify the existing Twinmold-soul restriction still prevents its use.
7. Save/reload before connecting, reconnect, Song of Time, boss shortcut pads, then test with both options off;
   also load existing 0.1/0.2/0.3 seeds/saves. In-game check tracker still does not evaluate supplemental area rules.

### Door keys (apworld 0.5.0, 2026-11-17)

Not part of 2S2H. `shuffle_door_keys` (`RO_SHUFFLE_DOOR_KEYS`, default off) locks 25 buildings in Termina until their
key `RI_DOOR_KEY_*` ("<building> Key") is found; one key opens every way in of its building (Curiosity Shop and
Stock Pot Inn have two). Table `doorKeys` in `Rando/MiscBehavior/DoorKeys.cpp` (key, building, "into" entrances),
generated to `DOOR_KEYS`; the generator stops unless every "into" is the exit of exactly one region, leads into
another scene, and the key's name, enum order and `RANDO_INF_OBTAINED_DOOR_KEY_*` flag match. 123 checks and 17
logic events lie behind the 27 ways in. Item group "Door Keys"; model and icon are the small key.

Chosen from the logic graph (exits into an interior with checks or events). Not included: temples, boss rooms,
grottos, spider houses, pirates' fortress, Ikana Castle, the well, the Moon, the Clock Tower interior, Sakon's
Hideout, Secret Shrine; Astral Observatory (the Clock Town way in is the Bombers' tunnel, no door actor), Romani's
Ranch House (no checks or events), Great Fairy Fountains, Goron Shrine, Zora Hall, Deku Shrine (no knob door),
the rooms inside Zora Hall / Deku Palace / Goron Shrine (not entered from outside), outdoor areas.

#### How the client locks a door

* All 27 ways in are knob doors (`En_Door`). 21 of them have a door schedule (`EnDoorScheduleType` in
  `z_en_door.h`), which proves the door actor; Deku Playground, Tourist Information, Mountain Smithy, Marine Lab,
  Fisherman's Hut and Ghost Hut are door actors by game knowledge only (no scene data in the repository).
* New hook `VB_OPEN_KNOB_DOOR` in `EnDoor_Idle` (`mm/src/overlays/actors/ovl_En_Door/z_en_door.c:736`), where the
  original handles locked doors: without the key the door becomes a talking door, does not open and no transition
  starts. `OnOpenText` replaces text 0x1800 by "It's locked. You need the <building> Key."
  (`Rando/ActorBehavior/EnDoor.cpp`).
* The door does not know its destination; Player walks onto a scene-exit floor behind it. `EntranceBehindDoor`
  (`EnDoor.cpp`) casts down on the far side (20-160 units) and reads `setupExitList`, i.e. the vanilla entrance
  before any entrance shuffle redirect.
* Only that entrance decides: `DoorKeys::IsEntranceLocked` (`DoorKeys.cpp:61`) = option on, entrance (without
  scene layer bits, `DoorKeys.cpp:53`) is an "into" of `doorKeys`, key flag missing. Scene, side and door kind are
  not inputs. A door out of a building leads to an outside entrance, which is in no "into" list, so it always opens
  (also after respawn or a later interior shuffle).

#### Logic

Each "into" exit gets "has key" in addition to its rule (`_with_door_key` in `__init__.py`), only in Glitchless
logic. The rule stays on the exit at the door's side, so with shuffled entrances the key of the door applies.
Logic only for the in-game check tracker: like `shuffleAreaRules`, it does not know the keys and may show checks
inside a locked building as reachable.

#### Verification

* `test/test_door_keys.py`: option off = no keys in the pool and every exit rule as generated (empty and full
  inventory); option on = all 25 keys in the pool as progression, every way in closed without its key (door region
  reachable) and open with it, building regions unreachable without the key, no other exit needs a key (all ways
  out included, at least 27 checked); the same together with both entrance options; No Logic drops the rules;
  Universal Tracker rebuilds the same options, pairing and reachability without keys / without single keys.
* `pytest test/general worlds/mm_2s2h/test -q`: **565 passed, 0 failed** (137,008 subtests).
* 100 random seeds with `shuffle_door_keys=true` and both entrance options false: **100/100**, 496 s; 100 with
  all three true: **100/100**, 502 s. Other options randomized.
* First test run: `DoorKeysOff.test_rules_unchanged` failed on `RR_MOON_DEKU_TRIAL THE_MOON:0` (the test compared a
  duplicate exit key with the wrong rule; test fixed), then `test_no_other_exit_needs_a_key` failed on
  `RR_IKANA_CANYON_LOWER SAKONS_HIDEOUT:0`: its rule needs meeting Kafei, an event inside the Curiosity Shop, not a
  key. The test now collects all events, so only a key rule on the exit itself would count.
* C++ test bench (`tools/entrance_client/run.py ../2s2h`) compiles the real `DoorKeys.cpp`: **1,780,704 cases
  passed** — every 16-bit entrance value with the option off, then on with no key and with each key alone; locked
  exactly for the 27 listed ways in whose key is missing, whatever the scene layer; the table equals the generated
  `DOOR_KEYS`; the 27 ways out taken from the logic graph are never locked, with any layer and without keys.
  It does not run `EntranceBehindDoor` or the door actor (no collision data without the game).
* Parity harness rebuilt, all own options off: **1200/1200 identical** (0 mismatching).
* Client builds locally (MSVC Release, CMake reconfigured for the new files). Changed C++ files pass clang-format
  (`SaveEditor.h` keeps its 5 pre-existing upstream differences).

#### In-game checklist — not tested in game

1. With the option on and no keys: each of the 27 doors says "It's locked. You need the ... Key." and does not open.
   Check especially the six doors without a schedule (Deku Playground, Tourist Information, Mountain Smithy,
   Marine Lab, Fisherman's Hut, Ghost Hut): **check that the door really is locked**.
2. Give a key (Save Editor, Rando flags `RANDO_INF_OBTAINED_DOOR_KEY_*`, or receive it): the door opens and leads
   into the right building; Stock Pot Inn (both doors) and Curiosity Shop (both doors) open with one key.
3. Inside every building: leave through each door, die and void, reload the save inside: the way out is never locked.
4. Doors with schedules (night closing, Milk Bar membership, Music Box House) behave as before once the key is there.
5. The key shows the small key model and icon, name "<building> Key"; other worlds' keys show like other items.
6. Option off and 0.1–0.4 seeds/saves: every door behaves as before.

### Grotto shuffle (apworld 0.6.0, 2026-11-17)

`shuffle_grottos` (`RO_SHUFFLE_GROTTOS`, default off) shuffles the 26 grottos of the logic among each other, coupled.
26 rows appended to `shuffleEntrances` (logic entrances, including the virtual `GROTTOS:17..31` of `Logic.cpp`) and
the game side in `grottoEntrances` (`Rando/MiscBehavior/EntranceShuffle.cpp`).

#### Code evidence

* A grotto is the pair (real entrance, grotto data). 13 chest grottos share `GROTTOS:4`, two cow grottos
  `GROTTOS:10` (`Logic.cpp` GetRegionIdFromEntrance). What lies inside is decided by
  `respawn[RESPAWN_MODE_UNK_3].data`, written by the hole (`z_door_ana.c:150`), never by the place outside:
  chests in room 4 by `z_en_torch.c:32-36` (chest flag = data, `Checks.cpp` scene flags, e.g. 0x1b for 0x3B), grass
  in room 4 by `ObjGrass.cpp` chestGrottoMap (`:286`, `:545`), cow grotto grass/cows/beehive/butterflies by
  `ObjGrass.cpp:636`, `EnCow.cpp:32`, `ObjComb.cpp:79,120`, `EnButte.cpp:139` (data 31 = Termina Field). All other
  grotto checks are keyed by (SCENE_KAKUSIANA, room, index), and the room follows from the real entrance. No check
  reads `respawn[UNK_3].entrance` or `respawn[DOWN]` (only the tracker did).
* So the client sets both entrance and data to grotto X's values and leaves A's respawn point: holes store it with
  `Play_SetupRespawnPoint(UNK_3)` (`z_door_ana.c:144`, `z_play.c:2050`), and the grotto exit 0xFFFF returns there
  with `respawnFlag = 4` (`z_player.c:6496`, spawn at the stored position `z_player.c:11437`).
* Grottos 12 (Deku Palace) and 16 (Lone Peak Shrine) are scene exits without that respawn point
  (`BetterMapSelect.c` `UNUSED_GROTTO_RESPAWN`; their way back is a real exit, `DEKU_PALACE:9`,
  `GORON_VILLAGE_WINTER:3`). Into such a grotto from a hole, its exit is replaced by the hole's respawn; from such
  an exit into a hole grotto, the way out is replaced by the exit's `outOf` (and the hole respawn is set as fallback).
* The grotto data per hole come from `BetterMapSelect.c` and agree with chestGrottoMap, the chest flags and
  `Logic.cpp:53`; the generator checks every shared pair (`check_grottos`) and aborts otherwise.
* The hole is recognised by the real entrance (and data where shared) outside a grotto, not by the scene: spring and
  winter variants (`Play_Init` converts winter/poisoned entrances, `z_play.c:2261-2283`) need no extra case, and a
  death or void reloading a grotto is never redirected twice.
* Death and void out in a grotto use `respawn[TOP]`/`[DOWN]` (`z_play.c:2073, 2083`), which are not updated inside
  grottos (`z_play.c:2053`): the player lands in the scene of hole A, never in another grotto. Pause saves keep
  `save.entrance` and all respawn records (`SavingEnhancements.cpp:46-48, 164-166`), so a reload stays in grotto X
  with the way back to A.
* Logic: every way back out of a grotto is `true`; hole rules stay at the hole. No grotto needs a `shuffleAreaRules`
  row (hot spring water is a global event, also from Beneath the Well; the scrub event is set in Clock Town).
  Option on: a grotto returns to the region of the hole leading to it; this differs from 2S2H only for grotto 27
  (`GREAT_BAY_COAST:0` resolves to `RR_GREAT_BAY_COAST`, UNTRANSLATED.md; the hole is on the cliffside). Option off:
  unchanged.
* In-game check tracker: with the option on, `Logic.cpp` asks `EntranceShuffle::LogicGrottoEntrance` (real entrance +
  data), so it shows the grotto the player is in. Its reachability does not know any shuffled entrance (also not
  temples and boss rooms).
* Save: `RandoSaveInfo::entrancePairs` is `u16[512]` (was `u8[32]`); same encoding (0 = own area, else row + 1).
  Old saves load through `GetGrownArray` unchanged. The generator aborts if the rows do not fit the field.

#### Verification

* `pytest test/general worlds/mm_2s2h/test -q`: **600 passed, 0 failed** (139,514 subtests), 229 s.
* New `test/test_grotto_shuffle.py`: table (26 rows, 15 distinct shared pairs, virtual entrances only for shared
  grottos, 12/16 scene exits); option off = graph exactly as generated, grotto 27 back to Great Bay Coast; option on
  = permutation of the 26 grottos only, coupled, every location reachable with all items, beatable, Universal Tracker
  rebuilds the pairing, hole rules identical to option off; No Logic; together with temples, bosses, door keys and
  NPC souls (pairing, Universal Tracker, No Logic, own-dungeon items). Legacy slot data of 0.5 rebuilds.
* 100 random seeds with `shuffle_grottos=true`: **100/100**, 460 s. Other options randomized.
* 100 random seeds with grottos, temples, bosses, door keys and NPC souls: **98/100**, 387 s. Both failures (seeds 42
  and 82) are the intended `OptionError` in `create_items` for a sparse option roll: 502 items for 433 locations and
  456 for 454 (door keys and NPC souls add 55 items). It is raised before entrances are shuffled; grottos add no
  items.
* C++ test bench (`tools/entrance_client/run.py ../2s2h`) compiles the real `EntranceShuffle.cpp` and
  `GetGrownArray`: **571,328 grotto cases passed, all 676 hole/grotto pairings covered** (26 cyclic shifts and 200
  random permutations, option on and off, each with temples/bosses off and on, hole in two scene variants): into
  X = X's real entrance and data, A's way back kept; tracker identity = X; death and void out land in A's scene;
  a reloaded grotto is not redirected; pause save/reload; way out back to A for all four hole/grotto kinds; unknown
  data stays vanilla; temples and bosses unaffected; the compiled table equals the generated one. Legacy save:
  32 u8 pairs load into u16[512] and the 8 temple/boss doors resolve as before; new format round trip. The older
  checks still pass (1,780,704 door-key cases; 4608 option/permutation cases, 36,864 entries, 46,080 warps, 88,704
  respawn checks, now also with grottos on). It does not run DoorAna, Player or the scene loader.
* Parity harness rebuilt, all own options off: **1200/1200 identical** (0 mismatching).
* Client builds locally (MSVC Release). Changed C++ files pass clang-format (`z64save.h` keeps pre-existing
  upstream differences outside the changed line).

#### In-game checklist — not tested in game

1. Option on: fall through several holes of each kind (visible, bomb-hidden, Woods of Mystery day 2, Mountain Village
   spring, Path to Snowhead, Ikana) and through the Deku Palace and Lone Peak Shrine ways in; the grotto is the one
   in the spoiler, and leaving puts you back at the hole you used.
2. Chest grottos and cow grottos behind foreign holes: chest, grass, cows, beehive and butterflies give the checks of
   the grotto you are in (compare with the spoiler/tracker).
3. Deku Palace hole into a chest grotto and back; a chest grotto hole into Deku Palace's grotto and Lone Peak Shrine
   and back (the two scene-exit cases).
4. Hole in spring/winter variants (Mountain Village, Path to Goron Village) and cleared swamp: the way back lands in
   the right variant.
5. In a shuffled grotto: void out, die, pause save and reload; never stuck in another grotto.
6. Check tracker shows the grotto you are in.
7. Option off and 0.1–0.5 seeds/saves: grottos as before; an old save with temple/boss pairs keeps them.

### Interior shuffle (apworld 0.7.0, 2026-11-17)

`shuffle_interiors` (`RO_SHUFFLE_INTERIORS`, default off) shuffles the knob doors from outside into buildings among
each other, coupled: through door A into building X, out of X back in front of A. 32 rows are appended to
`shuffleEntrances`; their hours class is in `interiorEntrances` (`Rando/MiscBehavior/EntranceShuffle.cpp`).

#### Pool and criteria

* A row is a knob door (`En_Door`) from outside into a scene of its own that is a dead end: every way out of that
  scene to another scene is the way back of one of its rows, and no row's door stands inside another interior. The
  generator checks this (`check_interiors`) plus the usual "exit of exactly one region, each other's way back".
* Clock Town: Trading Post, Bomb Shop, Curiosity Shop (front `CURIOSITY_SHOP:0`, Kafei's hideout `:1` from the
  Laundry Pool), Swordsman's School, Post Office, Lottery, Honey & Darling, Milk Bar, Mayor's Residence, Town Shooting
  Gallery, Treasure Chest Shop, Stock Pot Inn (ground floor `:0` and upstairs `:1`, one region `RR_INN`, told apart by
  their ways back `EAST_CLOCK_TOWN:9/10`), Deku Playground. Swamp: Tourist Information, Potion Shop, Swamp Shooting
  Gallery. Mountain: Smithy. Ranch: barn `RANCH_HOUSE:0`, main house `:1` (one scene, two regions), Cucco Shack,
  Doggy Racetrack. Great Bay: Marine Lab, Fisherman's Hut, Zora Hall shop and the rooms of Lulu, Evan, Japas and Mikau
  (`ZORA_HALL_ROOMS:0-3,5`, `En_Door` with `DOOR_DL_ZORA_HALL`, `z_en_door.c`). Ikana: Music Box House, Ghost Hut.
* Not in the pool: spider houses, Pirates' Fortress, Beneath the Well, Ikana Castle, Secret Shrine, Clock Tower,
  Moon, Sakon's Hideout (as decided); Dampé's House (its door is always locked, `sDoorSch_DampesHouse`; the logic's
  way in is a one-way hole, `East.cpp:256`); Astral Observatory (a passage from Clock Town to Termina Field, the
  Bombers' way in is no door); Goron Shrine and Goron Shop (no knob door, the shop lies inside the shrine).

#### Time of day: hours classes

* The time logic gives every region the same mask (`docs/time-logic.md`): a door's time condition only says that it
  opens at some time, it never narrows the time inside. Checks inside a building therefore silently rely on the hours
  of its own door. Examples: the Town Shooting Gallery checks only test the bow (`Central.cpp:460`), its door closes
  at 22 (`Central.cpp:117`, `sDoorSch_TownShootingGallery`); the Swamp Shooting Gallery the same (`South.cpp:506`,
  `:302`); the Treasure Chest Shop check says "before night 3, 18:00", the 22:00 closing of nights 1 and 2 is only at
  the door (`Central.cpp:113`, `:499`); Cucco Shack, Doggy Racetrack and the ranch house close at 20
  (`MilkRoad.cpp:278-280`). Behind the Curiosity Shop's door (22-5, `Central.cpp:261`) the gallery would be in logic
  but never open.
* So doors only mix within one class (`interiorEntrances`): **always open** (17: Trading Post, Bomb Shop, Swordsman's
  School, Inn upstairs, Deku Playground, Tourist Information, Potion Shop, Smithy, Marine Lab, Fisherman's Hut, the
  five Zora Hall doors, Music Box House, Ghost Hut), **daytime** (9: Lottery, Honey & Darling, Mayor's Residence, both
  shooting galleries, Treasure Chest Shop, ranch house, Cucco Shack, Doggy Racetrack) and **own hours** (6, never
  moved: Curiosity Shop front (nights), Kafei's hideout (letter events and hours), Post Office (events), Milk Bar
  (nights with Romani's Mask), Inn ground floor (Room Key), barn). A permutation in which every door covers the hours
  of the building behind it exists only for equal hours, hence classes rather than single pairs.
* `test_interior_shuffle.py::InteriorHoursClasses` proves both classes against the logic's own door rules: the hours
  of a door are the time slices at which its rule holds with every item; for every door A and building X of one class,
  every check, event, connection and exit c inside X and every half-day, if c can hold during X's own door hours it
  can also hold while A is open (289 + 81 pairs). The Trading Post (locked 21-22) and Swordsman's School (locked
  night 3, 23-24) are "always open" in the logic, and no check depends on those gaps. Limit: the proof takes every
  item as owned, so doors whose hours depend on items or events (Milk Bar, Inn ground floor, Post Office, Kafei's
  hideout) stay in "own hours". The barn would also pass for "daytime"; it stays fixed as planned.

#### Code evidence

* Redirect: `RedirectInterior` in `OnBeforeTransitionSetup` (hook at the start of `TRANS_MODE_SETUP`, `z_play.c`),
  after the grottos and before temples/bosses (which now skip interior rows). Into door A from any scene but A's own
  building: the base entrance of X. Out of X (from X's scene, X's way back): A's way back. Compared without the
  entrance layer bits 0-3 (variants of one entrance, e.g. the Milk Bar's show layers, `z_scene_table.c:359`); a
  building always loads its base entrance, and a cutscene layer (`nextCutsceneIndex >= 0xFFF0`) chosen for A's
  building is reset. The exterior scene is not tested: Play_Init converts the scene of a poisoned/winter entrance into
  the cleared/spring scene (`z_play.c:2261-2283`), the entrance stays.
* Void out and death inside X respawn at X's entrance from X's own scene, which is never redirected again; an exit
  that recorded itself as void-out point (`func_808354A4`, `z_player.c`) is moved along. Song of Time and Soaring
  lead to Clock Town and the owl statues, no table entrance. Pause saves keep `save.entrance` and the pairing is in the
  save, so a reload stays in X with the way back to A. Ways out are never key-locked.
* NPCs with schedules (Anju, Kafei, the postman) never cause a scene transition: schedule actors only open doors in
  their scene (`EnDoor_OpenScheduleActor`), and the logic's events belong to regions (`RR_INN`; Post Office and
  Kafei's hideout are fixed anyway).
* Door keys: the client tests the entrance of `setupExitList` behind the door, before any redirect
  (`EnDoor.cpp` EntranceBehindDoor, `DoorKeys::IsEntranceLocked`); the apworld keeps the key rule on the door's exit.
  The key of door A opens A, whatever lies behind it. New key `RI_DOOR_KEY_ZORA_HALL_ROOMS` ("Zora Hall Rooms Key")
  for the five Zora Hall doors. Seeds before 0.7 have no such key in their pool: the client sets
  `RANDO_INF_DOOR_KEYS_ZORA_HALL` only for apworld 0.7 and newer (`ApplySlotOptions`); without it those doors never
  lock, and Universal Tracker leaves the key out for such slot data.
* Area rules (`shuffleAreaRules`, only for a foreign building): Music Box House needs Sharp's curse broken (its own
  door needs it, `East.cpp:182`; the father's check only tests the song, `East.cpp:292`); Evan's room needs Zora (the
  band room doors need Zora, `West.cpp:416`; Evan's check does not say so; cautious).
* Spoiler names: interiors by their region, the Inn's two doors with their way back.
* In-game check tracker: it finds the region from `save.entrance`, which is X's own entrance, so it shows the building
  you are in; its reachability does not know shuffled entrances (as for temples, bosses, grottos).

#### Verification

* `pytest test/general worlds/mm_2s2h/test -q`: **650 passed, 0 failed** (143,528 subtests), 305 s.
* New `test/test_interior_shuffle.py` (50 tests): table (32 rows, classes 17/9/6, the Inn's two doors, Zora key with
  five doors, rows appended after all older rows); hours classes proof; option off = graph exactly as generated;
  option on = coupled permutation within each class, OWN_HOURS fixed, the Inn's doors come back through their own way
  back, every location reachable with all items, beatable, Universal Tracker rebuilds it; area rules with forced
  pairings; door keys stay at the door with both options on (key of A needed, key of X not, ways out never locked);
  No Logic; all own options together (also No Logic and own-dungeon items); 0.6 slot data rebuilds unchanged and
  without a key rule on the Zora Hall doors. `test_entrance_shuffle.py` and `test_door_keys.py` adjusted for the new
  rows.
* 100 random seeds with only `shuffle_interiors=true` (temples, bosses, grottos, door keys forced off, everything
  else randomized): **100/100**, 542 s.
* 100 random seeds with interiors, temples, bosses, grottos, door keys and NPC souls: **95/100**, 561 s. All five
  failures (seeds 24, 34, 58, 66, 71) are the intended `OptionError` in `create_items` for a sparse option roll with
  Plentiful Items: 391 items for 284 locations, 310/307, 479/466, 442/423, 398/381. It is raised before entrances are
  shuffled; interiors add no items, door keys one more (the Zora Hall Rooms Key).
* C++ test bench (`tools/entrance_client/run.py ../2s2h`), real `EntranceShuffle.cpp` and `DoorKeys.cpp`:
  **193,700 interior cases passed, all 376 door/building pairings** (17² + 9² + 6 fixed; 17 cyclic shifts, each with
  temples, bosses and grottos off and on): into A = X's base entrance for all 16 layer bits and in the cleared swamp
  and spring village, the void-out point moves along, a cutscene layer is reset; way out of X = A's way back; void
  out/death in X not redirected; Song of Time and the owl entrances (11 in all) from inside X unchanged; option off
  ignores saved pairs; cross-class and OWN_HOURS pairs from slot data are refused; the compiled table equals the
  generated one. Door keys: 3,632,128 cases (every entrance, each key, with and without the Zora flag). Older checks
  still pass with interiors on as well (9216 option/permutation cases, 73,728 entries, 92,160 warps, 177,408 respawn
  checks; 571,328 grotto cases). It does not run En_Door, Player or the scene loader.
* Parity harness rebuilt, all own options off: **1200/1200 identical** (0 mismatching).
* Client builds locally (MSVC Release). Changed C++ lines pass clang-format (SaveEditor.h keeps pre-existing upstream
  differences outside the changed lines).

#### In-game checklist — not tested in game

1. Option on: go through every door of both classes once (Clock Town, swamp, mountain, ranch, Great Bay, Ikana); the
   building is the one in the spoiler, and leaving puts you in front of the door you used.
2. Stock Pot Inn: in through a foreign door that leads to the upstairs door, out again; the ground floor door stays
   vanilla. Zora Hall: all five room doors.
3. Cleared swamp and spring Mountain Village: the Tourist Information, Potion Shop and Smithy doors still redirect,
   and the way back lands in the right variant.
4. Inside a foreign building: void out, die, pause save and reload, Song of Time, Song of Soaring; never stuck.
5. Entering a building at a time its own door would be closed (e.g. the Trading Post at 21:30 through an always-open
   door, a gallery after 22 through a daytime door that is still open): what the NPCs do, whether the inner door is
   locked; the logic never requires such a visit.
6. Closing-time kick-outs (Trading Post 21:00, galleries 22:00, ranch 20:00) inside a foreign building: you land in
   front of the door you used (if the game uses another entrance, at the building's own door).
7. Entry cutscenes and first visits (Mayor's Residence, Inn upstairs at night) play as in vanilla for that building.
8. Anju/Kafei chain, postman and Romani/aliens with the option on.
9. Door keys on: a locked door names its own key, not the building behind it; the Zora Hall doors need the Zora Hall
   Rooms Key; a 0.5/0.6 save with door keys keeps the Zora Hall doors open.
10. Option off and 0.1–0.6 seeds/saves: doors as before.

### Pool trimming (apworld 0.7.1, 2026-11-17)

Plentiful Items doubles every key, mask and major item. In sparse option rolls that made more items than locations,
and `create_items` raised `OptionError` after removing only junk (5 of 100 random seeds with all own options). Since
0.7.1 it trims in this order: junk (unchanged), traps, then the extra Plentiful copies (chosen with the world's
random). Every original item stays. Trimmed traps and copies are logged as warnings (`logging.warning`, with the
trimmed copies by name). Only if the pool is still too large after that does it raise `OptionError`, which now names
the counts and the options that help. No ids, options or logic change; the client is unchanged (it accepts `0.7`).

#### Verification

* `pytest test/general worlds/mm_2s2h/test -q`: **654 passed, 0 failed** (143,574 subtests), 174 s.
* New `test/test_pool_trim.py`:
  * Unchanged seeds: fingerprints (every location's item, starting items, slot data incl. entrance pairs, without the world version) of 38
    option rolls 0–39 of `random_seeds.py` with all own options, computed with 0.7.0 before the change. 0.7.1 gives
    the same fingerprint for **38/38**. Rolls 24 and 34 raised `OptionError` in 0.7.0 and are left out.
  * Crowded pool (default checks, Plentiful, 30 traps, NPC souls, door keys): 442 items for 284 locations; trims
    30 of 30 traps and 128 of 143 copies, two warnings, no trap left, every item of the same pool without Plentiful
    and traps still present, exactly 15 copies extra, beatable after fill.
  * Same without Plentiful: only traps are trimmed (15 of 30).
  * Too full without Plentiful (souls, clock, ocarina buttons, 1 heart, traps): `OptionError` with "339 items for 284
    locations, even without junk, traps ...".
* Spoiler comparison with `Generate.py --spoiler 3`, option rolls 0–29 with all own options, 0.7.0 (via
  `git stash`) against 0.7.1: settings, entrances, every location and the start inventory have the same content for
  **29/29** (roll 24 failed in 0.7.0, generates in 0.7.1). With `PYTHONHASHSEED=0` on both sides even the text before
  the playthrough is byte-identical for 29/29. Differences that remain are only the order of the start-inventory
  lines and the playthrough/paths section; 0.7.0 against itself (two runs, same seed, same hash seed) shows the same
  kind of differences, so they are already nondeterministic in 0.7.0 and not caused by this change.
* 100 random seeds with interiors, temples, bosses, grottos, door keys and NPC souls: **100/100**, 272 s (0.7.0:
  95/100). The five former failures now trim: seed 24: 391 items for 284 locations, 17 of 17
  traps and 90 of 149 copies; 34: 3 of 134 copies; 58: 13 of 214; 66: 19 of 208; 71: 17 of 144.
* Parity harness rebuilt, all own options off: **1200/1200 identical** (0 mismatching).

### Overworld shuffle (apworld 0.8.0, 2026-11-17)

`shuffle_overworld` (`RO_SHUFFLE_OVERWORLD`, default off): exits between outdoor areas lead to other outdoor areas,
coupled (through exit A into the place of exit X, back through X's way out in front of A). Stage 6a; temples, boss
rooms, grottos and interiors keep their own options and never mix with the overworld (stage 6b).

#### Pool and criteria

68 rows appended to `shuffleEntrances`, one per exit, two per transition (34 transitions). A row is taken when both
sides are outdoor scenes, the logic gives the exit a way back (no `ONE_WAY_EXIT`) and the arrival is on foot on land.

| Area | Transitions (Regions file: line there / line back) |
|---|---|
| Clock Town (7) | South–East lower `Central.cpp:236/110`, upper `231/112`, North–East `181/114`, South–North `233/182`, South–West upper `232/259`, lower `234/258`, South–Laundry Pool `235/154` |
| Clock Town gates (4) | South `TerminaField.cpp:578`/`Central.cpp:230`, East `579/109`, North `580/180`, West `574/257` |
| Termina Field (5) | Road to Southern Swamp `TerminaField.cpp:575`/`South.cpp:299`, Road to Ikana `576`/`East.cpp:331`, Milk Road `577`/`MilkRoad.cpp:140`, Path to Mountain Village `TerminaField.cpp:20`/`North.cpp:439`, Great Bay Coast `TerminaField.cpp:12`/`West.cpp:147` |
| Southern Swamp (5) | Road–Swamp `South.cpp:301/370`, Swamp–Woods of Mystery `633/461`, Swamp–Deku Palace `166/478`, treetop `167/497`, Swamp–Woodfall `496/558` |
| Milk Road (3) | Romani Ranch `MilkRoad.cpp:141/276`, Gorman Track front `142/68`, back `158/113` |
| Mountains (6) | Path–Mountain Village `North.cpp:462/315`, Village–Path to Goron Village `413/311`, Village–Path to Snowhead `314/497`, Path–Snowhead `551/527`, Path–Goron Village `414/140`, Path–Goron Racetrack `416/91` |
| Great Bay (1) | Great Bay Coast–Zora Cape `West.cpp:148/357` |
| Ikana (3) | Road–Canyon `East.cpp:303/151`, Road–Graveyard `317/255`, Canyon–Stone Tower `473/183` |

Fixed: one-way ways (Clock Tower rooftop `Central.cpp:237`, Ikana river `East.cpp:159`, `IKANA_GRAVEYARD:4`
`East.cpp:26`, Pirates' Fortress exterior :3 `PiratesFortress.cpp:318/357`, blue warps `GreatBayTemple.cpp:57`,
`StoneTowerTemple.cpp:435`, owls and songs `RR_MAX`); the Stone Tower inversion `East.cpp:481/545` and the canyon
cave in one scene `East.cpp:112/187`; arrivals in water (Pirates' Fortress exterior `West.cpp:151`, Pinnacle Rock
`West.cpp:149/244`, Zora Hall water tunnel `West.cpp:359/413`, Waterfall Rapids `West.cpp:283/360`): a Deku
arriving there would void out and respawn at the same entrance forever. Stage 6b: Astral Observatory, Zora Hall by
land, Great Fairy fountains, Ikana Castle, Sakon's hideout. Entrance cutscenes belong to the arrival spawn
(`z_demo.c:1548-1582`), so redirecting to the real entrance of a place plays what its vanilla way in plays.

#### Code evidence

* **Scene variants.** Play_Init turns `save.entrance` into the variant (`z_play.c:2261-2300`): WINTER → SPRING
  (Mountain Village, Goron Village, Path to Goron Village), POISONED → CLEARED, and the layer through
  `nextCutsceneIndex` (Woodfall 0xFFF1, Ikana Canyon 0xFFF2, Great Bay Coast/Zora Cape and the Snowhead area 0xFFF0,
  first-cycle Termina Field 0xFFF4), only while it is 0xFFEF or 0xFFF0. Rows name places: base entrances and base
  scenes only (the generator refuses `*_SPRING`, `*_CLEARED`, `SCENE_*2`, `SCENE_F41`). `RedirectOverworld` compares
  the next entrance as a place (spring/cleared group mapped to winter/poisoned, layer bits dropped) and the current
  scene through `Play_GetOriginalSceneId` (`z_play.c:1975`), sets the target's base entrance and resets
  `nextCutsceneIndex` ≥ 0xFFF0 to 0xFFEF, so Play_Init picks the target's variant. Scene exit lists are in the
  assets, so whether a spring scene's exit holds a SPRING or WINTER value is not known from source; both are handled.
* **Only real exits.** Redirected when `respawnFlag` is 0 or −2 (an exit that recorded itself as void-out point,
  `z_player.c:6510`; Play_Init resets it, `z_play.c:2497`), no cutscene runs and the current scene is the row's.
  Void out (1, `z_play.c:2074`), death and game over (−1/−6, `z_play.c:2084`, `z_game_over.c:97`), grotto return (4,
  `z_player.c:6497`), warp songs (−3, `z_kankyo.c:3143`) and Sakon's hideout (−7, `z_play.c:2063`) reload a place
  already reached. An exit that stored itself as void-out point (`z_player.c:6508`) gets the new entrance.
* **Epona.** Riding through an exit sets `gHorseIsMounted` (`z_player.c:6578`); Horse_SpawnOverworld then mounts
  Link in any scene, testing only the layer (`z_horse.c:124`). On a redirected exit Link gets off: the flag is cleared
  and `horseData` is set to the exit's scene and the horse's position, as after dismounting (`z_player.c:17188`). In
  a horse scene (`z_horse.c:59-69`) she appears next to Link with Epona's Song; the logic asks for the song in each
  scene (`CAN_RIDE_EPONA`, `Logic.h:205`).
* **Start.** The file starts as Human without sword in South Clock Town 0 (`OnFileCreate.cpp:30-43`), fixed. The four
  Clock Town gates (seen from inside the town) are placed first, each to the place of a random exit outside Clock
  Town; GER's group lookup alone could end with a gate and a Clock Town place left (seen in a trial run). The guards
  stay at the gate (`z_en_stop_heishi.c:375-395`); the logic's gate rules are `true`.
* **Area rules** (`shuffleAreaRules`), only where an arrival's region has no way in that lacks the vanilla guarantee
  (owl destinations count as such a way, `RO_SHUFFLE_OWL_STATUES`): Woodfall 0 `CAN_BE_DEKU` (only way in
  `South.cpp:496`, grass without rule); Deku Palace 5 (treetop) `CAN_BE_DEKU` (`South.cpp:497`); Goron Racetrack 0 and
  Path to Goron Village 2 (in front of its boulder) Powder Keg and Goron (`North.cpp:416`, 30 pots and 10 trees
  without rule); Milk Road 1 (past the boulder, in `RR_MILK_ROAD` with the way to Termina Field) day 3 or boulder
  destroyed (`MilkRoad.cpp:141`). Graveyard, Ikana Canyon, ranch, Gorman Track, Woods of Mystery and the mountain
  paths need none (their rules are explicit or the region is reached by owl).
* **Other options.** No overworld entrance is a row of another option (generator check; a grotto hole's logic way
  back such as `TERMINA_FIELD:0` is not, the grotto returns to the stored hole). Boss warps to e.g. `WOODFALL:0` come
  from a boss scene and stay. Grotto holes, building doors and temple doors stay in their area and move with it.

#### Verification

* `pytest test/general worlds/mm_2s2h/test -q`: **699 passed, 0 failed** (147,585 subtests), 194 s.
* New `test/test_overworld_shuffle.py` (45 tests): table (68 rows, each with its opposite row, appended after row 65,
  the four gates); every row names a place (no SPRING/CLEARED/inverted entrance, no variant scene; every place with
  a variant is in the pool by its base name); one-way, water, inversion, rooftop, observatory and Song of Time
  entrances are not in the pool; the five area rules; option off with all other own options = no overworld pair and
  every overworld exit as generated; option on = coupled over 12 seeds (inherited check plus: the opposite exit of
  each arrival leads back to the exit's place), the gates never arrive in Clock Town (10 seeds), every location
  reachable with all items, beatable, Universal Tracker rebuilds it; area rules with forced pairings (Road to Southern
  Swamp → Woodfall needs Deku, Woodfall's own exit keeps its Deku rule, Mountain Village → racetrack needs the keg and
  so does the coupled way to the racetrack's boulder); No Logic; all own options together (also No Logic and
  own-dungeon items); 0.7 slot data (no `RO_SHUFFLE_OVERWORLD`) rebuilds unchanged and beatable.
  `test_entrance_shuffle.py` and `test_interior_shuffle.py` narrowed to their own rows.
* `test_pool_trim.py`: the 38 fills pinned from 0.7.0 are **38/38 unchanged** with the overworld off. The
  fingerprint leaves out options added later while they are off (`NEW_OPTIONS`): they only add a key to the slot
  data. `tools/random_seeds.py` does not roll `shuffle_overworld` (that would shift every roll); set it with
  `shuffle_overworld=true`.
* 100 random seeds with `shuffle_overworld=true` (everything else randomized): **100/100**, 313 s. A trial run
  before the gates were placed first failed 1 of 5 in GER ("None of the available entrances are valid targets",
  a gate and a Clock Town place left); fixed as described above, 10/10 then.
* 100 random seeds with overworld, temples, bosses, grottos, interiors, door keys and NPC souls: **100/100**, 308 s.
* C++ test bench (`tools/entrance_client/run.py ../2s2h`), real `EntranceShuffle.cpp` with `Entrance_Create`
  (`z_scene.c`) and `Play_GetOriginalSceneId` (`z_play.c`) copied verbatim: **2,244,544 overworld transitions, all
  4624 exit/arrival pairings** (68², coupled), each with the other entrance options off and on, option off and on,
  the exit's scene and its variant (cleared swamp, spring villages, inverted Stone Tower), the entrance as base, as
  spring/cleared form and with layer bits, respawn flags 0/−2 (redirected) and 1/−1/−6/4/−3/−7 (not), with and
  without a running cutscene, mounted and not: 130,248 redirected, 1,984,824 kept. Redirected: base entrance of the
  target, layer index reset, void-out point moved, Link off Epona with `horseData` at the exit; for all four
  clear-flag states the Play_Init model (`z_play.c:2264-2283`) loads the target's variant (520,992 loads). After the
  arrival a void out/death or exit in the target's scene, a grotto return through a pool entrance and the 11 song/owl
  entrances are never redirected. Older parts pass unchanged (door keys 3,632,128; temples/bosses 9216 cases;
  grottos 571,328; interiors 193,700). It does not run Player, En_Horse or the scene loader.
* Parity harness rebuilt, all own options off: **1200/1200 identical** (0 mismatching).
* Client builds locally (MSVC Release, `2ship.exe`), no warning in `EntranceShuffle.cpp`; changed C++ lines pass
  clang-format.

#### In-game checklist — not tested in game

1. Option on: walk through every transition of the table once; the arrival is the one in the spoiler, and walking
   back puts you at the exit you used.
2. Variants: with Woodfall, Snowhead, Great Bay and Stone Tower cleared, enter the swamp, Mountain Village, Goron
   Village, Path to Goron Village, Great Bay Coast, Zora Cape and Ikana Canyon through foreign exits: spring/cleared
   scene and layer as in vanilla; leaving them through each exit redirects too.
3. Epona: ride through a foreign exit into a horse scene and into one without (e.g. Mountain Village): Link arrives
   on foot, Epona is not lost (Epona's Song; she waits at the exit on return).
4. Milk Road 1 before day 3 without the boulder destroyed, and Path to Goron Village 2 with the racetrack boulder:
   where exactly the spawn is (the logic assumes behind the boulder); never stuck (the exit behind you leads back).
5. Deku Palace treetop and Woodfall entered as Human: you can leave again.
6. Void out, die, pause save and reload, Song of Time, Song of Soaring and owls right after a foreign exit: you land
   at the arrival or the usual place, never in a loop.
7. First cycle: start in South Clock Town as Human, leave through a gate; the guards behave as in vanilla.
8. Entrance cutscenes on first arrival (Woodfall, Snowhead, Romani Ranch, first-cycle Termina Field) play as for
   that place's own way in.
9. Cremia's escort and other scripted transitions keep their vanilla destination.
10. Together with temples, bosses, grottos, interiors and door keys: holes, doors and temple doors behave as before.
11. Option off and 0.1–0.7 seeds/saves: everything as before.

### Stage 6b: remaining transitions (apworld 0.9.0, 2026-11-18)

Every two-way exit of the logic that was in no table: 100 exits, of which 26 join the older pools, 12 form the new
pool `shuffle_special_areas` and 62 stayed fixed before 6c. The table below records the current inventory. 23 rows
appended to `shuffleEntrances` (rows 134–156), so the saved row numbers of 0.3–0.8 stay valid.

#### Pools and criteria

| Option | New rows | What |
|---|---|---|
| `shuffle_overworld` | 4 (land) | Astral Observatory: Bombers' passage from East Clock Town `Central.cpp:111/31`, door to Termina Field `Central.cpp:15/44` |
| `shuffle_overworld` | 4 (water) | Great Bay Coast–Pirates' Fortress moat `West.cpp:151`/`PiratesFortress.cpp:171`, Zora Cape–Zora Hall tunnel `West.cpp:359/413` |
| `shuffle_interiors` | 9 | Great Fairy fountains `Central.cpp:183/137`, `South.cpp:559/539`, `North.cpp:580/539`, `West.cpp:334/217`, `East.cpp:186/287`; Goron Shop `North.cpp:39/104`; Goron Graveyard `North.cpp:313/15`; Deku King's chamber `South.cpp:112/51` and cell `South.cpp:122/46`, all "always open" |
| `shuffle_special_areas` (new) | 6 | Swamp Spider House `South.cpp:479`/`SpiderHouses.cpp:221`, Oceanside Spider House `West.cpp:152`/`SpiderHouses.cpp:64`, Beneath the Well `East.cpp:184`/`BeneathTheWell.cpp:83`, Ikana Castle `East.cpp:185`/`IkanaCastle.cpp:91`, Secret Shrine `East.cpp:158/408`, Deku Shrine `South.cpp:168/177` |

#### Code evidence

* **Water.** An arrival is reached only through the exit on the other side, in the same form. A land exit (any form)
  therefore never leads under water, where a Deku or Goron would void out and respawn at the same entrance forever.
  Both ways of the two water transitions are taken swimming (Zora in the logic); `overworldWaterEntrances` lists
  their four rows, `CanPair` refuses land/water pairs whatever the slot data says, the generator refuses a transition
  listed on one side only. Human Link dives as well (`z_player.c:8931`, every form but Zora), so Pinnacle Rock (SWIM
  only) and Waterfall Rapids (Hookshot) stay fixed: a Human could arrive in a Zora-only place. Zora Hall's door by
  land (`ZORA_HALL:1`) has no region in the logic (`UNTRANSLATED.md`): the apworld pairs a coast's water exit (Great
  Bay Coast, Zora Cape) always with a dead end's (moat, Zora Hall), so the water pool is vanilla or crossed. GER is
  not used for the water rows: in its pool, or paired before it, it deadlocked in tests (its first stage only places
  exits that open new regions); they stay vanilla during GER and are paired afterwards. GER only proved the graph with
  vanilla water, so a crossed pairing is kept only if it loses no region reachable with all items; otherwise the water
  stays vanilla. Without this check 4 of 400 seeds with own-dungeon items failed in `pre_fill` (e.g. Zora Cape was
  then reachable through nothing but itself; 0.8.0: 0 of 300); with it 0 of 400.
* **Blue warps during GER** (correction of the 0.8 code, seen in 0.9 testing): with the overworld shuffled, GER counted
  a place as reached through a blue warp's vanilla target, which `_connect_boss_warps` afterwards points at the door
  used. Overworld batch seed 8 (dungeon entrances on, Great Bay Temple behind Stone Tower's door) left Zora Cape and
  Laundry Pool as an island and failed the accessibility check. Now, with the overworld and temples or bosses
  shuffled, every blue warp leads back into its own boss room while GER runs. Without the overworld nothing changes,
  every warp target is reachable on its vanilla way (the 40 fills without interiors stay those of 0.8.0).
* **Area rules** (`shuffleAreaRules`): `ASTRAL_OBSERVATORY:0` projectile (the passage leads on to the observatory with
  `true`, `Central.cpp:34`, the way in from East Clock Town asks for the balloon, `Central.cpp:111`);
  `PIRATES_FORTRESS_EXTERIOR:0` and `ZORA_HALL:0` Zora and SWIM (the moat's regions and Zora Hall's tunnel back,
  `West.cpp:413`, assume it); `DEKU_KINGS_CHAMBER:0/1` Deku (the palace inside is Deku only, `South.cpp:171`; the
  princess event `South.cpp:55` does not say so); `SWAMP_SPIDER_HOUSE:0` its own door's rule (`South.cpp:479`). No
  rule for the other special areas: Ikana Canyon's upper part is also reached by owl (`East.cpp:197`), the Secret
  Shrine checks Light Arrows (`East.cpp:411`), the Deku Shrine its event (`South.cpp:180`), Darmani Lens of Truth
  (`North.cpp:12`).
* **Gates.** The Bombers' passage leads out of Clock Town like the four gates and is placed first like them.
* **Interiors.** The fountains are walk-in exits, one spawn per fountain in `SCENE_YOUSEI_IZUMI` like the Zora Hall
  rooms. The hours proof of 0.7 covers the new rows (757 = 26² + 9² door/building pairs).
* **Special areas.** `RedirectSpecialArea` compares as the overworld does: the place (`PlaceOf`: cleared swamp group
  and layer bits) and the original scene (`Play_GetOriginalSceneId`), only real exits (respawn flag 0 or −2), no
  running cutscene; it resets a layer chosen for the old place and moves the recorded void-out point. Void outs and
  deaths inside reload the area's own entrance from its own scene, never redirected again. The generator checks base
  names, one scene per area, no shared entrance with another option.
* **Older seeds.** A 0.8 (or older) seed has no saved pair for the 23 new rows: `InsideBehind` keeps them vanilla
  even with Shuffle Overworld or Interiors on; the Universal Tracker rebuilds missing rows as themselves; Shuffle
  Special Areas is off when its option is missing.

#### Verification

* `pytest test/general worlds/mm_2s2h/test -q`: **752 passed, 0 failed** (151,144 subtests), 167 s; 53 tests more than 0.8.0 (699). New: `test_special_areas.py` (table, option off
  with every other own option on, option on with coupling, pairs only among the six, Universal Tracker and
  reachability inherited, area rule through a forced pairing, No Logic, all own options together, own-dungeon items,
  real 0.8 slot data, fixed-exit inventory); in `test_overworld_shuffle.py` water rows, water only with water and
  coast with dead end over 10 seeds, area rules of observatory and water through forced pairings, two regression
  seeds; interior counts and the hours proof over 757 pairs; `test_golden_without_interiors`.
* Old seeds: the real slot data of two 0.8.0 seeds (all own options plus overworld; overworld only, in `test/data`)
  rebuild exactly in the Universal Tracker, all 23 new rows stay vanilla, the seed stays beatable.
* Golden: `GOLDEN` recomputed for 0.9.0 (0 of 38 equal, as expected: all 38 rolls have Shuffle Interiors on, whose pool
  grew). `GOLDEN_WITHOUT_INTERIORS`, the same 40 rolls without it, computed with 0.8.0 before any change: **40/40
  equal** with 0.9.0.
* Random seeds (`tools/random_seeds.py 100`): only special areas **100/100**; only overworld **100/100** (99/100 before
  the blue-warp correction: seed 8, see above); all own options together **100/100**.
* Own-dungeon items with all own options and the overworld, `pre_fill`: 0.8.0 0 of 300 seeds failed; 0.9 before the
  water check 4 of 400; with it 0 of 400; with the final code (also the blue-warp correction) **0 of 200**.
* GER deadlock (water rows in GER's pool or paired before it): 2 of 12 seeds; water paired after GER: **0 of 40**.
* C++ test bench (`tools/entrance_client/run.py ../2s2h`), real `EntranceShuffle.cpp`: special areas **163,347 cases,
  all 36 pairings** (both directions, every scene and entrance form, respawn flags, cutscenes, void-out point, songs
  and owls, with and without the other options, pairs with other options refused); overworld **2,759,104
  transitions, 5776 pairings** (76², of them 1152 land/water pairings refused); interiors **380,061 cases, 763
  pairings**; real 0.8 slot data: **260 cases**, the 0.8 pairs as saved, all 23 new rows vanilla both ways. Older
  parts unchanged (door keys 3,632,128; temples/bosses 9216 cases; grottos 571,328).
* Parity harness rebuilt, all own options off: **1200/1200 identical** (0 mismatching).
* Client builds locally (MSVC Release, `2ship.exe`), no warning in `EntranceShuffle.cpp`; changed C++ lines pass
  clang-format.


#### In-game checklist — not tested in game

1. Astral Observatory: through foreign exits into the Bombers' passage and into the observatory; the balloon, the
   telescope and the Moon's Tear as in vanilla; both ways out lead back to the exits used.
2. Water, crossed pairing (spoiler): Zora Cape's tunnel into the Pirates' Fortress moat and Great Bay Coast's into
   Zora Hall's pool, as Zora; the arrivals are in water and you can swim back. As Human (outside the logic): never
   trapped (land to stand on, Song of Time).
3. Water exits as Deku and Goron: no water exit can be reached at all (no void-out loop).
4. Each Great Fairy fountain behind a foreign door: the right Great Fairy, her reward and scene, the way out in front of
   the door used. Goron Shop and Goron Graveyard (Darmani) behind foreign doors; the shop door in the Goron Shrine leads
   into another building.
5. Deku King's chamber and the monkey's cell entered as Human through a foreign door: what the guards do and where you
   land; returning the princess there.
6. Each special area behind a foreign door, both ways: spider houses (also with the swamp cleared), Beneath the Well
   (its back exit into the castle courtyard, then out of the castle's front door in front of the castle's door), Ikana
   Castle, Secret Shrine, Deku Shrine (the butler race and its end), Ikana Canyon's layers on the way back.
7. Void out, die, pause save and reload, Song of Time, Song of Soaring and owls in every new area: you land at its
   entrance or the usual place, never in a loop.
8. A 0.8 seed and save with Shuffle Overworld and Shuffle Interiors on: observatory, water ways, fountains, Goron
   Shop/Graveyard and the King's chamber as in vanilla; Shuffle Special Areas off.
9. All options off: everything as before.

#### Exits that stay fixed (60)

Every two-way exit of the logic outside the shuffle tables, one row per exit. `FixedExitInventory` in
`test_special_areas.py` holds the same list and the 15 one-way exits. Stage 6c moved two exits from this list into the
inverted Stone Tower Temple row, active when dungeons join the mixed pool.

| Exit (region → entrance) | File:line | Reason |
|---|---|---|
| `RR_BENEATH_THE_WELL_BACK_EXIT` → `IKANA_CASTLE:0` | `BeneathTheWell.cpp:33` | Inside Ikana Castle and the well's back exit: inner ways of two special areas, they move with their front doors. |
| `RR_CLOCK_TOWER_INTERIOR` → `SOUTH_CLOCK_TOWN:0` | `Central.cpp:70` | Clock Tower: start of the game, Happy Mask Salesman and Song of Time scenes, every cycle starts here. |
| `RR_CLOCK_TOWN_SOUTH` → `CLOCK_TOWER_INTERIOR:1` | `Central.cpp:229` | Clock Tower: start of the game, Happy Mask Salesman and Song of Time scenes, every cycle starts here. |
| `RR_BENEATH_THE_GRAVEYARD_NIGHT_1_GRAVE` → `IKANA_GRAVEYARD:3` | `East.cpp:52` | Beneath the Graveyard: open one night each (hours of its own, like `INTERIOR_OWN_HOURS`), the hole appears after the Stalchild. |
| `RR_BENEATH_THE_GRAVEYARD_NIGHT_2_GRAVE_BEFORE_PIT` → `IKANA_GRAVEYARD:2` | `East.cpp:95` | Beneath the Graveyard: open one night each (hours of its own, like `INTERIOR_OWN_HOURS`), the hole appears after the Stalchild. |
| `RR_IKANA_CANYON_CAVE` → `IKANA_CANYON:13` | `East.cpp:112` | Ikana Canyon cave: both sides in one scene (as in 0.8). |
| `RR_IKANA_CANYON_LOWER` → `SAKONS_HIDEOUT:0` | `East.cpp:157` | Sakon's Hideout: open only on night 3 from 18:00 with Kafei, mandatory scene, respawn of its own (−7, `z_play.c:2063`). |
| `RR_IKANA_CANYON_UPPER` → `IKANA_CANYON:14` | `East.cpp:187` | Ikana Canyon cave: both sides in one scene (as in 0.8). |
| `RR_IKANA_GRAVEYARD_LOWER` → `BENEATH_THE_GRAVERYARD:0` | `East.cpp:257` | Beneath the Graveyard: open one night each (hours of its own, like `INTERIOR_OWN_HOURS`), the hole appears after the Stalchild. |
| `RR_IKANA_GRAVEYARD_LOWER` → `BENEATH_THE_GRAVERYARD:1` | `East.cpp:258` | Beneath the Graveyard: open one night each (hours of its own, like `INTERIOR_OWN_HOURS`), the hole appears after the Stalchild. |
| `RR_SAKON_HIDEOUT` → `IKANA_CANYON:6` | `East.cpp:372` | Sakon's Hideout: open only on night 3 from 18:00 with Kafei, mandatory scene, respawn of its own (−7, `z_play.c:2063`). |
| `RR_STONE_TOWER_INVERTED_LOWER` → `STONE_TOWER:1` | `East.cpp:481` | Stone Tower inversion: turns the scene over (as in 0.8). |
| `RR_STONE_TOWER_TOP` → `STONE_TOWER_INVERTED:0` | `East.cpp:545` | Stone Tower inversion: turns the scene over (as in 0.8). |
| `RR_IKANA_CASTLE_BEFORE_THRONE` → `IGOS_DU_IKANAS_LAIR:0` | `IkanaCastle.cpp:12` | Inside Ikana Castle and the well's back exit: inner ways of two special areas, they move with their front doors. |
| `RR_IKANA_CASTLE_COURTYARD` → `BENEATH_THE_WELL:1` | `IkanaCastle.cpp:58` | Inside Ikana Castle and the well's back exit: inner ways of two special areas, they move with their front doors. |
| `RR_IKANA_CASTLE_COURTYARD` → `IKANA_CASTLE:3` | `IkanaCastle.cpp:59` | Inside Ikana Castle and the well's back exit: inner ways of two special areas, they move with their front doors. |
| `RR_IKANA_CASTLE_MAIN_ROOM` → `IKANA_CASTLE:2` | `IkanaCastle.cpp:115` | Inside Ikana Castle and the well's back exit: inner ways of two special areas, they move with their front doors. |
| `RR_IKANA_CASTLE_THRONE_ROOM` → `IKANA_CASTLE:6` | `IkanaCastle.cpp:176` | Inside Ikana Castle and the well's back exit: inner ways of two special areas, they move with their front doors. |
| `RR_MOON_DEKU_TRIAL` → `THE_MOON:0` | `Moon.cpp:15` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_MOON_GORON_TRIAL` → `THE_MOON:0` | `Moon.cpp:39` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_MOON_LINK_TRIAL` → `THE_MOON:0` | `Moon.cpp:62` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_MOON_ZORA_TRIAL` → `THE_MOON:0` | `Moon.cpp:82` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_MOON` → `MOON_DEKU_TRIAL:0` | `Moon.cpp:91` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_MOON` → `MOON_GORON_TRIAL:0` | `Moon.cpp:97` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_MOON` → `MOON_ZORA_TRIAL:0` | `Moon.cpp:103` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_MOON` → `MOON_LINK_TRIAL:0` | `Moon.cpp:109` | Moon: endgame; all four ways back end at `THE_MOON:0` (no place per door), mask requirements. |
| `RR_GORON_SHRINE` → `GORON_VILLAGE_WINTER:2` | `North.cpp:38` | Goron Shrine: door rule `FIRST_DAY() || CAN_BE_GORON` (hours of its own) and no dead end (Goron Shop). |
| `RR_GORON_VILLAGE` → `GORON_SHRINE:0` | `North.cpp:141` | Goron Shrine: door rule `FIRST_DAY() || CAN_BE_GORON` (hours of its own) and no dead end (Goron Shop). |
| `RR_PIRATES_FORTRESS_CAPTAIN_ROOM_UPPER` → `PIRATES_FORTRESS:2` | `PiratesFortress.cpp:18` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_CAPTAIN_ROOM` → `PIRATES_FORTRESS:1` | `PiratesFortress.cpp:36` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_INSIDE_3_GUARD_ROOM` → `PIRATES_FORTRESS:3` | `PiratesFortress.cpp:47` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_INSIDE_CHEST_EGG_ROOM` → `PIRATES_FORTRESS:8` | `PiratesFortress.cpp:65` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_INSIDE_GREEN_GUARD` → `PIRATES_FORTRESS:5` | `PiratesFortress.cpp:79` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_INSIDE_LINE_GUARD` → `PIRATES_FORTRESS:7` | `PiratesFortress.cpp:88` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_INSIDE_MAZE_GUARD` → `PIRATES_FORTRESS:5` | `PiratesFortress.cpp:99` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_LEFT_CLAM_EGG_ROOM` → `PIRATES_FORTRESS:4` | `PiratesFortress.cpp:134` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_LEFT_PLATFORM` → `PIRATES_FORTRESS_INTERIOR:3` | `PiratesFortress.cpp:146` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_MOAT_HIGHER` → `PIRATES_FORTRESS:0` | `PiratesFortress.cpp:154` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_MOAT_HIGHER` → `PIRATES_FORTRESS_INTERIOR:10` | `PiratesFortress.cpp:155` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_MOAT_LOWER` → `PIRATES_FORTRESS_INTERIOR:9` | `PiratesFortress.cpp:172` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_MOAT_PLATFORM` → `PIRATES_FORTRESS:12` | `PiratesFortress.cpp:189` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA_LEFT_EXIT` → `PIRATES_FORTRESS_INTERIOR:7` | `PiratesFortress.cpp:198` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA_LEFT_LOWER` → `PIRATES_FORTRESS_INTERIOR:7` | `PiratesFortress.cpp:206` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA_LEFT_UPPER` → `PIRATES_FORTRESS_INTERIOR:2` | `PiratesFortress.cpp:221` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA_RIGHT_EXIT` → `PIRATES_FORTRESS_INTERIOR:5` | `PiratesFortress.cpp:234` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA_RIGHT` → `PIRATES_FORTRESS_EXTERIOR:5` | `PiratesFortress.cpp:245` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA_RIGHT` → `PIRATES_FORTRESS_INTERIOR:4` | `PiratesFortress.cpp:246` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA_TOWER` → `PIRATES_FORTRESS_INTERIOR:1` | `PiratesFortress.cpp:258` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA` → `PIRATES_FORTRESS_INTERIOR:0` | `PiratesFortress.cpp:279` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_PLAZA` → `PIRATES_FORTRESS_EXTERIOR:1` | `PiratesFortress.cpp:280` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_RIGHT_CLAM_EGG_ROOM` → `PIRATES_FORTRESS:6` | `PiratesFortress.cpp:303` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_SEWERS_PREGATE` → `PIRATES_FORTRESS_EXTERIOR:2` | `PiratesFortress.cpp:356` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_PIRATES_FORTRESS_TELESCOPE_ROOM` → `PIRATES_FORTRESS_EXTERIOR:6` | `PiratesFortress.cpp:378` | Inside Pirates' Fortress: inner structure; entrances 4 and 7 shared by several regions (`UNTRANSLATED.md`), moat arrivals under water, caught by guards you land at fixed points. Moves as a whole with its underwater way in. |
| `RR_WOODFALL` → `WOODFALL_TEMPLE:2` | `South.cpp:560` | Deku Princess's room: second door into the temple scene, open only after the temple; belongs to the temple. |
| `RR_GREAT_BAY_COAST` → `PINNACLE_ROCK:0` | `West.cpp:149` | Pinnacle Rock: under water, but the logic asks only for SWIM and Human Link dives too (`z_player.c:8931`): a Human could arrive in a Zora-only place. |
| `RR_PINNACLE_ROCK_ENTRANCE` → `GREAT_BAY_COAST:3` | `West.cpp:244` | Pinnacle Rock: under water, but the logic asks only for SWIM and Human Link dives too (`z_player.c:8931`): a Human could arrive in a Zora-only place. |
| `RR_WATERFALL_RAPIDS` → `ZORA_CAPE:4` | `West.cpp:283` | Waterfall Rapids: way in by Hookshot (Human), arrival in the river: land and water mixed. |
| `RR_ZORA_CAPE_BEFORE_GREAT_BAY_TEMPLE` → `ZORA_HALL:1` | `West.cpp:295` | Zora Hall by land: `ZORA_HALL:1` has no region in the logic (`UNTRANSLATED.md`), so no way back. |
| `RR_ZORA_CAPE` → `WATERFALL_RAPIDS:0` | `West.cpp:360` | Waterfall Rapids: way in by Hookshot (Human), arrival in the river: land and water mixed. |
| `RR_WOODFALL_TEMPLE_DEKU_PRINCESS_ROOM` → `WOODFALL:3` | `WoodfallTemple.cpp:90` | Deku Princess's room: second door into the temple scene, open only after the temple; belongs to the temple. |

### Stage 6c – mixed entrance pools (apworld 0.10.0)

The C++ tables define the selectable pools, the doors fixed during mixing, and the appended inverted temple row
(zero-based index 157). The generator checks unique options, masks, source scenes and entrances, reciprocal land
edges for fixed pairs, endpoint capacity and the connection between the inverted temple and Twinmold's boss door.
Mixing still uses Archipelago's coupled entrance randomizer. Each endpoint is one outgoing transition from a physical
place. A temple, interior or special area contributes an outside and an inside end. Land overworld rows contribute
one end each. The five Clock Town gates receive land partners outside town before the remaining land ends are split
evenly between the two groups for coupled placement.

`mixed_entrance_pairs` is optional slot data. Each pair gives two endpoint IDs, `2 * row` for the outside or land end
and `2 * row + 1` for the inside end. The other end's arrival is the spawn on that side of its door or exit.
Universal Tracker reconnects both directions from the saved pairs. The client validates compatible endpoints,
unique partners and coverage before using the map, and saves it separately from `entrancePairs`.
The JSON loader zero-fills a missing `mixedEntrancePairs` array and preserves the old row pairs.

Area rules follow a foreign destination, while its source keeps the original access requirements and door key.
Bosses and grottos shuffle in their own pools. Grotto returns can depend on the exact position saved when falling
into the hole. Even shops can have another way in, so a door leading back to a hole would need a verified
return point for that hole. Water stays within its existing coast-to-dead-end pool. The 24 unrestricted interiors
are eligible for the common pool, nine daytime interiors mix within their hours class, and the six unique schedules
stay fixed. Trading Post and Swordsman's School stay fixed when interiors are mixed. When the overworld is mixed,
the Ranch entrance and its return to Milk Road stay coupled to one another.

The inverted temple is eligible when dungeons are selected in `mix_entrance_pools`. Inversion within the outside
tower is still vanilla. A foreign entrance into the inverted temple adds the normal tower area requirements plus
Bow, Light Arrows and magic. Boss warps resolve the boss door and then the temple's return endpoint, including the
first warp through the Giants' Chamber. Completion flags still follow the boss. An empty pool list or a 0.9 seed
keeps the inverted temple entrance vanilla.

#### Verification

- `pytest test/general worlds/mm_2s2h/test -q` passed 779 tests and 151,149 subtests. All 11 pool selections were
  tested in Glitchless and No Logic, including coupling, reachability and Universal Tracker. The 38 + 40 existing
  Golden fingerprints match with mixing off. The new compatibility fixture contains 157 pairs generated by apworld
  0.9.0 with every entrance shuffle enabled.
- `random_seeds.py` passed 100/100 with `mix_entrance_pools=all` and 100/100 with `mix_entrance_pools=random`.
  Parity passed 1200/1200 scenarios after rebuilding the shim.
- The C++ harness passed 4,900 mixed transitions, 110 boss warps and 77 rejected maps across the 11 selections.
  It also round-tripped each of the 11 new mixed saves.
- The 0.9 save test uses client commit `87077c1bd19b46a6e82cfdea489867a3fad99ae3`. The harness reads the
  `RandoSaveInfo` layout, row order, option IDs and unmodified JSON codec from that commit. It populates the
  historical struct from the existing 0.9.0 slot data, writes `build/rando_save_0_9_0.json`, then opens the file
  with the 0.10 codec. All 157 pairs survive and resolve to the original partners. The loader clears the
  prefilled mix option and all 1,024 entries of the mixed pair array.
- The Windows x64 Release client built successfully. MSBuild stalled after `ZERO_CHECK` with 12 project nodes.
  With one node, MSVC reported `C1902: Program database manager mismatch` and, after switching to `/Z7`,
  `C2855: command-line option '/Z7' inconsistent with precompiled header`. The build passed with one project
  node, 12 compiler workers and rebuilt `/Z7` headers. These were local build flags.
- Initial runs exposed an outdated expectation in three area-rule tests (`AssertionError` for the new
  `STONE_TOWER_TEMPLE_INVERTED:0` rule) and blocked temp cleanup in seed generation
  (`PermissionError: [WinError 5] Access is denied`). The test inventory now includes the new rule, and both
  seed batches passed with `TEMP` and `TMP` set within the workspace.

#### In-game checklist — not tested in game

- Check a shop door into a temple and the temple exit back to the shop door. Check a temple entrance into a shop
  and the shop exit back to the temple entrance. Verify the original door key and opening hours.
- Walk through a land exit into a building and go back both ways. Try a cleared swamp entrance, a spring mountain
  entrance, the Great Bay turtle on its first and later rides, and a redirected exit while mounted on Epona.
- Take a Clock Town gate and the Bombers' passage. Verify the water routes, Trading Post and Swordsman's School
  closures, the daytime interiors, and the Ranch boulder on days 1 and 3. Try a grotto in each of its two variants
  (hole and scene exit) with all eligible pools mixed.
- Enter normal and inverted Stone Tower Temple, take Twinmold's door with bosses shuffled and vanilla, then take
  the first and a repeat blue warp. Verify the arrival, clear flag and return after death or void-out. Repeat a
  mixed-temple warp through the other bosses.
- Save and reload a mixed seed, then take the same entrances before reconnecting. Load a 0.9 save and connect it
  to its original slot. Verify that its old pairs and the vanilla inverted entrance still apply.

### Stage 4a – abilities (apworld 0.11.0)

- `Rando/MiscBehavior/Abilities.cpp` supplies the option, item and save flag for both abilities, and identifies the
  original logic expressions the generator augments. The generator checks the referenced macro and branch names and
  preserves the clauses around each branch. The original `Logic.h` and `Logic/Regions/*.cpp` serve as the parity reference.
- `Player_ActionHandler_3` checks the ride ability before setting mounted state. `Horse_Spawn` clears the carried
  mount when the ability is locked, then still dispatches the scripted Ranch layers. The song and call go through
  `z_message.c` and `EnHorse` as before. The two Gorman payment helpers in `z_en_in.c` check before debiting rupees,
  ahead of the transition that the skip enhancement intercepts. The Garo check, both Great Bay fence directions,
  both Ikana directions and the riding branches of the inner Gorman-track exits require the ability as well as the
  original song rule. The Night 2 escape via the alien-defense event remains a separate branch.
- `ObjBean` checks the bean ability when offering a planting exchange. The player's exchange code consumes the bean
  only after receiving that offer. The watering and platform branches run independently of the ability. The generator
  augments both `CAN_GROW_BEAN_PLANT` and `CAN_USE_DAY2_RAIN_BEAN`; the latter has its own beans-plus-clock branch for
  the Doggy Racetrack and upper Deku Palace. The normal water sources, Song of Storms, Hookshot/Zora alternatives,
  bean refills and the Beneath the Well trade keep their original rules.
- Item IDs are appended as 297 and 298 (AP IDs 757924137 and 757924138). The client accepts 0.1–0.11. New options and
  flags use existing save arrays. Universal Tracker restores both options and the entrance maps from slot data.

#### Verification

- `pytest test/general worlds/mm_2s2h/test -q` passed 794 tests and 151,158 subtests. Both sets of golden
  fingerprints matched with the ability options off (78/78).
- `test_abilities.py` passed 15 focused tests covering the two toggles, original prerequisites, Day 2 and Night 2,
  alternative routes, pool trimming, No Logic, Nearly No Logic, a beatable mixed fill and tracker reconstruction.
  The tracker comparison uses identical inventories, sweeping only event locations. A full CLI seed with Epona in
  `start_inventory_from_pool` placed it once in the precollected inventory, and placed one bean ability in the pool.
- `random_seeds.py` passed 100/100 with both abilities on and randomized mixed entrance pools. The rebuilt parity
  harness matched 1200/1200 scenarios with the new options off. The generator translated all rules.
- The C++ harness passed 32 hook decisions, six Romani return scenarios with and without the item, 12 Gorman fee
  scenarios with and without race skip, two planting cases and two uses of an already planted bean. The latter cover
  water contact and rain. In the Romani tests the scripted layers mount, while normal scene entry with a carried
  mount and the song leaves the player on foot without the ability. The rest of the entrance harness also passed.
- The 0.10 slot fixture came from apworld commit `8ff28914`. The C++ harness uses the 0.10 save structs and codec from
  client commit `aa6d5b494`. It preserves 49 regular pairs, 70 mixed pairs and old IDs, and loads both ability options and
  flags as off. Universal Tracker also reproduces the original connections from the fixture.
- Initial pytest runs reported `AssertionError` on the song cases and a `KeyError: 'RR_DEKU_PALACE_INSIDE'`.
  The fixtures now include the precollected ocarina buttons and use the correct region ID. The start inventory
  test uses the CLI, and the tracker test sweeps events with a fixed inventory. The first C++ harness attempts
  reported `C2065: 'f32': undeclared identifier` and failed `slot.apworldVersion == "0.10.0"`. The mocked game
  fields and the version assertion for newly generated slots were updated.
- The Windows x64 Release client built locally. The first MSVC run reported `C2855: command-line option '/Z7'
  inconsistent with precompiled header`. The build recreated the affected headers. Offline CMake configuration
  left `stb_image.h` empty, causing `C3861: 'stbi_image_free': identifier not found` and then `LNK2001` on the stale
  stb library. Restoring the pinned header from a verified local cache and recompiling `stb_impl.c` resolved those.
  The final link passed with local PDB output disabled after `LNK1103: debugging information corrupt`.

#### In-game checklist — not tested in game

- Play Epona's Song near a cow and call Epona with riding locked. Try the mount and both Great Bay and Ikana fences
  after collecting the ability. Ride through a mixed land exit, turn back on foot and verify the return point.
- Start Romani's training without the ability, finish and collect the check. Also lose or leave early with Epona's
  Song already learned. On the next normal scene entry, confirm Link is on foot and cannot remount.
- Try the Gorman race on the ordinary and alternate dialogue paths, with race skip on and off. While locked, confirm
  the fee stays in the wallet and the race does not start. Take the Night 2 track route after saving the cows.
- At the Doggy Racetrack and Deku Palace, try to plant before and after collecting the ability. Check bean ammo,
  water from a bottle, Song of Storms and natural rain at 07:00–17:30 on Day 2. Ride an already planted bean. Try the
  bean refill and the Beneath the Well trade while planting is locked.
- Save and reload with the abilities collected, reset the three-day cycle and load an existing 0.10 save offline.
  Verify its original mixed entrances, riding and planting. Check the item pickup models and text in a 0.11 seed.

### Stage 7 – entrance tracker (apworld 0.11.1)

Client only, no new ids or options. The apworld changes only where the spoiler names come from, so the version is
the patch 0.11.1; the client accepts it as `0.11` unchanged (`Archipelago.h`).

- **Window:** `Rando/EntranceTracker/EntranceTracker.cpp` (ImGui, registered in `BenGui.cpp` as
  `gWindows.EntranceTracker`, menu entry "Entrance Tracker" next to the Check Tracker in `Rando/Menu.cpp`). Settings
  are CVars: `gRando.EntranceTracker.ShowAll`, `.GroupByPool`, `.HighlightCurrentScene` (default on) and
  `.OnlyCurrentScene`. Without a rando save it says "No Rando Save Loaded", without shuffled entrances "No shuffled
  entrances in this save". Plain 2S2H rando cannot turn the entrance options on (`Rando/Menu.cpp` has none of them).
- **Lines:** `EntranceShuffle::TrackedEntrances()` lists one line per spoiler-log line. A door pool's coupled pair
  is one line "A <-> X" (temples, boss rooms, grottos, interiors, special areas). The overworld has one line per
  exit, and a mixed pool one per endpoint. Only rows whose option is on and that some other row may replace
  (`CanPair`) appear, so `INTERIOR_OWN_HOURS`, `entranceMixFixed` and the inverted temple outside the mixed pool
  stay hidden. A row that drew itself is shown, since that is part of the seed.
- **Discovery:** the new save field `u8 entrancesDiscovered[128]` (`z64save.h`) has one bit per endpoint, numbered
  `2 * row` (door side) and `2 * row + 1` (inside side) as in `mixedEntrancePairs`. Row numbers stay valid. The
  redirect that decides a transition marks both endpoints of its pairing (`Discover` in `EntranceShuffle.cpp`):
  `RedirectMixed`, the grotto hole and the grotto's way out, `RedirectInterior`, `RedirectOverworld` (the exit and
  the opposite exit of its target), `RedirectSpecialArea`, and the temple/boss loop of `OnBeforeTransitionSetup`,
  also for a row that drew itself. Marks need `respawnFlag` 0, -2 (exit stored as void-out point) or 4 (grotto
  way back). The blue-warp loop marks nothing. Loading a save never reaches the hook: `z_play.c:626`
  (`Play_UpdateTransition`) is its only caller. Rows of a mixed pool are only marked by `RedirectMixed`.
- **Save:** `BenJsonConversions.hpp` loads the field with `GetGrownArray`; saves without it load with no
  discoveries. `shipSaveInfo` survives the three-day reset, so discoveries do as well.
- **Names:** the new table `entranceNames` (`EntranceShuffle.cpp`, one row per `shuffleEntrances` row in order) holds
  the spoiler names: `name` (door pools' `_door_name`, an overworld row's place), plus the Archipelago exit names of
  both sides for mixed pools. The generator (`check_entrance_names`) aborts if a name differs from the formulas of
  0.11.0 or an exit name differs from the region graph, and writes `ENTRANCE_NAMES`. `_door_name` and `_place_name` in
  `__init__.py` read it. The exits keep the names `create_regions` gives them.
- **List logic:** `Rando/EntranceTracker/EntranceTrackerList.cpp` (no ImGui) builds the rows. With *Show All* off an
  undiscovered target is `???`. Search runs over the visible text only. The tooltip names the target's area only
  once visible. Grouping (scene of the starting side, or pool) and sorting (group, then "from") use only the
  starting side. The highlight is the current scene's doors, plus the line into the current area once it is
  visible, in grottos only the current grotto's.

#### Verification

- `test_spoiler_names.py`: `data/spoiler_entrances_0_11_0.json` holds the spoiler entrances that unchanged 0.11.0
  (apworld commit `a5e36c14`) wrote for 10 seeds: four with all six pools, three with all four mixed pools and three
  with two or three mixed pools (157 to 191 entries each). After the change all 10 match word for word. Another test
  compares every table name with the old formulas. With one name changed on purpose the generator aborts with
  `entranceNames: WOODFALL_TEMPLE:0 must be named 'Woodfall Temple', ...`.
- `pytest test/general worlds/mm_2s2h/test -q` passed 805 tests (11 of them new) and 151,151 subtests.
  Stage 4a reported 151,158 subtests; the cause of the difference was not investigated.
- Parity harness rebuilt: 1200/1200 scenarios. The generator output changed only by `ENTRANCE_NAMES`.
- C++ harness (`tools/entrance_client`, new `tracker.h`), with the real slot data of 0.9 (all pools unmixed), all 11
  mixed fixtures and the 0.9 options with every door unshuffled. 2187 lines across 13 saves. Going through a line
  arrives where the tracker says, and marks exactly its two endpoints. The way back marks the same two, in 4322
  traversals covering mixed pools, grottos (hole and way back) and rows that drew themselves. 13,122 respawns
  (flags 1, -1, -3, -6, 2, 5) and 130 blue warps (repeat warps and the first warp through the Giants' Chamber) mark
  nothing. With all options off there are no lines and nothing is marked.
- List logic: 25,080 views of two saves with the same options and discoveries but different targets (the 0.9 pairing
  against all unshuffled, and each mixed fixture against one with swapped partners). With *Show All* off the
  undiscovered rows are identical for every search (including each target's name), both groupings, a range of
  current scenes and grottos, and no, a third or two thirds of the lines discovered. Discovered rows keep their place
  in the order. With *Show All* on the views differ.
- Old save: a save encoded with the save structs and codec of client `eafa920b7` has no `entrancesDiscovered`. It
  loads with every discovery cleared and its pairs unchanged, and its 183 lines stay undiscovered. The new field
  round-trips.
- Windows x64 Release client built locally (client commit `6545724d5`). clang-format 19.1.5 is clean for the touched files. The CI runs
  clang-format 14, so `override {};` in the new header was written as `override {}`, where both versions agree.
  The first build failed with `LNK2001` on `EntranceTrackerWindow::Draw`: CMake globs new files only on
  reconfigure. After `cmake build/x64`, libultraship failed with `C2855: command-line option '/Zi' inconsistent
  with precompiled header`. Deleting the stale `cmake_pch.pch` fixed it.

#### In-game checklist — not tested in game

- Open the Entrance Tracker from the Rando menu in a seed with several pools (also with mixed pools). Unshuffled
  rows must not appear, and every target must show `???` at first.
- Go through a temple door, a grotto hole, an interior door, an overworld exit and a special-area door. The line and
  its way back show the target, and the arrival matches the tracker. Do the same from the inside first (e.g. leave a
  building you reached by warp).
- Beat a boss with shuffled boss rooms and take the blue warp. Void out, die and load a save in front of a shuffled
  door. None of these may reveal anything.
- Try the search with a target's name while it is hidden: it must find only lines that start there. Hover the
  tooltips. Switch grouping between area and pool and toggle *Show All*, *Highlight Current Scene* and
  *Only Current Scene*. The highlight follows the scene; in grottos only the current one is lit.
- Restart the game: the window settings are kept. Save, reload and reset the cycle: discoveries are kept. Load a
  save from client 0.11.0: it starts with nothing discovered.
- A save with all entrance options off shows only the note. A plain 2S2H rando save without Archipelago is not
  affected.

### Stage 8 – souls for all NPCs (apworld 0.12.0)

`shuffle_npc_souls` is a choice now: `off` (0), `classic` (1, the 30 souls of 0.2 to 0.11, also `true` in old yamls)
and `all` (2, 35 more souls, 65 in all). Plan, inventory and the code evidence for every decision:
`docs/plan-stufe8-npc-seelen.md` in the project folder (not in git). In short:

- **Table:** `npcSouls` (`Rando/ActorBehavior/Souls.cpp`) got two fields with defaults, `level` and `sceneId`; the 30
  old rows are unchanged character for character. The 35 new rows are appended with level 2. New ids are a second
  block at the end of `RandoItemId` and `RandoInf`; `IS_NPC_SOUL`/`NPC_SOUL_RI_TO_RANDO_INF` (`Souls.h`) know both
  blocks. No existing id moved (checked against the generated enums of the previous commit: only `RI_MAX` and
  `RANDO_INF_MAX` changed).
- **Shops:** EnSob1's profile id is `ACTOR_EN_OSSAN` (`z_en_sob1.c:70`), so the Bomb, Goron and Zora shopkeepers and
  the Trading Post are all `ACTOR_EN_OSSAN` at run time. Their four souls are told apart by `sceneId`.
- **Exceptions:** `npcSoulExceptions` keeps the Goron Shrine gatekeeper (`ENGO_GATEKEEPER`) visible: he opens the
  shrine (`North.cpp:141`). Sakon (opens his hideout, `z_en_suttari.c:1191` → `z_bg_iknv_obj.c:190`) and Pamela
  (opens the Music Box House, `z_en_pamera.c:193/309` → `z_en_door.c:249`) have no soul. So do the story NPCs,
  Pierre (hookshot target in exits), the Great Fairies, Epona and the shared alien actor.
- **Collision:** OBJ_UM (Cremia's cart) is the only DynaPoly actor among the 65 souls (`z_obj_um.c:648/760/763`).
  While hidden its collision is off (`DynaPoly_DisableCollision`), and on again with the soul.
- **Shadow:** `Actor_Draw` (`z_actor.c`) now draws the shadow only if `ShouldActorDraw` allowed the draw. Before, a
  hidden NPC still cast its shadow, with value 1 too. Other users of the hook: boss souls (Gyorg, Majora, Odolwa,
  Twinmold, and Goht, whose ice is drawn by the hook itself) now lose their shadow while hidden too, and so do the
  developer tools "prevent actor draw" (DeveloperTools.cpp, ActorViewer.cpp). Enemy souls only set grayscale and never
  `*should = false`, so nothing changes there.
- **Logic:** checks and events in a row need the soul, as before. Multi-source events (spring water, fish, bugs, hot
  spring water, Zora eggs) need no NPC at any source, so no region column was needed. Souls without checks or events
  (8) are `useful`. The pool trim moves them to the start inventory (warning in the log) once junk, traps and the
  Plentiful copies are gone, and only then raises OptionError; progression souls never move.
- **Pool limit (measured):** with the default checks (284 locations) `all` gives 278 items without junk. Up to 8
  items over the locations still generate (the useful souls go to the start inventory, e.g. boss souls + ocarina
  buttons: 288 items, 4 souls moved). With door keys on as well it is 304, which is an OptionError.

#### Verification

- `pytest test/general worlds/mm_2s2h/test -q` passed 832 tests and 153,924 subtests, 0 failed (805 before).
- New tests: `test_npc_souls.py` (value 1 has exactly the 30 old souls, all progression, and a check only a new soul
  would gate needs none; `classic` equals `true`; value 2 has all 65, 8 of them useful and not advancement, every
  listed check and event is out of logic without its soul and in logic with it (141 checks), beatable; No Logic with
  value 1 and 2; value 2 with every own option), `test_universal_tracker.py` (rebuild from slot data with value 2),
  `test_pool_trim.py` (4 useful souls to the start inventory with a warning, none of the progression souls;
  OptionError with door keys). The golden fingerprints (values 0 and 1 inside the rolls) matched 78/78.
- `random_seeds.py 100` with `shuffle_npc_souls=all`: **100/100**. With `all` plus every own option (interiors,
  dungeon and boss entrances, grottos, overworld, special areas, door keys, both abilities, swim,
  `mix_entrance_pools=random`): **100/100**.
- Parity harness rebuilt: 1200/1200 scenarios, 0 mismatching. The generator translated all rules; its output changed
  only by the new souls (`NPC_SOULS` with a fourth field, the level).
- Windows x64 Release client built locally. clang-format is clean for `Souls.cpp`, `Souls.h`, `Archipelago.h` and
  `Types.h`; `z_actor.c` has the same 11 upstream findings as before.

#### In-game checklist — not tested in game

- Value 2: an NPC appears, and casts a shadow, only once its soul is found; before, it is not targetable and nothing
  can be heard from it. Check at least: the carpenters in South Clock Town, the Gorons in Goron Village, and the
  Goron Elder's son in the Goron Shrine, whose crying must stop as well.
- Goron Shrine on day 2 and 3 as a Goron without the Gorons' soul: the gatekeeper must still be there and open it.
- No invisible walls: Cremia's cart (OBJ_UM) on the ranch and on Milk Road without the soul of Romani and Cremia;
  walk through where it stands. Once the soul arrives in the same scene, the cart is solid again.
- Shops: Bomb Shop, Goron Shop, Zora Shop and Trading Post each disappear with their own soul only.
- Exceptions stay: Sakon at his hideout on night 3, Pamela at the Music Box House.
- Blockers without their soul are gone and the way is free: Clock Town gate soldiers, Deku Palace guards, pirates in
  the fortress (also the beehive scene in Aviel's room), the talking Gibdos in the well, Biggoron's storm on the way
  to Snowhead, the Bombers' hideout guard.
- Value 1: exactly the old 30 NPCs are hidden, nothing else; hidden NPCs no longer cast a shadow.
- Boss souls: a boss without its soul casts no shadow.

### Stage 8c – models for the own items (client only, apworld unchanged)

No ids, options or logic changed, so no version changed. Plan with the code evidence per NPC:
`docs/plan-stufe8c-modelle.md` in the project folder (not in git).

- **NPC souls** (`Rando/DrawNpcSouls.cpp`): each soul draws its NPC's skeleton with an animation of that actor, shrunk,
  in the blue light of before. The table names per soul skeleton, animation, the segments the actor's draw function
  sets (textures stay textures, `Gfx_EnvColor` stays a color display list, end-only lists stay `D_801AEFA0`) and the
  limbs its override always hides or replaces. Group souls show one member (Romani, Lulu, the marine researcher,
  Shikashi, the Trading Post part-timer, the purple pirate, the cursed Spider House man, …). Playground and business
  scrubs share a model, and so do Gorons and the Goron shopkeeper. Goron racers are a rolled-up Goron, Kafei is the
  get-item model of Kafei's Mask (his own model is the player skeleton), the lottery is its kiosk.
- **Safety:** before `SkelAnime_Init`, skeleton and animation are loaded by name and checked (resource present, at most
  64 limbs, frame count > 0). If that fails the model stays off and only the blue flame is drawn. Each soul has one
  state, created on first use and reused (no allocation, like the enemy souls' `SETUP_DRAW`); the animation advances
  once per frame however many copies are on screen.
- **Size:** every model is scaled to the height the pirate enemy soul shows and moved down by 45 % of its height, from
  the actor scale and a rough world height per NPC. These are estimates.
- **Door keys:** Anju's room key (`GID_ROOM_KEY`) instead of the small key. **Riding Epona:** Gorman's horse
  (`gHorseBanditSkel`, idle animation), the land title deed if it cannot be drawn. **Planting beans:** the magic bean,
  as before.
- **Quick look:** Save Editor → "Queue Randomizer Item Gives" (`DeveloperTools/SaveEditor.cpp`), filter "Soul of": each
  button plays the get-item animation with the model above Link's head. It really gives the item, so use a test save.

#### Verification

- Windows x64 Release client built locally (client commit `6083c5e94`), no errors, no warnings in
  `DrawNpcSouls.cpp`. Every skeleton, animation, texture and display list name of the table resolved at compile
  time against the asset headers of 5.0.1.
- clang-format clean for `DrawNpcSouls.cpp`, `DrawNpcSouls.h` and `DrawItem.cpp`.
- `pytest test/general worlds/mm_2s2h/test -q`: 832 passed, 0 failed (apworld unchanged).
- Nothing of this was run in the game: size, position, eyes and the fallback are unchecked.

#### In-game checklist — not tested in game

Every model below is not tested in game.
- Each of the 65 souls once via "Queue Randomizer Item Gives" with a test save: the NPC (or kiosk, mask, rolled-up
  Goron) appears above Link's head in a blue light, with eyes (no garbage textures), without missing head, and about
  the size of an enemy soul. Note every soul that shows only the flame, crashes or looks wrong.
- A shop with a soul (also several souls side by side): nothing reaches through the shelf or into the next item,
  no stutter.
- A soul as an item on the ground (e.g. a freestanding check): it stands above the floor, not in it, nothing reaches
  through a wall.
- A door key and the riding ability: room key and horse, on the ground and in the get-item animation.

### Stage 8b – open object check markers

`Rando/ObjectMarker.cpp` checks the shuffled and obtained flags in the current save. It supplies the fixed yellow
marker and uses the same item conversion and CSMC classes as pots. The draw wrapper disables grayscale in every
display buffer it tints before control returns to the actor renderer and its shadow pass. The engraving and music
wall use the Wonder Item sparkle sprite on a fixed pulse. The music wall keeps its marker when the original notes
disappear after playing the song. Objects with several checks use the fixed yellow marker in both CSMC modes.

#### Verification

- The Windows x64 Release client built locally at client commit `661b00c0c`.
- `pytest test/general worlds/mm_2s2h/test -q` passed 832 tests and 153,932 subtests. The initial run failed in
  `TestSettingsSave.test_save` with `PermissionError: [Errno 13] Permission denied` on `host.yaml.tmp`. Using `TEMP`
  and `TMP` inside the workspace resolved it.
- The rebuilt parity harness matched 1,200 of 1,200 scenarios with `compare.py 150`.
- clang-format passed for all eight changed client files.
- CMake reported `curl: (6) Could not resolve host: raw.githubusercontent.com` and left `stb_image.h` empty.
  The local build used the SHA-256-verified pinned header from an existing cache. MSBuild stalled at
  `Checking Build System` inside the sandbox, and a single-node retry reported `C1902: Program database manager
  mismatch`. Building outside the sandbox passed that step.
- MSVC reported `C2664` on the sparkle display-list handle and `LNK2001` on `FrameInterpolation_RecordOpenChild`
  and `FrameInterpolation_RecordCloseChild`. The draw uses the usual cast for an asset handle and uses the
  engine's C-linkage declarations at file scope.

#### In-game checklist — not tested in game

For **every row**, check the marker before collection, switch CSMC on and off, then collect the check and confirm
the normal appearance in both settings. Also check the same object with its shuffle option off where applicable
and in a non-randomized save. Use nearby unshuffled or collected objects and shadows to spot any color bleed.

| Object | What to check while open and on collection |
|---|---|
| Small snowball | Yellow with CSMC off and tinted by item class with CSMC on. Check while it is held and again after collecting the drop and respawning the snowball. |
| Large snowball | The same tint as small snowballs. Check the normal break animation and that an intact snowball looks normal after collecting its drop and reloading the room. |
| Trees | Check woodland trees, palms, and snow-covered trees. Both trunk and foliage show the marker, with item-class tint under CSMC. Bonk the tree, collect the drop, and check that the same standing tree returns to normal. |
| Tree with the two Termina Field rupees | Fixed yellow with either CSMC setting. Collect each rupee first in separate trials. The marker stays until the other is collected, including after a scene reload. |
| Beehives | Yellow or item-class tint while hanging and swinging. Check normal appearance when respawned after collection. Include the cow grotto variants and the Pirates' Fortress hive with its existing drop fallback when the hive is already gone. |
| Swordsman's School gong | Yellow or item-class tint while intact, including while the gong and chain are moving. Collect the gong's drop and check that they return to normal in the same room. |
| Owl statues | Yellow or item-class tint on the statue model. Activation flashes keep their normal colors. The statue looks normal when the check is collected. |
| Termina Field music wall | Fixed yellow sparkle with either CSMC setting, independent of the reward timetable. Play the song and verify that the marker persists when the notes disappear. Collect the reward groups over several visits and confirm the sparkle ends only after the last open check. The Milk Bar music staff keeps its normal appearance. |
| Song of Soaring engraving | Yellow sparkle with CSMC off and item-class sparkle with CSMC on. Read the engraving, collect the check and verify the sparkle disappears immediately. |

For CSMC colors, compare examples of major, minor, health, boss key, mask, token, small key, stray fairy and filler
with the corresponding pots. Check pots, crates, barrels, grass and the original Wonder Item sparkles as well.

### Stage 9 – Shuffle Rocks (apworld 0.13.0)

`shuffle_rocks` defaults to off. The 122 named locations in `Rocks` comprise 56 small rocks, 20 blast boulders,
seven bronze boulders, 24 stones from three rock rings and 15 rolling boulders. The inventory covered 299 rooms and
414 headers of the NTSC-U 1.0 ROM. All 392 nonempty actor lists matched the extracted archive. The client uses the
same scene, room and actor-list-index identification as 2S2H's pots and grass, with layer aliases and ring child
indices where needed. There is no ROM-version distinction, as with pots and grass. GameCube data had no separate audit.

| Placements left out | Reason |
|---|---|
| 9 silver boulders, Great Bay Coast | Lifting requires Goron form. They sit 175–442 units underwater, below wading depth. `z_en_ishi.c` offers the lift and `z_player.c` restricts the form and water state. |
| 30 small rocks at Great Bay Coast and 2 at Pinnacle Rock | The water boxes and floor geometry put the rocks in deep water. A repeatable route to break each rock and collect its drop is unverified. The shallow coastal rocks are included. |
| Powder Keg barriers at Milk Road and the Goron Racetrack (winter/spring) | `Obj_Hugebombiwa` opens the ranch or racetrack and participates in the Powder Keg test and its cutscene. |
| 2 submerged boulders below Deku Palace lily pads | Both sit under the lily pads with shared switch parameters. The lily-pad route and collection underwater need game testing. |
| 1 boulder in the Swamp Spider House monument room | It shares its position with the bean and reward actors. The puzzle uses the original actors and switches. |
| 12 cliff rocks in Termina Field and 5 in Ikana Graveyard | The levitation flag selects the special collectible path and cutscene state in `En_Ishi`. Some of the fixed Termina Field drops are already Wonder Item checks. |
| 1 Termina Field boulder exclusive to cutscene layer 2 | It has no regular gameplay placement. |
| 12 rolling snowballs, 1 indestructible rock in Deku Shrine, boss debris, tombstones and temple scenery | These use snowball, hazard, puzzle or cutscene behavior. Only the breakable rolling-rock model of `En_Goroiwa` is included. |

Normal boulders covering grotto entrances or the Zora Cape fairy fountain and the boulders near the Lone Peak Shrine
chest can be checks. The original switches, collision, animations, exits and chests still run.

#### Access rules and placement windows

- `Rando/Logic/ExtraChecks.inc` is the shared table for names, checks, regions, access conditions, half-days and
  layouts. The generator validates the rows and appends them to `REGIONS`, retaining the original region files and
  `Logic.h`. The same table can take future check types. AP's regular check-type filter omits rocks when disabled.
- `z_actor.c` decodes the half-day bits from the room ActorList. The ordinary placements cover all six playable
  half-days (`0x3F` in table order D1/N1/D2/N2/D3/N3). The two rolling Road to Ikana rocks have raw mask `0x2AA`,
  which becomes `0x15` (D1/D2/D3). The generated rule passes the table mask via `ExtraCheckAvailable` to
  `time_logic.py`. Its filter intersects the placement window with the available time slices and clock mode.
- `z_play.c` selects spring scenes after clearing Snowhead. The 13 spring-only rows require
  `RE_CLEARED_SNOWHEAD_TEMPLE`. Five Zora Cape rocks have a base-layout placement only. The placement test
  excludes them in the cleared layout. AP reachability includes opportunities across cycles, so those five
  checks remain available in a fresh cycle after beating Gyorg. Aliases for the same physical stone share an ID.
- Land lifts use `CAN_BE_HUMAN || CAN_BE_GORON || CAN_BE_ZORA`. AP creates the file as Human Link, and the
  rooms entered in another form have solid ground to change form at the stone. The coast's deeper wading
  placement needs Goron or Zora form. The two high spring Mountain Village rocks use the Lens and magic,
  as does the waterfall chest. Bronze and rolling rocks use `CAN_BE_GORON`. Blast boulders use the explosive
  macro, with the Powder Keg as a Goron for the explosive-only type. The other blast type also accepts a
  Goron punch or pound.

#### Drop and save behavior

`Rocks.cpp` binds the check to the actor before it can change rooms while held. The lift or confirmed break
sets a pending bit and drops a `CustomItem` at ground level. A rolling boulder struck over a ledge can use Link's
position if the ground at the impact is too far below. The small rock's first throw suppresses its random drop
when the lift used the check. Picking up the check queues the same `eligible` flag as a pot. A pending drop can
be restored at the original placement when the room is reloaded, including boulders hidden by a switch. Retry
checks the live item list and the eligibility and obtained flags before spawning. A ring uses the spawner's
placement and child slot, and a rolling boulder uses its placed actor index. Newly created instances repeat the
same lookup and an obtained check never binds or spawns again. Older saves load the expanded check array and
default to no pending rock drops. The existing `ObjectMarker::AttachTint` colors open checks.

#### Verification

- Cross-checked all 149 client placement rows with the extracted ActorLists and all 122 generated windows with
  the placements. There are 102 unrestricted checks, 13 spring-only, five base-layout-only and two daytime-only.
  The generator reports 2,763 locations and zero untranslated rules.
- A C++ harness using the drop code with mocked actor spawning passed 122 lifecycles, including all 24 ring
  children and 15 rolling boulders. It exercised a failed spawn and retry, duplicate suppression while a drop
  is live, recovery after leaving, collection, and a new actor after collection. The save codec preserved 2,642
  pre-existing check records and defaulted all 122 appended records. The pending field also round-tripped.
- The Windows x64 Release build completed. The first sandboxed attempt stalled at MSBuild startup. CMake reported
  `curl: (6) Could not resolve host: raw.githubusercontent.com`; the build used the pinned, SHA-256-checked
  `stb_image.h` from an existing cache and ran MSBuild outside the sandbox.
- The rebuilt parity harness matched **1,200/1,200** cases with the fork options off. clang-format passed for
  21 complete files and the changed line in the save header.
- `pytest test/general worlds/mm_2s2h/test -q --disable-warnings` passed **861 tests and 159,978 subtests**.
  The existing seed fingerprints matched **78/78**. The rocks tests cover off, reachability, No Logic, Universal
  Tracker and the combination of all fork options. The availability test checks all 122 rocks against six
  half-days and both Snowhead and Great Bay layouts, for 2,928 combinations.
- `random_seeds.py` generated **100/100** with rocks as the only forced-on fork option, and **100/100** with rocks
  and all fork options enabled, including all NPC souls and random mixes of entrance pools.
- The first pytest run reported 78 Golden fingerprint mismatches (first: `fe9b6df368153389 != 8793db5584e70b89`)
  and five `OptionError: mix_entrance_pools needs at least two pools` failures in the new test. The seed driver now
  has an independent roll for rocks, the old fingerprints skip the added off-valued slot option, and the test
  passes the list of active pools.

#### In-game checklist — not tested in game

- Lift a small rock, then throw it. Confirm one check drop at the original position, one award on pickup and
  ordinary behavior when the rock respawns. Check a stone that spawns bugs, and a Goron or explosive hit on a
  small stone. Try picking up a small rock as Deku, then change to a form that can lift it.
- Leave the item on the ground, exit the room, and reenter. Collect the recreated item, then reload the scene and
  reset the cycle. Confirm it pays only once. Repeat for a switch boulder and a drop left uncollected across a save load.
- Break two neighboring ring children, leave their spawn radius, and return. Confirm each child gets only its
  own check. Repeat in the other graveyard header. Break and revisit a rolling boulder with a drop near a ledge.
- Check the Road to Ikana rollers during day and night, the mountain placements before and after Snowhead,
  and the five base-layout rocks at Zora Cape before Gyorg, after Gyorg and in the next cycle.
- With CSMC on and off, confirm an open check is marked and the marker disappears after pickup, including on
  a held rock or a respawned child. Check with the option off and in a vanilla save.
- Break both types of blast boulders and a bronze boulder with the supported hit types. Confirm that the
  grottos, Lone Peak Shrine chest, fairy fountain, ranch, racetrack and the spider-house bean puzzle still work.

### Clock Shuffle startup

On the first load of a new Archipelago save with Clock Shuffle, file select waits for the initial server inventory.
The client grants clocks in their server order before the normal cycle-start check. The save keeps an item index so
the usual gameplay receipt can catch up without granting a progressive clock again. An empty clock inventory uses
the configured Final Hours start. Press B to exit the wait and return to file select.

The client C++ harness passed 12 startup cases covering all six half-days, delayed inventory, both progressive
orders, save reload during receipt, an empty clock inventory, older saves and the disabled option. The Windows
x64 Release build and clang-format for the changed lines passed. The AP suite passed 861 tests and 159,982 subtests.

#### In-game checklist — not tested in game

- Create a new AP save with `clock_shuffle` and a starting clock. Confirm that gameplay opens in the starting
  clock's half-day. Repeat with a night clock and a slot that already received an earlier clock from another player.
- Repeat with a progressive clock. Confirm that it unlocks one half-day on the initial grant and stays at one
  after the AP item receipt catches up. Save, reload and verify the number of owned half-days.
- With a test slot that has no clocks, verify the configured Final Hours start. Open an older AP save, a native
  rando save and a fresh AP save without Clock Shuffle to check their usual load behavior.

### Stage 10 – Shuffle Signs (apworld 0.14.0)

`shuffle_signs` defaults to off. The `Signs` group has 87 checks from 133 placements, including 56 wooden signs,
27 wall signs and notices, and four epitaphs. The inventory covered 299 NTSC-U 1.0 rooms and 414 headers. The 392
nonempty ActorLists matched the extracted archive in order and raw values. The client identifies them by scene,
room and ActorList index, as 2S2H does for pots and grass and as Stage 9 does for rocks. Actor type, params and
position verify the placement, including aliases for alternate headers and seasons. There is no ROM-version
distinction or separate GameCube audit. See `Rando/ActorBehavior/SignPlacements.inc` and the scene-by-scene count
in `docs/plan-stufe10-schilder.md` in the project folder.

| Exclusion | Reason |
|---|---|
| Kafei's diary, Granny's diary and the Bomb Shop memo | Five `EN_TALK` placements contain private writings. Public posters, signs, schedules and the Trading Post display label are included. `z_en_talk.h:6`, text IDs `0x1C04`, `0x1C05`, `0x1C16`, `0x1C08`. |
| Two Music Box House posters | `OBJ_MU_PICT` drives Pamela's spoken tour and moves her during the text. `z_obj_mu_pict.c:61,98,120,124,136`. |
| Graves opened by Stalchildren and the invisible fight prompt | The 13 entries share the puzzle state, disappear when a grave switch is set, or cue the fight. Six `0xFFFF` entries represent two independent, readable soldier graves and are included. `z_obj_hakaisi.c:61,91,103,152,211`, `z_en_rail_skb.c:577`. |
| Gossip Stones, owl statues, Tatl and NPC text, Song of Soaring engraving, Swordsman's School board | These retain their own hint, activation, reward or combat behavior. `actor_table.h:255,537`, `ActorBehavior/EnTimeTag.cpp:35`, `z_obj_kendo_kanban.c:471`. |

#### Access and layout

`Logic/ExtraChecks.inc` supplies the same rules and half-day masks to the client and apworld. Woods of Mystery has
three exit signs on each day, in rooms 7, 3 and 5 respectively. The Gorman rear gate sign is present on day and
night 3. The east Post Office schedule is present on the first five half-days, intersected with the usual opening
hours. The west schedule uses all six. The Goron epitaph requires the Goron form to read the translated inscription
(`z_obj_ghaka.c:77`). Mikau's grave uses the existing healing requirements plus the Indigo-Go's soul in All mode,
just as the Mikau check does. The grave spawns after healing in the current cycle (`ActorBehavior/EnZog.cpp:8`,
`z_en_sekihi.c:114`, `Regions/West.cpp:128`). The Deku Palace signs inherit the approach to the lower courtyard.

| Shore placements | Region, condition and evidence |
|---|---|
| Great Bay Coast Fisherman's Hut, Pirates' Cove, Indigo-Go's, Great Bay Temple and Murky Water (`0x0321/22/25/26/27`) | `RR_GREAT_BAY_COAST`. Their floor triangles share the mainland component with the Termina Field entry and the route to Zora Cape, even when the collision analysis cuts connections below Y = −10. The Pirates' Cove **sign** attaches to that mainland. The underwater entrance to the fortress has its own rule. `Regions/West.cpp:128,147,151,158,220`. |
| Marine Research Lab (`0x0323`) | `RR_GREAT_BAY_COAST_MARINE_LAB_EXTERIOR`. At (−3088, 240, 4011) it shares the island's Y = 240 floor with the lab entrance spawn and its exterior pots in both layouts. The coast-to-island link requires swimming. `Regions/West.cpp:157,161,172`. |
| Fisherman's Jumping Game (`0x0312`) | `RR_GREAT_BAY_COAST` with the Gyorg clear event. The sign stands at (−2398, 15, 649) on the mainland component and appears only in room 0, layer 1. The separate timed minigame and its Hookshot access are on the platforms. `z_play.c:2290`, `Regions/West.cpp:184,193,209`. |
| Zora Cape Fall Headwaters, Great Bay Temple and Zora Game Site (`0x0328/2A/2B`) | Fall Headwaters stands on the Y = 40 beaver bank with the two nearby pots. Its isolated bank uses the stricter swimming rule in `RR_ZORA_CAPE`. The temple sign shares the Y = 14 island floor with the owl and temple spawns, so it uses `RR_ZORA_CAPE_BEFORE_GREAT_BAY_TEMPLE`. The game-site sign shares the dry beach with the Great Bay Coast entry and uses `RR_ZORA_CAPE`. `Regions/West.cpp:286,299,340,346,357,363`. |
| Ikana Canyon Secret Shrine (`0x0333`) | `RR_IKANA_CANYON_LOWER` with swimming. At (−2980, −505, 3087) the sign shares the isolated Y = −505 strip with spawn 12 at (−3068, −505, 2690), 15 units above the water. The canyon uses the same swimming condition to reach the shrine entrance. The internal Light Arrow gate belongs to the shrine's next region. `Regions/East.cpp:141,158,378,411`. |

The five mainland Great Bay Coast signs listed together and the three Zora Cape signs use the pre-Gyorg layout.
The logic also considers the next cycle, as for the rocks. No shore placements needed a further exclusion after the
collision and spawn comparison.

#### Text, hints and delivery

`Message_StartTextbox` supplies the exact actor and initial text ID after the existing message hooks run.
`ActorBehavior/Signs.cpp` marks the bound check eligible. The normal check queue waits for the original message
to close and sets the check obtained on item delivery. A waiting item actor and incoming AP item application
also wait during the sign conversation. The queued item can retry if its actor fails to spawn.

The West Clock Town Bank sign `0x1C14` keeps the existing exact hint and its `ApMapping::HintCheck` call
(`MiscBehavior/BankSignHint.cpp:20,42`). The soldier recruitment posters `0x1C06` in East and South Clock Town
and the South Clock Town Bank sign `0x1C18` retain their multi-page Remains and Transformation hints
(`ActorBehavior/EnTalk.cpp:55,130,141`). Those hints describe areas and already use the in-game text path.
The sign award follows the final page of each hint. The Soaring engraving keeps its own check and hint.

Chopping an open wooden sign leaves the original post readable with the standard text flow. Cutting still spawns
pieces. The original sign returns to its normal target and read behavior after the check is obtained.
Reloading the scene spawns a whole sign in layouts that include it. Vanilla also restores it when Link moves more
than 500 units away. `z_en_kanban.c:166,217,232,256,416`. The marker uses tint for the models and a sparkle for wall
text and cut wood. It disappears as soon as the check is obtained.

#### Verification

- The generator reports 2,850 locations and zero untranslated rules. All 133 client placement rows and 87
  generated half-day masks match the ROM. All 2,764 existing check enum entries and 2,763 existing location records
  retained their values. The existing 78 Golden fingerprints and their test code are unchanged.
- The full local suite (`pytest test/general worlds/mm_2s2h/test`) passed **891 tests and 164,087 subtests**.
  Sign coverage includes the disabled option, reachability, 1,044 half-day/layout combinations, No Logic,
  Universal Tracker regeneration and all fork options together.
- A C++ harness with mocked actor and save state exercised 133 placements, 100 fallbacks to the base header and 97
  wooden-sign stump interactions. It checked the initial text ID, repeated eligibility, the obtained state,
  target restoration for the spent stump and deferral of an item actor that has not started its give yet.
- Parity passed **1,200/1,200**. The tracker matched AP in **400/400** snapshots and **50/50** exit graphs for
  each entrance mode (separate and mixed). It also matched in **29/29** focused cases, including signs as the only
  fork option, No Logic, missing form or soul, Gyorg, half-days, and an already obtained sign. Native comparison
  output was identical for both ability settings.
- Seed generation passed **100/100** with signs as the only enabled fork option and **100/100** with all fork
  options enabled, including the mixed entrance pools and all NPC souls. The Windows x64 Release build passed.
  clang-format passed for the changed ranges and new files across 21 client files.
- The first pytest runs found an incorrect bank fixture name (`KeyError`), a missing `snowhead_cleared` argument
  (`TypeError`), missing inventory in the Mikau fixture (`AssertionError: False is not true`), and an old test
  that required every default option in slot data (`RO_SHUFFLE_SIGNS` was the only missing key). The final tests
  cover the intentional omission when off and transmission when on. Older slots still default to off.
- The first seed runs reported `PermissionError: [WinError 5] Access is denied` in the default sandbox temp
  directory. Both runs passed with workspace-local `TEMP` and `TMP`. The initial client build stalled in the
  sandbox. CMake also left an empty `stb_image.h`, producing `C2065: 'STBI_rgb_alpha': undeclared identifier`
  and `C3861: 'stbi_load_from_memory': identifier not found`. The successful build used the pinned GitHub header
  and ran MSBuild outside the sandbox. clang-format also flagged the standalone end comment of
  `SignPlacements.inc` until it was formatted.

#### In-game checklist — not tested in game

- Read a wooden sign, wall sign and each kind of grave. Verify the original text or menu closes before exactly
  one award, with and without Z-targeting. Read again, reload and reset the cycle. Try with `shuffle_signs` off.
- Chop an open sign until only the stump remains. Read it with and without Z-targeting. Confirm the same text and
  item flow, then the normal stump behavior after the award. Try reloading the scene or walking 500 units away
  before reading. Confirm the pieces have no checks and never lose the original check.
- With the West Bank hint enabled, read the sign, confirm that the bank reward hint reaches Archipelago, and
  collect the separate sign check after the text. Repeat for every page of the Remains and Transformation hints
  in Clock Town. Send an AP item during a page and during the sign award. Check repeated reading, an already
  queued reward, and leaving or reloading the scene while a sign award is pending.
- Verify that the marker vanishes on award in both CSMC modes. Check an intact and cut wooden sign, a wall notice
  and a grave. Check the tracker before and after the award and while out of logic, including with mixed entrances.
- Verify the Woods, Gorman and Post Office half-days, Swamp and mountain variants, both Great Bay layouts, the
  swimming approaches to the beaver bank and Secret Shrine, Darmani in Goron form, and Mikau after healing.
### Stage 11a – Shuffle Owl Warps

`shuffle_owl_warps` defaults to off. With it on, the seed stores a permutation of the ten selectable owl
destinations with no fixed points. Normal Soaring, Skip Soaring Cutscene and Pause Owl Warp resolve the same
destination. Internal dungeon returns, owl stone activation, owl checks and physical owl saves use their existing
paths. The target's swamp and mountain variants follow the cycle's completion flags. Each travel entry is
discovered on arrival. Older saves load with no map or discoveries. The Universal Tracker replays the saved map.

`StartingItems.default` in `options.py` includes the Song of Time, but `_starting_items` in `__init__.py` uses the
supplied item dictionary. With an empty dictionary the song can be in the pool, including in No Logic.
Button shuffle can also leave a player with the song but no way to play it (`Logic.h`, `canPlaySong`).
A playable Song of Time lets Link leave every owl arrival. Four arrivals need an alternate escape when that song
cannot be played. The Great Bay lab platform needs Swim, the Zora Cape platform needs Zora, the Woodfall owl
platform needs Deku, and the Stone Tower owl needs Hookshot. These rules come from `owlWarps` in
`EntranceShuffle.cpp`. The selection plays the usual error sound when the escape rule fails. It gives no destination
hint. The generator checks the source song and activation rules against `Logic.cpp`. The C++ table supplies the
Woodfall owl platform region, which the upstream logic leaves unassigned. The off path preserves that upstream graph.

#### Verification

- Pytest passed 901 tests and 164,105 subtests. The 78 Golden fingerprints are unchanged.
  Random generation passed 100/100 seeds with Owl Warps
  enabled and 100/100 with all fork options enabled, including the mixed entrance pools.
- Parity agreed in 1,200/1,200 scenarios. The tracker comparison agreed on 200 entrance graphs,
  1,600 inventory snapshots and 29 targeted cases.
- The native entrance harness exercised 10 owl arrivals, 10 physical owl-save arrivals and 20 void/death
  transitions. It also covered escape requirements in No Logic, discovery, JSON round-trips and a legacy save
  without the new fields. Windows x64 Release built successfully. Clang-format passed on the changed lines in
  17 files.
- Initial runs hit Windows `PermissionError: [WinError 5] Access is denied` during tempfile cleanup and
  MSVC `C1902: Program database manager mismatch` inside the sandbox. The retries used a workspace temp directory
  for Python and an unsandboxed MSBuild invocation. An initial parity run exited with `3221226505` because
  the harness's new warp-map field collided with the existing owl-inventory field. The fields now have distinct names.
  The native harness needed a single shared mock for `CHECK_WEEKEVENTREG` after MSVC reported `C2143`.
  The first full pytest run also found that the slot-data defaults test needed to accept the absent disabled option.

#### In-game checklist — not tested in game

- Travel using the normal song, the skipped sequence and the pause-menu override. Compare all three to the
  seed's Entrance Tracker. Check unexplored rows, search and tooltips before and after the first arrival.
- Warp into the cleared swamp and spring mountain village. Save at the actual owl, reload offline, then try
  both a void and a death. Confirm the arrival is neither redirected again nor marked as a different discovery.
- In a seed with empty starting items or shuffled buttons, try a blocked flight. Then acquire the appropriate
  escape item or the playable Song of Time and try again. Repeat in No Logic and with the option off.

### Stage 11b – Shuffle Spawn

`shuffle_spawn` defaults to off. The seed picks North Clock Town (`NORTH_CLOCK_TOWN:2`), lower West Clock
Town (`WEST_CLOCK_TOWN:1`) or lower East Clock Town (`EAST_CLOCK_TOWN:1`). The selected district and South
Clock Town keep their reciprocal, unconditional exits even when mixed with other pools. The generator verifies both
directions. The apworld replaces the cycle-start edge from Menu, and the Universal Tracker restores the slot's
`spawn_target`. The in-game Check Tracker follows the same edge. The Entrance Tracker reveals the district only
after arrival. Older slots and saves use South Clock Town.

AP file select queues the first spawn only after the usual starting-clock wait accepts the inventory. On the Song of
Time path, the normal cycle reset and save run before the entrance and `0xFFF7` transition are replaced. The client
uses the earliest owned half-day and invokes the normal Day Telop before spawning in the new district at dawn.
With no owned clock, it uses the configured Final Hours time. The Clock Tower opens from the day and time when the
player enters South Clock Town after midnight. The route requires no equipment or key. Saving at an owl, pause
saves, void, death, the midnight festival transition and the moon crash keep their own entry paths.

#### Verification

- 907 pytest tests and 164,102 subtests passed. All 78 Golden fingerprints were unchanged.
- Random generation passed 100/100 seeds with Shuffle Spawn enabled and 100/100 with all fork options enabled.
- Parity matched 1,200/1,200. The tracker matched 400 exit graphs, 3,200 inventory snapshots and 29
  targeted cases, including separate and mixed pools with both Shuffle Spawn and Shuffle Owl Warps.
- The native harness passed 48 cases for file load, rewind, the six half-days and Final Hours, including
  the midnight and 05:59 boundaries of the tower's open check. It also checked the reserved walks, a
  legacy save and an owl load. Windows x64 Release and clang-format for the changed lines passed.
- The first shim build reported `C2039: 'entrance': is not a member of MockSave` because it included the
  file-select hook. The first native build reported `C2365: redefinition` from duplicate type declarations.
  The shim now compiles only the spawn logic and save setter, and the native harness uses one source of types.

#### In-game checklist — not tested in game

- Start new files in each district with Clock Shuffle off, a day clock, a night clock and delayed starting items.
  Check that cancelling the initial wait returns to file select and reopening applies the same district.
- Play the Song of Time, save and reopen the cycle. Verify the district, time, player form and normal Day Telop.
  Repeat after leaving South Clock Town and with the Clock Tower cutscene enhancement on and off.
- With the shortest configured Final Hours window, take the direct exit to South Clock Town and check the previously
  opened tower, the rooftop and the Moon. The remaining travel time depends on the chosen terminal time.
- Save at an owl or using the pause menu, reload and die or void out. Verify that only the next cycle start uses
  the shuffled district and that the tracker reveals it after actual arrival.
