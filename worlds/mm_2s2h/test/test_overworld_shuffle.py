import unittest
from typing import Any, cast

from test.general import gen_steps, setup_multiworld
from worlds.AutoWorld import call_all

from .. import CLOCK_TOWN_SCENES, MM2S2HWorld, region_name
from ..enums_generated import ENTRANCE_SHUFFLE
from ..regions_generated import ENTRANCE_AREA_RULES, REGIONS
from . import MM2S2HTestBase
from . import test_dungeon_items as di, test_entrance_shuffle as es, test_pool_trim as pool_trim
from .test_door_keys import state_without
from .test_entrance_shuffle import connected
from .test_interior_shuffle import ALL_OWN as INTERIOR_ALL_OWN

OVER = [row for row in ENTRANCE_SHUFFLE if row["option"] == "RO_SHUFFLE_OVERWORLD"]
BY_INTO = {row["into"]: row for row in OVER}
ALL_OWN = INTERIOR_ALL_OWN | {"shuffle_overworld": True}
VARIANTS = ("_SPRING", "_CLEARED", "STONE_TOWER_INVERTED")


def scene(rr: str) -> str:
    return REGIONS[rr][1]


def is_gate(row: dict) -> bool:
    return scene(row["door_region"]) in CLOCK_TOWN_SCENES and scene(row["inside_region"]) not in CLOCK_TOWN_SCENES


def rebuild(original: MM2S2HWorld, pairs: dict[str, str]) -> MM2S2HWorld:
    """Universal Tracker rebuild of `original` with the overworld pairing `pairs` (exit's "into" -> arrival "into")."""
    slot_data: dict[str, Any] = original.fill_slot_data()
    over_values = {row["into_value"] for row in OVER}
    kept = [pair for pair in slot_data["entrance_pairs"] if pair[0] not in over_values]
    slot_data["entrance_pairs"] = kept + [[BY_INTO[a]["into_value"], BY_INTO[b]["into_value"]] for a, b in pairs.items()]
    mw = setup_multiworld(MM2S2HWorld, steps=(), seed=4242)
    setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: MM2S2HWorld.interpret_slot_data(slot_data)})
    for step in gen_steps:
        call_all(mw, step)
    return cast(MM2S2HWorld, mw.worlds[1])


def swapped(a: str, b: str) -> dict[str, str]:
    """Vanilla, but exits a and b trade their arrivals; coupled, so their opposite rows trade theirs back."""
    pairs = {row["into"]: row["into"] for row in OVER}
    pairs[a], pairs[b] = b, a
    pairs[BY_INTO[b]["out"]], pairs[BY_INTO[a]["out"]] = BY_INTO[a]["out"], BY_INTO[b]["out"]
    return pairs


