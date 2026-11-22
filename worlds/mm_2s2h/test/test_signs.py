from collections import Counter
from itertools import product

from BaseClasses import CollectionState

from ..enums_generated import ENTRANCE_MIX_POOLS, RI_ID
from ..items_generated import ITEMS
from ..locations_generated import LOCATIONS
from ..primitives import CLOCKS, SaveView
from ..regions_generated import EXTRA_CHECK_WINDOWS
from . import MM2S2HTestBase


SIGNS = {rc: data[0] for rc, data in LOCATIONS.items() if data[1] == "RCTYPE_SIGN"}
BANK = "West Clock Town: Sign Bank"
DARMANI = "Goron Graveyard: Sign Darmani's Grave"
MIKAU = "Great Bay Coast: Sign Mikau's Grave"
SHRINE = "Ikana Canyon: Sign Secret Shrine"
JUMPING_GAME = "Great Bay Coast: Sign Fisherman's Jumping Game"
CAN_HEAL_MIKAU = Counter(RI_ABILITY_SWIM=1, RI_OCARINA=1, RI_SONG_HEALING=1,
                         RI_OCARINA_BUTTON_C_LEFT=1, RI_OCARINA_BUTTON_C_RIGHT=1, RI_OCARINA_BUTTON_C_DOWN=1)


class SignsOff(MM2S2HTestBase):
    def test_default_pool_and_slot_have_no_sign_shuffle(self) -> None:
        self.assertEqual(self.world.ro["RO_SHUFFLE_SIGNS"], 0)
        self.assertNotIn("RO_SHUFFLE_SIGNS", self.world.fill_slot_data()["options"])
        self.assertFalse(set(SIGNS.values()) & {loc.name for loc in self.multiworld.get_locations(self.player)})


class SignsOn(MM2S2HTestBase):
    options = {"shuffle_signs": True}

    def test_every_sign_is_a_named_reachable_location(self) -> None:
        self.assertEqual(len(SIGNS), 87)
        self.assertEqual(self.world.location_name_groups["Signs"], set(SIGNS.values()))
        self.assertTrue(all(": Sign " in name for name in SIGNS.values()))
        self.assertEqual(self.world.fill_slot_data()["options"]["RO_SHUFFLE_SIGNS"], 1)
        state = self.multiworld.get_all_state()
        self.assertEqual([name for name in SIGNS.values() if not state.can_reach_location(name, self.player)], [])

    def test_bank_sign_and_reward_are_separate(self) -> None:
        state = CollectionState(self.multiworld)
        sign = self.multiworld.get_location(BANK, self.player)
        self.assertTrue(sign.can_reach(state))
        bank_reward = LOCATIONS["RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART"][0]
        self.assertNotEqual(sign.address, self.world.location_name_to_id[bank_reward])

    def test_reading_requirements(self) -> None:
        self.assertFalse(self.location_rule(DARMANI, Counter()))
        self.assertTrue(self.location_rule(DARMANI, Counter(RI_MASK_GORON=1)))
        self.assertFalse(self.location_rule(MIKAU, CAN_HEAL_MIKAU - Counter(RI_SONG_HEALING=1)))
        self.assertTrue(self.location_rule(MIKAU, CAN_HEAL_MIKAU))
        self.assertFalse(self.location_rule(JUMPING_GAME, Counter()))
        self.assertTrue(self.location_rule(JUMPING_GAME, Counter(), Counter(RE_CLEARED_GREAT_BAY_TEMPLE=1)))

    def location_rule(self, name: str, inventory: Counter, events: Counter | None = None) -> bool:
        state = CollectionState(self.multiworld)
        state.mm2s2h_views[self.player] = SaveView(inventory, events or Counter(), self.world)
        return self.multiworld.get_location(name, self.player).access_rule(state)


