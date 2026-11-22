"""Compare the in-game tracker evaluator with the AP world for fixed inventories."""
from __future__ import annotations

import argparse
import random
import re
import statistics
import subprocess
from collections import Counter
from dataclasses import dataclass, field

from compare import (ALL_ON, HERE, CollectionState, MM2S2HWorld, RR_BY_REGION_NAME, SaveView, parse_cpp, python_side,
                     random_inventory, scenario_text, setup_multiworld)
from test.general import gen_steps
from worlds.mm_2s2h import RI_BY_NAME
from worlds.mm_2s2h.enums_generated import ENTRANCE_MIX_POOLS
from worlds.mm_2s2h.primitives import BOTTLES


FORK_OPTIONS = dict(ALL_ON, shuffle_npc_souls="all", shuffle_door_keys=True, shuffle_epona_riding=True,
                    shuffle_bean_planting=True, shuffle_rocks=True)
ENTRANCE_OPTIONS = dict(shuffle_dungeon_entrances=True, shuffle_boss_entrances=True, shuffle_interiors=True,
                        shuffle_grottos=True, shuffle_overworld=True, shuffle_special_areas=True)
STEPS = ("generate_early", "create_regions", "connect_entrances")
INVENTORY_DENSITIES = (0, .15, .30, .45, .60, .75, .90, 1)
START = "RR_CLOCK_TOWN_SOUTH"
ENTRANCE_SCENES = {name: int(value) for name, value in re.findall(
    r"constexpr int ENTR_SCENE_(\w+) = (\d+);", (HERE / "build" / "consts.h").read_text())}


@dataclass
class Snapshot:
    name: str
    inventory: Counter
    checks: dict[str, bool] = field(default_factory=dict)
    events: dict[str, bool] = field(default_factory=dict)
    obtained: tuple[str, ...] = ()
    current_region: str = START


def run_client(feed: list[str]):
    run = subprocess.run([str(HERE / "build" / "parity.exe")], input="\n".join(feed) + "\n", capture_output=True,
                         text=True, check=False)
    if run.returncode:
        raise SystemExit(f"parity.exe exited {run.returncode}: {run.stderr.strip()}")
    return run.stdout


def compare_seed(mw, snapshots: list[Snapshot], *, zora_hall_key: bool = True,
                 times: list[float] | None = None) -> list[str]:
    world = mw.worlds[1]
    domain = set(world.shuffled)
    # CollectionState (on the AP side) has already collected the seed's starting items.
    starting = Counter({RI_BY_NAME[name]: count for name, count in CollectionState(mw).prog_items[1].items()
                        if name in RI_BY_NAME})
    setup = "\n".join(f"shuffled {check}" for check in sorted(domain))
    setup += "\n" + "\n".join(f"pair {a} {b}" for a, b in world.entrance_pairs)
    setup += "\n" + "\n".join(f"mixed {a} {b}" for a, b in world.mixed_entrance_pairs)
    if zora_hall_key:
        setup += "\ninf RANDO_INF_DOOR_KEYS_ZORA_HALL"
    feed = []
    for idx, case in enumerate(snapshots):
        scenario = scenario_text(case.name, world, SaveView(case.inventory + starting, Counter(), world))
        obtained = "\n".join(f"obtained {check}" for check in case.obtained)
        graph = "graph" if idx == 0 else ""
        feed.append(f"{scenario.removesuffix('end')}tracker {case.current_region}\n{setup}\n{obtained}\n{graph}\nend")

    differences = []
    output = run_client(feed)
    results = parse_cpp(output)
    if times is not None:
        times.extend(float(line.split()[1]) / 1000 for line in output.splitlines() if line.startswith("elapsed_us "))
    client_graph = {(source, int(exit_id)): target for source, exit_id, target in
                    re.findall(r"^edge (\S+) (\d+) (\S+)$", output, re.MULTILINE)}
    ap_graph = {}
    for (region, entrance), exit_ in world.exit_entrances.items():
        scene, spawn = entrance.split(":")
        value = ((ENTRANCE_SCENES[scene] & 0x7F) << 9) | ((int(spawn) & 0x1F) << 4)
        ap_graph[region, value] = RR_BY_REGION_NAME[exit_.connected_region.name]
    graph_diffs = [(key, client_graph.get(key), ap_graph.get(key)) for key in sorted(client_graph.keys() | ap_graph.keys())
                   if client_graph.get(key) != ap_graph.get(key)]
    if graph_diffs:
        differences.append(f"{snapshots[0].name} graph (exit, client, AP), {len(graph_diffs)} differences: {graph_diffs[:8]}")
    for case in snapshots:
        _, client_checks, client_events = results[case.name]
        _, ap_checks, ap_events = python_side(mw, world, case.inventory)
        ap_checks = (ap_checks & domain).difference(case.obtained)
        if client_checks != ap_checks:
            client_only, ap_only = sorted(client_checks - ap_checks), sorted(ap_checks - client_checks)
            differences.append(f"{case.name}: {len(client_only)} client only {client_only[:8]}, "
                               f"{len(ap_only)} AP only {ap_only[:8]}")
        for kind, assertions, client, ap in (("check", case.checks, client_checks, ap_checks),
                                              ("event", case.events, client_events, ap_events)):
            for name, expected in assertions.items():
                if (name in client) != expected or (name in ap) != expected:
                    differences.append(f"{case.name}: {kind} {name}, expected {expected}, "
                                       f"client {name in client}, AP {name in ap}")
    return differences


