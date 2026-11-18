from BaseClasses import CollectionState

from .. import ITEMS
from ..enums_generated import NPC_SOULS
from ..locations_generated import LOCATIONS
from . import MM2S2HTestBase

SOUL_NAMES = {ITEMS[ri][0] for ri in NPC_SOULS}
CLASSIC_NAMES = {ITEMS[ri][0] for ri, row in NPC_SOULS.items() if row[3] == 1}
USEFUL_NAMES = {ITEMS[ri][0] for ri, (_, checks, events, _) in NPC_SOULS.items() if not checks and not events}
ALL_CHECKS = {"shuffle_cows": True, "shuffle_tingle_shops": True, "shuffle_shops": True, "shuffle_frogs": True}


def check_souls(test: MM2S2HTestBase, level: int) -> int:
    """Every check and event listed for a soul of this level is out of logic without the soul and in logic with
    everything. Returns the number of gated checks."""
    mw, player = test.multiworld, test.player
    location_names = {loc.name for loc in mw.get_locations(player)}
    events_at: dict[str, list[str]] = {}
    for loc in mw.get_locations(player):
        if loc.address is None and " @ " in loc.name:
            events_at.setdefault(loc.name.split(" @ ")[0], []).append(loc.name)
    gated = 0
    for ri, (_, checks, events, row_level) in NPC_SOULS.items():
        if row_level > level or (not checks and not events):
            continue
        soul = ITEMS[ri][0]
        state = CollectionState(mw)
        for item in mw.itempool:
            if item.name != soul:
                state.collect(item, True)
        state.sweep_for_advancements()
        names = [LOCATIONS[rc][0] for rc in checks if LOCATIONS[rc][0] in location_names]
        event_locs = [name for event in events for name in events_at.get(event, [])]
        test.assertTrue(names or event_locs, f"{soul}: none of its checks or events is a location here")
        for name in names + event_locs:
            test.assertFalse(state.can_reach_location(name, player), f"{name} reachable without {soul}")
        state.collect(test.get_item_by_name(soul), True)
        state.sweep_for_advancements()
        for name in names:
            test.assertTrue(state.can_reach_location(name, player), f"{name} unreachable with {soul}")
        for event in events:  # an event may sit in several regions; one of them is enough
            if events_at.get(event):
                test.assertTrue(any(state.can_reach_location(name, player) for name in events_at[event]),
                                f"{event} unreachable with {soul}")
        gated += len(names)
    return gated


class NpcSoulsOff(MM2S2HTestBase):
    """Off by default: no soul items, nothing gated."""

    def test_no_souls_in_pool(self) -> None:
        self.assertFalse(SOUL_NAMES & {item.name for item in self.multiworld.itempool})


class NpcSouls(MM2S2HTestBase):
    """Value 1 (true in old yamls): the 30 souls of 0.2 to 0.11, nothing more."""
    options = {"shuffle_npc_souls": True} | ALL_CHECKS

    def test_classic_souls_in_pool(self) -> None:
        pool = [item.name for item in self.multiworld.itempool if item.name in SOUL_NAMES]
        self.assertEqual(len(CLASSIC_NAMES), 30)
        self.assertEqual(sorted(pool), sorted(CLASSIC_NAMES))
        self.assertTrue(all(item.advancement for item in self.multiworld.itempool if item.name in SOUL_NAMES))

    def test_checks_need_their_soul(self) -> None:
        self.assertGreater(check_souls(self, 1), 60)

    def test_new_souls_gate_nothing(self) -> None:
        """A check only a level-2 soul gates (e.g. a Trading Post slot) needs no soul at value 1."""
        name = LOCATIONS["RC_TRADING_POST_SHOP_ITEM_01"][0]
        self.assertIn(name, {loc.name for loc in self.multiworld.get_locations(self.player)})
        state = CollectionState(self.multiworld)
        for item in self.multiworld.itempool:
            state.collect(item, True)
        state.sweep_for_advancements()
        self.assertTrue(state.can_reach_location(name, self.player))


class NpcSoulsClassicByName(NpcSouls):
    """"classic" is the same as true."""
    options = {"shuffle_npc_souls": "classic"} | ALL_CHECKS


class NpcSoulsAll(MM2S2HTestBase):
    """Value 2: all 65 souls; every listed check and event needs its soul; souls without any are useful."""
    options = {"shuffle_npc_souls": "all"} | ALL_CHECKS

    def test_all_souls_in_pool(self) -> None:
        pool = [item for item in self.multiworld.itempool if item.name in SOUL_NAMES]
        self.assertEqual(len(SOUL_NAMES), 65)
        self.assertEqual(sorted(item.name for item in pool), sorted(SOUL_NAMES))
        for item in pool:
            if item.name in USEFUL_NAMES:
                self.assertFalse(item.advancement, item.name)
                self.assertTrue(item.useful, item.name)
            else:
                self.assertTrue(item.advancement, item.name)
        self.assertEqual(len(USEFUL_NAMES), 8)

    def test_checks_need_their_soul(self) -> None:
        self.assertGreater(check_souls(self, 2), 130)  # 141 with these options

    def test_beatable_with_everything(self) -> None:
        state = CollectionState(self.multiworld)
        for item in self.multiworld.itempool:
            state.collect(item, True)
        state.sweep_for_advancements()
        self.assertTrue(self.multiworld.can_beat_game(state))


class NpcSoulsNoLogic(MM2S2HTestBase):
    """No Logic drops the soul rules like every other rule."""
    options = {"shuffle_npc_souls": True, "logic": "no_logic"}

    def test_everything_reachable_without_items(self) -> None:
        state = CollectionState(self.multiworld)
        self.assertEqual([loc.name for loc in self.multiworld.get_locations(self.player)
                          if loc.address and not loc.can_reach(state)], [])


class NpcSoulsAllNoLogic(NpcSoulsNoLogic):
    options = {"shuffle_npc_souls": "all", "logic": "no_logic"}


class NpcSoulsAllOwnOptions(MM2S2HTestBase):
    """Value 2 together with every option of this fork (the base tests fill and check reachability)."""
    options = {"shuffle_npc_souls": "all", "shuffle_interiors": True, "shuffle_dungeon_entrances": True,
               "shuffle_boss_entrances": True, "shuffle_grottos": True, "shuffle_overworld": True,
               "shuffle_special_areas": True, "shuffle_door_keys": True, "shuffle_epona_riding": True,
               "shuffle_bean_planting": True, "shuffle_swim": True} | ALL_CHECKS

    def test_souls_in_pool(self) -> None:
        self.assertEqual(sum(item.name in SOUL_NAMES for item in self.multiworld.itempool), 65)
