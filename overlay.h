#pragma once
/*
 * overlay.h - Transparent DX11 overlay window
 */

#include <Windows.h>
#include <d3d11.h>

namespace Overlay {
    /* Create the overlay window and init DX11 + ImGui */
    bool Init();

    /* Clean up all resources */
    void Shutdown();

    /* Begin a new frame (message pump + ImGui new frame) */
    bool BeginFrame();

    /* End frame (render + present) */
    void EndFrame();

    /* Get the overlay HWND */
    HWND GetWindow();

    /* Update overlay position to match Roblox window */
    void MatchWindow(HWND target);

    /* Set click-through mode */
    void SetClickThrough(bool clickThrough);

    /* Hide the overlay from screen capture (WS_EX_DISPLAYAFFINITY) */
    void SetStreamProof(bool enabled);

    /* Set the FPS target; 0 = uncapped (vsync-off Present) */
    void SetFpsTarget(int fps);
}
