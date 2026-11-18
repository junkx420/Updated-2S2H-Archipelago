#include "ActorBehavior.h"
#include "Souls.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include "Rando/DrawFuncs.h"
#include "Rando/Logic/Logic.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"

extern "C" {
#include "variables.h"
#include "functions.h"

#include "overlays/actors/ovl_Boss_Hakugin/z_boss_hakugin.h"
#include "overlays/actors/ovl_En_Go/z_en_go.h"

void BossHakugin_DrawIce(BossHakugin*, PlayState*);
extern PlayState* gPlayState;
}

bool shouldMajoraRegister() {
    bool registerStatus = false;
    if (IS_RANDO) {
        if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] == RO_GENERIC_YES ||
            RANDO_SAVE_OPTIONS[RO_SHUFFLE_TRIFORCE_PIECES] == RO_GENERIC_YES) {
            registerStatus = true;
        }
    }
    return registerStatus;
}

// clang-format off
std::unordered_map<int16_t, RandoItemId> enemySoulMap = {
    // Real Bombchu and Flying Pot souls are excluded, as those actors are programmed to die to any collision, not just
    // the damaging type. We don't have a good answer for what behavior those should follow if the hit something.
    { ACTOR_EN_INVADEPOH,   RI_SOUL_ENEMY_ALIEN },
    { ACTOR_EN_AM,          RI_SOUL_ENEMY_ARMOS },
    { ACTOR_EN_BAT,         RI_SOUL_ENEMY_BAD_BAT },
    { ACTOR_EN_VM,          RI_SOUL_ENEMY_BEAMOS },
    { ACTOR_EN_BB,          RI_SOUL_ENEMY_BUBBLE },
    { ACTOR_EN_BBFALL,      RI_SOUL_ENEMY_BUBBLE },
    { ACTOR_EN_MKK,         RI_SOUL_ENEMY_BOE },
    { ACTOR_EN_BSB,         RI_SOUL_ENEMY_CAPTAIN_KEETA },
    { ACTOR_EN_SLIME,       RI_SOUL_ENEMY_CHUCHU },
    { ACTOR_EN_FAMOS,       RI_SOUL_ENEMY_DEATH_ARMOS },
    { ACTOR_EN_DRAGON,      RI_SOUL_ENEMY_DEEP_PYTHON },
    { ACTOR_EN_DEKUBABA,    RI_SOUL_ENEMY_DEKU_BABA },
    { ACTOR_EN_KAREBABA,    RI_SOUL_ENEMY_DEKU_BABA },
    { ACTOR_BOSS_05,        RI_SOUL_ENEMY_DEKU_BABA },
    { ACTOR_EN_WDHAND,      RI_SOUL_ENEMY_DEXIHAND },
    { ACTOR_EN_DINOFOS,     RI_SOUL_ENEMY_DINOLFOS },
    { ACTOR_EN_DODONGO,     RI_SOUL_ENEMY_DODONGO },
    { ACTOR_EN_GRASSHOPPER, RI_SOUL_ENEMY_DRAGONFLY },
    { ACTOR_EN_SNOWMAN,     RI_SOUL_ENEMY_EENO },
    { ACTOR_EN_EGOL,        RI_SOUL_ENEMY_EYEGORE },
    { ACTOR_EN_FZ,          RI_SOUL_ENEMY_FREEZARD },
    { ACTOR_EN_JSO,         RI_SOUL_ENEMY_GARO },
    { ACTOR_EN_JSO2,        RI_SOUL_ENEMY_GARO },
    { ACTOR_EN_BIGSLIME,    RI_SOUL_ENEMY_GEKKO },
    { ACTOR_EN_PAMETFROG,   RI_SOUL_ENEMY_GEKKO },
    { ACTOR_EN_BEE,         RI_SOUL_ENEMY_GIANT_BEE },
    { ACTOR_EN_DEATH,       RI_SOUL_ENEMY_GOMESS },
    { ACTOR_EN_MINIDEATH,   RI_SOUL_ENEMY_GOMESS },
    { ACTOR_EN_CROW,        RI_SOUL_ENEMY_GUAY },
    { ACTOR_EN_RUPPECROW,   RI_SOUL_ENEMY_GUAY },
    { ACTOR_EN_PP,          RI_SOUL_ENEMY_HIPLOOP },
    { ACTOR_EN_KNIGHT,      RI_SOUL_ENEMY_IGOS_DU_IKANA },
    { ACTOR_EN_IK,          RI_SOUL_ENEMY_IRON_KNUCKLE },
    { ACTOR_EN_FIREFLY,     RI_SOUL_ENEMY_KEESE },
    { ACTOR_EN_NEO_REEBA,   RI_SOUL_ENEMY_LEEVER },
    { ACTOR_EN_RR,          RI_SOUL_ENEMY_LIKE_LIKE },
    { ACTOR_EN_DEKUNUTS,    RI_SOUL_ENEMY_MAD_SCRUB },
    { ACTOR_EN_BAGUO,       RI_SOUL_ENEMY_NEJIRON },
    { ACTOR_EN_OKUTA,       RI_SOUL_ENEMY_OCTOROK },
    { ACTOR_EN_PEEHAT,      RI_SOUL_ENEMY_PEAHAT },
    { ACTOR_EN_KAIZOKU,     RI_SOUL_ENEMY_PIRATE },
    { ACTOR_EN_BIGPO,       RI_SOUL_ENEMY_POE },
    { ACTOR_EN_PO_SISTERS,  RI_SOUL_ENEMY_POE },
    { ACTOR_EN_POH,         RI_SOUL_ENEMY_POE },
    { ACTOR_EN_RD,          RI_SOUL_ENEMY_REDEAD },
    { ACTOR_EN_SB,          RI_SOUL_ENEMY_SHELLBLADE },
    { ACTOR_EN_PR,          RI_SOUL_ENEMY_SKULLFISH },
    { ACTOR_EN_PR2,         RI_SOUL_ENEMY_SKULLFISH },
    { ACTOR_EN_PRZ,         RI_SOUL_ENEMY_SKULLFISH },
    { ACTOR_EN_ST,          RI_SOUL_ENEMY_SKULLTULA },
    { ACTOR_EN_SW,          RI_SOUL_ENEMY_SKULLTULA },
    { ACTOR_EN_BIGPAMET,    RI_SOUL_ENEMY_SNAPPER },
    { ACTOR_EN_KAME,        RI_SOUL_ENEMY_SNAPPER },
    { ACTOR_EN_HINT_SKB,    RI_SOUL_ENEMY_STALCHILD },
    { ACTOR_EN_RAIL_SKB,    RI_SOUL_ENEMY_STALCHILD },
    { ACTOR_EN_SKB,         RI_SOUL_ENEMY_STALCHILD },
    { ACTOR_EN_THIEFBIRD,   RI_SOUL_ENEMY_TAKKURI },
    { ACTOR_EN_TITE,        RI_SOUL_ENEMY_TEKTITE },
    { ACTOR_EN_FLOORMAS,    RI_SOUL_ENEMY_WALLMASTER },
    { ACTOR_EN_WALLMAS,     RI_SOUL_ENEMY_WALLMASTER },
    { ACTOR_BOSS_04,        RI_SOUL_ENEMY_WART },
    { ACTOR_EN_TANRON2,     RI_SOUL_ENEMY_WART },
    { ACTOR_EN_WIZ,         RI_SOUL_ENEMY_WIZROBE },
    { ACTOR_EN_WF,          RI_SOUL_ENEMY_WOLFOS },
};
// clang-format on

