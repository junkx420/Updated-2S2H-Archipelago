#include "EntranceShuffle.h"
#include "Rando/Logic/Logic.h"
#include "2s2h/GameInteractor/GameInteractor.h"

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

static constexpr size_t MAX_ROWS = sizeof(RandoSaveInfo::entrancePairs);

static u8* SavedPairs() {
    return gSaveContext.save.shipSaveInfo.rando.entrancePairs;
}

// Row of the area behind this door.
static size_t InsideBehind(size_t door) {
    if (RANDO_SAVE_OPTIONS[shuffleEntrances[door].option] == RO_GENERIC_OFF) {
        return door;
    }
    u8 value = SavedPairs()[door];
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
    memset(SavedPairs(), 0, MAX_ROWS);
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
            SavedPairs()[door] = (u8)(inside + 1);
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

void OnBeforeTransitionSetup() {
    PlayState* play = gPlayState;
    if (play == nullptr) {
        return;
    }
    u16 next = play->nextEntrance;
    s16 scene = play->sceneId;

    // The scene check matters: a void out inside a shuffled area respawns at that area's (already redirected)
    // "into" entrance, which must not be redirected a second time.
    for (size_t door = 0; door < shuffleEntrances.size() && door < MAX_ROWS; door++) {
        const ShuffleEntrance& doorRow = shuffleEntrances[door];
        if (RANDO_SAVE_OPTIONS[doorRow.option] == RO_GENERIC_OFF) {
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
            if (shuffleEntrances[temple].templeExit == 0 && shuffleEntrances[temple].outOf == exterior) {
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
