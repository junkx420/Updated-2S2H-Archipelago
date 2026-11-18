#ifndef RANDO_ENTRANCE_TRACKER_H
#define RANDO_ENTRANCE_TRACKER_H

#include "Rando/Rando.h"
#include <ship/window/gui/GuiWindow.h>

namespace Rando {

namespace EntranceTracker {

// Shuffled entrances of the current save ("from -> to"), from the save and EntranceShuffle.cpp alone, so it works
// without a server connection. Undiscovered targets stay hidden unless "Show All" is on.
class EntranceTrackerWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override {}
    void DrawElement() override {}
    void Draw() override;
    void UpdateElement() override {}
};

} // namespace EntranceTracker

} // namespace Rando

#endif // RANDO_ENTRANCE_TRACKER_H
