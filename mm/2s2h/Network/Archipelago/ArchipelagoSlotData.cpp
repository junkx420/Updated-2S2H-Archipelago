#ifdef ENABLE_ARCHIPELAGO

#include "ArchipelagoSlotData.h"

#include <sstream>

namespace ApSlot {

std::vector<std::string> SlotData::Parse(const nlohmann::json& data) {
    std::vector<std::string> warnings;
    *this = SlotData{};
    if (!data.is_object()) {
        warnings.push_back("is not an object");
        return warnings;
    }
    try {
        version2s2h = data.value("version_2s2h", "");
        apworldVersion = data.value("apworld_version", "");
        if (data.contains("options") && data["options"].is_object()) {
            for (auto& [key, value] : data["options"].items()) {
                if (value.is_number_integer()) {
                    options[key] = value.get<int32_t>();
                } else {
                    warnings.push_back("option " + key + " is not an integer");
                }
            }
        } else {
            warnings.push_back("has no 'options' object");
        }
        if (data.contains("prices") && data["prices"].is_object()) {
            for (auto& [key, value] : data["prices"].items()) {
                prices[std::stoi(key)] = value.get<int32_t>();
            }
        }
        if (data.contains("starting_items")) {
            startingItems = data["starting_items"].get<std::vector<int32_t>>();
        }
        if (data.contains("vanilla_skulltulas")) {
            vanillaSkulltulas = data["vanilla_skulltulas"].get<std::vector<int32_t>>();
        }
        deathLink = data.value("death_link", false);
        randomSeed = data.value("seed", 0u);
    } catch (const std::exception& e) { warnings.push_back(std::string("parse error: ") + e.what()); }
    valid = true;
    return warnings;
}

std::string SlotData::Describe() const {
    std::ostringstream out;
    out << "  apworld " << (apworldVersion.empty() ? "?" : apworldVersion) << ", generated for 2S2H "
        << (version2s2h.empty() ? "?" : version2s2h) << "\n";
    out << "  " << options.size() << " randomizer options, " << prices.size() << " prices, " << startingItems.size()
        << " starting items, " << vanillaSkulltulas.size() << " vanilla skulltulas\n";
    out << "  DeathLink: " << (deathLink ? "on" : "off");
    return out.str();
}

} // namespace ApSlot

#endif // ENABLE_ARCHIPELAGO
