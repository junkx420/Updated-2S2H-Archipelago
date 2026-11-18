#include "Abilities.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/Rando.h"

extern "C" {
#include "functions.h"
}

namespace Rando::Abilities {

static constexpr Ability abilities[] = {
    { RO_SHUFFLE_EPONA_RIDING, RI_ABILITY_RIDE_EPONA, RANDO_INF_OBTAINED_ABILITY_RIDE_EPONA },
    { RO_SHUFFLE_BEAN_PLANTING, RI_ABILITY_PLANT_BEANS, RANDO_INF_OBTAINED_ABILITY_PLANT_BEANS },
};

// The AP generator adds the ability to the matching 2S2H macros. Gate both bean macros because rain has its own path.
[[maybe_unused]] static constexpr struct {
    RandoItemId item;
    const char* name;
} abilityMacros[] = {
    { RI_ABILITY_RIDE_EPONA, "CAN_RIDE_EPONA" },
    { RI_ABILITY_PLANT_BEANS, "CAN_GROW_BEAN_PLANT" },
    { RI_ABILITY_PLANT_BEANS, "CAN_USE_DAY2_RAIN_BEAN" },
};

// Some regions test the song directly. Only gate these riding branches, preserving cows and the Night 2 escape.
[[maybe_unused]] static constexpr struct {
    RandoItemId item;
    RandoRegionId from;
    const char* target;
    const char* branch;
} abilityRules[] = {
    { RI_ABILITY_RIDE_EPONA, RR_TERMINA_FIELD, "RR_TERMINA_FIELD_BEFORE_GREAT_BAY_COAST", "CAN_PLAY_SONG(EPONA)" },
    { RI_ABILITY_RIDE_EPONA, RR_TERMINA_FIELD_BEFORE_GREAT_BAY_COAST, "RR_TERMINA_FIELD", "CAN_PLAY_SONG(EPONA)" },
    { RI_ABILITY_RIDE_EPONA, RR_GORMAN_TRACK_FRONT, "RC_GORMAN_TRACK_GARO_MASK", "CAN_PLAY_SONG(EPONA)" },
    { RI_ABILITY_RIDE_EPONA, RR_GORMAN_TRACK, "RR_GORMAN_TRACK_FRONT", "CAN_PLAY_SONG(EPONA)" },
    { RI_ABILITY_RIDE_EPONA, RR_GORMAN_TRACK, "RR_GORMAN_TRACK_BACK", "CAN_PLAY_SONG(EPONA)" },
};

const Ability* FindByItem(RandoItemId item) {
    for (const auto& ability : abilities) {
        if (ability.item == item) {
            return &ability;
        }
    }
    return nullptr;
}

static bool CanUse(RandoItemId item) {
    const auto* ability = FindByItem(item);
    return !RANDO_SAVE_OPTIONS[ability->option] || Flags_GetRandoInf(ability->flag);
}

void RegisterHooks() {
    COND_VB_SHOULD(VB_RIDE_EPONA, IS_RANDO, { *should = *should && CanUse(RI_ABILITY_RIDE_EPONA); });
    COND_VB_SHOULD(VB_PLANT_MAGIC_BEAN, IS_RANDO, { *should = *should && CanUse(RI_ABILITY_PLANT_BEANS); });
}

} // namespace Rando::Abilities
