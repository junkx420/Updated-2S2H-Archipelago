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


# Options added after 0.7.0: off, they are only a new key in the slot data, which the 0.7.0 fingerprints cannot have.
NEW_OPTIONS = {"RO_SHUFFLE_OVERWORLD", "RO_SHUFFLE_SPECIAL_AREAS", "RO_MIX_ENTRANCE_POOLS",
               "RO_SHUFFLE_EPONA_RIDING", "RO_SHUFFLE_BEAN_PLANTING"}  # 0.8 through 0.11


def fingerprint(mw: MultiWorld) -> str:
    """Placement of every location, starting items and slot data (entrance pairs etc.) without the world version."""
    world = mw.worlds[1]
    slot = {k: v for k, v in world.fill_slot_data().items() if k != "apworld_version"}
    slot["options"] = {k: v for k, v in slot["options"].items() if not (k in NEW_OPTIONS and v == 0)}
    data = {"placed": sorted((loc.name, loc.item.name) for loc in mw.get_locations(1)),
            "start": sorted(i.name for i in mw.precollected_items[1]),
            "slot": slot}
    return hashlib.sha256(json.dumps(data, sort_keys=True, default=str).encode()).hexdigest()[:16]


def golden_cases() -> dict[int, dict]:
    """Option rolls 0..39 of random_seeds.py (seed = roll number) with all own options."""
    rng = random.Random(2026)
    return {i: roll(rng) | OWN for i in range(40)}


# First computed with 0.7.0 (before trimming existed): every seed that generated before kept exactly this fill up to
# 0.8.0. Recomputed with 0.9.0 and said so: all 40 rolls have Shuffle Interiors on, whose pool gained 9 rows in 0.9
# (Great Fairy fountains, Goron Shop, Goron Graveyard, Deku King's chamber and cell), so every interior pairing and
# hence every fill changed (0 of 38 equal). GOLDEN_WITHOUT_INTERIORS proves nothing else changed.
# Depends on roll() in tools/random_seeds.py; if its option lists change, recompute these.
# A later change that legitimately alters fills (new items, logic) must recompute them too and say so.
GOLDEN = [  # (option roll / seed, fingerprint); rolls 24 and 34 raised OptionError in 0.7.0
    (0, '8793db5584e70b89'),
    (1, '8056cab367cf2e07'),
    (2, '4854b99b5eafb22f'),
    (3, 'cd9f661748454433'),
    (4, 'ad7c658f5faf0cf9'),
    (5, '76a4295200657416'),
    (6, 'c019a5134ce98c64'),
    (7, '8be215d518b3d248'),
    (8, 'a09193be62d87916'),
    (9, 'ed90324ae910339c'),
    (10, '364ec1563ce595de'),
    (11, 'c59830df8c20a225'),
    (12, '3a3a6ccf1b7b0506'),
    (13, '1a0390cdf0e60552'),
    (14, '6336ff6db9c58ffd'),
    (15, '009f12753e0a9738'),
    (16, '2e3bda3018a46607'),
    (17, 'c57907aefd493655'),
    (18, '7e09f4cfcef22044'),
    (19, '72e51c0d27191dd3'),
    (20, '561cc011e1c37c1d'),
    (21, '7c63b805ca16d351'),
    (22, '39f68fee71ae979b'),
    (23, '2cb6ae53a0eea688'),
    (25, '40f030fe08e00944'),
    (26, '0e1eb69dd2f63a02'),
    (27, '0605ed4270a47199'),
    (28, '0d1c661f59788ee8'),
    (29, 'b5338c6beeac4231'),
    (30, 'e315109bc1a98a6f'),
    (31, 'c647a9c6e6fe6786'),
    (32, 'ed595502900472f0'),
    (33, '15aa8ddc09f0c24f'),
    (35, 'd4c8d6e5641d6441'),
    (36, '05814549cf437f4b'),
    (37, '135ad4370f0108f8'),
    (38, '374af20bc783b1a5'),
    (39, '53b6d50d8fed498b'),
]

# The same 40 rolls with Shuffle Interiors off (every other own option on), computed with 0.8.0 before any 0.9
# change: with the options of 0.8 and the new ones off, 0.9 must fill exactly like 0.8.
GOLDEN_WITHOUT_INTERIORS = [
    (0, 'd5685fb95703b0cf'),
    (1, '0f7947c933a1f76c'),
    (2, 'ed0cbdb99978c56d'),
    (3, 'ececb3a6e20142e2'),
    (4, '2c396fa8f69e090b'),
    (5, '9dc32b3dfd37aa8b'),
    (6, 'c5dc5ed107bba2d3'),
    (7, '6eeca526ddeb005e'),
    (8, '2e5eae47f48db3d0'),
    (9, 'dcabeb40cf0ef57b'),
    (10, '7ea24783966f77fa'),
    (11, 'ce54ce12f314ab5e'),
    (12, 'c5ae5749cae49a2b'),
    (13, 'ab75999dbb20ce4b'),
    (14, 'ab142393ea27064b'),
    (15, '13e0f7912609ce61'),
    (16, 'a76fee09c6998309'),
    (17, '2987f65487fdfcf4'),
    (18, '2d88833ab5168080'),
    (19, 'edf2844d689c3381'),
    (20, '91f03762a197b43c'),
    (21, 'f3313b0356645327'),
    (22, '275bd0293e466e1f'),
    (23, '9cda207d198210c9'),
    (24, '3116a567ddba1b5b'),
    (25, 'eb0c384719cc78d4'),
    (26, '4593289f832ac5a5'),
    (27, 'edeef7ee394a276f'),
    (28, 'd9d3595581ffa5b2'),
    (29, 'c5b116ae2038fc76'),
    (30, '03c341f1b286c003'),
    (31, 'eba74672ac483d64'),
    (32, 'f071274e52bebf70'),
    (33, '6ea17136f07bd348'),
    (34, '84b2a1b15ef0bc14'),
    (35, 'a36d63484eb52a1e'),
    (36, 'd8ee5d16b6860e69'),
    (37, '315d1a971785623e'),
    (38, '5a18069887ebae86'),
    (39, '389090adc0eede2b'),
]


class TestUnchangedSeeds(unittest.TestCase):
    def test_golden(self) -> None:
        cases = golden_cases()
        self.assertEqual(len(GOLDEN), 38)
        for seed, expected in GOLDEN:
            with self.subTest(seed=seed):
                self.assertEqual(fingerprint(build(cases[seed], seed)), expected)

    def test_golden_without_interiors(self) -> None:
        cases = golden_cases()
        self.assertEqual(len(GOLDEN_WITHOUT_INTERIORS), 40)
        for seed, expected in GOLDEN_WITHOUT_INTERIORS:
            with self.subTest(seed=seed):
                self.assertEqual(fingerprint(build(cases[seed] | {"shuffle_interiors": False}, seed)), expected)


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
