import pytest
from BaseClasses import CollectionState

from .. import SPAWN_OPTION, region_name
from ..enums_generated import CYCLE_SPAWNS, ENTRANCE_MIX_POOLS
from .test_pool_trim import build
from .test_special_areas import ALL_OWN, rebuilt_from


ROOT = ("RR_MAX", "SOUTH_CLOCK_TOWN:0")


def test_spawn_off_and_legacy():
    original = build({"shuffle_spawn": False}, 592, fill=False).worlds[1]
    slot = original.fill_slot_data()
    assert SPAWN_OPTION not in slot["options"] and "spawn_target" not in slot
    slot["apworld_version"] = "0.15.0"
    for world in (original, rebuilt_from(slot, 93)):
        assert world.exit_entrances[ROOT].connected_region.name == "Clock Town South"


@pytest.mark.parametrize("mode", ["clock", "overworld", "mixed", "no_logic"])
def test_spawn_route_and_regeneration(mode):
    opts = {"shuffle_spawn": True, "clock_shuffle": True, "starting_items": {}}
    if mode == "overworld":
        opts["shuffle_overworld"] = True
    if mode == "mixed":
        opts |= ALL_OWN | {"shuffle_owl_warps": True, "mix_entrance_pools": list(ENTRANCE_MIX_POOLS),
                           "shuffle_signs": True, "shuffle_rocks": True, "shuffle_npc_souls": "all",
                           "shuffle_epona_riding": True, "shuffle_bean_planting": True}
    if mode == "no_logic":
        opts |= {"logic": "no_logic", "shuffle_overworld": True}
    mw = build(opts, 592)
    world = mw.worlds[1]
    full = mw.get_all_state()
    assert mw.can_beat_game(full)
    assert all(loc.can_reach(full) for loc in mw.get_locations() if loc.address is not None)
    slot = world.fill_slot_data()
    rebuilt = rebuilt_from(slot, 93)
    assert world.spawn_target == slot["spawn_target"] == rebuilt.spawn_target
    into, back, _, region = CYCLE_SPAWNS[world.spawn_target]
    for instance in (world, rebuilt):
        assert instance.exit_entrances[ROOT].connected_region.name == region_name(region)
        assert instance.exit_entrances[region, back].connected_region.name == "Clock Town South"
        assert instance.exit_entrances["RR_CLOCK_TOWN_SOUTH", into].connected_region.name == region_name(region)
        starting = CollectionState(instance.multiworld)
        assert instance.exit_entrances[region, back].can_reach(starting)
        assert len([ri for ri in instance.starting if ri.startswith("RI_TIME_")]) == 1


def test_each_district_is_selectable_offline():
    world = build({"shuffle_spawn": True}, 592, fill=False).worlds[1]
    slot = world.fill_slot_data()
    for target, (_, back, _, region) in enumerate(CYCLE_SPAWNS):
        rebuilt = rebuilt_from(slot | {"spawn_target": target}, 93)
        assert rebuilt.exit_entrances[ROOT].connected_region.name == region_name(region)
        assert rebuilt.exit_entrances[region, back].can_reach(CollectionState(rebuilt.multiworld))
