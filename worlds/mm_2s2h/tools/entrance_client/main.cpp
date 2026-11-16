#include "Rando/MiscBehavior/EntranceShuffle.cpp"
#include <array>

using namespace Rando::EntranceShuffle;

int main() {
    std::array<int, 4> temples{ 0, 1, 2, 3 };
    const u16 exterior[] = { ENTRANCE(WOODFALL, 1), ENTRANCE(SNOWHEAD, 1), ENTRANCE(ZORA_CAPE, 7),
                             ENTRANCE(STONE_TOWER_INVERTED, 1) };
    const unsigned clearFlags[] = { 3, 4, 8, 16 };
    int permutations = 0, warps = 0, respawns = 0, entries = 0;
    for (int mode = 0; mode < 4; ++mode) {
        do {
            std::array<int, 4> bosses{ 0, 1, 2, 3 };
            do {
                gSaveContext = {};
                std::fill(std::begin(options), std::end(options), 0);
                options[RO_SHUFFLE_DUNGEON_ENTRANCES] = (mode & 1) != 0;
                options[RO_SHUFFLE_BOSS_ENTRANCES] = (mode & 2) != 0;
                std::vector<std::pair<int32_t, int32_t>> pairs;
                for (int i = 0; i < 4; ++i) {
                    pairs.emplace_back(shuffleEntrances[i].into, shuffleEntrances[temples[i]].into);
                    pairs.emplace_back(shuffleEntrances[i + 4].into, shuffleEntrances[bosses[i] + 4].into);
                }
                SetPairs(pairs);
                for (int i = 0; i < 8; ++i) {
                    const auto& row = shuffleEntrances[i];
                    playState = { row.into, row.outsideScene, 0 };
                    OnBeforeTransitionSetup();
                    int target = i < 4 ? ((mode & 1) ? temples[i] : i) : ((mode & 2) ? bosses[i - 4] + 4 : i);
                    assert(playState.nextEntrance == shuffleEntrances[target].into);
                    ++entries;
                }
                for (int boss = 0; boss < 4; ++boss) {
                    const auto& row = shuffleEntrances[boss + 4];
                    int door = (mode & 2) ? int(std::find(bosses.begin(), bosses.end(), boss) - bosses.begin()) : boss;
                    // The inverted exterior is not one of the four shuffled normal temple doors.
                    int outsideDoor = door < 3 && (mode & 1)
                        ? int(std::find(temples.begin(), temples.end(), door) - temples.begin()) : door;
                    u16 expected = door < 3 ? shuffleEntrances[outsideDoor].outOf : exterior[3];
                    bool moved = door != boss || outsideDoor != door;
                    for (u16 warp : row.bossWarpsTo) {
                        gSaveContext.respawnFlag = 0;
                        for (auto& point : gSaveContext.respawn) point = { row.into, 0, 999 };
                        flags = 0;
                        playState = { warp, row.insideScene, 999 };
                        gSaveContext.nextCutsceneIndex = 0xFFF2;
                        OnBeforeTransitionSetup();
                        if (playState.nextEntrance != (moved ? expected : warp)) {
                            std::cerr << "mode=" << mode << " boss=" << boss << " door=" << door
                                      << " exteriorDoor=" << outsideDoor << " warp=" << warp
                                      << " actual=" << playState.nextEntrance << " expected=" << (moved ? expected : warp) << '\n';
                        }
                        assert(playState.nextEntrance == (moved ? expected : warp));
                        assert(flags == (moved ? clearFlags[boss] : 0));
                        assert(gSaveContext.nextCutsceneIndex == (moved ? 0xFFEF : 0xFFF2));
                        ++warps;
                        if (moved) {
                            // Simulate scene load, then exercise the actual post-spawn hook with a new exterior position.
                            gSaveContext.save.entrance = expected;
                            playState.position = 123;
                            OnSceneInit(0, 0);
                            for (const auto& respawn : gSaveContext.respawn) {
                                assert(respawn.entrance == expected && respawn.position == 123 && respawn.data == 1);
                                ++respawns;
                            }
                        }
                    }
                    // First warp: preserve the Giants' Chamber path and its boss tag (giant/Oath processing).
                    gameInteractor.events = { GIEventTransition{ row.outOf, 0xFFF2, row.insideScene } };
                    playState = { ENTRANCE(GIANTS_CHAMBER, 0), row.insideScene, 999 };
                    OnBeforeTransitionSetup();
                    const auto& queued = std::get<GIEventTransition>(gameInteractor.events[0]);
                    assert(queued.entrance == (moved ? expected : row.outOf));
                    assert(queued.cutsceneIndex == (moved ? 0 : 0xFFF2));
                    assert(queued.transitionType == row.insideScene);
                    assert(playState.nextEntrance == ENTRANCE(GIANTS_CHAMBER, 0));
                    if (moved) {
                        gSaveContext.save.entrance = u16(queued.entrance);
                        playState.position = 456;
                        OnSceneInit(0, 0);
                        for (const auto& respawn : gSaveContext.respawn) {
                            assert(respawn.entrance == expected && respawn.position == 456);
                            ++respawns;
                        }
                    }
                    ++warps;
                    // Death and void: the already resolved boss entrance must not be shuffled a second time.
                    for (int respawnFlag : { -1, 1 }) {
                        gSaveContext.respawnFlag = respawnFlag;
                        playState = { row.into, row.insideScene, 0 };
                        OnBeforeTransitionSetup();
                        assert(playState.nextEntrance == row.into);
                        assert(gSaveContext.respawnFlag == respawnFlag);
                        ++respawns;
                    }
                }
                ++permutations;
            } while (std::next_permutation(bosses.begin(), bosses.end()));
        } while (std::next_permutation(temples.begin(), temples.end()));
    }
    // Legacy save: first four row indices survive, appended rows are zero (= identity).
    std::fill(std::begin(options), std::end(options), 0);
    options[RO_SHUFFLE_DUNGEON_ENTRANCES] = 1;
    SetPairs({ { shuffleEntrances[0].into, shuffleEntrances[1].into },
               { shuffleEntrances[1].into, shuffleEntrances[0].into } });
    for (int i = 4; i < 32; ++i) assert(gSaveContext.save.shipSaveInfo.rando.entrancePairs[i] == 0);
    std::cout << permutations << " option/permutation cases, " << entries << " entries, " << warps
              << " warps, " << respawns << " respawn checks passed\n";
}
