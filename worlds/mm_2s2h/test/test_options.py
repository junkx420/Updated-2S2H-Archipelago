from .. import MM2S2HWorld
from ..enums_generated import OPTION_DEFAULTS
from ..options import MM2S2HOptions
from . import MM2S2HTestBase


class TestOptionDefaults(MM2S2HTestBase):
    def test_defaults_match_2s2h(self):
        """Every AP option default equals the 2S2H default (StaticData/Options.cpp)."""
        for field, opt_cls in MM2S2HOptions.__annotations__.items():
            ro = getattr(opt_cls, "ro", "")
            if ro:
                with self.subTest(option=field):
                    self.assertEqual(opt_cls.default, OPTION_DEFAULTS[ro], ro)

    def test_slot_data_has_every_ro(self):
        world: MM2S2HWorld = self.multiworld.worlds[self.player]
        self.assertEqual(set(world.fill_slot_data()["options"]), set(OPTION_DEFAULTS))


class TestAllOn(MM2S2HTestBase):
    options = {name: True for name in (
        "shuffle_owl_statues", "shuffle_shops", "shuffle_tingle_shops", "shuffle_boss_remains", "shuffle_cows",
        "shuffle_enemy_drops", "shuffle_frogs", "shuffle_butterflies", "shuffle_freestanding", "shuffle_wonder_items",
        "shuffle_gold_skulltulas", "shuffle_pots", "shuffle_crates", "shuffle_barrels", "shuffle_snowballs",
        "shuffle_grass", "shuffle_trees", "shuffle_hives", "shuffle_boss_souls", "shuffle_enemy_souls",
        "shuffle_swim", "shuffle_ocarina_buttons", "clock_shuffle")}


class TestClockShuffleAscending(MM2S2HTestBase):
    options = {"clock_shuffle": True, "clock_shuffle_mode": "ascending"}
