#pragma once
// Shuffle Overworld against the fork's real EntranceShuffle.cpp (included by main.cpp): every coupled pairing of two
// exits, every scene variant of the exit's scene, every form of the entrance (winter/spring, poisoned/cleared, layer
// bits), every clear-flag state, real exits against void outs, deaths, grotto returns and warp songs, cutscenes,
// Epona and the void-out point, with and without the other entrance options.
#include "overworld_graph.h"

namespace OverworldTest {
using namespace Rando::EntranceShuffle;

inline int SceneGroup(u16 entrance) {
    return entrance >> 9;
}

inline u16 WithGroup(u16 entrance, int group) {
    return u16((group << 9) | (entrance & 0x1FF));
}

// The spring/cleared form of an entrance, which a spring or cleared scene's exit list may hold (the exit lists are in
// the assets, so both forms are tested).
inline u16 SpringForm(u16 entrance) {
    switch (SceneGroup(entrance)) {
        case ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER:
            return WithGroup(entrance, ENTR_SCENE_MOUNTAIN_VILLAGE_SPRING);
        case ENTR_SCENE_GORON_VILLAGE_WINTER:
            return WithGroup(entrance, ENTR_SCENE_GORON_VILLAGE_SPRING);
        case ENTR_SCENE_PATH_TO_GORON_VILLAGE_WINTER:
            return WithGroup(entrance, ENTR_SCENE_PATH_TO_GORON_VILLAGE_SPRING);
        case ENTR_SCENE_SOUTHERN_SWAMP_POISONED:
            return WithGroup(entrance, ENTR_SCENE_SOUTHERN_SWAMP_CLEARED);
        default:
            return entrance;
    }
}

// Model of z_play.c:2264-2283 (Play_Init): the entrance group loaded under the clear flags (bit 0 Woodfall, bit 1
// Snowhead). Layers are chosen by nextCutsceneIndex, which the redirect resets for the target.
inline int LoadedGroup(u16 entrance, int cleared) {
    int group = SceneGroup(entrance);
    if ((cleared & 2) && SpringForm(entrance) != entrance && group != ENTR_SCENE_SOUTHERN_SWAMP_POISONED) {
        return SceneGroup(SpringForm(entrance));
    }
    if ((cleared & 1) && group == ENTR_SCENE_SOUTHERN_SWAMP_POISONED) {
        return ENTR_SCENE_SOUTHERN_SWAMP_CLEARED;
    }
    return group;
}

// The scenes an exit's scene may be loaded as; Play_GetOriginalSceneId maps them back (z_play.c:1975).
inline std::vector<s16> SceneForms(s16 base) {
    switch (base) {
        case SCENE_20SICHITAI:
            return { base, SCENE_20SICHITAI2 };
        case SCENE_10YUKIYAMANOMURA:
            return { base, SCENE_10YUKIYAMANOMURA2 };
        case SCENE_11GORONNOSATO:
            return { base, SCENE_11GORONNOSATO2 };
        case SCENE_17SETUGEN:
            return { base, SCENE_17SETUGEN2 };
        case SCENE_F40:
            return { base, SCENE_F41 };
        default:
            return { base };
    }
}

inline void TestOverworld() {
    std::vector<size_t> rows;
    std::map<size_t, size_t> opposite;
    for (size_t i = 0; i < shuffleEntrances.size(); ++i) {
        if (shuffleEntrances[i].option == RO_SHUFFLE_OVERWORLD) {
            rows.push_back(i);
        }
    }
    assert(rows.size() == generatedOverworld.size() && rows.size() == 68);
    for (size_t n = 0; n < rows.size(); ++n) {
        const auto& generated = generatedOverworld[n];
        assert(size_t(generated[0]) == rows[n] && shuffleEntrances[rows[n]].into == generated[1]);
        opposite[rows[n]] = size_t(generated[2]);
        assert(shuffleEntrances[generated[2]].outOf == shuffleEntrances[rows[n]].into);
    }
    Player player;
    Actor horse;
    horse.world.pos = { 10, 20, 30 };
    horse.shape.rot.y = 77;
    long cases = 0, redirected = 0, kept = 0, loads = 0;
    for (int others = 0; others < 2; ++others) {
        for (size_t exitRow : rows) {
            for (size_t target : rows) {
                const auto& exit = shuffleEntrances[exitRow];
                const auto& arrival = shuffleEntrances[target];
                std::fill(std::begin(options), std::end(options), 0);
                options[RO_SHUFFLE_DUNGEON_ENTRANCES] = options[RO_SHUFFLE_BOSS_ENTRANCES] = others;
                options[RO_SHUFFLE_GROTTOS] = options[RO_SHUFFLE_INTERIORS] = others;
                gSaveContext = {};
                // Coupled: exit -> target's place, the target's opposite exit -> back to this exit's place.
                std::vector<std::pair<int32_t, int32_t>> pairs{ { exit.into, arrival.into } };
                pairs.emplace_back(shuffleEntrances[opposite[target]].into, shuffleEntrances[opposite[exitRow]].into);
                SetPairs(pairs);
                for (int on = 0; on < 2; ++on) {
                    options[RO_SHUFFLE_OVERWORLD] = on;
                    for (s16 scene : SceneForms(exit.outsideScene)) {
                        for (u16 form : { exit.into, SpringForm(exit.into), u16(exit.into | 1) }) {
                            for (int flag : { 0, -2, 1, -1, -6, 4, -3, -7 }) {
                                for (int cutscene : { 0, 1 }) {
                                    for (int mounted : { 0, 1 }) {
                                        gSaveContext.respawnFlag = flag;
                                        gSaveContext.nextCutsceneIndex = 0xFFF1;
                                        gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = form;
                                        gSaveContext.save.saveInfo.horseData = {};
                                        gHorseIsMounted = mounted;
                                        hasEponasSong = true;
                                        player.rideActor = mounted ? &horse : nullptr;
                                        playState = { form, scene, 0 };
                                        playState.csCtx.state = cutscene;
                                        playState.player = &player;
                                        OnBeforeTransitionSetup();
                                        bool real = on && (flag == 0 || flag == -2) && cutscene == 0;
                                        bool moved = real && target != exitRow;
                                        assert(playState.nextEntrance == (moved ? arrival.into : form));
                                        assert(gSaveContext.nextCutsceneIndex == (moved ? 0xFFEF : 0xFFF1));
                                        assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance ==
                                               (moved ? arrival.into : form));
                                        assert(gHorseIsMounted == (moved ? 0 : mounted));
                                        const auto& horseData = gSaveContext.save.saveInfo.horseData;
                                        if (moved && mounted) {
                                            assert(horseData.sceneId == scene && horseData.pos.x == 10 &&
                                                   horseData.pos.z == 30 && horseData.yaw == 77);
                                        } else {
                                            assert(horseData.sceneId == 0);
                                        }
                                        if (moved) {
                                            // A base entrance whatever form the exit held: Play_Init then loads the target's variant.
                                            for (int cleared = 0; cleared < 4; ++cleared) {
                                                assert(LoadedGroup(playState.nextEntrance, cleared) ==
                                                       LoadedGroup(arrival.into, cleared));
                                                ++loads;
                                            }
                                            ++redirected;
                                        } else {
                                            ++kept;
                                        }
                                        ++cases;
                                    }
                                }
                            }
                        }
                    }
                    if (on) {
                        // After arriving: a void out or death in the target's scene reloads its own entrance, and a
                        // plain exit from the target's scene to its own entrance is no row (rows leave their scene).
                        for (int flag : { 0, 1, -1 }) {
                            gSaveContext.respawnFlag = flag;
                            gHorseIsMounted = 0;
                            playState = { arrival.into, arrival.insideScene, 0 };
                            playState.player = &player;
                            OnBeforeTransitionSetup();
                            assert(playState.nextEntrance == arrival.into);
                            ++cases;
                        }
                        // Grotto return to a place the player had entered through a pool entrance (z_player.c:6497).
                        gSaveContext.respawnFlag = 4;
                        playState = { exit.into, SCENE_KAKUSIANA, 0 };
                        playState.player = &player;
                        OnBeforeTransitionSetup();
                        assert(playState.nextEntrance == exit.into);
                        // Songs and owls lead to entrances that are no row.
                        for (unsigned warp : songAndOwlEntrances) {
                            gSaveContext.respawnFlag = 0;
                            playState = { u16(warp), exit.outsideScene, 0 };
                            playState.player = &player;
                            OnBeforeTransitionSetup();
                            assert(playState.nextEntrance == warp);
                            ++cases;
                        }
                    }
                }
            }
        }
    }
    std::cout << "Overworld: " << cases << " transitions (" << rows.size() * rows.size() << " pairings, "
              << redirected << " redirected, " << kept << " kept, " << loads << " variant loads) passed\n";
}

} // namespace OverworldTest
