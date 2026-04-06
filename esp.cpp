/*
 * esp.cpp  —  x9  ESP Theme Dispatcher
 */

#include "esp.h"
#include "roblox.h"
#include "aimbot.h"
#include "imgui.h"
#include "overlay.h"
#include "settings.h"
#include <cmath>
#include <cstdio>
#include <unordered_map>

/* ── Define shared state (exactly once, in this TU) ─────────── */
#define X9_SHARED_IMPL
#include "x9_shared.h"

/* ── Include overlay effects ─────────────────────────────────── */
#include "esp_overlay_effects.h"

/* ── Include both ESP themes ──────────────────────────────────── */
#include "esp_theme_normal.h"

/* ── Public API ───────────────────────────────────────────────── */

ESPSettings& ESP::Settings() {
    return g_cfg.esp;
}

void ESP::Render() {
    /* ── Overlay effects (darken/snow/vignette) ─────────────── */
    bool menuOpen = Menu::IsVisible() || !Loader::IsDone();
    X9Effects::DrawEffects(menuOpen);

    if (g_cfg.uiTheme == 1)
        EspNormal::Render();

    /* ── Floating Spotify overlay ───────────────────────────── */
    X9DrawSpotifyOverlay(g_cfg.uiTheme);
}
