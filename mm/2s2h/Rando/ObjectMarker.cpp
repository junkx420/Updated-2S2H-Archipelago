#include "ObjectMarker.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/ObjectExtension/ObjectExtension.h"

extern "C" {
#include "assets/objects/gameplay_keep/gameplay_keep.h"
}

namespace {

using Rando::ObjectMarker::TintPasses;

constexpr Color_RGBA8 defaultColor = { 255, 225, 105, 255 };

struct TintMarker {
    ActorFunc originalDraw;
    RandoCheckId first;
    RandoCheckId last;
    TintPasses passes;
};

ObjectExtension::Register<TintMarker> tintMarkerRegister;

} // namespace

static void DrawTinted(Actor* actor, PlayState* play) {
    const auto& marker = *ObjectExtension::GetInstance().Get<TintMarker>(actor);
    auto color = marker.first == marker.last ? Rando::ObjectMarker::GetColor(marker.first)
                                             : Rando::ObjectMarker::GetFixedColor(marker.first, marker.last);
    if (!color) {
        marker.originalDraw(actor, play);
        return;
    }

    const bool tintXlu = marker.passes == TintPasses::OpaqueAndTranslucent;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);

    gDPSetGrayscaleColor(POLY_OPA_DISP++, color->r, color->g, color->b, color->a);
    gSPGrayscale(POLY_OPA_DISP++, true);
    if (tintXlu) {
        gDPSetGrayscaleColor(POLY_XLU_DISP++, color->r, color->g, color->b, color->a);
        gSPGrayscale(POLY_XLU_DISP++, true);
    }

    marker.originalDraw(actor, play);

    gSPGrayscale(POLY_OPA_DISP++, false);
    if (tintXlu) {
        gSPGrayscale(POLY_XLU_DISP++, false);
    }

    CLOSE_DISPS(gfxCtx);
}

namespace Rando::ObjectMarker {

bool IsOpen(RandoCheckId checkId) {
    return IS_RANDO && checkId > RC_UNKNOWN && checkId < RC_MAX && RANDO_SAVE_CHECKS[checkId].shuffled &&
           !RANDO_SAVE_CHECKS[checkId].obtained;
}

std::optional<Color_RGBA8> GetColor(RandoCheckId checkId) {
    if (!IsOpen(checkId)) {
        return std::nullopt;
    }
    if (!CVarGetInteger("gRando.CSMC", 0)) {
        return defaultColor;
    }

    RandoItemId itemId = ConvertItem(RANDO_SAVE_CHECKS[checkId].randoItemId, checkId);
    const auto item = StaticData::Items.find(itemId);
    const auto itemType = item != StaticData::Items.end() ? item->second.randoItemType : RITYPE_JUNK;
    // Match the pot's classes and use the accent colors of its corresponding textures.
    switch (itemType) {
        case RITYPE_BOSS_KEY:
            return Color_RGBA8{ 193, 146, 54, 255 };
        case RITYPE_HEALTH:
            return Color_RGBA8{ 125, 195, 224, 255 };
        case RITYPE_LESSER:
            return Color_RGBA8{ 221, 132, 71, 255 };
        case RITYPE_MAJOR:
            return Color_RGBA8{ 211, 207, 84, 255 };
        case RITYPE_MASK:
            return Color_RGBA8{ 208, 93, 147, 255 };
        case RITYPE_SKULLTULA_TOKEN:
            return Color_RGBA8{ 246, 211, 132, 255 };
        case RITYPE_SMALL_KEY:
            return Color_RGBA8{ 139, 121, 203, 255 };
        case RITYPE_STRAY_FAIRY:
            return Color_RGBA8{ 232, 170, 191, 255 };
        default:
            return Color_RGBA8{ 188, 159, 141, 255 };
    }
}

std::optional<Color_RGBA8> GetFixedColor(RandoCheckId first, RandoCheckId last) {
    for (int id = first; id <= last; ++id) {
        if (IsOpen(static_cast<RandoCheckId>(id))) {
            return defaultColor;
        }
    }
    return std::nullopt;
}

void AttachTint(Actor* actor, RandoCheckId checkId, TintPasses passes) {
    AttachTint(actor, checkId, checkId, passes);
}

void AttachTint(Actor* actor, RandoCheckId first, RandoCheckId last, TintPasses passes) {
    if (actor->draw == nullptr || actor->draw == DrawTinted || !GetFixedColor(first, last)) {
        return;
    }

    ObjectExtension::GetInstance().Set<TintMarker>(actor, { actor->draw, first, last, passes });
    actor->draw = DrawTinted;
}

} // namespace Rando::ObjectMarker

static void DrawSparkleSprite(PlayState* play, Vec3f pos, Color_RGBA8 color) {
    // A regular pulse with the Wonder Item sprite. Drawing it directly ties its lifetime to the check.
    const u32 phase = play->state.frames % 20;
    const f32 scale = 0.38f + 0.025f * static_cast<f32>(phase < 10 ? phase : 20 - phase);
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);

    Matrix_Push();
    Matrix_Translate(pos.x, pos.y, pos.z, MTXMODE_NEW);
    Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
    Matrix_Scale(scale, scale, 1.0f, MTXMODE_APPLY);

    Gfx_SetupDL25_Xlu(gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, color.r, color.g, color.b, 255);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_FOG | G_LIGHTING);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gEffSparklesDL);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_FOG | G_LIGHTING);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    Matrix_Pop();

    CLOSE_DISPS(gfxCtx);
}

void Rando::ObjectMarker::DrawSparkle(PlayState* play, Vec3f pos, std::optional<Color_RGBA8> color) {
    if (color) {
        DrawSparkleSprite(play, pos, *color);
    }
}
