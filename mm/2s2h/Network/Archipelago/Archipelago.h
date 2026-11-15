#pragma once
#ifndef ARCHIPELAGO_H
#define ARCHIPELAGO_H
#ifdef __cplusplus
#ifdef ENABLE_ARCHIPELAGO

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

#include "2s2h/Network/Archipelago/ArchipelagoSlotData.h"

// Forward declaration — apclient.hpp is heavy (asio/websocketpp/openssl) and
// is only included in Archipelago.cpp.
class APClient;

// CVar namespace for all Archipelago settings (mirrors SoH's
// CVAR_REMOTE_ARCHIPELAGO, but 2S2H uses plain string CVars).
#define CVAR_ARCHIPELAGO(var) "gNetwork.Archipelago." var

namespace AP_Client_consts {
// NOTE: This MUST match the `game` attribute of the mm_2s2h apworld.
static constexpr char const* AP_GAME_NAME = "Majora's Mask (2 Ship 2 Harkinian)";

// mm_2s2h ids: AP location id = AP_ID_BASE + RandoCheckId, AP item id = AP_ID_BASE + RandoItemId
// (worlds/mm_2s2h/__init__.py BASE_ID). No translation tables.
static constexpr int64_t AP_ID_BASE = 0x2D2D0000;

// Only seeds from an apworld with this major.minor version are accepted (archipelago.json world_version).
static constexpr char const* AP_WORLD_VERSION = "0.1";

// Version handshake with the AP server (matches SoH: server protocol 0.6.x).
static constexpr int AP_PROTOCOL_MAJOR = 0;
static constexpr int AP_PROTOCOL_MINOR = 6;
static constexpr int AP_PROTOCOL_BUILD = 3;

// items_handling bitmask sent in ConnectSlot:
//   bit 0 (1): receive items from other worlds
//   bit 1 (2): receive own items ("remote items")
//   bit 2 (4): receive starting inventory
// We run the LOCAL-GRANT model: scouted placements are written into
// RANDO_SAVE_CHECKS, so the native give path grants our own items the moment
// a check is collected. The server must therefore NOT echo our own items
// back (bit 1 off) or everything local would be granted twice.
static constexpr int AP_ITEMS_HANDLING = 0b101;

static constexpr int MAX_RETRIES = 3;
} // namespace AP_Client_consts

// Connection status values stored in CVAR_ARCHIPELAGO("ConnectionStatus")
enum ApConnectionStatus {
    AP_STATUS_DISCONNECTED = 0,
    AP_STATUS_CONNECTING = 1,
    AP_STATUS_CONNECTION_ERROR = 2,
    AP_STATUS_SLOT_CONNECTED = 3,
};

class ArchipelagoClient {
  public:
    static ArchipelagoClient& GetInstance();

    bool StartClient();
    bool StopClient();

    // Pumps the websocket + dispatches handlers. Must be called regularly;
    // registered on GameInteractor::OnGameStateUpdate.
    void Poll();

    bool IsConnected() const;
    uint8_t GetConnectionStatus() const;
    const nlohmann::json& GetSlotData() const;

    // Typed slot options; check .valid (true once a slot has connected).
    const ApSlot::SlotData& GetSlotOptions() const;

    // An item granted to us by the server, not yet applied to the save.
    // index is the absolute position in this slot's received-item sequence
    // and drives the save's apItemsReceived resync logic.
    struct ReceivedItem {
        int64_t itemId;
        int32_t index;
        std::string fromPlayer;
    };

    // Pending items queued by the items_received handler; consumed on safe
    // gameplay frames by ArchipelagoGameplay.cpp.
    std::vector<ReceivedItem>& GetPendingItems();

    // Reports these AP location ids as checked to the server.
    void SendLocationChecks(const std::list<int64_t>& locationIds);

    // What the multiworld placed at one of OUR locations, learned via
    // LocationScouts right after connecting. Names are resolved against the
    // server datapackage at receive time so the UI never needs apclient.
    struct ScoutedItem {
        int64_t itemId;
        int player;
        bool isLocalItem; // true when the item belongs to this slot
        std::string itemName;
        std::string playerName;
    };

    // AP location id -> scouted placement. Empty until ScoutsReady().
    const std::unordered_map<int64_t, ScoutedItem>& GetScoutedItems() const;
    bool ScoutsReady() const;

    // Tells the server this slot reached its goal (Majora defeated).
    void SendGoalComplete();

    // DeathLink: broadcasts our death to the multiworld.
    void SendDeathLink(const std::string& cause);
    // Deaths received from other DeathLink players ("source" names), consumed
    // by the gameplay glue which kills the player.
    std::vector<std::string>& GetPendingDeaths();

    std::unique_ptr<APClient> apClient;

  protected:
    ArchipelagoClient();

  private:
    ArchipelagoClient(ArchipelagoClient&) = delete;
    void operator=(const ArchipelagoClient&) = delete;

    std::string uri;
    std::string password;
    std::string uuid;
    nlohmann::json slotData;
    ApSlot::SlotData slotOptions;
    std::vector<ReceivedItem> pendingItems;
    std::unordered_map<int64_t, ScoutedItem> scoutedItems;
    bool scoutsReady = false;
    std::vector<std::string> pendingDeaths;
    bool disconnecting = false;
    int retries = 0;
};

#endif // ENABLE_ARCHIPELAGO
#endif // __cplusplus
#endif // ARCHIPELAGO_H
