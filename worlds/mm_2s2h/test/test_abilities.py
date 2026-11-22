import json
from collections import Counter
from itertools import product

import pytest
from BaseClasses import CollectionState

from .. import BASE_ID
from ..enums_generated import ABILITIES, ENTRANCE_MIX_POOLS, RI_ID
from ..items_generated import ITEMS
from ..macros_generated import CAN_GROW_BEAN_PLANT, CAN_RIDE_EPONA, CAN_USE_DAY2_RAIN_BEAN
from ..primitives import SaveView
from ..regions_generated import REGIONS
from .test_pool_trim import CROWDED, build
from .test_special_areas import ALL_OWN, DATA, rebuilt_from


RIDE = "RI_ABILITY_RIDE_EPONA"
PLANT = "RI_ABILITY_PLANT_BEANS"
RIDING = {"shuffle_epona_riding": True}
PLANTING = {"shuffle_bean_planting": True}
BOTH = RIDING | PLANTING
SONG = ("RI_OCARINA", "RI_SONG_EPONA")
BEANS = ("RI_MAGIC_BEAN",)
STORMS = ("RI_OCARINA", "RI_SONG_STORMS")
ALIENS = ("RE_COWS_FROM_ALIENS",)


def rule(rr, target):
    return dict(REGIONS[rr][2 if target.startswith("RC_") else 3])[target]


def view(world, items=(), events=()):
    return SaveView(Counter(world.starting) + Counter(items), Counter(events), world)


@pytest.mark.parametrize("ride,plant", list(product((False, True), repeat=2)))
def test_independent_progression_items(ride, plant):
    mw = build({"shuffle_epona_riding": ride, "shuffle_bean_planting": plant}, 411, fill=False)
    pool = Counter(item.name for item in mw.itempool)
    for item, enabled in ((RIDE, ride), (PLANT, plant)):
        assert pool[ITEMS[item][0]] == enabled
        created = mw.worlds[1].create_item(ITEMS[item][0])
        assert created.advancement and created.code == BASE_ID + RI_ID[item]


def test_riding_rules():
    world = build(RIDING, 411, fill=False).worlds[1]
    riding_rules = [CAN_RIDE_EPONA,
                    rule("RR_ROAD_TO_IKANA_BELOW_LEDGE", "RR_ROAD_TO_IKANA_FIELD_SIDE"),
                    rule("RR_ROAD_TO_IKANA_FIELD_SIDE", "RR_ROAD_TO_IKANA_BELOW_LEDGE"),
                    rule("RR_TERMINA_FIELD", "RR_TERMINA_FIELD_BEFORE_GREAT_BAY_COAST"),
                    rule("RR_TERMINA_FIELD_BEFORE_GREAT_BAY_COAST", "RR_TERMINA_FIELD"),
                    rule("RR_GORMAN_TRACK_FRONT", "RC_GORMAN_TRACK_GARO_MASK"),
                    rule("RR_GORMAN_TRACK", "RR_GORMAN_TRACK_FRONT"),
                    rule("RR_GORMAN_TRACK", "RR_GORMAN_TRACK_BACK")]
    for riding_rule in riding_rules:
        assert not riding_rule(view(world, SONG))
        assert not riding_rule(view(world, (RIDE,)))
        assert riding_rule(view(world, SONG + (RIDE,)))
    assert rule("RR_TERMINA_FIELD_COW_GROTTO", "RC_TERMINA_FIELD_COW_FRONT")(view(world, SONG))
    practice = rule("RR_ROMANI_RANCH", "RC_ROMANI_RANCH_EPONAS_SONG")
    assert practice(view(world, ("RI_MASK_GORON", "RI_POWDER_KEG")))


def test_night_two_gorman_escape_without_riding():
    world = build(RIDING | {"clock_shuffle": True, "clock_shuffle_mode": "any_order"}, 411, fill=False).worlds[1]
    for side in ("RR_GORMAN_TRACK_FRONT", "RR_GORMAN_TRACK_BACK"):
        escape = rule("RR_GORMAN_TRACK", side)
        assert escape(view(world, ("RI_TIME_NIGHT_2",), ALIENS))
        assert not escape(view(world, ("RI_TIME_NIGHT_2",)))
        assert not escape(view(world, ("RI_TIME_DAY_2",), ALIENS))


@pytest.mark.parametrize("water,events", [(STORMS, ()), (("RI_BOTTLE_EMPTY",), ("RE_ACCESS_SPRING_WATER",)),
                                         (("RI_BOTTLE_EMPTY",), ("RE_ACCESS_HOT_SPRING_WATER",))])
def test_manual_bean_watering(water, events):
    world = build(PLANTING, 411, fill=False).worlds[1]
    for items in (BEANS + water, (PLANT,) + water, (PLANT,) + BEANS):
        assert not CAN_GROW_BEAN_PLANT(view(world, items, events))
    assert CAN_GROW_BEAN_PLANT(view(world, (PLANT,) + BEANS + water, events))


