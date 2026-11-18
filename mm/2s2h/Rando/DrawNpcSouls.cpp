// NPC soul models (Archipelago fork): each NPC soul shows its NPC, shrunk, in a blue light, the way the enemy souls
// in DrawFuncs.cpp show their enemy in a gray one. Skeleton, animation and segments follow the NPC's own draw
// function in src/overlays/actors; the source line is noted per entry. Override functions of the actors need the
// actor struct and are not reused: the limbs they always hide or replace are listed here instead.
#include "DrawNpcSouls.h"
#include "DrawFuncs.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "BenPort.h"

#include <array>
#include <vector>
#include <libultraship/bridge/resourcebridge.h>

extern "C" {
#include "variables.h"
#include "functions.h"
#include "assets/code/actor/actor.h"

// BenPort.h declares these for C only
int ResourceMgr_OTRSigCheck(char* imgData);
AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path);
SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* skelAnime);

// clang-format off
/* Anju */              #include "assets/objects/object_an1/object_an1.h"
/* Banker, Mamamu */    #include "assets/objects/object_boj/object_boj.h"
/* Beavers */           #include "assets/objects/object_az/object_az.h"
/* Cows */              #include "assets/objects/object_cow/object_cow.h"
/* Deku Butler */       #include "assets/objects/object_dnj/object_dnj.h"
/* Evan */              #include "assets/objects/object_zos/object_zos.h"
/* Gorman Brothers */   #include "assets/objects/object_in/object_in.h"
/* Granny */            #include "assets/objects/object_nb/object_nb.h"
/* Grog */              #include "assets/objects/object_hs/object_hs.h"
/* Guru-Guru */         #include "assets/objects/object_fu/object_fu.h"
/* Honey & Darling */   #include "assets/objects/object_mu/object_mu.h"
/* Kamaro */            #include "assets/objects/object_yb/object_yb.h"
/* Keaton */            #include "assets/objects/object_kitan/object_kitan.h"
/* Lottery */           #include "assets/objects/object_kujiya/object_kujiya.h"
/* Madame Aroma */      #include "assets/objects/object_al/object_al.h"
/* Mamamu Yan */        #include "assets/objects/object_aob/object_aob.h"
/* Mayor */             #include "assets/objects/object_dt/object_dt.h"
/* Business Scrubs */   #include "assets/objects/object_dnt/object_dnt.h"
/* Poe Collector */     #include "assets/objects/object_ps/object_ps.h"
/* Postman */           #include "assets/objects/object_mm/object_mm.h"
/* Rosa Sisters */      #include "assets/objects/object_rz/object_rz.h"
/* Soldiers */          #include "assets/objects/object_sdn/object_sdn.h"
/* Shooting Gallery */  #include "assets/objects/object_shn/object_shn.h"
/* Smiths */            #include "assets/objects/object_kgy/object_kgy.h"
/* Spider House Men */  #include "assets/objects/object_ssh/object_ssh.h"
/* Swordsman */         #include "assets/objects/object_js/object_js.h"
/* Tingle */            #include "assets/objects/object_bal/object_bal.h"
/* Toilet Hand */       #include "assets/objects/object_bjt/object_bjt.h"
/* Toto */              #include "assets/objects/object_zm/object_zm.h"
/* Chest Shop Gal */    #include "assets/objects/object_bg/object_bg.h"
/* Kafei's Mask */      #include "assets/objects/object_mask_kerfay/object_mask_kerfay.h"
/* Bomb Shop Lady */    #include "assets/objects/object_bba/object_bba.h"
/* Curiosity Shop */    #include "assets/objects/object_fsn/object_fsn.h"
/* Bomb Shopkeeper */   #include "assets/objects/object_rsn/object_rsn.h"
/* Gorons */            #include "assets/objects/object_oF1d_map/object_oF1d_map.h"
/* Zoras */             #include "assets/objects/object_zo/object_zo.h"
/* Trading Post */      #include "assets/objects/object_ani/object_ani.h"
/* Kotake */            #include "assets/objects/object_trt/object_trt.h"
/* Koume */             #include "assets/objects/object_tru/object_tru.h"
/* Deku King */         #include "assets/objects/object_dno/object_dno.h"
/* Deku Princess */     #include "assets/objects/object_dnq/object_dnq.h"
/* Deku Palace */       #include "assets/objects/object_dnk/object_dnk.h"
/* Monkey */            #include "assets/objects/object_mnk/object_mnk.h"
/* Goron Elder */       #include "assets/objects/object_jg/object_jg.h"
/* Darmani */           #include "assets/objects/object_gg/object_gg.h"
/* Indigo-Gos */        #include "assets/objects/object_zov/object_zov.h"
/* Great Bay Folk */    #include "assets/objects/object_mk/object_mk.h"
/* Romani */            #include "assets/objects/object_ma1/object_ma1.h"
/* Gorman */            #include "assets/objects/object_in2/object_in2.h"
/* Mr. Barten */        #include "assets/objects/object_tab/object_tab.h"
/* Carpenters */        #include "assets/objects/object_daiku/object_daiku.h"
/* Bombers */           #include "assets/objects/object_cs/object_cs.h"
/* Townsfolk */         #include "assets/objects/object_bji/object_bji.h"
/* Pamela's Father */   #include "assets/objects/object_harfgibud/object_harfgibud.h"
/* Composer Brothers */ #include "assets/objects/object_po_composer/object_po_composer.h"
/* Dampe */             #include "assets/objects/object_tk/object_tk.h"
/* Frogs */             #include "assets/objects/object_fr/object_fr.h"
/* Kaepora Gaebora */   #include "assets/objects/object_owl/object_owl.h"
/* Talking Gibdos */    #include "assets/objects/object_rd/object_rd.h"
/* Pirates */           #include "assets/objects/object_gla/object_gla.h"
/* Gorman's horses */   #include "assets/objects/object_ha/object_ha.h"
// clang-format on
}

