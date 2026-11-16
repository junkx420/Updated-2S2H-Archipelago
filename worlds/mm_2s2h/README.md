# mm_2s2h – Majora's Mask (2 Ship 2 Harkinian)

Unofficial fan project, see the `main` branch README of `junkx420/Updated-2S2H-Archipelago`.

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
