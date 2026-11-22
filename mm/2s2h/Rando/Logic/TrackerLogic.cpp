#include "TrackerLogic.h"
#include "2s2h/Rando/ActorBehavior/Souls.h"
#include "2s2h/Rando/MiscBehavior/Abilities.h"

namespace Rando::Logic {

bool TrackerHasForkRules() {
    // Rocks also exist in native seeds. AP sets the existing Zora Hall key
    // compatibility flag for every version that supports Shuffle Rocks.
    return RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] || RANDO_SAVE_OPTIONS[RO_SHUFFLE_DOOR_KEYS] ||
           RANDO_SAVE_OPTIONS[RO_SHUFFLE_EPONA_RIDING] || RANDO_SAVE_OPTIONS[RO_SHUFFLE_BEAN_PLANTING] ||
           (RANDO_SAVE_OPTIONS[RO_SHUFFLE_ROCKS] && Flags_GetRandoInf(RANDO_INF_DOOR_KEYS_ZORA_HALL));
}

TrackerReachability EvaluateTrackerReachability(RandoRegionId currentRegion, bool useForkRules, bool openChecksOnly) {
    TrackerReachability result;
    auto& reachableRegions = result.regions;
    reachableRegions = { RR_MAX, currentRegion };
    auto regionTimeStates = InitializeRegionTimeStates(RR_MAX);
    std::set<const std::pair<RandoEvent, std::function<bool()>>*> appliedEvents;

    for (int i = 0; i < RE_MAX; i++) {
        RANDO_EVENTS[i] = 0;
    }

    auto isTracked = [openChecksOnly](RandoCheckId id) {
        const auto& check = RANDO_SAVE_CHECKS[id];
        return !openChecksOnly || (check.shuffled && !check.obtained);
    };
    if (useForkRules && RANDO_SAVE_OPTIONS[RO_LOGIC] != RO_LOGIC_GLITCHLESS) {
        // AP No Logic and Nearly No Logic treat every shuffled location as accessible.
        for (const auto& [regionId, region] : Regions) {
            reachableRegions.insert(regionId);
            for (const auto& [id, _] : region.checks) {
                if (isTracked(id)) {
                    result.checks.insert(id);
                }
            }
        }
        return result;
    }

    bool changed = true;
    while (changed) {
        changed = false;
        const auto regionsToExplore = reachableRegions;
        for (RandoRegionId regionId : regionsToExplore) {
            FindReachableRegions(regionId, reachableRegions, regionTimeStates, useForkRules);
        }

        for (RandoRegionId regionId : reachableRegions) {
            SetCurrentRegionTime(regionTimeStates, regionId);
            for (const auto& event : Regions[regionId].events) {
                if (!appliedEvents.contains(&event) &&
                    (useForkRules ? HaveNpcSoulsFor(event.first) &&
                                        Abilities::EvaluateTrackerRule(regionId, RR_MAX, event.second)
                                  : event.second())) {
                    RANDO_EVENTS[event.first]++;
                    appliedEvents.insert(&event);
                    changed = true;
                }
            }
        }
        changed |= reachableRegions.size() != regionsToExplore.size();
    }

    for (RandoRegionId regionId : reachableRegions) {
        SetCurrentRegionTime(regionTimeStates, regionId);
        for (const auto& [id, rule] : Regions[regionId].checks) {
            if (isTracked(id) &&
                (useForkRules ? HaveNpcSoulsFor(id) && Abilities::EvaluateTrackerRule(regionId, id, rule.first)
                              : rule.first())) {
                result.checks.insert(id);
            }
        }
    }
    return result;
}

} // namespace Rando::Logic
