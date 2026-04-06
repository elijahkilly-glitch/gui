/*
 * menu.cpp  —  x9  Theme Dispatcher
 *
 * Three complete themes compiled in, selected at runtime via g_cfg.uiTheme.
 *   1 = Industrial (dark sidebar nav, monospace labels, toggle switches)
 *   2 = Original   (cat light theme without dark mode extras)
 *
 * To add a theme: create menu_theme_XXXX.h as namespace ThemeXXXX,
 * include it here, bump kThemeCount, add label + dispatch case.
 */

#include "menu.h"
#include "loader.h"
#include "aimbot.h"
#include "roblox.h"
#include "settings.h"
#include "console.h"
#include "imgui.h"
#include <Windows.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>

/* ── Include all theme implementations ───────────────────────── */
#include "menu_theme_industrial.h"
#include "menu_theme_original.h"

/* ── Theme metadata ───────────────────────────────────────────── */
static constexpr int kThemeCount = 3;
static const char* kThemeNames[kThemeCount] = {
    "Neko  \xE0\xB8\x85^*^\xE0\xB8\x85",
    "Industrial",
    "Original",
};

/* Sync open/closed state — when switching themes, carry visibility over */
static void SyncVisibility(bool visible) {
    /* Set all themes to the same open state so toggling via DELETE
     * works regardless of which theme was previously active.          */
    if (visible) {
        ThemeIndustrial::s_visible = true;
        ThemeOriginal::s_visible   = true;
    } else {
        ThemeIndustrial::s_visible = false;
        ThemeOriginal::s_visible   = false;
    }
}

static bool GetCurrentVisibility() {
    switch (g_cfg.uiTheme) {
    case 1:  return ThemeIndustrial::s_visible;
    case 2:  return ThemeOriginal::s_visible;
    }
}

static void DispatchApplyTheme() {
    switch (g_cfg.uiTheme) {
    case 1:  ThemeIndustrial::ApplyTheme(); break;
    case 2:  ThemeOriginal::ApplyTheme();   break;
    }
}

/* ── Public Menu API ──────────────────────────────────────────── */

void Menu::ApplyTheme() {
    DispatchApplyTheme();
}

void Menu::Toggle() {
    bool newState = !GetCurrentVisibility();
    SyncVisibility(newState);
}

bool Menu::IsVisible() {
    return GetCurrentVisibility();
}

void Menu::Render() {
    /* Re-apply theme each frame so switching is instant */
    DispatchApplyTheme();

    /* ── Theme switcher pill — always drawn, top-right corner ──── */
    {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        float sw = ImGui::GetIO().DisplaySize.x;

        constexpr float pH   = 22.f;
        constexpr float pPad = 10.f;
        constexpr float gap  =  2.f;

        /* Measure total width */
        float totalW = 0.f;
        for (int i = 0; i < kThemeCount; i++)
            totalW += ImGui::CalcTextSize(kThemeNames[i]).x + pPad * 2.f + gap;

        float startX = sw - totalW - 12.f;
        float startY = 5.f;

        /* Outer pill background */
        fg->AddRectFilled(
            {startX - 4.f, startY - 2.f},
            {sw - 8.f, startY + pH + 2.f},
            IM_COL32(14, 14, 18, 210), pH * .5f + 2.f);
        fg->AddRect(
            {startX - 4.f, startY - 2.f},
            {sw - 8.f, startY + pH + 2.f},
            IM_COL32(55, 55, 65, 180), pH * .5f + 2.f, 0, 1.f);

        float curX = startX;
        for (int i = 0; i < kThemeCount; i++) {
            ImVec2 tsz  = ImGui::CalcTextSize(kThemeNames[i]);
            float  btnW = tsz.x + pPad * 2.f;
            bool   active = (g_cfg.uiTheme == i);

            ImVec2 bmin = {curX, startY};
            ImVec2 bmax = {curX + btnW, startY + pH};

            /* ImGui hit area */
            ImGui::SetCursorScreenPos(bmin);
            char bid[32]; snprintf(bid, sizeof(bid), "##tsw%d", i);
            ImGui::InvisibleButton(bid, {btnW, pH});
            bool hov = ImGui::IsItemHovered();
            bool clk = ImGui::IsItemClicked();

            if (clk && !active) {
                bool wasVisible = GetCurrentVisibility();
                g_cfg.uiTheme = i;
                SyncVisibility(wasVisible);
                DispatchApplyTheme();
            }

            /* Per-theme accent color for the active indicator */
            ImU32 acCol;
            if      (i == 0) acCol = IM_COL32(255,  77, 128, 255);  /* Neko hot pink */
            else if (i == 1) acCol = IM_COL32( 88,  88, 162, 255);  /* Industrial purple */
            else             acCol = IM_COL32(200,  80,  80, 255);  /* Original red */

            if (active) {
                fg->AddRectFilled(bmin, bmax,
                    (acCol & 0x00FFFFFF) | (0x50u << 24), pH * .45f);
                fg->AddRect(bmin, bmax,
                    (acCol & 0x00FFFFFF) | (0xC8u << 24), pH * .45f, 0, 1.2f);
                /* Tiny accent dot above label */
                fg->AddCircleFilled(
                    {curX + btnW * .5f, startY + 2.f}, 2.f, acCol, 8);
            } else if (hov) {
                fg->AddRectFilled(bmin, bmax, IM_COL32(255,255,255,18), pH*.45f);
            }

            /* Label text */
            fg->AddText(
                {curX + pPad, startY + (pH - tsz.y) * .5f},
                active
                    ? IM_COL32(255, 255, 255, 245)
                    : IM_COL32(140, 140, 150, 200),
                kThemeNames[i]);

            curX += btnW + gap;
        }
    }

    /* ── Dispatch render to active theme ─────────────────────── */
    switch (g_cfg.uiTheme) {
    case 1:  ThemeIndustrial::Render(); break;
    case 2:  ThemeOriginal::Render();   break;
    }
}