def targeted_cases() -> tuple[int, list[str]]:
    rng = random.Random(0)
    full = random_inventory(rng, 1)
    banker = "RC_CLOCK_TOWN_WEST_BANK_ADULTS_WALLET"
    trading_post = "RC_CLOCK_TOWN_WEST_TRADING_POST_POT"
    garo_mask = "RC_GORMAN_TRACK_GARO_MASK"
    bean_pot = "RC_TERMINA_FIELD_POT"
    spring_rock = "RC_MOUNTAIN_VILLAGE_ROCK_01"
    rolling_rock = "RC_ROAD_TO_IKANA_ROCK_ROLLING_01"
    evan = "RC_ZORA_HALL_EVANS_PIECE_OF_HEART"
    doggy_chest = "RC_DOGGY_RACETRACK_CHEST"
    cow = "RC_TERMINA_FIELD_COW_FRONT"
    aliens = "RE_COWS_FROM_ALIENS"
    missing_epona = full - Counter(RI_ABILITY_RIDE_EPONA=1)
    missing_beans = full - Counter(RI_ABILITY_PLANT_BEANS=1)
    missing_zora_key = full - Counter(RI_DOOR_KEY_ZORA_HALL_ROOMS=1)
    rain_only = full - Counter(["RI_HOOKSHOT", "RI_MASK_ZORA", "RI_SONG_STORMS", *BOTTLES])
    mw = setup_multiworld(MM2S2HWorld, STEPS, seed=0, options=FORK_OPTIONS)
    snapshots = [
        Snapshot("full", full, {banker: True, trading_post: True, garo_mask: True, bean_pot: True, spring_rock: True},
                 {aliens: True}),
        Snapshot("missing_banker", full - Counter(RI_SOUL_NPC_BANKER=1), {banker: False}),
        Snapshot("already_obtained", full, {banker: False}, obtained=(banker,)),
        Snapshot("missing_romani", full - Counter(RI_SOUL_NPC_ROMANI_AND_CREMIA=1), events={aliens: False}),
        Snapshot("missing_door_key", full - Counter(RI_DOOR_KEY_TRADING_POST=1), {trading_post: False}),
        Snapshot("inside_locked_door", full - Counter(RI_DOOR_KEY_TRADING_POST=1), {trading_post: False},
                 current_region="RR_TRADING_POST"),
        Snapshot("missing_epona_ability", missing_epona, {garo_mask: False, cow: True}),
        Snapshot("missing_bean_ability", missing_beans, {bean_pot: False}),
        Snapshot("missing_zora_key", missing_zora_key, {evan: False}),
        Snapshot("before_snowhead_clear", full - Counter(RI_SNOWHEAD_BOSS_KEY=1), {spring_rock: False}),
        Snapshot("rain_with_ability", rain_only, {doggy_chest: True}),
        Snapshot("rain_without_ability", rain_only - Counter(RI_ABILITY_PLANT_BEANS=1), {doggy_chest: False}),
        Snapshot("zora_without_ability", missing_beans - Counter(RI_HOOKSHOT=1), {doggy_chest: True}),
    ]
    differences = compare_seed(mw, snapshots)
    count = len(snapshots)
    for opts, cases in (
        (dict(shuffle_epona_riding=False, shuffle_bean_planting=False), [
            Snapshot("abilities_off", missing_epona - Counter(RI_ABILITY_PLANT_BEANS=1),
                     {garo_mask: True, bean_pot: True}),
        ]),
        (dict(shuffle_npc_souls="classic"), [
            Snapshot("classic_souls", full - Counter(RI_SOUL_NPC_ROMANI_AND_CREMIA=1), events={aliens: True}),
        ]),
        (dict(logic="no_logic"), [Snapshot("no_logic", Counter(), {banker: True, trading_post: True})]),
        (dict(clock_shuffle=True), [
            Snapshot("rock_at_night", full - Counter(RI_TIME_DAY_1=1, RI_TIME_DAY_2=1, RI_TIME_DAY_3=1,
                                                      RI_TIME_PROGRESSIVE=6),
                     {rolling_rock: False}),
            Snapshot("rock_in_daylight", full, {rolling_rock: True}),
        ]),
    ):
        mw = setup_multiworld(MM2S2HWorld, STEPS, seed=0, options=FORK_OPTIONS | opts)
        differences.extend(compare_seed(mw, cases))
        count += len(cases)

    mw = setup_multiworld(MM2S2HWorld, ("generate_early",), seed=0, options=FORK_OPTIONS)
    world = mw.worlds[1]
    world.door_keys = [row for row in world.door_keys if row["key"] != "RI_DOOR_KEY_ZORA_HALL_ROOMS"]
    world.create_regions()
    differences.extend(compare_seed(mw, [Snapshot("old_zora_hall", missing_zora_key, {evan: True})],
                                    zora_hall_key=False))
    return count + 1, differences


