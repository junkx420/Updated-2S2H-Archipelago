# mm_2s2h – Majora's Mask (2 Ship 2 Harkinian)

Unofficial fan project, see the `main` branch README of `junkx420/Updated-2S2H-Archipelago`.

## Downloads

The regular release is **0.1.5** (bundled apworld **0.1.4**). The **0.4.0 pre-release** packages this world's
current features with a matching new client: NPC souls, temple entrances and boss entrances, all off by default.
The new mechanics are **not tested in game**. Platform builds and preview publication run on GitHub Actions.
See the [download overview](https://github.com/junkx420/Updated-2S2H-Archipelago#choose-your-download),
[changelog](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/main/CHANGELOG.md) and
[publication progress](https://github.com/junkx420/Updated-2S2H-Archipelago/actions/workflows/publish-preview.yml).
Use the client and apworld from the same release; the release manifest records their exact commits.

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

- **NPC souls** (`shuffle_npc_souls`, apworld 0.2): 30 items `RI_SOUL_NPC_*`, appended to the enums so no existing
  id moves. The table `npcSouls` in `Rando/ActorBehavior/Souls.cpp` names, per soul, the actors it brings to life and
  the checks and logic events that need it. The generator writes it to `NPC_SOULS` and `__init__.py` adds the soul to
  those rules. To add or change a soul, edit that table and regenerate.
- **Dungeon entrance shuffle** (`shuffle_dungeon_entrances`, apworld 0.3): the doors of the four temples lead into a
  random one of them, coupled (the temple's exit and its boss's blue warp lead back out of the door used). The table
  `shuffleEntrances` in `Rando/MiscBehavior/EntranceShuffle.cpp` has one row per door (option, way in, way out,
  scenes, boss room and where its warp lands); the generator checks every row against the region graph and writes it
  to `ENTRANCE_SHUFFLE`, `connect_entrances` shuffles the rows of each enabled option with Archipelago's
  `entrance_rando`, and the pairing travels as `entrance_pairs` in the slot data into the save
  (`RandoSaveInfo::entrancePairs`). The client redirects at one place, the new `BeforeTransitionSetup` hook at the
  start of `TRANS_MODE_SETUP` in `z_play.c`. A further group of doors is new rows plus a new `RO_*` option. The
  inverted Stone Tower exterior and the Woodfall prison door are not shuffled. Boss rooms have their own option below.
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
  the temple containing the door used. The inverted temple's exterior remains fixed. If neither the boss nor its
  containing temple moved, the original warp and cutscene remain intact. Otherwise the completion cutscene is skipped,
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
- **Pool trimming** (apworld 0.7.1, no new ids or options, client unchanged): when the item pool is larger than the
  shuffled locations, `create_items` removes junk (as before), then traps, then extra Plentiful Items copies (picked
  at random; every original item stays). Each trimmed trap/copy count is logged as a warning. Only if the pool is
  still too large without all of these does it raise `OptionError`, naming the counts and the options that help.
  This path is only reached where 0.7.0 raised `OptionError`, so every seed that generated before is unchanged
  (`test/test_pool_trim.py` pins 38 fills from 0.7.0).

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
