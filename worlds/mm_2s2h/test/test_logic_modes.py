from BaseClasses import CollectionState
from Fill import distribute_items_restrictive

from .. import ITEMS, NNL_SCENE_BLACKLIST, NNL_TYPE_BLACKLIST
from ..locations_generated import LOCATIONS
from . import MM2S2HTestBase

LOC_INFO = {name: (rctype, scene) for name, rctype, scene, *_ in LOCATIONS.values()}


class NoLogic(MM2S2HTestBase):
    """No Logic must drop every access rule: all checks are reachable with nothing collected."""
    options = {"logic": "no_logic", "shuffle_pots": True, "clock_shuffle": True}

    def test_everything_reachable_without_items(self) -> None:
        state = CollectionState(self.multiworld)
        unreachable = [loc.name for loc in self.multiworld.get_locations(self.player)
                       if loc.address and not loc.can_reach(state)]
        self.assertEqual(unreachable, [])


class NearlyNoLogic(NoLogic):
    """Nearly No Logic: no access rules either, plus 2S2H's placement blacklists for this world's items."""
    options = {"logic": "nearly_no_logic", "shuffle_pots": True, "shuffle_cows": True, "clock_shuffle": True}

    def test_blacklists_respected(self) -> None:
        distribute_items_restrictive(self.multiworld)
        scene_bl = {ITEMS[ri][0]: scenes for ri, scenes in NNL_SCENE_BLACKLIST.items()}
        type_bl = {ITEMS[ri][0]: t for ri, t in NNL_TYPE_BLACKLIST.items()}
        checked = 0
        for loc in self.multiworld.get_locations(self.player):
            item = loc.item
            if not loc.address or not item or item.player != self.player:
                continue
            rctype, scene = LOC_INFO[loc.name]
            if item.name in scene_bl:
                self.assertNotIn(scene, scene_bl[item.name], f"{item.name} at {loc.name}")
                checked += 1
            if item.name in type_bl:
                self.assertNotEqual(rctype, type_bl[item.name], f"{item.name} at {loc.name}")
                checked += 1
        self.assertGreater(checked, 0)
