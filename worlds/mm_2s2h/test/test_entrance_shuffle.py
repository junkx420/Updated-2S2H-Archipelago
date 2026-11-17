from typing import Any, cast

from BaseClasses import CollectionState, Entrance

from test.general import gen_steps, setup_multiworld
from worlds.AutoWorld import call_all

from .. import MM2S2HWorld, region_name
from ..enums_generated import ENTRANCE_SHUFFLE
from ..items_generated import ITEMS
from ..regions_generated import ENTRANCE_AREA_RULES, REGIONS
from . import MM2S2HTestBase
from .test_dungeon_items import OwnDungeonDefaultChecks

DUNGEONS = [row for row in ENTRANCE_SHUFFLE if row["temple_exit"] is None]
BOSSES = [row for row in ENTRANCE_SHUFFLE if row["temple_exit"] is not None]
BY_VALUE = {row["into_value"]: row for row in ENTRANCE_SHUFFLE}


def vanilla_target(rr: str, key: str) -> str:
    return region_name(next(t for k, t, _ in REGIONS[rr][4] if k == key))


def connected(world: MM2S2HWorld, rr: str, key: str) -> str:
    return world.exit_entrances[(rr, key)].connected_region.name


class EntranceShuffleOff(MM2S2HTestBase):
    """Option off: no pairing in the slot data and every door, way back and boss warp as generated."""

    def test_graph_unchanged(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        self.assertEqual(world.fill_slot_data()["entrance_pairs"], [])
        for row in ENTRANCE_SHUFFLE:
            self.assertEqual(connected(world, row["door_region"], row["into"]), region_name(row["inside_region"]))
            if row["temple_exit"] is None:
                # As generated; only grotto 27's ambiguous way back differs from its hole's region (the cliffside).
                self.assertEqual(connected(world, row["inside_region"], row["out"]),
                                 vanilla_target(row["inside_region"], row["out"]))
            for boss_rr, warp in row["boss_warps"]:
                self.assertEqual(connected(world, boss_rr, warp), vanilla_target(boss_rr, warp))


class EntranceShuffleOn(MM2S2HTestBase):
    """Option on: the pairing is a permutation of the four temples, coupled, and bosses warp to the door used.
    The default tests of the base class add: with all items every location is reachable, and the fill works."""
    options = {"shuffle_dungeon_entrances": True}

    def check_pairing(self) -> list[int]:
        world = cast(MM2S2HWorld, self.world)
        pairs = world.fill_slot_data()["entrance_pairs"]
        enabled = [row for row in ENTRANCE_SHUFFLE if world.ro[row["option"]]]
        values = sorted(row["into_value"] for row in enabled)
        self.assertEqual(sorted(door for door, _ in pairs), values)
        self.assertEqual(sorted(inside for _, inside in pairs), values)
        for door_value, inside_value in pairs:
            door, inside = BY_VALUE[door_value], BY_VALUE[inside_value]
            self.assertEqual(connected(world, door["door_region"], door["into"]), region_name(inside["inside_region"]))
            self.assertEqual(door["option"], inside["option"])
            if door["temple_exit"] is None:
                self.assertEqual(connected(world, inside["inside_region"], inside["out"]), region_name(door["door_region"]))
        # Independently follow the actual graph: which boss door leads to this boss, and where does its temple exit?
        for boss in BOSSES:
            source = next(row for row in BOSSES if connected(world, row["door_region"], row["into"]) == region_name(boss["inside_region"]))
            temple = next((row for row in DUNGEONS if row["out"] == source["temple_exit"]), None)
            if temple is None:
                exterior = region_name("RR_STONE_TOWER_INVERTED_NEAR_TEMPLE")
                moved_temple = False
            else:
                exterior = connected(world, temple["inside_region"], temple["out"])
                moved_temple = exterior != region_name(temple["door_region"])
            for boss_rr, warp in boss["boss_warps"]:
                expected = exterior if source is not boss or moved_temple else vanilla_target(boss_rr, warp)
                self.assertEqual(connected(world, boss_rr, warp), expected, (source["into"], boss["into"]))
        self.assertTrue(self.multiworld.can_beat_game(self.multiworld.get_all_state()))
        return [inside for _, inside in sorted(pairs)]

    def test_pairings_over_seeds(self) -> None:
        seen = set()
        for seed in range(12):
            self.world_setup(seed)
            seen.add(tuple(self.check_pairing()))
        self.assertGreater(len(seen), 1, "every seed produced the same pairing")
        values = tuple(sorted(row["into_value"] for row in ENTRANCE_SHUFFLE if self.world.ro[row["option"]]))
        self.assertTrue(any(order != values for order in seen), "never shuffled")

    def test_ut_rebuilds_pairing(self) -> None:
        original = cast(MM2S2HWorld, self.world)
        slot_data: dict[str, Any] = original.fill_slot_data()
        mw = setup_multiworld(MM2S2HWorld, steps=(), seed=4242)
        setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: MM2S2HWorld.interpret_slot_data(slot_data)})
        for step in gen_steps:
            call_all(mw, step)
        rebuilt = cast(MM2S2HWorld, mw.worlds[1])
        self.assertEqual(sorted(original.entrance_pairs), sorted(rebuilt.entrance_pairs))
        keys = [(row["door_region"], row["into"]) for row in ENTRANCE_SHUFFLE] + \
               [(row["inside_region"], row["out"]) for row in ENTRANCE_SHUFFLE] + \
               [warp for row in ENTRANCE_SHUFFLE for warp in row["boss_warps"]]
        for rr, key in keys:
            self.assertEqual(connected(original, rr, key), connected(rebuilt, rr, key), key)


