"""The entrance names of the spoiler log stay word for word what apworld 0.11.0 wrote.

Since 0.11.1 the names come from the client's table (EntranceShuffle.cpp entranceNames, generated as ENTRANCE_NAMES) so
the in-game entrance tracker shows the same names. data/spoiler_entrances_0_11_0.json holds the spoiler entrances of
0.11.0 (apworld commit a5e36c14) for the seeds below; it was written by running this file as a script before the change:
    ../.venv/Scripts/python.exe -m worlds.mm_2s2h.test.test_spoiler_names   (from archipelago/)
Running it again would overwrite the reference with the current names; only do that on purpose.
"""
import json
from collections import Counter
from pathlib import Path

import pytest

from ..enums_generated import ENTRANCE_SHUFFLE
from ..regions_generated import REGIONS
from .test_pool_trim import build
from .test_special_areas import ALL_OWN

BASELINE = Path(__file__).parent / "data" / "spoiler_entrances_0_11_0.json"
MIX_ALL = ["dungeons", "interiors", "overworld", "special_areas"]
CASES = (
    [("all_pools", ALL_OWN, seed) for seed in (1, 2, 3, 4)]
    + [("mixed_all", ALL_OWN | {"mix_entrance_pools": MIX_ALL}, seed) for seed in (1, 2, 3)]
    + [(f"mixed_{'_'.join(pools)}", ALL_OWN | {"mix_entrance_pools": pools}, 5)
       for pools in (["dungeons", "overworld"], ["interiors", "special_areas"], ["dungeons", "interiors", "special_areas"])]
)


def case_id(name: str, seed: int) -> str:
    return f"{name}/{seed}"


def spoiler_entries(options: dict, seed: int) -> list[list[str]]:
    mw = build(options, seed, fill=False)
    return sorted([e["entrance"], e["exit"], e["direction"]] for e in mw.spoiler.entrances.values())


@pytest.mark.parametrize("name,options,seed", CASES, ids=[case_id(n, s) for n, _, s in CASES])
def test_spoiler_entrances_unchanged(name, options, seed):
    baseline = json.loads(BASELINE.read_text(encoding="utf-8"))
    entries = spoiler_entries(options, seed)
    assert len(entries) > 100
    assert entries == baseline[case_id(name, seed)]


def _old_region_name(rr: str) -> str:
    return "Menu" if rr == "RR_MAX" else " ".join(w.capitalize() for w in rr[3:].split("_"))


def _old_door_name(row: dict) -> str:  # __init__.py _door_name of 0.11.0
    if row.get("grotto"):
        return _old_region_name(row["inside_region"])
    if row.get("hours"):
        shared = sum(r["inside_region"] == row["inside_region"] for r in ENTRANCE_SHUFFLE) > 1
        return _old_region_name(row["inside_region"]) + (f" ({row['out'].replace('_', ' ').title()})" if shared else "")
    return row["into"].split(":")[0].replace("_", " ").title()


def _old_place_name(entrance: str) -> str:  # __init__.py _place_name of 0.11.0
    scene, spawn = entrance.split(":")
    return f"{scene.replace('_', ' ').title()} {spawn}"


def test_table_names_equal_old_formulas():
    from .. import _door_name, _place_name
    exit_names = {}
    for rr, (_, _, _, conns, exits, _, _) in REGIONS.items():
        seen = Counter()
        for key, tgt, _ in [(None, t, f) for t, f in conns] + exits:
            seen[tgt] += 1
            if key is not None:
                exit_names[(rr, key)] = (f"{_old_region_name(rr)} -> {_old_region_name(tgt)}"
                                         + (f" #{seen[tgt]}" if seen[tgt] > 1 else ""))
    from ..enums_generated import ENTRANCE_NAMES
    assert len(ENTRANCE_NAMES) == len(ENTRANCE_SHUFFLE)
    for row in ENTRANCE_SHUFFLE:
        name, door_exit, inside_exit = ENTRANCE_NAMES[row["into"]]
        overworld = row["option"] == "RO_SHUFFLE_OVERWORLD"
        assert name == (_old_place_name(row["into"]) if overworld else _old_door_name(row))
        assert (_place_name(row["into"]) if overworld else _door_name(row)) == name
        assert door_exit == exit_names[(row["door_region"], row["into"])]
        assert inside_exit == ("" if overworld else exit_names[(row["inside_region"], row["out"])])


if __name__ == "__main__":
    data = {case_id(name, seed): spoiler_entries(options, seed) for name, options, seed in CASES}
    BASELINE.write_text(json.dumps(data, indent=0, ensure_ascii=False) + "\n", encoding="utf-8")
    print({key: len(value) for key, value in data.items()})
