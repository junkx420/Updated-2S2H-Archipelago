from collections import Counter

import pytest
from BaseClasses import CollectionState

from .. import OWL_WARP_OPTION, region_name
from ..enums_generated import ENTRANCE_MIX_POOLS, OWL_WARPS, RI_ID
from ..items_generated import ITEMS
from ..primitives import OWLS, SaveView
from ..regions_generated import OWL_WARP_ESCAPE_RULES, REGIONS
from .test_pool_trim import build
from .test_special_areas import ALL_OWN, rebuilt_from


def test_owl_warps_off_and_legacy():
    world = build({"shuffle_owl_warps": False}, 871, fill=False).worlds[1]
    slot = world.fill_slot_data()
    assert OWL_WARP_OPTION not in slot["options"] and "owl_warp_targets" not in slot
    slot["apworld_version"] = "0.14.0"
    restored = rebuilt_from(slot, 98)
    original = {key: target for key, target, _ in REGIONS["RR_MAX"][4]}
    for entrance, _, _ in OWL_WARPS:
        for instance in (world, restored):
            assert instance.exit_entrances["RR_MAX", entrance].connected_region.name == region_name(original[entrance])


@pytest.mark.parametrize("logic,all_own", [("glitchless", False), ("no_logic", False), ("glitchless", True)])
def test_owl_warp_graph_and_universal_tracker(logic, all_own):
    options = ALL_OWN | {
        "shuffle_signs": True, "shuffle_npc_souls": "all", "shuffle_bean_planting": True,
        "shuffle_epona_riding": True, "mix_entrance_pools": list(ENTRANCE_MIX_POOLS),
    } if all_own else {}
    mw = build(options | {"shuffle_owl_warps": True, "logic": logic}, 871)
    world = mw.worlds[1]
    targets = world.owl_warp_targets
    assert len(targets) == 10 and sorted(targets) == list(range(10))
    assert all(target != source for source, target in enumerate(targets))
    rebuilt = rebuilt_from(world.fill_slot_data(), 98)
    assert rebuilt.owl_warp_targets == targets
    for (entrance, _, _), target in zip(OWL_WARPS, targets):
        for instance in (world, rebuilt):
            assert instance.exit_entrances["RR_MAX", entrance].connected_region.name == region_name(OWL_WARPS[target][1])
    full = mw.get_all_state()
    assert mw.can_beat_game(full)
    assert all(loc.can_reach(full) for loc in mw.get_locations() if loc.address is not None)


@pytest.mark.parametrize("logic", ["glitchless", "no_logic"])
def test_song_of_time_can_be_shuffled(logic):
    mw = build({"shuffle_owl_warps": True, "starting_items": {}, "logic": logic}, 871, fill=False)
    world = mw.worlds[1]
    assert RI_ID["RI_SONG_TIME"] not in world.fill_slot_data()["starting_items"]
    assert "Song of Time" not in {item.name for item in mw.precollected_items[1]}
    assert "Song of Time" in {item.name for item in mw.itempool}


@pytest.mark.parametrize("destination,escape_item", [
    ("WOODFALL:4", "RI_MASK_DEKU"),
    ("STONE_TOWER:3", "RI_HOOKSHOT"),
    ("ZORA_CAPE:6", "RI_MASK_ZORA"),
    ("GREAT_BAY_COAST:11", "RI_ABILITY_SWIM"),
])
def test_owl_warp_source_activation_and_escape(destination, escape_item):
    assert destination in OWL_WARP_ESCAPE_RULES
    world = build({"shuffle_owl_warps": True, "shuffle_owl_statues": True, "shuffle_swim": True,
                   "shuffle_ocarina_buttons": True, "starting_items": {}}, 871, fill=False).worlds[1]
    target = next(i for i, row in enumerate(OWL_WARPS) if row[0] == destination)
    source = world.owl_warp_targets.index(target)
    source_key = OWL_WARPS[source][0]
    source_rule = next(rule for key, _, rule in REGIONS["RR_MAX"][4] if key == source_key)
    buttons = ["RI_OCARINA_BUTTON_C_DOWN", "RI_OCARINA_BUTTON_C_LEFT", "RI_OCARINA_BUTTON_C_UP"]
    base_items = ["RI_OCARINA", "RI_SONG_SOARING", *buttons]
    source_owl, = [owl for owl in OWLS if source_rule(SaveView(Counter([*base_items, owl]), Counter(), world))]
    state = CollectionState(world.multiworld)
    exit_ = world.exit_entrances["RR_MAX", source_key]

    def give(*items):
        for item in items:
            state.collect(world.create_item(ITEMS[item][0]), True)

    give(*base_items, "RI_SONG_TIME")
    assert not exit_.access_rule(state)
    give(source_owl)
    assert not exit_.access_rule(state)  # Song of Time still needs A and C-right.
    give(escape_item)
    assert exit_.access_rule(state)
    state.remove(world.create_item(ITEMS[escape_item][0]))
    give("RI_OCARINA_BUTTON_A", "RI_OCARINA_BUTTON_C_RIGHT")
    assert exit_.access_rule(state)

