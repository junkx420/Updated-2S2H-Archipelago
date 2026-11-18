#ifndef RANDO_ENTRANCE_TRACKER_LIST_H
#define RANDO_ENTRANCE_TRACKER_LIST_H

#include "Rando/MiscBehavior/EntranceShuffle.h"
#include <functional>
#include <string>
#include <vector>

namespace Rando {

namespace EntranceTracker {

// What the entrance tracker window shows, without ImGui (tested in the apworld's tools/entrance_client).
// Unless showAll is set, nothing in a row may depend on an undiscovered target: its "to" is "???", and search,
// tooltip, grouping, sorting and highlighting use only the side the line starts from.
struct Options {
    bool showAll = false;
    bool groupByPool = false;             // else by area: the scene of the side the line starts from
    std::string search;                   // case-insensitive, matched against the visible text of a row
    s16 currentScene = -1;                // play->sceneId
    size_t currentGrottoRow = (size_t)-1; // EntranceShuffle::CurrentGrottoRow()
    std::function<std::string(s16)> sceneName;
};

struct Row {
    std::string group;
    std::string from;
    std::string to; // "???" while undiscovered and not showAll
    std::string tooltip;
    bool discovered;
    bool current; // a door of the current scene, or the line that leads into the current area (once visible)
    bool twoWay;  // a door pool's coupled pair, "A <=> B" in the spoiler log
    size_t fromEndpoint;
};

extern const char* const HIDDEN_TARGET;

const char* PoolName(RandoOptionId pool, bool mixed);

// Sorted by group, then by "from" (both from the starting side only).
std::vector<Row> BuildRows(const std::vector<EntranceShuffle::TrackedEntrance>& lines, const Options& options);

} // namespace EntranceTracker

} // namespace Rando

#endif // RANDO_ENTRANCE_TRACKER_LIST_H
