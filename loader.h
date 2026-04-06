#pragma once
/*
 * loader.h  -  x9 style ImGui loader
 *
 * States:
 *   Loader::State::Select   — game selection screen (shown on startup)
 *   Loader::State::Loading  — animated spinner while injecting
 *   Loader::State::Done     — loader finished, main menu takes over
 *
 * Usage (in your main loop):
 *
 *   if (!Loader::IsDone()) {
 *       Loader::Render();
 *   } else {
 *       Menu::Render();
 *       ESP::Render();
 *       Aimbot::Update();
 *   }
 */

namespace Loader {

    enum class State { Select, Loading, Done };

    void  Init();          // call once after Menu::ApplyTheme()
    void  Render();        // call every frame while !IsDone()
    bool  IsDone();        // true once loading animation finishes
    State GetState();
    void  SetAccent(float r, float g, float b);  // sync with menu theme

}
