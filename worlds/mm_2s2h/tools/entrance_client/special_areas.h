#pragma once
// Shuffle Special Areas against the fork's real EntranceShuffle.cpp (included by main.cpp): every pairing of the six
// areas, both directions, every scene form of the outside (cleared swamp) and of the entrance (cleared group, layer
// bits), real exits against void outs, deaths, grotto returns and warp songs, cutscenes, the void-out point, with and
// without the other entrance options. Then the real 0.8 slot data (test/data): rows appended in 0.9 stay vanilla.
#include "special_graph.h"

using namespace Rando::EntranceShuffle;

inline std::vector<size_t> SpecialRows() {
    std::vector<size_t> rows;
    for (size_t i = 0; i < shuffleEntrances.size(); ++i) {
        if (shuffleEntrances[i].option == RO_SHUFFLE_SPECIAL_AREAS) {
            rows.push_back(i);
        }
    }
    return rows;
}

// Entrance values a scene's exit list may hold for this place: base, cleared/spring group and every layer.
inline std::vector<u16> EntranceForms(u16 entrance) {
    std::vector<u16> forms;
    for (u16 base : { entrance, OverworldTest::SpringForm(entrance) }) {
        for (u16 layer = 0; layer < 16; ++layer) {
            forms.push_back(u16(base | layer));
        }
    }
    return forms;
}

inline void TestSpecialAreas() {
    auto rows = SpecialRows();
    assert(rows.size() == generatedSpecial.size() && rows.size() == 6);
    for (size_t i = 0; i < rows.size(); ++i) {
        assert(int(rows[i]) == generatedSpecial[i][0] && shuffleEntrances[rows[i]].into == generatedSpecial[i][1]);
    }
    long cases = 0, redirected = 0;
    std::set<std::pair<size_t, size_t>> covered;
    // Bit 0: the other entrance options on, each with a shift of its own; special areas must not notice them.
    for (int others = 0; others < 2; ++others) {
        for (size_t shift = 0; shift < rows.size(); ++shift) {
            std::map<size_t, size_t> behind;
            std::vector<std::pair<int32_t, int32_t>> pairs;
            for (size_t i = 0; i < rows.size(); ++i) {
                behind[rows[i]] = rows[(i + shift) % rows.size()];
                pairs.emplace_back(shuffleEntrances[rows[i]].into, shuffleEntrances[behind[rows[i]]].into);
            }
            if (others) {
                auto grottos = GrottoShift(shift);
                auto interiors = InteriorShift(shift);
                pairs.insert(pairs.end(), grottos.begin(), grottos.end());
                pairs.insert(pairs.end(), interiors.begin(), interiors.end());
            }
            for (int on = 0; on < 2; ++on) {
                gSaveContext = {};
                std::fill(std::begin(options), std::end(options), 0);
                options[RO_SHUFFLE_SPECIAL_AREAS] = on;
                options[RO_SHUFFLE_DUNGEON_ENTRANCES] = options[RO_SHUFFLE_BOSS_ENTRANCES] = others;
                options[RO_SHUFFLE_GROTTOS] = options[RO_SHUFFLE_INTERIORS] = options[RO_SHUFFLE_OVERWORLD] = others;
                SetPairs(pairs);
                for (auto [door, inside] : behind) {
                    const ShuffleEntrance& d = shuffleEntrances[door];
                    const ShuffleEntrance& x = shuffleEntrances[inside];
                    for (int flag : { 0, -2, 1, -1, -6, 4, -3, -7 }) {
                        for (int cutscene : { 0, 1 }) {
                            bool moved = on && door != inside && (flag == 0 || flag == -2) && cutscene == 0;
                            // Into door A, from A's outside in any of its scene forms: area X's entrance.
                            for (s16 scene : OverworldTest::SceneForms(d.outsideScene)) {
                                for (u16 form : EntranceForms(d.into)) {
                                    gSaveContext.respawnFlag = flag;
                                    gSaveContext.nextCutsceneIndex = 0xFFF1;
                                    gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = form;
                                    playState = { form, scene, 0 };
                                    playState.csCtx.state = cutscene;
                                    OnBeforeTransitionSetup();
                                    u16 expected = moved ? x.into : form;
                                    assert(playState.nextEntrance == expected);
                                    assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == expected);
                                    assert(gSaveContext.nextCutsceneIndex == (moved ? 0xFFEF : 0xFFF1));
                                    redirected += moved;
                                    ++cases;
                                }
                            }
                            // X's own way out: in front of door A, as a base entrance (Play_Init picks the variant).
                            for (u16 form : EntranceForms(x.outOf)) {
                                gSaveContext.respawnFlag = flag;
                                gSaveContext.nextCutsceneIndex = 0xFFF1;
                                gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = 0;
                                playState = { form, x.insideScene, 0 };
                                playState.csCtx.state = cutscene;
                                OnBeforeTransitionSetup();
                                assert(playState.nextEntrance == (moved ? d.outOf : form));
                                if (moved) {
                                    for (int cleared = 0; cleared < 4; ++cleared) {
                                        assert(OverworldTest::LoadedGroup(playState.nextEntrance, cleared) ==
                                               OverworldTest::LoadedGroup(d.outOf, cleared));
                                    }
                                }
                                assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == 0);
                                redirected += moved;
                                ++cases;
                            }
                        }
                    }
                    // A void out or death inside X reloads X's entrance from X's scene: never redirected again.
                    for (int flag : { 0, 1, -1 }) {
                        gSaveContext.respawnFlag = flag;
                        playState = { x.into, x.insideScene, 0 };
                        OnBeforeTransitionSetup();
                        assert(playState.nextEntrance == x.into);
                        ++cases;
                    }
                    // Songs and owls, from inside X and from A's outside, lead to no table entrance.
                    for (unsigned warp : songAndOwlEntrances) {
                        for (s16 scene : { x.insideScene, d.outsideScene }) {
                            gSaveContext.respawnFlag = 0;
                            playState = { u16(warp), scene, 0 };
                            OnBeforeTransitionSetup();
                            assert(playState.nextEntrance == warp);
                            ++cases;
                        }
                    }
                    if (on) {
                        covered.insert({ door, inside });
                    }
                }
            }
        }
    }
    assert(covered.size() == rows.size() * rows.size());
    // Pairs with another option's row are refused (identity), whatever slot data says.
    gSaveContext = {};
    std::fill(std::begin(options), std::end(options), 0);
    options[RO_SHUFFLE_SPECIAL_AREAS] = options[RO_SHUFFLE_INTERIORS] = options[RO_SHUFFLE_DUNGEON_ENTRANCES] = 1;
    SetPairs({ { shuffleEntrances[rows[0]].into, shuffleEntrances[0].into },
               { shuffleEntrances[0].into, shuffleEntrances[rows[1]].into },
               { shuffleEntrances[rows[2]].into, shuffleEntrances[InteriorTest::RowsOf(INTERIOR_ALWAYS_OPEN)[0]].into } });
    for (size_t row : { rows[0], size_t(0), rows[2] }) {
        assert(gSaveContext.save.shipSaveInfo.rando.entrancePairs[row] == 0);
        ++cases;
    }
    std::cout << cases << " special-area cases (" << covered.size() << " pairings, " << redirected
              << " redirected) passed\n";
}

