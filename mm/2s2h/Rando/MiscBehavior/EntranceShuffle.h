#ifndef RANDO_ENTRANCE_SHUFFLE_H
#define RANDO_ENTRANCE_SHUFFLE_H

#include "Rando/Rando.h"
#include <vector>

namespace Rando {

namespace EntranceShuffle {

// One door the randomizer may shuffle. "into" is taken from outside (outsideScene) into the area behind the door,
// "outOf" leads from that area (insideScene) back out in front of the door. Shuffling is coupled: when door A leads
// into area B, B's way out leads back in front of A.
struct ShuffleEntrance {
    RandoOptionId option; // the option that shuffles this row (rows sharing an option are shuffled among each other)
    u16 into;
    u16 outOf;
    s16 insideScene;
    s16 outsideScene;
    s16 bossScene;                // boss room whose blue warp should return to this area's door, SCENE_MAX if none
    std::vector<u16> bossWarpsTo; // where that blue warp normally lands
};

// Append only: a save stores the shuffle per row index (RandoSaveInfo::entrancePairs). Read by the apworld generator.
extern const std::vector<ShuffleEntrance> shuffleEntrances;

// Writes the pairing from the apworld's slot_data ([door's "into", "into" of the area behind it]) into the save.
void SetPairs(const std::vector<std::pair<int32_t, int32_t>>& pairs);

// BeforeTransitionSetup hook: replaces play->nextEntrance when it goes through a shuffled door.
void OnBeforeTransitionSetup();

} // namespace EntranceShuffle

} // namespace Rando

#endif
