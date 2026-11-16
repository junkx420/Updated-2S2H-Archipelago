#pragma once
#ifndef ARCHIPELAGO_MAPPING_H
#define ARCHIPELAGO_MAPPING_H
#ifdef __cplusplus
#ifdef ENABLE_ARCHIPELAGO

// ID translation between the mm_2s2h apworld and 2S2H's native Rando enums.
// The apworld is generated from these very enums, so the mapping is a fixed
// offset: AP id = AP_ID_BASE + enum value (see Archipelago.h).

#include <cstdint>
#include <string>

#include "2s2h/Rando/Types.h"

namespace ApMapping {

// Lookup helpers returning RC_UNKNOWN / RI_UNKNOWN / -1 on a miss.
RandoCheckId GetCheckFromApLocation(int64_t apLocationId);
int64_t GetApLocationFromCheck(RandoCheckId checkId);
RandoItemId GetItemFromApItem(int64_t apItemId);

// Display name of another world's item placed at one of our checks, e.g. "Alice's Hookshot", from this session's
// LocationScouts (printable ASCII only, long names shortened). Empty when not connected, not scouted or our own item.
std::string GetForeignItemLabel(RandoCheckId checkId);

// An in-game hint just named the item at this check and its exact location: announce it as an Archipelago hint
// too (once per session, only for still-missing locations of this slot, no-op when not connected).
void HintCheck(RandoCheckId checkId);

} // namespace ApMapping

#endif // ENABLE_ARCHIPELAGO
#endif // __cplusplus
#endif // ARCHIPELAGO_MAPPING_H
