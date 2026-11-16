#ifdef ENABLE_ARCHIPELAGO

// Gameplay glue between 2S2H's native Rando system and the Archipelago
// session (Step 3 of the integration):
//
//   OUT: any check marked `obtained` in RANDO_SAVE_CHECKS is reported to the
//        server via its AP location id. We rescan every gameplay frame but
//        only transmit the diff against what this session already sent; on
//        (re)connect the sent-set resets so the whole save is resynced (the
//        server deduplicates).
//
//   IN:  items queued by the items_received handler are applied one per
//        gameplay frame through the same Rando::GiveItem path the native
//        rando uses. The save's apItemsReceived counter (persisted via
//        BenJsonConversions) makes grants idempotent across reconnects and
//        save reloads: anything below the counter was already applied.
//
// Both directions only run while a rando save is loaded, the player actor is
// updating (same safety condition as Rando's own CheckQueue), and the slot is
// connected.

#include "Archipelago.h"
#include "ArchipelagoMapping.h"
#include "ArchipelagoConsoleWindow.h"

#include <unordered_set>
#include <algorithm>

#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/BenGui/Notification.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/StaticData/StaticData.h"

extern "C" {
#include "variables.h"
#include "functions.h"
}

namespace {

// AP location ids already reported this session (cleared on reconnect).
std::unordered_set<int64_t> sentLocations;
bool wasConnected = false;
// Scouted placements have been written into the current save's check table.
bool placementsApplied = false;

// Writes the native RO_* options from slot_data into RANDO_SAVE_OPTIONS (persisted
// with the save). The apworld sends 2S2H's own option ids, so this is 1:1.
void ApplySlotOptions(const ApSlot::SlotData& options) {
    for (const auto& [name, value] : options.options) {
        RandoOptionId optionId = Rando::StaticData::GetOptionIdFromName(name.c_str());
        if (optionId == RO_MAX) {
            ArchipelagoConsole_SendMessage("[ERROR] Unknown randomizer option '%s' in slot_data.", name.c_str());
            continue;
        }
        RANDO_SAVE_OPTIONS[optionId] = value;
    }
}

// Rewrites RANDO_SAVE_CHECKS so every AP location contains what the
// multiworld actually placed there: our own items become the real
// RandoItemId (native textbox/model/give all just work), other players'
// items become RI_ARCHIPELAGO_ITEM (give is a no-op; the server grants the
// owner when we report the location). A check is an AP location exactly when
// the apworld shuffled it, so `shuffled` here is the same flag 2S2H's
// GeneratePools would set; checks we leave alone keep their vanilla behaviour.
void ApplyScoutedPlacements(ArchipelagoClient& client) {
    if (placementsApplied || !client.ScoutsReady()) {
        return;
    }

    const auto& options = client.GetSlotOptions();
    ApplySlotOptions(options);

    int applied = 0, foreign = 0;
    for (const auto& [apLocationId, scout] : client.GetScoutedItems()) {
        RandoCheckId checkId = ApMapping::GetCheckFromApLocation(apLocationId);
        if (checkId == RC_UNKNOWN) {
            ArchipelagoConsole_SendMessage("[ERROR] AP location %lld is not a 2S2H check.", (long long)apLocationId);
            continue;
        }
        auto& saveCheck = RANDO_SAVE_CHECKS[checkId];
        if (saveCheck.obtained) {
            continue;
        }

        if (scout.isLocalItem) {
            RandoItemId randoItemId = ApMapping::GetItemFromApItem(scout.itemId);
            if (randoItemId == RI_UNKNOWN) {
                ArchipelagoConsole_SendMessage(
                    "[ERROR] Own item '%s' (AP id %lld) has no 2S2H equivalent; placing placeholder.",
                    scout.itemName.c_str(), (long long)scout.itemId);
                randoItemId = RI_ARCHIPELAGO_ITEM;
            }
            saveCheck.randoItemId = randoItemId;
        } else {
            saveCheck.randoItemId = RI_ARCHIPELAGO_ITEM;
            foreign++;
        }
        saveCheck.shuffled = true;
        applied++;
    }

    // Spider House tokens the apworld left vanilla (RO_SKULLTULA_SHUFFLED < 30): 2S2H marks
    // them shuffled with their own token so the token count still works.
    for (int32_t check : options.vanillaSkulltulas) {
        auto checkId = static_cast<RandoCheckId>(check);
        if (Rando::StaticData::Checks.contains(checkId)) {
            RANDO_SAVE_CHECKS[checkId].randoItemId = Rando::StaticData::Checks[checkId].randoItemId;
            RANDO_SAVE_CHECKS[checkId].shuffled = true;
        }
    }

    // Prices rolled by the apworld (the logic's CAN_AFFORD reads exactly these).
    for (const auto& [check, price] : options.prices) {
        auto checkId = static_cast<RandoCheckId>(check);
        if (Rando::StaticData::Checks.contains(checkId)) {
            RANDO_SAVE_CHECKS[checkId].price = (u16)price;
        }
    }

    // The Happy Mask Salesman checks are granted up front like native rando, but only
    // now that they hold their multiworld item (CheckQueue would otherwise give RI_UNKNOWN).
    for (RandoCheckId checkId : { RC_STARTING_ITEM_DEKU_MASK, RC_STARTING_ITEM_SONG_OF_HEALING }) {
        auto& saveCheck = RANDO_SAVE_CHECKS[checkId];
        if (saveCheck.shuffled && !saveCheck.obtained && !saveCheck.cycleObtained) {
            saveCheck.eligible = true;
        }
    }

    placementsApplied = true;
    ArchipelagoConsole_SendMessage("[LOG] Applied %d multiworld placements to this save (%d for other worlds).",
                                   applied, foreign);
}

void SendNewlyObtainedChecks(ArchipelagoClient& client) {
    // Every check type (grass, pots, crates, souls, ...) ends in CheckQueue setting
    // `obtained`, so this one scan covers all of them without per-type code.
    std::list<int64_t> batch;
    for (const auto& [apLocationId, _] : client.GetScoutedItems()) {
        RandoCheckId checkId = ApMapping::GetCheckFromApLocation(apLocationId);
        if (checkId == RC_UNKNOWN || !RANDO_SAVE_CHECKS[checkId].obtained || sentLocations.contains(apLocationId)) {
            continue;
        }
        batch.push_back(apLocationId);
        sentLocations.insert(apLocationId);

        // If this location held another player's item, tell the player what
        // they just sent out (the in-game textbox only showed a placeholder).
        const auto& scouts = client.GetScoutedItems();
        auto scout = scouts.find(apLocationId);
        if (scout != scouts.end() && !scout->second.isLocalItem) {
            Notification::Emit({
                .message = "Sent",
                .suffix = scout->second.itemName + " to " + scout->second.playerName,
            });
        }
    }
    if (!batch.empty()) {
        client.SendLocationChecks(batch);
        ArchipelagoConsole_SendMessage("[LOG] Reported %d location(s) to the server.", (int)batch.size());
    }
}

// Applies at most one pending item per frame so a reconnect burst trickles in
// visibly instead of dumping silently.
void ApplyNextPendingItem(ArchipelagoClient& client) {
    auto& pending = client.GetPendingItems();
    if (pending.empty()) {
        return;
    }

    // Items can arrive out of order across multiple packets; always apply the
    // lowest index first so apItemsReceived stays a simple high-water mark.
    auto next = std::min_element(pending.begin(), pending.end(),
                                 [](const auto& a, const auto& b) { return a.index < b.index; });

    // Already applied in a previous session (resync replay) — drop silently.
    if ((uint32_t)next->index < gSaveContext.save.shipSaveInfo.rando.apItemsReceived) {
        pending.erase(next);
        return;
    }

    RandoItemId randoItemId = ApMapping::GetItemFromApItem(next->itemId);
    if (randoItemId == RI_UNKNOWN) {
        ArchipelagoConsole_SendMessage("[ERROR] No 2S2H item mapped for AP item %lld (index %d) — skipped.",
                                       (long long)next->itemId, next->index);
    } else {
        RandoItemId converted = Rando::ConvertItem(randoItemId);
        Rando::GiveItem(converted);
        Notification::Emit({
            .itemIcon = Rando::StaticData::GetIconTexturePath(converted),
            .message = "Received",
            .suffix = Rando::StaticData::GetItemName(converted, false) + " from " + next->fromPlayer,
        });
        ArchipelagoConsole_SendMessage("[LOG] Received %s from %s.",
                                       Rando::StaticData::GetItemName(converted, false).c_str(),
                                       next->fromPlayer.c_str());
    }

    gSaveContext.save.shipSaveInfo.rando.apItemsReceived = (uint32_t)next->index + 1;
    pending.erase(next);
}

// DeathLink: watch health transitions. A death we caused (incoming link) must
// not be re-broadcast or two linked players would ping-pong forever.
void ProcessDeathLink(ArchipelagoClient& client) {
    static int16_t prevHealth = -1;
    static bool dyingFromDeathLink = false;

    if (!client.GetSlotOptions().deathLink) {
        return;
    }
    int16_t health = gSaveContext.save.saveInfo.playerData.health;

    auto& deaths = client.GetPendingDeaths();
    if (!deaths.empty() && health > 0) {
        Notification::Emit({ .message = "DeathLink", .suffix = "death received from " + deaths.front() });
        ArchipelagoConsole_SendMessage("[LOG] DeathLink: killed by %s.", deaths.front().c_str());
        deaths.erase(deaths.begin());
        gSaveContext.save.saveInfo.playerData.health = 0;
        dyingFromDeathLink = true;
        prevHealth = 0;
        return;
    }

    if (prevHealth > 0 && health <= 0) {
        if (dyingFromDeathLink) {
            dyingFromDeathLink = false;
        } else {
            client.SendDeathLink(std::string(CVarGetString(CVAR_ARCHIPELAGO("SlotName"), "Player")) +
                                 " met their fate in Termina");
        }
    }
    if (health > 0) {
        dyingFromDeathLink = false;
    }
    prevHealth = health;
}

void ProcessFrame() {
    if (!IS_RANDO) {
        return;
    }

    auto& client = ArchipelagoClient::GetInstance();
    if (!client.IsConnected()) {
        wasConnected = false;
        placementsApplied = false;
        return;
    }
    if (!wasConnected) {
        // Fresh (re)connection: resync every obtained check; the server
        // ignores locations it already knows about.
        sentLocations.clear();
        placementsApplied = false;
        wasConnected = true;
    }

    ApplyScoutedPlacements(client);
    if (!placementsApplied) {
        return; // don't hand out items before the check table is filled
    }
    SendNewlyObtainedChecks(client);
    ApplyNextPendingItem(client);
    ProcessDeathLink(client);
}

// Archipelago file creation: with a live AP connection, a newly created file
// becomes a rando save without running the native generator — the multiworld
// IS the seed; scouted placements fill the check table on the first gameplay
// frame. Mirrors the save-state setup of Rando::MiscBehavior::OnFileCreate
// minus pools/placement/spoiler. Native "Enable Rando" takes priority if on.
void OnApFileCreate(s16 fileNum) {
    auto& client = ArchipelagoClient::GetInstance();
    if (CVarGetInteger("gRando.Enabled", 0) || !client.IsConnected() || !client.GetSlotOptions().valid) {
        return;
    }

    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    memset(&gSaveContext.save.shipSaveInfo.rando, 0, sizeof(gSaveContext.save.shipSaveInfo.rando));
    memcpy(&gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys, &gSaveContext.save.saveInfo.inventory.dungeonKeys,
           sizeof(gSaveContext.save.saveInfo.inventory.dungeonKeys));

    // Same intro skip as the native rando: start as Human in South Clock Town.
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.save.hasTatl = true;
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    gSaveContext.save.isFirstCycle = true;
    SET_WEEKEVENTREG(WEEKEVENTREG_59_04);                                                  // Tatl
    SET_WEEKEVENTREG(WEEKEVENTREG_31_04);                                                  // Tatl
    gSaveContext.save.saveInfo.permanentSceneFlags[SCENE_INSIDETOWER].switch0 |= (1 << 0); // Happy Mask Salesman
    SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_NONE);
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_NONE;
    SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_NONE);

    // 2S2H defaults (no cvars — the multiworld is authoritative), then the
    // apworld's RO_* values on top.
    for (auto& [randoOptionId, randoStaticOption] : Rando::StaticData::Options) {
        RANDO_SAVE_OPTIONS[randoOptionId] = randoStaticOption.defaultValue;
    }
    const auto& options = client.GetSlotOptions();
    ApplySlotOptions(options);
    gSaveContext.save.shipSaveInfo.rando.finalSeed = options.randomSeed;

    // Like GeneratePools: every check starts out holding its vanilla item, unshuffled.
    for (auto& [randoCheckId, randoStaticCheck] : Rando::StaticData::Checks) {
        RANDO_SAVE_CHECKS[randoCheckId].randoItemId = randoStaticCheck.randoItemId;
    }

    // The non-item parts of Rando::GrantStartingItems. Starting *items* arrive from
    // the server as AP start inventory (items_handling bit 2), so they are not given here.
    if (RANDO_SAVE_OPTIONS[RO_STARTING_HEALTH] != 3) {
        gSaveContext.save.saveInfo.playerData.healthCapacity = gSaveContext.save.saveInfo.playerData.health =
            RANDO_SAVE_OPTIONS[RO_STARTING_HEALTH] * 0x10;
    }
    if (RANDO_SAVE_OPTIONS[RO_STARTING_CONSUMABLES]) {
        Rando::GiveItem(RI_DEKU_STICK);
        Rando::GiveItem(RI_DEKU_NUT);
        AMMO(ITEM_DEKU_STICK) = CUR_CAPACITY(UPG_DEKU_STICKS);
        AMMO(ITEM_DEKU_NUT) = CUR_CAPACITY(UPG_DEKU_NUTS);
    }

    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnRandoSeedGeneration>();
    Audio_PlaySfx(NA_SE_SY_ATTENTION_SOUND);
    ArchipelagoConsole_SendMessage("[LOG] Created an Archipelago save for slot '%s'.",
                                   CVarGetString(CVAR_ARCHIPELAGO("SlotName"), ""));
}

