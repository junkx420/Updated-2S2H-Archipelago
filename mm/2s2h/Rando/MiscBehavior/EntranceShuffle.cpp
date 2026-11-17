#include "EntranceShuffle.h"
#include "Rando/Logic/Logic.h"
#include "2s2h/GameInteractor/GameInteractor.h"

#include <algorithm>

extern "C" {
#include "variables.h"
#include "functions.h"
#include "z64horse.h"
}

namespace Rando {

namespace EntranceShuffle {

// Time macros in shuffleAreaRules (AFTER, ...) live in Rando::Logic, like in the region files.
using namespace Rando::Logic;

// clang-format off
const std::vector<ShuffleEntrance> shuffleEntrances = {
    // option,                       into,                              outOf,                    inside,         outside,            boss room,        boss warp lands at
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(WOODFALL_TEMPLE, 0),    ENTRANCE(WOODFALL, 1),    SCENE_MITURIN,  SCENE_21MITURINMAE, SCENE_MITURIN_BS, { ENTRANCE(WOODFALL_TEMPLE, 1), ENTRANCE(WOODFALL, 0) } },
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(SNOWHEAD_TEMPLE, 0),    ENTRANCE(SNOWHEAD, 1),    SCENE_HAKUGIN,  SCENE_12HAKUGINMAE, SCENE_HAKUGIN_BS, { ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7) } },
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(GREAT_BAY_TEMPLE, 0),   ENTRANCE(ZORA_CAPE, 7),   SCENE_SEA,      SCENE_31MISAKI,     SCENE_SEA_BS,     { ENTRANCE(ZORA_CAPE, 9), ENTRANCE(ZORA_CAPE, 8) } },
    // The inverted exterior is not shuffled. Boss rows below resolve its boss warp separately.
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(STONE_TOWER_TEMPLE, 0), ENTRANCE(STONE_TOWER, 2), SCENE_INISIE_N, SCENE_F40,          SCENE_MAX,        {} },
    // Boss doors and blue warps (Regions/*Temple.cpp); append only, including the inverted temple's own boss door.
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(ODOLWAS_LAIR, 0),   ENTRANCE(WOODFALL_TEMPLE, 1),         SCENE_MITURIN_BS, SCENE_MITURIN,  SCENE_MITURIN_BS, { ENTRANCE(WOODFALL_TEMPLE, 1), ENTRANCE(WOODFALL, 0) }, ENTRANCE(WOODFALL, 1) },
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(GOHTS_LAIR, 0),     ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7), SCENE_HAKUGIN_BS, SCENE_HAKUGIN,  SCENE_HAKUGIN_BS, { ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7) }, ENTRANCE(SNOWHEAD, 1) },
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(GYORGS_LAIR, 0),    ENTRANCE(ZORA_CAPE, 9),              SCENE_SEA_BS,     SCENE_SEA,      SCENE_SEA_BS,     { ENTRANCE(ZORA_CAPE, 9), ENTRANCE(ZORA_CAPE, 8) }, ENTRANCE(ZORA_CAPE, 7) },
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(TWINMOLDS_LAIR, 0), ENTRANCE(IKANA_CANYON, 15),          SCENE_INISIE_BS,  SCENE_INISIE_R, SCENE_INISIE_BS,  { ENTRANCE(IKANA_CANYON, 15) }, ENTRANCE(STONE_TOWER_INVERTED, 1) },
    // Grottos (Regions/*.cpp, "ENTRANCE(GROTTOS"), logic entrances; the game side is in grottoEntrances below.
    // outOf is the logic's way back out of the grotto region (not unique: several grottos return to TERMINA_FIELD:0).
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 0),  ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 1),  ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 2),  ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 3),  ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 5),  ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), SCENE_KAKUSIANA, SCENE_17SETUGEN,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 7),  ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 9),  ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 11), ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 12), ENTRANCE(DEKU_PALACE, 9),                  SCENE_KAKUSIANA, SCENE_22DEKUCITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 13), ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 16), ENTRANCE(GORON_VILLAGE_WINTER, 3),         SCENE_KAKUSIANA, SCENE_11GORONNOSATO,    SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 17), ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 18), ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 19), ENTRANCE(TERMINA_FIELD, 0),                SCENE_KAKUSIANA, SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 20), ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0),       SCENE_KAKUSIANA, SCENE_24KEMONOMITI,     SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 21), ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),      SCENE_KAKUSIANA, SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 22), ENTRANCE(WOODS_OF_MYSTERY, 0),             SCENE_KAKUSIANA, SCENE_26SARUNOMORI,     SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 23), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 0),      SCENE_KAKUSIANA, SCENE_10YUKIYAMANOMURA, SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 24), ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), SCENE_KAKUSIANA, SCENE_17SETUGEN,        SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 25), ENTRANCE(PATH_TO_SNOWHEAD, 1),             SCENE_KAKUSIANA, SCENE_14YUKIDAMANOMITI, SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 26), ENTRANCE(GREAT_BAY_COAST, 0),              SCENE_KAKUSIANA, SCENE_30GYOSON,         SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 27), ENTRANCE(GREAT_BAY_COAST, 0),              SCENE_KAKUSIANA, SCENE_30GYOSON,         SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 28), ENTRANCE(ZORA_CAPE, 0),                    SCENE_KAKUSIANA, SCENE_31MISAKI,         SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 29), ENTRANCE(IKANA_CANYON, 0),                 SCENE_KAKUSIANA, SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 30), ENTRANCE(IKANA_GRAVEYARD, 0),              SCENE_KAKUSIANA, SCENE_BOTI,             SCENE_MAX, {} },
    { RO_SHUFFLE_GROTTOS, ENTRANCE(GROTTOS, 31), ENTRANCE(ROAD_TO_IKANA, 0),                SCENE_KAKUSIANA, SCENE_IKANAMAE,         SCENE_MAX, {} },
    // Interiors (Regions/*.cpp): knob doors from outside into a dead-end building, one row per door; their hours are in
    // interiorEntrances below. outsideScene is the vanilla exterior; for interiors the redirect does not test it,
    // because the exterior may be its cleared or spring variant (Southern Swamp, Mountain Village).
    { RO_SHUFFLE_INTERIORS, ENTRANCE(TRADING_POST, 0),            ENTRANCE(WEST_CLOCK_TOWN, 5),          SCENE_8ITEMSHOP,    SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(BOMB_SHOP, 0),               ENTRANCE(WEST_CLOCK_TOWN, 6),          SCENE_BOMYA,        SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(CURIOSITY_SHOP, 0),          ENTRANCE(WEST_CLOCK_TOWN, 4),          SCENE_AYASHIISHOP,  SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(CURIOSITY_SHOP, 1),          ENTRANCE(LAUNDRY_POOL, 1),             SCENE_AYASHIISHOP,  SCENE_ALLEY,            SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(SWORDMANS_SCHOOL, 0),        ENTRANCE(WEST_CLOCK_TOWN, 3),          SCENE_DOUJOU,       SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(POST_OFFICE, 0),             ENTRANCE(WEST_CLOCK_TOWN, 7),          SCENE_POSTHOUSE,    SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(LOTTERY_SHOP, 0),            ENTRANCE(WEST_CLOCK_TOWN, 8),          SCENE_TAKARAKUJI,   SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(HONEY_AND_DARLINGS_SHOP, 0), ENTRANCE(EAST_CLOCK_TOWN, 6),          SCENE_BOWLING,      SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(MILK_BAR, 0),                ENTRANCE(EAST_CLOCK_TOWN, 11),         SCENE_MILK_BAR,     SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(MAYORS_RESIDENCE, 0),        ENTRANCE(EAST_CLOCK_TOWN, 7),          SCENE_SONCHONOIE,   SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(TOWN_SHOOTING_GALLERY, 0),   ENTRANCE(EAST_CLOCK_TOWN, 8),          SCENE_SYATEKI_MIZU, SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(TREASURE_CHEST_SHOP, 0),     ENTRANCE(EAST_CLOCK_TOWN, 4),          SCENE_TAKARAYA,     SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(STOCK_POT_INN, 0),           ENTRANCE(EAST_CLOCK_TOWN, 9),          SCENE_YADOYA,       SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(STOCK_POT_INN, 1),           ENTRANCE(EAST_CLOCK_TOWN, 10),         SCENE_YADOYA,       SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(DEKU_SCRUB_PLAYGROUND, 0),   ENTRANCE(NORTH_CLOCK_TOWN, 4),         SCENE_DEKUTES,      SCENE_BACKTOWN,         SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(TOURIST_INFORMATION, 0),     ENTRANCE(SOUTHERN_SWAMP_POISONED, 1),  SCENE_MAP_SHOP,     SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(MAGIC_HAGS_POTION_SHOP, 0),  ENTRANCE(SOUTHERN_SWAMP_POISONED, 5),  SCENE_WITCH_SHOP,   SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(SWAMP_SHOOTING_GALLERY, 0),  ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 2),   SCENE_SYATEKI_MORI, SCENE_24KEMONOMITI,     SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(MOUNTAIN_SMITHY, 0),         ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 1),  SCENE_KAJIYA,       SCENE_10YUKIYAMANOMURA, SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(RANCH_HOUSE, 0),             ENTRANCE(ROMANI_RANCH, 2),             SCENE_OMOYA,        SCENE_F01,              SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(RANCH_HOUSE, 1),             ENTRANCE(ROMANI_RANCH, 3),             SCENE_OMOYA,        SCENE_F01,              SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(CUCCO_SHACK, 0),             ENTRANCE(ROMANI_RANCH, 4),             SCENE_F01C,         SCENE_F01,              SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(DOGGY_RACETRACK, 0),         ENTRANCE(ROMANI_RANCH, 5),             SCENE_F01_B,        SCENE_F01,              SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(MARINE_RESEARCH_LAB, 0),     ENTRANCE(GREAT_BAY_COAST, 7),          SCENE_LABO,         SCENE_30GYOSON,         SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(FISHERMANS_HUT, 0),          ENTRANCE(GREAT_BAY_COAST, 4),          SCENE_FISHERMAN,    SCENE_30GYOSON,         SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(ZORA_HALL_ROOMS, 5),         ENTRANCE(ZORA_HALL, 2),                SCENE_BANDROOM,     SCENE_33ZORACITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(ZORA_HALL_ROOMS, 2),         ENTRANCE(ZORA_HALL, 3),                SCENE_BANDROOM,     SCENE_33ZORACITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(ZORA_HALL_ROOMS, 3),         ENTRANCE(ZORA_HALL, 4),                SCENE_BANDROOM,     SCENE_33ZORACITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(ZORA_HALL_ROOMS, 1),         ENTRANCE(ZORA_HALL, 5),                SCENE_BANDROOM,     SCENE_33ZORACITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(ZORA_HALL_ROOMS, 0),         ENTRANCE(ZORA_HALL, 6),                SCENE_BANDROOM,     SCENE_33ZORACITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(MUSIC_BOX_HOUSE, 0),         ENTRANCE(IKANA_CANYON, 2),             SCENE_MUSICHOUSE,   SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(GHOST_HUT, 0),               ENTRANCE(IKANA_CANYON, 1),             SCENE_TOUGITES,     SCENE_IKANA,            SCENE_MAX, {} },
    // Overworld (Regions/*.cpp): two rows per transition, one per direction. A row is one exit: "into" is where it
    // arrives (in insideScene), "outOf" is the arrival of the opposite direction (the next row's "into"), outsideScene is
    // where the exit is. Every row names a place, never a scene variant: base entrances (WINTER, POISONED) and scenes;
    // z_play.c Play_Init picks the spring/cleared scene and the scene layer. One-way, water and cutscene ways stay out.
    // Clock Town (Central.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(EAST_CLOCK_TOWN, 1),              ENTRANCE(SOUTH_CLOCK_TOWN, 7),             SCENE_TOWN,             SCENE_CLOCKTOWER,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTH_CLOCK_TOWN, 7),             ENTRANCE(EAST_CLOCK_TOWN, 1),              SCENE_CLOCKTOWER,       SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(EAST_CLOCK_TOWN, 3),              ENTRANCE(SOUTH_CLOCK_TOWN, 2),             SCENE_TOWN,             SCENE_CLOCKTOWER,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTH_CLOCK_TOWN, 2),             ENTRANCE(EAST_CLOCK_TOWN, 3),              SCENE_CLOCKTOWER,       SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(EAST_CLOCK_TOWN, 5),              ENTRANCE(NORTH_CLOCK_TOWN, 1),             SCENE_TOWN,             SCENE_BACKTOWN,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(NORTH_CLOCK_TOWN, 1),             ENTRANCE(EAST_CLOCK_TOWN, 5),              SCENE_BACKTOWN,         SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(NORTH_CLOCK_TOWN, 2),             ENTRANCE(SOUTH_CLOCK_TOWN, 4),             SCENE_BACKTOWN,         SCENE_CLOCKTOWER,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTH_CLOCK_TOWN, 4),             ENTRANCE(NORTH_CLOCK_TOWN, 2),             SCENE_CLOCKTOWER,       SCENE_BACKTOWN,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(WEST_CLOCK_TOWN, 2),              ENTRANCE(SOUTH_CLOCK_TOWN, 3),             SCENE_ICHIBA,           SCENE_CLOCKTOWER,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTH_CLOCK_TOWN, 3),             ENTRANCE(WEST_CLOCK_TOWN, 2),              SCENE_CLOCKTOWER,       SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(WEST_CLOCK_TOWN, 1),              ENTRANCE(SOUTH_CLOCK_TOWN, 5),             SCENE_ICHIBA,           SCENE_CLOCKTOWER,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTH_CLOCK_TOWN, 5),             ENTRANCE(WEST_CLOCK_TOWN, 1),              SCENE_CLOCKTOWER,       SCENE_ICHIBA,           SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(LAUNDRY_POOL, 0),                 ENTRANCE(SOUTH_CLOCK_TOWN, 6),             SCENE_ALLEY,            SCENE_CLOCKTOWER,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTH_CLOCK_TOWN, 6),             ENTRANCE(LAUNDRY_POOL, 0),                 SCENE_CLOCKTOWER,       SCENE_ALLEY,            SCENE_MAX, {} },
    // Clock Town gates (TerminaField.cpp, Central.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTH_CLOCK_TOWN, 1),             ENTRANCE(TERMINA_FIELD, 6),                SCENE_CLOCKTOWER,       SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 6),                ENTRANCE(SOUTH_CLOCK_TOWN, 1),             SCENE_00KEIKOKU,        SCENE_CLOCKTOWER,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(EAST_CLOCK_TOWN, 0),              ENTRANCE(TERMINA_FIELD, 7),                SCENE_TOWN,             SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 7),                ENTRANCE(EAST_CLOCK_TOWN, 0),              SCENE_00KEIKOKU,        SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(NORTH_CLOCK_TOWN, 0),             ENTRANCE(TERMINA_FIELD, 8),                SCENE_BACKTOWN,         SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 8),                ENTRANCE(NORTH_CLOCK_TOWN, 0),             SCENE_00KEIKOKU,        SCENE_BACKTOWN,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(WEST_CLOCK_TOWN, 0),              ENTRANCE(TERMINA_FIELD, 0),                SCENE_ICHIBA,           SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 0),                ENTRANCE(WEST_CLOCK_TOWN, 0),              SCENE_00KEIKOKU,        SCENE_ICHIBA,           SCENE_MAX, {} },
    // Termina Field (TerminaField.cpp, South.cpp, East.cpp, MilkRoad.cpp, North.cpp, West.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0),       ENTRANCE(TERMINA_FIELD, 1),                SCENE_24KEMONOMITI,     SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 1),                ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0),       SCENE_00KEIKOKU,        SCENE_24KEMONOMITI,     SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ROAD_TO_IKANA, 0),                ENTRANCE(TERMINA_FIELD, 4),                SCENE_IKANAMAE,         SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 4),                ENTRANCE(ROAD_TO_IKANA, 0),                SCENE_00KEIKOKU,        SCENE_IKANAMAE,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MILK_ROAD, 0),                    ENTRANCE(TERMINA_FIELD, 5),                SCENE_ROMANYMAE,        SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 5),                ENTRANCE(MILK_ROAD, 0),                    SCENE_00KEIKOKU,        SCENE_ROMANYMAE,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 0),     ENTRANCE(TERMINA_FIELD, 3),                SCENE_13HUBUKINOMITI,   SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 3),                ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 0),     SCENE_00KEIKOKU,        SCENE_13HUBUKINOMITI,   SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GREAT_BAY_COAST, 0),              ENTRANCE(TERMINA_FIELD, 2),                SCENE_30GYOSON,         SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 2),                ENTRANCE(GREAT_BAY_COAST, 0),              SCENE_00KEIKOKU,        SCENE_30GYOSON,         SCENE_MAX, {} },
    // Southern Swamp (South.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTHERN_SWAMP_POISONED, 0),      ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 1),       SCENE_20SICHITAI,       SCENE_24KEMONOMITI,     SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 1),       ENTRANCE(SOUTHERN_SWAMP_POISONED, 0),      SCENE_24KEMONOMITI,     SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(WOODS_OF_MYSTERY, 0),             ENTRANCE(SOUTHERN_SWAMP_POISONED, 7),      SCENE_26SARUNOMORI,     SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTHERN_SWAMP_POISONED, 7),      ENTRANCE(WOODS_OF_MYSTERY, 0),             SCENE_20SICHITAI,       SCENE_26SARUNOMORI,     SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(DEKU_PALACE, 0),                  ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),      SCENE_22DEKUCITY,       SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),      ENTRANCE(DEKU_PALACE, 0),                  SCENE_20SICHITAI,       SCENE_22DEKUCITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(DEKU_PALACE, 5),                  ENTRANCE(SOUTHERN_SWAMP_POISONED, 4),      SCENE_22DEKUCITY,       SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTHERN_SWAMP_POISONED, 4),      ENTRANCE(DEKU_PALACE, 5),                  SCENE_20SICHITAI,       SCENE_22DEKUCITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(WOODFALL, 0),                     ENTRANCE(SOUTHERN_SWAMP_POISONED, 2),      SCENE_21MITURINMAE,     SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SOUTHERN_SWAMP_POISONED, 2),      ENTRANCE(WOODFALL, 0),                     SCENE_20SICHITAI,       SCENE_21MITURINMAE,     SCENE_MAX, {} },
    // Milk Road (MilkRoad.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ROMANI_RANCH, 0),                 ENTRANCE(MILK_ROAD, 1),                    SCENE_F01,              SCENE_ROMANYMAE,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MILK_ROAD, 1),                    ENTRANCE(ROMANI_RANCH, 0),                 SCENE_ROMANYMAE,        SCENE_F01,              SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GORMAN_TRACK, 0),                 ENTRANCE(MILK_ROAD, 3),                    SCENE_KOEPONARACE,      SCENE_ROMANYMAE,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MILK_ROAD, 3),                    ENTRANCE(GORMAN_TRACK, 0),                 SCENE_ROMANYMAE,        SCENE_KOEPONARACE,      SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GORMAN_TRACK, 3),                 ENTRANCE(MILK_ROAD, 2),                    SCENE_KOEPONARACE,      SCENE_ROMANYMAE,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MILK_ROAD, 2),                    ENTRANCE(GORMAN_TRACK, 3),                 SCENE_ROMANYMAE,        SCENE_KOEPONARACE,      SCENE_MAX, {} },
    // Mountains (North.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 6),      ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 1),     SCENE_10YUKIYAMANOMURA, SCENE_13HUBUKINOMITI,   SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 1),     ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 6),      SCENE_13HUBUKINOMITI,   SCENE_10YUKIYAMANOMURA, SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 2),      SCENE_17SETUGEN,        SCENE_10YUKIYAMANOMURA, SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 2),      ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), SCENE_10YUKIYAMANOMURA, SCENE_17SETUGEN,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_SNOWHEAD, 0),             ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 4),      SCENE_14YUKIDAMANOMITI, SCENE_10YUKIYAMANOMURA, SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 4),      ENTRANCE(PATH_TO_SNOWHEAD, 0),             SCENE_10YUKIYAMANOMURA, SCENE_14YUKIDAMANOMITI, SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(SNOWHEAD, 0),                     ENTRANCE(PATH_TO_SNOWHEAD, 1),             SCENE_12HAKUGINMAE,     SCENE_14YUKIDAMANOMITI, SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_SNOWHEAD, 1),             ENTRANCE(SNOWHEAD, 0),                     SCENE_14YUKIDAMANOMITI, SCENE_12HAKUGINMAE,     SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GORON_VILLAGE_WINTER, 0),         ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 1), SCENE_11GORONNOSATO,    SCENE_17SETUGEN,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 1), ENTRANCE(GORON_VILLAGE_WINTER, 0),         SCENE_17SETUGEN,        SCENE_11GORONNOSATO,    SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GORON_RACETRACK, 0),              ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 2), SCENE_GORONRACE,        SCENE_17SETUGEN,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 2), ENTRANCE(GORON_RACETRACK, 0),              SCENE_17SETUGEN,        SCENE_GORONRACE,        SCENE_MAX, {} },
    // Great Bay (West.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ZORA_CAPE, 0),                    ENTRANCE(GREAT_BAY_COAST, 1),              SCENE_31MISAKI,         SCENE_30GYOSON,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GREAT_BAY_COAST, 1),              ENTRANCE(ZORA_CAPE, 0),                    SCENE_30GYOSON,         SCENE_31MISAKI,         SCENE_MAX, {} },
    // Ikana (East.cpp)
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(IKANA_CANYON, 0),                 ENTRANCE(ROAD_TO_IKANA, 1),                SCENE_IKANA,            SCENE_IKANAMAE,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ROAD_TO_IKANA, 1),                ENTRANCE(IKANA_CANYON, 0),                 SCENE_IKANAMAE,         SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(IKANA_GRAVEYARD, 0),              ENTRANCE(ROAD_TO_IKANA, 2),                SCENE_BOTI,             SCENE_IKANAMAE,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ROAD_TO_IKANA, 2),                ENTRANCE(IKANA_GRAVEYARD, 0),              SCENE_IKANAMAE,         SCENE_BOTI,             SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(STONE_TOWER, 0),                  ENTRANCE(IKANA_CANYON, 3),                 SCENE_F40,              SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(IKANA_CANYON, 3),                 ENTRANCE(STONE_TOWER, 0),                  SCENE_IKANA,            SCENE_F40,              SCENE_MAX, {} },
    // apworld 0.9 (stage 6b). Older seeds have no pairs for these rows, so they stay vanilla there (InsideBehind).
    // Astral Observatory (Central.cpp): the Bombers' passage from East Clock Town and the door to Termina Field.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ASTRAL_OBSERVATORY, 0),           ENTRANCE(EAST_CLOCK_TOWN, 2),              SCENE_TENMON_DAI,       SCENE_TOWN,             SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(EAST_CLOCK_TOWN, 2),              ENTRANCE(ASTRAL_OBSERVATORY, 0),           SCENE_TOWN,             SCENE_TENMON_DAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ASTRAL_OBSERVATORY, 1),           ENTRANCE(TERMINA_FIELD, 9),                SCENE_TENMON_DAI,       SCENE_00KEIKOKU,        SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(TERMINA_FIELD, 9),                ENTRANCE(ASTRAL_OBSERVATORY, 1),           SCENE_00KEIKOKU,        SCENE_TENMON_DAI,       SCENE_MAX, {} },
    // Underwater ways (West.cpp, PiratesFortress.cpp), only among each other: overworldWaterEntrances below.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0),    ENTRANCE(GREAT_BAY_COAST, 5),              SCENE_TORIDE,           SCENE_30GYOSON,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GREAT_BAY_COAST, 5),              ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0),    SCENE_30GYOSON,         SCENE_TORIDE,           SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ZORA_HALL, 0),                    ENTRANCE(ZORA_CAPE, 1),                    SCENE_33ZORACITY,       SCENE_31MISAKI,         SCENE_MAX, {} },
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ZORA_CAPE, 1),                    ENTRANCE(ZORA_HALL, 0),                    SCENE_31MISAKI,         SCENE_33ZORACITY,       SCENE_MAX, {} },
    // More interiors (Regions/*.cpp): Great Fairy fountains (walk-in, one spawn per fountain like the Zora Hall rooms),
    // the Goron Shop inside the fixed Goron Shrine, the Goron Graveyard, the Deku King's chamber and the monkey's cell.
    { RO_SHUFFLE_INTERIORS, ENTRANCE(FAIRY_FOUNTAIN, 0),               ENTRANCE(NORTH_CLOCK_TOWN, 3),             SCENE_YOUSEI_IZUMI,     SCENE_BACKTOWN,         SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(FAIRY_FOUNTAIN, 1),               ENTRANCE(WOODFALL, 2),                     SCENE_YOUSEI_IZUMI,     SCENE_21MITURINMAE,     SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(FAIRY_FOUNTAIN, 2),               ENTRANCE(SNOWHEAD, 2),                     SCENE_YOUSEI_IZUMI,     SCENE_12HAKUGINMAE,     SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(FAIRY_FOUNTAIN, 3),               ENTRANCE(ZORA_CAPE, 5),                    SCENE_YOUSEI_IZUMI,     SCENE_31MISAKI,         SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(FAIRY_FOUNTAIN, 4),               ENTRANCE(IKANA_CANYON, 11),                SCENE_YOUSEI_IZUMI,     SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(GORON_SHOP, 0),                   ENTRANCE(GORON_SHRINE, 1),                 SCENE_GORONSHOP,        SCENE_16GORON_HOUSE,    SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(GORON_GRAVERYARD, 0),             ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 3),      SCENE_GORON_HAKA,       SCENE_10YUKIYAMANOMURA, SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(DEKU_KINGS_CHAMBER, 0),           ENTRANCE(DEKU_PALACE, 2),                  SCENE_DEKU_KING,        SCENE_22DEKUCITY,       SCENE_MAX, {} },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(DEKU_KINGS_CHAMBER, 1),           ENTRANCE(DEKU_PALACE, 3),                  SCENE_DEKU_KING,        SCENE_22DEKUCITY,       SCENE_MAX, {} },
    // Special areas (Regions/*.cpp): small dungeons and shrines behind one door, rows like the temples' (a door and the
    // area behind it). Beneath the Well's back exit stays joined to Ikana Castle's courtyard, both move with their door.
    { RO_SHUFFLE_SPECIAL_AREAS, ENTRANCE(SWAMP_SPIDER_HOUSE, 0),       ENTRANCE(SOUTHERN_SWAMP_POISONED, 8),      SCENE_KINSTA1,          SCENE_20SICHITAI,       SCENE_MAX, {} },
    { RO_SHUFFLE_SPECIAL_AREAS, ENTRANCE(OCEANSIDE_SPIDER_HOUSE, 0),   ENTRANCE(GREAT_BAY_COAST, 8),              SCENE_KINDAN2,          SCENE_30GYOSON,         SCENE_MAX, {} },
    { RO_SHUFFLE_SPECIAL_AREAS, ENTRANCE(BENEATH_THE_WELL, 0),         ENTRANCE(IKANA_CANYON, 5),                 SCENE_REDEAD,           SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_SPECIAL_AREAS, ENTRANCE(IKANA_CASTLE, 1),             ENTRANCE(IKANA_CANYON, 8),                 SCENE_CASTLE,           SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_SPECIAL_AREAS, ENTRANCE(SECRET_SHRINE, 0),            ENTRANCE(IKANA_CANYON, 12),                SCENE_RANDOM,           SCENE_IKANA,            SCENE_MAX, {} },
    { RO_SHUFFLE_SPECIAL_AREAS, ENTRANCE(DEKU_SHRINE, 0),              ENTRANCE(DEKU_PALACE, 4),                  SCENE_DANPEI,           SCENE_22DEKUCITY,       SCENE_MAX, {} },
};