// File-local helpers (static, not an anonymous namespace: OPEN_DISPS declares FrameInterpolation functions at block
// scope, which must bind to the extern "C" declarations at file scope)

// The blue light of every NPC soul (the enemy souls use mostly gray), as before 8c.
const Color_RGB8 kNpcSoulLight = { 90, 170, 255 };

// Size: every model is scaled to the height the pirate enemy soul shows (DrawPirate: scale 0.01 on a model drawn
// at actor scale 0.0125, about 60 units tall), and moved down by 45 % of its height (DrawPirate: -2000), so it is
// centered on the origin like the enemy souls. height = rough height of the NPC in the world, actorScale = its
// Actor_SetScale. Both are estimates from the actor sources and still need checking in the game.
constexpr f32 kShownHeight = 48.0f;

constexpr s32 kMaxLimbs = 64;

enum class SegKind : u8 { Texture, EnvColor, EmptyDL };

struct SegmentSetting {
    u8 segment;
    SegKind kind;
    const void* texture;
    Color_RGBA8 color;
};

#define SEG_T(seg, tex)                                           \
    SegmentSetting {                                              \
        seg, SegKind::Texture, (const void*)(tex), { 0, 0, 0, 0 } \
    }
#define SEG_E(seg, r, g, b, a)             \
    SegmentSetting {                       \
        seg, SegKind::EnvColor, nullptr, { \
            r, g, b, a                     \
        }                                  \
    }
#define SEG_L(seg)                                     \
    SegmentSetting {                                   \
        seg, SegKind::EmptyDL, nullptr, { 0, 0, 0, 0 } \
    }

struct LimbSetting {
    s32 limbIndex;
    const void* dList; // nullptr hides the limb
};

enum class MatAnim : u8 { None, Draw, DrawStep, DrawStepOpa };

struct NpcSoulModel {
    RandoItemId randoItemId;
    const void* skeleton;
    const void* animation;
    bool flex;
    f32 actorScale;
    f32 height;
    bool setupDL37;     // Gfx_SetupDL37_Opa instead of Gfx_SetupDL25_Opa, as in the actor
    bool renderModeXlu; // Scene_SetRenderModeXlu(play, 0, 1) before drawing, as in the actor
    Color_RGBA8 envColor;
    std::vector<SegmentSetting> segments;
    std::vector<LimbSetting> limbs;
    MatAnim matAnim = MatAnim::None;
    const void* matAnimHeader = nullptr;
};

constexpr Color_RGBA8 kEnvBlack = { 0, 0, 0, 255 };

