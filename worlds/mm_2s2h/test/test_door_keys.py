from typing import Any, cast

from BaseClasses import CollectionState
from test.general import gen_steps, setup_multiworld
from worlds.AutoWorld import call_all

from .. import MM2S2HWorld, region_name
from ..enums_generated import DOOR_KEYS
from ..items_generated import ITEMS
from ..regions_generated import REGIONS
from . import MM2S2HTestBase

KEY_NAMES = {ITEMS[row["key"]][0] for row in DOOR_KEYS}
INTO = {into for row in DOOR_KEYS for into in row["into"]}


def state_without(world: MM2S2HWorld, missing: set[str]) -> CollectionState:
    """Every pool item except `missing`, swept."""
    state = CollectionState(world.multiworld)
    for item in world.multiworld.itempool:
        if item.player == world.player and item.name not in missing:
            state.collect(item, True)
    state.sweep_for_advancements()
    return state


class DoorKeysTable(MM2S2HTestBase):
    """The generated table: one key per building, every "into" a single exit from outside into another scene."""

    def test_table(self) -> None:
        self.assertEqual(len(DOOR_KEYS), 26)  # 25 buildings, since 0.7 also the Zora Hall rooms
        self.assertEqual(len(INTO), sum(len(row["into"]) for row in DOOR_KEYS))
        for row in DOOR_KEYS:
            self.assertEqual(ITEMS[row["key"]][0], row["building"] + " Key")
            for door, inside in zip(row["door_regions"], row["inside_regions"]):
                self.assertNotEqual(REGIONS[door][1], REGIONS[inside][1])


class DoorKeysOff(MM2S2HTestBase):
    """Off by default: no keys in the pool and every exit rule as generated."""

    def test_no_keys_in_pool(self) -> None:
        self.assertFalse(KEY_NAMES & {item.name for item in self.multiworld.itempool})

    def test_rules_unchanged(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        states = [CollectionState(self.multiworld), state_without(world, set())]
        # exit_entrances keeps the last exit of a region with a given entrance (the Moon has duplicates)
        rules = {(rr, key): f for rr, (*_, exits, _, _) in REGIONS.items() for key, _, f in exits}
        for (rr, key), f in rules.items():
            exit_ = world.exit_entrances[(rr, key)]
            for state in states:
                expected = f is None or bool(f(world.view(state)))
                self.assertEqual(exit_.access_rule(state), expected, f"{rr} {key}")


class DoorKeysOn(MM2S2HTestBase):
    """Every listed entrance is closed without its key and open with it; no other exit needs any key. The default
    tests of the base class add: with all items every location is reachable, and the fill works."""
    options = {"shuffle_door_keys": True}

    def test_all_keys_in_pool(self) -> None:
        pool = [item for item in self.multiworld.itempool if item.name in KEY_NAMES]
        self.assertEqual(sorted(item.name for item in pool), sorted(KEY_NAMES))
        self.assertTrue(all(item.advancement for item in pool))

    def test_entrances_need_their_key(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        for row in DOOR_KEYS:
            key = ITEMS[row["key"]][0]
            without = state_without(world, {key})
            with_key = state_without(world, set())
            for into, door, inside in zip(row["into"], row["door_regions"], row["inside_regions"]):
                exit_ = world.exit_entrances[(door, into)]
                self.assertTrue(without.can_reach_region(region_name(door), self.player), f"{door} unreachable")
                self.assertFalse(exit_.can_reach(without), f"{into} open without {key}")
                self.assertTrue(exit_.can_reach(with_key), f"{into} closed with {key}")
            for inside in row["inside_regions"]:
                self.assertFalse(without.can_reach_region(region_name(inside), self.player), f"{inside} without {key}")

    def test_no_other_exit_needs_a_key(self) -> None:
        """Ways out (and every other exit) are never locked: all items but no key opens them like all items."""
        world = cast(MM2S2HWorld, self.world)

        # Events and vanilla items behind a locked door may be missing without keys (e.g. meeting Kafei for Sakon's
        # Hideout). Collect all of them, so only a key rule on the exit itself could make a difference.
        def everything(missing: set[str]) -> CollectionState:
            state = CollectionState(self.multiworld)
            placed = [loc.item for loc in self.multiworld.get_locations(self.player) if loc.address is None and loc.item]
            for item in [*self.multiworld.itempool, *placed]:
                if item.player == self.player and item.name not in missing:
                    state.collect(item, True)
            return state
        full, no_keys = everything(set()), everything(KEY_NAMES)
        outward = 0
        insides = {rr for row in DOOR_KEYS for rr in row["inside_regions"]}
        for rr, (*_, exits, _, _) in REGIONS.items():
            for key, tgt, _ in exits:
                if key in INTO:
                    continue
                exit_ = world.exit_entrances[(rr, key)]
                self.assertEqual(exit_.access_rule(no_keys), exit_.access_rule(full), f"{rr} {key}")
                if rr in insides and REGIONS[tgt][1] != REGIONS[rr][1]:
                    outward += 1
        self.assertGreaterEqual(outward, 27)  # every building has at least one way out per way in

    def test_slot_data_option(self) -> None:
        self.assertEqual(self.world.fill_slot_data()["options"]["RO_SHUFFLE_DOOR_KEYS"], 1)


class DoorKeysWithEntrances(DoorKeysOn):
    """Together with both entrance options: the key stays on the door, whatever lies behind it."""
    options = {"shuffle_door_keys": True, "shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True}


class DoorKeysNoLogic(MM2S2HTestBase):
    """No Logic drops the key rules like every other rule."""
    options = {"shuffle_door_keys": True, "logic": "no_logic"}

    def test_everything_reachable_without_items(self) -> None:
        state = CollectionState(self.multiworld)
        self.assertEqual([loc.name for loc in self.multiworld.get_locations(self.player)
                          if loc.address and not loc.can_reach(state)], [])


class DoorKeysUniversalTracker(MM2S2HTestBase):
    """Universal Tracker rebuilds the key rules from the slot data, also with both entrance options."""
    options = {"shuffle_door_keys": True, "shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True}

    def test_regen_from_slot_data(self) -> None:
        original = cast(MM2S2HWorld, self.world)
        slot_data: dict[str, Any] = original.fill_slot_data()
        mw = setup_multiworld(MM2S2HWorld, steps=(), seed=999)
        setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: MM2S2HWorld.interpret_slot_data(slot_data)})
        for step in gen_steps:
            call_all(mw, step)
        rebuilt = cast(MM2S2HWorld, mw.worlds[1])
        self.assertEqual(original.ro, rebuilt.ro)
        self.assertEqual(original.entrance_pairs, rebuilt.entrance_pairs)

        def reachable(world: MM2S2HWorld, missing: set[str]) -> set[str]:
            state = state_without(world, missing)
            return {loc.name for loc in world.multiworld.get_locations(world.player) if loc.can_reach(state)}
        for missing in (KEY_NAMES, {ITEMS[DOOR_KEYS[0]["key"]][0]}, {ITEMS[DOOR_KEYS[11]["key"]][0]}):
            self.assertEqual(reachable(original, missing), reachable(rebuilt, missing))
        # and with only what the seed starts with
        start = lambda w: {loc.name for loc in w.multiworld.get_locations(w.player)  # noqa: E731
                           if loc.can_reach(CollectionState(w.multiworld))}
        self.assertEqual(start(original), start(rebuilt))
