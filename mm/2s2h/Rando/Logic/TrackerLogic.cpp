#include "TrackerLogic.h"
#include "2s2h/Rando/ActorBehavior/Souls.h"
#include "2s2h/Rando/MiscBehavior/Abilities.h"
#include <algorithm>

namespace Rando::EntranceShuffle {
namespace {

struct LogicEndpoints {
    RandoRegionId outside;
    RandoRegionId inside;
};

const std::vector<LogicEndpoints>& LogicEndpointsByRow() {
    static const auto endpoints = [] {
        std::vector<LogicEndpoints> result;
        result.reserve(shuffleEntrances.size());
        for (const auto& row : shuffleEntrances) {
            // Overworld places can also be grotto returns. Match the exit in
            // its own scene, just as the apworld generator does.
            const auto door = std::find_if(Logic::Regions.begin(), Logic::Regions.end(), [&row](const auto& entry) {
                const auto& region = entry.second;
                return region.sceneId == row.outsideScene && region.exits.contains(row.into);
            });
            result.push_back(
                { door != Logic::Regions.end() ? door->first : RR_MAX, Logic::GetRegionIdFromLogicEntrance(row.into) });
        }
        return result;
    }();
    return endpoints;
}

const std::function<bool()>* ForeignAreaRule(u16 original, u16 arrival) {
    if (original != arrival) {
        for (const auto& area : shuffleAreaRules) {
            if (area.into == arrival) {
                return &area.condition;
            }
        }
    }
    return nullptr;
}

} // namespace

bool HasActiveShuffle() {
    return RANDO_SAVE_OPTIONS[RO_SHUFFLE_OWL_WARPS] ||
           std::any_of(shuffleEntrances.begin(), shuffleEntrances.end(),
                       [](const auto& row) { return RANDO_SAVE_OPTIONS[row.option] != RO_GENERIC_OFF; });
}

TrackerExits BuildTrackerExits() {
    TrackerExits result;
    if (!HasActiveShuffle()) {
        return result;
    }
    for (const auto& owl : owlWarps) {
        const size_t target = OwlWarpTarget(owl.id);
        if (target != owl.id) {
            const auto& arrival = owlWarps[target];
            result[{ RR_MAX, owl.entrance }] = { arrival.region, &arrival.canLeave };
        }
    }
    const auto& regions = LogicEndpointsByRow();
    for (size_t i = 0; i < shuffleEntrances.size(); i++) {
        const auto& row = shuffleEntrances[i];
        if (!RANDO_SAVE_OPTIONS[row.option]) {
            continue;
        }
        const auto [outside, inside] = regions[i];
        const auto behind = InsideBehind(i);
        if (behind != i) {
            result[{ outside, row.into }] = { regions[behind].inside,
                                              ForeignAreaRule(row.into, shuffleEntrances[behind].into) };
        }
        // Each overworld direction has its own row. Boss returns are one-way warps.
        if (row.option != RO_SHUFFLE_OVERWORLD && !row.templeExit) {
            const auto door = DoorForInside(i);
            // A fixed grotto may still need to return to its hole's exact region.
            if (door != i || row.option == RO_SHUFFLE_GROTTOS) {
                result[{ inside, row.outOf }] = { regions[door].outside };
            }
        }
    }

    for (size_t end = 0; end < 2 * shuffleEntrances.size(); end++) {
        const auto partner = MixedPartner(end);
        if (partner == end) {
            continue;
        }
        const auto& row = shuffleEntrances[end / 2];
        const auto [outside, inside] = regions[end / 2];
        const auto [targetOutside, targetInside] = regions[partner / 2];
        const auto exit = end % 2 ? row.outOf : row.into;
        result[{ end % 2 ? inside : outside, exit }] = { partner % 2 ? targetInside : targetOutside,
                                                         ForeignAreaRule(exit, EndpointArrival(partner)) };
    }

    for (size_t i = 0; i < shuffleEntrances.size(); i++) {
        const auto& row = shuffleEntrances[i];
        if (!row.templeExit) {
            continue;
        }
        const auto exterior = ShuffledBossExterior(i);
        if (exterior) {
            const auto target = Logic::GetRegionIdFromLogicEntrance(*exterior);
            for (const auto& [id, region] : Logic::Regions) {
                if (region.sceneId != row.bossScene) {
                    continue;
                }
                for (u16 warp : row.bossWarpsTo) {
                    if (region.exits.contains(warp)) {
                        result[{ id, warp }] = { target };
                    }
                }
            }
        }
    }
    return result;
}

} // namespace Rando::EntranceShuffle

namespace Rando::Logic {

bool TrackerHasForkRules() {
    // Rocks also exist in native seeds. AP sets the existing Zora Hall key
    // compatibility flag for every version that supports Shuffle Rocks.
    return RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] || RANDO_SAVE_OPTIONS[RO_SHUFFLE_DOOR_KEYS] ||
           RANDO_SAVE_OPTIONS[RO_SHUFFLE_EPONA_RIDING] || RANDO_SAVE_OPTIONS[RO_SHUFFLE_BEAN_PLANTING] ||
           ((RANDO_SAVE_OPTIONS[RO_SHUFFLE_ROCKS] || RANDO_SAVE_OPTIONS[RO_SHUFFLE_SIGNS]) &&
            Flags_GetRandoInf(RANDO_INF_DOOR_KEYS_ZORA_HALL)) ||
           EntranceShuffle::HasActiveShuffle();
}

TrackerReachability EvaluateTrackerReachability(RandoRegionId currentRegion, bool useForkRules, bool openChecksOnly) {
    TrackerReachability result;
    auto& reachableRegions = result.regions;
    reachableRegions.insert(RR_MAX);
    if (!useForkRules) {
        reachableRegions.insert(currentRegion);
    }
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

    const auto exits = useForkRules ? EntranceShuffle::BuildTrackerExits() : EntranceShuffle::TrackerExits{};
    bool changed = true;
    while (changed) {
        changed = false;
        const auto regionsToExplore = reachableRegions;
        for (RandoRegionId regionId : regionsToExplore) {
            FindReachableRegions(regionId, reachableRegions, regionTimeStates, useForkRules ? &exits : nullptr);
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
