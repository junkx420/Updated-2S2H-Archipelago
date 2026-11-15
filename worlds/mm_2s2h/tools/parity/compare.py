"""Phase 3 parity check: mm_2s2h apworld vs 2S2H's real C++ logic.

For fixed option sets and many random inventories, both sides compute what is reachable:
  C++    : build/parity.exe = unmodified Regions/*.cpp + Logic.h + Logic.cpp + TimeLogic.cpp on a mocked save
  Python : the actual AP world (regions, entrances, event locations, rules) via CollectionState
Both get the same save state: SaveView (primitives.py) turns the RI inventory into save facts. That RI->save
step (port of GiveItem) is therefore shared and not covered here; everything above it is compared.

Usage (from the archipelago root, with its venv):  python worlds/mm_2s2h/tools/parity/compare.py [scenarios-per-set]
"""
from __future__ import annotations

import pathlib
import random
import subprocess
import sys
from collections import Counter

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[3]))  # archipelago root
import ModuleUpdate  # noqa: E402

ModuleUpdate.update_ran = True
from BaseClasses import CollectionState  # noqa: E402
from test.general import setup_multiworld  # noqa: E402
from worlds.mm_2s2h import MM2S2HWorld, region_name  # noqa: E402
from worlds.mm_2s2h.items_generated import ITEMS  # noqa: E402
from worlds.mm_2s2h.locations_generated import LOCATIONS  # noqa: E402
from worlds.mm_2s2h.primitives import LOGIC_ITEMS, SaveView  # noqa: E402
from worlds.mm_2s2h.regions_generated import REGIONS  # noqa: E402

ALL_ON = {k: True for k in (
    "shuffle_owl_statues", "shuffle_shops", "shuffle_tingle_shops", "shuffle_boss_remains", "shuffle_cows",
    "shuffle_enemy_drops", "shuffle_frogs", "shuffle_butterflies", "shuffle_freestanding", "shuffle_wonder_items",
    "shuffle_gold_skulltulas", "shuffle_pots", "shuffle_crates", "shuffle_barrels", "shuffle_snowballs",
    "shuffle_grass", "shuffle_trees", "shuffle_hives")}
OPTION_SETS = {
    "default": {},
    "all_on": dict(ALL_ON),
    "all_on_souls_buttons": dict(ALL_ON, shuffle_boss_souls=True, shuffle_enemy_souls=True, shuffle_ocarina_buttons=True,
                                 shuffle_swim=True),
    "clock_any": dict(ALL_ON, clock_shuffle=True, clock_shuffle_mode="any_order"),
    "clock_ascending": dict(ALL_ON, clock_shuffle=True, clock_shuffle_mode="ascending"),
    "clock_descending": dict(ALL_ON, clock_shuffle=True, clock_shuffle_mode="descending"),
    "access_form_or_song": dict(ALL_ON, access_dungeons="form_or_song", moon_masks_required=8, moon_remains_required=2),
    "access_open": dict(ALL_ON, access_dungeons="open", access_trials="open", stray_fairies_required=7,
                        skulltula_tokens_required=12),
}
CAPS = {"RI_PROGRESSIVE_SWORD": 3, "RI_PROGRESSIVE_BOW": 3, "RI_PROGRESSIVE_BOMB_BAG": 3, "RI_PROGRESSIVE_WALLET": 3,
        "RI_PROGRESSIVE_MAGIC": 2, "RI_PROGRESSIVE_LULLABY": 2, "RI_TIME_PROGRESSIVE": 6, "RI_HEART_PIECE": 52,
        "RI_HEART_CONTAINER": 8, "RI_GS_TOKEN_SWAMP": 30, "RI_GS_TOKEN_OCEAN": 30, "RI_WOODFALL_SMALL_KEY": 1,
        "RI_SNOWHEAD_SMALL_KEY": 3, "RI_GREAT_BAY_SMALL_KEY": 1, "RI_STONE_TOWER_SMALL_KEY": 4,
        "RI_WOODFALL_STRAY_FAIRY": 15, "RI_SNOWHEAD_STRAY_FAIRY": 15, "RI_GREAT_BAY_STRAY_FAIRY": 15,
        "RI_STONE_TOWER_STRAY_FAIRY": 15, "RI_TRIFORCE_PIECE": 20}
RR_BY_REGION_NAME = {region_name(rr): rr for rr in REGIONS}
KNOWN = set(__import__("re").findall(r"constexpr int (\w+) =", (HERE / "build" / "consts.h").read_text()))


def random_inventory(rng: random.Random, p: float) -> Counter:
    inv = Counter()
    for ri in sorted(LOGIC_ITEMS):
        if ri not in ITEMS or rng.random() >= p:
            continue
        cap = CAPS.get(ri, 1)
        inv[ri] = cap if p >= 1 else rng.randint(1, cap)
    return inv


