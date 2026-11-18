"""Shuffle Special Areas (apworld 0.9, RO_SHUFFLE_SPECIAL_AREAS), the 0.9 rows of the older options against real 0.8
slot data, and the inventory of every two-way exit that stays fixed (stage 6b; stage 6c decides from this list)."""
import json
from collections import Counter
from pathlib import Path
from typing import Any, cast

from test.general import gen_steps, setup_multiworld
from worlds.AutoWorld import call_all

from .. import MM2S2HWorld, region_name
from ..enums_generated import ENTRANCE_SHUFFLE
from ..regions_generated import ENTRANCE_AREA_RULES, REGIONS
from . import MM2S2HTestBase
from . import test_dungeon_items as di, test_entrance_shuffle as es
from .test_door_keys import state_without
from .test_entrance_shuffle import connected, vanilla_target
from .test_overworld_shuffle import ALL_OWN as OVERWORLD_ALL_OWN

SPECIAL = [row for row in ENTRANCE_SHUFFLE if row["option"] == "RO_SHUFFLE_SPECIAL_AREAS"]
BY_INTO = {row["into"]: row for row in SPECIAL}
ALL_OWN = OVERWORLD_ALL_OWN | {"shuffle_special_areas": True}
DATA = Path(__file__).parent / "data"
ROWS_08 = 134  # shuffleEntrances rows of apworld 0.8; 0.9 appends 23


def with_pairs(original: MM2S2HWorld, swaps: list[tuple[str, str]]) -> MM2S2HWorld:
    """Universal Tracker rebuild of `original` with the special areas vanilla except that each (a, b) swaps doors a, b."""
    slot_data: dict[str, Any] = original.fill_slot_data()
    behind = {door: inside for door, inside in slot_data["entrance_pairs"]}
    behind.update({row["into_value"]: row["into_value"] for row in SPECIAL})
    for a, b in swaps:
        va, vb = BY_INTO[a]["into_value"], BY_INTO[b]["into_value"]
        behind[va], behind[vb] = vb, va
    slot_data["entrance_pairs"] = [[door, inside] for door, inside in behind.items()]
    return rebuilt_from(slot_data, 4242)


def rebuilt_from(slot_data: dict[str, Any], seed: int) -> MM2S2HWorld:
    mw = setup_multiworld(MM2S2HWorld, steps=(), seed=seed)
    setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: MM2S2HWorld.interpret_slot_data(slot_data)})
    for step in gen_steps:
        call_all(mw, step)
    return cast(MM2S2HWorld, mw.worlds[1])


class SpecialAreaTable(MM2S2HTestBase):
    """Six areas behind one door each, appended after every older row. The generator already proves door and way back,
    base places only, one scene per area and no entrance shared with another option."""

    def test_table(self) -> None:
        self.assertEqual([row["into"] for row in SPECIAL],
                         ["SWAMP_SPIDER_HOUSE:0", "OCEANSIDE_SPIDER_HOUSE:0", "BENEATH_THE_WELL:0", "IKANA_CASTLE:1",
                          "SECRET_SHRINE:0", "DEKU_SHRINE:0"])
        self.assertEqual(ENTRANCE_SHUFFLE[151:157], SPECIAL)
        self.assertEqual(len(ENTRANCE_SHUFFLE), ROWS_08 + 23 + 1)
        # only the Swamp Spider House needs its door's guarantee (South.cpp:479); the others say it themselves
        self.assertEqual({into for into in ENTRANCE_AREA_RULES if into in BY_INTO}, {"SWAMP_SPIDER_HOUSE:0"})
        for row in SPECIAL:
            self.assertIsNone(row["temple_exit"])
            self.assertFalse(row["boss_warps"] or row["grotto"] or row["hours"] or row["water"])