class SignsAvailability(MM2S2HTestBase):
    options = {"shuffle_signs": True, "clock_shuffle": True, "clock_shuffle_mode": "any_order"}

    def test_every_sign_half_day_and_layout(self) -> None:
        expected_masks = {
            **{f"RC_WOODS_OF_MYSTERY_SIGN_DAY_{day}_EXIT_{exit_}": 3 << (2 * (day - 1))
               for day, exit_ in product(range(1, 4), repeat=2)},
            "RC_GORMAN_TRACK_SIGN_REAR_GATE": 0x30,
            "RC_POST_OFFICE_SIGN_SCHEDULE_EAST": 0x1F,
        }
        base_layout = {
            "RC_GREAT_BAY_COAST_SIGN_FISHERMANS_HUT", "RC_GREAT_BAY_COAST_SIGN_PIRATES_COVE",
            "RC_GREAT_BAY_COAST_SIGN_INDIGO_GOS", "RC_GREAT_BAY_COAST_SIGN_GREAT_BAY_TEMPLE",
            "RC_GREAT_BAY_COAST_SIGN_MURKY_WATER", "RC_ZORA_CAPE_SIGN_FALL_HEADWATERS",
            "RC_ZORA_CAPE_SIGN_GREAT_BAY_TEMPLE", "RC_ZORA_CAPE_SIGN_ZORA_GAME_SITE",
        }
        self.assertLessEqual(expected_masks.keys() | base_layout, SIGNS.keys())
        state = CollectionState(self.multiworld)
        items = Counter({ri: 1 for ri in RI_ID if not ri.startswith("RI_TIME_")})
        for half_day, gyorg_cleared in product(range(6), (False, True)):
            view = SaveView(items | Counter({CLOCKS[half_day]: 1}),
                            Counter(RE_CLEARED_GREAT_BAY_TEMPLE=int(gyorg_cleared)), self.world)
            state.mm2s2h_views[self.player] = view
            for rc, name in SIGNS.items():
                with self.subTest(check=rc, half_day=half_day, gyorg_cleared=gyorg_cleared):
                    half_days, layout = EXTRA_CHECK_WINDOWS[rc]
                    self.assertEqual(half_days, expected_masks.get(rc, 0x3F))
                    self.assertEqual(layout, "GREAT_BAY_BASE" if rc in base_layout else "ANY")
                    in_window = bool(half_days & (1 << half_day))
                    self.assertEqual(view.time.extra_check_available(half_days, layout, snowhead_cleared=False,
                                                                     great_bay_cleared=gyorg_cleared),
                                     in_window and (rc not in base_layout or not gyorg_cleared))
                    expected = in_window and (name != JUMPING_GAME or gyorg_cleared)
                    self.assertEqual(self.multiworld.get_location(name, self.player).access_rule(state), expected)


class SignsNoLogic(MM2S2HTestBase):
    options = {"shuffle_signs": True, "logic": "no_logic"}

    def test_every_sign_is_reachable_without_items(self) -> None:
        state = CollectionState(self.multiworld)
        self.assertEqual([name for name in SIGNS.values() if not state.can_reach_location(name, self.player)], [])


class SignsAllOwnOptions(MM2S2HTestBase):
    options = {
        "shuffle_signs": True, "shuffle_rocks": True, "shuffle_npc_souls": "all", "shuffle_interiors": True,
        "shuffle_dungeon_entrances": True, "shuffle_boss_entrances": True, "shuffle_grottos": True,
        "shuffle_overworld": True, "shuffle_special_areas": True, "shuffle_door_keys": True,
        "shuffle_epona_riding": True, "shuffle_bean_planting": True, "shuffle_swim": True,
        "mix_entrance_pools": list(ENTRANCE_MIX_POOLS),
    }

    def test_all_signs_are_reachable_with_full_inventory(self) -> None:
        state = self.multiworld.get_all_state()
        self.assertEqual([name for name in SIGNS.values() if not state.can_reach_location(name, self.player)], [])

    def test_mikau_and_shrine_use_shuffled_swimming_and_mikau_needs_his_soul(self) -> None:
        state = CollectionState(self.multiworld)
        mikau = self.multiworld.get_location(MIKAU, self.player)
        shrine = self.multiworld.get_location(SHRINE, self.player)
        self.assertFalse(shrine.access_rule(state))
        inventory = CAN_HEAL_MIKAU
        state.mm2s2h_views[self.player] = SaveView(inventory, Counter(), self.world)
        self.assertTrue(shrine.access_rule(state))
        self.assertFalse(mikau.access_rule(state))
        state.collect(self.world.create_item(ITEMS["RI_SOUL_NPC_INDIGO_GOS"][0]))
        state.mm2s2h_views[self.player] = SaveView(inventory, Counter(), self.world)
        self.assertTrue(mikau.access_rule(state))
