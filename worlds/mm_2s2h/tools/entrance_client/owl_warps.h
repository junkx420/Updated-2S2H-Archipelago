#pragma once

inline void TestOwlWarps() {
    using namespace Rando::EntranceShuffle;
    namespace ET = Rando::EntranceTracker;
    std::fill_n(options, RO_MAX, 0);
    gSaveContext = {};
    auto& save = gSaveContext.save.shipSaveInfo.rando;
    const std::vector<int32_t> targets{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    const json slotJson = { { "options", { { "RO_SHUFFLE_OWL_WARPS", 1 } } }, { "owl_warp_targets", targets } };
    ApSlot::SlotData slot;
    assert(slot.Parse(slotJson).empty() && slot.owlWarpTargets == targets);
    SetOwlWarpTargets(slot.owlWarpTargets);
    assert(TrackedEntrances().empty());
    for (const auto& owl : owlWarps) {
        assert(Rando_ResolveOwlWarp(owl.id, owl.entrance) == owl.entrance);
    }

    options[RO_SHUFFLE_OWL_WARPS] = 1;
    auto malformed = targets;
    malformed[0] = malformed[1];
    SetOwlWarpTargets(malformed);
    assert(TrackedEntrances().empty());
    SetOwlWarpTargets(slot.owlWarpTargets);
    const auto lines = TrackedEntrances();
    assert(lines.size() == 10);
    assert(ET::BuildRows(lines, {}).size() == 10);
    for (const auto& row : ET::BuildRows(lines, {})) {
        assert(row.to == ET::HIDDEN_TARGET);
    }

    options[RO_LOGIC] = RO_LOGIC_NO_LOGIC;
    canPlayTime = canBeDeku = canBeZora = canSwim = hasHookshot = false;
    for (const auto& owl : owlWarps) {
        const auto target = OwlWarpTarget(owl.id);
        const auto id = owlWarps[target].id;
        const bool needsEscape = id == OWL_WARP_GREAT_BAY_COAST || id == OWL_WARP_ZORA_CAPE ||
                                 id == OWL_WARP_WOODFALL || id == OWL_WARP_STONE_TOWER;
        assert(Rando_CanSelectOwlWarp(owl.id) == !needsEscape); // applies even in No Logic
    }
    canPlayTime = true;
    for (const auto& owl : owlWarps) {
        assert(Rando_CanSelectOwlWarp(owl.id));
    }

    for (const auto& owl : owlWarps) {
        const size_t target = OwlWarpTarget(owl.id);
        const auto& destination = owlWarps[target];
        flags = WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE | WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE;
        auto expected = destination.entrance;
        if (destination.id == OWL_WARP_MOUNTAIN_VILLAGE) {
            expected = ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8);
        } else if (destination.id == OWL_WARP_SOUTHERN_SWAMP) {
            expected = ENTRANCE(SOUTHERN_SWAMP_CLEARED, 10);
        }
        assert(Rando_ResolveOwlWarp(owl.id, owl.entrance) == expected);
        assert(!IsDiscovered(lines[owl.id].from));
        gSaveContext.save.entrance = expected;
        OnSceneInit(TrackedArrivalScene(lines[owl.id]), 0);
        assert(IsDiscovered(lines[owl.id].from));
        assert(owl.id == 9 || !IsDiscovered(lines[owl.id + 1].from));
        assert(ToName(lines[owl.id]) == destination.name);
    }

    const json roundTrip = save;
    save = {};
    from_json(roundTrip, save);
    assert(json(save) == roundTrip);
    SetOwlWarpTargets(slot.owlWarpTargets);
    assert(IsDiscovered(lines[0].from)); // reconnecting the same seed keeps its discoveries
    std::fill_n(save.owlWarpsDiscovered, sizeof(save.owlWarpsDiscovered), 0);

    // Owl save reload supplies the physical owl, and both respawns supply the real saved arrival.
    for (const auto& owl : owlWarps) {
        gSaveContext.save.entrance = owl.entrance;
        OnSceneInit(Entrance_GetSceneIdAbsolute(owl.entrance), 0);
        for (int flag : { -1, 1 }) {
            gSaveContext.respawnFlag = flag;
            playState.nextEntrance = owl.entrance;
            OnBeforeTransitionSetup();
            assert(playState.nextEntrance == owl.entrance);
        }
        assert(!IsDiscovered(lines[owl.id].from));
    }
    json legacy = roundTrip;
    legacy.erase("owlWarpTargets");
    legacy.erase("owlWarpsDiscovered");
    from_json(legacy, save);
    assert(TrackedEntrances().empty());
    assert(!IsDiscovered(lines[0].from));
    std::cout << "Owl warp client: 10 arrivals, 20 respawns, 10 owl loads; escape, tracker, slot and save checks passed\n";
}