// Overworld rows whose arrival is under water: both ways of these transitions are taken swimming (Zora in the logic),
// so they pair only among each other. A land exit (any form) never leads into water, where a Deku or Goron would void
// out and respawn at the same entrance forever. Human Link can dive too (z_player.c:8931, all forms but Zora), so
// Pinnacle Rock (West.cpp:149, SWIM only) and Waterfall Rapids (West.cpp:360, Hookshot) stay fixed. The apworld pairs a
// coast's exit with a dead end's (moat, Zora Hall): Zora Hall's door by land has no region in the logic.
const std::vector<u16> overworldWaterEntrances = {
    ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0), // West.cpp:151, Zora and SWIM
    ENTRANCE(GREAT_BAY_COAST, 5),           // PiratesFortress.cpp:171, Zora and SWIM
    ENTRANCE(ZORA_HALL, 0),                 // West.cpp:359, Zora and SWIM
    ENTRANCE(ZORA_CAPE, 1),                 // West.cpp:413, the tunnel back (rule true: arrived as Zora)
};

// Hours of every interior row, read from its door's rule in the logic (Regions/*.cpp) and its schedule
// (z_en_door.c sDoorSch_*). A building's checks assume its own door's hours; INTERIOR_OWN_HOURS rows never move.
const std::vector<InteriorEntrance> interiorEntrances = {
    { ENTRANCE(TRADING_POST, 0),            INTERIOR_ALWAYS_OPEN }, // logic true; locked 21-22 (sDoorSch_TradingPost)
    { ENTRANCE(BOMB_SHOP, 0),               INTERIOR_ALWAYS_OPEN }, // sDoorSch_BombShop: never locked
    { ENTRANCE(CURIOSITY_SHOP, 0),          INTERIOR_OWN_HOURS },   // nights 22-5 (Central.cpp WCT exit)
    { ENTRANCE(CURIOSITY_SHOP, 1),          INTERIOR_OWN_HOURS },   // Kafei's events and hours (Central.cpp laundry)
    { ENTRANCE(SWORDMANS_SCHOOL, 0),        INTERIOR_ALWAYS_OPEN }, // locked only night 3 23-24
    { ENTRANCE(POST_OFFICE, 0),             INTERIOR_OWN_HOURS },   // afternoons/evenings and letter events
    { ENTRANCE(LOTTERY_SHOP, 0),            INTERIOR_DAYTIME },     // 6-23, checks by day
    { ENTRANCE(HONEY_AND_DARLINGS_SHOP, 0), INTERIOR_DAYTIME },     // until 22
    { ENTRANCE(MILK_BAR, 0),                INTERIOR_OWN_HOURS },   // 10-21, nights with Romani's Mask
    { ENTRANCE(MAYORS_RESIDENCE, 0),        INTERIOR_DAYTIME },     // 10-20, day 3 from 10
    { ENTRANCE(TOWN_SHOOTING_GALLERY, 0),   INTERIOR_DAYTIME },     // until 22; checks only test the bow
    { ENTRANCE(TREASURE_CHEST_SHOP, 0),     INTERIOR_DAYTIME },     // until 22
    { ENTRANCE(STOCK_POT_INN, 0),           INTERIOR_OWN_HOURS },   // 8-20:30 or Room Key
    { ENTRANCE(STOCK_POT_INN, 1),           INTERIOR_ALWAYS_OPEN }, // upstairs, Deku only (sDoorSch_InnUpperEntrance)
    { ENTRANCE(DEKU_SCRUB_PLAYGROUND, 0),   INTERIOR_ALWAYS_OPEN },
    { ENTRANCE(TOURIST_INFORMATION, 0),     INTERIOR_ALWAYS_OPEN },
    { ENTRANCE(MAGIC_HAGS_POTION_SHOP, 0),  INTERIOR_ALWAYS_OPEN }, // sDoorSch_MagicHagsPotionShop: never locked
    { ENTRANCE(SWAMP_SHOOTING_GALLERY, 0),  INTERIOR_DAYTIME },     // until 22; checks only test the bow
    { ENTRANCE(MOUNTAIN_SMITHY, 0),         INTERIOR_ALWAYS_OPEN },
    { ENTRANCE(RANCH_HOUSE, 0),             INTERIOR_OWN_HOURS },   // barn: night 1 until 2:30, day 2, day 3 until 20
    { ENTRANCE(RANCH_HOUSE, 1),             INTERIOR_DAYTIME },     // until 20
    { ENTRANCE(CUCCO_SHACK, 0),             INTERIOR_DAYTIME },     // until 20
    { ENTRANCE(DOGGY_RACETRACK, 0),         INTERIOR_DAYTIME },     // until 20
    { ENTRANCE(MARINE_RESEARCH_LAB, 0),     INTERIOR_ALWAYS_OPEN },
    { ENTRANCE(FISHERMANS_HUT, 0),          INTERIOR_ALWAYS_OPEN },
    { ENTRANCE(ZORA_HALL_ROOMS, 5),         INTERIOR_ALWAYS_OPEN }, // shop
    { ENTRANCE(ZORA_HALL_ROOMS, 2),         INTERIOR_ALWAYS_OPEN }, // Lulu
    { ENTRANCE(ZORA_HALL_ROOMS, 3),         INTERIOR_ALWAYS_OPEN }, // Evan
    { ENTRANCE(ZORA_HALL_ROOMS, 1),         INTERIOR_ALWAYS_OPEN }, // Japas
    { ENTRANCE(ZORA_HALL_ROOMS, 0),         INTERIOR_ALWAYS_OPEN }, // Mikau
    { ENTRANCE(MUSIC_BOX_HOUSE, 0),         INTERIOR_ALWAYS_OPEN }, // once Sharp's curse is broken
    { ENTRANCE(GHOST_HUT, 0),               INTERIOR_ALWAYS_OPEN },
    { ENTRANCE(FAIRY_FOUNTAIN, 0),          INTERIOR_ALWAYS_OPEN }, // Central.cpp:183, true
    { ENTRANCE(FAIRY_FOUNTAIN, 1),          INTERIOR_ALWAYS_OPEN }, // South.cpp:559, Deku and Mad Scrubs, no time
    { ENTRANCE(FAIRY_FOUNTAIN, 2),          INTERIOR_ALWAYS_OPEN }, // North.cpp:580, true
    { ENTRANCE(FAIRY_FOUNTAIN, 3),          INTERIOR_ALWAYS_OPEN }, // West.cpp:334, explosives, no time
    { ENTRANCE(FAIRY_FOUNTAIN, 4),          INTERIOR_ALWAYS_OPEN }, // East.cpp:186, true
    { ENTRANCE(GORON_SHOP, 0),              INTERIOR_ALWAYS_OPEN }, // North.cpp:39, true
    { ENTRANCE(GORON_GRAVERYARD, 0),        INTERIOR_ALWAYS_OPEN }, // North.cpp:313, Lens of Truth, no time
    { ENTRANCE(DEKU_KINGS_CHAMBER, 0),      INTERIOR_ALWAYS_OPEN }, // South.cpp:112, true
    { ENTRANCE(DEKU_KINGS_CHAMBER, 1),      INTERIOR_ALWAYS_OPEN }, // South.cpp:122, true
};