class OverworldTable(MM2S2HTestBase):
    """76 rows, one per exit of 38 transitions: 68 of 0.8, then 8 appended in 0.9 (Astral Observatory, the underwater
    ways into Pirates' Fortress and Zora Hall). The generator already proves that each row's opposite exists, that both
    regions are in the row's scenes and that no other option shares them."""

    def test_table(self) -> None:
        self.assertEqual(len(OVER), 76)
        first = ENTRANCE_SHUFFLE.index(OVER[0])
        self.assertEqual(ENTRANCE_SHUFFLE[first:first + 68], OVER[:68])
        self.assertEqual(first, 66)  # 4 temples, 4 bosses, 26 grottos, 32 interiors: saved row numbers stay valid
        self.assertEqual(ENTRANCE_SHUFFLE[134:142], OVER[68:])  # 0.9, right after the 0.8 rows
        for row in OVER:
            opposite = BY_INTO[row["out"]]
            self.assertEqual((opposite["out"], opposite["door_region"]), (row["into"], row["inside_region"]))
        # the four Clock Town gates, seen from inside the town: they arrive in Termina Field
        # since 0.9 also the Bombers' passage from East Clock Town
        self.assertEqual(sorted(row["into"] for row in OVER if is_gate(row)),
                         ["ASTRAL_OBSERVATORY:0", "TERMINA_FIELD:0", "TERMINA_FIELD:6", "TERMINA_FIELD:7",
                          "TERMINA_FIELD:8"])

    def test_water_rows(self) -> None:
        """Arrivals under water (EntranceShuffle.cpp overworldWaterEntrances): both ways of a transition or neither."""
        water = {row["into"] for row in OVER if row["water"]}
        self.assertEqual(water, {"PIRATES_FORTRESS_EXTERIOR:0", "GREAT_BAY_COAST:5", "ZORA_HALL:0", "ZORA_CAPE:1"})
        for row in OVER:
            self.assertEqual(row["water"], BY_INTO[row["out"]]["water"], row["into"])

    def test_rows_name_places(self) -> None:
        """A row stands for a place, not a scene variant: base entrances and the logic's base scenes only; the game
        picks spring, cleared and the scene layer itself (z_play.c Play_Init)."""
        for row in OVER:
            for entrance in (row["into"], row["out"]):
                self.assertFalse(any(v in entrance for v in VARIANTS), entrance)
            for rr in (row["door_region"], row["inside_region"]):
                self.assertFalse(scene(rr).endswith("2") or scene(rr) in ("SCENE_F41", "SCENE_INISIE_R"), rr)
        # every place with a variant is in the pool once per spawn, by its base name
        places = {row["into"] for row in OVER}
        for entrance in ("SOUTHERN_SWAMP_POISONED:0", "MOUNTAIN_VILLAGE_WINTER:6", "GORON_VILLAGE_WINTER:0",
                         "PATH_TO_GORON_VILLAGE_WINTER:0", "IKANA_CANYON:0", "GREAT_BAY_COAST:0", "ZORA_CAPE:0",
                         "WOODFALL:0", "SNOWHEAD:0"):
            self.assertIn(entrance, places)

    def test_fixed_ways_stay_out(self) -> None:
        """One-way, inversion and cutscene ways are not in the pool, nor the water ways a Human can take (Pinnacle
        Rock, Waterfall Rapids) and Zora Hall's door by land (the logic has no way back)."""
        intos = {row["into"] for row in OVER} | {row["out"] for row in OVER}
        for entrance in ("CLOCK_TOWER_ROOFTOP:0", "SOUTHERN_SWAMP_POISONED:9", "STONE_TOWER_INVERTED:0", "STONE_TOWER:1",
                         "PINNACLE_ROCK:0", "GREAT_BAY_COAST:3", "WATERFALL_RAPIDS:0", "ZORA_CAPE:4", "ZORA_HALL:1",
                         "IKANA_CANYON:14", "SOUTH_CLOCK_TOWN:0", "CLOCK_TOWER_INTERIOR:1"):
            self.assertNotIn(entrance, intos)

    def test_area_rules(self) -> None:
        self.assertEqual({into for into in ENTRANCE_AREA_RULES if into in BY_INTO},
                         {"WOODFALL:0", "DEKU_PALACE:5", "GORON_RACETRACK:0", "PATH_TO_GORON_VILLAGE_WINTER:2",
                          "MILK_ROAD:1", "ASTRAL_OBSERVATORY:0", "PIRATES_FORTRESS_EXTERIOR:0", "ZORA_HALL:0"})


