from BaseClasses import CollectionState

from .. import ITEMS
from ..enums_generated import NPC_SOULS
from ..locations_generated import LOCATIONS
from . import MM2S2HTestBase

SOUL_NAMES = {ITEMS[ri][0] for ri in NPC_SOULS}


class NpcSoulsOff(MM2S2HTestBase):
    """Off by default: no soul items, nothing gated."""

    def test_no_souls_in_pool(self) -> None:
        self.assertFalse(SOUL_NAMES & {item.name for item in self.multiworld.itempool})


class NpcSouls(MM2S2HTestBase):
    """Every check listed for an NPC is out of logic without that NPC's soul and in logic with everything."""
    options = {"shuffle_npc_souls": True, "shuffle_cows": True, "shuffle_tingle_shops": True, "shuffle_shops": True}

    def test_all_souls_in_pool(self) -> None:
        pool = [item.name for item in self.multiworld.itempool if item.name in SOUL_NAMES]
        self.assertEqual(sorted(pool), sorted(SOUL_NAMES))
        self.assertTrue(all(item.advancement for item in self.multiworld.itempool if item.name in SOUL_NAMES))

    def test_checks_need_their_soul(self) -> None:
        mw, player = self.multiworld, self.player
        location_names = {loc.name for loc in mw.get_locations(player)}
        gated = 0
        for ri, (_, checks, _) in NPC_SOULS.items():
            soul = ITEMS[ri][0]
            state = CollectionState(mw)
            for item in mw.itempool:
                if item.name != soul:
                    state.collect(item, True)
            state.sweep_for_advancements()
            names = [LOCATIONS[rc][0] for rc in checks if LOCATIONS[rc][0] in location_names]
            self.assertTrue(names, f"{soul}: none of its checks is a location with these options")
            for name in names:
                self.assertFalse(state.can_reach_location(name, player), f"{name} reachable without {soul}")
            state.collect(self.get_item_by_name(soul), True)
            state.sweep_for_advancements()
            for name in names:
                self.assertTrue(state.can_reach_location(name, player), f"{name} unreachable with {soul}")
            gated += len(names)
        self.assertGreater(gated, 60)


class NpcSoulsNoLogic(MM2S2HTestBase):
    """No Logic drops the soul rules like every other rule."""
    options = {"shuffle_npc_souls": True, "logic": "no_logic"}

    def test_everything_reachable_without_items(self) -> None:
        state = CollectionState(self.multiworld)
        self.assertEqual([loc.name for loc in self.multiworld.get_locations(self.player)
                          if loc.address and not loc.can_reach(state)], [])
