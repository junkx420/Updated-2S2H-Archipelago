from typing import Any, cast

from BaseClasses import CollectionState
from test.general import gen_steps, setup_multiworld
from worlds.AutoWorld import call_all

from .. import MM2S2HWorld
from . import MM2S2HTestBase


class UTRegenTest(MM2S2HTestBase):
    """Generate a seed, then rebuild it from its slot data the way Universal Tracker does, with *different*
    yaml options and seed: the rebuilt world must have the same options, rolls, locations and logic."""
    options = {"shuffle_shops": True, "shuffle_gold_skulltulas": True, "skulltulas_shuffled": 10,
               "clock_shuffle": True, "shuffle_pots": True}

    def test_regen_from_slot_data(self) -> None:
        original = self.world
        slot_data: dict[str, Any] = original.fill_slot_data()
        mw = setup_multiworld(MM2S2HWorld, steps=(), seed=999)
        setattr(mw, "re_gen_passthrough", {MM2S2HWorld.game: MM2S2HWorld.interpret_slot_data(slot_data)})
        for step in gen_steps:
            call_all(mw, step)
        rebuilt = cast(MM2S2HWorld, mw.worlds[1])

        self.assertEqual(original.ro, rebuilt.ro)
        self.assertEqual(original.prices, rebuilt.prices)
        self.assertEqual(original.vanilla_skulltulas, rebuilt.vanilla_skulltulas)
        self.assertEqual(original.starting, rebuilt.starting)
        names = lambda w: {loc.name for loc in w.multiworld.get_locations(w.player) if loc.address}  # noqa: E731
        self.assertEqual(names(original), names(rebuilt))

        # same reachability with the same (start) inventory
        def reachable(w: MM2S2HWorld) -> set[str]:
            state = CollectionState(w.multiworld)
            return {loc.name for loc in w.multiworld.get_locations(w.player) if loc.address and loc.can_reach(state)}
        self.assertEqual(reachable(original), reachable(rebuilt))
