"""Pool larger than the locations (0.7.1): create_items trims junk, then traps, then the extra Plentiful copies,
and only then raises OptionError. Seeds that already generated before must not change."""
import hashlib
import json
import random
import unittest
from argparse import Namespace
from collections import Counter

from BaseClasses import CollectionState, MultiWorld
from Fill import distribute_items_restrictive
from Generate import get_seed_name
from Options import OptionError
from test.general import gen_steps
from worlds import AutoWorld
from worlds.AutoWorld import call_all

from ..tools.random_seeds import roll
from ..items_generated import ITEMS


GAME = "Majora's Mask (2 Ship 2 Harkinian)"
OWN = {"shuffle_interiors": True, "shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True,
       "shuffle_grottos": True, "shuffle_door_keys": True, "shuffle_npc_souls": True}

# Default checks with Plentiful Items, 30 traps, NPC souls and door keys: 442 items for 284 locations.
CROWDED = {"plentiful_items": True, "shuffle_traps": True, "trap_amount": 30, "shuffle_npc_souls": True,
           "shuffle_door_keys": True}
# Too full even without Plentiful and traps: 339 items for 284 locations.
OVERFULL = {"shuffle_npc_souls": True, "shuffle_door_keys": True, "shuffle_boss_souls": True,
            "shuffle_enemy_souls": True, "clock_shuffle": True, "shuffle_ocarina_buttons": True,
            "starting_health": 1, "shuffle_traps": True, "trap_amount": 30}


def build(options: dict, seed: int, fill: bool = True) -> MultiWorld:
    mw = MultiWorld(1)
    mw.game[1] = GAME
    mw.player_name = {1: "Tester"}
    mw.set_seed(seed)
    random.seed(mw.seed)
    mw.seed_name = get_seed_name(random)
    args = Namespace()
    for name, option in AutoWorld.AutoWorldRegister.world_types[GAME].options_dataclass.type_hints.items():
        setattr(args, name, {1: option.from_any(options.get(name, option.default))})
    mw.set_options(args)
    mw.state = CollectionState(mw)
    for step in gen_steps:
        call_all(mw, step)
    if fill:
        distribute_items_restrictive(mw)
    return mw


def fingerprint(mw: MultiWorld) -> str:
    """Placement of every location, starting items and slot data (entrance pairs etc.) without the world version."""
    world = mw.worlds[1]
    data = {"placed": sorted((loc.name, loc.item.name) for loc in mw.get_locations(1)),
            "start": sorted(i.name for i in mw.precollected_items[1]),
            "slot": {k: v for k, v in world.fill_slot_data().items() if k != "apworld_version"}}
    return hashlib.sha256(json.dumps(data, sort_keys=True, default=str).encode()).hexdigest()[:16]


def golden_cases() -> dict[int, dict]:
    """Option rolls 0..39 of random_seeds.py (seed = roll number) with all own options."""
    rng = random.Random(2026)
    return {i: roll(rng) | OWN for i in range(40)}


