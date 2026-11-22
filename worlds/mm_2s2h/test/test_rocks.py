from collections import Counter
from itertools import product

from BaseClasses import CollectionState

from ..enums_generated import ENTRANCE_MIX_POOLS, RI_ID
from ..locations_generated import LOCATIONS
from ..primitives import CLOCKS, SaveView
from ..regions_generated import EXTRA_CHECK_WINDOWS
from . import MM2S2HTestBase


ROCKS = {rc: data[0] for rc, data in LOCATIONS.items() if data[1] == "RCTYPE_ROCK"}


class RocksOff(MM2S2HTestBase):
    def test_default_pool_has_no_rocks(self) -> None:
        self.assertEqual(self.world.ro["RO_SHUFFLE_ROCKS"], 0)
        self.assertFalse(set(ROCKS.values()) & {loc.name for loc in self.multiworld.get_locations(self.player)})


class RocksOn(MM2S2HTestBase):
    options = {"shuffle_rocks": True}

    def test_every_rock_is_a_named_reachable_location(self) -> None:
        self.assertEqual(len(ROCKS), 122)
        self.assertEqual(self.world.location_name_groups["Rocks"], set(ROCKS.values()))
        self.assertTrue(all(": Rock " in name for name in ROCKS.values()))
        state = self.multiworld.get_all_state()
        self.assertEqual([name for name in ROCKS.values() if not state.can_reach_location(name, self.player)], [])

    def test_lift_bronze_and_bomb_damage(self) -> None:
        self.assertTrue(self.location_rule("Goron Shrine: Rock 1"))
        self.assertFalse(self.location_rule("Deku Palace: Rock 1", "RI_PROGRESSIVE_BOMB_BAG"))
        self.assertTrue(self.location_rule("Deku Palace: Rock 1", "RI_MASK_GORON"))
        self.assertTrue(self.location_rule("Termina Field: Rock Cow Grotto", "RI_MASK_GORON"))
        self.assertFalse(self.location_rule("Lone Peak Shrine: Rock 1", "RI_MASK_GORON"))
        self.assertTrue(self.location_rule("Lone Peak Shrine: Rock 1", "RI_PROGRESSIVE_BOMB_BAG"))
        self.assertTrue(self.location_rule("Lone Peak Shrine: Rock 1", "RI_MASK_GORON", "RI_POWDER_KEG"))

    def location_rule(self, name: str, *items: str) -> bool:
        state = CollectionState(self.multiworld)
        state.mm2s2h_views[self.player] = SaveView(Counter(items), Counter(), self.world)
        return self.multiworld.get_location(name, self.player).access_rule(state)


class RocksAvailability(MM2S2HTestBase):
    options = {"shuffle_rocks": True, "clock_shuffle": True, "clock_shuffle_mode": "any_order"}

    def test_every_rock_half_day_and_layout(self) -> None:
        # N64-US ActorLists: two 0x2AA roll paths, ten spring Mountain Village rocks, three spring
        # Path to Goron Village rocks, and five Zora Cape placements only in the base header.
        day_only = {f"RC_ROAD_TO_IKANA_ROCK_ROLLING_{i:02}" for i in (1, 2)}
        spring = {rc for rc in ROCKS if LOCATIONS[rc][2] in {"SCENE_10YUKIYAMANOMURA2", "SCENE_17SETUGEN2"}}
        great_bay_base = {f"RC_ZORA_CAPE_ROCK_{i:02}" for i in range(13, 18)}
        self.assertEqual(len(day_only), 2)
        self.assertEqual(len(spring), 13)
        self.assertLessEqual(day_only | great_bay_base, set(ROCKS))
        self.assertLessEqual(set(ROCKS), set(EXTRA_CHECK_WINDOWS))
        state = CollectionState(self.multiworld)
        available_items = Counter({ri: 1 for ri in RI_ID if not ri.startswith("RI_TIME_")})
        checks = {rc: self.multiworld.get_location(name, self.player) for rc, name in ROCKS.items()}

        for half_day, snowhead_cleared, great_bay_cleared in product(range(6), (False, True), (False, True)):
            events = Counter(RE_CLEARED_SNOWHEAD_TEMPLE=int(snowhead_cleared),
                             RE_CLEARED_GREAT_BAY_TEMPLE=int(great_bay_cleared))
            view = SaveView(available_items | Counter({CLOCKS[half_day]: 1}), events, self.world)
            state.mm2s2h_views[self.player] = view
            for rc, loc in checks.items():
                with self.subTest(check=rc, half_day=half_day, spring=snowhead_cleared,
                                  great_bay_cleared=great_bay_cleared):
                    in_window = (rc not in day_only or half_day % 2 == 0) and (rc not in spring or snowhead_cleared)
                    half_days, layout = EXTRA_CHECK_WINDOWS[rc]
                    self.assertEqual(
                        view.time.extra_check_available(half_days, layout, snowhead_cleared=snowhead_cleared,
                                                        great_bay_cleared=great_bay_cleared),
                        in_window and (rc not in great_bay_base or not great_bay_cleared),
                    )
                    # AP also considers a fresh cycle after Gyorg.
                    self.assertEqual(loc.access_rule(state), in_window)


class RocksNoLogic(MM2S2HTestBase):
    options = {"shuffle_rocks": True, "logic": "no_logic"}

    def test_every_rock_is_reachable_without_items(self) -> None:
        state = CollectionState(self.multiworld)
        self.assertEqual([name for name in ROCKS.values() if not state.can_reach_location(name, self.player)], [])


class RocksAllOwnOptions(RocksOn):
    options = RocksOn.options | {
        "shuffle_npc_souls": "all", "shuffle_interiors": True, "shuffle_dungeon_entrances": True,
        "shuffle_boss_entrances": True, "shuffle_grottos": True, "shuffle_overworld": True,
        "shuffle_special_areas": True, "shuffle_door_keys": True, "shuffle_epona_riding": True,
        "shuffle_bean_planting": True, "shuffle_swim": True, "mix_entrance_pools": list(ENTRANCE_MIX_POOLS),
    }
