#pragma once
// Shuffle Grottos and the widened save field, against the fork's real EntranceShuffle.cpp (included by main.cpp).
#include <map>
#include <random>
#include <set>
#include "grown_array.h"

namespace GrottoTest {
using namespace Rando::EntranceShuffle;

inline std::vector<size_t> GrottoRows() {
    std::vector<size_t> rows;
    for (size_t i = 0; i < shuffleEntrances.size(); ++i) {
        if (shuffleEntrances[i].option == RO_SHUFFLE_GROTTOS) {
            rows.push_back(i);
        }
    }
    return rows;
}

inline std::vector<std::pair<int32_t, int32_t>> PairsOf(const std::vector<size_t>& insideOf) {
    std::vector<std::pair<int32_t, int32_t>> pairs;
    auto rows = GrottoRows();
    for (size_t i = 0; i < rows.size(); ++i) {
        pairs.emplace_back(shuffleEntrances[rows[i]].into, shuffleEntrances[insideOf[i]].into);
    }
    return pairs;
}

// The hole of grotto row i leads into grotto row i + shift; the 26 shifts together pair every hole with every grotto.
inline std::vector<std::pair<int32_t, int32_t>> Shift(size_t shift) {
    auto rows = GrottoRows();
    std::vector<size_t> insideOf;
    for (size_t i = 0; i < rows.size(); ++i) {
        insideOf.push_back(rows[(i + shift) % rows.size()]);
    }
    return PairsOf(insideOf);
}
} // namespace GrottoTest

inline std::vector<std::pair<int32_t, int32_t>> GrottoShift(size_t shift) {
    return GrottoTest::Shift(shift);
}

// A save from apworld 0.3-0.5 stored u8 entrancePairs[32]. The real GetGrownArray loads it into u16[512] unchanged,
// so its temple and boss pairs resolve to the same rows as before.
inline void TestLegacySave() {
    using namespace Rando::EntranceShuffle;
    std::vector<int> old(32, 0);
    const int temples[] = { 3, 1, 4, 2 }, bosses[] = { 6, 8, 5, 7 }; // row + 1
    for (int i = 0; i < 4; ++i) {
        old[i] = temples[i];
        old[i + 4] = bosses[i];
    }
    gSaveContext = {};
    std::fill(std::begin(options), std::end(options), 0);
    options[RO_SHUFFLE_DUNGEON_ENTRANCES] = options[RO_SHUFFLE_BOSS_ENTRANCES] = 1;
    GetGrownArray(json(old), gSaveContext.save.shipSaveInfo.rando.entrancePairs);
    const u16* saved = gSaveContext.save.shipSaveInfo.rando.entrancePairs;
    for (int i = 0; i < 512; ++i) {
        assert(saved[i] == (i < 32 ? old[i] : 0));
    }
    int checks = 0;
    for (int i = 0; i < 8; ++i, ++checks) {
        const auto& row = shuffleEntrances[i];
        playState = { row.into, row.outsideScene, 0 };
        OnBeforeTransitionSetup();
        assert(playState.nextEntrance == shuffleEntrances[old[i] - 1].into);
    }
    // Round trip of the new format: SetPairs writes row + 1 as u16, and it loads back the same.
    SetPairs(GrottoShift(5));
    json written = gSaveContext.save.shipSaveInfo.rando.entrancePairs;
    u16 reloaded[512];
    GetGrownArray(written, reloaded);
    assert(std::equal(std::begin(reloaded), std::end(reloaded), saved));
    std::cout << "legacy save: 32 u8 pairs load into u16[512], " << checks << " temple/boss doors unchanged\n";
}

