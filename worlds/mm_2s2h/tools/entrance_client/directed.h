#pragma once

inline void TestDirectedEntrances() {
    using namespace Rando::EntranceShuffle;
    namespace ET = Rando::EntranceTracker;
    int walked = 0;
    int oneWay = 0;
    int bossReturns = 0;
    for (const char* fixture : directedSlots) {
        const json data = json::parse(fixture);
        ApSlot::SlotData slot;
        assert(slot.Parse(data).empty());
        const std::map<std::string, int> supported = {
            { "RO_SHUFFLE_DUNGEON_ENTRANCES", RO_SHUFFLE_DUNGEON_ENTRANCES },
            { "RO_SHUFFLE_BOSS_ENTRANCES", RO_SHUFFLE_BOSS_ENTRANCES },
            { "RO_SHUFFLE_GROTTOS", RO_SHUFFLE_GROTTOS },
            { "RO_SHUFFLE_INTERIORS", RO_SHUFFLE_INTERIORS },
            { "RO_SHUFFLE_OVERWORLD", RO_SHUFFLE_OVERWORLD },
            { "RO_SHUFFLE_SPECIAL_AREAS", RO_SHUFFLE_SPECIAL_AREAS },
            { "RO_MIX_ENTRANCE_POOLS", RO_MIX_ENTRANCE_POOLS },
            { "RO_SHUFFLE_SPAWN", RO_SHUFFLE_SPAWN },
            { "RO_DECOUPLE_ENTRANCES", RO_DECOUPLE_ENTRANCES },
        };
        std::fill_n(options, RO_MAX, 0);
        for (const auto& [name, id] : supported) {
            options[id] = slot.options[name];
        }
        gSaveContext = {};
        SetCycleSpawn(slot.spawnTarget);
        SetPairs(slot.entrancePairs, slot.mixedEntrancePairs);
        SetDirectedTargets(slot.directedEntranceTargets);
        const auto lines = TrackedEntrances();
        assert(std::count_if(lines.begin(), lines.end(), [](const auto& line) { return line.directed; }) ==
               (int)slot.directedEntranceTargets.size());
        for (auto [source, target] : slot.directedEntranceTargets) {
            assert(DirectedTarget(source) == target);
            assert(!IsDirectedEndpoint(shuffleEntrances.size() * 2));
            TrackerTest::ClearDiscoveries();
            const auto& row = shuffleEntrances[source / 2];
            assert(!IsGrottoRow(source / 2) && !row.templeExit && !IsWaterRow(source / 2) && !IsTownGate(row));
            const u16 original = source % 2 ? row.outOf : row.into;
            const u16 arrival = EndpointArrival(target);
            gSaveContext.respawnFlag = -2;
            gSaveContext.nextCutsceneIndex = 0xFFF2;
            const s16 scene = SideScene(source);
            playState = { original, scene, 0 };
            gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = original;
            OnBeforeTransitionSetup();
            assert(playState.nextEntrance == arrival && gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == arrival);
            assert(IsDiscovered(source) && (target == source || !IsDiscovered(target)));
            assert(DirectedTarget(target));
            oneWay += DirectedTarget(target) != source;
            ++walked;

            for (int respawn : { 1, -1 }) {
                gSaveContext.respawnFlag = respawn;
                playState = { arrival, SideScene(target), 0 };
                OnBeforeTransitionSetup();
                assert(playState.nextEntrance == arrival);
            }
        }
        for (size_t boss = 0; boss < shuffleEntrances.size(); ++boss) {
            if (!shuffleEntrances[boss].templeExit) {
                continue;
            }
            size_t bossDoor = DoorForInside(boss);
            auto temple = std::find_if(shuffleEntrances.begin(), shuffleEntrances.end(), [&](const auto& row) {
                return row.option == RO_SHUFFLE_DUNGEON_ENTRANCES && row.outOf == shuffleEntrances[bossDoor].templeExit;
            });
            assert(temple != shuffleEntrances.end());
            const size_t templeInside = 2 * (temple - shuffleEntrances.begin()) + 1;
            const auto entry = std::find_if(slot.directedEntranceTargets.begin(), slot.directedEntranceTargets.end(),
                                            [&](const auto& pair) { return pair.second == templeInside; });
            if (entry != slot.directedEntranceTargets.end() && (entry->first != templeInside - 1 || bossDoor != boss)) {
                assert(ShuffledBossExterior(boss) == EndpointArrival(entry->first));
                ++bossReturns;
            }
        }
        auto& save = gSaveContext.save.shipSaveInfo.rando;
        auto before = json(save);
        auto bad = slot.directedEntranceTargets;
        bad.pop_back();
        SetDirectedTargets(bad);
        assert(!DirectedTarget(slot.directedEntranceTargets.front().first));
        from_json(before, save);
        assert(json(save) == before && DirectedTarget(slot.directedEntranceTargets.front().first));
        before.erase("directedEntranceTargets");
        from_json(before, save);
        assert(!DirectedTarget(slot.directedEntranceTargets.front().first));
    }
    assert(oneWay > 0 && bossReturns > 0);
    std::cout << "Directed client: " << walked << " passages, " << oneWay << " independent directions, "
              << bossReturns << " blue warps; discovery, respawns, slot data and saves passed\n";
}