// The grotto data of each hole: DeveloperTools/BetterMapSelect.c (grottoRespawn.data); for the shared chest and cow
// grottos also ObjGrass.cpp chestGrottoMap/cowGrottoMap, the chest flags in Checks.cpp (data & 0x1F, z_en_torch.c:36)
// and Logic.cpp GetRegionIdFromEntrance (0x3F). Grottos 12 and 16 have no grotto respawn (UNUSED_GROTTO_RESPAWN).
const std::vector<GrottoEntrance> grottoEntrances = {
    // into,                 game entrance,         data, kind
    { ENTRANCE(GROTTOS, 0),  ENTRANCE(GROTTOS, 0),  0x1F, GROTTO_HOLE },       // Termina Field gossip stones #3
    { ENTRANCE(GROTTOS, 1),  ENTRANCE(GROTTOS, 1),  0x1F, GROTTO_HOLE },       // Termina Field gossip stones #1
    { ENTRANCE(GROTTOS, 2),  ENTRANCE(GROTTOS, 2),  0x1F, GROTTO_HOLE },       // Termina Field gossip stones #4
    { ENTRANCE(GROTTOS, 3),  ENTRANCE(GROTTOS, 3),  0x1F, GROTTO_HOLE },       // Termina Field gossip stones #2
    { ENTRANCE(GROTTOS, 5),  ENTRANCE(GROTTOS, 5),  0xFF, GROTTO_HOLE },       // Path to Goron Village hot spring
    { ENTRANCE(GROTTOS, 7),  ENTRANCE(GROTTOS, 7),  0x1F, GROTTO_HOLE },       // Termina Field dodongo
    { ENTRANCE(GROTTOS, 9),  ENTRANCE(GROTTOS, 9),  0x1F, GROTTO_HOLE },       // Termina Field scrub
    { ENTRANCE(GROTTOS, 11), ENTRANCE(GROTTOS, 11), 0x1F, GROTTO_HOLE },       // Termina Field bio baba
    { ENTRANCE(GROTTOS, 12), ENTRANCE(GROTTOS, 12), -1,   GROTTO_SCENE_EXIT }, // Deku Palace bean salesman
    { ENTRANCE(GROTTOS, 13), ENTRANCE(GROTTOS, 13), 0x1F, GROTTO_HOLE },       // Termina Field peahat
    { ENTRANCE(GROTTOS, 16), ENTRANCE(GROTTOS, 16), -1,   GROTTO_SCENE_EXIT }, // Lone Peak Shrine
    { ENTRANCE(GROTTOS, 17), ENTRANCE(GROTTOS, 10), 0x1F, GROTTO_HOLE },       // Termina Field cow
    { ENTRANCE(GROTTOS, 18), ENTRANCE(GROTTOS, 4),  0x9A, GROTTO_HOLE },       // Termina Field pillar
    { ENTRANCE(GROTTOS, 19), ENTRANCE(GROTTOS, 4),  0x3F, GROTTO_HOLE },       // Termina Field tall grass
    { ENTRANCE(GROTTOS, 20), ENTRANCE(GROTTOS, 4),  0x3E, GROTTO_HOLE },       // Road to Southern Swamp
    { ENTRANCE(GROTTOS, 21), ENTRANCE(GROTTOS, 4),  0x3D, GROTTO_HOLE },       // Southern Swamp
    { ENTRANCE(GROTTOS, 22), ENTRANCE(GROTTOS, 4),  0x5C, GROTTO_HOLE },       // Woods of Mystery
    { ENTRANCE(GROTTOS, 23), ENTRANCE(GROTTOS, 4),  0x3B, GROTTO_HOLE },       // Mountain Village tunnel (spring)
    { ENTRANCE(GROTTOS, 24), ENTRANCE(GROTTOS, 4),  0x99, GROTTO_HOLE },       // Path to Goron Village ramp
    { ENTRANCE(GROTTOS, 25), ENTRANCE(GROTTOS, 4),  0x33, GROTTO_HOLE },       // Path to Snowhead
    { ENTRANCE(GROTTOS, 26), ENTRANCE(GROTTOS, 4),  0x37, GROTTO_HOLE },       // Great Bay Coast fisherman
    { ENTRANCE(GROTTOS, 27), ENTRANCE(GROTTOS, 10), 0xFF, GROTTO_HOLE },       // Great Bay Coast cow
    { ENTRANCE(GROTTOS, 28), ENTRANCE(GROTTOS, 4),  0x95, GROTTO_HOLE },       // Zora Cape
    { ENTRANCE(GROTTOS, 29), ENTRANCE(GROTTOS, 4),  0xB4, GROTTO_HOLE },       // Ikana Canyon
    { ENTRANCE(GROTTOS, 30), ENTRANCE(GROTTOS, 4),  0xB8, GROTTO_HOLE },       // Ikana Graveyard
    { ENTRANCE(GROTTOS, 31), ENTRANCE(GROTTOS, 4),  0x96, GROTTO_HOLE },       // Road to Ikana
};
// clang-format on