class OverworldShuffleOff(MM2S2HTestBase):
    """Option off (default) with every other own option on: no overworld pair, every overworld exit as generated."""
    options = INTERIOR_ALL_OWN

    def test_graph_unchanged(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        self.assertEqual(world.fill_slot_data()["options"]["RO_SHUFFLE_OVERWORLD"], 0)
        over_values = {row["into_value"] for row in OVER}
        self.assertFalse(any(door in over_values for door, _ in world.entrance_pairs))
        for row in OVER:
            self.assertEqual(connected(world, row["door_region"], row["into"]), region_name(row["inside_region"]))


class OverworldShuffleOn(es.EntranceShuffleOn):
    """Option on: coupled (inherited check: exit A into B's place, B's way back to A), all reachable with all items,
    fill works, Universal Tracker rebuilds the pairing (inherited)."""
    options = {"shuffle_overworld": True}

    def test_gates_leave_clock_town(self) -> None:
        moved = water_moved = 0
        for seed in range(10):
            self.world_setup(seed)
            world = cast(MM2S2HWorld, self.world)
            pairs = {door: inside for door, inside in world.entrance_pairs}
            self.assertEqual(len(pairs), 76)
            for row in OVER:
                target = es.BY_VALUE[pairs[row["into_value"]]]
                self.assertEqual(target["option"], "RO_SHUFFLE_OVERWORLD")
                self.assertEqual(target["water"], row["water"], (seed, row["into"]))  # water only with water
                # a coast's water exit leads into a dead end (moat, Zora Hall), a dead end's back to a coast
                dead_ends = ("PIRATES_FORTRESS_EXTERIOR:0", "ZORA_HALL:0")
                if row["water"]:
                    self.assertEqual(row["into"] in dead_ends, target["into"] in dead_ends, (seed, row["into"]))
                water_moved += row["water"] and target is not row
                # coupling: the opposite exit of the arrival leads back to this exit's place
                back = pairs[es.BY_VALUE[BY_INTO[target["out"]]["into_value"]]["into_value"]]
                self.assertEqual(back, BY_INTO[row["out"]]["into_value"], row["into"])
                if is_gate(row):
                    self.assertNotIn(scene(target["inside_region"]), CLOCK_TOWN_SCENES, (seed, row["into"]))
                moved += target is not row
        self.assertGreater(moved, 10 * 30)
        self.assertGreater(water_moved, 0)


class OverworldAreaRules(MM2S2HTestBase):
    """An exit keeps its own rule; a foreign arrival adds its area rule. Forced pairings via the Universal Tracker
    path: Road to Southern Swamp -> Termina Field now arrives in Woodfall, Mountain Village -> Path to Goron Village
    at the Goron Racetrack."""
    options = {"shuffle_overworld": True}

    def test_area_rules(self) -> None:
        pairs = swapped("TERMINA_FIELD:1", "WOODFALL:0")
        racetrack = swapped("PATH_TO_GORON_VILLAGE_WINTER:0", "GORON_RACETRACK:0")
        pairs.update({k: v for k, v in racetrack.items() if v != k})
        world = rebuild(cast(MM2S2HWorld, self.world), pairs)
        exit_of = {row["into"]: world.exit_entrances[(row["door_region"], row["into"])] for row in OVER}
        self.assertEqual(exit_of["TERMINA_FIELD:1"].connected_region.name, region_name("RR_WOODFALL"))
        self.assertEqual(exit_of["WOODFALL:0"].connected_region.name, region_name("RR_TERMINA_FIELD"))
        self.assertEqual(exit_of["PATH_TO_GORON_VILLAGE_WINTER:0"].connected_region.name,
                         region_name("RR_GORON_RACETRACK"))
        full = state_without(world, set())
        deku = state_without(world, es.item_names("RI_MASK_DEKU"))
        keg = state_without(world, es.item_names("RI_POWDER_KEG"))
        for into in ("TERMINA_FIELD:1", "WOODFALL:0", "PATH_TO_GORON_VILLAGE_WINTER:0", "GORON_RACETRACK:0"):
            self.assertTrue(exit_of[into].access_rule(full), into)
        self.assertFalse(exit_of["TERMINA_FIELD:1"].access_rule(deku))  # area rule of Woodfall
        self.assertFalse(exit_of["WOODFALL:0"].access_rule(deku))       # its own rule stays (South.cpp:496)
        self.assertTrue(exit_of["TERMINA_FIELD:1"].access_rule(keg))
        self.assertFalse(exit_of["PATH_TO_GORON_VILLAGE_WINTER:0"].access_rule(keg))  # area rule of the racetrack
        # coupled: Path to Goron Village -> Mountain Village now arrives in front of the racetrack's boulder
        self.assertFalse(exit_of["MOUNTAIN_VILLAGE_WINTER:2"].access_rule(keg))
        self.assertEqual(exit_of["MOUNTAIN_VILLAGE_WINTER:2"].connected_region.name, region_name("RR_PATH_TO_GORON_VILLAGE"))
        # and the racetrack's way out arrives in Mountain Village, which has no area rule
        self.assertTrue(world.exit_entrances[("RR_GORON_RACETRACK", "PATH_TO_GORON_VILLAGE_WINTER:2")].access_rule(keg))

    def test_observatory_area_rule(self) -> None:
        """Road to Southern Swamp -> Termina Field now arrives in the Bombers' passage: a projectile, for the balloon."""
        world = rebuild(cast(MM2S2HWorld, self.world), swapped("TERMINA_FIELD:1", "ASTRAL_OBSERVATORY:0"))
        exit_of = {row["into"]: world.exit_entrances[(row["door_region"], row["into"])] for row in OVER}
        self.assertEqual(exit_of["TERMINA_FIELD:1"].connected_region.name, region_name("RR_ASTRAL_OBSERVATORY_PASSAGE"))
        no_projectile = state_without(world, set().union(*(es.item_names(i) for i in
                                                            ("RI_BOW", "RI_HOOKSHOT", "RI_MASK_DEKU", "RI_MASK_ZORA"))))
        self.assertTrue(exit_of["TERMINA_FIELD:1"].access_rule(state_without(world, set())))
        self.assertFalse(exit_of["TERMINA_FIELD:1"].access_rule(no_projectile))
        # the passage's own way out keeps its rule (true) and now arrives on the Road to Southern Swamp
        self.assertTrue(exit_of["EAST_CLOCK_TOWN:2"].access_rule(no_projectile))
        self.assertEqual(exit_of["EAST_CLOCK_TOWN:2"].connected_region.name, region_name("RR_ROAD_TO_SOUTHERN_SWAMP"))

    def test_water_area_rules(self) -> None:
        """Zora Cape's tunnel now arrives in the Pirates' Fortress moat, Great Bay Coast's in Zora Hall: Zora either way."""
        world = rebuild(cast(MM2S2HWorld, self.world), swapped("ZORA_HALL:0", "PIRATES_FORTRESS_EXTERIOR:0"))
        exit_of = {row["into"]: world.exit_entrances[(row["door_region"], row["into"])] for row in OVER}
        self.assertEqual(exit_of["ZORA_HALL:0"].connected_region.name, region_name("RR_PIRATES_FORTRESS_MOAT_LOWER"))
        self.assertEqual(exit_of["PIRATES_FORTRESS_EXTERIOR:0"].connected_region.name, region_name("RR_ZORA_HALL"))
        no_zora = state_without(world, es.item_names("RI_MASK_ZORA"))
        for into in ("ZORA_HALL:0", "PIRATES_FORTRESS_EXTERIOR:0", "ZORA_CAPE:1", "GREAT_BAY_COAST:5"):
            self.assertTrue(exit_of[into].access_rule(state_without(world, set())), into)
        for into in ("ZORA_HALL:0", "PIRATES_FORTRESS_EXTERIOR:0"):
            self.assertFalse(exit_of[into].access_rule(no_zora), into)
        # Zora Hall's tunnel back (rule true) now arrives at Great Bay Coast, which has land ways in: no area rule
        self.assertTrue(exit_of["ZORA_CAPE:1"].access_rule(no_zora))


class OverworldShuffleNoLogic(es.EntranceShuffleNoLogic):
    options = {"shuffle_overworld": True, "logic": "no_logic"}


class OverworldShuffleWithAllOwnOptions(es.EntranceShuffleOn):
    """Together with temples, bosses, grottos, interiors, door keys and NPC souls: still coupled and beatable."""
    options = ALL_OWN


class OverworldShuffleAllNoLogic(es.EntranceShuffleNoLogic):
    options = ALL_OWN | {"logic": "no_logic"}


class OverworldShuffleOwnDungeonItems(di.OwnDungeonDefaultChecks):
    options = di.OwnDungeonDefaultChecks.options | ALL_OWN


class OverworldLegacySlots(MM2S2HTestBase):
    """Slot data of 0.7 (no RO_SHUFFLE_OVERWORLD): rebuilt with every overworld exit as generated."""
    options = INTERIOR_ALL_OWN

    def test_old_slot_data(self) -> None:
        original = cast(MM2S2HWorld, self.world)
        data = original.fill_slot_data()
        data["apworld_version"] = "0.7.1"
        data["options"] = dict(data["options"])
        data["options"].pop("RO_SHUFFLE_OVERWORLD")
        mw = setup_multiworld(MM2S2HWorld, steps=(), seed=999)
        setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: data})
        for step in gen_steps:
            call_all(mw, step)
        rebuilt = cast(MM2S2HWorld, mw.worlds[1])
        self.assertFalse(rebuilt.ro["RO_SHUFFLE_OVERWORLD"])
        self.assertEqual(sorted(original.entrance_pairs), sorted(rebuilt.entrance_pairs))
        for rr, key in original.exit_entrances:
            self.assertEqual(connected(rebuilt, rr, key), connected(original, rr, key), (rr, key))
        self.assertTrue(mw.can_beat_game(mw.get_all_state()))


