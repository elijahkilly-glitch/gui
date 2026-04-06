/*
 * main.cpp  -  x9 external  —  entry point
 *
 * Automatically re-launches as administrator via UAC if not already elevated.
 * Without admin rights, OpenProcess(PROCESS_VM_WRITE) fails silently and
 * no memory writes go through.
 */

#include "overlay.h"
#include "menu.h"
#include "loader.h"
#include "esp.h"
#include "aimbot.h"
#include "driver.h"
#include "roblox.h"
#include "settings.h"

#ifdef ENABLE_CONSOLE
#include "console.h"
#endif

#include <Windows.h>
#include <shellapi.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

/* ── Elevation ───────────────────────────────────────────────────── */

static bool IsAdmin()
{
    BOOL elevated = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION te{};
        DWORD sz = sizeof(te);
        if (GetTokenInformation(hToken, TokenElevation, &te, sz, &sz))
            elevated = te.TokenIsElevated;
        CloseHandle(hToken);
    }
    return elevated == TRUE;
}

/* Returns true if a re-launch was triggered (caller must exit). */
static bool ElevateIfNeeded()
{
    if (IsAdmin()) return false;

    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(NULL, path, MAX_PATH);

    SHELLEXECUTEINFOW sei{};
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOASYNC;
    sei.lpVerb       = L"runas";
    sei.lpFile       = path;
    sei.lpParameters = L"";
    sei.nShow        = SW_SHOWNORMAL;

    if (ShellExecuteExW(&sei))
        return true;

    MessageBoxW(NULL,
        L"x9 requires administrator privileges to read/write game memory.\n\n"
        L"Right-click x9.exe \u2192 Run as administrator.",
        L"x9 \u2014 Admin Required",
        MB_OK | MB_ICONWARNING);
    return true;
}

/* ── Precise 120fps frame cap ─────────────────────────────────────
 * timeBeginPeriod(1) gives 1ms Sleep resolution.
 * We Sleep for most of the budget then busy-wait the last 1ms
 * to hit the target time exactly without burning full CPU.          */
static LARGE_INTEGER g_qpcFreq;
static LARGE_INTEGER g_frameStart;
static constexpr double kTargetFrameMs = 1000.0 / 120.0;  /* 8.333ms */

static void FrameCapInit() {
    QueryPerformanceFrequency(&g_qpcFreq);
    QueryPerformanceCounter(&g_frameStart);
    timeBeginPeriod(1);
}

static void FrameCapWait() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsedMs = (double)(now.QuadPart - g_frameStart.QuadPart)
                     * 1000.0 / (double)g_qpcFreq.QuadPart;

    double remaining = kTargetFrameMs - elapsedMs;

    if (remaining > 1.5) {
        Sleep((DWORD)(remaining - 1.5));
    }

    do {
        QueryPerformanceCounter(&now);
        elapsedMs = (double)(now.QuadPart - g_frameStart.QuadPart)
                  * 1000.0 / (double)g_qpcFreq.QuadPart;
    } while (elapsedMs < kTargetFrameMs);

    g_frameStart = now;
}

/* ── Entry point ─────────────────────────────────────────────────── */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    if (ElevateIfNeeded())
        return 0;

#ifdef ENABLE_CONSOLE
    Console::Init();
    Console::Log("x9 starting (elevated)...");
#endif

    if (!Overlay::Init())
        return 1;

    FrameCapInit();
    Menu::ApplyTheme();
    Loader::Init();
    Overlay::SetClickThrough(false);

    bool cheatReady = false;

    while (Overlay::BeginFrame())
    {
        if (GetAsyncKeyState(VK_DELETE) & 1) Menu::Toggle();
        if (GetAsyncKeyState(VK_END)    & 1) break;

        /* Loader phase */
        if (!Loader::IsDone()) {
            Loader::Render();
            Overlay::EndFrame();
            FrameCapWait();
            continue;
        }

        /* One-time init after loader completes */
        if (!cheatReady) {
            cheatReady = true;
            Overlay::SetClickThrough(false);
            Driver::Init();
            Roblox::Init();
            Roblox::StartScanner();
#ifdef ENABLE_CONSOLE
            Console::Success("Cheat ready");
#endif
        }

        /* Main cheat loop */
        Overlay::SetClickThrough(!Menu::IsVisible());
        HWND hRoblox = Roblox::GetWindow();
        if (hRoblox) Overlay::MatchWindow(hRoblox);

        Roblox::Update();
        Roblox::HitboxExpanderTick();
        Aimbot::Update();
        ESP::Render();
        if (Menu::IsVisible()) Menu::Render();

        Overlay::EndFrame();
        FrameCapWait();
    }

    Roblox::StopScanner();
    Overlay::Shutdown();
    return 0;
}