// clang-format off
const std::vector<NpcSoulModel> sNpcSoulModels = {
    // z_en_an.c:3959-3960
    { RI_SOUL_NPC_ANJU, gAnju1Skel, gAnju1IdleAnim, true, 0.01f, 62.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gAnju1EyeOpenTex), SEG_T(0x09, gAnju1MouthClosedTex) }, {} },
    // z_en_ginko_man.c:683, 709-710
    { RI_SOUL_NPC_BANKER, object_boj_Skel_00C240, object_boj_Anim_0043F0, true, 0.01f, 60.0f, false, false, kEnvBlack,
      { SEG_E(0x08, 50, 80, 0, 0), SEG_E(0x09, 50, 80, 0, 0) }, { { OBJECT_BOJ_LIMB_0F, object_boj_DL_00B1D8 } } },
    // z_en_az.c:1897 (older brother: no segments)
    { RI_SOUL_NPC_BEAVERS, gBeaverOlderBrotherSkel, gBeaverWalkAnim, true, 0.012f, 46.0f, false, false, kEnvBlack,
      {}, {} },
    // z_en_cow.c:416, 433-434
    { RI_SOUL_NPC_COWS, gCowSkel, gCowChewAnim, true, 0.01f, 70.0f, true, false, kEnvBlack,
      {}, { { COW_LIMB_NOSE_RING, nullptr } } },
    // z_en_dno.c:1013 (props are drawn per state in EnDno_PostLimbDraw; hidden here)
    { RI_SOUL_NPC_DEKU_BUTLER, gDekuButlerSkel, gDekuButlerIdleAnim, true, 0.01f, 58.0f, false, false, kEnvBlack,
      {}, { { DEKU_BUTLER_LIMB_CANDLE, nullptr }, { DEKU_BUTLER_LIMB_CANDLE_WICK, nullptr },
            { DEKU_BUTLER_LIMB_PARASOL_HANDLE, nullptr }, { DEKU_BUTLER_LIMB_PARASOL_LOWER_PETALS, nullptr },
            { DEKU_BUTLER_LIMB_PARASOL_MIDDLE_PETALS, nullptr }, { DEKU_BUTLER_LIMB_PARASOL_UPPER_PETALS, nullptr } } },
    // z_en_zos.c:746
    { RI_SOUL_NPC_EVAN, gEvanSkel, gEvanLeanOnKeyboardAnim, true, 0.0115f, 50.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gEvanEyeOpenTex) }, {} },
    // z_en_in.c:1677-1682 (set in EnIn_OverrideLimbDraw)
    { RI_SOUL_NPC_GORMAN_BROTHERS, object_in_Skel_014EA8, object_in_Anim_001D10, true, 0.01f, 64.0f, false, false,
      kEnvBlack, { SEG_T(0x08, object_in_Tex_0035E0), SEG_T(0x09, object_in_Tex_003520) }, {},
      MatAnim::DrawStep, object_in_Matanimheader_001C30 },
    // z_en_nb.c:931-932
    { RI_SOUL_NPC_GRANNY, gNbSkel, gNbIdleAnim, true, 0.01f, 55.0f, true, false, kEnvBlack, {}, {} },
    // z_en_hs.c:320-324, 360
    { RI_SOUL_NPC_GROG, gHsSkel, gHsIdleAnim, true, 0.01f, 60.0f, true, false, kEnvBlack,
      {}, { { HS_LIMB_HIDDEN_HAIR, nullptr } } },
    // z_en_guruguru.c:406-407
    { RI_SOUL_NPC_GURU_GURU, gGuruGuruSkel, gGuruGuruPlayStillAnim, true, 0.01f, 60.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gGuruGuruEyeClosedTex), SEG_T(0x09, gGuruGuruMouthOpenTex) }, {} },
    // z_en_fu.c:1435-1436
    { RI_SOUL_NPC_HONEY_AND_DARLING, gHoneyAndDarlingSkel, gHoneyAndDarlingCupCheeksLoopAnim, true, 0.01f, 55.0f,
      false, false, kEnvBlack, { SEG_E(0x08, 0, 50, 160, 0), SEG_E(0x09, 255, 255, 255, 0) }, {} },
    // z_en_yb.c:479-481
    { RI_SOUL_NPC_KAMARO, gYbSkel, object_yb_Anim_000200, true, 0.01f, 70.0f, false, true, kEnvBlack, {}, {} },
    // z_en_kitan.c:388-389
    { RI_SOUL_NPC_KEATON, gKeatonSkel, gKeatonIdleAnim, true, 0.015f, 40.0f, true, false, kEnvBlack, {}, {} },
    // z_en_al.c:1166-1172 (the shawl is drawn separately by the actor)
    { RI_SOUL_NPC_MADAME_AROMA, gMadameAromaSkel, object_al_Anim_000C54, true, 0.01f, 62.0f, false, false, kEnvBlack,
      {}, { { MADAME_AROMA_LIMB_SHAWL_MIDDLE, nullptr }, { MADAME_AROMA_LIMB_SHAWL_UPPER, nullptr },
            { MADAME_AROMA_LIMB_SHAWL_LEFT_LOWER_MIDDLE, nullptr }, { MADAME_AROMA_LIMB_SHAWL_LEFT_LOWER, nullptr },
            { MADAME_AROMA_LIMB_SHAWL_RIGHT_LOWER_MIDDLE, nullptr }, { MADAME_AROMA_LIMB_SHAWL_RIGHT_LOWER, nullptr } } },
    // z_en_aob_01.c:1169-1171 (head and eyes in the override), 1219-1220
    { RI_SOUL_NPC_MAMAMU_YAN, gMamamuYanSkel, gMamamuYanIdleAnim, true, 0.01f, 64.0f, false, false, kEnvBlack,
      { SEG_E(0x08, 50, 80, 0, 0), SEG_E(0x09, 50, 80, 0, 0), SEG_T(0x0A, gMamamuYanEyeOpenTex) },
      { { MAMAMU_YAN_LIMB_HEAD, gMamamuYanHeadDL } } },
    // z_en_dt.c:839-845
    { RI_SOUL_NPC_MAYOR, object_dt_Skel_00B0CC, gDotourWaitAnim, true, 0.01f, 70.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gDotourEyeOpenTex), SEG_T(0x09, gDotourEyebrowMidTex) }, {} },
    // z_en_lift_nuts.c:1090-1100
    { RI_SOUL_NPC_PLAYGROUND_SCRUBS, gBusinessScrubSkel, gBusinessScrubStandingAnim, true, 0.01f, 40.0f, false, false,
      kEnvBlack, {},
      { { BUSINESS_SCRUB_LIMB_RIGHT_HAND_HAT, nullptr }, { BUSINESS_SCRUB_LIMB_RIGHT_HAND_BAG, nullptr },
        { BUSINESS_SCRUB_LIMB_LEFT_HAND_BAG, nullptr }, { BUSINESS_SCRUB_LIMB_LEFT_ARM_BAG, nullptr },
        { BUSINESS_SCRUB_LIMB_SCALP, nullptr }, { BUSINESS_SCRUB_LIMB_HAIR, nullptr },
        { BUSINESS_SCRUB_LIMB_EYES, gBusinessScrubEyesDL } } },
    // z_en_gb2.c:1006, 1027-1031
    { RI_SOUL_NPC_POE_COLLECTOR, object_ps_Skel_007230, object_ps_Anim_00049C, true, 0.01f, 75.0f, false, true,
      { 255, 255, 255, 255 }, {}, { { OBJECT_PS_LIMB_01, nullptr } } },
    // z_en_mm3.c:599-600 (0x0C is an end-only display list)
    { RI_SOUL_NPC_POSTMAN, object_mm_Skel_0096E8, object_mm_Anim_00A4E0, true, 0.01f, 63.0f, false, false, kEnvBlack,
      { SEG_T(0x08, object_mm_Tex_002950), SEG_L(0x0C) }, {} },
    // z_en_rz.c:703-713
    { RI_SOUL_NPC_ROSA_SISTERS, object_rz_Skel_00D8D8, gRosaSistersStandingAnim, true, 0.01f, 70.0f, false, false,
      kEnvBlack, { SEG_T(0x09, object_rz_Tex_00BC50) }, {}, MatAnim::DrawStepOpa, object_rz_Matanimheader_00D768 },
    // z_en_stone_heishi.c:542
    { RI_SOUL_NPC_SHIRO, gSoldierSkel, gSoldierWaveAnim, true, 0.01f, 70.0f, false, false, kEnvBlack, {}, {} },
    // z_en_syateki_man.c:1551-1554
    { RI_SOUL_NPC_SHOOTING_GALLERY_OWNERS, gBurlyGuySkel, gBurlyGuyHeadScratchLoopAnim, true, 0.01f, 70.0f, true,
      false, kEnvBlack,
      { SEG_T(0x08, gSwampShootingGalleryManEyeOpenTex), SEG_T(0x09, gSwampShootingGalleryManEyeOpenTex) }, {} },
    // z_en_kgy.c:1152-1157, 1226
    { RI_SOUL_NPC_SMITHS, object_kgy_Skel_00F910, object_kgy_Anim_004B98, true, 0.01f, 70.0f, false, false, kEnvBlack,
      {}, { { OBJECT_KGY_LIMB_11, nullptr } } },
    // z_en_ssh.c:970-972 (En_Sth needs a head display list from the overlay; the cursed man is drawn instead)
    { RI_SOUL_NPC_SPIDER_HOUSE_MEN, object_ssh_Skel_006470, object_ssh_Anim_001494, false, 0.04f, 50.0f, false, false,
      kEnvBlack, { SEG_T(0x08, object_ssh_Tex_001970) }, {} },
    // z_en_kendo_js.c:799
    { RI_SOUL_NPC_SWORDSMAN, object_js_Skel_006990, object_js_Anim_000F4C, true, 0.01f, 70.0f, false, false, kEnvBlack,
      {}, {} },
    // z_en_bal.c:1172
    { RI_SOUL_NPC_TINGLE, gTingleSkel, gTingleFloatIdleAnim, true, 0.02f, 90.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gTingleEyeOpenTex) }, {} },
    // z_en_bjt.c:508
    { RI_SOUL_NPC_TOILET_HAND, gToiletHandSkel, gToiletHandWaitingAnim, true, 0.01f, 40.0f, false, false, kEnvBlack,
      {}, {} },
    // z_en_toto.c:717-718
    { RI_SOUL_NPC_TOTO, object_zm_Skel_00A978, object_zm_Anim_00C880, true, 0.01f, 55.0f, false, true, kEnvBlack,
      { SEG_T(0x08, gTotoEyesOpenTex) }, {} },
    // z_en_takaraya.c:430
    { RI_SOUL_NPC_TREASURE_CHEST_SHOP_GAL, gTreasureChestShopGalSkel, object_bg_Anim_009890, true, 0.01f, 62.0f, false,
      false, kEnvBlack, { SEG_T(0x08, gTreasureChestShopGalEyeOpenUpTex) }, {} },
    // z_en_baba.c:825-827
    { RI_SOUL_NPC_BOMB_SHOP_LADY, gBombShopLadySkel, gBombShopLadyWalkingHoldingBagAnim, true, 0.01f, 64.0f, true,
      false, kEnvBlack, { SEG_T(0x08, gBombShopLadyEyeTex) }, {} },
    // z_en_fsn.c:1696, 1726-1728
    { RI_SOUL_NPC_CURIOSITY_SHOP_OWNER, gFsnSkel, gFsnIdleAnim, true, 0.01f, 64.0f, true, false, kEnvBlack,
      { SEG_T(0x08, gFsnEyeOpenTex), SEG_T(0x09, gFsnEyeOpenTex) }, { { FSN_LIMB_TOUPEE, nullptr } } },
    // z_en_sob1.c:1742
    { RI_SOUL_NPC_BOMB_SHOP_OWNER, gBombShopkeeperSkel, gBombShopkeeperWalkAnim, true, 0.01f, 64.0f, false, false,
      kEnvBlack, { SEG_T(0x08, gBombShopkeeperEyeTex) }, {} },
    // z_en_sob1.c:1719
    { RI_SOUL_NPC_GORON_SHOPKEEPER, gGoronSkel, gGoronStandingIdleAnim, true, 0.01f, 80.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gGoronEyeOpenTex) }, {} },
    // z_en_sob1.c:1694-1696
    { RI_SOUL_NPC_ZORA_SHOPKEEPER, gZoraSkel, gZoraIdleAnim, true, 0.01f, 60.0f, false, false, kEnvBlack,
      { SEG_L(0x0C), SEG_T(0x08, gZoraEyeOpenTex) }, {} },
    // z_en_ossan.c:1803-1804 (the part-timer, so it differs from the Curiosity Shop owner)
    { RI_SOUL_NPC_TRADING_POST, gAniSkel, gAniStandingNormalAnim, true, 0.01f, 64.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gAniOpenEyeTex) }, {} },
    // z_en_trt.c:1796-1798, 1841-1842
    { RI_SOUL_NPC_KOTAKE, gKotakeSkel, gKotakeHoverAnim, true, 0.008f, 50.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gKotakeEyeOpenTex), SEG_T(0x09, gKotakeEyeOpenTex) }, { { KOTAKE_LIMB_BROOM, nullptr } } },
    // z_en_tru.c:1263-1264, 1333-1334
    { RI_SOUL_NPC_KOUME, gKoumeSkel, gKoumeIdleAnim, true, 0.008f, 50.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gKoumeEyeOpenTex), SEG_T(0x09, gKoumeEyeOpenTex) }, { { KOUME_LIMB_BROOM, nullptr } } },
    // z_en_dnq.c:545
    { RI_SOUL_NPC_DEKU_KING, gDekuKingSkel, gDekuKingIdleAnim, true, 0.02f, 80.0f, false, false, kEnvBlack, {}, {} },
    // z_en_dnp.c:566
    { RI_SOUL_NPC_DEKU_PRINCESS, gDekuPrincessSkel, gDekuPrincessIdleAnim, true, 0.0085f, 38.0f, false, false,
      kEnvBlack, { SEG_T(0x08, gDekuPrincessEyeOpenTex) }, {} },
    // z_en_guard_nuts.c:384-386 (not a flex skeleton)
    { RI_SOUL_NPC_DEKU_PALACE_RESIDENTS, gDekuPalaceGuardSkel, gDekuPalaceGuardWaitAnim, false, 0.015f, 50.0f, false,
      false, kEnvBlack, { SEG_T(0x08, gDekuPalaceGuardEyeOpenTex) }, {} },
    // z_en_mnk.c:2193-2211
    { RI_SOUL_NPC_MONKEY, gMonkeySkel, object_mnk_Anim_0105DC, true, 0.012f, 35.0f, true, false, kEnvBlack,
      { SEG_T(0x08, object_mnk_Tex_015020) }, {} },
    // z_en_akindonuts.c:1750-1791 (same model as the playground scrubs)
    { RI_SOUL_NPC_BUSINESS_SCRUBS, gBusinessScrubSkel, gBusinessScrubStandingAnim, true, 0.01f, 40.0f, false, false,
      kEnvBlack, {},
      { { BUSINESS_SCRUB_LIMB_RIGHT_HAND_HAT, nullptr }, { BUSINESS_SCRUB_LIMB_RIGHT_HAND_BAG, nullptr },
        { BUSINESS_SCRUB_LIMB_LEFT_HAND_BAG, nullptr }, { BUSINESS_SCRUB_LIMB_LEFT_ARM_BAG, nullptr },
        { BUSINESS_SCRUB_LIMB_SCALP, nullptr }, { BUSINESS_SCRUB_LIMB_HAIR, nullptr },
        { BUSINESS_SCRUB_LIMB_EYES, gBusinessScrubEyesDL } } },
    // z_en_go.c:2996 (same model as the Goron shopkeeper)
    { RI_SOUL_NPC_GORONS, gGoronSkel, gGoronStandingIdleAnim, true, 0.01f, 80.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gGoronEyeOpenTex) }, {} },
    // z_en_jg.c:1043
    { RI_SOUL_NPC_GORON_ELDER_AND_SON, gGoronElderSkel, gGoronElderIdleAnim, true, 0.01f, 100.0f, false, false,
      kEnvBlack, {}, {} },
    // RI_SOUL_NPC_GORON_RACERS: rolled up, see DrawGoronRacer
    // z_en_gg.c:820-825
    { RI_SOUL_NPC_DARMANI, object_gg_Skel_00F6C0, object_gg_Anim_00F578, true, 0.01f, 72.0f, false, false, kEnvBlack,
      { SEG_T(0x08, object_gg_Tex_009260) }, {} },
    // z_en_zot.c:1429-1431
    { RI_SOUL_NPC_ZORAS, gZoraSkel, gZoraIdleAnim, true, 0.01f, 60.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gZoraEyeOpenTex), SEG_L(0x0C) }, {} },
    // z_en_zov.c:651-652 (Lulu: eyes on 0x09, mouth on 0x08)
    { RI_SOUL_NPC_INDIGO_GOS, gLuluSkel, gLuluLookDownAnim, true, 0.01f, 60.0f, false, false, kEnvBlack,
      { SEG_T(0x09, gLuluEyeOpenTex), SEG_T(0x08, gLuluMouthClosedTex) }, {} },
    // z_en_mk.c:501
    { RI_SOUL_NPC_GREAT_BAY_FOLK, gMarineResearcherSkel, gMarineResearcherIdleAnim, true, 0.01f, 65.0f, false, false,
      kEnvBlack, {}, {} },
    // z_en_ma4.c:1114-1115 (Romani)
    { RI_SOUL_NPC_ROMANI_AND_CREMIA, gRomaniSkel, gRomaniIdleAnim, true, 0.01f, 46.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gRomaniEyeOpenTex), SEG_T(0x09, gRomaniMouthHappyTex) }, {} },
    // z_en_gm.c:2067
    { RI_SOUL_NPC_GORMAN, object_in2_Skel_0078B0, object_in2_Anim_009CDC, true, 0.01f, 62.0f, false, false, kEnvBlack,
      { SEG_T(0x08, object_in2_Tex_0054A8) }, {} },
    // z_en_tab.c:787
    { RI_SOUL_NPC_MR_BARTEN, gBartenSkel, gBartenIdleAnim, true, 0.01f, 62.0f, false, false, kEnvBlack,
      { SEG_T(0x08, gBartenEyeOpenTex) }, {} },
    // z_en_stop_heishi.c:585
    { RI_SOUL_NPC_GUARDS_AND_OFFICIALS, gSoldierSkel, gSoldierStandHandOnHipAnim, true, 0.01f, 70.0f, false, false,
      kEnvBlack, {}, {} },
    // z_en_daiku.c:365 (first carpenter color), 385
    { RI_SOUL_NPC_CARPENTERS, object_daiku_Skel_00A850, object_daiku_Anim_002FA0, true, 0.01f, 60.0f, false, false,
      { 170, 10, 70, 255 }, {}, {} },
    // z_en_bombers.c:562-575, 589-593 (0x0A is a display list of the overlay; an empty one here)
    { RI_SOUL_NPC_BOMBERS, object_cs_Skel_00F82C, gBomberIdleAnim, true, 0.01f, 40.0f, false, true, kEnvBlack,
      { SEG_T(0x08, object_cs_Tex_00C520), SEG_T(0x09, object_cs_Tex_00E620), SEG_L(0x0A) },
      { { OBJECT_CS_LIMB_0F, nullptr }, { OBJECT_CS_LIMB_13, nullptr } } },
    // z_en_bji_01.c:496-497 (Shikashi)
    { RI_SOUL_NPC_TOWNSFOLK, gShikashiSkel, object_bji_Anim_000FDC, true, 0.01f, 64.0f, false, false, kEnvBlack,
      { SEG_T(0x08, object_bji_Tex_0049F0) }, {} },
    // z_en_hgo.c:430-431
    { RI_SOUL_NPC_PAMELAS_FATHER, gPamelasFatherHumanSkel, gPamelasFatherArmsFoldedAnim, true, 0.01f, 64.0f, false,
      false, kEnvBlack, { SEG_T(0x08, gPamelasFatherHumanEyeOpenTex) }, {} },
    // z_en_po_composer.c:164-172, 625, 674-684 (Sharp's colors, light fully on)
    { RI_SOUL_NPC_COMPOSER_BROTHERS, gPoeComposerSkel, gPoeComposerIdleAnim, true, 0.01f, 60.0f, false, false,
      kEnvBlack,
      { SEG_E(0x08, 255, 255, 210, 255), SEG_E(0x0A, 75, 20, 25, 255), SEG_E(0x0B, 90, 85, 50, 255), SEG_L(0x0C) },
      { { POE_COMPOSER_LIMB_LANTERN, nullptr } } },
    // z_en_tk.c:1434-1436
    { RI_SOUL_NPC_DAMPE, object_tk_Skel_00B9E8, object_tk_Anim_001FA8, true, 0.01f, 52.0f, false, false, kEnvBlack,
      { SEG_T(0x08, object_tk_Tex_004390) }, {} },
    // DrawMinifrog in DrawFuncs.cpp (yellow frog)
    { RI_SOUL_NPC_FROGS, gFrogSkel, gFrogIdleAnim, true, 0.01f, 14.0f, false, false, { 200, 170, 0, 255 },
      { SEG_T(0x08, gFrogIrisOpenTex), SEG_T(0x09, gFrogIrisOpenTex) }, {} },
    // z_en_owl.c:82, 1195-1197 (perching)
    { RI_SOUL_NPC_KAEPORA_GAEBORA, gOwlPerchingSkel, gOwlPerchAnim, true, 0.025f, 80.0f, true, false, kEnvBlack,
      { SEG_T(0x08, gOwlEyeOpenTex) }, {} },
    // z_en_talk_gibud.c:1214-1217
    { RI_SOUL_NPC_TALKING_GIBDOS, gGibdoSkel, gGibdoRedeadIdleAnim, true, 0.01f, 70.0f, false, false, kEnvBlack,
      { SEG_L(0x08) }, {} },
    // z_en_ge2.c:772-773 (purple pirate)
    { RI_SOUL_NPC_PIRATES, gGerudoPurpleSkel, gGerudoPurpleLookingAboutAnim, true, 0.01f, 70.0f, true, false,
      kEnvBlack, { SEG_T(0x08, gGerudoPurpleEyeOpenTex) }, {} },
};
// clang-format on

