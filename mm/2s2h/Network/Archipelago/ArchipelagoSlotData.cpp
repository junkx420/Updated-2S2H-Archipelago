#ifdef ENABLE_ARCHIPELAGO

#include "ArchipelagoSlotData.h"

#include <algorithm>
#include <sstream>

namespace ApSlot {

namespace {

// slot_data is producer-controlled; never let a missing or mistyped field
// take the client down. Toggles arrive as 0/1 integers.
class Reader {
  public:
    Reader(const nlohmann::json& data, std::vector<std::string>& warnings)
        : mData(data), mWarnings(warnings) {
    }

    int32_t Int(const char* key, int32_t fallback) {
        auto it = Find(key);
        if (it == mData.end()) {
            return fallback;
        }
        if (!it->is_number()) {
            mWarnings.push_back(std::string(key) + ": expected number");
            return fallback;
        }
        return it->get<int32_t>();
    }

    uint32_t UInt(const char* key, uint32_t fallback) {
        auto it = Find(key);
        if (it == mData.end()) {
            return fallback;
        }
        if (!it->is_number()) {
            mWarnings.push_back(std::string(key) + ": expected number");
            return fallback;
        }
        return it->get<uint32_t>();
    }

    bool Bool(const char* key, bool fallback) {
        auto it = Find(key);
        if (it == mData.end()) {
            return fallback;
        }
        if (it->is_boolean()) {
            return it->get<bool>();
        }
        if (it->is_number()) {
            return it->get<int32_t>() != 0;
        }
        mWarnings.push_back(std::string(key) + ": expected bool/number");
        return fallback;
    }

    std::vector<int32_t> IntList(const char* key) {
        std::vector<int32_t> out;
        auto it = Find(key);
        if (it == mData.end()) {
            return out;
        }
        if (!it->is_array()) {
            mWarnings.push_back(std::string(key) + ": expected array");
            return out;
        }
        for (const auto& v : *it) {
            if (v.is_number()) {
                out.push_back(v.get<int32_t>());
            }
        }
        return out;
    }

  private:
    nlohmann::json::const_iterator Find(const char* key) {
        auto it = mData.find(key);
        if (it == mData.end()) {
            mWarnings.push_back(std::string(key) + ": missing, using default");
        }
        return it;
    }

    const nlohmann::json& mData;
    std::vector<std::string>& mWarnings;
};

} // namespace

std::vector<std::string> SlotData::Parse(const nlohmann::json& data) {
    std::vector<std::string> warnings;
    Reader r(data, warnings);

    majoraRemainsRequired = r.Int("majora_remains_required", 4);
    moonRemainsRequired = r.Int("moon_remains_required", 4);
    requiredSkullTokens = r.Int("required_skull_tokens", 30);
    requiredStrayFairies = r.Int("required_stray_fairies", 15);
    remainsAllowBossWarps = r.Bool("remains_allow_boss_warps", true);

    skullsanity = (Skullsanity)r.Int("skullsanity", 0);
    fairysanity = r.Bool("fairysanity", false);
    shopsanity = (Shopsanity)r.Int("shopsanity", 0);
    scrubsanity = r.Bool("scrubsanity", false);
    cowsanity = r.Bool("cowsanity", false);
    keysanity = r.Bool("keysanity", false);
    bossKeysanity = r.Bool("bosskeysanity", false);
    introChecks = r.Bool("intro_checks", false);
    shuffleSpiderHouseReward = r.Bool("shuffle_spiderhouse_reward", false);
    shuffleGreatFairyRewards = r.Bool("shuffle_great_fairy_rewards", false);
    shuffleRegionalMaps = (ShuffleRegionalMaps)r.Int("shuffle_regional_maps", 1);

    // The apworld ships this key with a typo ("curiostity"); accept the
    // corrected spelling too in case upstream ever fixes it.
    if (data.contains("curiostity_shop_trades")) {
        curiosityShopTrades = r.Bool("curiostity_shop_trades", false);
    } else {
        curiosityShopTrades = r.Bool("curiosity_shop_trades", false);
    }

    shopPrices = r.IntList("shop_prices_ints");

    startingHeartLocations = r.Int("starting_heart_locations", 0);
    startWithConsumables = r.Bool("start_with_consumables", true);
    permanentChateauRomani = r.Bool("permanent_chateau_romani", true);
    startWithInvertedTime = r.Bool("start_with_inverted_time", false);
    receiveFilledWallets = r.Bool("receive_filled_wallets", true);

    damageMultiplier = (DamageMultiplier)r.Int("damage_multiplier", 1);
    deathBehavior = (DeathBehavior)r.Int("death_behavior", 0);
    deathLink = r.Bool("death_link", false);
    camc = r.Bool("camc", true);
    magicIsATrap = r.Bool("magic_is_a_trap", false);
    linkTunicColor = r.UInt("link_tunic_color", 0x1E691B);
    randomSeed = r.UInt("random_seed", 0);
    logicDifficulty = (LogicDifficulty)r.Int("logic_difficulty", 1);

    valid = true;
    return warnings;
}

std::string SlotData::Describe() const {
    auto onOff = [](bool b) { return b ? "on" : "off"; };

    std::ostringstream out;
    out << "Goal: " << moonRemainsRequired << " remains to reach the moon, " << majoraRemainsRequired
        << " to fight Majora" << (remainsAllowBossWarps ? ", remains grant boss warps" : "") << "\n";

    out << "Shuffles:";
    const char* skullNames[] = { "vanilla", "anything", "ignored" };
    out << " skulls=" << skullNames[std::min((int)skullsanity, 2)] << " (" << requiredSkullTokens << " tokens)";
    const char* shopNames[] = { "vanilla", "enabled", "advanced" };
    out << " shops=" << shopNames[std::min((int)shopsanity, 2)];
    out << " fairies=" << onOff(fairysanity) << " (" << requiredStrayFairies << " per temple)";
    out << " scrubs=" << onOff(scrubsanity) << " cows=" << onOff(cowsanity);
    out << " keys=" << onOff(keysanity) << " bosskeys=" << onOff(bossKeysanity);
    out << " greatfairies=" << onOff(shuffleGreatFairyRewards) << " spiderreward=" << onOff(shuffleSpiderHouseReward);
    out << " intro=" << onOff(introChecks) << " curiosity=" << onOff(curiosityShopTrades);
    const char* mapNames[] = { "vanilla", "starting", "anywhere" };
    out << " maps=" << mapNames[std::min((int)shuffleRegionalMaps, 2)] << "\n";

    const char* dmgNames[] = { "half", "normal", "double", "quad", "ohko" };
    const char* deathNames[] = { "vanilla", "fast", "instant", "moon crash" };
    out << "Behavior: damage=" << dmgNames[std::min((int)damageMultiplier, 4)]
        << " death=" << deathNames[std::min((int)deathBehavior, 3)] << " deathlink=" << onOff(deathLink)
        << " camc=" << onOff(camc) << " magictrap=" << onOff(magicIsATrap) << "\n";

    out << "Start: heartlocations=" << startingHeartLocations << " consumables=" << onOff(startWithConsumables)
        << " chateau=" << onOff(permanentChateauRomani) << " invertedtime=" << onOff(startWithInvertedTime)
        << " filledwallets=" << onOff(receiveFilledWallets) << "\n";

    out << "Logic: " << (logicDifficulty == LogicDifficulty::NoLogic ? "no logic" : "normal") << ", "
        << shopPrices.size() << " shop prices, seed " << randomSeed;
    return out.str();
}

} // namespace ApSlot

#endif // ENABLE_ARCHIPELAGO
