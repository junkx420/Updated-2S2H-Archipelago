#pragma once
// Only game state/API boundaries are mocked. The test compiles the fork's real EntranceShuffle.cpp and DoorKeys.cpp.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>
#include <variant>
#include <vector>
#include "constants.h"
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using f32 = float;
#include "rando_save.h" // verbatim from z64save.h
struct RespawnData { u16 entrance = 0; s8 data = 0; int position = 0; };
struct Vec3f { float x = 0, y = 0, z = 0; };
struct Vec3s { s16 x = 0, y = 0, z = 0; };
struct HorseData { s16 sceneId = 0; Vec3s pos; s16 yaw = 0; }; // z64save.h
struct SaveContext {
    struct Save {
        struct Ship { RandoSaveInfo rando; } shipSaveInfo;
        u16 entrance = 0;
        u16 cutsceneIndex = 0;
        bool isOwlSave = false;
        u8 day = 0;
        u16 time = 0;
        u8 eventDayCount = 0;
        struct SaveInfo {
            HorseData horseData;
            struct { int rupees = 0; } playerData;
        } saveInfo;
    } save;
    u16 nextCutsceneIndex = 0;
    int gameMode = 0;
    int respawnFlag = 0;
    RespawnData respawn[4];
    int sceneLayer = 0;
    int forcedSeqId = 0;
};
struct Actor {
    struct { Vec3f pos; } world;
    struct { Vec3s rot; } shape;
    Actor* child = nullptr;
    int id = 0;
    float xzDistToPlayer = 0;
};
struct Player { Actor actor; Actor* rideActor = nullptr; int transformation = 0; unsigned stateFlags1 = 0; };
struct CutsceneContext { int state = 0; };
struct MessageContext { int choiceIndex = 0; int unk1206C = 0; };
enum { CS_STATE_IDLE = 0 };
struct PlayState {
    u16 nextEntrance = 0;
    s16 sceneId = 0;
    int position = 0;
    CutsceneContext csCtx;
    Player* player = nullptr;
    MessageContext msgCtx;
    int actorCtx = 0, colCtx = 0, state = 0, colChkCtx = 0;
    int transitionType = 0, transitionTrigger = 0;
};
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
inline bool canBeDeku = true, canBeZora = true, canSwim = true, hasHookshot = true, canPlayTime = true;
inline bool isRando = true;
#define IS_RANDO isRando
#define CAN_BE_DEKU canBeDeku
#define CAN_BE_GORON true
#define CAN_BE_ZORA canBeZora
#define HAS_MAGIC true
#define HAS_ITEM(item) (std::string_view(#item) != "ITEM_HOOKSHOT" || hasHookshot)
#define CAN_PLAY_SONG(song) (std::string_view(#song) != "TIME" || canPlayTime)
#define CAN_USE_PROJECTILE true
#define CAN_USE_ABILITY(ability) (std::string_view(#ability) != "SWIM" || canSwim)
#define CAN_LIGHT_TORCH_NEAR_ANOTHER true
inline int randoEvents[RE_MAX + 1]{};
#define RANDO_EVENTS randoEvents
#define SET_WEEKEVENTREG(flag) (flags |= (flag))
inline bool CHECK_WEEKEVENTREG(unsigned flag) { return (flags & flag) != 0; }
enum { WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE = 1, WEEKEVENTREG_ENTERED_WOODFALL_TEMPLE_PRISON = 2,
       WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE = 4, WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE = 8,
       WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE = 16 };
// z64save.h RespawnMode values; z64player.h start modes are only passed through.
enum { RESPAWN_MODE_DOWN = 0, RESPAWN_MODE_TOP = 2, RESPAWN_MODE_UNK_3 = 3 };
enum { PLAYER_START_MODE_D, PLAYER_START_MODE_GROTTO };
#define PLAYER_PARAMS(room, mode) (mode)
#define ENTRANCE(scene, spawn) (((ENTR_SCENE_##scene & 0x7F) << 9) | ((spawn & 0x1F) << 4))
inline s32 Entrance_GetSceneIdAbsolute(u16 entrance) { return entrance >> 9; }
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
#include "clock_functions.h"
