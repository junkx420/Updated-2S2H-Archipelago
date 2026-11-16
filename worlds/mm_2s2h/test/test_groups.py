from BaseClasses import LocationProgressType, PlandoOptions
from Fill import distribute_items_restrictive
from worlds.generic.Rules import exclusion_rules

from . import MM2S2HTestBase


class ExcludeGroup(MM2S2HTestBase):
    """A location group in exclude_locations excludes every check of that type (no progression placed there)."""
    options = {"shuffle_grass": True, "exclude_locations": {"Grass"}}

    def test_grass_excluded(self) -> None:
        option = self.world.options.exclude_locations
        option.verify(type(self.world), "Tester", PlandoOptions.none)  # what Generate does: expands the group
        excluded = option.value
        self.assertNotIn("Grass", excluded)
        exclusion_rules(self.multiworld, self.player, excluded)  # what Main does after set_rules
        grass = self.world.location_name_groups["Grass"]
        locs = [loc for loc in self.multiworld.get_locations(self.player) if loc.address and loc.name in grass]
        self.assertGreater(len(locs), 1000)
        self.assertTrue(all(loc.progress_type == LocationProgressType.EXCLUDED for loc in locs))
        distribute_items_restrictive(self.multiworld)
        self.assertFalse([loc.name for loc in locs if loc.item and loc.item.advancement])
