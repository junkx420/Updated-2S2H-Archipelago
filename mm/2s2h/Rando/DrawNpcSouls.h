#ifndef RANDO_DRAW_NPC_SOULS_H
#define RANDO_DRAW_NPC_SOULS_H

#include "Rando/Rando.h"

namespace Rando {

// Draws an NPC soul: the NPC's model in a blue light (Archipelago fork). The light is always drawn; returns false if
// the model itself could not be drawn, which leaves the blue flame alone.
bool DrawNpcSoul(RandoItemId randoItemId);

// Draws Gorman's horse for RI_ABILITY_RIDE_EPONA; returns false if it could not be drawn.
bool DrawEponaRidingAbility();

} // namespace Rando

#endif // RANDO_DRAW_NPC_SOULS_H
