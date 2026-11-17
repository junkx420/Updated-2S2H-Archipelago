#pragma once
// Shuffle Interiors against the fork's real EntranceShuffle.cpp (included by main.cpp): every pairing of each hours
// class, both directions, every entrance layer, cleared/spring exteriors, void outs, songs and owls.
#include <map>
#include <set>

namespace InteriorTest {
using namespace Rando::EntranceShuffle;

inline int HoursOfRow(size_t row) {
    for (const InteriorEntrance& interior : interiorEntrances) {
        if (interior.into == shuffleEntrances[row].into) {
            return interior.hours;
        }
    }
    return -1;
}

inline std::vector<size_t> RowsOf(int hours) {
    std::vector<size_t> rows;
    for (size_t i = 0; i < shuffleEntrances.size(); ++i) {
        if (shuffleEntrances[i].option == RO_SHUFFLE_INTERIORS && HoursOfRow(i) == hours) {
            rows.push_back(i);
        }
    }
    return rows;
}

// Door i of each moving class leads into building i + shift of the same class; OWN_HOURS rows stay.
inline std::map<size_t, size_t> Behind(size_t shift) {
    std::map<size_t, size_t> behind;
    for (int hours : { INTERIOR_ALWAYS_OPEN, INTERIOR_DAYTIME, INTERIOR_OWN_HOURS }) {
        auto rows = RowsOf(hours);
        for (size_t i = 0; i < rows.size(); ++i) {
            behind[rows[i]] = hours == INTERIOR_OWN_HOURS ? rows[i] : rows[(i + shift) % rows.size()];
        }
    }
    return behind;
}

inline std::vector<std::pair<int32_t, int32_t>> PairsOf(const std::map<size_t, size_t>& behind) {
    std::vector<std::pair<int32_t, int32_t>> pairs;
    for (auto [door, inside] : behind) {
        pairs.emplace_back(shuffleEntrances[door].into, shuffleEntrances[inside].into);
    }
    return pairs;
}

// The cleared swamp and spring village are other scenes with the same entrances (z_play.c Play_Init).
inline std::vector<s16> ExteriorScenes(s16 scene) {
    if (scene == SCENE_20SICHITAI) {
        return { scene, SCENE_20SICHITAI2 };
    }
    if (scene == SCENE_10YUKIYAMANOMURA) {
        return { scene, SCENE_10YUKIYAMANOMURA2 };
    }
    return { scene };
}
} // namespace InteriorTest

inline std::vector<std::pair<int32_t, int32_t>> InteriorShift(size_t shift) {
    return InteriorTest::PairsOf(InteriorTest::Behind(shift));
}

