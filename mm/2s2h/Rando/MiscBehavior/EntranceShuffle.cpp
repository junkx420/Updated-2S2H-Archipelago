#include "EntranceShuffle.h"
#include "Rando/Logic/Logic.h"
#include "2s2h/GameInteractor/GameInteractor.h"

#include <algorithm>

extern "C" {
#include "variables.h"
#include "functions.h"
}

namespace Rando {

namespace EntranceShuffle {

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
};
// clang-format on

// Elements, not bytes: the field is u16 since apworld 0.6 (u8[32] before; GetGrownArray loads both).
static constexpr size_t MAX_ROWS = sizeof(RandoSaveInfo::entrancePairs) / sizeof(RandoSaveInfo::entrancePairs[0]);

static u16* SavedPairs() {
    return gSaveContext.save.shipSaveInfo.rando.entrancePairs;
}

// Row of the area behind this door.
static size_t InsideBehind(size_t door) {
    if (RANDO_SAVE_OPTIONS[shuffleEntrances[door].option] == RO_GENERIC_OFF) {
        return door;
    }
    u16 value = SavedPairs()[door];
    return (value == 0 || value > shuffleEntrances.size() ||
            shuffleEntrances[value - 1].option != shuffleEntrances[door].option)
               ? door
               : value - 1;
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
        if (door < MAX_ROWS && inside < MAX_ROWS && shuffleEntrances[door].option == shuffleEntrances[inside].option) {
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

void OnBeforeTransitionSetup() {
    PlayState* play = gPlayState;
    if (play == nullptr) {
        return;
    }
    if (RedirectGrotto(play)) {
        return;
    }
    u16 next = play->nextEntrance;
    s16 scene = play->sceneId;

    // The scene check matters: a void out inside a shuffled area respawns at that area's (already redirected)
    // "into" entrance, which must not be redirected a second time.
    for (size_t door = 0; door < shuffleEntrances.size() && door < MAX_ROWS; door++) {
        const ShuffleEntrance& doorRow = shuffleEntrances[door];
        if (RANDO_SAVE_OPTIONS[doorRow.option] == RO_GENERIC_OFF || IsGrottoRow(door)) {
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
            if (!IsGrottoRow(temple) && shuffleEntrances[temple].templeExit == 0 &&
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
