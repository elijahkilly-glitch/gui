/*
 * overlay.cpp - Transparent DX11 overlay with ImGui
 *
 * Creates a borderless topmost window with DWM transparency.
 * Initializes DirectX 11 device/swapchain and ImGui backends.
 * Console is compiled out of release builds — define ENABLE_CONSOLE
 * to re-enable it for debugging.
 */

#include "overlay.h"
#ifdef ENABLE_CONSOLE
#include "console.h"
#define CON_LOG(...)     Console::Log(__VA_ARGS__)
#define CON_ERROR(...)   Console::Error(__VA_ARGS__)
#define CON_SUCCESS(...) Console::Success(__VA_ARGS__)
#else
#define CON_LOG(...)     ((void)0)
#define CON_ERROR(...)   ((void)0)
#define CON_SUCCESS(...) ((void)0)
#endif

#include <dwmapi.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstring>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

/* Forward declare ImGui WndProc handler */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

/* ── Globals ─────────────────────────────────────────────────── */

static HWND                     g_hwnd = NULL;
static WNDCLASSEXA             g_wc = {};
static ID3D11Device*            g_device = nullptr;
static ID3D11DeviceContext*     g_context = nullptr;
static IDXGISwapChain*          g_swapChain = nullptr;
static ID3D11RenderTargetView*  g_rtv = nullptr;
static char                     g_className[64] = {};

/* ── WndProc ─────────────────────────────────────────────────── */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return true;

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (g_device && wp != SIZE_MINIMIZED) {
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            g_swapChain->ResizeBuffers(0, (UINT)LOWORD(lp), (UINT)HIWORD(lp), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* backBuf = nullptr;
            g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuf));
            if (backBuf) {
                g_device->CreateRenderTargetView(backBuf, nullptr, &g_rtv);
                backBuf->Release();
            }
        }
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ── Generate random class name ──────────────────────────────── */

static void GenerateRandomClassName() {
    srand((unsigned int)GetTickCount64());
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < 12; i++) {
        g_className[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    g_className[12] = '\0';
}

/* ── Init ────────────────────────────────────────────────────── */

bool Overlay::Init() {
    /* Generate a random window class name to avoid detection */
    GenerateRandomClassName();

    g_wc.cbSize = sizeof(WNDCLASSEXA);
    g_wc.style = CS_HREDRAW | CS_VREDRAW;
    g_wc.lpfnWndProc = WndProc;
    g_wc.hInstance = GetModuleHandleA(NULL);
    g_wc.lpszClassName = g_className;

    if (!RegisterClassExA(&g_wc)) {
        CON_ERROR("Failed to register window class");
        return false;
    }

    /* Get primary monitor size */
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    /* Create the overlay window */
    g_hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        g_className,
        "",  /* No title */
        WS_POPUP,
        0, 0, screenW, screenH,
        NULL, NULL, g_wc.hInstance, NULL
    );

    if (!g_hwnd) {
        CON_ERROR("Failed to create overlay window");
        return false;
    }

    /* ── DWM glass — extends into entire client area ─────────── */
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(g_hwnd, &margins);

    /* Remove LWA_COLORKEY (causes black halo around glow).
     * We rely on DWM alpha compositing instead — alpha=0 pixels
     * in the render target are the transparent areas.              */
    SetWindowLong(g_hwnd, GWL_EXSTYLE,
        GetWindowLong(g_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    /* ── Create DX11 device and swapchain ────────────────────── */

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Width  = screenW;
    scd.BufferDesc.Height = screenH;
    /* RGBA with pre-multiplied alpha required for DWM compositing */
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator   = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    scd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = g_hwnd;
    scd.SampleDesc.Count   = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed   = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0,
        D3D11_SDK_VERSION, &scd,
        &g_swapChain, &g_device, &featureLevel, &g_context
    );

    if (FAILED(hr)) {
        CON_ERROR("Failed to create DX11 device (0x%08X)", hr);
        return false;
    }

    /* Create render target view */
    ID3D11Texture2D* backBuffer = nullptr;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
    backBuffer->Release();

    /* ── Premultiplied alpha blend state ─────────────────────────
     * Standard blend: src_alpha * src + (1-src_alpha) * dst
     * This lets alpha=0 pixels be fully transparent when DWM
     * composites the overlay onto the desktop.                     */
    {
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable           = TRUE;
        bd.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        ID3D11BlendState* pBlend = nullptr;
        if (SUCCEEDED(g_device->CreateBlendState(&bd, &pBlend))) {
            float bf[4] = {};
            g_context->OMSetBlendState(pBlend, bf, 0xFFFFFFFF);
            pBlend->Release();
        }
    }

    CON_SUCCESS("DX11 device created (Feature Level: 0x%X)", featureLevel);

    /* ── Init ImGui ──────────────────────────────────────────── */

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  /* Don't save layout to disk */

    /* ── Load custom font (CustomFont.ttf next to the exe) ───── */
    {
        ImGuiIO& fontIO = ImGui::GetIO();
        /* Build path: exe directory + "\CustomFont.ttf" */
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        /* Strip exe filename, keep directory */
        for (int i = (int)strlen(exePath) - 1; i >= 0; i--) {
            if (exePath[i] == '\\' || exePath[i] == '/') {
                exePath[i + 1] = '\0';
                break;
            }
        }
        char fontPath[MAX_PATH] = {};
        snprintf(fontPath, MAX_PATH, "%sCustomFont.ttf", exePath);

        /* Try loading at 14px — fallback to default if file missing */
        ImFont* customFont = fontIO.Fonts->AddFontFromFileTTF(fontPath, 14.0f);
        if (!customFont) {
            /* Fallback: try relative path */
            customFont = fontIO.Fonts->AddFontFromFileTTF("CustomFont.ttf", 14.0f);
        }
        if (!customFont) {
            fontIO.Fonts->AddFontDefault();
            CON_LOG("CustomFont.ttf not found — using default font");
        } else {
            /* Build atlas so the font is ready before first frame */
            fontIO.Fonts->Build();
            CON_SUCCESS("CustomFont.ttf loaded at 14px");
        }
    }

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);

    CON_SUCCESS("ImGui initialized");

    return true;
}