// NPC souls (Archipelago fork). Without its soul an NPC is there but neither updates nor draws: it cannot be
// talked to, hands out nothing and runs no minigame. One entry per soul: the item, the actors it brings to life,
// the checks that need it and the logic events that need it.
// The apworld's rules are generated from this table (worlds/mm_2s2h/tools/gen_from_2s2h.py), so a check or event
// that cannot be done without the NPC has to be listed here. Listing too much only restricts item placement,
// listing too little can make a seed unbeatable.
// level 1: the 30 souls of apworld 0.2, used with shuffle_npc_souls 1 and 2. level 2: added in 0.12, used with
// shuffle_npc_souls 2 only. sceneId: the soul only owns its actors in this scene (EnSob1 runs as ACTOR_EN_OSSAN in
// the Bomb, Goron and Zora shops, and the Trading Post is ACTOR_EN_OSSAN too); -1 is every scene.
struct NpcSoul {
    RandoItemId randoItemId;
    std::vector<int16_t> actorIds;
    std::vector<RandoCheckId> checks;
    std::vector<RandoEvent> events;
    int level = 1;
    int16_t sceneId = -1;
};

// clang-format off
const std::vector<NpcSoul> npcSouls = {
    { RI_SOUL_NPC_ANJU, { ACTOR_EN_AN }, { RC_STOCK_POT_INN_ROOM_KEY, RC_STOCK_POT_INN_LETTER_TO_KAFEI, RC_STOCK_POT_INN_COUPLES_MASK }, {} },
    { RI_SOUL_NPC_BANKER, { ACTOR_EN_GINKO_MAN }, { RC_CLOCK_TOWN_WEST_BANK_ADULTS_WALLET, RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART, RC_CLOCK_TOWN_WEST_BANK_INTEREST }, {} },
    { RI_SOUL_NPC_BEAVERS, { ACTOR_EN_AZ }, { RC_WATERFALL_RAPIDS_BEAVER_RACE_01, RC_WATERFALL_RAPIDS_BEAVER_RACE_02 }, {} },
    { RI_SOUL_NPC_COWS, { ACTOR_EN_COW }, { RC_BENEATH_THE_WELL_COW, RC_GREAT_BAY_COAST_COW_BACK, RC_GREAT_BAY_COAST_COW_FRONT, RC_ROMANI_RANCH_BARN_COW_LEFT, RC_ROMANI_RANCH_BARN_COW_MIDDLE, RC_ROMANI_RANCH_BARN_COW_RIGHT, RC_ROMANI_RANCH_FIELD_COW_ENTRANCE, RC_ROMANI_RANCH_FIELD_COW_NEAR_HOUSE_BACK, RC_ROMANI_RANCH_FIELD_COW_NEAR_HOUSE_FRONT, RC_TERMINA_FIELD_COW_BACK, RC_TERMINA_FIELD_COW_FRONT }, { RE_ACCESS_MILK_REFILL } },
    { RI_SOUL_NPC_DEKU_BUTLER, { ACTOR_EN_DNO }, { RC_DEKU_SHRINE_MASK_OF_SCENTS }, {} },
    { RI_SOUL_NPC_EVAN, { ACTOR_EN_ZOS }, { RC_ZORA_HALL_EVANS_PIECE_OF_HEART }, {} },
    { RI_SOUL_NPC_GORMAN_BROTHERS, { ACTOR_EN_IN }, { RC_GORMAN_TRACK_GARO_MASK, RC_GORMAN_MILK_PURCHASE }, {} },
    { RI_SOUL_NPC_GRANNY, { ACTOR_EN_NB }, { RC_STOCK_POT_INN_GRANDMA_SHORT_STORY, RC_STOCK_POT_INN_GRANDMA_LONG_STORY }, {} },
    { RI_SOUL_NPC_GROG, { ACTOR_EN_HS }, { RC_ROMANI_RANCH_GROG }, {} },
    { RI_SOUL_NPC_GURU_GURU, { ACTOR_EN_GURUGURU }, { RC_CLOCK_TOWN_LAUNDRY_GURU_GURU }, {} },
    { RI_SOUL_NPC_HONEY_AND_DARLING, { ACTOR_EN_FU }, { RC_CLOCK_TOWN_EAST_HONEY_DARLING_ANY_DAY, RC_CLOCK_TOWN_EAST_HONEY_DARLING_ALL_DAYS }, {} },
    { RI_SOUL_NPC_KAMARO, { ACTOR_EN_YB }, { RC_TERMINA_FIELD_KAMARO_MASK }, {} },
    { RI_SOUL_NPC_KEATON, { ACTOR_EN_KITAN }, { RC_KEATON_QUIZ }, {} },
    { RI_SOUL_NPC_LOTTERY_GIRL, { ACTOR_EN_KUJIYA }, { RC_CLOCK_TOWN_WEST_LOTTERY }, {} },
    { RI_SOUL_NPC_MADAME_AROMA, { ACTOR_EN_AL }, { RC_MAYORS_OFFICE_KAFEIS_MASK, RC_MILK_BAR_MADAME_AROMA }, {} },
    { RI_SOUL_NPC_MAMAMU_YAN, { ACTOR_EN_AOB_01 }, { RC_DOGGY_RACETRACK_PIECE_OF_HEART }, {} },
    { RI_SOUL_NPC_MAYOR, { ACTOR_EN_DT }, { RC_MAYORS_OFFICE_PIECE_OF_HEART }, {} },
    { RI_SOUL_NPC_PLAYGROUND_SCRUBS, { ACTOR_EN_LIFT_NUTS }, { RC_DEKU_PLAYGROUND_ANY_DAY, RC_DEKU_PLAYGROUND_ALL_DAYS, RC_DEKU_PLAYGROUND_DAY_1_RUPEE_01, RC_DEKU_PLAYGROUND_DAY_1_RUPEE_02, RC_DEKU_PLAYGROUND_DAY_1_RUPEE_03, RC_DEKU_PLAYGROUND_DAY_1_RUPEE_04, RC_DEKU_PLAYGROUND_DAY_1_RUPEE_05, RC_DEKU_PLAYGROUND_DAY_1_RUPEE_06, RC_DEKU_PLAYGROUND_DAY_2_RUPEE_01, RC_DEKU_PLAYGROUND_DAY_2_RUPEE_02, RC_DEKU_PLAYGROUND_DAY_2_RUPEE_03, RC_DEKU_PLAYGROUND_DAY_2_RUPEE_04, RC_DEKU_PLAYGROUND_DAY_2_RUPEE_05, RC_DEKU_PLAYGROUND_DAY_2_RUPEE_06, RC_DEKU_PLAYGROUND_DAY_3_RUPEE_01, RC_DEKU_PLAYGROUND_DAY_3_RUPEE_02, RC_DEKU_PLAYGROUND_DAY_3_RUPEE_03, RC_DEKU_PLAYGROUND_DAY_3_RUPEE_04, RC_DEKU_PLAYGROUND_DAY_3_RUPEE_05, RC_DEKU_PLAYGROUND_DAY_3_RUPEE_06 }, {} },
    { RI_SOUL_NPC_POE_COLLECTOR, { ACTOR_EN_GB2 }, { RC_IKANA_CANYON_GHOST_HUT_PIECE_OF_HEART }, {} },
    { RI_SOUL_NPC_POSTMAN, { ACTOR_EN_PM, ACTOR_EN_MM3 }, { RC_CLOCK_TOWN_WEST_POSTMAN_MINIGAME, RC_CLOCK_TOWN_EAST_POSTMAN_HAT }, { RE_MEET_KAFEI, RE_POSTMAN_FREEDOM } },
    { RI_SOUL_NPC_ROSA_SISTERS, { ACTOR_EN_RZ }, { RC_CLOCK_TOWN_WEST_SISTERS_PIECE_OF_HEART }, {} },
    { RI_SOUL_NPC_SHIRO, { ACTOR_EN_STONE_HEISHI }, { RC_ROAD_TO_IKANA_STONE_MASK }, {} },
    { RI_SOUL_NPC_SHOOTING_GALLERY_OWNERS, { ACTOR_EN_SYATEKI_MAN }, { RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_HIGH_SCORE, RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_PERFECT_SCORE, RC_SWAMP_SHOOTING_GALLERY_HIGH_SCORE, RC_SWAMP_SHOOTING_GALLERY_PERFECT_SCORE }, {} },
    { RI_SOUL_NPC_SMITHS, { ACTOR_EN_KGY, ACTOR_EN_KBT }, { RC_MOUNTAIN_VILLAGE_SMITHY_RAZOR_SWORD, RC_MOUNTAIN_VILLAGE_SMITHY_GILDED_SWORD }, { RE_ACCESS_SMITHY } },
    { RI_SOUL_NPC_SPIDER_HOUSE_MEN, { ACTOR_EN_SSH, ACTOR_EN_STH }, { RC_SWAMP_SPIDER_HOUSE_MASK_OF_TRUTH, RC_OCEAN_SPIDER_HOUSE_WALLET }, {} },
    { RI_SOUL_NPC_SWORDSMAN, { ACTOR_EN_KENDO_JS }, { RC_SWORDSMAN_SCHOOL_PIECE_OF_HEART }, {} },
    { RI_SOUL_NPC_TINGLE, { ACTOR_EN_BAL }, { RC_CLOCK_TOWN_NORTH_TINGLE_MAP_01, RC_CLOCK_TOWN_NORTH_TINGLE_MAP_02, RC_GREAT_BAY_COAST_TINGLE_MAP_01, RC_GREAT_BAY_COAST_TINGLE_MAP_02, RC_IKANA_CANYON_TINGLE_MAP_01, RC_IKANA_CANYON_TINGLE_MAP_02, RC_MILK_ROAD_TINGLE_MAP_01, RC_MILK_ROAD_TINGLE_MAP_02, RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_01, RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_02, RC_TWIN_ISLANDS_TINGLE_MAP_01, RC_TWIN_ISLANDS_TINGLE_MAP_02 }, { RE_ACCESS_PICTOGRAPH_TINGLE } },
    { RI_SOUL_NPC_TOILET_HAND, { ACTOR_EN_BJT }, { RC_STOCK_POT_INN_TOILET_HAND }, {} },
    { RI_SOUL_NPC_TOTO, { ACTOR_EN_TOTO }, { RC_MILK_BAR_CIRCUS_LEADER_MASK }, {} },
    { RI_SOUL_NPC_TREASURE_CHEST_SHOP_GAL, { ACTOR_EN_TAKARAYA }, { RC_CLOCK_TOWN_EAST_TREASURE_CHEST_GAME_DEKU, RC_CLOCK_TOWN_EAST_TREASURE_CHEST_GAME_GORON, RC_CLOCK_TOWN_EAST_TREASURE_CHEST_GAME_HUMAN, RC_CLOCK_TOWN_EAST_TREASURE_CHEST_GAME_ZORA }, {} },
    // apworld 0.12 (level 2). Rows that list nothing only hide their NPCs: guards and other blockers, whose absence
    // only makes the way easier. Where a check might need the NPC, it is listed anyway.
    { RI_SOUL_NPC_KAFEI, { ACTOR_EN_TEST3 }, { RC_KAFEIS_HIDEOUT_PENDANT_OF_MEMORIES, RC_STOCK_POT_INN_COUPLES_MASK }, { RE_MEET_KAFEI, RE_RETRIEVE_SUN_MASK }, 2 },
    { RI_SOUL_NPC_BOMB_SHOP_LADY, { ACTOR_EN_BABA }, { RC_CLOCK_TOWN_NORTH_BOMB_LADY }, { RE_SAVE_BOMB_SHOP_LADY }, 2 },
    { RI_SOUL_NPC_CURIOSITY_SHOP_OWNER, { ACTOR_EN_FSN }, { RC_CURIOSITY_SHOP_SPECIAL_ITEM, RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM, RC_KAFEIS_HIDEOUT_KEATON_MASK, RC_KAFEIS_HIDEOUT_LETTER_TO_MAMA }, {}, 2 },
    { RI_SOUL_NPC_BOMB_SHOP_OWNER, { ACTOR_EN_OSSAN }, { RC_BOMB_SHOP_ITEM_01, RC_BOMB_SHOP_ITEM_02, RC_BOMB_SHOP_ITEM_03, RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM }, {}, 2, SCENE_BOMYA },
    { RI_SOUL_NPC_GORON_SHOPKEEPER, { ACTOR_EN_OSSAN }, { RC_GORON_SHOP_ITEM_01, RC_GORON_SHOP_ITEM_02, RC_GORON_SHOP_ITEM_03 }, {}, 2, SCENE_GORONSHOP },
    { RI_SOUL_NPC_ZORA_SHOPKEEPER, { ACTOR_EN_OSSAN }, { RC_ZORA_SHOP_ITEM_01, RC_ZORA_SHOP_ITEM_02, RC_ZORA_SHOP_ITEM_03 }, {}, 2, SCENE_BANDROOM },
    { RI_SOUL_NPC_TRADING_POST, { ACTOR_EN_OSSAN }, { RC_TRADING_POST_SHOP_ITEM_01, RC_TRADING_POST_SHOP_ITEM_02, RC_TRADING_POST_SHOP_ITEM_03, RC_TRADING_POST_SHOP_ITEM_04, RC_TRADING_POST_SHOP_ITEM_05, RC_TRADING_POST_SHOP_ITEM_06, RC_TRADING_POST_SHOP_ITEM_07, RC_TRADING_POST_SHOP_ITEM_08 }, {}, 2, SCENE_8ITEMSHOP },
    { RI_SOUL_NPC_KOTAKE, { ACTOR_EN_TRT, ACTOR_EN_TRT2 }, { RC_HAGS_POTION_SHOP_ITEM_01, RC_HAGS_POTION_SHOP_ITEM_02, RC_HAGS_POTION_SHOP_ITEM_03, RC_HAGS_POTION_SHOP_KOTAKE }, { RE_ACCESS_RED_POTION_REFILL }, 2 },
    { RI_SOUL_NPC_KOUME, { ACTOR_EN_TRU, ACTOR_EN_TRU_MT, ACTOR_EN_DNH, ACTOR_EN_SHN }, { RC_TOURIST_INFORMATION_ARCHERY, RC_TOURIST_INFORMATION_PICTOBOX, RC_TOURIST_INFORMATION_GOOD_PHOTO }, { RE_SAVED_KOUME, RE_SOUTHERN_SWAMP_RIDE_BOAT }, 2 },
    { RI_SOUL_NPC_DEKU_KING, { ACTOR_EN_DNQ }, {}, { RE_RETURN_DEKU_PRINCESS, RE_ACCESS_PICTOGRAPH_DEKU_KING }, 2 },
    { RI_SOUL_NPC_DEKU_PRINCESS, { ACTOR_EN_DNP }, {}, { RE_ACCESS_DEKU_PRINCESS, RE_RETURN_DEKU_PRINCESS }, 2 },
    { RI_SOUL_NPC_DEKU_PALACE_RESIDENTS, { ACTOR_EN_GUARD_NUTS, ACTOR_EN_LOOK_NUTS, ACTOR_EN_DNS, ACTOR_EN_MS }, {}, {}, 2 },
    { RI_SOUL_NPC_MONKEY, { ACTOR_EN_MNK }, { RC_DEKU_KINGS_CHAMBER_MONKEY }, {}, 2 },
    { RI_SOUL_NPC_BUSINESS_SCRUBS, { ACTOR_EN_AKINDONUTS, ACTOR_EN_SELLNUTS, ACTOR_EN_SCOPENUTS }, { RC_CLOCK_TOWN_SCRUB_DEED, RC_SOUTHERN_SWAMP_SCRUB_DEED, RC_SOUTHERN_SWAMP_SCRUB_BEANS, RC_SOUTHERN_SWAMP_PIECE_OF_HEART, RC_GORON_VILLAGE_SCRUB_DEED, RC_GORON_VILLAGE_SCRUB_BOMB_BAG, RC_GORON_VILLAGE_PIECE_OF_HEART, RC_ZORA_HALL_SCRUB_DEED, RC_ZORA_HALL_SCRUB_POTION_REFILL, RC_ZORA_HALL_SCRUB_PIECE_OF_HEART, RC_IKANA_CANYON_SCRUB_HUGE_RUPEE, RC_IKANA_CANYON_SCRUB_POTION_REFILL, RC_IKANA_CANYON_SCRUB_PIECE_OF_HEART, RC_TERMINA_FIELD_GROTTO_SCRUB }, { RE_ACCESS_BLUE_POTION_REFILL, RE_ACCESS_GREEN_POTION_REFILL, RE_ACCESS_BEANS_REFILL, RE_TERMINA_FIELD_SCRUB_ENTERED_GROTTO }, 2 },
    { RI_SOUL_NPC_GORONS, { ACTOR_EN_GO, ACTOR_EN_S_GORO, ACTOR_EN_IG, ACTOR_EN_DAI, ACTOR_EN_GEG }, { RC_GORON_VILLAGE_MEDIGORON, RC_MOUNTAIN_VILLAGE_DON_GERO_MASK }, {}, 2 },
    { RI_SOUL_NPC_GORON_ELDER_AND_SON, { ACTOR_EN_JG, ACTOR_EN_GK }, { RC_PATH_TO_GORON_VILLAGE_LULLABY_INTRO, RC_GORON_SHRINE_FULL_LULLABY }, { RE_SPAWN_SIRLOIN }, 2 },
    { RI_SOUL_NPC_GORON_RACERS, { ACTOR_EN_RG }, { RC_GORON_RACETRACK_GOLD_DUST }, { RE_ACCESS_GOLD_DUST }, 2 },
    { RI_SOUL_NPC_DARMANI, { ACTOR_EN_GG, ACTOR_EN_GG2 }, { RC_GORON_GRAVEYARD_DARMANI }, {}, 2 },
    { RI_SOUL_NPC_ZORAS, { ACTOR_EN_ZOT }, { RC_ZORA_HALL_SCENE_LIGHTS }, {}, 2 },
    { RI_SOUL_NPC_INDIGO_GOS, { ACTOR_EN_ZOG, ACTOR_EN_ZOB, ACTOR_EN_ZOD, ACTOR_EN_ZOV }, { RC_GREAT_BAY_COAST_MIKAU }, {}, 2 },
    { RI_SOUL_NPC_GREAT_BAY_FOLK, { ACTOR_EN_TSN, ACTOR_EN_JGAME_TSN, ACTOR_EN_OT, ACTOR_EN_MK }, { RC_GREAT_BAY_COAST_FISHERMAN_MINIGAME, RC_PINNACLE_ROCK_REUNITE_SEAHORSE }, { RE_ACCESS_SEAHORSE }, 2 },
    { RI_SOUL_NPC_ROMANI_AND_CREMIA, { ACTOR_EN_MA4, ACTOR_EN_MA_YTS, ACTOR_EN_MA_YTO, ACTOR_OBJ_UM }, { RC_ROMANI_RANCH_EPONAS_SONG, RC_ROMANI_RANCH_ALIENS, RC_CREMIA_ESCORT }, { RE_COWS_FROM_ALIENS }, 2 },
    { RI_SOUL_NPC_GORMAN, { ACTOR_EN_GM }, {}, {}, 2 },
    { RI_SOUL_NPC_MR_BARTEN, { ACTOR_EN_TAB }, { RC_MILK_BAR_PURCHASE_MILK, RC_MILK_BAR_PURCHASE_CHATEAU }, {}, 2 },
    { RI_SOUL_NPC_GUARDS_AND_OFFICIALS, { ACTOR_EN_STOP_HEISHI, ACTOR_EN_HEISHI, ACTOR_EN_BAISEN, ACTOR_EN_RECEPGIRL }, {}, {}, 2 },
    { RI_SOUL_NPC_CARPENTERS, { ACTOR_EN_DAIKU, ACTOR_EN_DAIKU2, ACTOR_EN_MUTO }, {}, {}, 2 },
    { RI_SOUL_NPC_BOMBERS, { ACTOR_EN_BOMBERS, ACTOR_EN_BOMBERS2, ACTOR_EN_BOMJIMA, ACTOR_EN_BOMJIMB }, { RC_CLOCK_TOWN_BOMBERS_NOTEBOOK }, { RE_HIDE_SEEK_DAY1, RE_HIDE_SEEK_DAY2, RE_HIDE_SEEK_DAY3, RE_BOMBERS_NORTH_DAY1, RE_BOMBERS_NORTH_DAY2, RE_BOMBERS_NORTH_DAY3, RE_BOMBERS_EAST_DAY1, RE_BOMBERS_EAST_DAY2, RE_BOMBERS_EAST_DAY3, RE_BOMBERS_WEST_DAY1, RE_BOMBERS_WEST_DAY2, RE_BOMBERS_WEST_DAY3, RE_BOMBER_CODE }, 2 },
    { RI_SOUL_NPC_TOWNSFOLK, { ACTOR_EN_AH, ACTOR_EN_JA, ACTOR_EN_ANI, ACTOR_EN_BJI_01, ACTOR_EN_STH2 }, {}, {}, 2 },
    { RI_SOUL_NPC_PAMELAS_FATHER, { ACTOR_EN_HG, ACTOR_EN_HGO }, { RC_MUSIC_BOX_HOUSE_FATHER }, {}, 2 },
    { RI_SOUL_NPC_COMPOSER_BROTHERS, { ACTOR_EN_PO_COMPOSER }, { RC_BENEATH_THE_GRAVEYARD_SONG_OF_STORMS }, { RE_BREAK_SHARP_CURSE }, 2 },
    { RI_SOUL_NPC_DAMPE, { ACTOR_EN_TK }, { RC_BENEATH_THE_GRAVEYARD_DAMPE_CHEST }, {}, 2 },
    { RI_SOUL_NPC_FROGS, { ACTOR_EN_MINIFROG }, { RC_CLOCK_TOWN_LAUNDRY_FROG, RC_GREAT_BAY_TEMPLE_GEKKO_FROG, RC_SOUTHERN_SWAMP_FROG, RC_WOODFALL_TEMPLE_GEKKO_FROG, RC_MOUNTAIN_VILLAGE_FROG_CHOIR }, {}, 2 },
    { RI_SOUL_NPC_KAEPORA_GAEBORA, { ACTOR_EN_OWL }, {}, {}, 2 },
    { RI_SOUL_NPC_TALKING_GIBDOS, { ACTOR_EN_TALK_GIBUD }, {}, {}, 2 },
    { RI_SOUL_NPC_PIRATES, { ACTOR_EN_GE1, ACTOR_EN_GE2, ACTOR_EN_GE3 }, {}, {}, 2 },
};

