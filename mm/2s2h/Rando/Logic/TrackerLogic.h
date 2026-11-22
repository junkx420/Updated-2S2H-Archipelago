#ifndef RANDO_TRACKER_LOGIC_H
#define RANDO_TRACKER_LOGIC_H

#include "2s2h/Rando/Logic/Logic.h"

namespace Rando::Logic {

struct TrackerReachability {
    std::set<RandoRegionId> regions;
    std::set<RandoCheckId> checks;
};

bool TrackerHasForkRules();
TrackerReachability EvaluateTrackerReachability(RandoRegionId currentRegion, bool useForkRules,
                                                bool openChecksOnly = true);

void FindReachableRegions(RandoRegionId currentRegion, std::set<RandoRegionId>& reachableRegions,
                          std::unordered_map<RandoRegionId, RegionTimeState>& regionTimeStates, bool useForkRules);

} // namespace Rando::Logic

#endif // RANDO_TRACKER_LOGIC_H