// Read from the rules of each door's vanilla way (Logic/Regions), items and songs only; see EntranceShuffle.h.
// Snowhead's temple has no row: its only check before the Goron Mask block is an enemy drop
// (SnowheadTemple.cpp, RR_SNOWHEAD_TEMPLE_ENTRANCE_BEFORE_BLOCK).
// clang-format off
const std::vector<ShuffleAreaRule> shuffleAreaRules = {
    // Southern Swamp -> Woodfall, owl platform, door (South.cpp RR_SOUTHERN_SWAMP_SOUTH_UPPER, RR_WOODFALL*)
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(WOODFALL_TEMPLE, 0),    [] { return CAN_BE_DEKU; } },
    // Zora Cape -> ledge before the temple, door (West.cpp RR_ZORA_CAPE, RR_ZORA_CAPE_BEFORE_GREAT_BAY_TEMPLE)
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(GREAT_BAY_TEMPLE, 0),   [] { return CAN_BE_ZORA && HAS_ITEM(ITEM_HOOKSHOT); } },
    // Stone Tower climb and door (East.cpp RR_STONE_TOWER_BOTTOM, _MIDDLE, _UPPER, _TOP)
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(STONE_TOWER_TEMPLE, 0), [] { return HAS_ITEM(ITEM_HOOKSHOT) && CAN_PLAY_SONG(ELEGY) && CAN_BE_GORON && CAN_BE_ZORA; } },
    // WoodfallTemple.cpp:228 guarantees Deku and Bow before the vanilla boss door.
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(ODOLWAS_LAIR, 0),   [] { return CAN_BE_DEKU && HAS_ITEM(ITEM_BOW); } },
    // SnowheadTemple.cpp:260 guarantees Goron before the temple interior; Logic.h's fight rule only tests Fire Arrows.
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(GOHTS_LAIR, 0),     [] { return CAN_BE_GORON; } },
    // GreatBayTemple.cpp:276 and West.cpp's temple approach guarantee Zora; the fight also allows Fierce Deity.
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(GYORGS_LAIR, 0),    [] { return CAN_BE_ZORA; } },
    // East.cpp:545 guarantees Bow, Light Arrows and magic on the way to the inverted temple (and hence Twinmold).
    { RO_SHUFFLE_BOSS_ENTRANCES, ENTRANCE(TWINMOLDS_LAIR, 0), [] { return HAS_ITEM(ITEM_BOW) && HAS_ITEM(ITEM_ARROW_LIGHT) && HAS_MAGIC; } },
    // East.cpp:182: its own door needs Sharp's curse broken; the father's check (East.cpp:292) only tests the song.
    { RO_SHUFFLE_INTERIORS, ENTRANCE(MUSIC_BOX_HOUSE, 0), [] { return RANDO_EVENTS[RE_BREAK_SHARP_CURSE]; } },
    // West.cpp:416: the band room doors need Zora; Evan's check (West.cpp:367) does not say so. Cautious: never looser.
    { RO_SHUFFLE_INTERIORS, ENTRANCE(ZORA_HALL_ROOMS, 3), [] { return CAN_BE_ZORA; } },
    // Overworld: only where the arrival's region has no way in that lacks the vanilla guarantee (owl statues count as
    // such a way: RO_SHUFFLE_OWL_STATUES). South.cpp:496 is Woodfall's only way in; its grass (South.cpp:545) has no rule.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(WOODFALL, 0),                     [] { return CAN_BE_DEKU; } },
    // South.cpp:497: the palace treetop; it shares RR_DEKU_PALACE_OUTSIDE with the ground.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(DEKU_PALACE, 5),                  [] { return CAN_BE_DEKU; } },
    // North.cpp:416 is the racetrack's only way in; its 30 pots and 10 trees have no rule.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(GORON_RACETRACK, 0),              [] { return HAS_ITEM(ITEM_POWDER_KEG) && CAN_BE_GORON; } },
    // Out of the racetrack: in front of its boulder, inside RR_PATH_TO_GORON_VILLAGE (North.cpp:416).
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 2), [] { return HAS_ITEM(ITEM_POWDER_KEG) && CAN_BE_GORON; } },
    // Out of the ranch: past the boulder (MilkRoad.cpp:141), inside RR_MILK_ROAD with the way to Termina Field.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(MILK_ROAD, 1),                    [] { return AFTER(TIME_DAY3_AM_06_00) || RANDO_EVENTS[RE_DESTROY_MILK_ROAD_BOULDER]; } },
    // Central.cpp:34: the passage leads on to the observatory (true); East Clock Town's way in (Central.cpp:111) asks
    // for a projectile, for the balloon in the passage. Cautious: never looser.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ASTRAL_OBSERVATORY, 0),           [] { return CAN_USE_PROJECTILE; } },
    // West.cpp:151: the only way into the moat; its regions (PiratesFortress.cpp) assume a Zora who can swim.
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0),    [] { return CAN_BE_ZORA && CAN_USE_ABILITY(SWIM); } },
    // West.cpp:359: Zora Hall's tunnel back (West.cpp:413) is true; its other way in, the ledge (West.cpp:295), needs a
    // Zora as well (West.cpp:363 and the temple rule above).
    { RO_SHUFFLE_OVERWORLD, ENTRANCE(ZORA_HALL, 0),                    [] { return CAN_BE_ZORA && CAN_USE_ABILITY(SWIM); } },
    // South.cpp:112/122: the palace inside is Deku only (South.cpp:171), the princess event (South.cpp:55) does not
    // say so. Cautious, like Evan's room.
    { RO_SHUFFLE_INTERIORS, ENTRANCE(DEKU_KINGS_CHAMBER, 0),           [] { return CAN_BE_DEKU; } },
    { RO_SHUFFLE_INTERIORS, ENTRANCE(DEKU_KINGS_CHAMBER, 1),           [] { return CAN_BE_DEKU; } },
    // South.cpp:479: its own door; the house's checks were written for a player who got there (torch, Deku or boat).
    { RO_SHUFFLE_SPECIAL_AREAS, ENTRANCE(SWAMP_SPIDER_HOUSE, 0),       [] { return CAN_LIGHT_TORCH_NEAR_ANOTHER && (CAN_BE_DEKU || (RANDO_EVENTS[RE_CLEARED_WOODFALL_TEMPLE] && (CAN_USE_ABILITY(SWIM) || CAN_BE_ZORA))); } },
};
// clang-format on

