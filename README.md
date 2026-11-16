# Updated 2S2H Archipelago (unofficial, work in progress)

> **Unofficial fan update.** This repository is **not** made by, affiliated with, or endorsed by the
> Harbour Masters, the 2 Ship 2 Harkinian team, xirba13, the MMRecompRando team or the Archipelago project.
> It is **not officially supported**. Please **do not** send support requests or bug reports about anything in
> here to the original projects.

This connects [2 Ship 2 Harkinian](https://github.com/HarbourMasters/2ship2harkinian) (**2S2H 5.0.1**) to
[Archipelago](https://archipelago.gg) with **every option of 2S2H's own randomizer**, including grass, pots,
crates, barrels, beehives, snowballs, trees, enemy drops, butterflies, frogs, owl statues, boss and enemy souls,
time (clock) shuffle and ocarina buttons.

**Status: feature-complete for the planned scope, but barely play-tested.** Generation, the test suites and the
Windows/Linux client builds pass, the logic matches 2S2H's own logic in every compared case, and first play
sessions worked, including a two-slot multiworld (items sent to and received from another world). Things may still be broken, seeds may turn out unbeatable in game, and save files or
generated games may not carry over between versions of this repository. Use at your own risk.

## Branches

| Branch | Contents | State |
|---|---|---|
| `client` | 2 Ship 2 Harkinian **5.0.1** with the Archipelago client (client layer ported from `xirba13/2ship2harkinian-archipelago`, branch `archipelago`) | Builds on Windows (MSVC) and Linux in GitHub Actions. Reads all options from the slot data. Expects apworld **0.1**. |
| `apworld` | Archipelago with the `mm_2s2h` world in `worlds/mm_2s2h` | apworld version **0.1.1**. Logic generated from the 2S2H 5.0.1 sources. |
| `main` | This README only | |

Client and apworld have to come from this repository **together**. The client checks the apworld's major.minor
version and refuses seeds of other versions. Neither side is compatible with MMRecompRando (`mm_recomp`) seeds.

## How it works

- **The logic is generated, not hand-written.** `worlds/mm_2s2h/tools/gen_from_2s2h.py` reads 2S2H's
  `Rando/Types.h`, `StaticData/*.cpp`, `Logic/Logic.h` and `Logic/Regions/*.cpp` and writes the regions,
  2641 locations, 508 rules, 120 events, the items and the option defaults. A new 2S2H release means: run the
  generator, review the diff. Only the save-state layer (`primitives.py`, a port of `GiveItem.cpp`), the options
  and the pool construction (`GeneratePools.cpp`, `StartingItems.cpp`) are written by hand.
- **IDs are the 2S2H enums.** AP location id = base + `RandoCheckId`, AP item id = base + `RandoItemId`, so the
  client needs no translation tables. Options travel as native `RO_*` values and land 1:1 in the save.
- **One reporting path for every check type.** A check is shuffled exactly when it is an AP location; every type
  (chests, grass, pots, ...) reports through 2S2H's own `CheckQueue`.
- **Time logic:** 2S2H tracks reachable time per region. For the 5.0.1 region graph that mask is the same for
  every region, so time operators depend only on the collected clocks. The generator guards this assumption.
  See `worlds/mm_2s2h/docs/time-logic.md`.

## What is in it

- All check pools of the 2S2H randomizer: songs, owl statues, shops, Tingle maps, boss remains, cows, enemy drops,
  frogs, butterflies, freestanding items, wonder items, Gold Skulltulas (with shuffled count), pots, crates,
  barrels, snowballs, grass, trees, beehives.
- All item pool options: boss souls, enemy souls, swim, ocarina buttons, Sun's Song, Song of Double Time,
  Inverted Song of Time, Saria's Song, Tycoon's Wallet, Skeleton Key, traps, plentiful items, Triforce Hunt.
- Time shuffle (any order / progressive ascending / descending), dungeon and trial access, Majora and Moon
  requirements, dungeon item placement (anywhere / own dungeon / start with), stray fairy and token counts,
  starting health, consumables, rupees, maps and compasses, starting items, hint options, DeathLink.
- Items for other worlds appear with an Archipelago model; collecting them reports the location.
- Universal Tracker support (the world is rebuilt exactly from the slot data).

## Not supported (yet)

- Glitched logic variants: 2S2H's "No Logic" is passed through, "Nearly No Logic" and "Vanilla" are not offered.
- Own-dungeon items are restricted by item rules, not pre-placed like 2S2H does; very tight pools may fail to fill.
- macOS client build (the CI job does not run on the fork).

## Testing

- **Logic parity:** 2S2H's unmodified region logic is compiled with a mocked save file and compared against the AP
  world on 1200 random inventories over 8 option sets: identical reachable regions, checks and event counts in all
  of them.
- Archipelago's test suite (all worlds) passes. 199 of 200 seeds with random options generate (the remaining one
  is a pool that is too large, which 2S2H refuses as well).
- Details: `worlds/mm_2s2h/docs/testing.md` in the `apworld` branch.

## Not included

No ROMs, game data, extracted assets, `*.otr` or `*.o2r` files are part of this repository.
You need your **own legally obtained copy** of Majora's Mask to use 2 Ship 2 Harkinian.

## Credits and sources

All credit for the original work goes to the respective projects and their contributors. They are listed here
as sources, not as affiliations:

- [HarbourMasters/2ship2harkinian](https://github.com/HarbourMasters/2ship2harkinian) – 2 Ship 2 Harkinian and its randomizer
- [xirba13/2ship2harkinian-archipelago](https://github.com/xirba13/2ship2harkinian-archipelago) – Archipelago client layer for 2S2H
- [ArchipelagoMW/Archipelago](https://github.com/ArchipelagoMW/Archipelago) – Archipelago
- [black-sliver/apclientpp](https://github.com/black-sliver/apclientpp) – Archipelago client library used by the client

License files and credits of the original projects are kept unchanged in the `client` and `apworld` branches
and apply there.
