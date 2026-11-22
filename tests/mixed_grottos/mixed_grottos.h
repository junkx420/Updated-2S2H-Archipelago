#pragma once

namespace MixedGrottoTest {
using namespace Rando::EntranceShuffle;

static void CheckLanding(const GrottoReturnPoint& expected, u16 entrance) {
    const auto& r = gSaveContext.respawn[RESPAWN_MODE_UNK_3];
    assert(gSaveContext.respawnFlag == 4 && r.entrance == entrance && r.roomIndex == expected.room);
    assert(r.pos.x == expected.pos.x && r.pos.y == expected.pos.y && r.pos.z == expected.pos.z);
    assert(r.yaw == expected.yaw && r.playerParams == PLAYER_PARAMS(0xFF, PLAYER_START_MODE_GROTTO));
    assert(r.tempSwitchFlags == 0 && r.unk_18 == 0 && r.tempCollectFlags == 0);
    gSaveContext.save.entrance = entrance;
    const auto row = std::find_if(shuffleEntrances.begin(), shuffleEntrances.end(),
                                  [&expected](const auto& row) { return row.into == expected.into; });
    assert(row != shuffleEntrances.end());
    playState.sceneId = row->outsideScene;
    playState.position = 123;
    OnSceneInit(playState.sceneId, 0);
    for (const auto mode : { RESPAWN_MODE_DOWN, RESPAWN_MODE_TOP }) {
        assert(gSaveContext.respawn[mode].entrance == entrance && gSaveContext.respawn[mode].position == 123);
    }
}

static void Passage(size_t from, size_t to, int grottoExitFlag = 4, s16 sceneOverride = SCENE_MAX) {
    const auto& source = shuffleEntrances[from / 2];
    const auto& destination = shuffleEntrances[to / 2];
    const auto* cave = GrottoOf(from / 2);
    const bool inside = from % 2 != 0;
    auto& respawn = gSaveContext.respawn[RESPAWN_MODE_UNK_3];
    respawn = { ENTRANCE(SOUTH_CLOCK_TOWN, 0), 0, 999 };
    respawn.tempSwitchFlags = 0x11223344;
    playState = { inside ? source.outOf : source.into, inside ? source.insideScene : source.outsideScene, 0 };
    if (sceneOverride != SCENE_MAX) {
        playState.sceneId = sceneOverride;
    }
    const bool mounted = IsOverworldRow(from / 2);
    Actor horse;
    Player rider;
    if (mounted) {
        horse.world.pos = { 1, 2, 3 };
        rider.rideActor = &horse;
        playState.player = &rider;
        gHorseIsMounted = 1;
        hasEponasSong = true;
    }
    gSaveContext.respawnFlag = 0;
    gSaveContext.nextCutsceneIndex = 0xFFF0;
    if (cave) {
        respawn.data = (s8)cave->data;
        if (inside) {
            gSaveContext.save.entrance = cave->gameEntrance;
            gSaveContext.respawnFlag = grottoExitFlag;
            playState.nextEntrance = respawn.entrance;
        } else {
            playState.nextEntrance = cave->gameEntrance;
        }
    }
    std::fill_n(Discovered(), sizeof(RandoSaveInfo::entrancesDiscovered), 0);
    OnBeforeTransitionSetup();
    if (mounted) {
        assert(!gHorseIsMounted && gSaveContext.save.saveInfo.horseData.sceneId == source.outsideScene);
        assert(gSaveContext.save.saveInfo.horseData.pos.x == 1);
        playState.player = nullptr;
        hasEponasSong = false;
    }
    const bool walking = !cave || !inside || grottoExitFlag == 4;
    assert(IsDiscovered(from) == walking && IsDiscovered(to) == walking);
    if (destination.option != RO_SHUFFLE_GROTTOS) {
        assert(gSaveContext.respawnFlag == 0 && playState.nextEntrance == EndpointArrival(to));
    } else if (to % 2) {
        const auto& expected = *GrottoOf(to / 2);
        assert(gSaveContext.respawnFlag == 0 && playState.nextEntrance == expected.gameEntrance);
        assert((u8)respawn.data == expected.data);
        gSaveContext.save.entrance = playState.nextEntrance;
        assert(CurrentGrottoRow() == to / 2);
    } else {
        const auto& point = *GrottoReturnFor(to / 2);
        u16 entrance = point.entrance;
        if (entrance == ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0) && (flags & WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
            entrance = ENTRANCE(PATH_TO_GORON_VILLAGE_SPRING, 0);
        }
        if (entrance == ENTRANCE(SOUTHERN_SWAMP_POISONED, 3) && (flags & WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE)) {
            entrance = ENTRANCE(SOUTHERN_SWAMP_CLEARED, 3);
        }
        assert(playState.nextEntrance == entrance && gSaveContext.nextCutsceneIndex == 0);
        CheckLanding(point, entrance);
    }
}

static void Run() {
    std::fill(std::begin(options), std::end(options), 0);
    for (const auto option : { RO_SHUFFLE_DUNGEON_ENTRANCES, RO_SHUFFLE_INTERIORS, RO_SHUFFLE_OVERWORLD,
                               RO_SHUFFLE_SPECIAL_AREAS, RO_SHUFFLE_GROTTOS, RO_DECOUPLE_ENTRANCES }) {
        options[option] = 1;
    }
    options[RO_MIX_ENTRANCE_POOLS] = 31;
    flags = 0;
    gSaveContext = {};
    const auto rowOf = [](u16 into) {
        const auto row = std::find_if(shuffleEntrances.begin(), shuffleEntrances.end(),
                                      [into](const auto& row) { return row.into == into; });
        assert(row != shuffleEntrances.end());
        return (size_t)(row - shuffleEntrances.begin());
    };
    const std::array fixed{
        ENTRANCE(GROTTOS, 5),  ENTRANCE(GROTTOS, 12), ENTRANCE(GROTTOS, 16), ENTRANCE(GROTTOS, 22),
        ENTRANCE(GROTTOS, 23), ENTRANCE(GROTTOS, 27), ENTRANCE(GROTTOS, 28), ENTRANCE(GROTTOS, 31)
    };
    assert(std::size(grottoReturnPoints) == 18);
    size_t doorPairs = 0, landPairs = 0, cavePairs = 0, rejections = 0, doorRejections = 0, respawns = 0;
    for (const auto& cave : grottoEntrances) {
        const size_t row = rowOf(cave.into);
        const bool isFixed = std::find(fixed.begin(), fixed.end(), cave.into) != fixed.end();
        assert(bool(GrottoReturnFor(row)) == !isFixed);
        for (size_t a : { 2 * row, 2 * row + 1 }) {
            for (size_t b = 0; b < 2 * shuffleEntrances.size(); ++b) {
                const auto& target = shuffleEntrances[b / 2];
                const bool land = IsOverworldRow(b / 2);
                const bool valid =
                    a != b && !isFixed && IsMixedEndpoint(b) && !IsTownGate(target) && (land || a % 2 != b % 2);
                assert(CanMix(a, b) == valid);
                if (!valid) {
                    SetPairs({}, { { (int32_t)a, (int32_t)b } });
                    assert(SavedMixedPairs()[a] == 0 && SavedMixedPairs()[b] == 0);
                    ++rejections;
                    if (target.option == RO_SHUFFLE_DUNGEON_ENTRANCES || target.option == RO_SHUFFLE_INTERIORS ||
                        target.option == RO_SHUFFLE_SPECIAL_AREAS) {
                        ++doorRejections;
                    }
                    continue;
                }
                if (IsGrottoRow(b / 2) && b < a) {
                    continue; // Test each pair's two directions together.
                }
                // Runtime pair validation is exercised with an isolated pair and with a complete map below.
                SavedMixedPairs()[a] = (u16)b + 1;
                SavedMixedPairs()[b] = (u16)a + 1;
                assert(MixedPartner(a) == b && MixedPartner(b) == a);
                assert(!IsDirectedEndpoint(a) && !IsDirectedEndpoint(b));
                Passage(a, b);
                Passage(b, a);
                if (a % 2) {
                    Passage(a, b, 1);
                    Passage(a, b, -1);
                    respawns += 2;
                }
                if (IsGrottoRow(b / 2)) {
                    ++cavePairs;
                } else if (land) {
                    ++landPairs;
                } else {
                    ++doorPairs;
                }
                SavedMixedPairs()[a] = 0;
                SavedMixedPairs()[b] = 0;
            }
        }
    }
    const size_t swamp = 2 * rowOf(ENTRANCE(GROTTOS, 21));
    const size_t ramp = 2 * rowOf(ENTRANCE(GROTTOS, 24));
    const size_t temple = 2 * rowOf(ENTRANCE(WOODFALL_TEMPLE, 0)) + 1;
    const size_t boss = rowOf(ENTRANCE(ODOLWAS_LAIR, 0));
    for (auto outside : { swamp, ramp }) {
        SavedMixedPairs()[outside] = (u16)temple + 1;
        SavedMixedPairs()[temple] = (u16)outside + 1;
        const auto& bossRow = shuffleEntrances[boss];
        const auto& point = *GrottoReturnFor(outside / 2);
        flags = WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE | WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE;
        const u16 entrance =
            outside == swamp ? ENTRANCE(SOUTHERN_SWAMP_CLEARED, 3) : ENTRANCE(PATH_TO_GORON_VILLAGE_SPRING, 0);
        Passage(outside, temple, 4, outside == swamp ? SCENE_20SICHITAI2 : SCENE_17SETUGEN2);
        Passage(temple, outside);
        gSaveContext.respawnFlag = 0;
        playState = { bossRow.bossWarpsTo.front(), bossRow.insideScene, 0 };
        OnBeforeTransitionSetup();
        assert(playState.nextEntrance == entrance && (flags & WEEKEVENTREG_ENTERED_WOODFALL_TEMPLE_PRISON));
        CheckLanding(point, entrance);
        gameInteractor.events = { GIEventTransition{ 0, 0, bossRow.insideScene } };
        gSaveContext.respawnFlag = 0;
        playState = { ENTRANCE(GIANTS_CHAMBER, 0), bossRow.insideScene, 0 };
        OnBeforeTransitionSetup();
        assert(playState.nextEntrance == ENTRANCE(GIANTS_CHAMBER, 0) && gSaveContext.respawnFlag == 0);
        gSaveContext.save.entrance = (u16)std::get<GIEventTransition>(gameInteractor.events.front()).entrance;
        ApplyGiantsChamberGrottoReturn();
        assert(gSaveContext.save.entrance == entrance);
        CheckLanding(point, entrance);
        gameInteractor.events.clear();
        SavedMixedPairs()[outside] = 0;
        SavedMixedPairs()[temple] = 0;
    }
    // A complete 0.17-format map with the new bit works through the slot setter and the real save codec.
    options[RO_MIX_ENTRANCE_POOLS] = 17;
    std::vector<std::pair<int32_t, int32_t>> mixed;
    for (size_t row = 0; row < shuffleEntrances.size(); ++row) {
        if (IsMixedEndpoint(2 * row)) {
            const size_t outside = 2 * row;
            const size_t inside = outside == swamp ? temple : outside == temple - 1 ? swamp + 1 : outside + 1;
            mixed.emplace_back(outside, inside);
        }
    }
    SetPairs({}, mixed);
    assert(MixedPartner(swamp) == temple && MixedPartner(temple - 1) == swamp + 1);
    SetDirectedTargets({ { (int32_t)temple, (int32_t)swamp } });
    assert(!DirectedTarget(temple));
    auto& save = gSaveContext.save.shipSaveInfo.rando;
    const json encoded = save;
    save = {};
    from_json(encoded, save);
    assert(MixedPartner(swamp) == temple && MixedPartner(temple - 1) == swamp + 1);
    Passage(temple - 1, swamp + 1);
    namespace ET = Rando::EntranceTracker;
    ET::Options track;
    track.currentScene = SCENE_KAKUSIANA;
    track.currentGrottoRow = swamp / 2;
    const auto lines = ET::BuildRows(TrackedEntrances(), track);
    for (const auto& line : lines) {
        assert(line.current == (line.fromEndpoint == swamp + 1));
    }
    assert(std::count_if(lines.begin(), lines.end(), [](const auto& line) { return line.current; }) == 1);

    // Removing a pair must invalidate the entire mixed map.
    mixed.pop_back();
    SetPairs({}, mixed);
    assert(std::all_of(std::begin(save.mixedEntrancePairs), std::end(save.mixedEntrancePairs),
                       [](u16 value) { return value == 0; }));
    std::cout << doorPairs << " grotto/door pairs, " << landPairs << " grotto/land pairs, " << cavePairs
              << " grotto/grotto pairs in both directions, " << rejections << " rejected pair attempts ("
              << doorRejections << " grotto/door), " << respawns
              << " death/void passages, 4 boss warps, and a mixed save/tracker round trip passed\n";
}
} // namespace MixedGrottoTest