// Elements, not bytes: the field is u16 since apworld 0.6 (u8[32] before; GetGrownArray loads both).
static constexpr size_t MAX_ROWS = sizeof(RandoSaveInfo::entrancePairs) / sizeof(RandoSaveInfo::entrancePairs[0]);

static u16* SavedPairs() {
    return gSaveContext.save.shipSaveInfo.rando.entrancePairs;
}

static bool IsInteriorRow(size_t row) {
    return shuffleEntrances[row].option == RO_SHUFFLE_INTERIORS;
}

// Hours class of an interior row; rows of different classes are never paired.
static InteriorHours HoursOf(size_t row) {
    for (const InteriorEntrance& interior : interiorEntrances) {
        if (interior.into == shuffleEntrances[row].into) {
            return interior.hours;
        }
    }
    return INTERIOR_OWN_HOURS;
}

// An overworld row arriving under water (overworldWaterEntrances); such rows pair only with each other.
static bool IsWaterRow(size_t row) {
    return shuffleEntrances[row].option == RO_SHUFFLE_OVERWORLD &&
           std::find(overworldWaterEntrances.begin(), overworldWaterEntrances.end(), shuffleEntrances[row].into) !=
               overworldWaterEntrances.end();
}

static bool CanPair(size_t door, size_t inside) {
    if (shuffleEntrances[door].option != shuffleEntrances[inside].option || IsWaterRow(door) != IsWaterRow(inside)) {
        return false;
    }
    return !IsInteriorRow(door) ||
           (HoursOf(door) == HoursOf(inside) && (HoursOf(door) != INTERIOR_OWN_HOURS || door == inside));
}

