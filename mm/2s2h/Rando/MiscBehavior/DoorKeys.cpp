#include "DoorKeys.h"

extern "C" {
#include "variables.h"
#include "functions.h"
}

namespace Rando {

namespace DoorKeys {

// Buildings with checks or logic events behind a door actor (Logic/Regions: exits into an interior). Every listed
// entrance leads from outside in; the building's ways out are not listed. Doors with a schedule in z_en_door.h
// (EnDoorScheduleType) are known door actors; the others (playground, tourist information, smithy, lab, hut, ghost hut)
// are door actors by game knowledge only and are on the play-test list.
// clang-format off
const std::vector<DoorKey> doorKeys = {
    // key,                              building,                  entrances into the building
    { RI_DOOR_KEY_TRADING_POST,           "Trading Post",           { ENTRANCE(TRADING_POST, 0) } },
    { RI_DOOR_KEY_BOMB_SHOP,              "Bomb Shop",              { ENTRANCE(BOMB_SHOP, 0) } },
    { RI_DOOR_KEY_CURIOSITY_SHOP,         "Curiosity Shop",         { ENTRANCE(CURIOSITY_SHOP, 0), ENTRANCE(CURIOSITY_SHOP, 1) } },
    { RI_DOOR_KEY_SWORDSMANS_SCHOOL,      "Swordsman's School",     { ENTRANCE(SWORDMANS_SCHOOL, 0) } },
    { RI_DOOR_KEY_POST_OFFICE,            "Post Office",            { ENTRANCE(POST_OFFICE, 0) } },
    { RI_DOOR_KEY_LOTTERY_SHOP,           "Lottery Shop",           { ENTRANCE(LOTTERY_SHOP, 0) } },
    { RI_DOOR_KEY_HONEY_AND_DARLING,      "Honey & Darling",        { ENTRANCE(HONEY_AND_DARLINGS_SHOP, 0) } },
    { RI_DOOR_KEY_MILK_BAR,               "Milk Bar",               { ENTRANCE(MILK_BAR, 0) } },
    { RI_DOOR_KEY_MAYORS_RESIDENCE,       "Mayor's Residence",      { ENTRANCE(MAYORS_RESIDENCE, 0) } },
    { RI_DOOR_KEY_TOWN_SHOOTING_GALLERY,  "Town Shooting Gallery",  { ENTRANCE(TOWN_SHOOTING_GALLERY, 0) } },
    { RI_DOOR_KEY_TREASURE_CHEST_SHOP,    "Treasure Chest Shop",    { ENTRANCE(TREASURE_CHEST_SHOP, 0) } },
    { RI_DOOR_KEY_STOCK_POT_INN,          "Stock Pot Inn",          { ENTRANCE(STOCK_POT_INN, 0), ENTRANCE(STOCK_POT_INN, 1) } },
    { RI_DOOR_KEY_DEKU_PLAYGROUND,        "Deku Playground",        { ENTRANCE(DEKU_SCRUB_PLAYGROUND, 0) } },
    { RI_DOOR_KEY_TOURIST_INFORMATION,    "Tourist Information",    { ENTRANCE(TOURIST_INFORMATION, 0) } },
    { RI_DOOR_KEY_POTION_SHOP,            "Potion Shop",            { ENTRANCE(MAGIC_HAGS_POTION_SHOP, 0) } },
    { RI_DOOR_KEY_SWAMP_SHOOTING_GALLERY, "Swamp Shooting Gallery", { ENTRANCE(SWAMP_SHOOTING_GALLERY, 0) } },
    { RI_DOOR_KEY_MOUNTAIN_SMITHY,        "Mountain Smithy",        { ENTRANCE(MOUNTAIN_SMITHY, 0) } },
    { RI_DOOR_KEY_RANCH_BARN,             "Ranch Barn",             { ENTRANCE(RANCH_HOUSE, 0) } },
    { RI_DOOR_KEY_CUCCO_SHACK,            "Cucco Shack",            { ENTRANCE(CUCCO_SHACK, 0) } },
    { RI_DOOR_KEY_DOGGY_RACETRACK,        "Doggy Racetrack",        { ENTRANCE(DOGGY_RACETRACK, 0) } },
    { RI_DOOR_KEY_MARINE_LAB,             "Marine Lab",             { ENTRANCE(MARINE_RESEARCH_LAB, 0) } },
    { RI_DOOR_KEY_FISHERMANS_HUT,         "Fisherman's Hut",        { ENTRANCE(FISHERMANS_HUT, 0) } },
    { RI_DOOR_KEY_MUSIC_BOX_HOUSE,        "Music Box House",        { ENTRANCE(MUSIC_BOX_HOUSE, 0) } },
    { RI_DOOR_KEY_GHOST_HUT,              "Ghost Hut",              { ENTRANCE(GHOST_HUT, 0) } },
    { RI_DOOR_KEY_DAMPES_HOUSE,           "Dampe's House",          { ENTRANCE(DAMPES_HOUSE, 0) } },
};
// clang-format on

// ENTRANCE(scene, spawn) sets bits 4-15; bits 0-3 select a scene layer and do not change where the door leads.
static constexpr u16 ENTRANCE_WITHOUT_LAYER = 0xFFF0;

const DoorKey* FindByEntrance(u16 entrance) {
    for (const auto& doorKey : doorKeys) {
        for (u16 into : doorKey.into) {
            if ((into & ENTRANCE_WITHOUT_LAYER) == (entrance & ENTRANCE_WITHOUT_LAYER)) {
                return &doorKey;
            }
        }
    }
    return nullptr;
}

bool IsEntranceLocked(u16 entrance) {
    if (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_DOOR_KEYS]) {
        return false;
    }
    const DoorKey* doorKey = FindByEntrance(entrance);
    return doorKey != nullptr && !Flags_GetRandoInf(DOOR_KEY_RI_TO_RANDO_INF(doorKey->key));
}

} // namespace DoorKeys

} // namespace Rando