void Overlay::Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_rtv) g_rtv->Release();
    if (g_swapChain) g_swapChain->Release();
    if (g_context) g_context->Release();
    if (g_device) g_device->Release();

    if (g_hwnd) DestroyWindow(g_hwnd);
    UnregisterClassA(g_className, g_wc.hInstance);
}

bool Overlay::BeginFrame() {
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        if (msg.message == WM_QUIT) return false;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    return true;
}




HWND Overlay::GetWindow() {
    return g_hwnd;
}

void Overlay::MatchWindow(HWND target) {
    if (!target || !g_hwnd) return;

    RECT r;
    if (GetWindowRect(target, &r)) {
        int w = r.right - r.left;
        int h = r.bottom - r.top;
        SetWindowPos(g_hwnd, HWND_TOPMOST, r.left, r.top, w, h, SWP_NOACTIVATE);
    }
}

void Overlay::SetClickThrough(bool clickThrough) {
    if (!g_hwnd) return;

    LONG_PTR exStyle = GetWindowLongPtrA(g_hwnd, GWL_EXSTYLE);
    if (clickThrough) {
        exStyle |= WS_EX_TRANSPARENT;
    } else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtrA(g_hwnd, GWL_EXSTYLE, exStyle);
}

/* ── Stream Proof ─────────────────────────────────────────────── */
/* WDA_EXCLUDEFROMCAPTURE (value 0x11) hides the window from         *
 * screen capture tools (OBS, Windows Game Bar, BitBlt, etc.)       *
 * while keeping it visible to the user on-screen.                  *
 * Falls back gracefully on older Windows versions (pre-20H1).      */

void Overlay::SetStreamProof(bool enabled) {
    if (!g_hwnd) return;

    /* SetWindowDisplayAffinity is available from Win7+.
     * WDA_EXCLUDEFROMCAPTURE = 0x00000011 (Win10 20H1+)            */
    typedef BOOL (WINAPI *PFN_SWDA)(HWND, DWORD);
    static PFN_SWDA pfn = nullptr;
    if (!pfn)
        pfn = (PFN_SWDA)GetProcAddress(GetModuleHandleA("user32.dll"),
                                        "SetWindowDisplayAffinity");
    if (pfn) {
        /* 0x11 = WDA_EXCLUDEFROMCAPTURE, 0x00 = WDA_NONE */
        pfn(g_hwnd, enabled ? 0x00000011u : 0x00000000u);
    }
}

/* ── FPS target ───────────────────────────────────────────────── */

static int g_fpsTarget = 0;   /* 0 = uncapped */

void Overlay::SetFpsTarget(int fps) {
    g_fpsTarget = fps;
}

void Overlay::EndFrame() {
    ImGui::Render();

    /* Clear to fully transparent (alpha=0 → DWM composites as see-through) */
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
    g_context->ClearRenderTargetView(g_rtv, clearColor);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    /* SyncInterval 0 = uncapped (no vsync), 1 = vsync              */
    UINT syncInterval = (g_fpsTarget == 0) ? 0u : 1u;

    /* When a specific target is set, busy-wait to hit the frame time */
    if (g_fpsTarget > 0) {
        static LARGE_INTEGER s_lastFrame = {};
        LARGE_INTEGER freq, now;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&now);

        if (s_lastFrame.QuadPart != 0) {
            double targetS = 1.0 / (double)g_fpsTarget;
            double elapsedS = (double)(now.QuadPart - s_lastFrame.QuadPart)
                              / (double)freq.QuadPart;
            if (elapsedS < targetS) {
                double remMs = (targetS - elapsedS) * 1000.0;
                if (remMs > 1.5) Sleep((DWORD)(remMs - 1.0));
                /* Spin the remaining sub-ms */
                do {
                    QueryPerformanceCounter(&now);
                } while (((double)(now.QuadPart - s_lastFrame.QuadPart)
                           / (double)freq.QuadPart) < targetS);
            }
        }
        QueryPerformanceCounter(&s_lastFrame);
    }

    g_swapChain->Present(syncInterval, 0);
}