// ENTRANCE(scene, spawn) sets bits 4-15; bits 0-3 pick a variant of that entrance (z_scene_table.c, e.g. the Milk
// Bar's show layers). Interior doors compare without them and always load the target's base entrance.
static constexpr u16 ENTRANCE_WITHOUT_LAYER = 0xFFF0;

static bool SameDoor(u16 a, u16 b) {
    return (a & ENTRANCE_WITHOUT_LAYER) == (b & ENTRANCE_WITHOUT_LAYER);
}

// Row of the area behind this door.
static size_t InsideBehind(size_t door) {
    if (RANDO_SAVE_OPTIONS[shuffleEntrances[door].option] == RO_GENERIC_OFF) {
        return door;
    }
    u16 value = SavedPairs()[door];
    return (value == 0 || value > shuffleEntrances.size() || !CanPair(door, value - 1)) ? door : value - 1;
}

static size_t DoorForInside(size_t inside) {
    for (size_t door = 0; door < shuffleEntrances.size() && door < MAX_ROWS; door++) {
        if (shuffleEntrances[door].option == shuffleEntrances[inside].option && InsideBehind(door) == inside) {
            return door;
        }
    }
    return inside;
}

void SetPairs(const std::vector<std::pair<int32_t, int32_t>>& pairs) {
    std::fill(SavedPairs(), SavedPairs() + MAX_ROWS, (u16)0);
    auto rowOf = [](int32_t into) -> size_t {
        for (size_t row = 0; row < shuffleEntrances.size() && row < MAX_ROWS; row++) {
            if (shuffleEntrances[row].into == into) {
                return row;
            }
        }
        return MAX_ROWS;
    };
    for (const auto& [doorInto, insideInto] : pairs) {
        size_t door = rowOf(doorInto);
        size_t inside = rowOf(insideInto);
        if (door < MAX_ROWS && inside < MAX_ROWS && CanPair(door, inside)) {
            SavedPairs()[door] = (u16)(inside + 1);
        }
    }
}

// The clear flags the vanilla warp (or its skipped Woodfall cutscene) would set, so the boss's own region still heals.
static void MarkCleared(s16 bossScene) {
    switch (bossScene) {
        case SCENE_MITURIN_BS:
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE);
            SET_WEEKEVENTREG(WEEKEVENTREG_ENTERED_WOODFALL_TEMPLE_PRISON);
            break;
        case SCENE_HAKUGIN_BS:
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);
            break;
        case SCENE_SEA_BS:
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE);
            break;
        case SCENE_INISIE_BS:
            // z_door_warp1.c:763 and z_demo.c:676: the Twinmold completion flag.
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE);
            break;
        default:
            break;
    }
}

static bool pendingWarpRespawn = false;
static u16 warpRespawnEntrance = 0;

void OnSceneInit(s16 sceneId, s8 spawnNum) {
    if (!pendingWarpRespawn) {
        return;
    }
    pendingWarpRespawn = false;
    // SkipGiantsChamber resolves the first warp before actors spawn. Use the exterior's position, room and flags,
    // never the boss room's old position. The normal Player_Init does this too; enforce it for both return modes.
    if (gSaveContext.save.entrance == warpRespawnEntrance) {
        Play_SetupRespawnPoint(gPlayState, RESPAWN_MODE_DOWN, PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D));
        // Play_SetRespawnData normalizes the spawn index to zero. Keep the resolved exterior entrance as well.
        gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = warpRespawnEntrance;
        gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 1;
        gSaveContext.respawn[RESPAWN_MODE_TOP] = gSaveContext.respawn[RESPAWN_MODE_DOWN];
    }
}

