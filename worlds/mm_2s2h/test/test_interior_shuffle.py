from typing import Any, cast

from BaseClasses import CollectionState
from test.general import gen_steps, setup_multiworld
from worlds.AutoWorld import call_all

from .. import MM2S2HWorld, region_name
from ..enums_generated import DOOR_KEYS, ENTRANCE_SHUFFLE
from ..items_generated import ITEMS
from ..regions_generated import REGIONS
from ..time_logic import TIME_SLICE_COUNT, TimeContext, half_day_mask
from . import MM2S2HTestBase
from . import test_dungeon_items as di, test_entrance_shuffle as es
from .test_door_keys import state_without
from .test_entrance_shuffle import BY_VALUE, connected, vanilla_target

INTERIORS = [row for row in ENTRANCE_SHUFFLE if row["hours"]]
BY_INTO = {row["into"]: row for row in INTERIORS}
KEY_OF = {into: ITEMS[row["key"]][0] for row in DOOR_KEYS for into in row["into"]}
ALL_OWN = {"shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True, "shuffle_grottos": True,
           "shuffle_door_keys": True, "shuffle_npc_souls": True, "shuffle_interiors": True}
ALL_SLICES = (1 << TIME_SLICE_COUNT) - 1


def door_rule(row: dict):
    return next(f for key, _, f in REGIONS[row["door_region"]][4] if key == row["into"])


def contents(row: dict) -> list:
    """Every condition inside the building: checks, connections, exits and events of the region behind the door."""
    _, _, checks, conns, exits, events, _ = REGIONS[row["inside_region"]]
    return [(n, f) for n, f in checks] + [(n, f) for n, f in conns] + [(n, f) for n, _, f in exits] + \
           [(n, f) for n, f in events]


def with_pairs(original: MM2S2HWorld, swaps: list[tuple[str, str]]) -> MM2S2HWorld:
    """Universal Tracker rebuild of `original` with the interior pairing replaced: each (a, b) swaps doors a and b."""
    slot_data: dict[str, Any] = original.fill_slot_data()
    behind = {door: inside for door, inside in slot_data["entrance_pairs"]}
    for a, b in swaps:
        va, vb = BY_INTO[a]["into_value"], BY_INTO[b]["into_value"]
        behind[va], behind[vb] = vb, va
    slot_data["entrance_pairs"] = [[door, inside] for door, inside in behind.items()]
    mw = setup_multiworld(MM2S2HWorld, steps=(), seed=4242)
    setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: MM2S2HWorld.interpret_slot_data(slot_data)})
    for step in gen_steps:
        call_all(mw, step)
    return cast(MM2S2HWorld, mw.worlds[1])


class InteriorTable(MM2S2HTestBase):
    """32 doors in three hours classes; the Inn's two doors share one region. The generator already proves that
    every row is a plain door into a dead end and back."""

    def test_table(self) -> None:
        self.assertEqual(len(INTERIORS), 32)
        classes = {h: {row["into"] for row in INTERIORS if row["hours"] == h} for h in ("ALWAYS_OPEN", "DAYTIME", "OWN_HOURS")}
        self.assertEqual([len(classes[h]) for h in classes], [17, 9, 6])
        self.assertEqual(classes["OWN_HOURS"], {"CURIOSITY_SHOP:0", "CURIOSITY_SHOP:1", "POST_OFFICE:0", "MILK_BAR:0",
                                                "STOCK_POT_INN:0", "RANCH_HOUSE:0"})
        self.assertEqual([row["into"] for row in INTERIORS if row["inside_region"] == "RR_INN"],
                         ["STOCK_POT_INN:0", "STOCK_POT_INN:1"])
        zora = next(row for row in DOOR_KEYS if row["key"] == "RI_DOOR_KEY_ZORA_HALL_ROOMS")
        self.assertEqual(sorted(zora["into"]), [f"ZORA_HALL_ROOMS:{i}" for i in (0, 1, 2, 3, 5)])
        # appended after every older row: saved row numbers of 0.3-0.6 pairings stay valid (0.8 appends after them)
        first = ENTRANCE_SHUFFLE.index(INTERIORS[0])
        self.assertEqual(ENTRANCE_SHUFFLE[first:first + len(INTERIORS)], INTERIORS)
        self.assertEqual(first + len(INTERIORS), 66)


