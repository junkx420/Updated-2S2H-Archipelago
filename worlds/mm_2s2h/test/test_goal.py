from .. import ITEMS
from . import MM2S2HTestBase

SWORDS = [ITEMS[ri][0] for ri in ("RI_PROGRESSIVE_SWORD", "RI_SWORD_KOKIRI", "RI_SWORD_RAZOR", "RI_SWORD_GILDED",
                                  "RI_GREAT_FAIRY_SWORD", "RI_MASK_FIERCE_DEITY")]


class GoalNeedsAWeapon(MM2S2HTestBase):
    """Reaching Majora's lair is not the goal: the fight needs a sword (2S2H's logic has no rule for it)."""
    options = {"starting_items": {"Ocarina of Time": 1, "Song of Time": 1}}

    def test_no_sword_no_goal(self) -> None:
        self.collect_all_but(SWORDS)
        self.assertTrue(self.multiworld.state.can_reach_region("Moon Majoras Lair", self.player))
        self.assertFalse(self.can_reach_location("Defeat Majora"))
        self.collect_by_name(ITEMS["RI_GREAT_FAIRY_SWORD"][0])
        self.assertTrue(self.can_reach_location("Defeat Majora"))


class GoalNoLogic(MM2S2HTestBase):
    """Without logic the fight rule is dropped like every other access rule."""
    options = {"logic": "no_logic", "starting_items": {"Ocarina of Time": 1, "Song of Time": 1}}

    def test_goal_open(self) -> None:
        self.assertTrue(self.can_reach_location("Defeat Majora"))