// Real 0.8 slot data: the 0.8 rows keep their pairing, every row appended in 0.9 stays vanilla although its option
// (Shuffle Overworld, Shuffle Interiors) is on, and Shuffle Special Areas, unknown to 0.8, is off.
inline void TestLegacy08() {
    auto rowOf = [](int32_t into) {
        for (size_t row = 0; row < shuffleEntrances.size(); ++row) {
            if (shuffleEntrances[row].into == into) {
                return row;
            }
        }
        assert(false);
        return size_t(0);
    };
    long cases = 0;
    assert(shuffleEntrances.size() == 158 && rows08 == 134);
    for (const LegacySlot& slot : legacy08) {
        gSaveContext = {};
        std::fill(std::begin(options), std::end(options), 0);
        for (int option : slot.options) {
            options[option] = 1;
        }
        assert(options[RO_SHUFFLE_OVERWORLD] && !options[RO_SHUFFLE_SPECIAL_AREAS]);
        SetPairs(slot.pairs);
        const u16* saved = gSaveContext.save.shipSaveInfo.rando.entrancePairs;
        for (size_t row = rows08; row < 512; ++row) {
            assert(saved[row] == 0);
        }
        std::map<size_t, size_t> behind;
        for (auto [doorInto, insideInto] : slot.pairs) {
            size_t door = rowOf(doorInto), inside = rowOf(insideInto);
            assert(door < rows08 && inside < rows08 && saved[door] == inside + 1);
            behind[door] = inside;
        }
        // Through every 0.8 row of an option that is on: where the 0.8 pairing leads.
        for (auto [door, inside] : behind) {
            const ShuffleEntrance& d = shuffleEntrances[door];
            if (d.option != RO_SHUFFLE_OVERWORLD && d.option != RO_SHUFFLE_INTERIORS) {
                continue;
            }
            gSaveContext.respawnFlag = 0;
            playState = { d.into, d.outsideScene, 0 };
            OnBeforeTransitionSetup();
            assert(playState.nextEntrance == shuffleEntrances[inside].into);
            ++cases;
        }
        // Through every row appended in 0.9, both directions: vanilla.
        for (size_t row = rows08; row < shuffleEntrances.size(); ++row) {
            const ShuffleEntrance& r = shuffleEntrances[row];
            gSaveContext.respawnFlag = 0;
            playState = { r.into, r.outsideScene, 0 };
            OnBeforeTransitionSetup();
            assert(playState.nextEntrance == r.into);
            playState = { r.outOf, r.insideScene, 0 };
            OnBeforeTransitionSetup();
            assert(playState.nextEntrance == r.outOf);
            cases += 2;
        }
    }
    std::cout << cases << " legacy 0.8 cases (" << legacy08.size() << " real slot data files) passed\n";
}