#undef SEG_T
#undef SEG_E
#undef SEG_L

// One state per model, created on first use and reused, like the static SkelAnime in SETUP_DRAW (DrawFuncs.cpp).
struct ModelState {
    bool tried = false;
    bool ok = false;
    SkelAnime skelAnime;
    std::array<Vec3s, kMaxLimbs> jointTable;
    std::array<Vec3s, kMaxLimbs> morphTable;
    u32 lastUpdate = 0;
};

static const NpcSoulModel* sCurrentModel = nullptr;

static s32 NpcSoul_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* thisx) {
    if (sCurrentModel != nullptr) {
        for (const LimbSetting& limb : sCurrentModel->limbs) {
            if (limb.limbIndex == limbIndex) {
                *dList = (Gfx*)limb.dList;
            }
        }
    }
    return false;
}

// Loads skeleton and animation and checks them before SkelAnime_Init touches them: a missing resource or a
// skeleton with more limbs than the tables hold leaves the model off (blue flame), instead of crashing.
static bool InitModel(const NpcSoulModel& model, ModelState& state) {
    if (!ResourceMgr_OTRSigCheck((char*)model.skeleton)) {
        return false;
    }
    SkeletonHeader* header = ResourceMgr_LoadSkeletonByName((const char*)model.skeleton, nullptr);
    if (header == nullptr) {
        return false;
    }
    s32 limbCount = header->limbCount + 1;
    if (limbCount > kMaxLimbs) {
        return false;
    }
    if (model.flex && ((FlexSkeletonHeader*)header)->dListCount <= 0) {
        return false;
    }
    if (model.animation != nullptr) {
        if (!ResourceMgr_OTRSigCheck((char*)model.animation)) {
            return false;
        }
        AnimationHeaderCommon* anim = ResourceMgr_LoadAnimByName((const char*)model.animation);
        if (anim == nullptr || anim->frameCount <= 0) {
            return false;
        }
    }

    if (model.flex) {
        SkelAnime_InitFlex(gPlayState, &state.skelAnime, (FlexSkeletonHeader*)model.skeleton,
                           (AnimationHeader*)model.animation, state.jointTable.data(), state.morphTable.data(),
                           limbCount);
    } else {
        SkelAnime_Init(gPlayState, &state.skelAnime, (SkeletonHeader*)model.skeleton, (AnimationHeader*)model.animation,
                       state.jointTable.data(), state.morphTable.data(), limbCount);
    }
    return state.skelAnime.skeleton != nullptr;
}

