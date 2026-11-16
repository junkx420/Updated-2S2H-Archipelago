from Fill import distribute_items_restrictive

from .. import DUNGEON_SCENES, MM2S2HWorld
from ..locations_generated import LOCATIONS
from . import MM2S2HTestBase

LOC_SCENE = {name: scene for name, _, scene, *_ in LOCATIONS.values()}


class OwnDungeonDefaultChecks(MM2S2HTestBase):
    """Own-dungeon keys and fairies are pre-placed in pre_fill: they must end up in their own dungeon and the
    seed must stay beatable."""
    options = {"small_keys": "own_dungeon", "boss_keys": "own_dungeon", "stray_fairies": "own_dungeon"}

    def test_confined_items_in_own_dungeon(self) -> None:
        world: MM2S2HWorld = self.world  # type: ignore[assignment]
        self.assertTrue(world.confined)
        self.assertFalse([i for i in self.multiworld.itempool if i.player == self.player and i.name in world.confined],
                         "confined items left in the pool after pre_fill")
        distribute_items_restrictive(self.multiworld)
        placed = 0
        for loc in self.multiworld.get_locations(self.player):
            if loc.item and loc.item.player == self.player and loc.item.name in world.confined:
                self.assertIn(LOC_SCENE[loc.name], DUNGEON_SCENES[world.confined[loc.item.name]], loc.name)
                placed += 1
        self.assertGreater(placed, 0)
        self.assertTrue(self.multiworld.can_beat_game())


class OwnDungeonPlentiful(OwnDungeonDefaultChecks):
    """Keys doubled by plentiful items (needs one more check type, or the pool does not fit at all)."""
    options = OwnDungeonDefaultChecks.options | {"plentiful_items": True, "shuffle_skeleton_key": False,
                                                 "shuffle_pots": True}


class OwnDungeonAllChecks(OwnDungeonDefaultChecks):
    options = OwnDungeonDefaultChecks.options | {"shuffle_pots": True, "shuffle_grass": True, "shuffle_crates": True,
                                         "shuffle_boss_souls": True, "shuffle_enemy_souls": True,
                                         "clock_shuffle": True}
