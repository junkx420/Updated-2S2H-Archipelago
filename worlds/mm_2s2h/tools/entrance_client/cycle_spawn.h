#pragma once

inline void TestCycleSpawn() {
    using namespace Rando::EntranceShuffle;
    namespace ET = Rando::EntranceTracker;
    std::fill_n(options, RO_MAX, 0);
    gSaveContext = {};
    auto& save = gSaveContext.save;
    auto& rando = save.shipSaveInfo.rando;
    SetCycleSpawn(0);
    save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
    QueueCycleSpawnOnLoad();
    assert(!Rando_PrepareCycleSpawn() && save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 0));
    options[RO_SHUFFLE_SPAWN] = 1;
    options[RO_CLOCK_SHUFFLE] = 1;
    options[RO_CLOCK_TERMINAL_TIME] = 350;

    int cases = 0;
    for (int target = 0; target < (int)cycleSpawns.size(); ++target) {
        ApSlot::SlotData slot;
        assert(slot.Parse(json{ { "options", json::object() }, { "spawn_target", target } }).empty());
        SetCycleSpawn(slot.spawnTarget);
        const auto& spawn = cycleSpawns[target];
        const auto lines = TrackedEntrances();
        assert(lines.size() == 1 && ET::BuildRows(lines, {}).front().to == ET::HIDDEN_TARGET);

        for (int halfDay = -1; halfDay < Rando::ClockItems::HALF_COUNT; ++halfDay) {
            for (bool playedSong : { false, true }) {
                const bool dawn = halfDay >= 0 && halfDay % 2 == 0;
                ownedClocks = 0;
                gSaveContext.nextCutsceneIndex = playedSong ? 0xFFF7 : 0xFFEF;
                save.entrance = playedSong ? ENTRANCE(CUTSCENE, 1) : ENTRANCE(SOUTH_CLOCK_TOWN, 0);
                save.day = save.time = 0;
                save.eventDayCount = 7;
                save.isOwlSave = false;
                if (!playedSong) {
                    // The file-select hook queues the target only after the clock wait succeeds.
                    assert(!Rando_PrepareCycleSpawn());
                    QueueCycleSpawnOnLoad();
                }
                ownedClocks = halfDay == -1 ? 0 : 1 << halfDay;
                assert(Rando_PrepareCycleSpawn() == dawn);
                assert(save.entrance == spawn.entrance);
                assert(gSaveContext.nextCutsceneIndex == 0xFFEF && save.cutsceneIndex == 0);
                assert(save.eventDayCount == 0);
                assert(save.day == (halfDay == -1 ? 3 : halfDay / 2 + (dawn ? 0 : 1)));
                assert(save.time == (halfDay == -1 ? CLOCK_TIME(5, 50) : dawn ? DAWN_TIME : DUSK_TIME));
                if (halfDay == -1) {
                    assert(TowerOpensAt(save.day, save.time));
                }
                assert(!Rando_PrepareCycleSpawn());
                OnSceneInit(TrackedArrivalScene(lines.front()), 0);
                assert(IsDiscovered(lines.front().from));
                rando.cycleSpawn[1] = 0;
                ++cases;
            }
        }
        for (int minutes : { 0, 359 }) {
            options[RO_CLOCK_TERMINAL_TIME] = minutes;
            ownedClocks = 0;
            gSaveContext.nextCutsceneIndex = 0xFFF7;
            save.entrance = ENTRANCE(CUTSCENE, 0);
            assert(!Rando_PrepareCycleSpawn());
            assert(save.entrance == spawn.entrance && TowerOpensAt(save.day, save.time));
            assert(save.time == CLOCK_TIME(0, minutes));
            OnSceneInit(TrackedArrivalScene(lines.front()), 0);
            ++cases;
        }
        options[RO_CLOCK_TERMINAL_TIME] = 350;
        const auto route = std::find_if(shuffleEntrances.begin(), shuffleEntrances.end(),
                                        [&](const auto& row) { return row.into == spawn.entrance; });
        assert(route != shuffleEntrances.end());
        const auto mirror = std::find_if(shuffleEntrances.begin(), shuffleEntrances.end(),
                                         [&](const auto& row) { return row.into == spawn.exitToSouth; });
        assert(mirror != shuffleEntrances.end());
        options[RO_SHUFFLE_OVERWORLD] = 1;
        for (auto entry : { route, mirror }) {
            const size_t row = entry - shuffleEntrances.begin();
            assert(IsCycleSpawnRoute(row) && !CanMix(2 * row, 2 * row + 2));
            assert(!CanPair(row, (row + 1) % shuffleEntrances.size()));
        }
        options[RO_SHUFFLE_OVERWORLD] = 0;

        save.isOwlSave = true;
        save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
        QueueCycleSpawnOnLoad();
        assert(!Rando_PrepareCycleSpawn() && save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 0));
        save.entrance = ENTRANCE(CUTSCENE, 0);
        gSaveContext.nextCutsceneIndex = 0xFFF2;
        assert(!Rando_PrepareCycleSpawn() && save.entrance == ENTRANCE(CUTSCENE, 0));
    }

    json legacy = rando;
    legacy.erase("cycleSpawn");
    from_json(legacy, rando);
    assert(!SelectedCycleSpawn() && TrackedEntrances().empty());
    std::cout << "Cycle spawn client: " << cases << " clock/start cases; fixed town routes and legacy save passed\n";
}
