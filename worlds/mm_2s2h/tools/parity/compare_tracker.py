"""Compare the in-game tracker evaluator with the AP world for fixed inventories."""
from __future__ import annotations

import argparse
import random
import subprocess
from collections import Counter
from dataclasses import dataclass, field

from compare import (ALL_ON, HERE, MM2S2HWorld, SaveView, parse_cpp, python_side, random_inventory, scenario_text,
                     setup_multiworld)
from worlds.mm_2s2h.primitives import BOTTLES


FORK_OPTIONS = dict(ALL_ON, shuffle_npc_souls="all", shuffle_door_keys=True, shuffle_epona_riding=True,
                    shuffle_bean_planting=True, shuffle_rocks=True)
STEPS = ("generate_early", "create_regions", "connect_entrances")
INVENTORY_DENSITIES = (0, .15, .30, .45, .60, .75, .90, 1)
START = "RR_CLOCK_TOWN_SOUTH"


@dataclass
class Snapshot:
    name: str
    inventory: Counter
    checks: dict[str, bool] = field(default_factory=dict)
    events: dict[str, bool] = field(default_factory=dict)
    obtained: tuple[str, ...] = ()


def run_client(feed: list[str]):
    run = subprocess.run([str(HERE / "build" / "parity.exe")], input="\n".join(feed) + "\n", capture_output=True,
                         text=True, check=False)
    if run.returncode:
        raise SystemExit(f"parity.exe exited {run.returncode}: {run.stderr.strip()}")
    return parse_cpp(run.stdout)


def compare_seed(mw, snapshots: list[Snapshot], *, zora_hall_key: bool = True) -> list[str]:
    world = mw.worlds[1]
    domain = set(world.shuffled)
    setup = f"tracker {START}\n" + "\n".join(f"shuffled {check}" for check in sorted(domain))
    if zora_hall_key:
        setup += "\ninf RANDO_INF_DOOR_KEYS_ZORA_HALL"
    feed = []
    for case in snapshots:
        scenario = scenario_text(case.name, world, SaveView(case.inventory, Counter(), world))
        obtained = "\n".join(f"obtained {check}" for check in case.obtained)
        feed.append(f"{scenario.removesuffix('end')}{setup}\n{obtained}\nend")

    differences = []
    results = run_client(feed)
    for case in snapshots:
        _, client_checks, client_events = results[case.name]
        _, ap_checks, ap_events = python_side(mw, world, case.inventory)
        ap_checks = (ap_checks & domain).difference(case.obtained)
        if client_checks != ap_checks:
            differences.append(f"{case.name}: client only {sorted(client_checks - ap_checks)}, "
                               f"AP only {sorted(ap_checks - client_checks)}")
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
    results = run_client(feed)
    return [] if results["native_off"] == results["native_on"] else ["Abilities altered native 2S2H logic"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("seeds", type=int, nargs="?", default=100)
    args = parser.parse_args()
    rng = random.Random(0x2A02)
    count, focused_diffs = targeted_cases()
    native_diffs = native_check()
    seed_diffs = []
    for idx in range(args.seeds):
        seed = rng.getrandbits(64)
        mw = setup_multiworld(MM2S2HWorld, STEPS, seed=seed, options=FORK_OPTIONS)
        snapshots = [Snapshot(f"{seed}_{i}", random_inventory(rng, density))
                     for i, density in enumerate(INVENTORY_DENSITIES)]
        seed_diffs.extend(compare_seed(mw, snapshots))
        if (idx + 1) % 10 == 0:
            print(f"{idx + 1}/{args.seeds} seeds compared, {len(seed_diffs)} differences", flush=True)
    print(f"{args.seeds} seeds, {args.seeds * len(INVENTORY_DENSITIES)} snapshots: {len(seed_diffs)} differences")
    print(f"{count} targeted snapshots: {len(focused_diffs)} differences; native: {len(native_diffs)} differences")
    for diff in (focused_diffs + native_diffs + seed_diffs)[:20]:
        print(diff)
    return int(bool(focused_diffs or native_diffs or seed_diffs))


if __name__ == "__main__":
    raise SystemExit(main())