static void ApplySizeAndPosition(f32 actorScale, f32 height) {
    f32 scale = kShownHeight * actorScale / height;
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    Matrix_Translate(0.0f, -0.45f * height / actorScale, 0.0f, MTXMODE_APPLY);
}

// Returns false if nothing was drawn.
static bool DrawSkeletonModel(const NpcSoulModel& model, ModelState& state) {
    if (!state.tried) {
        state.tried = true;
        state.ok = InitModel(model, state);
    }
    if (!state.ok) {
        return false;
    }
    if (state.lastUpdate != gPlayState->state.frames) {
        state.lastUpdate = gPlayState->state.frames;
        SkelAnime_Update(&state.skelAnime);
        // Keep the model on the spot: animations that walk move the root
        state.skelAnime.jointTable[0].x = 0;
        state.skelAnime.jointTable[0].z = 0;
    }

    OPEN_DISPS(gPlayState->state.gfxCtx);

    Matrix_Push();
    if (model.setupDL37) {
        Gfx_SetupDL37_Opa(gPlayState->state.gfxCtx);
    } else {
        Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    }
    ApplySizeAndPosition(model.actorScale, model.height);

    if (model.matAnim == MatAnim::Draw) {
        AnimatedMat_Draw(gPlayState, (AnimatedMaterial*)Lib_SegmentedToVirtual((void*)model.matAnimHeader));
    } else if (model.matAnim == MatAnim::DrawStep) {
        AnimatedMat_DrawStep(gPlayState, (AnimatedMaterial*)Lib_SegmentedToVirtual((void*)model.matAnimHeader), 0);
    } else if (model.matAnim == MatAnim::DrawStepOpa) {
        AnimatedMat_DrawStepOpa(gPlayState, (AnimatedMaterial*)Lib_SegmentedToVirtual((void*)model.matAnimHeader), 0);
    }
    if (model.renderModeXlu) {
        Scene_SetRenderModeXlu(gPlayState, 0, 1);
    }
    gDPSetEnvColor(POLY_OPA_DISP++, model.envColor.r, model.envColor.g, model.envColor.b, model.envColor.a);
    for (const SegmentSetting& seg : model.segments) {
        switch (seg.kind) {
            case SegKind::Texture:
                gSPSegment(POLY_OPA_DISP++, seg.segment, (uintptr_t)Lib_SegmentedToVirtual((void*)seg.texture));
                break;
            case SegKind::EnvColor:
                gSPSegment(POLY_OPA_DISP++, seg.segment,
                           (uintptr_t)Gfx_EnvColor(gPlayState->state.gfxCtx, seg.color.r, seg.color.g, seg.color.b,
                                                   seg.color.a));
                break;
            case SegKind::EmptyDL:
                gSPSegment(POLY_OPA_DISP++, seg.segment, (uintptr_t)D_801AEFA0);
                break;
        }
    }

    sCurrentModel = &model;
    OverrideLimbDrawOpa overrideLimbDraw = model.limbs.empty() ? nullptr : NpcSoul_OverrideLimbDraw;
    if (model.flex) {
        SkelAnime_DrawFlexOpa(gPlayState, state.skelAnime.skeleton, state.skelAnime.jointTable,
                              state.skelAnime.dListCount, overrideLimbDraw, nullptr, nullptr);
    } else {
        SkelAnime_DrawOpa(gPlayState, state.skelAnime.skeleton, state.skelAnime.jointTable, overrideLimbDraw, nullptr,
                          nullptr);
    }
    sCurrentModel = nullptr;
    Matrix_Pop();

    CLOSE_DISPS(gPlayState->state.gfxCtx);
    return true;
}