# Computed with 0.7.0 (before trimming existed): every seed that generated before must keep exactly this fill.
# Depends on roll() in tools/random_seeds.py; if its option lists change, recompute these with the 0.7.0 code.
# A later change that legitimately alters fills (new items, logic) must recompute them too and say so.
GOLDEN = [  # (option roll / seed, fingerprint); rolls 24 and 34 raised OptionError
    (0, '53a87237acf029d9'),
    (1, '1f1374a414cc9aff'),
    (2, 'a3d20427c0307edd'),
    (3, '058194135988badb'),
    (4, '0f2d83128a388096'),
    (5, 'fefb55289ab2fa85'),
    (6, '765d92e941ce3c30'),
    (7, 'a9b99f60deb8b2f4'),
    (8, '5b2102376d08b3c8'),
    (9, '53ddbeaed1a842d2'),
    (10, '6a729fbe9d196635'),
    (11, '2efce5933c24708a'),
    (12, '235e1d7ddec3342c'),
    (13, '805d774739f7fa21'),
    (14, '4275120d5f06fffe'),
    (15, 'e4d8d513cb21ba1d'),
    (16, '32b19faa78afbabe'),
    (17, '90331bf562b0ddb8'),
    (18, 'e5333114ca33e1e8'),
    (19, 'a7176cd297fa95d1'),
    (20, '9507b74606e403e5'),
    (21, '7fdc0bfa47e92a92'),
    (22, '4fe08bc2d03f76ff'),
    (23, '3a78ed3f9829b789'),
    (25, '4f0d85a773030d21'),
    (26, '06167e8a9182c064'),
    (27, 'e7ee8a1eb3880163'),
    (28, 'c6a7cdbedbc19c43'),
    (29, 'f0850113f916c7b5'),
    (30, '1eaa630706add02d'),
    (31, '3f5a6f0344bcf8b7'),
    (32, 'bd3f12c9764707af'),
    (33, '324ef12de2481132'),
    (35, '94df9bae1d153d2b'),
    (36, 'd0cf71220b3d1b44'),
    (37, 'b618a2ca831c385f'),
    (38, 'c8a26b1f10a2d486'),
    (39, '5f5548a6238afe4b'),
]


class TestUnchangedSeeds(unittest.TestCase):
    def test_golden(self) -> None:
        cases = golden_cases()
        self.assertEqual(len(GOLDEN), 38)
        for seed, expected in GOLDEN:
            with self.subTest(seed=seed):
                self.assertEqual(fingerprint(build(cases[seed], seed)), expected)


class TestTrim(unittest.TestCase):
    def test_crowded_pool_generates(self) -> None:
        with self.assertLogs(level="WARNING") as logs:
            mw = build(CROWDED, 1, fill=False)
        world = mw.worlds[1]
        pool = Counter(i.name for i in mw.itempool if i.player == 1)
        self.assertEqual(sum(pool.values()), len(world.shuffled))  # nothing confined to dungeons here
        self.assertEqual(pool["Knockoff Item"], 0, "traps are trimmed before Plentiful copies")
        self.assertEqual(len(logs.output), 2, logs.output)
        self.assertIn("removed 30 of 30 traps", logs.output[0])
        self.assertIn("removed 128 of 143 extra Plentiful copies", logs.output[1])
        # every original stays: the pool without Plentiful and traps (minus its junk) is contained in the trimmed one
        base = build(CROWDED | {"plentiful_items": False, "shuffle_traps": False}, 1, fill=False)
        junk = {v[0] for v in ITEMS.values() if v[1] == "RITYPE_JUNK"}
        base_pool = Counter(i.name for i in base.itempool if i.player == 1 and i.name not in junk)
        self.assertFalse(base_pool - pool, "an original item was trimmed")
        self.assertEqual(sum((pool - base_pool).values()), 143 - 128, "exactly the kept copies are extra")
        distribute_items_restrictive(mw)
        self.assertTrue(mw.can_beat_game())

    def test_only_traps_trimmed(self) -> None:
        """Without Plentiful, traps alone are trimmed (15 of 30 here)."""
        with self.assertLogs(level="WARNING") as logs:
            mw = build(CROWDED | {"plentiful_items": False}, 1, fill=False)
        self.assertEqual(len(logs.output), 1)
        self.assertIn("removed 15 of 30 traps", logs.output[0])
        self.assertEqual(Counter(i.name for i in mw.itempool)["Knockoff Item"], 15)

    def test_without_plentiful_still_too_full(self) -> None:
        with self.assertRaises(OptionError) as err:
            build(OVERFULL, 1, fill=False)
        self.assertIn("339 items for 284 locations", str(err.exception))
        self.assertIn("even without junk, traps", str(err.exception))
        self.assertIn("Enable more check types", str(err.exception))


if __name__ == "__main__":
    unittest.main()
