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
