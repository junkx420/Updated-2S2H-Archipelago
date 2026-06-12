#ifdef ENABLE_ARCHIPELAGO

// Registers the Archipelago section in BenMenu's "Dev Tools" tab via the
// official MenuInit extension point (BenMenu::InitElement runs all functions
// registered through RegisterMenuInitFunc after building its own sections —
// same pattern as AudioEditor.cpp / ResolutionEditor.cpp).
//
// The WIDGET_WINDOW_BUTTON widget resolves the window by name at DRAW time
// (Menu.cpp), so it doesn't matter that the window itself is registered later
// (ShipInit::InitAll runs after SetupGuiElements). Bonus: while the window is
// not popped out, the menu embeds the window's contents inline in this
// section, so the console is usable directly inside the ESC menu.

#include "2s2h/BenGui/BenMenu.h"
#include "2s2h/BenGui/BenGui.hpp"
#include "2s2h/BenGui/UIWidgets.hpp"
#include "2s2h/Network/Archipelago/Archipelago.h"

namespace BenGui {
extern std::shared_ptr<BenMenu> mBenMenu;
}

static void RegisterArchipelagoMenuWidgets() {
    // New sidebar entry under the existing "Dev Tools" tab, single column.
    // (Once this matures, "Network" could become its own top-level section
    // via AddMenuEntry, mirroring SoH's Network tab.)
    BenGui::mBenMenu->AddSidebarEntry("Dev Tools", "Archipelago", 1);
    WidgetPath path = { "Dev Tools", "Archipelago", SECTION_COLUMN_1 };

    BenGui::mBenMenu->AddWidget(path, "Archipelago Multiworld", WIDGET_SEPARATOR_TEXT);

    BenGui::mBenMenu->AddWidget(path, "Popout Archipelago Console", WIDGET_WINDOW_BUTTON)
        .CVar("gWindows.ApConsole")
        .WindowName("Archipelago Console") // must match the name passed to AddGuiWindow in Archipelago.cpp
        .Options(UIWidgets::ButtonOptions()
                     .Tooltip("Connect to an Archipelago multiworld server and view the session log.")
                     .Size(UIWidgets::Sizes::Inline));
}

static RegisterMenuInitFunc initArchipelagoMenu(RegisterArchipelagoMenuWidgets);

#endif // ENABLE_ARCHIPELAGO