class OverworldRegressionSeeds(unittest.TestCase):
    """Seeds that failed while stage 6b was tested, built like Generate.py (test_pool_trim.build)."""

    def test_blue_warp_island(self) -> None:
        """Overworld batch seed 8: with Great Bay Temple behind Stone Tower's door, GER once counted Zora Cape as reached
        through Gyorg's vanilla warp, which then leads to Stone Tower: Zora Cape and Laundry Pool were an island."""
        options = {  # tools/random_seeds.py roll 8 with shuffle_overworld=true, as in its YAML
            "clock_shuffle": True, "shuffle_owl_statues": False, "shuffle_shops": True,
            "shuffle_tingle_shops": False, "shuffle_boss_remains": True, "shuffle_cows": False,
            "shuffle_enemy_drops": False, "shuffle_frogs": False, "shuffle_butterflies": True,
            "shuffle_freestanding": True, "shuffle_wonder_items": False, "shuffle_gold_skulltulas": False,
            "shuffle_pots": True, "shuffle_crates": True, "shuffle_barrels": False, "shuffle_snowballs": True,
            "shuffle_grass": False, "shuffle_trees": False, "shuffle_hives": True, "shuffle_boss_souls": False,
            "shuffle_enemy_souls": True, "shuffle_npc_souls": False, "shuffle_swim": False,
            "shuffle_ocarina_buttons": True, "shuffle_song_sun": True, "shuffle_song_double_time": False,
            "shuffle_song_inverted_time": True, "shuffle_song_saria": True, "shuffle_tycoon_wallet": False,
            "shuffle_skeleton_key": False, "shuffle_dungeon_entrances": True, "shuffle_boss_entrances": False,
            "shuffle_door_keys": False, "shuffle_grottos": False, "shuffle_interiors": False,
            "shuffle_traps": False, "plentiful_items": False, "triforce_hunt": False, "starting_consumables": True,
            "starting_rupees": True, "starting_maps_and_compasses": True, "death_link": False,
            "access_dungeons": "form_or_song", "access_trials": "20_masks", "clock_shuffle_mode": "ascending",
            "small_keys": "start_with", "boss_keys": "start_with", "stray_fairies": "own_dungeon",
            "majora_remains_required": 1, "majora_masks_required": 10, "moon_remains_required": 4,
            "moon_masks_required": 20, "skulltulas_shuffled": 20, "skulltula_tokens_required": 14,
            "trap_amount": 27, "triforce_pieces_max": 54, "triforce_pieces_required": 52, "stray_fairies_max": 8,
            "stray_fairies_required": 14, "starting_health": 16, "shuffle_overworld": True}
        mw = pool_trim.build(options, 8)
        self.assertTrue(mw.fulfills_accessibility())

    def test_water_island(self) -> None:
        """Own-dungeon items, seed 2021: the crossed water pairing once cut Zora Cape off (pre_fill failed)."""
        options = pool_trim.OWN | {"shuffle_overworld": True, "shuffle_special_areas": True, "small_keys": "own_dungeon",
                                   "boss_keys": "own_dungeon", "stray_fairies": "own_dungeon"}
        mw = pool_trim.build(options, 2021, fill=False)
        state = mw.get_all_state(False)
        self.assertEqual([r.name for r in mw.get_regions(1) if not r.can_reach(state)], [])