inline void TestGrottos() {
    using namespace Rando::EntranceShuffle;
    using namespace GrottoTest;
    auto rows = GrottoRows();
    assert(rows.size() == 26 && grottoEntrances.size() == 26);
    assert(shuffleEntrances.size() == generatedRows && shuffleEntrances.size() <= 512);
    // The compiled table is the one the apworld was generated from.
    assert(generatedGrottos.size() == rows.size());
    for (const auto& g : generatedGrottos) {
        const GrottoEntrance* grotto = GrottoOf(size_t(g[0]));
        assert(grotto && shuffleEntrances[g[0]].into == g[1] && grotto->gameEntrance == g[2] && grotto->data == g[3] &&
               (grotto->kind == GROTTO_SCENE_EXIT) == (g[4] != 0));
    }

    long cases = 0;
    std::set<std::pair<size_t, size_t>> covered;
    std::mt19937 rng(20261117);
    std::vector<std::vector<size_t>> perms;
    for (size_t shift = 0; shift < rows.size(); ++shift) {
        std::vector<size_t> p;
        for (size_t i = 0; i < rows.size(); ++i) p.push_back(rows[(i + shift) % rows.size()]);
        perms.push_back(p);
    }
    for (int r = 0; r < 200; ++r) {
        auto p = rows;
        std::shuffle(p.begin(), p.end(), rng);
        perms.push_back(p);
    }

    for (int otherModes = 0; otherModes < 4; ++otherModes) {
        for (size_t pi = 0; pi < perms.size(); ++pi) {
            const auto& perm = perms[pi];
            for (int on = 0; on < 2; ++on) {
                std::fill(std::begin(options), std::end(options), 0);
                options[RO_SHUFFLE_GROTTOS] = on;
                options[RO_SHUFFLE_DUNGEON_ENTRANCES] = (otherModes & 1) != 0;
                options[RO_SHUFFLE_BOSS_ENTRANCES] = (otherModes & 2) != 0;
                gSaveContext = {};
                auto pairs = PairsOf(perm);
                // Temples and bosses: some fixed non-identity pairing alongside.
                pairs.emplace_back(shuffleEntrances[0].into, shuffleEntrances[2].into);
                pairs.emplace_back(shuffleEntrances[2].into, shuffleEntrances[0].into);
                pairs.emplace_back(shuffleEntrances[4].into, shuffleEntrances[7].into);
                pairs.emplace_back(shuffleEntrances[7].into, shuffleEntrances[4].into);
                SetPairs(pairs);

                for (size_t i = 0; i < rows.size(); ++i) {
                    size_t a = rows[i], x = on ? perm[i] : a;
                    const ShuffleEntrance& A = shuffleEntrances[a];
                    const GrottoEntrance& GA = *GrottoOf(a);
                    const GrottoEntrance& GX = *GrottoOf(x);
                    if (on) covered.insert({ a, x });
                    // The hole's scene: the logic's one, or another variant (spring/winter, cleared swamp).
                    for (s16 scene : { A.outsideScene, s16(SCENE_10YUKIYAMANOMURA2) }) {
                        // Outside, in front of hole A. Entered the scene at `outside`, standing at position 100 + i.
                        u16 outside = u16(0x6000 + 0x10 * i);
                        gSaveContext.save.entrance = outside;
                        gSaveContext.respawnFlag = 0;
                        gSaveContext.respawn[RESPAWN_MODE_DOWN] = { u16(outside & ~0x1F0), 1, 50 };
                        gSaveContext.respawn[RESPAWN_MODE_TOP] = gSaveContext.respawn[RESPAWN_MODE_DOWN];
                        playState = { GA.gameEntrance, scene, int(100 + i) };
                        if (GA.kind == GROTTO_HOLE) { // z_door_ana.c:144-150
                            Play_SetupRespawnPoint(&playState, RESPAWN_MODE_UNK_3, 0);
                            gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = s8(GA.data);
                        } else {
                            gSaveContext.respawn[RESPAWN_MODE_UNK_3] = { 0x7770, 0x55, 7 }; // stale, from long ago
                        }
                        RespawnData holeRespawn = gSaveContext.respawn[RESPAWN_MODE_UNK_3];
                        OnBeforeTransitionSetup();
                        ++cases;
                        // Into grotto X: X's real entrance and X's grotto data, always as a pair.
                        assert(playState.nextEntrance == GX.gameEntrance);
                        const RespawnData& g = gSaveContext.respawn[RESPAWN_MODE_UNK_3];
                        if (GX.data >= 0) assert(u8(g.data) == u8(GX.data));
                        if (GA.kind == GROTTO_HOLE) {
                            // The way back stays at hole A (entrance and position as DoorAna stored them).
                            assert(g.entrance == holeRespawn.entrance && g.position == holeRespawn.position);
                        } else if (x != a) {
                            assert(g.entrance == u16(outside & ~0x1F0) && g.position == int(100 + i));
                        }
                        assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == u16(outside & ~0x1F0));

                        // Load grotto X.
                        RespawnData grottoRespawn = g;
                        gSaveContext.save.entrance = playState.nextEntrance;
                        playState.sceneId = SCENE_KAKUSIANA;
                        // The check tracker (and every grotto check lookup) sees X: entrance and data are X's.
                        s32 logic = 0;
                        bool known = LogicGrottoEntrance(gSaveContext.save.entrance, &logic);
                        assert(known == (on != 0));
                        if (on) assert(logic == shuffleEntrances[x].into);
                        ++cases;

                        // Death (TOP) and void out (DOWN) inside X: the scene of hole A, never another grotto.
                        for (int flag : { -1, -2 }) {
                            gSaveContext.respawnFlag = flag;
                            playState.nextEntrance = gSaveContext.respawn[flag == -1 ? RESPAWN_MODE_TOP : RESPAWN_MODE_DOWN].entrance;
                            OnBeforeTransitionSetup();
                            assert(playState.nextEntrance == u16(outside & ~0x1F0) && gSaveContext.respawnFlag == flag);
                            ++cases;
                        }
                        // A death that reloads the grotto itself is not redirected again.
                        gSaveContext.respawnFlag = -1;
                        playState.nextEntrance = GX.gameEntrance;
                        OnBeforeTransitionSetup();
                        assert(playState.nextEntrance == GX.gameEntrance);
                        ++cases;

                        // Pause save and reload inside X: save.entrance and all respawn records are restored.
                        SaveContext saved = gSaveContext;
                        gSaveContext = {};
                        gSaveContext = saved;
                        assert(LogicGrottoEntrance(gSaveContext.save.entrance, &logic) == (on != 0));
                        assert(gSaveContext.respawn[RESPAWN_MODE_UNK_3].entrance == grottoRespawn.entrance);

                        // X's own way out: back to hole A.
                        if (GX.kind == GROTTO_HOLE) {
                            playState.nextEntrance = gSaveContext.respawn[RESPAWN_MODE_UNK_3].entrance;
                            gSaveContext.respawnFlag = 4;
                        } else {
                            playState.nextEntrance = shuffleEntrances[x].outOf;
                            gSaveContext.respawnFlag = 0;
                        }
                        OnBeforeTransitionSetup();
                        ++cases;
                        if (GA.kind == GROTTO_HOLE) {
                            assert(playState.nextEntrance == holeRespawn.entrance && gSaveContext.respawnFlag == 4);
                            assert(gSaveContext.respawn[RESPAWN_MODE_UNK_3].position == holeRespawn.position);
                        } else {
                            assert(playState.nextEntrance == A.outOf && gSaveContext.respawnFlag == 0);
                        }
                    }
                }
                // Temples and bosses keep their own pairing next to the grottos.
                for (int d : { 0, 2, 4, 7 }) {
                    const auto& row = shuffleEntrances[d];
                    playState = { row.into, row.outsideScene, 0 };
                    OnBeforeTransitionSetup();
                    bool shuffled = d < 4 ? options[RO_SHUFFLE_DUNGEON_ENTRANCES] : options[RO_SHUFFLE_BOSS_ENTRANCES];
                    int target = !shuffled ? d : (d == 0 ? 2 : d == 2 ? 0 : d == 4 ? 7 : 4);
                    assert(playState.nextEntrance == shuffleEntrances[target].into);
                    ++cases;
                }
            }
        }
    }
    // A chest-grotto hole with data the table does not know stays vanilla.
    std::fill(std::begin(options), std::end(options), 0);
    options[RO_SHUFFLE_GROTTOS] = 1;
    SetPairs(GrottoShift(1));
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x11;
    playState = { ENTRANCE(GROTTOS, 4), SCENE_00KEIKOKU, 0 };
    OnBeforeTransitionSetup();
    assert(playState.nextEntrance == ENTRANCE(GROTTOS, 4) && gSaveContext.respawn[RESPAWN_MODE_UNK_3].data == 0x11);
    assert(covered.size() == rows.size() * rows.size());
    std::cout << cases << " grotto cases passed; all " << covered.size() << " hole/grotto pairings covered\n";
}