class EntranceShuffleOwnDungeonItems(OwnDungeonDefaultChecks):
    """Own-dungeon keys and fairies stay in the temple they belong to, whichever door it is behind."""
    options = OwnDungeonDefaultChecks.options | {"shuffle_dungeon_entrances": True}


# what the area rule of each temple needs, one item per temple that the test takes away (shuffleAreaRules)
AREA_ITEMS = {"WOODFALL_TEMPLE:0": ["RI_MASK_DEKU"], "GREAT_BAY_TEMPLE:0": ["RI_MASK_ZORA", "RI_HOOKSHOT"],
              "STONE_TOWER_TEMPLE:0": ["RI_HOOKSHOT", "RI_MASK_ZORA", "RI_MASK_GORON", "RI_SONG_ELEGY"]}
BOSS_AREA_ITEMS = {"ODOLWAS_LAIR:0": ["RI_MASK_DEKU", "RI_BOW"], "GOHTS_LAIR:0": ["RI_MASK_GORON"],
                   "GYORGS_LAIR:0": ["RI_MASK_ZORA"],
                   "TWINMOLDS_LAIR:0": ["RI_BOW", "RI_ARROW_LIGHT", "RI_SINGLE_MAGIC"]}


def item_names(item: str) -> set[str]:
    equivalents = {"RI_SINGLE_MAGIC": ("RI_SINGLE_MAGIC", "RI_DOUBLE_MAGIC", "RI_PROGRESSIVE_MAGIC"),
                   "RI_BOW": ("RI_BOW", "RI_PROGRESSIVE_BOW")}
    return {ITEMS[ri][0] for ri in equivalents.get(item, (item,))}


class EntranceShuffleAreaRules(MM2S2HTestBase):
    """A door with another temple behind it also needs that temple's area rule; a door with its own temple keeps
    exactly its own rule; Universal Tracker sets the same rules."""
    options = {"shuffle_dungeon_entrances": True}
    area_items = AREA_ITEMS

    def without(self, item: str) -> CollectionState:
        state = self.multiworld.get_all_state()
        names = item_names(item)
        for it in [i for i in self.multiworld.itempool if i.player == self.player and i.name in names]:
            state.remove(it)
        return state

    def doors(self, world: MM2S2HWorld) -> list[tuple[dict, dict, Entrance]]:
        out = []
        for door_value, inside_value in world.entrance_pairs:
            door, inside = BY_VALUE[door_value], BY_VALUE[inside_value]
            out.append((door, inside, world.exit_entrances[(door["door_region"], door["into"])]))
        return out

    def own_rule(self, world: MM2S2HWorld, door: dict):
        f = next(f for k, _, f in REGIONS[door["door_region"]][4] if k == door["into"])
        return world._rule(f) or (lambda state: True)

    def test_area_rules(self) -> None:
        temple_rows = {r["into"] for r in ENTRANCE_SHUFFLE
                       if r["option"] not in ("RO_SHUFFLE_INTERIORS", "RO_SHUFFLE_OVERWORLD", "RO_SHUFFLE_SPECIAL_AREAS")}
        self.assertEqual(set(ENTRANCE_AREA_RULES) & temple_rows, set(AREA_ITEMS) | set(BOSS_AREA_ITEMS))
        foreign = 0
        for seed in range(12):
            self.world_setup(seed)
            world = cast(MM2S2HWorld, self.world)
            for door, inside, exit_ in self.doors(world):
                own = self.own_rule(world, door)
                for item in sorted({i for items in self.area_items.values() for i in items}):
                    state = self.without(item)
                    needed = inside is not door and item in self.area_items.get(inside["into"], [])
                    if needed:
                        self.assertFalse(exit_.access_rule(state), (door["into"], inside["into"], item))
                        foreign += own(state)  # passable by its own rule, so the area rule blocks it
                    else:
                        self.assertEqual(exit_.access_rule(state), own(state), (door["into"], inside["into"], item))
        self.assertGreater(foreign, 0, "no door was blocked by the area rule alone")

    def test_ut_sets_same_rules(self) -> None:
        original = cast(MM2S2HWorld, self.world)
        slot_data: dict[str, Any] = original.fill_slot_data()
        mw = setup_multiworld(MM2S2HWorld, steps=(), seed=4242)
        setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: MM2S2HWorld.interpret_slot_data(slot_data)})
        for step in gen_steps:
            call_all(mw, step)
        rebuilt = cast(MM2S2HWorld, mw.worlds[1])
        items = sorted({i for items in self.area_items.values() for i in items})
        for (door, inside, a), (_, _, b) in zip(self.doors(original), self.doors(rebuilt)):
            for item in items:
                sa = self.without(item)
                sb = mw.get_all_state()
                names = item_names(item)
                for it in [i for i in mw.itempool if i.name in names]:
                    sb.remove(it)
                self.assertEqual(a.access_rule(sa), b.access_rule(sb), (door["into"], inside["into"], item))


