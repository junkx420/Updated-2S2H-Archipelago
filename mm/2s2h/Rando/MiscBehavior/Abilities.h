#ifndef RANDO_ABILITIES_H
#define RANDO_ABILITIES_H

#include "2s2h/Rando/Types.h"
#include <functional>
#include <string_view>

namespace Rando::Abilities {

struct Ability {
    RandoOptionId option;
    RandoItemId item;
    RandoInf flag;
};

const Ability* FindByItem(RandoItemId item);
bool CanUse(RandoItemId item);
bool TrackerAllowsMacro(std::string_view name);
bool TrackerAllowsBranch(std::string_view name);
bool EvaluateTrackerRule(RandoRegionId region, RandoCheckId check, const std::function<bool()>& rule);
bool EvaluateTrackerRule(RandoRegionId region, RandoRegionId target, const std::function<bool()>& rule);
void RegisterHooks();

} // namespace Rando::Abilities

#endif