void RegisterArchipelagoGameplay() {
    // Same cadence + safety condition as Rando::MiscBehavior::CheckQueue:
    // once per frame while the player actor updates (i.e. actual gameplay,
    // never file select / title / scene transitions). Connection and save
    // state are checked at fire time, so one boot-time registration is fine.
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(
        ACTOR_PLAYER, [](Actor* actor) { ProcessFrame(); });

    // Placements are per-save: re-apply (and re-send) after any save load so
    // switching files mid-session can't leak the previous file's state.
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>([](s16 fileNum) {
        placementsApplied = false;
        sentLocations.clear();
    });

    // AP file-select flow: a file created while connected becomes an AP save.
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveInit>(OnApFileCreate);

    // Fires from Majora's defeat (z_boss_07.c) and from alternate completion
    // paths like triforce hunt (GiveItem.cpp).
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameCompletion>([]() {
        auto& client = ArchipelagoClient::GetInstance();
        if (IS_RANDO && client.IsConnected()) {
            client.SendGoalComplete();
        }
    });
}

} // namespace

// The 2S2H textbox font only covers printable ASCII; other games' names can be anything.
static std::string TextboxSafe(const std::string& in, size_t maxLen) {
    std::string out;
    for (unsigned char c : in) {
        if (c >= 0x20 && c < 0x7F) {
            out += static_cast<char>(c);
        } else if (c < 0x80 || c >= 0xC0) { // control char or start of a UTF-8 sequence; continuation bytes dropped
            out += '?';
        }
    }
    if (out.size() > maxLen) {
        out.resize(maxLen - 3);
        out += "...";
    }
    return out;
}

std::string ApMapping::GetForeignItemLabel(RandoCheckId checkId) {
    int64_t apLocationId = ApMapping::GetApLocationFromCheck(checkId);
    if (apLocationId < 0) {
        return "";
    }
    const auto& scouted = ArchipelagoClient::GetInstance().GetScoutedItems();
    auto it = scouted.find(apLocationId);
    if (it == scouted.end() || it->second.isLocalItem || it->second.itemName.empty()) {
        return "";
    }
    return TextboxSafe(it->second.playerName, 16) + "'s " + TextboxSafe(it->second.itemName, 40);
}

void ApMapping::HintCheck(RandoCheckId checkId) {
    int64_t apLocationId = ApMapping::GetApLocationFromCheck(checkId);
    if (apLocationId < 0 || RANDO_SAVE_CHECKS[checkId].obtained) {
        return;
    }
    ArchipelagoClient::GetInstance().HintLocation(apLocationId);
}

static RegisterShipInitFunc initGameplayFunc(RegisterArchipelagoGameplay);

#endif // ENABLE_ARCHIPELAGO
