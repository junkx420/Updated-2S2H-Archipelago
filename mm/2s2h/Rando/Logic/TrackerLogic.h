#ifndef RANDO_TRACKER_LOGIC_H
#define RANDO_TRACKER_LOGIC_H

#include "2s2h/Rando/Logic/Logic.h"
#include "2s2h/Rando/MiscBehavior/EntranceShuffle.h"

namespace Rando::Logic {

struct TrackerReachability {
    std::set<RandoRegionId> regions;
    std::set<RandoCheckId> checks;
};

bool TrackerHasForkRules();
TrackerReachability EvaluateTrackerReachability(RandoRegionId currentRegion, bool useForkRules,
                                                bool openChecksOnly = true);

// Graph entrances are already logical IDs, including the virtual grotto spawns.
RandoRegionId GetRegionIdFromLogicEntrance(s32 entrance);
void FindReachableRegions(RandoRegionId currentRegion, std::set<RandoRegionId>& reachableRegions,
                          std::unordered_map<RandoRegionId, RegionTimeState>& regionTimeStates,
                          const EntranceShuffle::TrackerExits* trackerExits);

} // namespace Rando::Logic

#endif // RANDO_TRACKER_LOGIC_H
