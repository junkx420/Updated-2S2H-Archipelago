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

// 2S2H check -> mm_recomp shop slot id (Constants.py SHOP_ID_*), the index
// into slot_data's shop_prices_ints. Numbering verified against the vanilla
// stock on both sides.
const std::unordered_map<RandoCheckId, size_t> SHOP_PRICE_INDEX = {
    { RC_TRADING_POST_SHOP_ITEM_01, 0x0A }, { RC_TRADING_POST_SHOP_ITEM_02, 0x05 },
    { RC_TRADING_POST_SHOP_ITEM_03, 0x06 }, { RC_TRADING_POST_SHOP_ITEM_04, 0x03 },
    { RC_TRADING_POST_SHOP_ITEM_05, 0x07 }, { RC_TRADING_POST_SHOP_ITEM_06, 0x08 },
    { RC_TRADING_POST_SHOP_ITEM_07, 0x09 }, { RC_TRADING_POST_SHOP_ITEM_08, 0x04 },
    { RC_BOMB_SHOP_ITEM_01, 0x1A },         { RC_BOMB_SHOP_ITEM_02, 0x19 },
    { RC_BOMB_SHOP_ITEM_03, 0x17 },         { RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM, 0x15 },
    { RC_CURIOSITY_SHOP_SPECIAL_ITEM, 0x13 },
    { RC_HAGS_POTION_SHOP_ITEM_01, 0x02 },  { RC_HAGS_POTION_SHOP_ITEM_02, 0x01 },
    { RC_HAGS_POTION_SHOP_ITEM_03, 0x00 },
    { RC_ZORA_SHOP_ITEM_01, 0x1B },         { RC_ZORA_SHOP_ITEM_02, 0x1C },
    { RC_ZORA_SHOP_ITEM_03, 0x1D },
    { RC_GORON_SHOP_ITEM_01, 0x1E },        { RC_GORON_SHOP_ITEM_02, 0x1F },
    { RC_GORON_SHOP_ITEM_03, 0x20 },
};

// AP shuffles each Tingle map as ONE location, but 2S2H has TWO vendor slots
// per map. The alternate slot mirrors the primary (same item/price), and
// collecting either reports the same AP location.
const std::pair<RandoCheckId, RandoCheckId> TINGLE_VENDOR_ALIASES[] = {
    // { primary (mapped to AP), alternate (same map, other vendor) }
    { RC_CLOCK_TOWN_NORTH_TINGLE_MAP_01, RC_IKANA_CANYON_TINGLE_MAP_02 },         // Clock Town
    { RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_01, RC_CLOCK_TOWN_NORTH_TINGLE_MAP_02 },  // Woodfall
    { RC_TWIN_ISLANDS_TINGLE_MAP_01, RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_02 },      // Snowhead
    { RC_MILK_ROAD_TINGLE_MAP_01, RC_TWIN_ISLANDS_TINGLE_MAP_02 },                   // Romani Ranch
    { RC_GREAT_BAY_COAST_TINGLE_MAP_01, RC_MILK_ROAD_TINGLE_MAP_02 },                // Great Bay
    { RC_IKANA_CANYON_TINGLE_MAP_01, RC_GREAT_BAY_COAST_TINGLE_MAP_02 },             // Stone Tower
};

// Keep aliased Tingle slots in lockstep: buying from either vendor obtains
// both (so the map can't be bought twice and either purchase reports the
// shared AP location through the primary's mapping).
void SyncTingleAliases() {
    for (const auto& [primary, alternate] : TINGLE_VENDOR_ALIASES) {
        auto& p = RANDO_SAVE_CHECKS[primary];
        auto& a = RANDO_SAVE_CHECKS[alternate];
        if (p.obtained != a.obtained) {
            p.obtained = a.obtained = true;
            p.cycleObtained = a.cycleObtained = true;
            p.eligible = a.eligible = false;
        }
    }
}