def test_day_two_rain_bean_branch_and_alternatives():
    world = build(PLANTING | {"clock_shuffle": True, "clock_shuffle_mode": "any_order"}, 411, fill=False).worlds[1]
    day = BEANS + ("RI_TIME_DAY_2", "RI_MASK_DEKU")
    rain_rules = [CAN_USE_DAY2_RAIN_BEAN,
                  rule("RR_DOGGY_RACETRACK", "RC_DOGGY_RACETRACK_CHEST"),
                  rule("RR_DEKU_PALACE_OUTSIDE", "RR_DEKU_PALACE_INSIDE_UPPER_BEAN_SIDE")]
    for rain_rule in rain_rules:
        assert not rain_rule(view(world, day))
        assert not rain_rule(view(world, day[1:] + (PLANT,)))
        assert not rain_rule(view(world, BEANS + ("RI_TIME_NIGHT_2", "RI_MASK_DEKU", PLANT)))
        assert rain_rule(view(world, day + (PLANT,)))
        assert rain_rule(view(world, BEANS + STORMS + ("RI_TIME_NIGHT_2", "RI_MASK_DEKU", PLANT)))
    assert not CAN_GROW_BEAN_PLANT(view(world, day + (PLANT,)))
    assert rain_rules[1](view(world, ("RI_HOOKSHOT",)))
    assert rain_rules[1](view(world, ("RI_MASK_ZORA",)))
    assert dict(REGIONS["RR_SOUTHERN_SWAMP_NORTH"][5])["RE_ACCESS_BEANS_REFILL"](view(world, day))


def test_crowded_plentiful_pool_preserves_both_originals():
    mw = build(BOTH | CROWDED, 411)
    assert {ITEMS[item][0] for item in ABILITIES} <= {item.name for item in mw.itempool}
    assert len(mw.itempool) == sum(loc.address is not None for loc in mw.get_locations())
    assert mw.can_beat_game()


@pytest.mark.parametrize("logic", ["no_logic", "nearly_no_logic"])
def test_no_logic_keeps_the_items_but_ignores_the_gates(logic):
    mw = build(BOTH | {"logic": logic}, 411, fill=False)
    assert {ITEMS[ri][0] for ri in ABILITIES} <= {item.name for item in mw.itempool}
    assert all(loc.can_reach(CollectionState(mw)) for loc in mw.get_locations() if loc.address is not None)


def test_mixed_fill_and_universal_tracker():
    mw = build(ALL_OWN | BOTH | {"mix_entrance_pools": list(ENTRANCE_MIX_POOLS)}, 610)
    world = mw.worlds[1]
    assert mw.can_beat_game()
    rebuilt = rebuilt_from(world.fill_slot_data(), 4242)
    assert rebuilt.ro == world.ro
    assert rebuilt.mixed_entrance_pairs == world.mixed_entrance_pairs
    assert rebuilt.entrance_pairs == world.entrance_pairs

    def reachable(w, missing):
        state = CollectionState(w.multiworld)
        for item in mw.itempool:
            if item.name not in missing:
                state.collect(w.create_item(item.name), True)
        state.sweep_for_advancements(loc for loc in w.multiworld.get_locations() if loc.address is None)
        return {loc.name for loc in w.multiworld.get_locations() if loc.address is not None and loc.can_reach(state)}

    for missing in (set(), {ITEMS[RIDE][0]}, {ITEMS[PLANT][0]}, {ITEMS[RIDE][0], ITEMS[PLANT][0]}):
        assert reachable(world, missing) == reachable(rebuilt, missing)
    assert len(reachable(world, set())) == len(world.shuffled)


def test_real_010_slot_data_defaults_abilities_off():
    slot = json.loads((DATA / "slot_data_0_10_0_mixed.json").read_text(encoding="utf-8"))
    assert slot["apworld_version"] == "0.10.0"
    world = rebuilt_from(slot, 4242)
    pools = [pool for pool in ENTRANCE_MIX_POOLS if pool != "grottos"]
    current = build(ALL_OWN | {"mix_entrance_pools": pools}, 610, fill=False).worlds[1]
    for opt, _ in ABILITIES.values():
        assert opt not in slot["options"] and world.ro[opt] == 0
    assert world.entrance_pairs == current.entrance_pairs == list(map(tuple, slot["entrance_pairs"]))
    assert world.mixed_entrance_pairs == current.mixed_entrance_pairs == list(map(tuple, slot["mixed_entrance_pairs"]))
    assert {key: e.connected_region.name for key, e in world.exit_entrances.items()} == {
        key: e.connected_region.name for key, e in current.exit_entrances.items()}