def native_check() -> list[str]:
    inventory = random_inventory(random.Random(0), 1)
    inventory -= Counter(RI_ABILITY_RIDE_EPONA=1, RI_ABILITY_PLANT_BEANS=1)
    feed = []
    for name, enabled in (("native_off", False), ("native_on", True)):
        opts = dict(ALL_ON, shuffle_epona_riding=enabled, shuffle_bean_planting=enabled)
        mw = setup_multiworld(MM2S2HWorld, STEPS, seed=0, options=opts)
        world = mw.worlds[1]
        feed.append(scenario_text(name, world, SaveView(inventory, Counter(), world)))
    results = parse_cpp(run_client(feed))
    return [] if results["native_off"] == results["native_on"] else ["Abilities altered native 2S2H logic"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("seeds", type=int, nargs="?", default=100)
    parser.add_argument("--entrances", action="store_true", help="Run this many seeds for each entrance pool mode")
    args = parser.parse_args()
    rng = random.Random(0x2A02)
    count, focused_diffs = targeted_cases()
    native_diffs = native_check()
    modes = [("vanilla entrances", {})]
    steps = STEPS
    if args.entrances:
        modes = [("separate", ENTRANCE_OPTIONS),
                 ("mixed", ENTRANCE_OPTIONS | {"mix_entrance_pools": list(ENTRANCE_MIX_POOLS)})]
        steps = gen_steps
    differences = focused_diffs + native_diffs
    print(f"{count} targeted snapshots: {len(focused_diffs)} differences; native: {len(native_diffs)} differences")
    for mode, options in modes:
        seed_diffs, times = [], []
        for idx in range(args.seeds):
            seed = rng.getrandbits(64)
            mw = setup_multiworld(MM2S2HWorld, steps, seed=seed, options=FORK_OPTIONS | options)
            snapshots = [Snapshot(f"{seed}_{i}", random_inventory(rng, density))
                         for i, density in enumerate(INVENTORY_DENSITIES)]
            seed_diffs.extend(compare_seed(mw, snapshots, times=times))
            if (idx + 1) % 10 == 0:
                print(f"{mode}: {idx + 1}/{args.seeds} seeds compared, {len(seed_diffs)} differences", flush=True)
        print(f"{mode}: {args.seeds} seeds, {args.seeds * len(INVENTORY_DENSITIES)} snapshots: {len(seed_diffs)} differences")
        if times:
            times.sort()
            p95 = times[(95 * len(times) - 1) // 100]
            print(f"{mode}: tracker refresh median {statistics.median(times):.3f} ms, p95 {p95:.3f} ms, "
                  f"max {times[-1]:.3f} ms")
        differences += seed_diffs
    for diff in differences[:20]:
        print(diff)
    return int(bool(differences))


if __name__ == "__main__":
    raise SystemExit(main())