// Translates slot_data options into the native RANDO_SAVE_OPTIONS the logic
// and actor behaviors already read (persisted with the save).
void ApplySlotOptions(const ApSlot::SlotData& options) {
    RANDO_SAVE_OPTIONS[RO_ACCESS_MOON_REMAINS_COUNT] = options.moonRemainsRequired;
    RANDO_SAVE_OPTIONS[RO_ACCESS_MAJORA_REMAINS_COUNT] = options.majoraRemainsRequired;
    if (options.shopsanity != ApSlot::Shopsanity::Vanilla) {
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_SHOPS] = 1;
    }
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_GOLD_SKULLTULAS] = (options.skullsanity == ApSlot::Skullsanity::Anything) ? 1 : 0;
    RANDO_SAVE_OPTIONS[RO_SKULLTULA_TOKENS_REQUIRED] = options.requiredSkullTokens;
    RANDO_SAVE_OPTIONS[RO_STRAY_FAIRIES_REQUIRED] = options.requiredStrayFairies;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_COWS] = options.cowsanity ? 1 : 0;
    if (options.shuffleRegionalMaps == ApSlot::ShuffleRegionalMaps::Anywhere) {
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_TINGLE_SHOPS] = 1;
    }
}

// Rewrites RANDO_SAVE_CHECKS so every mapped check contains what the
// multiworld actually placed there: our own items become the real
// RandoItemId (native textbox/model/give all just work), other players'
// items become RI_ARCHIPELAGO_ITEM (give is a no-op; the server grants the
// owner when we report the location).
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
            continue; // AP location with no 2S2H check (see mapping report)
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

        // Shopsanity: stamp the multiworld price onto the shop slot.
        auto priceIndex = SHOP_PRICE_INDEX.find(checkId);
        if (priceIndex != SHOP_PRICE_INDEX.end() && priceIndex->second < options.shopPrices.size()) {
            saveCheck.price = (u16)options.shopPrices[priceIndex->second];
        }
    }

    // Tingle prices: AP doesn't price map purchases (they're not part of
    // shop_prices), and the AP file flow never runs the native generator that
    // would have rolled prices — so give the slots their vanilla prices
    // (local map cheap, far map pricier).
    static const std::pair<RandoCheckId, u16> TINGLE_VANILLA_PRICES[] = {
        { RC_CLOCK_TOWN_NORTH_TINGLE_MAP_01, 5 },          { RC_CLOCK_TOWN_NORTH_TINGLE_MAP_02, 40 },
        { RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_01, 20 },   { RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_02, 40 },
        { RC_TWIN_ISLANDS_TINGLE_MAP_01, 20 },             { RC_TWIN_ISLANDS_TINGLE_MAP_02, 40 },
        { RC_MILK_ROAD_TINGLE_MAP_01, 20 },                { RC_MILK_ROAD_TINGLE_MAP_02, 40 },
        { RC_GREAT_BAY_COAST_TINGLE_MAP_01, 20 },          { RC_GREAT_BAY_COAST_TINGLE_MAP_02, 40 },
        { RC_IKANA_CANYON_TINGLE_MAP_01, 20 },             { RC_IKANA_CANYON_TINGLE_MAP_02, 40 },
    };
    for (const auto& [checkId, price] : TINGLE_VANILLA_PRICES) {
        RANDO_SAVE_CHECKS[checkId].price = price;
    }

    // Mirror primary Tingle slots onto their alternate vendors.
    for (const auto& [primary, alternate] : TINGLE_VENDOR_ALIASES) {
        auto& p = RANDO_SAVE_CHECKS[primary];
        auto& a = RANDO_SAVE_CHECKS[alternate];
        if (p.shuffled) {
            a.randoItemId = p.randoItemId;
            a.shuffled = true;
        }
    }

    placementsApplied = true;
    ArchipelagoConsole_SendMessage("[LOG] Applied %d multiworld placements to this save (%d for other worlds).",
                                   applied, foreign);
}

void SendNewlyObtainedChecks(ArchipelagoClient& client) {
    std::list<int64_t> batch;
    for (const auto& [checkId, apLocationId] : ApMapping::CHECK_TO_AP_LOCATION) {
        if (!RANDO_SAVE_CHECKS[checkId].obtained || sentLocations.contains(apLocationId)) {
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
    SyncTingleAliases();
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

    // Sane logic defaults (no cvars — the multiworld is authoritative), then
    // the AP-controlled values on top.
    for (auto& [randoOptionId, randoStaticOption] : Rando::StaticData::Options) {
        RANDO_SAVE_OPTIONS[randoOptionId] = randoStaticOption.defaultValue;
    }
    ApplySlotOptions(client.GetSlotOptions());
    gSaveContext.save.shipSaveInfo.rando.finalSeed = client.GetSlotOptions().randomSeed;

    // The Happy Mask Salesman checks are granted up front, like native rando.
    RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].eligible = true;
    RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].eligible = true;

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

static RegisterShipInitFunc initGameplayFunc(RegisterArchipelagoGameplay);

#endif // ENABLE_ARCHIPELAGO
