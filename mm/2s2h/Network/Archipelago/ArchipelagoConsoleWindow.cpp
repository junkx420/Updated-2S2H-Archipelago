#ifdef ENABLE_ARCHIPELAGO

#include "ArchipelagoConsoleWindow.h"
#include "Archipelago.h"

#include <libultraship/libultraship.h>
#include <imgui.h>

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

namespace {
constexpr size_t MAX_LOG_LINES = 512;

std::vector<std::string> logLines;
bool scrollToBottom = false;

char addressBuf[128] = {};
char slotNameBuf[64] = {};
char passwordBuf[64] = {};
bool buffersInitialized = false;

void InitBuffersFromCVars() {
    snprintf(addressBuf, sizeof(addressBuf), "%s", CVarGetString(CVAR_ARCHIPELAGO("ServerAddress"), "localhost:38281"));
    snprintf(slotNameBuf, sizeof(slotNameBuf), "%s", CVarGetString(CVAR_ARCHIPELAGO("SlotName"), ""));
    snprintf(passwordBuf, sizeof(passwordBuf), "%s", CVarGetString(CVAR_ARCHIPELAGO("Password"), ""));
    buffersInitialized = true;
}
} // namespace

void ArchipelagoConsole_SendMessage(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Split on newlines so multi-line messages render cleanly
    std::string message(buf);
    size_t start = 0;
    while (start <= message.size()) {
        size_t end = message.find('\n', start);
        if (end == std::string::npos) {
            logLines.push_back(message.substr(start));
            break;
        }
        logLines.push_back(message.substr(start, end - start));
        start = end + 1;
    }

    if (logLines.size() > MAX_LOG_LINES) {
        logLines.erase(logLines.begin(), logLines.begin() + (logLines.size() - MAX_LOG_LINES));
    }
    scrollToBottom = true;
}

void ArchipelagoConsoleWindow::DrawElement() {
    if (!buffersInitialized) {
        InitBuffersFromCVars();
    }

    auto& client = ArchipelagoClient::GetInstance();
    uint8_t status = client.GetConnectionStatus();
    bool busy = (status == AP_STATUS_CONNECTING) || client.IsConnected();

    // --- Connection form -----------------------------------------------------
    ImGui::BeginDisabled(busy);
    if (ImGui::InputText("Server Address", addressBuf, sizeof(addressBuf))) {
        CVarSetString(CVAR_ARCHIPELAGO("ServerAddress"), addressBuf);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    if (ImGui::InputText("Slot Name", slotNameBuf, sizeof(slotNameBuf))) {
        CVarSetString(CVAR_ARCHIPELAGO("SlotName"), slotNameBuf);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    if (ImGui::InputText("Password", passwordBuf, sizeof(passwordBuf), ImGuiInputTextFlags_Password)) {
        CVarSetString(CVAR_ARCHIPELAGO("Password"), passwordBuf);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    ImGui::EndDisabled();

    if (!busy) {
        if (ImGui::Button("Connect", ImVec2(120, 0))) {
            client.StartClient();
        }
    } else {
        if (ImGui::Button("Disconnect", ImVec2(120, 0))) {
            client.StopClient();
        }
    }

    ImGui::SameLine();
    switch (status) {
        case AP_STATUS_CONNECTING:
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Connecting...");
            break;
        case AP_STATUS_CONNECTION_ERROR:
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Connection error");
            break;
        case AP_STATUS_SLOT_CONNECTED:
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Connected");
            break;
        default:
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Disconnected");
            break;
    }

    ImGui::Separator();

    // --- Log -----------------------------------------------------------------
    if (ImGui::BeginChild("ApConsoleLog", ImVec2(0, 0), ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        for (const auto& line : logLines) {
            if (line.rfind("[ERROR]", 0) == 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", line.c_str());
            } else if (line.rfind("[LOG]", 0) == 0) {
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", line.c_str());
            } else {
                ImGui::TextUnformatted(line.c_str());
            }
        }
        if (scrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom = false;
        }
    }
    ImGui::EndChild();
}

#endif // ENABLE_ARCHIPELAGO