// Never hidden, whatever soul their actor belongs to: (params & mask) == value. They open a way the logic treats as
// open: the Goron Shrine gatekeeper opens the shrine (Regions/North.cpp, Goron Village exit to the shrine).
struct NpcSoulException {
    int16_t actorId;
    int16_t paramsMask;
    int16_t paramsValue;
};
const std::vector<NpcSoulException> npcSoulExceptions = {
    { ACTOR_EN_GO, 0xF, ENGO_GATEKEEPER }, // ENGO_GET_TYPE
};

// Actors that belong to the NPC they are attached to (actor->parent): Romani's balloon.
const std::vector<std::pair<int16_t, int16_t>> npcSoulCompanions = {
    { ACTOR_EN_PO_FUSEN, ACTOR_EN_MA4 },
};

// Soul actors with DynaPoly collision: while hidden their collision is off, it would be an invisible wall.
const std::vector<int16_t> npcSoulDynaActors = { ACTOR_OBJ_UM };
// clang-format on

bool HaveNpcSoul(Actor* actor) {
    int level = IS_RANDO ? RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] : RO_NPC_SOULS_OFF;
    if (level == RO_NPC_SOULS_OFF) {
        return true;
    }
    // per actor id: (scene or -1, flag) of each soul of the current level that owns it
    static std::vector<std::vector<std::pair<int16_t, RandoInf>>> soulsOfActor;
    static int soulsLevel = RO_NPC_SOULS_OFF;
    if (soulsLevel != level) {
        soulsLevel = level;
        soulsOfActor.assign(ACTOR_ID_MAX, {});
        for (const auto& npcSoul : npcSouls) {
            if (npcSoul.level > level) {
                continue;
            }
            for (int16_t npcActorId : npcSoul.actorIds) {
                soulsOfActor[npcActorId].push_back(
                    { npcSoul.sceneId, (RandoInf)NPC_SOUL_RI_TO_RANDO_INF(npcSoul.randoItemId) });
            }
        }
    }
    for (const auto& [childId, parentId] : npcSoulCompanions) {
        if (actor->id == childId && actor->parent != NULL && actor->parent->id == parentId) {
            actor = actor->parent;
            break;
        }
    }
    for (const auto& exception : npcSoulExceptions) {
        if (actor->id == exception.actorId && (actor->params & exception.paramsMask) == exception.paramsValue) {
            return true;
        }
    }
    if (actor->id < 0 || actor->id >= ACTOR_ID_MAX) {
        return true;
    }
    for (const auto& [sceneId, flag] : soulsOfActor[actor->id]) {
        if (sceneId == -1 || (gPlayState != NULL && sceneId == gPlayState->sceneId)) {
            return Flags_GetRandoInf(flag);
        }
    }
    return true;
}

