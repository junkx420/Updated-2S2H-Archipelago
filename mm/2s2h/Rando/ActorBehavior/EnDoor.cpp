#include "ActorBehavior.h"
#include "Rando/MiscBehavior/DoorKeys.h"
#include "2s2h/CustomMessage/CustomMessage.h"

extern "C" {
#include "variables.h"
#include "functions.h"
}

#define DOOR_WONT_OPEN_MESSAGE_ID 0x1800

// The building whose key the door Player faces is missing (shown by the next "It won't open" text), or nullptr.
static const Rando::DoorKeys::DoorKey* lockedBuilding = nullptr;
static s16 lockedBuildingScene = -1;

// The entrance a knob door leads to. Such a door does not know it: Player walks through the open door onto a floor
// polygon with a scene exit (Player_HandleExitsAndVoids), so look for that floor behind the door, on the side away
// from Player. Returns -1 for a door between rooms of one scene (no exit behind it).
static s32 EntranceBehindDoor(Actor* door, f32 playerZ) {
    if (gPlayState->setupExitList == NULL) {
        return -1;
    }
    // Inverse of Actor_WorldToActorCoords for the point (0, y, z) in the door's coordinates
    f32 sin = Math_SinS(door->shape.rot.y);
    f32 cos = Math_CosS(door->shape.rot.y);
    f32 away = (playerZ >= 0.0f) ? -1.0f : 1.0f;

    for (f32 distance = 20.0f; distance <= 160.0f; distance += 20.0f) {
        Vec3f pos = { door->world.pos.x + away * distance * sin, door->world.pos.y + 50.0f,
                      door->world.pos.z + away * distance * cos };
        CollisionPoly* poly = NULL;
        s32 bgId = BGCHECK_SCENE;
        f32 floorY = BgCheck_EntityRaycastFloor5(&gPlayState->colCtx, &poly, &bgId, door, &pos);

        if (poly != NULL && floorY > BGCHECK_Y_MIN && fabsf(floorY - door->world.pos.y) < 60.0f) {
            u32 exitIndexPlusOne = SurfaceType_GetSceneExitIndex(&gPlayState->colCtx, poly, bgId);
            if (exitIndexPlusOne != 0) {
                return gPlayState->setupExitList[exitIndexPlusOne - 1];
            }
        }
    }
    return -1;
}

void Rando::ActorBehavior::InitEnDoorBehavior() {
    bool shouldRegister = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_DOOR_KEYS];
    lockedBuilding = nullptr;

    // Only the entrance behind the door decides (DoorKeys::IsEntranceLocked): the door out of a building leads to an
    // outside entrance, which is no "into" entrance of doorKeys, so it always opens.
    COND_VB_SHOULD(VB_OPEN_KNOB_DOOR, shouldRegister, {
        Actor* door = va_arg(args, Actor*);
        f32 playerZ = (f32)va_arg(args, double);
        s32 entrance = EntranceBehindDoor(door, playerZ);

        lockedBuilding = nullptr;
        if (entrance >= 0 && Rando::DoorKeys::IsEntranceLocked((u16)entrance)) {
            lockedBuilding = Rando::DoorKeys::FindByEntrance((u16)entrance);
            lockedBuildingScene = gPlayState->sceneId;
            *should = false;
        }
    });

    COND_ID_HOOK(OnOpenText, DOOR_WONT_OPEN_MESSAGE_ID, shouldRegister, [](u16* textId, bool* loadFromMessageTable) {
        if (lockedBuilding == nullptr || lockedBuildingScene != gPlayState->sceneId) {
            return; // another door that won't open: vanilla text
        }
        auto entry = CustomMessage::LoadVanillaMessageTableEntry(*textId);
        entry.msg = "It's locked.\nYou need the %r" + std::string(lockedBuilding->building) + " Key%w.";
        CustomMessage::LoadCustomMessageIntoFont(entry);
        *loadFromMessageTable = false;
    });
}