// ---- Grottos ----

static constexpr size_t NO_ROW = (size_t)-1;

static bool IsGrottoRow(size_t row) {
    return shuffleEntrances[row].option == RO_SHUFFLE_GROTTOS;
}

static const GrottoEntrance* GrottoOf(size_t row) {
    for (const GrottoEntrance& grotto : grottoEntrances) {
        if (grotto.into == shuffleEntrances[row].into) {
            return &grotto;
        }
    }
    return nullptr;
}

// The grotto row whose room the game loads for this real entrance. Where grottos share the entrance (4 chest grottos,
// 10 cow grottos), the grotto data decides; DoorAna writes it before the transition (z_door_ana.c:150).
static size_t GrottoRowFor(u16 gameEntrance, s8 data) {
    size_t found = NO_ROW;
    int sharing = 0;
    for (size_t row = 0; row < shuffleEntrances.size() && row < MAX_ROWS; row++) {
        const GrottoEntrance* grotto = IsGrottoRow(row) ? GrottoOf(row) : nullptr;
        if (grotto == nullptr || grotto->gameEntrance != gameEntrance) {
            continue;
        }
        sharing++;
        if (found == NO_ROW || (grotto->data >= 0 && (u8)grotto->data == (u8)data)) {
            found = row;
        }
    }
    if (sharing > 1 && found != NO_ROW && (u8)GrottoOf(found)->data != (u8)data) {
        return NO_ROW; // a hole the table does not know: leave it vanilla
    }
    return found;
}

bool LogicGrottoEntrance(s32 entrance, s32* logicEntrance) {
    if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_GROTTOS] == RO_GENERIC_OFF) {
        return false;
    }
    size_t row = GrottoRowFor((u16)entrance, gSaveContext.respawn[RESPAWN_MODE_UNK_3].data);
    if (row == NO_ROW) {
        return false;
    }
    *logicEntrance = shuffleEntrances[row].into;
    return true;
}

// Grotto rows only. Into hole A: load grotto X = the pair (real entrance, grotto data) of X; A's own respawn point
// stays the way back. Out of X: back to the hole A it lies behind. Returns true if the transition was a grotto's.
static bool RedirectGrotto(PlayState* play) {
    if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_GROTTOS] == RO_GENERIC_OFF) {
        return false;
    }
    u16 next = play->nextEntrance;
    RespawnData& grottoRespawn = gSaveContext.respawn[RESPAWN_MODE_UNK_3];

    if (play->sceneId != SCENE_KAKUSIANA) {
        // No scene check beyond this: a hole's scene may be the spring or winter variant, and a death or void out
        // inside a grotto (scene SCENE_KAKUSIANA) reloads the grotto it is in and must not be redirected again.
        size_t hole = GrottoRowFor(next, grottoRespawn.data);
        if (hole == NO_ROW) {
            return false;
        }
        size_t inside = InsideBehind(hole);
        if (inside == hole) {
            return true;
        }
        const GrottoEntrance* holeGrotto = GrottoOf(hole);
        const GrottoEntrance* insideGrotto = GrottoOf(inside);
        if (holeGrotto->kind == GROTTO_SCENE_EXIT) {
            // Only a fallback: the way out of the grotto is redirected to this exit's outOf below.
            Play_SetupRespawnPoint(play, RESPAWN_MODE_UNK_3, PLAYER_PARAMS(0xFF, PLAYER_START_MODE_GROTTO));
        }
        if (insideGrotto->data >= 0) {
            grottoRespawn.data = (s8)insideGrotto->data;
        }
        play->nextEntrance = insideGrotto->gameEntrance;
        // An exit can record itself as the void-out point before the transition (z_player.c:6508); never the old room.
        if (gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == next) {
            gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = insideGrotto->gameEntrance;
        }
        return true;
    }

    size_t inside = GrottoRowFor(gSaveContext.save.entrance, grottoRespawn.data);
    if (inside == NO_ROW) {
        return false;
    }
    const GrottoEntrance* insideGrotto = GrottoOf(inside);
    // The grotto's own way out: exit 0xFFFF back to the stored hole (z_player.c:6496), or a scene exit to its outOf.
    // Void outs (respawn DOWN, never set in grottos: z_play.c:2053), deaths, songs and warps are left alone.
    bool wayOut = (gSaveContext.respawnFlag == 4 && next == grottoRespawn.entrance) ||
                  (insideGrotto->kind == GROTTO_SCENE_EXIT && next == shuffleEntrances[inside].outOf);
    if (!wayOut) {
        return true;
    }
    size_t hole = DoorForInside(inside);
    if (hole == inside) {
        return true;
    }
    if (GrottoOf(hole)->kind == GROTTO_HOLE) {
        // The hole stored itself when the player fell in; return there exactly like a vanilla grotto exit.
        play->nextEntrance = grottoRespawn.entrance;
        gSaveContext.respawnFlag = 4;
    } else {
        play->nextEntrance = shuffleEntrances[hole].outOf;
        gSaveContext.respawnFlag = 0;
    }
    return true;
}

// ---- Interiors ----

// Interior rows only. Into door A (any scene but A's own building): load building X = the base entrance of X's door.
// Out of X (its own way out, from X's scene): back in front of A. A void out or death inside X respawns at X's
// entrance from X's own scene, which is never redirected again; songs, owls and warps lead to no table entrance.
static bool RedirectInterior(PlayState* play) {
    if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_INTERIORS] == RO_GENERIC_OFF) {
        return false;
    }
    u16 next = play->nextEntrance;
    s16 scene = play->sceneId;
    for (size_t row = 0; row < shuffleEntrances.size() && row < MAX_ROWS; row++) {
        if (!IsInteriorRow(row)) {
            continue;
        }
        // No test of the exterior scene: it may be its cleared or spring variant (z_play.c Play_Init converts the
        // scene, not the entrance), so the entrance alone names the door.
        if (scene != shuffleEntrances[row].insideScene && SameDoor(next, shuffleEntrances[row].into)) {
            size_t inside = InsideBehind(row);
            if (inside != row) {
                play->nextEntrance = shuffleEntrances[inside].into;
                // A cutscene layer chosen for A's building (z_play.c:632) must not play in X.
                if (gSaveContext.nextCutsceneIndex >= 0xFFF0) {
                    gSaveContext.nextCutsceneIndex = 0xFFEF;
                }
            }
        } else if (scene == shuffleEntrances[row].insideScene && SameDoor(next, shuffleEntrances[row].outOf)) {
            size_t door = DoorForInside(row);
            if (door != row) {
                play->nextEntrance = shuffleEntrances[door].outOf;
            }
        } else {
            continue;
        }
        // An exit may record itself as the void-out point before the transition (z_player.c func_808354A4).
        if (play->nextEntrance != next && gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == next) {
            gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = play->nextEntrance;
        }
        return true;
    }
    return false;
}

// ---- Overworld ----

static bool IsOverworldRow(size_t row) {
    return shuffleEntrances[row].option == RO_SHUFFLE_OVERWORLD;
}

// The place an entrance leads to, without its variant: the spring and cleared entrance groups have the spawns of the
// winter and poisoned ones (Play_Init swaps only the scene and keeps spawn and layer, z_play.c:2264-2299), and the
// layer bits (0-3) are chosen per arrival. Rows are written with these base entrances.
static u16 PlaceOf(u16 entrance) {
    s32 scene = entrance >> 9;
    switch (scene) {
        case ENTR_SCENE_MOUNTAIN_VILLAGE_SPRING:
            scene = ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_GORON_VILLAGE_SPRING:
            scene = ENTR_SCENE_GORON_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_PATH_TO_GORON_VILLAGE_SPRING:
            scene = ENTR_SCENE_PATH_TO_GORON_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_SOUTHERN_SWAMP_CLEARED:
            scene = ENTR_SCENE_SOUTHERN_SWAMP_POISONED;
            break;
        default:
            break;
    }
    return Entrance_Create(scene, (entrance >> 4) & 0x1F, 0);
}

