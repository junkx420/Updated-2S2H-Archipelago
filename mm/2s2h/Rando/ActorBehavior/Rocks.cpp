#include "ActorBehavior.h"

#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ObjectExtension/ActorListIndex.h"
#include "2s2h/ObjectExtension/ObjectExtension.h"
#include "2s2h/Rando/ObjectMarker.h"

extern "C" {
#include "overlays/actors/ovl_Obj_Mure2/z_obj_mure2.h"
}

namespace Rando::ActorBehavior {
namespace {

struct RockPlacement {
    s16 scene;
    s8 room;
    u8 layer;
    s16 index;
    s16 actorId;
    u16 params;
    Vec3s pos;
    RandoCheckId checkId;
};

// Ring rows name the first of eight consecutive checks, in vanilla child spawn order.
static constexpr RockPlacement rockPlacements[] = {
    { SCENE_KAKUSIANA, 5, 0, 6, ACTOR_OBJ_BOMBIWA, 0x0104, { 4321, 0, -756 }, RC_LONE_PEAK_SHRINE_ROCK_01 },
    { SCENE_KAKUSIANA, 5, 0, 7, ACTOR_OBJ_BOMBIWA, 0x0105, { 4227, 0, -313 }, RC_LONE_PEAK_SHRINE_ROCK_02 },
    { SCENE_KAKUSIANA, 5, 0, 9, ACTOR_OBJ_BOMBIWA, 0x0106, { 4625, 0, -358 }, RC_LONE_PEAK_SHRINE_ROCK_03 },
    { SCENE_KAKUSIANA, 11, 0, 1, ACTOR_EN_ISHI, 0x01f2, { 2940, 0, 1368 }, RC_TERMINA_FIELD_BIO_BABA_GROTTO_ROCK_04 },
    { SCENE_KAKUSIANA,
      14,
      0,
      10,
      ACTOR_OBJ_BOMBIWA,
      0x0114,
      { 6999, 1, 644 },
      RC_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO_ROCK_05 },
    { SCENE_KAKUSIANA,
      14,
      0,
      11,
      ACTOR_OBJ_BOMBIWA,
      0x0115,
      { 7254, 2, 814 },
      RC_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO_ROCK_06 },
    { SCENE_KAKUSIANA,
      14,
      0,
      12,
      ACTOR_OBJ_BOMBIWA,
      0x0116,
      { 7234, 2, 1158 },
      RC_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO_ROCK_07 },
    { SCENE_KAKUSIANA,
      14,
      0,
      13,
      ACTOR_OBJ_BOMBIWA,
      0x0117,
      { 6799, 2, 1156 },
      RC_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO_ROCK_08 },
    { SCENE_KAKUSIANA,
      14,
      0,
      14,
      ACTOR_OBJ_BOMBIWA,
      0x0118,
      { 6776, 1, 814 },
      RC_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO_ROCK_09 },
    { SCENE_00KEIKOKU, 0, 0, 127, ACTOR_EN_ISHI, 0x01f2, { -6779, -367, -843 }, RC_TERMINA_FIELD_ROCK_01 },
    { SCENE_00KEIKOKU, 0, 0, 140, ACTOR_OBJ_MURE2, 0x1f02, { -2532, 47, -2977 }, RC_TERMINA_FIELD_ROCK_RING_01 },
    { SCENE_00KEIKOKU,
      0,
      0,
      198,
      ACTOR_OBJ_BOMBIWA,
      0x8002,
      { -2782, 48, -1654 },
      RC_TERMINA_FIELD_ROCK_BIO_BABA_GROTTO },
    { SCENE_00KEIKOKU, 0, 0, 199, ACTOR_OBJ_BOMBIWA, 0x8003, { -5160, -281, -572 }, RC_TERMINA_FIELD_ROCK_COW_GROTTO },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 0, ACTOR_OBJ_HAMISHI, 0x00ff, { 146, -30, 831 }, RC_MOUNTAIN_VILLAGE_ROCK_01 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 6, ACTOR_OBJ_HAMISHI, 0x00ff, { 146, -30, 831 }, RC_MOUNTAIN_VILLAGE_ROCK_01 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 1, ACTOR_OBJ_HAMISHI, 0x00ff, { 147, -30, 770 }, RC_MOUNTAIN_VILLAGE_ROCK_02 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 7, ACTOR_OBJ_HAMISHI, 0x00ff, { 147, -30, 770 }, RC_MOUNTAIN_VILLAGE_ROCK_02 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 2, ACTOR_OBJ_HAMISHI, 0x00ff, { 79, -30, 805 }, RC_MOUNTAIN_VILLAGE_ROCK_03 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 8, ACTOR_OBJ_HAMISHI, 0x00ff, { 79, -30, 805 }, RC_MOUNTAIN_VILLAGE_ROCK_03 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 14, ACTOR_EN_ISHI, 0x00a0, { 244, 1408, -1322 }, RC_MOUNTAIN_VILLAGE_ROCK_04 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 20, ACTOR_EN_ISHI, 0x00a0, { 244, 1408, -1322 }, RC_MOUNTAIN_VILLAGE_ROCK_04 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 15, ACTOR_EN_ISHI, 0x0040, { 514, 1408, -1213 }, RC_MOUNTAIN_VILLAGE_ROCK_05 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 21, ACTOR_EN_ISHI, 0x0040, { 514, 1408, -1213 }, RC_MOUNTAIN_VILLAGE_ROCK_05 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 16, ACTOR_EN_ISHI, 0x01f0, { 654, -17, 850 }, RC_MOUNTAIN_VILLAGE_ROCK_06 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 22, ACTOR_EN_ISHI, 0x01f0, { 654, -17, 850 }, RC_MOUNTAIN_VILLAGE_ROCK_06 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 17, ACTOR_EN_ISHI, 0x01f0, { 686, -14, 808 }, RC_MOUNTAIN_VILLAGE_ROCK_07 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 23, ACTOR_EN_ISHI, 0x01f0, { 686, -14, 808 }, RC_MOUNTAIN_VILLAGE_ROCK_07 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 18, ACTOR_EN_ISHI, 0x01f0, { 656, -20, 936 }, RC_MOUNTAIN_VILLAGE_ROCK_08 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 24, ACTOR_EN_ISHI, 0x01f0, { 656, -20, 936 }, RC_MOUNTAIN_VILLAGE_ROCK_08 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 19, ACTOR_EN_ISHI, 0x01f0, { 705, -18, 973 }, RC_MOUNTAIN_VILLAGE_ROCK_09 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 25, ACTOR_EN_ISHI, 0x01f0, { 705, -18, 973 }, RC_MOUNTAIN_VILLAGE_ROCK_09 },
    { SCENE_10YUKIYAMANOMURA2, 0, 0, 20, ACTOR_EN_ISHI, 0x01f2, { 626, -20, 892 }, RC_MOUNTAIN_VILLAGE_ROCK_10 },
    { SCENE_10YUKIYAMANOMURA2, 0, 1, 26, ACTOR_EN_ISHI, 0x01f2, { 626, -20, 892 }, RC_MOUNTAIN_VILLAGE_ROCK_10 },
    { SCENE_16GORON_HOUSE, 0, 0, 2, ACTOR_EN_ISHI, 0x01f0, { -207, -440, 241 }, RC_GORON_SHRINE_ROCK_01 },
    { SCENE_16GORON_HOUSE, 0, 1, 7, ACTOR_EN_ISHI, 0x01f0, { -207, -440, 241 }, RC_GORON_SHRINE_ROCK_01 },
    { SCENE_16GORON_HOUSE, 0, 0, 3, ACTOR_EN_ISHI, 0x01f0, { -146, -440, -404 }, RC_GORON_SHRINE_ROCK_02 },
    { SCENE_16GORON_HOUSE, 0, 1, 8, ACTOR_EN_ISHI, 0x01f0, { -146, -440, -404 }, RC_GORON_SHRINE_ROCK_02 },
    { SCENE_16GORON_HOUSE, 0, 0, 4, ACTOR_EN_ISHI, 0x01f0, { -214, -440, -350 }, RC_GORON_SHRINE_ROCK_03 },
    { SCENE_16GORON_HOUSE, 0, 1, 9, ACTOR_EN_ISHI, 0x01f0, { -214, -440, -350 }, RC_GORON_SHRINE_ROCK_03 },
    { SCENE_16GORON_HOUSE, 0, 0, 5, ACTOR_EN_ISHI, 0x01f0, { -221, -440, -432 }, RC_GORON_SHRINE_ROCK_04 },
    { SCENE_16GORON_HOUSE, 0, 1, 10, ACTOR_EN_ISHI, 0x01f0, { -221, -440, -432 }, RC_GORON_SHRINE_ROCK_04 },
    { SCENE_16GORON_HOUSE, 0, 0, 6, ACTOR_EN_ISHI, 0x01f0, { 512, -320, 499 }, RC_GORON_SHRINE_ROCK_05 },
    { SCENE_16GORON_HOUSE, 0, 1, 11, ACTOR_EN_ISHI, 0x01f0, { 512, -320, 499 }, RC_GORON_SHRINE_ROCK_05 },
    { SCENE_16GORON_HOUSE, 0, 0, 7, ACTOR_EN_ISHI, 0x01f0, { 777, -320, 57 }, RC_GORON_SHRINE_ROCK_06 },
    { SCENE_16GORON_HOUSE, 0, 1, 12, ACTOR_EN_ISHI, 0x01f0, { 777, -320, 57 }, RC_GORON_SHRINE_ROCK_06 },
    { SCENE_16GORON_HOUSE, 0, 0, 8, ACTOR_EN_ISHI, 0x01f0, { 818, -320, -275 }, RC_GORON_SHRINE_ROCK_07 },
    { SCENE_16GORON_HOUSE, 0, 1, 13, ACTOR_EN_ISHI, 0x01f0, { 818, -320, -275 }, RC_GORON_SHRINE_ROCK_07 },
    { SCENE_16GORON_HOUSE, 0, 0, 9, ACTOR_EN_ISHI, 0x01f0, { 795, -320, -357 }, RC_GORON_SHRINE_ROCK_08 },
    { SCENE_16GORON_HOUSE, 0, 1, 14, ACTOR_EN_ISHI, 0x01f0, { 795, -320, -357 }, RC_GORON_SHRINE_ROCK_08 },
    { SCENE_16GORON_HOUSE, 0, 0, 10, ACTOR_EN_ISHI, 0x01f0, { -394, -134, -323 }, RC_GORON_SHRINE_ROCK_09 },
    { SCENE_16GORON_HOUSE, 0, 1, 15, ACTOR_EN_ISHI, 0x01f0, { -394, -134, -323 }, RC_GORON_SHRINE_ROCK_09 },
    { SCENE_16GORON_HOUSE, 0, 0, 11, ACTOR_EN_ISHI, 0x01f0, { -401, -134, -248 }, RC_GORON_SHRINE_ROCK_10 },
    { SCENE_16GORON_HOUSE, 0, 1, 16, ACTOR_EN_ISHI, 0x01f0, { -401, -134, -248 }, RC_GORON_SHRINE_ROCK_10 },
    { SCENE_16GORON_HOUSE, 0, 0, 12, ACTOR_EN_ISHI, 0x01f0, { -808, -134, -269 }, RC_GORON_SHRINE_ROCK_11 },
    { SCENE_16GORON_HOUSE, 0, 1, 17, ACTOR_EN_ISHI, 0x01f0, { -808, -134, -269 }, RC_GORON_SHRINE_ROCK_11 },
    { SCENE_16GORON_HOUSE, 0, 0, 13, ACTOR_EN_ISHI, 0x01f0, { -543, -134, -377 }, RC_GORON_SHRINE_ROCK_12 },
    { SCENE_16GORON_HOUSE, 0, 1, 18, ACTOR_EN_ISHI, 0x01f0, { -543, -134, -377 }, RC_GORON_SHRINE_ROCK_12 },
    { SCENE_16GORON_HOUSE, 0, 0, 14, ACTOR_EN_ISHI, 0x01f0, { -557, -134, 411 }, RC_GORON_SHRINE_ROCK_13 },
    { SCENE_16GORON_HOUSE, 0, 1, 19, ACTOR_EN_ISHI, 0x01f0, { -557, -134, 411 }, RC_GORON_SHRINE_ROCK_13 },
    { SCENE_16GORON_HOUSE, 0, 0, 15, ACTOR_EN_ISHI, 0x01f0, { -489, -134, 295 }, RC_GORON_SHRINE_ROCK_14 },
    { SCENE_16GORON_HOUSE, 0, 1, 20, ACTOR_EN_ISHI, 0x01f0, { -489, -134, 295 }, RC_GORON_SHRINE_ROCK_14 },
    { SCENE_16GORON_HOUSE, 0, 0, 16, ACTOR_EN_ISHI, 0x01f0, { -686, -134, 295 }, RC_GORON_SHRINE_ROCK_15 },
    { SCENE_16GORON_HOUSE, 0, 1, 21, ACTOR_EN_ISHI, 0x01f0, { -686, -134, 295 }, RC_GORON_SHRINE_ROCK_15 },
    { SCENE_16GORON_HOUSE, 0, 0, 17, ACTOR_EN_ISHI, 0x01f0, { -693, -134, -214 }, RC_GORON_SHRINE_ROCK_16 },
    { SCENE_16GORON_HOUSE, 0, 1, 22, ACTOR_EN_ISHI, 0x01f0, { -693, -134, -214 }, RC_GORON_SHRINE_ROCK_16 },
    { SCENE_17SETUGEN2, 0, 0, 24, ACTOR_EN_ISHI, 0x0060, { -1326, 320, 172 }, RC_PATH_TO_GORON_VILLAGE_ROCK_01 },
    { SCENE_17SETUGEN2, 0, 0, 25, ACTOR_EN_ISHI, 0x0060, { -1349, 320, 114 }, RC_PATH_TO_GORON_VILLAGE_ROCK_02 },
    { SCENE_17SETUGEN2, 0, 0, 26, ACTOR_EN_ISHI, 0x0080, { -1264, 320, 161 }, RC_PATH_TO_GORON_VILLAGE_ROCK_03 },
    { SCENE_22DEKUCITY, 1, 0, 12, ACTOR_OBJ_HAMISHI, 0x00ff, { 962, 2, 1757 }, RC_DEKU_PALACE_ROCK_01 },
    { SCENE_22DEKUCITY, 1, 0, 13, ACTOR_OBJ_HAMISHI, 0x00ff, { 639, 2, 1615 }, RC_DEKU_PALACE_ROCK_02 },
    { SCENE_22DEKUCITY, 1, 0, 14, ACTOR_OBJ_HAMISHI, 0x00ff, { 733, 2, 790 }, RC_DEKU_PALACE_ROCK_03 },
    { SCENE_30GYOSON, 0, 0, 95, ACTOR_EN_ISHI, 0x0032, { 80, 80, 5565 }, RC_GREAT_BAY_COAST_ROCK_01 },
    { SCENE_30GYOSON, 0, 1, 105, ACTOR_EN_ISHI, 0x0032, { 80, 80, 5565 }, RC_GREAT_BAY_COAST_ROCK_01 },
    { SCENE_30GYOSON, 0, 0, 96, ACTOR_EN_ISHI, 0x0032, { 1033, 80, 3267 }, RC_GREAT_BAY_COAST_ROCK_02 },
    { SCENE_30GYOSON, 0, 1, 107, ACTOR_EN_ISHI, 0x0032, { 1033, 80, 3267 }, RC_GREAT_BAY_COAST_ROCK_02 },
    { SCENE_30GYOSON, 0, 0, 97, ACTOR_EN_ISHI, 0x0032, { -55, 80, 3039 }, RC_GREAT_BAY_COAST_ROCK_03 },
    { SCENE_30GYOSON, 0, 1, 104, ACTOR_EN_ISHI, 0x0032, { -55, 80, 3039 }, RC_GREAT_BAY_COAST_ROCK_03 },
    { SCENE_30GYOSON, 0, 0, 98, ACTOR_EN_ISHI, 0x0032, { 393, 80, 4971 }, RC_GREAT_BAY_COAST_ROCK_04 },
    { SCENE_30GYOSON, 0, 1, 106, ACTOR_EN_ISHI, 0x0032, { 393, 80, 4971 }, RC_GREAT_BAY_COAST_ROCK_04 },
    { SCENE_30GYOSON, 0, 0, 99, ACTOR_EN_ISHI, 0x0032, { -190, 80, 5242 }, RC_GREAT_BAY_COAST_ROCK_05 },
    { SCENE_30GYOSON, 0, 1, 103, ACTOR_EN_ISHI, 0x0032, { -190, 80, 5242 }, RC_GREAT_BAY_COAST_ROCK_05 },
    { SCENE_30GYOSON, 0, 0, 100, ACTOR_EN_ISHI, 0x0032, { 1844, 67, -68 }, RC_GREAT_BAY_COAST_ROCK_06 },
    { SCENE_30GYOSON, 0, 1, 110, ACTOR_EN_ISHI, 0x0032, { 1844, 67, -68 }, RC_GREAT_BAY_COAST_ROCK_06 },
    { SCENE_30GYOSON, 0, 0, 101, ACTOR_EN_ISHI, 0x0032, { 1240, -32, -2152 }, RC_GREAT_BAY_COAST_ROCK_07 },
    { SCENE_30GYOSON, 0, 1, 109, ACTOR_EN_ISHI, 0x0032, { 1240, -32, -2152 }, RC_GREAT_BAY_COAST_ROCK_07 },
    { SCENE_30GYOSON, 0, 0, 102, ACTOR_EN_ISHI, 0x0032, { 1220, 26, 1251 }, RC_GREAT_BAY_COAST_ROCK_08 },
    { SCENE_30GYOSON, 0, 1, 108, ACTOR_EN_ISHI, 0x0032, { 1220, 26, 1251 }, RC_GREAT_BAY_COAST_ROCK_08 },
    { SCENE_31MISAKI, 0, 0, 32, ACTOR_EN_ISHI, 0x01f0, { -2258, 120, 3743 }, RC_ZORA_CAPE_ROCK_01 },
    { SCENE_31MISAKI, 0, 1, 32, ACTOR_EN_ISHI, 0x01f0, { -2258, 120, 3743 }, RC_ZORA_CAPE_ROCK_01 },
    { SCENE_31MISAKI, 0, 0, 33, ACTOR_EN_ISHI, 0x01f0, { -2231, 120, 3783 }, RC_ZORA_CAPE_ROCK_02 },
    { SCENE_31MISAKI, 0, 1, 33, ACTOR_EN_ISHI, 0x01f0, { -2231, 120, 3783 }, RC_ZORA_CAPE_ROCK_02 },
    { SCENE_31MISAKI, 0, 0, 34, ACTOR_EN_ISHI, 0x01f0, { -2303, 120, 3743 }, RC_ZORA_CAPE_ROCK_03 },
    { SCENE_31MISAKI, 0, 1, 34, ACTOR_EN_ISHI, 0x01f0, { -2303, 120, 3743 }, RC_ZORA_CAPE_ROCK_03 },
    { SCENE_31MISAKI, 0, 0, 35, ACTOR_EN_ISHI, 0x01f0, { -2209, 120, 3814 }, RC_ZORA_CAPE_ROCK_04 },
    { SCENE_31MISAKI, 0, 1, 35, ACTOR_EN_ISHI, 0x01f0, { -2209, 120, 3814 }, RC_ZORA_CAPE_ROCK_04 },
    { SCENE_31MISAKI, 0, 0, 36, ACTOR_EN_ISHI, 0x01f0, { -2345, 120, 3748 }, RC_ZORA_CAPE_ROCK_05 },
    { SCENE_31MISAKI, 0, 1, 36, ACTOR_EN_ISHI, 0x01f0, { -2345, 120, 3748 }, RC_ZORA_CAPE_ROCK_05 },
    { SCENE_31MISAKI, 0, 0, 37, ACTOR_EN_ISHI, 0x01f0, { 3525, -19, 2748 }, RC_ZORA_CAPE_ROCK_06 },
    { SCENE_31MISAKI, 0, 1, 37, ACTOR_EN_ISHI, 0x01f0, { 3525, -19, 2748 }, RC_ZORA_CAPE_ROCK_06 },
    { SCENE_31MISAKI, 0, 0, 38, ACTOR_EN_ISHI, 0x01f0, { 3541, -17, 2699 }, RC_ZORA_CAPE_ROCK_07 },
    { SCENE_31MISAKI, 0, 1, 38, ACTOR_EN_ISHI, 0x01f0, { 3541, -17, 2699 }, RC_ZORA_CAPE_ROCK_07 },
    { SCENE_31MISAKI, 0, 0, 39, ACTOR_EN_ISHI, 0x01f0, { 3570, -9, 2758 }, RC_ZORA_CAPE_ROCK_08 },
    { SCENE_31MISAKI, 0, 1, 39, ACTOR_EN_ISHI, 0x01f0, { 3570, -9, 2758 }, RC_ZORA_CAPE_ROCK_08 },
    { SCENE_31MISAKI, 0, 0, 40, ACTOR_EN_ISHI, 0x01f0, { 3559, -9, 2654 }, RC_ZORA_CAPE_ROCK_09 },
    { SCENE_31MISAKI, 0, 1, 40, ACTOR_EN_ISHI, 0x01f0, { 3559, -9, 2654 }, RC_ZORA_CAPE_ROCK_09 },
    { SCENE_31MISAKI, 0, 0, 41, ACTOR_EN_ISHI, 0x01f0, { 3619, -4, 2761 }, RC_ZORA_CAPE_ROCK_10 },
    { SCENE_31MISAKI, 0, 1, 41, ACTOR_EN_ISHI, 0x01f0, { 3619, -4, 2761 }, RC_ZORA_CAPE_ROCK_10 },
    { SCENE_31MISAKI, 0, 0, 42, ACTOR_OBJ_BOMBIWA, 0x0100, { -1614, 200, 4382 }, RC_ZORA_CAPE_ROCK_11 },
    { SCENE_31MISAKI, 0, 1, 42, ACTOR_OBJ_BOMBIWA, 0x0100, { -1614, 200, 4382 }, RC_ZORA_CAPE_ROCK_11 },
    { SCENE_31MISAKI, 0, 0, 43, ACTOR_OBJ_BOMBIWA, 0x0101, { -1695, 200, 4362 }, RC_ZORA_CAPE_ROCK_12 },
    { SCENE_31MISAKI, 0, 1, 43, ACTOR_OBJ_BOMBIWA, 0x0101, { -1695, 200, 4362 }, RC_ZORA_CAPE_ROCK_12 },
    { SCENE_31MISAKI, 0, 0, 44, ACTOR_OBJ_BOMBIWA, 0x8002, { -562, 80, 2707 }, RC_ZORA_CAPE_ROCK_GROTTO },
    { SCENE_31MISAKI, 0, 1, 44, ACTOR_OBJ_BOMBIWA, 0x8002, { -562, 80, 2707 }, RC_ZORA_CAPE_ROCK_GROTTO },
    { SCENE_31MISAKI, 0, 0, 53, ACTOR_EN_ISHI, 0x0032, { 668, 12, 605 }, RC_ZORA_CAPE_ROCK_13 },
    { SCENE_31MISAKI, 0, 0, 54, ACTOR_EN_ISHI, 0x0032, { -45, 6, 734 }, RC_ZORA_CAPE_ROCK_14 },
    { SCENE_31MISAKI, 0, 0, 55, ACTOR_EN_ISHI, 0x0032, { 1549, 12, 1159 }, RC_ZORA_CAPE_ROCK_15 },
    { SCENE_31MISAKI, 0, 0, 56, ACTOR_EN_ISHI, 0x0032, { 2064, 12, 1595 }, RC_ZORA_CAPE_ROCK_16 },
    { SCENE_31MISAKI, 0, 0, 57, ACTOR_EN_ISHI, 0x0032, { 3716, 6, 2377 }, RC_ZORA_CAPE_ROCK_17 },
    { SCENE_BOTI, 1, 0, 44, ACTOR_OBJ_MURE2, 0x0402, { 106, 314, -1777 }, RC_IKANA_GRAVEYARD_ROCK_RING_01 },
    { SCENE_BOTI, 1, 1, 6, ACTOR_OBJ_MURE2, 0x0402, { 110, 314, -1884 }, RC_IKANA_GRAVEYARD_ROCK_RING_01 },
    { SCENE_IKANAMAE, 0, 0, 3, ACTOR_EN_GOROIWA, 0x4d01, { 3775, 1461, -1170 }, RC_ROAD_TO_IKANA_ROCK_ROLLING_01 },
    { SCENE_IKANAMAE, 0, 0, 4, ACTOR_EN_GOROIWA, 0x4d02, { 4443, 1462, 45 }, RC_ROAD_TO_IKANA_ROCK_ROLLING_02 },
    { SCENE_IKANAMAE, 0, 0, 5, ACTOR_OBJ_HAMISHI, 0x00ff, { -426, 200, -334 }, RC_ROAD_TO_IKANA_ROCK_01 },
    { SCENE_IKANAMAE, 0, 0, 35, ACTOR_OBJ_MURE2, 0x1f02, { 1740, 200, -94 }, RC_ROAD_TO_IKANA_ROCK_RING_01 },
    { SCENE_IKANAMAE, 0, 0, 60, ACTOR_OBJ_BOMBIWA, 0x0100, { 3166, 515, -895 }, RC_ROAD_TO_IKANA_ROCK_02 },
    { SCENE_IKANAMAE, 0, 0, 62, ACTOR_OBJ_BOMBIWA, 0x0101, { 2528, 475, -1061 }, RC_ROAD_TO_IKANA_ROCK_03 },
    { SCENE_IKANAMAE, 0, 0, 64, ACTOR_OBJ_BOMBIWA, 0x0102, { 3333, 564, -547 }, RC_ROAD_TO_IKANA_ROCK_04 },
    { SCENE_IKANAMAE, 0, 0, 65, ACTOR_OBJ_BOMBIWA, 0x0103, { 3949, 575, -685 }, RC_ROAD_TO_IKANA_ROCK_05 },
    { SCENE_IKANAMAE, 0, 0, 66, ACTOR_OBJ_BOMBIWA, 0x0104, { 4510, 575, -264 }, RC_ROAD_TO_IKANA_ROCK_06 },
    { SCENE_IKANAMAE, 0, 0, 67, ACTOR_OBJ_BOMBIWA, 0x0105, { 4836, 575, -402 }, RC_ROAD_TO_IKANA_ROCK_07 },
    { SCENE_IKANAMAE, 0, 0, 68, ACTOR_OBJ_BOMBIWA, 0x0106, { 5047, 575, -54 }, RC_ROAD_TO_IKANA_ROCK_08 },
    { SCENE_F40, 0, 0, 27, ACTOR_EN_GOROIWA, 0x4104, { 674, -1320, 1129 }, RC_STONE_TOWER_ROCK_ROLLING_01 },
    { SCENE_F40, 0, 0, 28, ACTOR_EN_GOROIWA, 0x4105, { 553, -1330, 1129 }, RC_STONE_TOWER_ROCK_ROLLING_02 },
    { SCENE_F40, 0, 0, 29, ACTOR_EN_GOROIWA, 0x4103, { -299, -2282, 1165 }, RC_STONE_TOWER_ROCK_ROLLING_03 },
    { SCENE_F40, 0, 0, 30, ACTOR_EN_GOROIWA, 0x4102, { -297, -2697, 1328 }, RC_STONE_TOWER_ROCK_ROLLING_04 },
    { SCENE_F40, 0, 0, 31, ACTOR_EN_GOROIWA, 0x4106, { 1297, -701, 2659 }, RC_STONE_TOWER_ROCK_ROLLING_05 },
    { SCENE_F40, 0, 0, 32, ACTOR_EN_GOROIWA, 0x4100, { -854, -1104, 2913 }, RC_STONE_TOWER_ROCK_ROLLING_06 },
    { SCENE_F40, 0, 0, 33, ACTOR_EN_GOROIWA, 0x4101, { 194, -1700, 3339 }, RC_STONE_TOWER_ROCK_ROLLING_07 },
    { SCENE_F40, 0, 0, 34, ACTOR_EN_GOROIWA, 0x4107, { 719, -518, 3412 }, RC_STONE_TOWER_ROCK_ROLLING_08 },
    { SCENE_F41, 0, 0, 3, ACTOR_EN_GOROIWA, 0x4106, { 689, 1972, 1324 }, RC_INVERTED_STONE_TOWER_ROCK_ROLLING_01 },
    { SCENE_F41, 0, 0, 4, ACTOR_EN_GOROIWA, 0x4107, { 511, 1981, 1354 }, RC_INVERTED_STONE_TOWER_ROCK_ROLLING_02 },
    { SCENE_F41, 0, 0, 5, ACTOR_EN_GOROIWA, 0x4108, { 688, 1126, -1204 }, RC_INVERTED_STONE_TOWER_ROCK_ROLLING_03 },
    { SCENE_F41, 0, 0, 6, ACTOR_EN_GOROIWA, 0x4109, { -305, 3325, 1196 }, RC_INVERTED_STONE_TOWER_ROCK_ROLLING_04 },
    { SCENE_F41, 0, 0, 7, ACTOR_EN_GOROIWA, 0x410a, { -308, 460, 1277 }, RC_INVERTED_STONE_TOWER_ROCK_ROLLING_05 },
    { SCENE_F41, 0, 0, 21, ACTOR_EN_ISHI, 0x0042, { 289, 854, -313 }, RC_INVERTED_STONE_TOWER_ROCK_01 },
    { SCENE_F41, 0, 0, 22, ACTOR_EN_ISHI, 0x0040, { -92, 854, -690 }, RC_INVERTED_STONE_TOWER_ROCK_02 },
    { SCENE_KINSTA1, 0, 0, 3, ACTOR_EN_ISHI, 0x006a, { -176, 120, -5 }, RC_SWAMP_SPIDER_HOUSE_ROCK_01 },
    { SCENE_KINSTA1, 0, 0, 4, ACTOR_EN_ISHI, 0x006a, { 180, 120, 92 }, RC_SWAMP_SPIDER_HOUSE_ROCK_02 },
    { SCENE_KINSTA1, 4, 0, 5, ACTOR_EN_ISHI, 0x006a, { 648, 180, -1355 }, RC_SWAMP_SPIDER_HOUSE_ROCK_03 },
};

struct RockInstance {
    RandoCheckId checkId;
    bool suppressVanillaDrop = false;
};

static ObjectExtension::Register<RockInstance> rockInstanceRegistration;

struct FailedDrop {
    Vec3f pos;
    s8 room;
};
static std::map<RandoCheckId, FailedDrop> failedDrops;

RockInstance* GetRock(Actor* actor) {
    return ObjectExtension::GetInstance().Get<RockInstance>(actor);
}

RandoCheckId IdentifyRock(Actor* actor) {
    const s16 index = GetActorListIndex(actor);
    if (index < 0) {
        return RC_UNKNOWN;
    }

    RandoCheckId baseLayerCheck = RC_UNKNOWN;
    for (const auto& placement : rockPlacements) {
        if (placement.scene != gPlayState->sceneId || placement.room != actor->room || placement.index != index ||
            placement.actorId != actor->id || placement.params != static_cast<u16>(actor->params)) {
            continue;
        }

        if (placement.layer == gSaveContext.sceneLayer) {
            return placement.checkId;
        }

        // A scene layer can use the room's base header. Require the original placement in that case.
        const Vec3f& home = actor->home.pos;
        if (placement.layer == 0 && home.x == placement.pos.x && home.y == placement.pos.y &&
            home.z == placement.pos.z) {
            baseLayerCheck = placement.checkId;
        }
    }
    return baseLayerCheck;
}

void BindRock(Actor* actor, RandoCheckId id) {
    if (ObjectMarker::IsOpen(id)) {
        ObjectExtension::GetInstance().Set<RockInstance>(actor, { id });
    }
}

bool CanDrop(RandoCheckId id) {
    return ObjectMarker::IsOpen(id) && !RANDO_SAVE_CHECKS[id].eligible && RANDO_SAVE_CHECKS[id].rockDropPending;
}

void CollectRockDrop(Actor* actor, PlayState*) {
    auto& check = RANDO_SAVE_CHECKS[CUSTOM_ITEM_PARAM];
    check.rockDropPending = false;
    if (ObjectMarker::IsOpen(static_cast<RandoCheckId>(CUSTOM_ITEM_PARAM))) {
        check.eligible = true;
    }
}

void DrawRockDrop(Actor* actor, PlayState*) {
    const auto id = static_cast<RandoCheckId>(CUSTOM_ITEM_PARAM);
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    DrawItem(ConvertItem(RANDO_SAVE_CHECKS[id].randoItemId, id), id, actor);
}

// A room may still have its previous actors during a transition. Check the live list before spawning a recovery drop.
bool TrySpawnRockDrop(RandoCheckId id, Vec3f pos, bool toss) {
    if (!CanDrop(id)) {
        return true;
    }
    for (Actor* drop = gPlayState->actorCtx.actorLists[ACTORCAT_MISC].first; drop != nullptr; drop = drop->next) {
        if (drop->update != nullptr && GetObjectRandoCheckId(drop) == id) {
            return true;
        }
    }
    s16 flags = CustomItem::KILL_ON_TOUCH | CustomItem::ABLE_TO_ZORA_RANG;
    if (toss) {
        flags |= CustomItem::TOSS_ON_SPAWN;
    }
    EnItem00* drop = CustomItem::Spawn(pos.x, pos.y, pos.z, 0, flags, id, CollectRockDrop, DrawRockDrop);
    if (drop == nullptr) {
        return false;
    }
    SetObjectRandoCheckId(&drop->actor, id);
    return true;
}

Vec3f GetDropPosition(Actor* rock) {
    Vec3f pos = rock->id == ACTOR_EN_GOROIWA ? rock->world.pos : rock->home.pos;
    CollisionPoly* floorPoly;
    s32 bgId;
    pos.y += 30.0f;
    f32 floor = BgCheck_EntityRaycastFloor5(&gPlayState->colCtx, &floorPoly, &bgId, rock, &pos);
    pos.y -= 30.0f;
    if (floor > BGCHECK_Y_MIN && pos.y - floor <= 140.0f) {
        pos.y = floor;
    } else if (rock->id == ACTOR_EN_GOROIWA) {
        // A boulder can be struck as it leaves the edge of the path.
        pos = GET_PLAYER(gPlayState)->actor.world.pos;
    }
    return pos;
}

void SpawnOrRetry(Actor* rock, RandoCheckId id, bool toss) {
    const Vec3f pos = GetDropPosition(rock);
    const auto room = gPlayState->roomCtx.curRoom.num;
    if (!TrySpawnRockDrop(id, pos, toss)) {
        failedDrops.insert_or_assign(id, FailedDrop{ pos, room });
    }
}

void RecoverRockDrop(Actor* actor) {
    const auto* rock = GetRock(actor);
    if (rock != nullptr && CanDrop(rock->checkId) && actor->room == gPlayState->roomCtx.curRoom.num) {
        SpawnOrRetry(actor, rock->checkId, false);
    }
}

void MarkRock(Actor* actor) {
    if (const auto* rock = GetRock(actor)) {
        ObjectMarker::AttachTint(actor, rock->checkId, ObjectMarker::TintPasses::OpaqueAndTranslucent);
    }
}

void OpenRock(Actor* actor) {
    auto* rock = GetRock(actor);
    if (rock == nullptr || !ObjectMarker::IsOpen(rock->checkId)) {
        return;
    }

    auto& check = RANDO_SAVE_CHECKS[rock->checkId];
    rock->suppressVanillaDrop = true;
    if (!check.eligible && !check.rockDropPending) {
        check.rockDropPending = true;
        if (actor->room == gPlayState->roomCtx.curRoom.num) {
            SpawnOrRetry(actor, rock->checkId, true);
        }
    }
}

void RetryFailedDrops() {
    if (gPlayState == nullptr) {
        return;
    }
    const auto room = gPlayState->roomCtx.curRoom.num;
    std::erase_if(failedDrops, [room](const auto& entry) {
        return entry.second.room != room || TrySpawnRockDrop(entry.first, entry.second.pos, false);
    });
}

} // namespace