class EntranceShuffleNoLogic(MM2S2HTestBase):
    """No Logic: like every rule, the area rules do not apply."""
    options = {"shuffle_dungeon_entrances": True, "logic": "no_logic"}

    def test_doors_open(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        state = CollectionState(self.multiworld)
        for door_value, _ in world.entrance_pairs:
            door = BY_VALUE[door_value]
            self.assertTrue(world.exit_entrances[(door["door_region"], door["into"])].access_rule(state))


class BossEntranceShuffleOn(EntranceShuffleOn):
    options = {"shuffle_boss_entrances": True}

    def test_fixed_point_warp_unchanged(self) -> None:
        fixed = set()
        for seed in range(12):
            self.world_setup(seed)
            world = cast(MM2S2HWorld, self.world)
            for door, inside in world.entrance_pairs:
                if door == inside:
                    fixed.add(door)
                    for rr, key in BY_VALUE[door]["boss_warps"]:
                        self.assertEqual(connected(world, rr, key), vanilla_target(rr, key))
        self.assertEqual(fixed, {row["into_value"] for row in BOSSES})


class BothEntranceShufflesOn(EntranceShuffleOn):
    options = {"shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True}


class BossEntranceAreaRules(EntranceShuffleAreaRules):
    options = {"shuffle_boss_entrances": True}
    area_items = BOSS_AREA_ITEMS

    def test_boss_keys_stay_at_source(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        keys = dict(zip(("ODOLWAS_LAIR:0", "GOHTS_LAIR:0", "GYORGS_LAIR:0", "TWINMOLDS_LAIR:0"),
                        (f"RI_{d}_BOSS_KEY" for d in ("WOODFALL", "SNOWHEAD", "GREAT_BAY", "STONE_TOWER"))))
        for row in BOSSES:
            # Twinmold's actual locked door is before the separate hole-to-lair region; it is not shuffled.
            rr, key = (("RR_STONE_TOWER_TEMPLE_INVERTED_SPIKED_BAR_ROOM_LOWER", "STONE_TOWER_TEMPLE_INVERTED:1")
                       if row["into"] == "TWINMOLDS_LAIR:0" else (row["door_region"], row["into"]))
            exit_ = world.exit_entrances[(rr, key)]
            self.assertTrue(exit_.access_rule(self.multiworld.get_all_state()))
            for door, item in keys.items():
                self.assertEqual(exit_.access_rule(self.without(item)), door != row["into"], (row["into"], item))


class BothEntranceAreaRules(BossEntranceAreaRules):
    options = {"shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True}
    area_items = AREA_ITEMS | BOSS_AREA_ITEMS


class BothEntranceNoLogic(EntranceShuffleNoLogic):
    options = {"shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True, "logic": "no_logic"}


class BothEntranceOwnDungeonItems(OwnDungeonDefaultChecks):
    options = OwnDungeonDefaultChecks.options | {"shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True}


class EntranceShuffleLegacySlots(MM2S2HTestBase):
    options = {"shuffle_dungeon_entrances": True}

    def test_old_slot_data(self) -> None:
        original = cast(MM2S2HWorld, self.world)
        for version in ("0.1.5", "0.2.0", "0.3.1", "0.5.0"):
            with self.subTest(version=version):
                data = original.fill_slot_data()
                data["apworld_version"] = version
                data["options"] = dict(data["options"])
                data["options"].pop("RO_SHUFFLE_GROTTOS")
                data["options"].pop("RO_SHUFFLE_INTERIORS")
                if version != "0.5.0":
                    data["options"].pop("RO_SHUFFLE_BOSS_ENTRANCES")
                if version not in ("0.3.1", "0.5.0"):
                    data["options"].pop("RO_SHUFFLE_DUNGEON_ENTRANCES")
                    data.pop("entrance_pairs")
                mw = setup_multiworld(MM2S2HWorld, steps=(), seed=999)
                setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: data})
                for step in gen_steps:
                    call_all(mw, step)
                rebuilt = cast(MM2S2HWorld, mw.worlds[1])
                self.assertFalse(rebuilt.ro["RO_SHUFFLE_GROTTOS"])
                if version != "0.5.0":
                    self.assertFalse(rebuilt.ro["RO_SHUFFLE_BOSS_ENTRANCES"])
                for rr, key in original.exit_entrances:
                    expected = connected(original, rr, key) if version in ("0.3.1", "0.5.0") else vanilla_target(rr, key)
                    self.assertEqual(connected(rebuilt, rr, key), expected, (version, rr, key))
