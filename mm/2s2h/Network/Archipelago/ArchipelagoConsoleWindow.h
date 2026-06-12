#pragma once
#ifndef ARCHIPELAGO_CONSOLE_WINDOW_H
#define ARCHIPELAGO_CONSOLE_WINDOW_H
#ifdef ENABLE_ARCHIPELAGO

#include <ship/window/gui/GuiWindow.h>

class ArchipelagoConsoleWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~ArchipelagoConsoleWindow(){};

  protected:
    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};

// printf-style logger; safe to call from APClient handlers (they fire on the
// main thread, inside ArchipelagoClient::Poll()).
void ArchipelagoConsole_SendMessage(const char* fmt, ...);

#endif // ENABLE_ARCHIPELAGO
#endif // ARCHIPELAGO_CONSOLE_WINDOW_H
