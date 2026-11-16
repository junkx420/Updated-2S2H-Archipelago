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