void InitRockBehavior() {
    const bool enabled = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_ROCKS] != RO_GENERIC_OFF;
    failedDrops.clear();

    COND_HOOK(ShouldActorInit, enabled, [](Actor* actor, bool* should) {
        if (*should && (actor->id == ACTOR_EN_ISHI || actor->id == ACTOR_OBJ_BOMBIWA ||
                        actor->id == ACTOR_OBJ_HAMISHI || actor->id == ACTOR_EN_GOROIWA)) {
            BindRock(actor, IdentifyRock(actor));
        }
    });
    COND_HOOK(OnActorInit, enabled, [](Actor* actor) {
        RecoverRockDrop(actor);
        MarkRock(actor);
    });
    // En_Ishi sets its draw callback after its object slot finishes loading.
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_ISHI, enabled, MarkRock);
    COND_HOOK(OnRoomInit, enabled, [](s8, s8) { failedDrops.clear(); });
    COND_HOOK(OnGameStateUpdate, enabled, RetryFailedDrops);

    COND_VB_SHOULD(VB_OBJ_MURE2_SET_CHILD_ROOM, enabled, {
        auto* spawner = va_arg(args, ObjMure2*);
        const s32 childIndex = va_arg(args, s32);
        Actor* child = spawner->actors[childIndex];
        if (child != nullptr && child->id == ACTOR_EN_ISHI) {
            const auto base = IdentifyRock(&spawner->actor);
            if (base != RC_UNKNOWN) {
                BindRock(child, static_cast<RandoCheckId>(base + childIndex));
                if (spawner->actor.room == gPlayState->roomCtx.curRoom.num) {
                    RecoverRockDrop(child);
                }
            }
        }
    });

    COND_VB_SHOULD(VB_ROCK_INTERACT, enabled, { OpenRock(va_arg(args, Actor*)); });
    COND_VB_SHOULD(VB_ROCK_DROP_VANILLA, enabled, {
        if (const auto* rock = GetRock(va_arg(args, Actor*)); rock != nullptr && rock->suppressVanillaDrop) {
            *should = false;
        }
    });
}

} // namespace Rando::ActorBehavior
