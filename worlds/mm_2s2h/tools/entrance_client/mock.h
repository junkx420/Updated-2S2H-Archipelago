#pragma once
// Only game state/API boundaries are mocked. The test compiles the fork's real EntranceShuffle.cpp and DoorKeys.cpp.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <variant>
#include <vector>
#include "constants.h"
using u8 = uint8_t;
using u16 = uint16_t;
using s8 = int8_t;
using s16 = int16_t;
struct RandoSaveInfo { u8 entrancePairs[32]{}; };
struct Respawn { u16 entrance = 0; int data = 0; int position = 0; };
struct SaveContext {
    struct Save {
        struct Ship { RandoSaveInfo rando; } shipSaveInfo;
        u16 entrance = 0;
    } save;
    u16 nextCutsceneIndex = 0;
    int respawnFlag = 0;
    Respawn respawn[2];
};
struct PlayState { u16 nextEntrance = 0; s16 sceneId = 0; int position = 0; };
inline SaveContext gSaveContext;
inline PlayState playState;
inline PlayState* gPlayState = &playState;
inline int options[RO_MAX]{};
inline unsigned flags = 0;
#define RANDO_SAVE_OPTIONS options
// Area rules are evaluated by the AP tests; the transition hook never evaluates them.
#define CAN_BE_DEKU true
#define CAN_BE_GORON true
#define CAN_BE_ZORA true
#define HAS_MAGIC true
#define HAS_ITEM(item) true
#define CAN_PLAY_SONG(song) true
#define SET_WEEKEVENTREG(flag) (flags |= (flag))
enum { WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE = 1, WEEKEVENTREG_ENTERED_WOODFALL_TEMPLE_PRISON = 2,
       WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE = 4, WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE = 8,
       WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE = 16 };
enum { RESPAWN_MODE_DOWN, RESPAWN_MODE_TOP, PLAYER_START_MODE_D };
#define PLAYER_PARAMS(room, mode) (mode)
#define ENTRANCE(scene, spawn) (((ENTR_SCENE_##scene & 0x7F) << 9) | ((spawn & 0x1F) << 4))
struct GIEventTransition { int entrance; int cutsceneIndex; int transitionType; };
struct GameInteractor {
    std::vector<std::variant<GIEventTransition>> events;
    static GameInteractor* Instance;
};
inline GameInteractor gameInteractor;
inline GameInteractor* GameInteractor::Instance = &gameInteractor;
inline void Play_SetupRespawnPoint(PlayState* play, int mode, int params) {
    // z_play.c Play_SetRespawnData clears the spawn bits, retaining entrance scene and scene layer.
    gSaveContext.respawn[mode] = { u16(gSaveContext.save.entrance & ~0x1F0), 0, play->position };
}
inline bool randoInf[RANDO_INF_MAX]{};
inline int Flags_GetRandoInf(int flag) { return randoInf[flag]; }
