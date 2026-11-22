#pragma once
#ifndef ARCHIPELAGO_SLOT_DATA_H
#define ARCHIPELAGO_SLOT_DATA_H
#ifdef __cplusplus
#ifdef ENABLE_ARCHIPELAGO

// slot_data of the mm_2s2h apworld (worlds/mm_2s2h/__init__.py fill_slot_data).
// The apworld mirrors 2S2H's own randomizer, so options arrive as native RO_*
// names and are written straight into RANDO_SAVE_OPTIONS.

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace ApSlot {

struct SlotData {
    std::string version2s2h;                                // 2S2H version the apworld was generated from
    std::string apworldVersion;                             // mm_2s2h world_version, e.g. "0.1.0"
    std::unordered_map<std::string, int32_t> options;       // "RO_*" -> value
    std::unordered_map<int32_t, int32_t> prices;            // RandoCheckId -> price
    std::vector<int32_t> startingItems;                     // RandoItemId (also sent as AP start inventory)
    std::vector<int32_t> vanillaSkulltulas;                 // RandoCheckId holding their vanilla token
    std::vector<std::pair<int32_t, int32_t>> entrancePairs; // shuffled doors: [door's "into", "into" behind it]
    std::vector<std::pair<int32_t, int32_t>> mixedEntrancePairs; // optional coupled endpoint IDs
    std::vector<int32_t> owlWarpTargets;                         // one destination ID per owl in game order
    int32_t spawnTarget = -1;
    std::vector<std::pair<int32_t, int32_t>> directedEntranceTargets;
    bool deathLink = false;
    uint32_t randomSeed = 0;

    bool valid = false; // set once Parse() has run on real slot_data

    // Missing/mistyped fields keep their defaults and are reported as warnings.
    std::vector<std::string> Parse(const nlohmann::json& data);

    // Human readable summary for the AP console.
    std::string Describe() const;
};

} // namespace ApSlot

#endif // ENABLE_ARCHIPELAGO
#endif // __cplusplus
#endif // ARCHIPELAGO_SLOT_DATA_H
