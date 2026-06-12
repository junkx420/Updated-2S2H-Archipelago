#pragma once
#ifndef ARCHIPELAGO_SLOT_DATA_H
#define ARCHIPELAGO_SLOT_DATA_H
#ifdef __cplusplus
#ifdef ENABLE_ARCHIPELAGO

// Typed view of the mm_recomp apworld's slot_data (see fill_slot_data in the
// apworld's __init__.py). Enum values mirror the apworld's Options.py and
// must not be renumbered independently.

#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ApSlot {

enum class LogicDifficulty : int32_t {
    Normal = 1,
    NoLogic = 4,
};

enum class Skullsanity : int32_t {
    Vanilla = 0,
    Anything = 1, // tokens shuffled into the pool
    Ignore = 2,   // token locations contain nothing
};

enum class Shopsanity : int32_t {
    Vanilla = 0,
    Enabled = 1,
    Advanced = 2,
};

enum class ShopPrices : int32_t {
    Vanilla = 0,
    Free = 1,
    Cheap = 2,
    Expensive = 3,
    Offensive = 4,
};

enum class DamageMultiplier : int32_t {
    Half = 0,
    Normal = 1,
    Double = 2,
    Quad = 3,
    OHKO = 4,
};

enum class DeathBehavior : int32_t {
    Vanilla = 0,
    Fast = 1,
    Instant = 2,
    MoonCrash = 3,
};

enum class ShuffleRegionalMaps : int32_t {
    Vanilla = 0,
    Starting = 1,
    Anywhere = 2,
};

struct SlotData {
    // --- goal / progression gates
    int32_t majoraRemainsRequired = 4; // remains to fight Majora (0-4)
    int32_t moonRemainsRequired = 4;   // remains to enter the moon (0-4)
    int32_t requiredSkullTokens = 30;  // per spider house, if reward shuffled
    int32_t requiredStrayFairies = 15; // per temple, for great fairy rewards
    bool remainsAllowBossWarps = true;

    // --- shuffles
    Skullsanity skullsanity = Skullsanity::Vanilla;
    bool fairysanity = false;
    Shopsanity shopsanity = Shopsanity::Vanilla;
    bool scrubsanity = false;
    bool cowsanity = false;
    bool keysanity = false;
    bool bossKeysanity = false;
    bool introChecks = false;           // Link's Inventory / Clock Tower checks
    bool curiosityShopTrades = false;
    bool shuffleSpiderHouseReward = false;
    bool shuffleGreatFairyRewards = false;
    ShuffleRegionalMaps shuffleRegionalMaps = ShuffleRegionalMaps::Starting;

    // --- shop prices, indexed by the recomp shop slot id (Constants.py
    // SHOP_ID_*). Empty when shopsanity is vanilla.
    std::vector<int32_t> shopPrices;

    // --- starting state
    int32_t startingHeartLocations = 0; // how many "Heart Item #N" locations exist
    bool startWithConsumables = true;
    bool permanentChateauRomani = true;
    bool startWithInvertedTime = false;
    bool receiveFilledWallets = true;

    // --- behavior / QoL
    DamageMultiplier damageMultiplier = DamageMultiplier::Normal;
    DeathBehavior deathBehavior = DeathBehavior::Vanilla;
    bool deathLink = false;
    bool camc = true; // chest appearance matches contents
    bool magicIsATrap = false;
    uint32_t linkTunicColor = 0x1E691B; // packed RGB
    uint32_t randomSeed = 0;
    LogicDifficulty logicDifficulty = LogicDifficulty::Normal;

    bool valid = false; // set once Parse() has run on real slot_data

    // Parses the slot_data json. Missing/mistyped fields keep their defaults
    // (and are reported in the returned warning list).
    std::vector<std::string> Parse(const nlohmann::json& data);

    // One-line-per-group human readable summary for the AP console.
    std::string Describe() const;
};

} // namespace ApSlot

#endif // ENABLE_ARCHIPELAGO
#endif // __cplusplus
#endif // ARCHIPELAGO_SLOT_DATA_H
