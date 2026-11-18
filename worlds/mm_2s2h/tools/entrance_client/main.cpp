#include "Rando/MiscBehavior/EntranceShuffle.cpp"
#include "Rando/MiscBehavior/DoorKeys.cpp"
#include <array>
#include <set>
#include "door_keys_graph.h"
#include "grotto_graph.h"
#include "grottos.h"
#include "interior_graph.h"
#include "interiors.h"
#include "overworld.h"
#include "special_areas.h"
#include "mixed.h"

using namespace Rando::EntranceShuffle;

// Door keys: whether a door opens depends on nothing but the entrance it leads to (DoorKeys::IsEntranceLocked).
// Every 16-bit entrance value is tried: option off, then on with no key and with each key alone.
static void TestDoorKeys() {
    namespace DK = Rando::DoorKeys;
    long cases = 0;
    std::set<unsigned> listed;
    assert(DK::doorKeys.size() == doorKeyInto.size());
    for (size_t i = 0; i < DK::doorKeys.size(); ++i) {
        // The compiled table is the one the apworld was generated from: same keys and entrances, same order.
        assert(DK::doorKeys[i].key == doorKeyInto[i].first);
        assert(std::vector<unsigned>(DK::doorKeys[i].into.begin(), DK::doorKeys[i].into.end()) == doorKeyInto[i].second);
        listed.insert(doorKeyInto[i].second.begin(), doorKeyInto[i].second.end());
    }
    std::fill(std::begin(options), std::end(options), 0);
    std::fill(std::begin(randoInf), std::end(randoInf), false);
    for (unsigned e = 0; e <= 0xFFFF; ++e, ++cases) {
        assert(!DK::IsEntranceLocked(u16(e))); // option off: no door is ever locked
    }
    options[RO_SHUFFLE_DOOR_KEYS] = 1;
    // zora = 0: a seed from before apworld 0.7 (no Zora Hall Rooms key in its pool), those doors never lock.
    for (int zora = 0; zora < 2; ++zora)
    for (int have = -1; have < int(DK::doorKeys.size()); ++have) { // -1: no key at all, else only key `have`
        std::fill(std::begin(randoInf), std::end(randoInf), false);
        randoInf[RANDO_INF_DOOR_KEYS_ZORA_HALL] = zora != 0;
        if (have >= 0) {
            randoInf[DOOR_KEY_RI_TO_RANDO_INF(DK::doorKeys[have].key)] = true;
        }
        for (unsigned e = 0; e <= 0xFFFF; ++e, ++cases) {
            const DK::DoorKey* row = DK::FindByEntrance(u16(e));
            bool isInto = listed.count(e & 0xFFF0) != 0; // the scene layer (bits 0-3) does not matter
            assert((row != nullptr) == isInto);
            bool zoraRow = row != nullptr && row->key == RI_DOOR_KEY_ZORA_HALL_ROOMS;
            bool expected = isInto && (have < 0 || row != &DK::doorKeys[have]) && (zora || !zoraRow);
            assert(DK::IsEntranceLocked(u16(e)) == expected);
        }
        for (unsigned out : doorKeyOutward) {
            for (unsigned layer = 0; layer < 16; ++layer, ++cases) {
                assert(!DK::IsEntranceLocked(u16(out | layer))); // a way out is never locked, even without keys
            }
        }
    }
    std::cout << cases << " door-key cases (" << listed.size() << " ways in, " << doorKeyOutward.size()
              << " ways out never locked) passed\n";
}

int main(int argc, char** argv) {
    assert(argc == 2);
    TestDoorKeys();
    std::array<int, 4> temples{ 0, 1, 2, 3 };
    const u16 exterior[] = { ENTRANCE(WOODFALL, 1), ENTRANCE(SNOWHEAD, 1), ENTRANCE(ZORA_CAPE, 7),
                             ENTRANCE(STONE_TOWER_INVERTED, 1) };
    const unsigned clearFlags[] = { 3, 4, 8, 16 };
    int permutations = 0, warps = 0, respawns = 0, entries = 0;
    // Bit 2: Shuffle Grottos on as well (a grotto shift per permutation), bit 3: Shuffle Interiors (an interior shift);
    // temples and bosses must not notice.
    for (int mode = 0; mode < 16; ++mode) {
        do {
            std::array<int, 4> bosses{ 0, 1, 2, 3 };
            do {
                gSaveContext = {};
                std::fill(std::begin(options), std::end(options), 0);
                options[RO_SHUFFLE_DUNGEON_ENTRANCES] = (mode & 1) != 0;
                options[RO_SHUFFLE_BOSS_ENTRANCES] = (mode & 2) != 0;
                options[RO_SHUFFLE_GROTTOS] = (mode & 4) != 0;
                options[RO_SHUFFLE_INTERIORS] = (mode & 8) != 0;
                std::vector<std::pair<int32_t, int32_t>> pairs = GrottoShift(permutations % 26);
                auto interiors = InteriorShift(permutations % 17);
                pairs.insert(pairs.end(), interiors.begin(), interiors.end());
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
                            for (int mode : { RESPAWN_MODE_DOWN, RESPAWN_MODE_TOP }) {
                                const auto& respawn = gSaveContext.respawn[mode];
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
                        for (int mode : { RESPAWN_MODE_DOWN, RESPAWN_MODE_TOP }) {
                            const auto& respawn = gSaveContext.respawn[mode];
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
    for (int i = 2; i < 512; ++i) assert(gSaveContext.save.shipSaveInfo.rando.entrancePairs[i] == 0);
    std::cout << permutations << " option/permutation cases, " << entries << " entries, " << warps
              << " warps, " << respawns << " respawn checks passed\n";
    TestLegacySave();
    TestGrottos();
    TestInteriors();
    OverworldTest::TestOverworld();
    TestSpecialAreas();
    TestLegacy08();
    TestMixedEntrances();
    TestLegacy09(argv[1]);
}