// Kafei: his own model is the player skeleton (z_en_test3.c:1357-1403); the get-item model of Kafei's Mask instead.
static void DrawKafeisMask() {
    Matrix_Push();
    Matrix_Scale(0.8f, 0.8f, 0.8f, MTXMODE_APPLY);
    GetItem_Draw(gPlayState, GID_MASK_KAFEIS_MASK);
    Matrix_Pop();
}

// Lottery: the kiosk, as EnKujiya_Draw draws it (z_en_kujiya.c:369-390), actor scale 0.1 (z_en_kujiya.c:59).
static void DrawLotteryKiosk() {
    if (!ResourceMgr_OTRSigCheck((char*)gLotteryShopTexAnim) ||
        ResourceGetDataByName((const char*)gLotteryShopTexAnim) == nullptr) {
        return;
    }
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Matrix_Push();
    ApplySizeAndPosition(0.1f, 110.0f);
    AnimatedMat_Draw(gPlayState, (AnimatedMaterial*)Lib_SegmentedToVirtual((void*)gLotteryShopTexAnim));
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopCylinderDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopBackSignDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopOpenBoxDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopScrollingEffectsDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopRedRupeesDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopGreenRupeesDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopBlueRupeesDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopTargetArrowAndSignDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopScrollingArrowDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLotteryShopMaskDL);
    Matrix_Pop();
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

