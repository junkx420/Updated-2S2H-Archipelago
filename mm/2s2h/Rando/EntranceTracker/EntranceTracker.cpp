#include "EntranceTracker.h"
#include "EntranceTrackerList.h"
#include "2s2h/BenGui/UIWidgets.hpp"
#include "2s2h/ShipUtils.h"

#include <imgui.h>
#include <map>

extern "C" {
#include "variables.h"
}

#define CVAR_NAME_SHOW_ENTRANCE_TRACKER "gWindows.EntranceTracker"
#define CVAR_NAME_SHOW_ALL "gRando.EntranceTracker.ShowAll"
#define CVAR_NAME_GROUP_BY_POOL "gRando.EntranceTracker.GroupByPool"
#define CVAR_NAME_HIGHLIGHT_CURRENT "gRando.EntranceTracker.HighlightCurrentScene"
#define CVAR_NAME_ONLY_CURRENT "gRando.EntranceTracker.OnlyCurrentScene"

namespace Rando {

namespace EntranceTracker {

namespace {

char sSearch[128] = "";

// The shuffled lines change only with the pairing and the options: rebuilt when either changes, not every frame.
std::vector<EntranceShuffle::TrackedEntrance> sLines;
u64 sLinesKey = 0;
bool sLinesValid = false;

u64 PairingKey() {
    const RandoSaveInfo& rando = gSaveContext.save.shipSaveInfo.rando;
    u64 hash = 1469598103934665603ull; // FNV-1a
    auto add = [&hash](const void* data, size_t size) {
        for (size_t i = 0; i < size; i++) {
            hash = (hash ^ ((const u8*)data)[i]) * 1099511628211ull;
        }
    };
    add(rando.entrancePairs, sizeof(rando.entrancePairs));
    add(rando.mixedEntrancePairs, sizeof(rando.mixedEntrancePairs));
    add(rando.randoSaveOptions, sizeof(rando.randoSaveOptions));
    return hash;
}

const std::vector<EntranceShuffle::TrackedEntrance>& Lines() {
    u64 key = PairingKey();
    if (!sLinesValid || key != sLinesKey) {
        sLines = EntranceShuffle::TrackedEntrances();
        sLinesKey = key;
        sLinesValid = true;
    }
    return sLines;
}

void CenteredText(const char* text) {
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) / 2);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2 - 10.0f);
    ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Gray), "%s", text);
}

void DrawRows(const std::vector<Row>& rows) {
    bool highlight = CVarGetInteger(CVAR_NAME_HIGHLIGHT_CURRENT, 1);
    bool onlyCurrent = CVarGetInteger(CVAR_NAME_ONLY_CURRENT, 0);
    // Groups in the sorted order of BuildRows, with their counts.
    std::vector<std::string> order;
    std::map<std::string, std::pair<int, int>> counts; // discovered, total
    for (const Row& row : rows) {
        if (onlyCurrent && !row.current) {
            continue;
        }
        if (counts.find(row.group) == counts.end()) {
            order.push_back(row.group);
        }
        counts[row.group].first += row.discovered;
        counts[row.group].second++;
    }
    size_t next = 0;
    for (const std::string& group : order) {
        std::string header = group + " (" + std::to_string(counts[group].first) + "/" +
                             std::to_string(counts[group].second) + ")###" + group;
        bool open = ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        while (next < rows.size() && rows[next].group != group) {
            next++;
        }
        if (!open) {
            continue;
        }
        if (!ImGui::BeginTable(("##" + group).c_str(), 3, ImGuiTableFlags_SizingStretchProp)) {
            continue;
        }
        ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Arrow", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        for (size_t i = next; i < rows.size() && rows[i].group == group; i++) {
            const Row& row = rows[i];
            if (onlyCurrent && !row.current) {
                continue;
            }
            ImVec4 color = highlight && row.current  ? UIWidgets::ColorValues.at(UIWidgets::Colors::Yellow)
                           : row.to == HIDDEN_TARGET ? UIWidgets::ColorValues.at(UIWidgets::Colors::Gray)
                                                     : ImVec4(1, 1, 1, 1);
            ImGui::PushID((int)row.fromEndpoint);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", row.from.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", row.tooltip.c_str());
            }
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", row.twoWay ? "<->" : "->");
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", row.to.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", row.tooltip.c_str());
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

} // namespace

void EntranceTrackerWindow::Draw() {
    if (!CVarGetInteger(CVAR_NAME_SHOW_ENTRANCE_TRACKER, 0)) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(520.0f, 480.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Entrance Tracker", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }
    if (!gPlayState || !IS_RANDO) {
        sLinesValid = false;
        CenteredText("No Rando Save Loaded");
        ImGui::End();
        return;
    }
    const auto& lines = Lines();
    if (lines.empty()) {
        CenteredText("No shuffled entrances in this save");
        ImGui::End();
        return;
    }

    UIWidgets::CVarCheckbox("Show All (Spoilers)", CVAR_NAME_SHOW_ALL);
    ImGui::SameLine();
    UIWidgets::CVarCheckbox("Group by Pool", CVAR_NAME_GROUP_BY_POOL);
    UIWidgets::CVarCheckbox("Highlight Current Scene", CVAR_NAME_HIGHLIGHT_CURRENT,
                            UIWidgets::CheckboxOptions().DefaultValue(true));
    ImGui::SameLine();
    UIWidgets::CVarCheckbox("Only Current Scene", CVAR_NAME_ONLY_CURRENT);
    UIWidgets::PushStyleInput();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##EntranceSearch", "Search", sSearch, sizeof(sSearch));
    UIWidgets::PopStyleInput();

    Options options;
    options.showAll = CVarGetInteger(CVAR_NAME_SHOW_ALL, 0);
    options.groupByPool = CVarGetInteger(CVAR_NAME_GROUP_BY_POOL, 0);
    options.search = sSearch;
    options.currentScene = gPlayState->sceneId;
    options.currentGrottoRow = EntranceShuffle::CurrentGrottoRow();
    options.sceneName = [](s16 scene) { return std::string(Ship_GetSceneName(scene)); };
    std::vector<Row> rows = BuildRows(lines, options);

    ImGui::BeginChild("Entrances");
    if (rows.empty()) {
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Gray), "No entrance matches");
    }
    DrawRows(rows);
    ImGui::EndChild();
    ImGui::End();
}

} // namespace EntranceTracker

} // namespace Rando
