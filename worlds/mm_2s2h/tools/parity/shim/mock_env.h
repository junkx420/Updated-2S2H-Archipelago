// Mocked game environment for compiling 2S2H's real logic outside the game.
// Only the save-file reads the logic does are mocked; their contents come from the scenario file.
#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef float f32;
typedef int SceneId;
typedef int ActorId;
typedef int ItemId;
typedef int DungeonSceneIndex;

#include "Rando/Types.h"
#include "consts.h"

// Use the game's indices so that the same saved entrance IDs identify door rules.
#define ENTRANCE(scene, spawn) (((ENTR_SCENE_##scene & 0x7F) << 9) | (((spawn) & 0x1F) << 4))

struct Actor;
struct RandoSaveInfo {
    int foundDungeonKeys[4] = {};
    u16 entrancePairs[512] = {};
    u16 mixedEntrancePairs[1024] = {};
    u8 owlWarpTargets[10] = {};
    u8 owlWarpsDiscovered[2] = {};
    u8 cycleSpawn[2] = {};
    u16 directedEntranceTargets[1024] = {};
};
struct MockSaveCheck {
    u16 price = 0;
    bool shuffled = false;
    bool obtained = false;
};
struct MockSave {
    struct {
        struct {
            struct {
                u16 healthCapacity = 0x30;
                u8 isMagicAcquired = 0;
                u32 owlActivationFlags = 0;
            } playerData;
            struct {
                u8 strayFairies[4] = {};
            } inventory;
        } saveInfo;
        struct {
            RandoSaveInfo rando;
        } shipSaveInfo;
        int playerForm = PLAYER_FORM_HUMAN;
    } save;
    struct {
        int entrance = 0;
        int data = 0;
    } respawn[8];
};
extern MockSave gSaveContext;

struct MockState {
    std::set<int> inv, quest, inf, weekreg;
    std::set<std::pair<int, int>> dungeonItems;
    std::map<int, int> tokens;
    int sword = 0, shield = 0, quiver = 0, bombbag = 0, wallet = 0;
    bool bottle = false;
};
extern MockState gMock;
extern int RANDO_SAVE_OPTIONS[RO_MAX];
extern int RANDO_EVENTS[RE_MAX];
extern MockSaveCheck RANDO_SAVE_CHECKS[RC_MAX];

#define GET_PLAYER_FORM (gSaveContext.save.playerForm)
inline int INV_CONTENT(int item) {
    if (item == ITEM_BOTTLE) {
        return gMock.bottle ? ITEM_BOTTLE : ITEM_NONE;
    }
    return gMock.inv.contains(item) ? item : ITEM_NONE;
}
#define CHECK_QUEST_ITEM(q) (gMock.quest.contains(q))
#define CHECK_WEEKEVENTREG(w) (gMock.weekreg.contains(w))
inline bool Flags_GetRandoInf(int flag) {
    return gMock.inf.contains(flag);
}
inline int GET_CUR_EQUIP_VALUE(int type) {
    return type == EQUIP_TYPE_SWORD ? gMock.sword : gMock.shield;
}
inline int CUR_UPG_VALUE(int upg) {
    return upg == UPG_WALLET ? gMock.wallet : upg == UPG_BOMB_BAG ? gMock.bombbag : gMock.quiver;
}
#define GET_CUR_UPG_VALUE(upg) CUR_UPG_VALUE(upg)
#define CHECK_DUNGEON_ITEM(item, dungeon) (gMock.dungeonItems.contains({ (int)(item), (int)(dungeon) }))
inline int Inventory_GetSkullTokenCount(int scene) {
    return gMock.tokens.contains(scene) ? gMock.tokens.at(scene) : 0;
}
inline bool Inventory_HasItemInBottle(int) {
    assert(false && "not used by 5.0.1 logic");
    return false;
}
inline bool Flags_GetSceneSwitch(int, int) {
    assert(false && "not used by 5.0.1 logic");
    return false;
}
#define SPDLOG_ERROR(...)
inline const char* Ship_GetSceneName(int) {
    return "";
}

// Souls.h
#define SOUL_RI_TO_RANDO_INF(randoItemId) ((randoItemId - RI_SOUL_BOSS_GOHT) + RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT)
bool HaveEnemySoul(ActorId enemyId);

namespace Rando::StaticData {
struct MockStaticCheck {
    const char* name = "";
};
extern std::map<RandoCheckId, MockStaticCheck> Checks;
} // namespace Rando::StaticData
