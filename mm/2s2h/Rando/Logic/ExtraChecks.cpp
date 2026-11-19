#include "ExtraChecks.h"

#include "Logic.h"

namespace Rando::Logic {
namespace {

bool HasHalfDay(unsigned mask) {
    if (mask == 0x3F) {
        return true;
    }
    return ((mask & 1) && IS_DAY1()) || ((mask & 2) && IS_NIGHT1()) || ((mask & 4) && IS_DAY2()) ||
           ((mask & 8) && IS_NIGHT2()) || ((mask & 16) && IS_DAY3()) || ((mask & 32) && IS_NIGHT3());
}

enum CheckLayout { ANY, SNOWHEAD_SPRING, GREAT_BAY_BASE };

bool IsLayoutReachable(CheckLayout layout) {
    // The base Great Bay layout is available in a fresh cycle.
    return layout != SNOWHEAD_SPRING || RANDO_EVENTS[RE_CLEARED_SNOWHEAD_TEMPLE];
}

} // namespace

void RegisterExtraChecks() {
#define EXTRA_CHECK(id, type, scene, name, region, mask, layout, condition) \
    Regions.at(region).checks.try_emplace(                                  \
        id, [] { return HasHalfDay(mask) && IsLayoutReachable(layout) && (condition); }, LogicString(#condition));
#include "ExtraChecks.inc"
#undef EXTRA_CHECK
}

} // namespace Rando::Logic