def scenario_text(name: str, world, v: SaveView) -> str:
    out = [f"scenario {name}"]
    out += [f"opt {k} {val}" for k, val in world.ro.items()]
    out += [f"price {rc} {pr}" for rc, pr in world.prices.items()]
    out += [f"inv {i}" for i in sorted(v.inv)]
    assert all(i in KNOWN for i in v.inv), [i for i in v.inv if i not in KNOWN]
    out += [f"quest {q}" for q in sorted(v.quest)]
    out += [f"inf {f}" for f in sorted(v.flags)]
    out += [f"week {w}" for w in sorted(v.weekreg) if w in KNOWN]  # others are never read
    out += [f"owl {o}" for o in sorted(v.owls)]
    out += [f"sword {v.sword}", f"shield {v.shield}", f"quiver {v.quiver}", f"bombbag {v.bombbag}",
            f"wallet {v.wallet}", f"magic {int(v.magic)}", f"health {v.health}", f"bottle {int(v.bottle)}"]
    out += [f"key DUNGEON_SCENE_INDEX_{d} {n}" for d, n in v.keys.items()]
    out += [f"fairy {d} {n}" for d, n in v.fairies.items()]
    out += [f"token {s} {n}" for s, n in v.tokens.items()]
    out += [f"ditem {a} {b}" for a, b in sorted(v.dungeon_items)]
    out.append("end")
    return "\n".join(out)


def python_side(mw, world, inv: Counter):
    p = world.player
    state = CollectionState(mw)
    for ri, n in inv.items():
        for _ in range(n):
            state.collect(world.create_item(ITEMS[ri][0]), prevent_sweep=True)
    events = [loc for loc in mw.get_locations(p) if loc.item and loc.item.name.startswith("RE_")]
    done = set()
    while True:
        new = [loc for loc in events if loc not in done and loc.can_reach(state)]
        if not new:
            break
        for loc in new:
            done.add(loc)
            state.collect(loc.item, prevent_sweep=True, location=loc)
    regions = {RR_BY_REGION_NAME[r.name] for r in mw.get_regions(p) if r.can_reach(state)}
    by_name = {d[0]: rc for rc, d in LOCATIONS.items()}
    checks = {by_name[loc.name] for loc in mw.get_locations(p) if loc.name in by_name and loc.can_reach(state)}
    ev = Counter(loc.item.name for loc in done)
    return regions, checks, ev


def parse_cpp(text: str):
    res, cur = {}, None
    for line in text.splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "scenario":
            cur = res[parts[1]] = (set(), set(), Counter())
        elif parts[0] == "region":
            cur[0].add(parts[1])
        elif parts[0] == "check":
            cur[1].add(parts[1])
        elif parts[0] == "event":
            cur[2][parts[1]] = int(parts[2])
    return res


def main() -> int:
    per_set = int(sys.argv[1]) if len(sys.argv) > 1 else 30
    exe = HERE / "build" / "parity.exe"
    rng = random.Random(3)
    total, bad, report = 0, 0, []
    stats = Counter()
    for set_name, opts in OPTION_SETS.items():
        mw = setup_multiworld(MM2S2HWorld, ("generate_early", "create_regions"), seed=11, options=opts)
        world = mw.worlds[1]
        domain = {rc for rc, d in LOCATIONS.items() if mw.get_location(d[0], 1, ) if True} if False else None
        locs = {loc.name for loc in mw.get_locations(1)}
        domain = {rc for rc, d in LOCATIONS.items() if d[0] in locs}
        cases = {}
        for k in range(per_set):
            p = [0.0, 0.15, 0.3, 0.45, 0.6, 0.75, 0.9, 1.0][k % 8]
            inv = random_inventory(rng, p)
            cases[f"{set_name}_{k}"] = inv
        feed = "\n".join(scenario_text(n, world, SaveView(inv, Counter(), world)) for n, inv in cases.items()) + "\n"
        run = subprocess.run([str(exe)], input=feed, capture_output=True, text=True)
        if run.returncode:
            sys.exit(f"parity.exe failed: {run.stderr.strip()}")
        cpp = parse_cpp(run.stdout)
        for name, inv in cases.items():
            total += 1
            py_regions, py_checks, py_ev = python_side(mw, world, inv)
            c_regions, c_checks, c_ev = cpp[name]
            c_checks &= domain
            stats["checks_reachable"] += len(py_checks)
            stats["checks_domain"] += len(domain)
            diffs = []
            if py_regions != c_regions:
                diffs.append(f"regions only C++ {sorted(c_regions - py_regions)[:5]} only AP {sorted(py_regions - c_regions)[:5]}")
            if py_checks != c_checks:
                diffs.append(f"checks only C++ {sorted(c_checks - py_checks)[:5]} only AP {sorted(py_checks - c_checks)[:5]}")
            if py_ev != c_ev:
                keys = {k for k in set(py_ev) | set(c_ev) if py_ev[k] != c_ev[k]}
                diffs.append("events " + ", ".join(f"{k}: AP {py_ev[k]} C++ {c_ev[k]}" for k in sorted(keys)[:5]))
            if diffs:
                bad += 1
                report.append(f"{name} ({sum(inv.values())} items): " + " | ".join(diffs))
        print(f"{set_name}: {per_set} scenarios, domain {len(domain)} checks")
    print(f"\nTOTAL {total} scenarios, {bad} mismatching; avg reachable {stats['checks_reachable'] / total:.0f} "
          f"of {stats['checks_domain'] / total:.0f} checks")
    for r in report[:15]:
        print(" ", r)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
