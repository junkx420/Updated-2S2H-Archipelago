#ifdef ENABLE_ARCHIPELAGO

#ifdef _WIN32
// This TU is excluded from 2S2H's precompiled header (see
// CMake/Archipelago.cmake), so this prologue really is the first thing the
// compiler sees. asio demands WinSock2 before any WinSock 1 header: define
// lean-and-mean so Windows.h skips WinSock.h, then include winsock2 ourselves
// so _WINSOCK2API_ is set before anything else can interfere.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "Archipelago.h"

// apclientpp pulls in asio/websocketpp/openssl — keep these includes confined
// to this translation unit.
#include <apuuid.hpp>
#include <apclient.hpp>

#include <algorithm>

#include <libultraship/libultraship.h>
#include <spdlog/spdlog.h>

#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/Network/Archipelago/ArchipelagoConsoleWindow.h"

ArchipelagoClient::ArchipelagoClient() {
    uri = "";
    password = "";
}

ArchipelagoClient& ArchipelagoClient::GetInstance() {
    static ArchipelagoClient client;
    return client;
}

bool ArchipelagoClient::StartClient() {
    if (apClient != nullptr) {
        apClient.reset();
    }

    disconnecting = false;
    retries = 0;
    uri = CVarGetString(CVAR_ARCHIPELAGO("ServerAddress"), "localhost:38281");
    password = CVarGetString(CVAR_ARCHIPELAGO("Password"), "");

    // Persistent per-install UUID, used by the server to recognize this client
    uuid = ap_get_uuid(Ship::Context::GetPathRelativeToAppDirectory("ap-client-uuid"));

    // CA bundle for wss:// (installed by CMake/Archipelago.cmake)
    const std::string cert = Ship::Context::LocateFileAcrossAppDirs("networking/cacert.pem");
    SPDLOG_DEBUG("[AP] Location of cert: {}", cert);

    apClient = std::make_unique<APClient>(uuid, AP_Client_consts::AP_GAME_NAME, uri, cert);

    CVarSetInteger(CVAR_ARCHIPELAGO("ConnectionStatus"), AP_STATUS_CONNECTING);
    ArchipelagoConsole_SendMessage("[LOG] Connecting to %s ...", uri.c_str());

    apClient->set_socket_connected_handler(
        [&]() { ArchipelagoConsole_SendMessage("[LOG] Socket connected, waiting for room info..."); });

    apClient->set_socket_error_handler([&](const std::string& msg) {
        // With a scheme-less address apclientpp tries wss:// first and falls
        // back to ws://. The failed TLS attempt against a plaintext local
        // server is expected, not an error worth alarming the user about.
        if (msg.find("TLS handshake failed") != std::string::npos) {
            ArchipelagoConsole_SendMessage("[LOG] Secure connection unavailable, retrying without TLS...");
            return;
        }
        retries++;
        if (retries >= AP_Client_consts::MAX_RETRIES) {
            ArchipelagoConsole_SendMessage("[ERROR] Could not connect to server after several tries.\n"
                                           "Are the entered server address and port correct?");
            CVarSetInteger(CVAR_ARCHIPELAGO("ConnectionStatus"), AP_STATUS_CONNECTION_ERROR);
            disconnecting = true;
            return;
        }
        ArchipelagoConsole_SendMessage("[ERROR] %s", msg.c_str());
    });

    apClient->set_socket_disconnected_handler([&]() {
        if (!disconnecting) {
            ArchipelagoConsole_SendMessage("[LOG] Socket disconnected.");
        }
    });

    // Server greeted us — now authenticate against the slot.
    apClient->set_room_info_handler([&]() {
        std::list<std::string> tags;
        // DeathLink etc. get added here in later steps.
        apClient->ConnectSlot(CVarGetString(CVAR_ARCHIPELAGO("SlotName"), ""), password,
                              AP_Client_consts::AP_ITEMS_HANDLING, tags,
                              { AP_Client_consts::AP_PROTOCOL_MAJOR, AP_Client_consts::AP_PROTOCOL_MINOR,
                                AP_Client_consts::AP_PROTOCOL_BUILD });
    });

    apClient->set_slot_connected_handler([&](const nlohmann::json data) {
        CVarSetInteger(CVAR_ARCHIPELAGO("ConnectionStatus"), AP_STATUS_SLOT_CONNECTED);
        slotData = data;
        ArchipelagoConsole_SendMessage("[LOG] Connected to slot '%s' (team %d, slot %d).",
                                       CVarGetString(CVAR_ARCHIPELAGO("SlotName"), ""), apClient->get_team_number(),
                                       apClient->get_player_number());

        auto warnings = slotOptions.Parse(slotData);
        for (const auto& warning : warnings) {
            ArchipelagoConsole_SendMessage("[ERROR] slot_data %s", warning.c_str());
            SPDLOG_WARN("[AP] slot_data {}", warning);
        }
        ArchipelagoConsole_SendMessage("[LOG] Slot options:\n%s", slotOptions.Describe().c_str());

        // Tags can only be set after we know the slot options; opt into
        // DeathLink bounces when the seed asks for it.
        if (slotOptions.deathLink) {
            apClient->ConnectUpdate(false, 0, true, { "DeathLink" });
        }

        // Scout every still-missing location of our slot so the game can show
        // and place the real multiworld items (location_info handler below).
        scoutedItems.clear();
        scoutsReady = false;
        const auto missing = apClient->get_missing_locations();
        if (!missing.empty()) {
            apClient->LocationScouts(std::list<int64_t>(missing.begin(), missing.end()));
        } else {
            scoutsReady = true;
        }
    });

    // DeathLink deaths arrive as Bounced packets tagged "DeathLink".
    apClient->set_bounced_handler([&](const nlohmann::json& packet) {
        if (!slotOptions.deathLink || !packet.contains("tags")) {
            return;
        }
        const auto& tags = packet["tags"];
        if (std::find(tags.begin(), tags.end(), "DeathLink") == tags.end()) {
            return;
        }
        std::string source = packet.value("data", nlohmann::json::object()).value("source", "another world");
        // The server bounces our own packet back too — don't kill ourselves twice.
        if (source == CVarGetString(CVAR_ARCHIPELAGO("SlotName"), "")) {
            return;
        }
        pendingDeaths.push_back(source);
    });

    apClient->set_location_info_handler([&](const std::list<APClient::NetworkItem>& items) {
        if (apClient == nullptr) {
            return;
        }
        int ownSlot = apClient->get_player_number();
        for (const auto& item : items) {
            scoutedItems[item.location] = ScoutedItem{
                item.item,
                item.player,
                item.player == ownSlot,
                apClient->get_item_name(item.item, apClient->get_player_game(item.player)),
                apClient->get_player_alias(item.player),
            };
        }
        scoutsReady = true;
        ArchipelagoConsole_SendMessage("[LOG] Scouted %d locations from the server.", (int)scoutedItems.size());
    });

    apClient->set_slot_refused_handler([&](const std::list<std::string>& errors) {
        for (const auto& error : errors) {
            ArchipelagoConsole_SendMessage("[ERROR] Connection refused: %s", error.c_str());
        }
        CVarSetInteger(CVAR_ARCHIPELAGO("ConnectionStatus"), AP_STATUS_CONNECTION_ERROR);
        disconnecting = true;
    });

    // Server chat / events — render as plain text into our console for now.
    apClient->set_print_json_handler([&](const APClient::PrintJSONArgs& args) {
        if (apClient == nullptr) {
            return;
        }
        ArchipelagoConsole_SendMessage("%s", apClient->render_json(args.data, APClient::RenderFormat::TEXT).c_str());
    });

    // Queue items for ArchipelagoGameplay.cpp, which applies them on safe
    // gameplay frames and tracks the save's apItemsReceived index.
    apClient->set_items_received_handler([&](const std::list<APClient::NetworkItem>& items) {
        for (const auto& item : items) {
            std::string from = apClient->get_player_alias(item.player);
            pendingItems.push_back(ReceivedItem{ item.item, item.index, from });
            SPDLOG_INFO("[AP] Queued item {} (index {}) from {}", item.item, item.index, from);
        }
    });

    return true;
}

