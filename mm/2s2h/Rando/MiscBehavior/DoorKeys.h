#ifndef RANDO_DOOR_KEYS_H
#define RANDO_DOOR_KEYS_H

#include "Rando/Rando.h"
#include <vector>

#define RI_DOOR_KEY_FIRST RI_DOOR_KEY_TRADING_POST
#define RI_DOOR_KEY_LAST RI_DOOR_KEY_ZORA_HALL_ROOMS
#define IS_DOOR_KEY(randoItemId) ((randoItemId) >= RI_DOOR_KEY_FIRST && (randoItemId) <= RI_DOOR_KEY_LAST)
#define DOOR_KEY_RI_TO_RANDO_INF(randoItemId) \
    (((randoItemId) - RI_DOOR_KEY_FIRST) + RANDO_INF_OBTAINED_DOOR_KEY_TRADING_POST)

namespace Rando {

namespace DoorKeys {

// One building in Termina that Shuffle Door Keys locks. "into" lists every entrance from outside into the building
// (the exits of the logic graph, Logic/Regions); one key opens all of them. The way out is never listed and so never
// locked. Read by the apworld generator: every "into" must be the exit of exactly one region.
struct DoorKey {
    RandoItemId key;
    const char* building; // item name is "<building> Key" (StaticData/Items.cpp)
    std::vector<u16> into;
};

// Append only: RI_DOOR_KEY_* and RANDO_INF_OBTAINED_DOOR_KEY_* follow this order.
extern const std::vector<DoorKey> doorKeys;

// The building whose door leads to this entrance, or nullptr. Spawn and scene decide, the scene layer does not.
const DoorKey* FindByEntrance(u16 entrance);

// True when Shuffle Door Keys is on and `entrance` is the "into" entrance of a building whose key is missing.
// Depends on nothing but the entrance a door leads to: not on the scene, the side or the kind of door actor.
bool IsEntranceLocked(u16 entrance);

} // namespace DoorKeys

} // namespace Rando

#endif
