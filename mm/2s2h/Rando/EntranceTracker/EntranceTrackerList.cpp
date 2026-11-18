#include "EntranceTrackerList.h"

#include <algorithm>
#include <cctype>

extern "C" {
#include "functions.h"
}

namespace Rando {

namespace EntranceTracker {

const char* const HIDDEN_TARGET = "???";

const char* PoolName(RandoOptionId pool, bool mixed) {
    if (mixed) {
        return "Mixed Pool";
    }
    switch (pool) {
        case RO_SHUFFLE_DUNGEON_ENTRANCES:
            return "Temples";
        case RO_SHUFFLE_BOSS_ENTRANCES:
            return "Boss Rooms";
        case RO_SHUFFLE_GROTTOS:
            return "Grottos";
        case RO_SHUFFLE_INTERIORS:
            return "Interiors";
        case RO_SHUFFLE_OVERWORLD:
            return "Overworld";
        case RO_SHUFFLE_SPECIAL_AREAS:
            return "Special Areas";
        default:
            return "Other";
    }
}

static std::string Lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return text;
}

// Rows name base scenes; the current one may be a variant (cleared swamp, spring). The inverted Stone Tower and its
// temple have rows of their own and are compared as they are, like EntranceShuffle.cpp MixedScene.
static bool InScene(s16 lineScene, s16 currentScene) {
    if (currentScene < 0) {
        return false;
    }
    if (currentScene == SCENE_INISIE_R || currentScene == SCENE_F41) {
        return lineScene == currentScene;
    }
    return lineScene == currentScene || lineScene == Play_GetOriginalSceneId(currentScene);
}

std::vector<Row> BuildRows(const std::vector<EntranceShuffle::TrackedEntrance>& lines, const Options& options) {
    const auto& table = EntranceShuffle::shuffleEntrances;
    std::string search = Lower(options.search);
    std::vector<Row> rows;
    for (const auto& line : lines) {
        bool discovered = EntranceShuffle::IsDiscovered(line.from);
        bool visible = options.showAll || discovered;
        std::string fromArea = options.sceneName ? options.sceneName(line.fromScene) : std::to_string(line.fromScene);
        Row row;
        row.group = options.groupByPool ? PoolName(line.pool, line.mixed) : fromArea;
        row.from = EntranceShuffle::FromName(line);
        row.to = visible ? EntranceShuffle::ToName(line) : HIDDEN_TARGET;
        row.discovered = discovered;
        row.twoWay = line.twoWay;
        row.fromEndpoint = line.from;
        row.tooltip = std::string(PoolName(line.pool, line.mixed)) + ", from " + fromArea;
        // Where the line arrives: the inside of a door pool's area, else the scene of the endpoint it leads to.
        size_t toRow = line.to / 2;
        s16 toScene = (line.twoWay || line.to % 2 != 0) ? table[toRow].insideScene : table[toRow].outsideScene;
        if (visible) {
            std::string toArea = options.sceneName ? options.sceneName(toScene) : std::to_string(toScene);
            row.tooltip += ", to " + toArea;
        }
        row.current = InScene(line.fromScene, options.currentScene);
        // Standing in the area a door pool's line leads to tells only what is already known once visible. Grottos
        // share one scene, so only the current grotto's line counts.
        if (!row.current && visible && line.twoWay && InScene(table[toRow].insideScene, options.currentScene)) {
            row.current = table[toRow].option != RO_SHUFFLE_GROTTOS || toRow == options.currentGrottoRow;
        }
        if (!search.empty() && Lower(row.group + " " + row.from + " " + row.to).find(search) == std::string::npos) {
            continue;
        }
        rows.push_back(std::move(row));
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.group != b.group) {
            return a.group < b.group;
        }
        if (a.from != b.from) {
            return a.from < b.from;
        }
        return a.fromEndpoint < b.fromEndpoint;
    });
    return rows;
}

} // namespace EntranceTracker

} // namespace Rando