bool ArchipelagoClient::StopClient() {
    if (apClient == nullptr) {
        return false;
    }
    // Defer teardown to Poll() so we never destroy the client from inside one
    // of its own handlers.
    disconnecting = true;
    ArchipelagoConsole_SendMessage("[LOG] Disconnecting...");
    return true;
}

void ArchipelagoClient::Poll() {
    if (apClient == nullptr) {
        return;
    }

    if (disconnecting) {
        apClient->reset();
        apClient = nullptr;
        disconnecting = false;
        slotOptions = {}; // don't keep stale options past the session
        pendingItems.clear();
        scoutedItems.clear();
        scoutsReady = false;
        pendingDeaths.clear();
        CVarSetInteger(CVAR_ARCHIPELAGO("ConnectionStatus"), AP_STATUS_DISCONNECTED);
        return;
    }

    apClient->poll();
}

bool ArchipelagoClient::IsConnected() const {
    return apClient != nullptr && apClient->get_state() == APClient::State::SLOT_CONNECTED;
}

uint8_t ArchipelagoClient::GetConnectionStatus() const {
    return (uint8_t)CVarGetInteger(CVAR_ARCHIPELAGO("ConnectionStatus"), AP_STATUS_DISCONNECTED);
}

const nlohmann::json& ArchipelagoClient::GetSlotData() const {
    return slotData;
}

