from typing import cast

from BaseClasses import CollectionState

from test.general import gen_steps, setup_multiworld
from worlds.AutoWorld import call_all

from .. import MM2S2HWorld, region_name
from ..enums_generated import ENTRANCE_SHUFFLE
from ..regions_generated import REGIONS
from . import MM2S2HTestBase
from . import test_dungeon_items as di, test_entrance_shuffle as es
from .test_entrance_shuffle import BY_VALUE, connected, vanilla_target

GROTTOS = [row for row in ENTRANCE_SHUFFLE if row["grotto"]]
ALL_OWN = {"shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True, "shuffle_grottos": True,
           "shuffle_door_keys": True, "shuffle_npc_souls": True}


class GrottoTable(MM2S2HTestBase):
    """The 26 grottos of the logic, one row each, and their real entrance/data pairs."""

    def test_table(self) -> None:
        self.assertEqual(len(GROTTOS), 26)
        holes = {key for reg in REGIONS.values() for key, _, _ in reg[4] if key.startswith("GROTTOS:")}
        self.assertEqual({row["into"] for row in GROTTOS}, holes)
        pairs = [(row["grotto"]["game_entrance"], row["grotto"]["data"]) for row in GROTTOS]
        shared = [p for p in pairs if p[0] in ("GROTTOS:4", "GROTTOS:10")]
        self.assertEqual(len(shared), 15)
        self.assertEqual(len(set(shared)), 15, "grottos sharing an entrance must differ in their data")
        # Logic.cpp: virtual entrances 17..31 never reach the game, they are the shared ones.
        for row in GROTTOS:
            spawn = int(row["into"].split(":")[1])
            self.assertEqual(spawn >= 17, row["grotto"]["game_entrance"] != row["into"], row["into"])
        self.assertEqual({row["into"] for row in GROTTOS if row["grotto"]["kind"] == "SCENE_EXIT"},
                         {"GROTTOS:12", "GROTTOS:16"})


class GrottoShuffleOff(MM2S2HTestBase):
    """Option off: every hole and way back exactly as the 2S2H logic, including grotto 27's way back."""

    def test_graph_unchanged(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        self.assertEqual(world.fill_slot_data()["options"]["RO_SHUFFLE_GROTTOS"], 0)
        self.assertEqual(world.entrance_pairs, [])
        for row in GROTTOS:
            self.assertEqual(connected(world, row["door_region"], row["into"]), region_name(row["inside_region"]))
            self.assertEqual(connected(world, row["inside_region"], row["out"]),
                             vanilla_target(row["inside_region"], row["out"]))
        cow = next(row for row in GROTTOS if row["into"] == "GROTTOS:27")
        self.assertEqual(connected(world, cow["inside_region"], cow["out"]), region_name("RR_GREAT_BAY_COAST"))


class GrottoShuffleOn(es.EntranceShuffleOn):
    """Option on: a permutation of the 26 grottos, coupled (a grotto returns to the region of the hole leading to it),
    not mixed with other rows; Universal Tracker rebuilds it (inherited). Base tests: everything reachable, fill."""
    options = {"shuffle_grottos": True}

    def test_hole_rules_stay_at_hole(self) -> None:
        on = cast(MM2S2HWorld, self.world)
        off_mw = setup_multiworld(MM2S2HWorld, steps=(), seed=self.multiworld.seed)
        for step in gen_steps:
            call_all(off_mw, step)
        off = cast(MM2S2HWorld, off_mw.worlds[1])
        states = {"none": (CollectionState(self.multiworld), CollectionState(off_mw)),
                  "all": (self.multiworld.get_all_state(), off_mw.get_all_state())}
        moved = 0
        for door_value, inside_value in on.entrance_pairs:
            door = BY_VALUE[door_value]
            moved += door_value != inside_value
            for name, (s_on, s_off) in states.items():
                self.assertEqual(on.exit_entrances[(door["door_region"], door["into"])].access_rule(s_on),
                                 off.exit_entrances[(door["door_region"], door["into"])].access_rule(s_off),
                                 (door["into"], name))
        self.assertGreater(moved, 0)

    def test_only_grottos_paired(self) -> None:
        world = cast(MM2S2HWorld, self.world)
        self.assertEqual(len(world.entrance_pairs), 26)
        for door_value, inside_value in world.entrance_pairs:
            self.assertTrue(BY_VALUE[door_value]["grotto"] and BY_VALUE[inside_value]["grotto"])


class GrottoShuffleNoLogic(es.EntranceShuffleNoLogic):
    options = {"shuffle_grottos": True, "logic": "no_logic"}


class GrottoShuffleWithAllOwnOptions(es.EntranceShuffleOn):
    """Together with temples, bosses, door keys and NPC souls: every group shuffled on its own, still beatable."""
    options = ALL_OWN


class GrottoShuffleAllNoLogic(es.EntranceShuffleNoLogic):
    options = ALL_OWN | {"logic": "no_logic"}


class GrottoShuffleOwnDungeonItems(di.OwnDungeonDefaultChecks):
    options = di.OwnDungeonDefaultChecks.options | ALL_OWN