class InteriorHoursClasses(MM2S2HTestBase):
    """The time logic gives every region the same mask, so it never tests that a door and a check inside are open at
    the same time. Proof for each class: for every door A and building X of one class, every condition c inside X
    and every half-day, if c can hold during X's own door hours, it can also hold while A is open as well."""

    def slices(self, view, f) -> int:
        if f is None:
            return ALL_SLICES
        saved, mask = view.time, 0
        for t in range(TIME_SLICE_COUNT):
            ctx = TimeContext(False, 0, 0)
            ctx.mask = 1 << t
            view.time = ctx
            if f(view):
                mask |= 1 << t
        view.time = saved
        return mask

    def test_classes_keep_hours(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        view = world.view(self.multiworld.get_all_state())
        hours = {row["into"]: self.slices(view, door_rule(row)) for row in INTERIORS}
        # The Town Shooting Gallery shows why: its checks only need the bow, its door closes at 22.
        gallery = BY_INTO["TOWN_SHOOTING_GALLERY:0"]
        self.assertNotEqual(hours[gallery["into"]], ALL_SLICES)
        self.assertTrue(all(f is None or self.slices(view, f) == ALL_SLICES for _, f in contents(gallery)))
        pairs = 0
        for cls in ("ALWAYS_OPEN", "DAYTIME"):
            members = [row for row in INTERIORS if row["hours"] == cls]
            for door in members:
                for inside in members:
                    own = hours[inside["into"]]
                    for name, f in contents(inside):
                        need = self.slices(view, f) & own
                        for h in range(6):
                            half = half_day_mask(h)
                            if need & half:
                                self.assertTrue(need & hours[door["into"]] & half, (cls, door["into"], inside["into"], name, h))
                    pairs += 1
        self.assertEqual(pairs, 17 * 17 + 9 * 9)


class InteriorShuffleOff(MM2S2HTestBase):
    """Option off (default): no pairing, every door and way back exactly as generated."""

    def test_graph_unchanged(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        self.assertEqual(world.fill_slot_data()["options"]["RO_SHUFFLE_INTERIORS"], 0)
        self.assertEqual(world.entrance_pairs, [])
        for row in INTERIORS:
            self.assertEqual(connected(world, row["door_region"], row["into"]), region_name(row["inside_region"]))
            self.assertEqual(connected(world, row["inside_region"], row["out"]), vanilla_target(row["inside_region"], row["out"]))


class InteriorShuffleOn(es.EntranceShuffleOn):
    """Option on: a coupled permutation inside each hours class, OWN_HOURS rows stay; Universal Tracker rebuilds it
    (inherited). Base tests: everything reachable with all items, and the fill works."""
    options = {"shuffle_interiors": True}

    def test_pairs_within_class(self) -> None:
        moved = set()
        for seed in range(8):
            self.world_setup(seed)
            world = cast(MM2S2HWorld, self.world)
            self.assertEqual(len(world.entrance_pairs), 32)
            for door_value, inside_value in world.entrance_pairs:
                door, inside = BY_VALUE[door_value], BY_VALUE[inside_value]
                self.assertTrue(door["hours"] and door["hours"] == inside["hours"], (door["into"], inside["into"]))
                if door["hours"] == "OWN_HOURS":
                    self.assertIs(door, inside)
                elif door is not inside:
                    moved.add(door["hours"])
            # the Inn: each of its doors comes back out through its own way back
            for row in INTERIORS:
                if row["inside_region"] == "RR_INN":
                    source = next(BY_VALUE[d] for d, i in world.entrance_pairs if i == row["into_value"])
                    self.assertEqual(connected(world, row["inside_region"], row["out"]), region_name(source["door_region"]))
        self.assertEqual(moved, {"ALWAYS_OPEN", "DAYTIME"})


class InteriorAreaRules(MM2S2HTestBase):
    """A door keeps its own rule; a foreign building adds its area rule (Music Box House: Sharp's curse broken,
    Evan's room: Zora). Forced pairings via the Universal Tracker path."""
    options = {"shuffle_interiors": True}

    def test_area_rules(self) -> None:
        world = with_pairs(cast(MM2S2HWorld, self.world),
                           [("BOMB_SHOP:0", "MUSIC_BOX_HOUSE:0"), ("TRADING_POST:0", "ZORA_HALL_ROOMS:3")])
        exit_of = {row["into"]: world.exit_entrances[(row["door_region"], row["into"])] for row in INTERIORS}

        def without(item: str) -> CollectionState:
            return state_without(world, es.item_names(item))
        storms, zora = without("RI_SONG_STORMS"), without("RI_MASK_ZORA")
        full = state_without(world, set())
        for into in ("BOMB_SHOP:0", "MUSIC_BOX_HOUSE:0", "TRADING_POST:0", "ZORA_HALL_ROOMS:3"):
            self.assertTrue(exit_of[into].access_rule(full), into)
        self.assertFalse(exit_of["BOMB_SHOP:0"].access_rule(storms))      # area rule of the music box house
        self.assertFalse(exit_of["MUSIC_BOX_HOUSE:0"].access_rule(storms))  # its own door rule stays
        self.assertTrue(exit_of["BOMB_SHOP:0"].access_rule(zora))
        self.assertFalse(exit_of["TRADING_POST:0"].access_rule(zora))     # area rule of Evan's room
        self.assertFalse(exit_of["ZORA_HALL_ROOMS:3"].access_rule(zora))  # the band room door needs Zora itself
        self.assertTrue(exit_of["TRADING_POST:0"].access_rule(storms))


class InteriorDoorKeys(MM2S2HTestBase):
    """With door keys: the key of the door opens it, whatever building lies behind (the client tests the entrance of
    setupExitList before the redirect, DoorKeys::IsEntranceLocked)."""
    options = {"shuffle_interiors": True, "shuffle_door_keys": True}

    def test_key_stays_at_door(self) -> None:
        checked = 0
        for seed in range(6):
            self.world_setup(seed)
            world = cast(MM2S2HWorld, self.world)
            self.assertIn("Zora Hall Rooms Key", {item.name for item in self.multiworld.itempool})
            for door_value, inside_value in world.entrance_pairs:
                door, inside = BY_VALUE[door_value], BY_VALUE[inside_value]
                exit_ = world.exit_entrances[(door["door_region"], door["into"])]
                own, foreign = KEY_OF.get(door["into"]), KEY_OF.get(inside["into"])
                if own:
                    self.assertFalse(exit_.access_rule(state_without(world, {own})), door["into"])
                if foreign and foreign != own:
                    self.assertTrue(exit_.access_rule(state_without(world, {foreign})), (door["into"], inside["into"]))
                    checked += 1
                # the way out of the building is never locked
                back = world.exit_entrances[(inside["inside_region"], inside["out"])]
                self.assertTrue(back.access_rule(state_without(world, set(KEY_OF.values()))), inside["out"])
        self.assertGreater(checked, 0)


class InteriorShuffleNoLogic(es.EntranceShuffleNoLogic):
    options = {"shuffle_interiors": True, "logic": "no_logic"}


class InteriorShuffleWithAllOwnOptions(es.EntranceShuffleOn):
    """Together with temples, bosses, grottos, door keys and NPC souls: every group on its own, still beatable."""
    options = ALL_OWN


class InteriorShuffleAllNoLogic(es.EntranceShuffleNoLogic):
    options = ALL_OWN | {"logic": "no_logic"}


class InteriorShuffleOwnDungeonItems(di.OwnDungeonDefaultChecks):
    options = di.OwnDungeonDefaultChecks.options | ALL_OWN


class InteriorLegacySlots(MM2S2HTestBase):
    """Slot data of 0.6 (no RO_SHUFFLE_INTERIORS, no Zora Hall Rooms key): rebuilt unchanged, the Zora Hall doors
    without a key rule, like the client keeps them open for such a seed."""
    options = {"shuffle_grottos": True, "shuffle_door_keys": True}

    def test_old_slot_data(self) -> None:
        original = cast(MM2S2HWorld, self.world)
        data = original.fill_slot_data()
        data["apworld_version"] = "0.6.0"
        data["options"] = dict(data["options"])
        data["options"].pop("RO_SHUFFLE_INTERIORS")
        mw = setup_multiworld(MM2S2HWorld, steps=(), seed=999)
        setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: data})
        for step in gen_steps:
            call_all(mw, step)
        rebuilt = cast(MM2S2HWorld, mw.worlds[1])
        self.assertFalse(rebuilt.ro["RO_SHUFFLE_INTERIORS"])
        self.assertEqual(sorted(original.entrance_pairs), sorted(rebuilt.entrance_pairs))
        for rr, key in original.exit_entrances:
            self.assertEqual(connected(rebuilt, rr, key), connected(original, rr, key), (rr, key))
        state = CollectionState(mw)
        for item in mw.itempool:
            if item.player == rebuilt.player and item.name != "Zora Hall Rooms Key":
                state.collect(item, True)
        state.sweep_for_advancements()
        for i in (0, 1, 2, 3, 5):
            exit_ = rebuilt.exit_entrances[("RR_ZORA_HALL", f"ZORA_HALL_ROOMS:{i}")]
            self.assertTrue(exit_.access_rule(state), i)
            self.assertFalse(original.exit_entrances[("RR_ZORA_HALL", f"ZORA_HALL_ROOMS:{i}")].access_rule(
                state_without(original, {"Zora Hall Rooms Key"})), i)
