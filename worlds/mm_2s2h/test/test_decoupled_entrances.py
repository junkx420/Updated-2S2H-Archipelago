import pytest

from .. import DECOUPLE_OPTION, region_name
from ..enums_generated import ENTRANCE_MIX_POOLS, ENTRANCE_SHUFFLE
from .test_pool_trim import build
from .test_special_areas import ALL_OWN, rebuilt_from


def graph(world):
    return {key: exit_.connected_region.name for key, exit_ in world.exit_entrances.items()}


def test_decoupled_off_and_legacy_slot():
    world = build(ALL_OWN, 5, fill=False).worlds[1]
    slot = world.fill_slot_data()
    assert not world.directed_entrance_targets
    assert DECOUPLE_OPTION not in slot["options"] and "directed_entrance_targets" not in slot
    assert graph(world) == graph(rebuilt_from(slot | {"apworld_version": "0.16.0"}, 8))


@pytest.mark.parametrize("mixed", [False, True])
@pytest.mark.parametrize("logic", ["glitchless", "no_logic"])
def test_directed_map_escapes_and_universal_tracker(mixed, logic):
    opts = ALL_OWN | {
        "shuffle_owl_warps": True, "shuffle_spawn": True, "shuffle_signs": True, "shuffle_rocks": True,
        "shuffle_npc_souls": "all", "shuffle_bean_planting": True, "shuffle_epona_riding": True,
        "decouple_entrances": True, "starting_items": {}, "logic": logic,
    }
    if mixed:
        opts["mix_entrance_pools"] = list(ENTRANCE_MIX_POOLS)
    mw = build(opts, 5)
    world = mw.worlds[1]
    full = mw.get_all_state()
    assert mw.can_beat_game(full)
    assert all(loc.can_reach(full) for loc in mw.get_locations() if loc.address is not None)
    original = graph(world)
    rebuilt = rebuilt_from(world.fill_slot_data(), 8)
    assert graph(rebuilt) == original
    targets = dict(world.directed_entrance_targets)
    assert world.directed_entrance_targets == rebuilt.directed_entrance_targets
    assert set(targets) == set(targets.values())
    assert any(targets[target] != source for source, target in targets.items())
    assert all(not world._town_gate(ENTRANCE_SHUFFLE[end // 2]) for end in targets)
    assert all(not ENTRANCE_SHUFFLE[end // 2]["water"] for end in targets)
    grotto_pairs = [(a, b) for a, b in world.mixed_entrance_pairs
                    if any(ENTRANCE_SHUFFLE[end // 2]["grotto"] for end in (a, b))]
    if mixed:
        assert grotto_pairs
    for a, b in grotto_pairs:
        assert a not in targets and b not in targets
        for source, arrival in ((a, b), (b, a)):
            rr, key, _ = world._endpoint(source)
            assert original[rr, key] == region_name(world._endpoint(arrival)[0])
    for source, target in targets.items():
        source_rr, source_key, _ = world._endpoint(source)
        target_rr, _, _ = world._endpoint(target)
        assert original[source_rr, source_key] == region_name(target_rr)

    baseline, _ = world._directed_groups()
    witnesses = {}
    unsafe, critical = world._escape_failures(targets, witnesses)
    assert critical == world.directed_escape_checks
    assert all(targets[end] == baseline[end] and targets[targets[end]] == end for end in unsafe)
    world._apply_directed(baseline)
    originally_unsafe, _ = world._escape_failures(baseline, witnesses)
    assert unsafe <= originally_unsafe
    world._apply_directed(targets)


def test_decouple_only_changes_enabled_entrance_pools():
    world = build({"decouple_entrances": True, "shuffle_interiors": True}, 5, fill=False).worlds[1]
    assert {ENTRANCE_SHUFFLE[end // 2]["option"] for end, _ in world.directed_entrance_targets} == {
        "RO_SHUFFLE_INTERIORS",
    }
    assert not build({"decouple_entrances": True}, 5, fill=False).worlds[1].directed_entrance_targets


@pytest.mark.parametrize("logic", ["glitchless", "no_logic"])
def test_woodfall_ledge_requires_an_escape_even_in_no_logic(logic):
    world = build({"decouple_entrances": True, "shuffle_dungeon_entrances": True, "starting_items": {},
                   "logic": logic}, 1, fill=False).worlds[1]
    baseline, _ = world._directed_groups()
    woodfall = next(end for end in baseline if world._endpoint(end)[2] == "WOODFALL:1")
    snowhead = next(end for end in baseline if world._endpoint(end)[0] == "RR_SNOWHEAD_TEMPLE_ENTRANCE_BEFORE_BLOCK")
    old_source = next(end for end, dest in baseline.items() if dest == woodfall)
    forced = baseline | {snowhead: woodfall, old_source: baseline[snowhead]}
    if old_source == snowhead:
        original = dict(world.directed_entrance_targets)
        assert original[snowhead] == woodfall
    world._apply_directed(forced)
    witnesses = {}
    unsafe, critical = world._escape_failures(forced, witnesses)
    assert critical and snowhead in unsafe
    assert "RI_MASK_DEKU" not in witnesses[snowhead].ri
