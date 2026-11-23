#ifndef RANDO_TRACKER_ABILITY_MACROS_H
#define RANDO_TRACKER_ABILITY_MACROS_H

#include <nlohmann/json.hpp>

#include "2s2h/Rando/Logic/Logic.h"
#include "2s2h/Rando/MiscBehavior/Abilities.h"

namespace Rando::Logic {

// Keep the original expressions, including the independent rain branch.
inline bool BaseCanRideEpona() {
    return CAN_RIDE_EPONA;
}
inline bool BaseCanGrowBeanPlant() {
    return CAN_GROW_BEAN_PLANT;
}
inline bool BaseCanUseDay2RainBean() {
    return CAN_USE_DAY2_RAIN_BEAN;
}

// Instantiating with the quest token preserves the original CAN_PLAY_SONG test.
template <int QuestSong, int OcarinaSong> bool BaseCanPlaySong() {
    return HAS_ITEM(ITEM_OCARINA_OF_TIME) && CHECK_QUEST_ITEM(QuestSong) && canPlaySong(OcarinaSong);
}

} // namespace Rando::Logic

#undef CAN_RIDE_EPONA
#define CAN_RIDE_EPONA (Rando::Abilities::TrackerAllowsMacro("CAN_RIDE_EPONA") && Rando::Logic::BaseCanRideEpona())
#undef CAN_GROW_BEAN_PLANT
#define CAN_GROW_BEAN_PLANT \
    (Rando::Abilities::TrackerAllowsMacro("CAN_GROW_BEAN_PLANT") && Rando::Logic::BaseCanGrowBeanPlant())
#undef CAN_USE_DAY2_RAIN_BEAN
#define CAN_USE_DAY2_RAIN_BEAN \
    (Rando::Abilities::TrackerAllowsMacro("CAN_USE_DAY2_RAIN_BEAN") && Rando::Logic::BaseCanUseDay2RainBean())
#undef CAN_PLAY_SONG
#define CAN_PLAY_SONG(song)                                               \
    (Rando::Abilities::TrackerAllowsBranch("CAN_PLAY_SONG(" #song ")") && \
     Rando::Logic::BaseCanPlaySong<QUEST_SONG_##song, QUEST_SONG_##song - QUEST_SONG_SONATA + OCARINA_SONG_SONATA>())

#endif // RANDO_TRACKER_ABILITY_MACROS_H