inline void TestInteriors() {
    using namespace Rando::EntranceShuffle;
    using namespace InteriorTest;
    // The compiled table is the one the apworld was generated from.
    std::vector<size_t> rows;
    for (size_t i = 0; i < shuffleEntrances.size(); ++i) {
        if (shuffleEntrances[i].option == RO_SHUFFLE_INTERIORS) {
            rows.push_back(i);
        }
    }
    assert(rows.size() == generatedInteriors.size() && interiorEntrances.size() == rows.size());
    for (size_t i = 0; i < rows.size(); ++i) {
        assert(int(rows[i]) == generatedInteriors[i][0] && shuffleEntrances[rows[i]].into == generatedInteriors[i][1]);
        assert(HoursOfRow(rows[i]) == generatedInteriors[i][2]);
    }
    auto always = RowsOf(INTERIOR_ALWAYS_OPEN), day = RowsOf(INTERIOR_DAYTIME), own = RowsOf(INTERIOR_OWN_HOURS);
    assert(always.size() == 17 && day.size() == 9 && own.size() == 6);

    long cases = 0;
    std::set<std::pair<size_t, size_t>> covered;
    const u16 otherCutscene = 0xFFF1;
    // Bits 0-2: temples, bosses, grottos as well (identity or a grotto shift); interiors must not notice.
    for (int mode = 0; mode < 8; ++mode) {
        for (size_t shift = 0; shift < always.size(); ++shift) {
            gSaveContext = {};
            std::fill(std::begin(options), std::end(options), 0);
            options[RO_SHUFFLE_INTERIORS] = 1;
            options[RO_SHUFFLE_DUNGEON_ENTRANCES] = (mode & 1) != 0;
            options[RO_SHUFFLE_BOSS_ENTRANCES] = (mode & 2) != 0;
            options[RO_SHUFFLE_GROTTOS] = (mode & 4) != 0;
            auto behind = Behind(shift);
            auto pairs = PairsOf(behind);
            auto grottos = GrottoShift(shift % 26);
            pairs.insert(pairs.end(), grottos.begin(), grottos.end());
            SetPairs(pairs);
            for (auto [door, inside] : behind) {
                const ShuffleEntrance& d = shuffleEntrances[door];
                const ShuffleEntrance& x = shuffleEntrances[inside];
                bool moved = door != inside;
                for (u16 layer = 0; layer < 16; ++layer) {
                    for (s16 scene : ExteriorScenes(d.outsideScene)) {
                        // Into door A: building X's base entrance; the void-out point an exit recorded moves along.
                        u16 next = u16(d.into | layer);
                        gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = next;
                        gSaveContext.nextCutsceneIndex = otherCutscene;
                        playState = { next, scene, 0 };
                        OnBeforeTransitionSetup();
                        u16 expected = moved ? x.into : next;
                        assert(playState.nextEntrance == expected);
                        assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == expected);
                        assert(gSaveContext.nextCutsceneIndex == (moved ? 0xFFEF : otherCutscene));
                        ++cases;
                    }
                    // X's own way out: in front of door A
                    u16 out = u16(x.outOf | layer);
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = 0;
                    playState = { out, x.insideScene, 0 };
                    OnBeforeTransitionSetup();
                    assert(playState.nextEntrance == (moved ? d.outOf : out));
                    assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == 0);
                    ++cases;
                }
                // Void out or death inside X respawns at X's entrance from X's scene: never redirected again.
                playState = { x.into, x.insideScene, 0 };
                OnBeforeTransitionSetup();
                assert(playState.nextEntrance == x.into);
                // Song of Time and Soaring from inside X lead to no table entrance.
                for (unsigned warp : songAndOwlEntrances) {
                    playState = { u16(warp), x.insideScene, 0 };
                    OnBeforeTransitionSetup();
                    assert(playState.nextEntrance == warp);
                    ++cases;
                }
                covered.insert({ door, inside });
            }
        }
    }
    assert(covered.size() == always.size() * always.size() + day.size() * day.size() + own.size());

    // Option off: saved pairs are ignored.
    std::fill(std::begin(options), std::end(options), 0);
    SetPairs(InteriorShift(1));
    for (size_t row : rows) {
        playState = { shuffleEntrances[row].into, shuffleEntrances[row].outsideScene, 0 };
        OnBeforeTransitionSetup();
        assert(playState.nextEntrance == shuffleEntrances[row].into);
        ++cases;
    }
    // Pairs across classes or with an OWN_HOURS row are refused (identity), whatever slot data says.
    options[RO_SHUFFLE_INTERIORS] = 1;
    SetPairs({ { shuffleEntrances[own[0]].into, shuffleEntrances[own[1]].into },
               { shuffleEntrances[always[0]].into, shuffleEntrances[day[0]].into },
               { shuffleEntrances[day[1]].into, shuffleEntrances[always[1]].into },
               { shuffleEntrances[always[2]].into, shuffleEntrances[own[2]].into } });
    for (size_t row : { own[0], always[0], day[1], always[2] }) {
        assert(gSaveContext.save.shipSaveInfo.rando.entrancePairs[row] == 0);
        playState = { shuffleEntrances[row].into, shuffleEntrances[row].outsideScene, 0 };
        OnBeforeTransitionSetup();
        assert(playState.nextEntrance == shuffleEntrances[row].into);
        ++cases;
    }
    std::cout << cases << " interior cases (" << covered.size() << " door/building pairings, 16 entrance layers, "
              << songAndOwlEntrances.size() << " song/owl entrances) passed\n";
}