// Riding through a redirected exit would bring Epona into any scene (gHorseIsMounted, z_player.c:6578, and
// Horse_SpawnOverworld, z_horse.c:124, which tests only the layer). Link gets off instead and Epona waits at the exit,
// as after dismounting (z_player.c:17188). In a horse scene she still appears next to him with Epona's Song.
static void LeaveEponaAtExit(PlayState* play) {
    if (!gHorseIsMounted) {
        return;
    }
    gHorseIsMounted = false;
    Player* player = GET_PLAYER(play);
    Actor* horse = player != nullptr ? player->rideActor : nullptr;
    if (horse != nullptr && CHECK_QUEST_ITEM(QUEST_SONG_EPONA)) {
        gSaveContext.save.saveInfo.horseData.sceneId = play->sceneId;
        gSaveContext.save.saveInfo.horseData.pos.x = horse->world.pos.x;
        gSaveContext.save.saveInfo.horseData.pos.y = horse->world.pos.y;
        gSaveContext.save.saveInfo.horseData.pos.z = horse->world.pos.z;
        gSaveContext.save.saveInfo.horseData.yaw = horse->shape.rot.y;
    }
}

// Overworld rows only, one row per exit. Through exit A (scene and place of A's row): arrive where A's partner row
// arrives, and let Play_Init choose that place's variant. Returns true if the transition was an overworld exit.
static bool RedirectOverworld(PlayState* play) {
    if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_OVERWORLD] == RO_GENERIC_OFF) {
        return false;
    }
    // Only real scene exits: 0, or -2 for an exit that recorded itself as void-out point (z_player.c:6510); Play_Init
    // resets the flag (z_play.c:2497). Void outs (1), deaths (-1, -6), grotto returns (4) and warp songs (-3) reload
    // a place already reached (z_play.c:2074, 2084, z_game_over.c:97, z_player.c:6497, z_kankyo.c:3143).
    if (gSaveContext.respawnFlag != 0 && gSaveContext.respawnFlag != -2) {
        return false;
    }
    // Scripted transitions (e.g. Cremia's escort) keep their destination.
    if (play->csCtx.state != CS_STATE_IDLE) {
        return false;
    }
    u16 next = play->nextEntrance;
    u16 place = PlaceOf(next);
    s16 scene = Play_GetOriginalSceneId(play->sceneId);
    for (size_t row = 0; row < shuffleEntrances.size() && row < MAX_ROWS; row++) {
        if (!IsOverworldRow(row) || scene != shuffleEntrances[row].outsideScene ||
            place != shuffleEntrances[row].into) {
            continue;
        }
        size_t target = InsideBehind(row);
        if (target == row) {
            return true;
        }
        play->nextEntrance = shuffleEntrances[target].into;
        // A layer chosen for A's destination must not apply; Play_Init then picks the target's (z_play.c:2261).
        if (gSaveContext.nextCutsceneIndex >= 0xFFF0) {
            gSaveContext.nextCutsceneIndex = 0xFFEF;
        }
        // An exit may record itself as the void-out point before the transition (z_player.c:6508).
        if (gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == next) {
            gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = play->nextEntrance;
        }
        LeaveEponaAtExit(play);
        return true;
    }
    return false;
}

// ---- Special areas ----

static bool IsSpecialAreaRow(size_t row) {
    return shuffleEntrances[row].option == RO_SHUFFLE_SPECIAL_AREAS;
}

// Special-area rows only, a door and the area behind it like the temples. Into door A (A's place, from A's outside
// scene or its cleared variant): load area X. Out of X (its own way out, from X's scene): back in front of A; Play_Init
// picks that place's variant (cleared swamp, Ikana Canyon's layers), as for the overworld. The same filters: void
// outs, deaths and songs reload a place already reached, scripted transitions keep their destination.
static bool RedirectSpecialArea(PlayState* play) {
    if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_SPECIAL_AREAS] == RO_GENERIC_OFF) {
        return false;
    }
    if ((gSaveContext.respawnFlag != 0 && gSaveContext.respawnFlag != -2) || play->csCtx.state != CS_STATE_IDLE) {
        return false;
    }
    u16 next = play->nextEntrance;
    u16 place = PlaceOf(next);
    s16 scene = Play_GetOriginalSceneId(play->sceneId);
    for (size_t row = 0; row < shuffleEntrances.size() && row < MAX_ROWS; row++) {
        if (!IsSpecialAreaRow(row)) {
            continue;
        }
        const ShuffleEntrance& entry = shuffleEntrances[row];
        size_t other;
        u16 target;
        if (scene == entry.outsideScene && place == entry.into) {
            other = InsideBehind(row);
            target = shuffleEntrances[other].into;
        } else if (scene == entry.insideScene && place == entry.outOf) {
            other = DoorForInside(row);
            target = shuffleEntrances[other].outOf;
        } else {
            continue;
        }
        if (other == row) {
            return true;
        }
        play->nextEntrance = target;
        if (gSaveContext.nextCutsceneIndex >= 0xFFF0) {
            gSaveContext.nextCutsceneIndex = 0xFFEF;
        }
        // An exit may record itself as the void-out point before the transition (z_player.c:6508).
        if (gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == next) {
            gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = target;
        }
        LeaveEponaAtExit(play);
        return true;
    }
    return false;
}

void OnBeforeTransitionSetup() {
    PlayState* play = gPlayState;
    if (play == nullptr) {
        return;
    }
    if (RedirectGrotto(play) || RedirectInterior(play) || RedirectOverworld(play) || RedirectSpecialArea(play)) {
        return;
    }
    u16 next = play->nextEntrance;
    s16 scene = play->sceneId;

    // The scene check matters: a void out inside a shuffled area respawns at that area's (already redirected)
    // "into" entrance, which must not be redirected a second time.
    for (size_t door = 0; door < shuffleEntrances.size() && door < MAX_ROWS; door++) {
        const ShuffleEntrance& doorRow = shuffleEntrances[door];
        if (RANDO_SAVE_OPTIONS[doorRow.option] == RO_GENERIC_OFF || IsGrottoRow(door) || IsInteriorRow(door) ||
            IsOverworldRow(door) || IsSpecialAreaRow(door)) {
            continue;
        }
        size_t inside = InsideBehind(door);
        if (inside == door) {
            continue;
        }
        const ShuffleEntrance& insideRow = shuffleEntrances[inside];

        if (scene == doorRow.outsideScene && next == doorRow.into) {
            play->nextEntrance = insideRow.into;
            return;
        }
        if (doorRow.templeExit == 0 && scene == insideRow.insideScene && next == insideRow.outOf) {
            play->nextEntrance = doorRow.outOf;
            return;
        }
    }

    // Resolve both inverse pairings before redirecting a warp: boss -> containing temple -> exterior door.
    // Boss rows exist even for old seeds; disabled/missing pairings resolve to identity.
    for (size_t boss = 0; boss < shuffleEntrances.size() && boss < MAX_ROWS; boss++) {
        const ShuffleEntrance& bossRow = shuffleEntrances[boss];
        if (bossRow.templeExit == 0 || scene != bossRow.insideScene) {
            continue;
        }
        size_t bossDoor = DoorForInside(boss);
        u16 exterior = shuffleEntrances[bossDoor].templeExit;
        bool moved = bossDoor != boss;
        for (size_t temple = 0; temple < shuffleEntrances.size() && temple < MAX_ROWS; temple++) {
            if (shuffleEntrances[temple].option == RO_SHUFFLE_DUNGEON_ENTRANCES &&
                shuffleEntrances[temple].outOf == exterior) {
                size_t templeDoor = DoorForInside(temple);
                exterior = shuffleEntrances[templeDoor].outOf;
                moved |= templeDoor != temple;
                break;
            }
        }
        if (!moved) {
            return; // Preserve vanilla warp destinations and cutscenes for fixed points.
        }

        // First warp: it heads for the Giants' Chamber, and SkipGiantsChamber swaps in the transition that
        // HandleGiantsCutsceneSkip queued (tagged with the boss scene). Point that one at the door instead.
        if (next == ENTRANCE(GIANTS_CHAMBER, 0)) {
            for (auto& event : GameInteractor::Instance->events) {
                if (auto transition = std::get_if<GIEventTransition>(&event)) {
                    if (transition->transitionType == bossRow.insideScene) {
                        transition->entrance = exterior;
                        transition->cutsceneIndex = 0;
                        gSaveContext.respawnFlag = 0;
                        pendingWarpRespawn = true;
                        warpRespawnEntrance = exterior;
                        break;
                    }
                }
            }
            return;
        }
        // Repeat warps go straight to the region's clear cutscene: skip it, keep its effect.
        for (u16 warpTarget : bossRow.bossWarpsTo) {
            if (next == warpTarget) {
                MarkCleared(bossRow.insideScene);
                play->nextEntrance = exterior;
                gSaveContext.nextCutsceneIndex = 0xFFEF;
                gSaveContext.respawnFlag = 0;
                pendingWarpRespawn = true;
                warpRespawnEntrance = exterior;
                return;
            }
        }
    }
}

} // namespace EntranceShuffle

} // namespace Rando
