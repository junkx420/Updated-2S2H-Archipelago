# Testing mm_2s2h (Phase 3 results, 2S2H 5.0.1)

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
  clear `OptionError` naming the counts.

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
