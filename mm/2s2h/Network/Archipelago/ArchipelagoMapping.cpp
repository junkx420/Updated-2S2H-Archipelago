#ifdef ENABLE_ARCHIPELAGO

#include "ArchipelagoMapping.h"
#include "Archipelago.h"

#include "2s2h/Rando/StaticData/StaticData.h"

namespace ApMapping {

RandoCheckId GetCheckFromApLocation(int64_t apLocationId) {
    int64_t value = apLocationId - AP_Client_consts::AP_ID_BASE;
    if (value <= RC_UNKNOWN || value >= RC_MAX) {
        return RC_UNKNOWN;
    }
    auto checkId = static_cast<RandoCheckId>(value);
    return Rando::StaticData::Checks.contains(checkId) ? checkId : RC_UNKNOWN;
}

int64_t GetApLocationFromCheck(RandoCheckId checkId) {
    if (checkId <= RC_UNKNOWN || checkId >= RC_MAX) {
        return -1;
    }
    return AP_Client_consts::AP_ID_BASE + checkId;
}

RandoItemId GetItemFromApItem(int64_t apItemId) {
    int64_t value = apItemId - AP_Client_consts::AP_ID_BASE;
    if (value <= RI_UNKNOWN || value >= RI_MAX) {
        return RI_UNKNOWN;
    }
    auto itemId = static_cast<RandoItemId>(value);
    return Rando::StaticData::Items.contains(itemId) ? itemId : RI_UNKNOWN;
}

} // namespace ApMapping

#endif // ENABLE_ARCHIPELAGO