bool HaveEnemySoul(ActorId enemyId) {
    auto findSoulFlag = enemySoulMap.find(enemyId);
    if (findSoulFlag != enemySoulMap.end()) {
        RandoItemId randoItemId = findSoulFlag->second;
        if (randoItemId != RI_UNKNOWN) {
            return Flags_GetRandoInf(SOUL_RI_TO_RANDO_INF(randoItemId));
        }
    }
    // Enemy soul does not exist, so act as if it is obtained
    return true;
}

void ShouldActorUpdate(Actor* actor, bool* should, RandoInf randoInf) {
    if (!Flags_GetRandoInf(randoInf)) {
        *should = false;
        actor->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    } else if (!actor->flags & ACTOR_FLAG_ATTENTION_ENABLED) {
        actor->flags |= ACTOR_FLAG_ATTENTION_ENABLED;
    }
}

void ShouldActorDraw(Actor* actor, bool* should, RandoInf randoInf) {
    if (!Flags_GetRandoInf(randoInf)) {
        *should = false;
    }
}

void SetEnemyInjureGrayscale(bool enable) {
    OPEN_DISPS(gPlayState->state.gfxCtx);
    if (enable) {
        gDPSetGrayscaleColor(POLY_OPA_DISP++, 70, 70, 70, 255);
        gDPSetGrayscaleColor(POLY_XLU_DISP++, 70, 70, 70, 255);
    }
    gSPGrayscale(POLY_OPA_DISP++, enable);
    gSPGrayscale(POLY_XLU_DISP++, enable);
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void Rando::ActorBehavior::InitSoulsBehavior() {
    bool shouldBossRegister = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] == RO_GENERIC_YES;
    bool shouldEnemyInjure = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] == RO_GENERIC_YES;
    bool shouldNpcRegister = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] != RO_NPC_SOULS_OFF;

    // NPCs without their soul: no update (no talking, no rewards, no collider, no sound) and no draw (and so no
    // shadow, z_actor.c). They cannot be targeted either, and DynaPoly collision is off; both come back with the soul.
    static std::unordered_set<Actor*> untargetable;
    static std::unordered_set<Actor*> collisionOff;
    COND_HOOK(OnSceneInit, shouldNpcRegister, [](s8 sceneId, s8 spawnNum) {
        untargetable.clear();
        collisionOff.clear();
    });
    COND_HOOK(ShouldActorUpdate, shouldNpcRegister, [](Actor* actor, bool* should) {
        bool isDyna =
            std::find(npcSoulDynaActors.begin(), npcSoulDynaActors.end(), actor->id) != npcSoulDynaActors.end();
        if (!HaveNpcSoul(actor)) {
            *should = false;
            if (actor->flags & ACTOR_FLAG_ATTENTION_ENABLED) {
                actor->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
                untargetable.insert(actor);
            }
            s32 bgId = isDyna ? ((DynaPolyActor*)actor)->bgId : BGACTOR_NEG_ONE;
            if (bgId != BGACTOR_NEG_ONE && !collisionOff.count(actor)) {
                DynaPoly_DisableCollision(gPlayState, &gPlayState->colCtx.dyna, bgId);
                collisionOff.insert(actor);
            }
        } else {
            if (!untargetable.empty() && untargetable.erase(actor)) {
                actor->flags |= ACTOR_FLAG_ATTENTION_ENABLED;
            }
            if (isDyna && !collisionOff.empty() && collisionOff.erase(actor)) {
                DynaPoly_EnableCollision(gPlayState, &gPlayState->colCtx.dyna, ((DynaPolyActor*)actor)->bgId);
            }
        }
    });
    COND_HOOK(ShouldActorDraw, shouldNpcRegister, [](Actor* actor, bool* should) {
        if (!HaveNpcSoul(actor)) {
            *should = false;
        }
    });

    COND_VB_SHOULD(VB_PERFORM_AC_COLLISION, shouldEnemyInjure, {
        Collider* at = va_arg(args, Collider*);
        Collider* ac = va_arg(args, Collider*);
        *should = HaveEnemySoul((ActorId)ac->actor->id);
    });

    // ShouldActorDraw & ShouldActorUpdate for Boss Souls
    COND_ID_HOOK(ShouldActorDraw, ACTOR_BOSS_HAKUGIN, shouldBossRegister, [](Actor* actor, bool* should) {
        if (!Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT)) {
            BossHakugin_DrawIce((BossHakugin*)actor, gPlayState);
            *should = false;
        }
    });

    COND_VB_SHOULD(VB_GOHT_UNFREEZE, shouldBossRegister, {
        if (!Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT)) {
            *should = false;
        }
    });

    COND_ID_HOOK(ShouldActorDraw, ACTOR_BOSS_03, shouldBossRegister, [](Actor* actor, bool* should) {
        ShouldActorDraw(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_GYORG);
    });

    COND_ID_HOOK(ShouldActorDraw, ACTOR_BOSS_07, shouldMajoraRegister(), [](Actor* actor, bool* should) {
        ShouldActorDraw(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_MAJORA);
    });

    COND_ID_HOOK(ShouldActorDraw, ACTOR_BOSS_01, shouldBossRegister, [](Actor* actor, bool* should) {
        ShouldActorDraw(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_ODOLWA);
    });

    COND_ID_HOOK(ShouldActorDraw, ACTOR_BOSS_02, shouldBossRegister, [](Actor* actor, bool* should) {
        ShouldActorDraw(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_TWINMOLD);
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BOSS_03, shouldBossRegister, [](Actor* actor, bool* should) {
        ShouldActorUpdate(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_GYORG);
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BOSS_07, shouldMajoraRegister(), [](Actor* actor, bool* should) {
        ShouldActorUpdate(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_MAJORA);
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BOSS_01, shouldBossRegister, [](Actor* actor, bool* should) {
        ShouldActorUpdate(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_ODOLWA);
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BOSS_02, shouldBossRegister, [](Actor* actor, bool* should) {
        ShouldActorUpdate(actor, should, RANDO_INF_OBTAINED_SOUL_OF_BOSS_TWINMOLD);
    });

    /*
     * Giant's Mask functionality is handled by two pieces. The scene (Twinmold's Lair) determines whether the mask can
     * be used, while the Twinmold actor itself handles the transformation. Boss Souls prevent Twinmold from updating
     * unless its soul has been obtained, which results in a softlock. In this case, disable the item.
     */
    COND_VB_SHOULD(VB_ITEM_BE_RESTRICTED, shouldBossRegister, {
        ItemId itemId = *va_arg(args, ItemId*);
        if (itemId == ITEM_MASK_GIANT && gPlayState->sceneId == SCENE_INISIE_BS &&
            !Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_TWINMOLD)) {
            *should = true;
        }
    });

    COND_VB_SHOULD(VB_DRAW_LOCK_ON_ARROW, shouldEnemyInjure, {
        Actor* refActor = va_arg(args, Actor*);
        ActorId actorId = (ActorId)refActor->id;
        // ACTOR_EN_INVADEPOH represents multiple actors, including Romani and the dog. The aliens cannot be targeted
        // anyway, so just don't draw this arrow if the actor is ACTOR_EN_INVADEPOH.
        if (actorId != ACTOR_EN_INVADEPOH && !HaveEnemySoul(actorId)) {
            DrawEnLight({ 155, 0, 0 }, { 1.0f, 1.0f, 1.0f });
            *should = false;
        }
    });

    COND_HOOK(ShouldActorDraw, shouldEnemyInjure, [](Actor* actor, bool* should) {
        ActorId actorId = (ActorId)actor->id;
        if (actorId != ACTOR_EN_INVADEPOH && !HaveEnemySoul(actorId)) {
            SetEnemyInjureGrayscale(true);
        }
    });

    COND_HOOK(OnActorDraw, shouldEnemyInjure, [](Actor* actor) {
        ActorId actorId = (ActorId)actor->id;
        if (actorId != ACTOR_EN_INVADEPOH && !HaveEnemySoul(actorId)) {
            SetEnemyInjureGrayscale(false);
        }
    });
}