const ApSlot::SlotData& ArchipelagoClient::GetSlotOptions() const {
    return slotOptions;
}

std::vector<ArchipelagoClient::ReceivedItem>& ArchipelagoClient::GetPendingItems() {
    return pendingItems;
}

void ArchipelagoClient::SendLocationChecks(const std::list<int64_t>& locationIds) {
    if (apClient == nullptr || locationIds.empty()) {
        return;
    }
    apClient->LocationChecks(locationIds);
}

const std::unordered_map<int64_t, ArchipelagoClient::ScoutedItem>& ArchipelagoClient::GetScoutedItems() const {
    return scoutedItems;
}

bool ArchipelagoClient::ScoutsReady() const {
    return scoutsReady;
}

void ArchipelagoClient::SendGoalComplete() {
    if (apClient == nullptr) {
        return;
    }
    apClient->StatusUpdate(APClient::ClientStatus::GOAL);
    ArchipelagoConsole_SendMessage("[LOG] Goal complete! Reported to the server.");
}

void ArchipelagoClient::SendDeathLink(const std::string& cause) {
    if (apClient == nullptr || !slotOptions.deathLink) {
        return;
    }
    apClient->Bounce(
        {
            { "time", apClient->get_server_time() },
            { "source", CVarGetString(CVAR_ARCHIPELAGO("SlotName"), "") },
            { "cause", cause },
        },
        {}, {}, { "DeathLink" });
    ArchipelagoConsole_SendMessage("[LOG] DeathLink: your death was shared with the multiworld.");
}

std::vector<std::string>& ArchipelagoClient::GetPendingDeaths() {
    return pendingDeaths;
}

// ============================================================================
// Registration
// ============================================================================

void RegisterArchipelago() {
    // Construct the singleton up front
    ArchipelagoClient::GetInstance();

    // The websocket must be pumped continuously — OnGameStateUpdate fires for
    // every game state (title, file select, gameplay), see mm/src/code/game.c.
    COND_HOOK(OnGameStateUpdate, true, []() { ArchipelagoClient::GetInstance().Poll(); });

    // SetupGuiElements() runs before ShipInit::InitAll() (see BenPort.cpp), so
    // the Gui exists and we can self-register our window without touching
    // BenGui.cpp.
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    gui->AddGuiWindow(
        std::make_shared<ArchipelagoConsoleWindow>("gWindows.ApConsole", "Archipelago Console", ImVec2(520, 600)));

    // Reset stale status from a previous session
    CVarSetInteger(CVAR_ARCHIPELAGO("ConnectionStatus"), AP_STATUS_DISCONNECTED);
}

static RegisterShipInitFunc initFunc(RegisterArchipelago);

#endif // ENABLE_ARCHIPELAGO
