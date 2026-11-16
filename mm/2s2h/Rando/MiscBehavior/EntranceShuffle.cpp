#include "EntranceShuffle.h"
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
    // Twinmold is reached through the inverted temple, whose door is not shuffled: its warp stays vanilla
    { RO_SHUFFLE_DUNGEON_ENTRANCES, ENTRANCE(STONE_TOWER_TEMPLE, 0), ENTRANCE(STONE_TOWER, 2), SCENE_INISIE_N, SCENE_F40,          SCENE_MAX,        {} },
};
// clang-format on

static constexpr size_t MAX_ROWS = sizeof(RandoSaveInfo::entrancePairs);

static u8* SavedPairs() {
    return gSaveContext.save.shipSaveInfo.rando.entrancePairs;
}

// Row of the area behind this door.
static size_t InsideBehind(size_t door) {
    u8 value = SavedPairs()[door];
    return (value == 0 || value > shuffleEntrances.size()) ? door : value - 1;
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
        default:
            break;
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
        if (scene == insideRow.insideScene && next == insideRow.outOf) {
            play->nextEntrance = doorRow.outOf;
            return;
        }
        if (insideRow.bossScene == SCENE_MAX || scene != insideRow.bossScene) {
            continue;
        }

        // First warp: it heads for the Giants' Chamber, and SkipGiantsChamber swaps in the transition that
        // HandleGiantsCutsceneSkip queued (tagged with the boss scene). Point that one at the door instead.
        if (next == ENTRANCE(GIANTS_CHAMBER, 0)) {
            for (auto& event : GameInteractor::Instance->events) {
                if (auto transition = std::get_if<GIEventTransition>(&event)) {
                    if (transition->transitionType == insideRow.bossScene) {
                        transition->entrance = doorRow.outOf;
                        transition->cutsceneIndex = 0;
                    }
                }
            }
            return;
        }
        // Repeat warps go straight to the region's clear cutscene: skip it, keep its effect.
        for (u16 warpTarget : insideRow.bossWarpsTo) {
            if (next == warpTarget) {
                MarkCleared(insideRow.bossScene);
                play->nextEntrance = doorRow.outOf;
                gSaveContext.nextCutsceneIndex = 0xFFEF;
                return;
            }
        }
    }
}

} // namespace EntranceShuffle

} // namespace Rando
