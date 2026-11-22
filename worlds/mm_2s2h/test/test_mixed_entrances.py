import hashlib
import json
from itertools import combinations

import pytest
from Options import OptionError

from .. import CLOCK_TOWN_SCENES, ENTRANCE_ROW_INDEX, MIX_OPTION, OVERWORLD, region_name
from ..enums_generated import (ENTRANCE_MIX_FIXED, ENTRANCE_MIX_ONLY, ENTRANCE_MIX_POOLS, ENTRANCE_NAMES,
                               ENTRANCE_SHUFFLE, GROTTO_RETURN_POINTS)
from ..regions_generated import REGIONS
from .test_door_keys import state_without
from .test_entrance_shuffle import BOSSES, BY_VALUE, connected, item_names, vanilla_target
from .test_pool_trim import build
from .test_special_areas import ALL_OWN, DATA, rebuilt_from


COMBINATIONS = [pools for count in range(2, len(ENTRANCE_MIX_POOLS) + 1)
                for pools in combinations(ENTRANCE_MIX_POOLS, count)]
FIXED_GROTTOS = {f"GROTTOS:{i}" for i in (5, 12, 16, 22, 23, 27, 28, 31)}


def exit_key(endpoint):
    row = ENTRANCE_SHUFFLE[endpoint // 2]
    return (row["inside_region"], row["out"]) if endpoint % 2 else (row["door_region"], row["into"])


def check_warps(world):
    for boss in BOSSES:
        source = next(row for row in BOSSES
                      if connected(world, row["door_region"], row["into"]) == region_name(boss["inside_region"]))
        temple = next(row for row in ENTRANCE_SHUFFLE
                      if row["option"] == "RO_SHUFFLE_DUNGEON_ENTRANCES" and row["out"] == source["temple_exit"])
        exterior = connected(world, temple["inside_region"], temple["out"])
        for rr, key in boss["boss_warps"]:
            expected = exterior if source is not boss or exterior != region_name(temple["door_region"]) else vanilla_target(rr, key)
            assert connected(world, rr, key) == expected


def test_mix_table_inventory():
    assert ENTRANCE_ROW_INDEX[ENTRANCE_MIX_ONLY] == 157
    assert set(ENTRANCE_MIX_FIXED) == FIXED_GROTTOS | {
        "TRADING_POST:0", "SWORDMANS_SCHOOL:0", "ROMANI_RANCH:0", "MILK_ROAD:1",
    }
    assert len(GROTTO_RETURN_POINTS) == 18
    assert set(GROTTO_RETURN_POINTS) == {row["into"] for row in ENTRANCE_SHUFFLE if row["grotto"]} - FIXED_GROTTOS
    assert len(COMBINATIONS) == 26
    assert sum(row["hours"] == "ALWAYS_OPEN" and row["into"] not in ENTRANCE_MIX_FIXED
               for row in ENTRANCE_SHUFFLE) == 24


@pytest.mark.parametrize("pools", COMBINATIONS)
@pytest.mark.parametrize("logic", ["glitchless", "no_logic"])
def test_mixed_pool_combinations(pools, logic):
    mw = build(ALL_OWN | {"mix_entrance_pools": pools, "logic": logic}, 610)
    world = mw.worlds[1]
    pairs = world.mixed_entrance_pairs
    selected = {ENTRANCE_MIX_POOLS[name][0] for name in pools}
    expected_endpoints = set()
    for i, row in enumerate(ENTRANCE_SHUFFLE):
        if (row["option"] in selected and row["into"] not in ENTRANCE_MIX_FIXED and not row["water"]
                and row["hours"] in (None, "ALWAYS_OPEN")):
            expected_endpoints.add(2 * i)
            if row["option"] != OVERWORLD:
                expected_endpoints.add(2 * i + 1)
    ends = [end for pair in pairs for end in pair]
    assert set(ends) == expected_endpoints and len(ends) == len(expected_endpoints)
    assert any(ENTRANCE_SHUFFLE[a // 2]["option"] != ENTRANCE_SHUFFLE[b // 2]["option"] for a, b in pairs)
    for pair in pairs:
        for source, target in (pair, pair[::-1]):
            assert connected(world, *exit_key(source)) == region_name(exit_key(target)[0])
            row = ENTRANCE_SHUFFLE[source // 2]
            source_name = ENTRANCE_NAMES[row["into"]][1 + source % 2]
            target_name = ENTRANCE_NAMES[ENTRANCE_SHUFFLE[target // 2]["into"]][1 + target % 2]
            assert mw.spoiler.entrances[source_name, "entrance", 1]["exit"] == target_name
            if row["option"] == OVERWORLD and REGIONS[row["door_region"]][1] in CLOCK_TOWN_SCENES:
                if REGIONS[row["inside_region"]][1] not in CLOCK_TOWN_SCENES:
                    assert ENTRANCE_SHUFFLE[target // 2]["option"] == OVERWORLD
                    assert REGIONS[exit_key(target)[0]][1] not in CLOCK_TOWN_SCENES
    for a, b in world.entrance_pairs:
        source, target = BY_VALUE[a], BY_VALUE[b]
        assert source["option"] == target["option"]
        assert source["hours"] == target["hours"] and source["water"] == target["water"]
    for row in ENTRANCE_SHUFFLE:
        if row["into"] in ENTRANCE_MIX_FIXED and row["option"] in selected:
            assert connected(world, row["door_region"], row["into"]) == region_name(row["inside_region"])
            assert connected(world, row["inside_region"], row["out"]) == region_name(row["door_region"])
    state = mw.get_all_state()
    assert mw.can_beat_game(state)
    assert all(loc.can_reach(state) for loc in mw.get_locations() if loc.address is not None)
    check_warps(world)
    rebuilt = rebuilt_from(world.fill_slot_data(), 4242)
    assert rebuilt.mixed_entrance_pairs == pairs
    assert rebuilt.entrance_pairs == world.entrance_pairs
    assert {key: e.connected_region.name for key, e in rebuilt.exit_entrances.items()} == {
        key: e.connected_region.name for key, e in world.exit_entrances.items()}


def test_legacy_09_and_mix_off():
    data = json.loads((DATA / "slot_data_0_9_0_all_pools.json").read_text(encoding="utf-8"))
    assert data["apworld_version"] == "0.9.0"
    assert MIX_OPTION not in data["options"] and "mixed_entrance_pairs" not in data
    rebuilt = rebuilt_from(data, 4242)
    new = build(ALL_OWN | {"mix_entrance_pools": []}, 610, fill=False).worlds[1]
    assert len(new.entrance_pairs) == 157
    assert rebuilt.entrance_pairs == new.entrance_pairs == [tuple(pair) for pair in data["entrance_pairs"]]
    assert "mixed_entrance_pairs" not in new.fill_slot_data()
    assert not rebuilt.ro[MIX_OPTION]
    assert {key: e.connected_region.name for key, e in rebuilt.exit_entrances.items()} == {
        key: e.connected_region.name for key, e in new.exit_entrances.items()}
    inverted = ENTRANCE_SHUFFLE[ENTRANCE_ROW_INDEX[ENTRANCE_MIX_ONLY]]
    assert connected(rebuilt, inverted["door_region"], inverted["into"]) == region_name(inverted["inside_region"])
    assert connected(rebuilt, inverted["inside_region"], inverted["out"]) == region_name(inverted["door_region"])


def test_legacy_017_with_four_mixed_pools_and_decouple():
    slot = json.loads((DATA / "slot_data_0_17_0_mixed.json").read_text(encoding="utf-8"))
    assert slot["apworld_version"] == "0.17.0"
    pools = [name for name in ENTRANCE_MIX_POOLS if name != "grottos"]
    current = build(ALL_OWN | {"mix_entrance_pools": pools, "decouple_entrances": True}, 610, fill=False).worlds[1]
    rebuilt = rebuilt_from(slot, 4242)
    for key in ("entrance_pairs", "mixed_entrance_pairs", "directed_entrance_targets"):
        assert current.fill_slot_data()[key] == rebuilt.fill_slot_data()[key] == slot[key]
    for world in (current, rebuilt):
        graph = sorted((key, exit_.connected_region.name) for key, exit_ in world.exit_entrances.items())
        assert hashlib.sha256(json.dumps(graph).encode()).hexdigest() == (
            "f3d4fd6dd8d22fbda0c47cc8346936c0c33d5f2fa740e15887663b2acddda9b6")


def test_key_and_bombs_stay_at_the_source_with_a_shared_grotto():
    world = build(ALL_OWN | {"mix_entrance_pools": ["grottos", "interiors"]}, 610, fill=False).worlds[1]
    slot = world.fill_slot_data()
    hole = 2 * ENTRANCE_ROW_INDEX["GROTTOS:30"]
    shop = 2 * ENTRANCE_ROW_INDEX["BOMB_SHOP:0"]
    outsides = sorted(end for pair in world.mixed_entrance_pairs for end in pair if end % 2 == 0)
    slot["mixed_entrance_pairs"] = [(end, end + 1) for end in outsides if end not in (shop, hole)] + [
        (shop, hole + 1), (hole, shop + 1)]
    rebuilt = rebuilt_from(slot, 4242)
    hole_rule = rebuilt.exit_entrances[exit_key(hole)].access_rule
    door_rule = rebuilt.exit_entrances[exit_key(shop)].access_rule
    no_key = state_without(rebuilt, item_names("RI_DOOR_KEY_BOMB_SHOP"))
    assert hole_rule(no_key) and not door_rule(no_key)
    no_explosives = item_names("RI_PROGRESSIVE_BOMB_BAG") | item_names("RI_MASK_BLAST")
    assert not hole_rule(state_without(rebuilt, no_explosives))
    assert connected(rebuilt, *exit_key(shop)) == region_name(exit_key(hole + 1)[0])
    assert connected(rebuilt, *exit_key(shop + 1)) == region_name(exit_key(hole)[0])


@pytest.mark.parametrize("empty", [False, True])
def test_tracker_rejects_incomplete_grotto_mixed_slot(empty):
    world = build(ALL_OWN | {"mix_entrance_pools": ["grottos", "interiors"]}, 610, fill=False).worlds[1]
    slot = world.fill_slot_data()
    slot["mixed_entrance_pairs"] = [] if empty else slot["mixed_entrance_pairs"][:-1]
    with pytest.raises(OptionError, match="incomplete mixed entrance pairing"):
        rebuilt_from(slot, 4242)


def test_inverted_stone_tower_behind_a_shop():
    world = build(ALL_OWN | {"mix_entrance_pools": ["dungeons", "interiors"]}, 610, fill=False).worlds[1]
    inverted = 2 * ENTRANCE_ROW_INDEX[ENTRANCE_MIX_ONLY]
    shop = 2 * ENTRANCE_ROW_INDEX["BOMB_SHOP:0"]
    data = world.fill_slot_data()
    ends = sorted(end for pair in world.mixed_entrance_pairs for end in pair if end // 2 not in (inverted // 2, shop // 2))
    data["mixed_entrance_pairs"] = [(end, end + 1) for end in ends if end % 2 == 0] + [
        (shop, inverted + 1), (inverted, shop + 1)]
    rebuilt = rebuilt_from(data, 4242)
    door = rebuilt.exit_entrances[exit_key(shop)]
    assert door.connected_region.name == region_name(exit_key(inverted + 1)[0])
    assert door.access_rule(state_without(rebuilt, set()))
    assert not door.access_rule(state_without(rebuilt, item_names("RI_ARROW_LIGHT")))
    assert not door.access_rule(state_without(rebuilt, item_names("RI_DOOR_KEY_BOMB_SHOP")))
    check_warps(rebuilt)


@pytest.mark.parametrize("options", [
    {"mix_entrance_pools": ["dungeons"], "shuffle_dungeon_entrances": True},
    {"mix_entrance_pools": ["dungeons", "interiors"], "shuffle_dungeon_entrances": True},
    {"mix_entrance_pools": ["dungeons", "grottos"], "shuffle_dungeon_entrances": True},
])
def test_mix_requires_two_enabled_pools(options):
    with pytest.raises(OptionError, match="at least two pools|enable shuffle_(interiors|grottos)"):
        build(options, 610, fill=False)