// Goron racers: rolled up, as EnGo_Draw_NoSkeleton draws a rolled Goron (z_en_go.c:2874-2892).
static void DrawGoronRacer() {
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Matrix_Push();
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    f32 scale = kShownHeight * 0.01f / 40.0f;
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gGoronRolledUpDL);
    Matrix_Pop();
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

// Gorman's horse for the Epona riding ability: Epona herself is a Skin model (z_en_horse.c:755), the bandit horse a
// plain flex skeleton (z_en_horse.c:958, drawn at 4723 without segments), idle animation from sHniAnimations.
const NpcSoulModel sEponaRidingModel = {
    RI_ABILITY_RIDE_EPONA, gHorseBanditSkel, gHorseIdleAnim, true, 0.01f, 100.0f, false, false, kEnvBlack, {}, {}
};

bool Rando::DrawNpcSoul(RandoItemId randoItemId) {
    if (gPlayState == nullptr) {
        return false;
    }
    // One state per table row, allocated once (about 1 KB each)
    static std::vector<ModelState> sStates(sNpcSoulModels.size());

    bool drawn = false;
    switch (randoItemId) {
        case RI_SOUL_NPC_KAFEI:
            DrawKafeisMask();
            drawn = true;
            break;
        case RI_SOUL_NPC_LOTTERY_GIRL:
            DrawLotteryKiosk();
            drawn = true;
            break;
        case RI_SOUL_NPC_GORON_RACERS:
            DrawGoronRacer();
            drawn = true;
            break;
        default:
            for (size_t i = 0; i < sNpcSoulModels.size(); i++) {
                if (sNpcSoulModels[i].randoItemId == randoItemId) {
                    drawn = DrawSkeletonModel(sNpcSoulModels[i], sStates[i]);
                    break;
                }
            }
            break;
    }

    // The same blue light as before 8c, also the fallback when a model cannot be drawn
    Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
    DrawEnLight(kNpcSoulLight, { 10.0f, 10.0f, 10.0f });
    return drawn;
}

bool Rando::DrawEponaRidingAbility() {
    if (gPlayState == nullptr) {
        return false;
    }
    static ModelState sState;
    return DrawSkeletonModel(sEponaRidingModel, sState);
}
