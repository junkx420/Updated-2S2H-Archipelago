#include "ActorBehavior.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ObjectExtension/ActorListIndex.h"
#include "2s2h/ObjectExtension/ObjectExtension.h"
#include "2s2h/Rando/ObjectMarker.h"

extern "C" {
#include "overlays/actors/ovl_En_Kanban/z_en_kanban.h"
#include "variables.h"
}

namespace Rando::ActorBehavior {
namespace {

struct SignPlacement {
    s16 scene;
    s8 room;
    u8 layer;
    s16 index;
    s16 actorId;
    u16 params;
    u16 textId;
    Vec3s pos;
    RandoCheckId checkId;
};

constexpr SignPlacement signPlacements[] = {
#include "SignPlacements.inc"
};

struct SignInstance {
    const SignPlacement* placement;
};

ObjectExtension::Register<SignInstance> signRegister;
bool readingSign = false;

const SignPlacement* IdentifySign(const Actor* actor) {
    const s16 index = GetActorListIndex(actor);
    if (index < 0) {
        return nullptr;
    }

    const SignPlacement* basePlacement = nullptr;
    const Vec3f& home = actor->home.pos;
    for (const auto& placement : signPlacements) {
        if (placement.scene != gPlayState->sceneId || placement.room != actor->room || placement.index != index ||
            placement.actorId != actor->id || placement.params != static_cast<u16>(actor->params) ||
            home.x != placement.pos.x || home.y != placement.pos.y || home.z != placement.pos.z) {
            continue;
        }
        if (placement.layer == gSaveContext.sceneLayer) {
            return &placement;
        }
        // Some scene layers still use the room's original ActorList.
        if (placement.layer == 0) {
            basePlacement = &placement;
        }
    }
    return basePlacement;
}

void DrawSignSparkle(Actor* actor, PlayState* play) {
    const auto* sign = ObjectExtension::GetInstance().Get<SignInstance>(actor);
    if (sign == nullptr) {
        return;
    }
    Vec3f pos = actor->world.pos;
    pos.y += actor->id == ACTOR_EN_TALK ? 8.0f : 44.0f;
    ObjectMarker::DrawSparkle(play, pos, ObjectMarker::GetColor(sign->placement->checkId));
}

void TintSign(Actor* actor) {
    const auto* sign = ObjectExtension::GetInstance().Get<SignInstance>(actor);
    if (sign != nullptr) {
        ObjectMarker::AttachTint(actor, sign->placement->checkId, ObjectMarker::TintPasses::OpaqueAndTranslucent);
    }
}

void BindSign(Actor* actor) {
    switch (actor->id) {
        case ACTOR_EN_KANBAN:
        case ACTOR_EN_TALK:
        case ACTOR_OBJ_HAKAISI:
        case ACTOR_OBJ_GHAKA:
        case ACTOR_EN_SEKIHI:
            break;
        default:
            return;
    }

    const auto* placement = IdentifySign(actor);
    if (placement == nullptr) {
        return;
    }
    ObjectExtension::GetInstance().Set<SignInstance>(actor, { placement });
    if (actor->id == ACTOR_EN_TALK) {
        actor->draw = DrawSignSparkle;
    } else {
        TintSign(actor);
    }
}

void OnSignTextboxStarted(Actor* actor, u16 textId) {
    const auto* sign = ObjectExtension::GetInstance().Get<SignInstance>(actor);
    readingSign = sign != nullptr;
    if (sign == nullptr || textId != sign->placement->textId) {
        return;
    }

    const auto checkId = sign->placement->checkId;
    if (ObjectMarker::IsOpen(checkId)) {
        // The normal check queue starts the give once the entire message is finished.
        RANDO_SAVE_CHECKS[checkId].eligible = true;
    }
}

} // namespace

bool IsReadingSign() {
    if (gPlayState == nullptr || gPlayState->msgCtx.msgMode == MSGMODE_NONE) {
        readingSign = false;
    }
    return readingSign;
}

void InitSignBehavior() {
    readingSign = false;
    const bool enabled = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_SIGNS] != RO_GENERIC_OFF;
    COND_HOOK(OnSceneInit, enabled, [](s8, s8) { readingSign = false; });
    COND_HOOK(OnActorInit, enabled, BindSign);
    COND_HOOK(OnStartTextbox, enabled, OnSignTextboxStarted);
    // Mikau's grave gains a draw callback only after loading its object.
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_SEKIHI, enabled, TintSign);
    COND_ID_HOOK(OnActorDraw, ACTOR_EN_KANBAN, enabled, [](Actor* actor) {
        if (reinterpret_cast<EnKanban*>(actor)->partFlags != 0xFFFF) {
            DrawSignSparkle(actor, gPlayState);
        }
    });
    COND_VB_SHOULD(VB_READ_CUT_SIGN, enabled, {
        const auto* sign = ObjectExtension::GetInstance().Get<SignInstance>(va_arg(args, Actor*));
        *should = sign != nullptr && ObjectMarker::IsOpen(sign->placement->checkId);
    });
}

} // namespace Rando::ActorBehavior
