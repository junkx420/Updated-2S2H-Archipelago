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
using s32 = int32_t;
struct RandoSaveInfo { u16 entrancePairs[512]{}; }; // z64save.h
struct RespawnData { u16 entrance = 0; s8 data = 0; int position = 0; };
struct Vec3f { float x = 0, y = 0, z = 0; };
struct Vec3s { s16 x = 0, y = 0, z = 0; };
struct HorseData { s16 sceneId = 0; Vec3s pos; s16 yaw = 0; }; // z64save.h
struct SaveContext {
    struct Save {
        struct Ship { RandoSaveInfo rando; } shipSaveInfo;
        u16 entrance = 0;
        struct SaveInfo { HorseData horseData; } saveInfo;
    } save;
    u16 nextCutsceneIndex = 0;
    int respawnFlag = 0;
    RespawnData respawn[4];
};
struct Actor { struct { Vec3f pos; } world; struct { Vec3s rot; } shape; };
struct Player { Actor actor; Actor* rideActor = nullptr; };
struct CutsceneContext { int state = 0; };
enum { CS_STATE_IDLE = 0 };
struct PlayState { u16 nextEntrance = 0; s16 sceneId = 0; int position = 0; CutsceneContext csCtx; Player* player = nullptr; };
#define GET_PLAYER(play) ((play)->player)
inline int gHorseIsMounted = 0; // z64horse.h
inline bool hasEponasSong = false;
enum { QUEST_SONG_EPONA };
#define CHECK_QUEST_ITEM(item) hasEponasSong
#define AFTER(slice) true
enum { TIME_DAY3_AM_06_00 };
namespace Rando { namespace Logic {} }
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
inline int randoEvents[RE_MAX + 1]{};
#define RANDO_EVENTS randoEvents
#define SET_WEEKEVENTREG(flag) (flags |= (flag))
enum { WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE = 1, WEEKEVENTREG_ENTERED_WOODFALL_TEMPLE_PRISON = 2,
       WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE = 4, WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE = 8,
       WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE = 16 };
// z64save.h RespawnMode values; z64player.h start modes are only passed through.
enum { RESPAWN_MODE_DOWN = 0, RESPAWN_MODE_TOP = 2, RESPAWN_MODE_UNK_3 = 3 };
enum { PLAYER_START_MODE_D, PLAYER_START_MODE_GROTTO };
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
    // z_play.c Play_SetupRespawnPoint does nothing inside grottos; Play_SetRespawnData clears the spawn bits,
    // retaining entrance scene and scene layer.
    if (play->sceneId != SCENE_KAKUSIANA) {
        gSaveContext.respawn[mode] = { u16(gSaveContext.save.entrance & ~0x1F0), 0, play->position };
    }
}
inline bool randoInf[RANDO_INF_MAX]{};
inline int Flags_GetRandoInf(int flag) { return randoInf[flag]; }
// Verbatim from the game (run.py copies them): z_scene.c Entrance_Create, z_play.c Play_GetOriginalSceneId.
#include "game_functions.h"