class SpecialAreasOff(MM2S2HTestBase):
    """Option off (default) with every other own option on: no special-area pair, every door as generated."""
    options = OVERWORLD_ALL_OWN

    def test_graph_unchanged(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        self.assertEqual(world.fill_slot_data()["options"]["RO_SHUFFLE_SPECIAL_AREAS"], 0)
        values = {row["into_value"] for row in SPECIAL}
        self.assertFalse(any(door in values for door, _ in world.entrance_pairs))
        for row in SPECIAL:
            self.assertEqual(connected(world, row["door_region"], row["into"]), region_name(row["inside_region"]))
            self.assertEqual(connected(world, row["inside_region"], row["out"]),
                             vanilla_target(row["inside_region"], row["out"]))


class SpecialAreasOn(es.EntranceShuffleOn):
    """Option on: a coupled permutation of the six areas (inherited checks: door A into X, X's way out in front of A),
    all reachable with all items, fill works, Universal Tracker rebuilds the pairing (inherited)."""
    options = {"shuffle_special_areas": True}

    def test_pairs_only_among_special_areas(self) -> None:
        moved = 0
        for seed in range(8):
            self.world_setup(seed)
            world = cast(MM2S2HWorld, self.world)
            self.assertEqual(len(world.entrance_pairs), 6)
            for door_value, inside_value in world.entrance_pairs:
                door, inside = es.BY_VALUE[door_value], es.BY_VALUE[inside_value]
                self.assertEqual((door["option"], inside["option"]), ("RO_SHUFFLE_SPECIAL_AREAS",) * 2)
                moved += door is not inside
        self.assertGreater(moved, 8 * 2)


class SpecialAreaRules(MM2S2HTestBase):
    """A door keeps its own rule; the Swamp Spider House behind a foreign door adds its own door's guarantee."""
    options = {"shuffle_special_areas": True}

    def test_area_rule(self) -> None:
        world = with_pairs(cast(MM2S2HWorld, self.world), [("SWAMP_SPIDER_HOUSE:0", "OCEANSIDE_SPIDER_HOUSE:0")])
        door_of = {row["into"]: world.exit_entrances[(row["door_region"], row["into"])] for row in SPECIAL}
        ocean_door, swamp_door = door_of["OCEANSIDE_SPIDER_HOUSE:0"], door_of["SWAMP_SPIDER_HOUSE:0"]
        self.assertEqual(ocean_door.connected_region.name, region_name("RR_SWAMP_SPIDER_HOUSE_ENTRANCE"))
        self.assertEqual(swamp_door.connected_region.name, region_name("RR_OCEAN_SPIDER_HOUSE_ENTRANCE_UPPER"))
        # Deku Sticks are always at hand in the logic; without the Deku Mask there is neither Deku nor a cleared Woodfall.
        no_deku = state_without(world, es.item_names("RI_MASK_DEKU"))
        self.assertTrue(ocean_door.access_rule(state_without(world, set())))
        self.assertFalse(ocean_door.access_rule(no_deku))  # Great Bay Coast's door (true) + the swamp house's guarantee
        self.assertFalse(swamp_door.access_rule(no_deku))  # its own rule stays at the door (South.cpp:479)
        self.assertTrue(world.exit_entrances[("RR_GREAT_BAY_COAST", "PIRATES_FORTRESS_EXTERIOR:0")].access_rule(no_deku))
        # coupled: each house's way out arrives in front of the other door
        self.assertEqual(connected(world, "RR_SWAMP_SPIDER_HOUSE_ENTRANCE", "SOUTHERN_SWAMP_POISONED:8"),
                         region_name("RR_GREAT_BAY_COAST"))
        self.assertEqual(connected(world, "RR_OCEAN_SPIDER_HOUSE_ENTRANCE_UPPER", "GREAT_BAY_COAST:8"),
                         region_name("RR_SOUTHERN_SWAMP_SOUTH"))


class SpecialAreasNoLogic(es.EntranceShuffleNoLogic):
    options = {"shuffle_special_areas": True, "logic": "no_logic"}


class SpecialAreasWithAllOwnOptions(es.EntranceShuffleOn):
    """Together with temples, bosses, grottos, interiors, overworld, door keys and NPC souls: coupled and beatable."""
    options = ALL_OWN


class SpecialAreasAllNoLogic(es.EntranceShuffleNoLogic):
    options = ALL_OWN | {"logic": "no_logic"}


class SpecialAreasOwnDungeonItems(di.OwnDungeonDefaultChecks):
    options = di.OwnDungeonDefaultChecks.options | ALL_OWN


class Legacy08SlotData(MM2S2HTestBase):
    """Real slot data generated with apworld 0.8.0 (test/data): Universal Tracker rebuilds its pairing exactly, every row
    appended in 0.9 stays vanilla although Shuffle Overworld and Shuffle Interiors are on, Special Areas is off."""

    def check(self, name: str) -> None:
        data = json.loads((DATA / f"slot_data_0_8_0_{name}.json").read_text(encoding="utf-8"))
        self.assertTrue(data["apworld_version"].startswith("0.8"))
        self.assertNotIn("RO_SHUFFLE_SPECIAL_AREAS", data["options"])
        self.assertTrue(data["options"]["RO_SHUFFLE_OVERWORLD"])
        world = rebuilt_from(data, 808)
        self.assertFalse(world.ro["RO_SHUFFLE_SPECIAL_AREAS"])
        by_value = {row["into_value"]: row for row in ENTRANCE_SHUFFLE}
        # The tracker's own list: the 0.8 pairs exactly, and each 0.9 row of an option that is on as itself.
        old = [tuple(p) for p in world.entrance_pairs if ENTRANCE_SHUFFLE.index(by_value[p[0]]) < ROWS_08]
        new = [tuple(p) for p in world.entrance_pairs if ENTRANCE_SHUFFLE.index(by_value[p[0]]) >= ROWS_08]
        self.assertEqual(sorted(old), sorted(map(tuple, data["entrance_pairs"])))
        self.assertTrue(new and all(door == inside for door, inside in new))
        for door_value, inside_value in data["entrance_pairs"]:
            door, inside = by_value[door_value], by_value[inside_value]
            self.assertLess(ENTRANCE_SHUFFLE.index(door), ROWS_08)
            self.assertEqual(connected(world, door["door_region"], door["into"]), region_name(inside["inside_region"]))
        for row in ENTRANCE_SHUFFLE[ROWS_08:]:
            self.assertEqual(connected(world, row["door_region"], row["into"]), region_name(row["inside_region"]),
                             row["into"])
            self.assertEqual(connected(world, row["inside_region"], row["out"]),
                             vanilla_target(row["inside_region"], row["out"]), row["into"])
        self.assertTrue(world.multiworld.can_beat_game(world.multiworld.get_all_state()))

    def test_all_own_options(self) -> None:
        self.check("all_own")

    def test_overworld_only(self) -> None:
        self.check("overworld")


# Every two-way exit of the logic that is in no table (stage 6b inventory, docs/testing.md): region, entrance.
FIXED_TWO_WAY = [
    ("RR_BENEATH_THE_WELL_BACK_EXIT", "IKANA_CASTLE:0"),  # BeneathTheWell.cpp:33
    ("RR_CLOCK_TOWER_INTERIOR", "SOUTH_CLOCK_TOWN:0"),  # Central.cpp:70
    ("RR_CLOCK_TOWN_SOUTH", "CLOCK_TOWER_INTERIOR:1"),  # Central.cpp:229
    ("RR_BENEATH_THE_GRAVEYARD_NIGHT_1_GRAVE", "IKANA_GRAVEYARD:3"),  # East.cpp:52
    ("RR_BENEATH_THE_GRAVEYARD_NIGHT_2_GRAVE_BEFORE_PIT", "IKANA_GRAVEYARD:2"),  # East.cpp:95
    ("RR_IKANA_CANYON_CAVE", "IKANA_CANYON:13"),  # East.cpp:112
    ("RR_IKANA_CANYON_LOWER", "SAKONS_HIDEOUT:0"),  # East.cpp:157
    ("RR_IKANA_CANYON_UPPER", "IKANA_CANYON:14"),  # East.cpp:187
    ("RR_IKANA_GRAVEYARD_LOWER", "BENEATH_THE_GRAVERYARD:0"),  # East.cpp:257
    ("RR_IKANA_GRAVEYARD_LOWER", "BENEATH_THE_GRAVERYARD:1"),  # East.cpp:258
    ("RR_SAKON_HIDEOUT", "IKANA_CANYON:6"),  # East.cpp:372
    ("RR_STONE_TOWER_INVERTED_LOWER", "STONE_TOWER:1"),  # East.cpp:481
    ("RR_STONE_TOWER_TOP", "STONE_TOWER_INVERTED:0"),  # East.cpp:545
    ("RR_IKANA_CASTLE_BEFORE_THRONE", "IGOS_DU_IKANAS_LAIR:0"),  # IkanaCastle.cpp:12
    ("RR_IKANA_CASTLE_COURTYARD", "BENEATH_THE_WELL:1"),  # IkanaCastle.cpp:58
    ("RR_IKANA_CASTLE_COURTYARD", "IKANA_CASTLE:3"),  # IkanaCastle.cpp:59
    ("RR_IKANA_CASTLE_MAIN_ROOM", "IKANA_CASTLE:2"),  # IkanaCastle.cpp:115
    ("RR_IKANA_CASTLE_THRONE_ROOM", "IKANA_CASTLE:6"),  # IkanaCastle.cpp:176
    ("RR_MOON_DEKU_TRIAL", "THE_MOON:0"),  # Moon.cpp:15
    ("RR_MOON_GORON_TRIAL", "THE_MOON:0"),  # Moon.cpp:39
    ("RR_MOON_LINK_TRIAL", "THE_MOON:0"),  # Moon.cpp:62
    ("RR_MOON_ZORA_TRIAL", "THE_MOON:0"),  # Moon.cpp:82
    ("RR_MOON", "MOON_DEKU_TRIAL:0"),  # Moon.cpp:91
    ("RR_MOON", "MOON_GORON_TRIAL:0"),  # Moon.cpp:97
    ("RR_MOON", "MOON_ZORA_TRIAL:0"),  # Moon.cpp:103
    ("RR_MOON", "MOON_LINK_TRIAL:0"),  # Moon.cpp:109
    ("RR_GORON_SHRINE", "GORON_VILLAGE_WINTER:2"),  # North.cpp:38
    ("RR_GORON_VILLAGE", "GORON_SHRINE:0"),  # North.cpp:141
    ("RR_PIRATES_FORTRESS_CAPTAIN_ROOM_UPPER", "PIRATES_FORTRESS:2"),  # PiratesFortress.cpp:18
    ("RR_PIRATES_FORTRESS_CAPTAIN_ROOM", "PIRATES_FORTRESS:1"),  # PiratesFortress.cpp:36
    ("RR_PIRATES_FORTRESS_INSIDE_3_GUARD_ROOM", "PIRATES_FORTRESS:3"),  # PiratesFortress.cpp:47
    ("RR_PIRATES_FORTRESS_INSIDE_CHEST_EGG_ROOM", "PIRATES_FORTRESS:8"),  # PiratesFortress.cpp:65
    ("RR_PIRATES_FORTRESS_INSIDE_GREEN_GUARD", "PIRATES_FORTRESS:5"),  # PiratesFortress.cpp:79
    ("RR_PIRATES_FORTRESS_INSIDE_LINE_GUARD", "PIRATES_FORTRESS:7"),  # PiratesFortress.cpp:88
    ("RR_PIRATES_FORTRESS_INSIDE_MAZE_GUARD", "PIRATES_FORTRESS:5"),  # PiratesFortress.cpp:99
    ("RR_PIRATES_FORTRESS_LEFT_CLAM_EGG_ROOM", "PIRATES_FORTRESS:4"),  # PiratesFortress.cpp:134
    ("RR_PIRATES_FORTRESS_LEFT_PLATFORM", "PIRATES_FORTRESS_INTERIOR:3"),  # PiratesFortress.cpp:146
    ("RR_PIRATES_FORTRESS_MOAT_HIGHER", "PIRATES_FORTRESS:0"),  # PiratesFortress.cpp:154
    ("RR_PIRATES_FORTRESS_MOAT_HIGHER", "PIRATES_FORTRESS_INTERIOR:10"),  # PiratesFortress.cpp:155
    ("RR_PIRATES_FORTRESS_MOAT_LOWER", "PIRATES_FORTRESS_INTERIOR:9"),  # PiratesFortress.cpp:172
    ("RR_PIRATES_FORTRESS_MOAT_PLATFORM", "PIRATES_FORTRESS:12"),  # PiratesFortress.cpp:189
    ("RR_PIRATES_FORTRESS_PLAZA_LEFT_EXIT", "PIRATES_FORTRESS_INTERIOR:7"),  # PiratesFortress.cpp:198
    ("RR_PIRATES_FORTRESS_PLAZA_LEFT_LOWER", "PIRATES_FORTRESS_INTERIOR:7"),  # PiratesFortress.cpp:206
    ("RR_PIRATES_FORTRESS_PLAZA_LEFT_UPPER", "PIRATES_FORTRESS_INTERIOR:2"),  # PiratesFortress.cpp:221
    ("RR_PIRATES_FORTRESS_PLAZA_RIGHT_EXIT", "PIRATES_FORTRESS_INTERIOR:5"),  # PiratesFortress.cpp:234
    ("RR_PIRATES_FORTRESS_PLAZA_RIGHT", "PIRATES_FORTRESS_EXTERIOR:5"),  # PiratesFortress.cpp:245
    ("RR_PIRATES_FORTRESS_PLAZA_RIGHT", "PIRATES_FORTRESS_INTERIOR:4"),  # PiratesFortress.cpp:246
    ("RR_PIRATES_FORTRESS_PLAZA_TOWER", "PIRATES_FORTRESS_INTERIOR:1"),  # PiratesFortress.cpp:258
    ("RR_PIRATES_FORTRESS_PLAZA", "PIRATES_FORTRESS_INTERIOR:0"),  # PiratesFortress.cpp:279
    ("RR_PIRATES_FORTRESS_PLAZA", "PIRATES_FORTRESS_EXTERIOR:1"),  # PiratesFortress.cpp:280
    ("RR_PIRATES_FORTRESS_RIGHT_CLAM_EGG_ROOM", "PIRATES_FORTRESS:6"),  # PiratesFortress.cpp:303
    ("RR_PIRATES_FORTRESS_SEWERS_PREGATE", "PIRATES_FORTRESS_EXTERIOR:2"),  # PiratesFortress.cpp:356
    ("RR_PIRATES_FORTRESS_TELESCOPE_ROOM", "PIRATES_FORTRESS_EXTERIOR:6"),  # PiratesFortress.cpp:378
    ("RR_WOODFALL", "WOODFALL_TEMPLE:2"),  # South.cpp:560
    ("RR_GREAT_BAY_COAST", "PINNACLE_ROCK:0"),  # West.cpp:149
    ("RR_PINNACLE_ROCK_ENTRANCE", "GREAT_BAY_COAST:3"),  # West.cpp:244
    ("RR_WATERFALL_RAPIDS", "ZORA_CAPE:4"),  # West.cpp:283
    ("RR_ZORA_CAPE_BEFORE_GREAT_BAY_TEMPLE", "ZORA_HALL:1"),  # West.cpp:295 (the logic has no way back)
    ("RR_ZORA_CAPE", "WATERFALL_RAPIDS:0"),  # West.cpp:360
    ("RR_WOODFALL_TEMPLE_DEKU_PRINCESS_ROOM", "WOODFALL:3"),  # WoodfallTemple.cpp:90
]
# ONE_WAY_EXIT in the logic: no way back, never a candidate.
ONE_WAY = [
    ("RR_BENEATH_THE_GRAVEYARD_DAMPE", "IKANA_GRAVEYARD:4"),
    ("RR_CLOCK_TOWER_ROOF", "THE_MOON:0"),
    ("RR_CLOCK_TOWN_SOUTH", "CLOCK_TOWER_ROOFTOP:0"),
    ("RR_IKANA_CANYON_LOWER", "SOUTHERN_SWAMP_POISONED:9"),
    ("RR_IKANA_CASTLE_INNER_ROOF", "IKANA_CASTLE:4"),
    ("RR_IKANA_CASTLE_INNER_ROOF", "IKANA_CASTLE:5"),
    ("RR_IKANA_GRAVEYARD_LOWER", "DAMPES_HOUSE:0"),
    ("RR_MOON", "MAJORAS_LAIR:0"),
    ("RR_MOON_DEKU_TRIAL", "THE_MOON:0"),  # the mask child's exit, besides the way back above
    ("RR_MOON_GORON_TRIAL", "THE_MOON:0"),
    ("RR_MOON_LINK_TRIAL", "THE_MOON:0"),
    ("RR_MOON_ZORA_TRIAL", "THE_MOON:0"),
    ("RR_PIRATES_FORTRESS_SEWERS_POSTGATE", "PIRATES_FORTRESS_EXTERIOR:3"),
    ("RR_PIRATES_FORTRESS_SEWERS_PREGATE", "PIRATES_FORTRESS_EXTERIOR:3"),
    ("RR_STONE_TOWER_TEMPLE_INVERTED_SPIKED_BAR_ROOM_LOWER", "STONE_TOWER_TEMPLE_INVERTED:1"),
]


class FixedExitInventory(MM2S2HTestBase):
    """Every exit of the logic is in a table (a door, or the way back out of the area behind it), or on one of the two
    lists above. A new row or a changed region file makes this fail until the inventory is updated."""

    def test_inventory(self) -> None:
        intos = {row["into"] for row in ENTRANCE_SHUFFLE}
        outs = {(row["inside_region"], row["out"]) for row in ENTRANCE_SHUFFLE}
        left = Counter((rr, key) for rr, region in REGIONS.items() if rr != "RR_MAX"
                       for key, _, _ in region[4] if key not in intos and (rr, key) not in outs)
        self.assertEqual(len(FIXED_TWO_WAY), 60)
        self.assertEqual(left, Counter(FIXED_TWO_WAY + ONE_WAY))
