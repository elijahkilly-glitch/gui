#pragma once
/*
 * x9_shared.h — Shared runtime state + Spotify mini-player
 *
 * Fully redesigned with modernized UI/UX for all themes.
 */

#include <Windows.h>
#include <cmath>
#include <cstring>
#include <cstdio>

/* ── Spotify state ─────────────────────────────────────────────── */
struct X9SpotifyState {
    char  artist[128] = {};
    char  title[256]  = {};
    bool  playing     = false;   /* true = actively playing        */
    bool  found       = false;   /* Spotify window detected at all */
    DWORD lastPoll    = 0;
};

/* ── Session state ─────────────────────────────────────────────── */
struct X9SessionState {
    DWORD startTick = 0;
    int   kills     = 0;
};

#ifdef X9_SHARED_IMPL
X9SpotifyState g_spotify;
X9SessionState g_session;
#else
extern X9SpotifyState g_spotify;
extern X9SessionState g_session;
#endif

/* ── EnumWindows proc — finds Spotify by class + title content ── */
struct X9SpotifyFindData { char title[512]; bool found; };

static BOOL CALLBACK X9SpotifyEnumProc(HWND hwnd, LPARAM lp) {
    X9SpotifyFindData* d = reinterpret_cast<X9SpotifyFindData*>(lp);
    char cls[64]={}, ttl[512]={};
    GetClassNameA(hwnd, cls, sizeof(cls));
    GetWindowTextA(hwnd, ttl, sizeof(ttl));
    if (!ttl[0]) return TRUE;
    bool knownCls = strcmp(cls,"SpotifyMainWindow")==0 ||
                   strcmp(cls,"Chrome_WidgetWin_1")==0 ||
                   strcmp(cls,"Chrome_WidgetWin_0")==0;
    if (!knownCls) return TRUE;
    bool hasArrow  = strstr(ttl," - ")!=nullptr;
    bool isSpotify = strncmp(ttl,"Spotify",7)==0;
    if (!hasArrow && !isSpotify) return TRUE;
    strncpy_s(d->title, sizeof(d->title), ttl, sizeof(d->title)-1);
    d->found = true;
    return FALSE;
}

inline void X9PollSpotify() {
    DWORD now = GetTickCount();
    if (now - g_spotify.lastPoll < 1500) return;
    g_spotify.lastPoll = now;
    X9SpotifyFindData fd={};
    EnumWindows(X9SpotifyEnumProc, reinterpret_cast<LPARAM>(&fd));
    if (!fd.found) { g_spotify.found=false; g_spotify.playing=false; return; }
    g_spotify.found = true;
    const char* sep = strstr(fd.title, " - ");
    if (sep && strncmp(fd.title,"Spotify",7)!=0) {
        g_spotify.playing = true;
        int aLen=(int)(sep-fd.title);
        if(aLen>=(int)sizeof(g_spotify.artist)) aLen=(int)sizeof(g_spotify.artist)-1;
        strncpy_s(g_spotify.artist, sizeof(g_spotify.artist), fd.title, aLen);
        strncpy_s(g_spotify.title,  sizeof(g_spotify.title),  sep+3, sizeof(g_spotify.title)-1);
    } else {
        g_spotify.playing = false;
        strncpy_s(g_spotify.artist, sizeof(g_spotify.artist), "Spotify", 8);
        strncpy_s(g_spotify.title,  sizeof(g_spotify.title),  "Paused", 7);
    }
}

/* ── Shared Visual Helpers ───────────────────────────────────── */
static inline void X9DrawPrev(ImDrawList* dl, float bx, float by, ImU32 c) {
    dl->AddRectFilled(ImVec2(bx-6, by-5), ImVec2(bx-4, by+5), c, 1.0f);
    dl->AddTriangleFilled(ImVec2(bx+5, by-6), ImVec2(bx+5, by+6), ImVec2(bx-3, by), c);
}
static inline void X9DrawNext(ImDrawList* dl, float bx, float by, ImU32 c) {
    dl->AddRectFilled(ImVec2(bx+4, by-5), ImVec2(bx+6, by+5), c, 1.0f);
    dl->AddTriangleFilled(ImVec2(bx-5, by-6), ImVec2(bx-5, by+6), ImVec2(bx+3, by), c);
}
static inline void X9DrawPause(ImDrawList* dl, float bx, float by, ImU32 c) {
    dl->AddRectFilled(ImVec2(bx-4, by-5), ImVec2(bx-1, by+5), c, 1.0f);
    dl->AddRectFilled(ImVec2(bx+1, by-5), ImVec2(bx+4, by+5), c, 1.0f);
}
static inline void X9DrawPlay(ImDrawList* dl, float bx, float by, ImU32 c) {
    dl->AddTriangleFilled(ImVec2(bx-3, by-6), ImVec2(bx-3, by+6), ImVec2(bx+6, by), c);
}

/* ── Drag and Scroll State ───────────────────────────────────── */

/* ══════════════════════════════════════════════════════════════════
   UNIFIED SPOTIFY WIDGET
   ══════════════════════════════════════════════════════════════════
   X9DrawMenuSpotify()  —  call from any theme's Misc/Settings tab.
   X9DrawSpotifyOverlay() — call from esp.cpp / overlay render loop.

   Theme enum controls accent colours and surface style:
     0 = Neko       — hot pink accent, dark glass
     1 = Industrial — user accent, sharp corners, monospace feel
     2 = Original   — warm rose accent, slightly lighter surface
*/

/* ── Shared animation state (one per TU, hidden via anonymous ns) */
namespace X9SpotInternal {
    static float timer    = 0.f;
    static float progress = 0.f;
    /* Overlay drag position — initialised on first use */
    static float ox = -1.f, oy = -1.f;
}

/* ── Theme-parameterised colour pack ──────────────────────────── */
struct X9SpotTheme {
    ImU32 surface;       /* card background            */
    ImU32 surfaceRim;    /* gloss strip top             */
    ImU32 border;        /* card border (playing)       */
    ImU32 borderOff;     /* card border (idle/paused)   */
    ImU32 accent;        /* primary accent              */
    ImU32 accentDim;     /* progress track fill         */
    ImU32 textPrimary;   /* song title                  */
    ImU32 textSecondary; /* artist / timestamps         */
    ImU32 textMuted;     /* idle message                */
    float cornerRadius;  /* card rounding               */
};

static inline X9SpotTheme X9SpotGetTheme(int uiTheme)
{
    switch (uiTheme) {
    case 1: /* Industrial — accent from g_cfg is already applied; we reuse purple tones */
        return {
            IM_COL32( 13,  13,  16, 248),  /* surface      */
            IM_COL32(255, 255, 255,   6),  /* surfaceRim   */
            IM_COL32( 88,  88, 162, 100),  /* border on    */
            IM_COL32( 40,  40,  48, 200),  /* border off   */
            IM_COL32( 88,  88, 162, 255),  /* accent       */
            IM_COL32( 88,  88, 162, 180),  /* accentDim    */
            IM_COL32(210, 210, 218, 255),  /* textPrimary  */
            IM_COL32(110, 110, 124, 220),  /* textSecondary*/
            IM_COL32( 80,  80,  92, 200),  /* textMuted    */
            3.f,                           /* cornerRadius */
        };
    case 2: /* Original — warm rose */
        return {
            IM_COL32( 18,  13,  13, 245),
            IM_COL32(255, 200, 200,   8),
            IM_COL32(200,  80,  80, 100),
            IM_COL32( 60,  36,  36, 200),
            IM_COL32(200,  80,  80, 255),
            IM_COL32(200,  80,  80, 170),
            IM_COL32(230, 222, 222, 255),
            IM_COL32(140, 110, 110, 220),
            IM_COL32( 90,  70,  70, 200),
            8.f,
        };
    default: /* Neko — hot pink */
        return {
            IM_COL32( 14,  10,  16, 248),
            IM_COL32(255, 255, 255,   7),
            IM_COL32(255,  77, 128,  90),
            IM_COL32( 55,  30,  45, 200),
            IM_COL32(255,  77, 128, 255),
            IM_COL32(255,  77, 128, 160),
            IM_COL32(242, 236, 244, 255),
            IM_COL32(155, 125, 145, 220),
            IM_COL32( 95,  70,  85, 200),
            10.f,
        };
    }
}

/* ── Core card renderer — shared by both in-menu and overlay ──── */
/*
 * Draws a self-contained Spotify card into [px, py .. px+W, py+H].
 * dl        — draw list to use
 * px, py    — top-left corner of card
 * W, H      — card dimensions
 * th        — colour pack from X9SpotGetTheme()
 * anim      — current animation timer (seconds, free-running)
 * progress  — playback progress 0..1
 */
static void X9SpotDrawCard(ImDrawList* dl,
                            float px, float py, float W, float H,
                            const X9SpotTheme& th,
                            float anim, float progress)
{
    using namespace X9SpotInternal;
    float R = th.cornerRadius;

    /* ── Shadow ─────────────────────────────────────────────── */
    dl->AddRectFilled(ImVec2(px+3, py+3), ImVec2(px+W+3, py+H+3),
        IM_COL32(0,0,0,55), R+2.f);

    /* ── Surface ─────────────────────────────────────────────── */
    dl->AddRectFilled(ImVec2(px, py), ImVec2(px+W, py+H),
        th.surface, R);
    /* Top gloss strip */
    dl->AddRectFilledMultiColor(
        ImVec2(px, py), ImVec2(px+W, py+H*0.40f),
        th.surfaceRim, th.surfaceRim,
        IM_COL32(0,0,0,0), IM_COL32(0,0,0,0));

    /* ── Border — accent when playing, dim when not ─────────── */
    ImU32 border = g_spotify.playing ? th.border : th.borderOff;
    dl->AddRect(ImVec2(px, py), ImVec2(px+W, py+H), border, R, 0, 1.f);

    /* ── Idle / not found state ─────────────────────────────── */
    if (!g_spotify.playing) {
        /* Muted music icon */
        float ix = px + 16.f, iy = py + H * .5f;
        ImU32 mc = th.textMuted;
        dl->AddRectFilled(ImVec2(ix-2,iy-7), ImVec2(ix+2,iy+4),  mc, 1.f);
        dl->AddRectFilled(ImVec2(ix+2,iy-5), ImVec2(ix+8,iy-2),  mc, 1.f);
        dl->AddCircleFilled(ImVec2(ix-4,iy+4), 4.f, mc, 12);
        dl->AddCircleFilled(ImVec2(ix+6,iy+2), 3.f, mc, 12);
        const char* msg = g_spotify.found ? "Spotify paused" : "Spotify not detected";
        ImVec2 ms = ImGui::CalcTextSize(msg);
        dl->AddText(ImVec2(px+W*.5f-ms.x*.5f, py+(H-ms.y)*.5f), th.textMuted, msg);
        return;
    }

    /* ── Vinyl disc (album art placeholder) ──────────────────── */
    constexpr float ART  = 58.f;
    constexpr float PADL = 12.f;
    float ax  = px + PADL;
    float ay  = py + (H - ART) * .5f;
    float acx = ax + ART * .5f, acy = ay + ART * .5f;
    float vr  = ART * .44f;

    dl->AddRectFilled(ImVec2(ax, ay), ImVec2(ax+ART, ay+ART),
        IM_COL32(22, 16, 24, 255), 8.f);
    dl->AddRect(ImVec2(ax, ay), ImVec2(ax+ART, ay+ART),
        IM_COL32(255,255,255,10), 8.f, 0, 1.f);

    /* Disc body */
    dl->AddCircleFilled(ImVec2(acx, acy), vr, IM_COL32(16, 11, 20, 255), 40);
    /* Groove rings */
    for (int g = 1; g <= 4; g++)
        dl->AddCircle(ImVec2(acx, acy), vr*(0.35f + g*0.155f),
            IM_COL32(255,255,255, 5+g*3), 40, 1.f);
    /* Orbiting shimmer — rotates with anim */
    float angle = anim * 1.1f;
    float sx = acx + cosf(angle)*vr*.74f;
    float sy = acy + sinf(angle)*vr*.74f;
    dl->AddCircleFilled(ImVec2(sx, sy), 2.f, IM_COL32(255,255,255,50), 8);
    /* Centre label circle in accent */
    dl->AddCircleFilled(ImVec2(acx, acy), vr*.32f, th.accentDim, 24);
    dl->AddCircleFilled(ImVec2(acx, acy), vr*.10f, IM_COL32(14,10,16,255), 12);
    dl->AddCircle(ImVec2(acx, acy), vr, IM_COL32(255,255,255,14), 40, 1.2f);

    /* ── Info block ──────────────────────────────────────────── */
    float infoX = ax + ART + 11.f;
    float infoW = W - ART - PADL*2.f - 11.f;
    float lineH = ImGui::GetTextLineHeight();

    char titleBuf[48], artistBuf[40];
    int tlen = (int)strlen(g_spotify.title);
    int alen = (int)strlen(g_spotify.artist);
    snprintf(titleBuf,  sizeof(titleBuf),  "%.26s%s",
        g_spotify.title,  tlen>26 ? "..." : "");
    snprintf(artistBuf, sizeof(artistBuf), "%.22s%s",
        g_spotify.artist, alen>22 ? "..." : "");

    dl->AddText(ImVec2(infoX, py+12.f), th.textPrimary,   titleBuf);
    dl->AddText(ImVec2(infoX, py+12.f+lineH+2.f), th.textSecondary, artistBuf);

    /* Pulsing live dot — top-right */
    float dotX = px+W-11.f, dotY = py+12.f+lineH*.5f;
    float pulse = sinf(anim*3.5f)*.5f+.5f;
    ImU32 dotFill = (th.accent & 0x00FFFFFF) | ((ImU32)(160+pulse*80)<<24);
    ImU32 dotRim  = (th.accent & 0x00FFFFFF) | ((ImU32)(35+pulse*25)<<24);
    dl->AddCircleFilled(ImVec2(dotX, dotY), 3.5f, dotFill, 12);
    dl->AddCircle(ImVec2(dotX, dotY), 3.5f+pulse*2.5f, dotRim, 12, 1.f);

    /* ── Progress bar ────────────────────────────────────────── */
    float pbY  = py + H - 26.f;
    float pbX0 = infoX;
    float pbW  = infoW - 2.f;
    /* Track */
    dl->AddRectFilled(ImVec2(pbX0, pbY), ImVec2(pbX0+pbW, pbY+3.f),
        IM_COL32(50,38,52,255), 1.5f);
    /* Fill */
    float fillW = pbW * progress;
    if (fillW > 3.f) {
        ImU32 aFade = (th.accent & 0x00FFFFFF) | (0xC0u<<24);
        dl->AddRectFilledMultiColor(
            ImVec2(pbX0, pbY), ImVec2(pbX0+fillW, pbY+3.f),
            aFade, th.accent, th.accent, aFade);
        /* Playhead knob */
        dl->AddCircleFilled(ImVec2(pbX0+fillW, pbY+1.5f), 4.5f,
            IM_COL32(235,232,238,230), 12);
        dl->AddCircle(ImVec2(pbX0+fillW, pbY+1.5f), 4.5f,
            th.accentDim, 12, 1.f);
    }

    /* ── Equaliser bars ──────────────────────────────────────── */
    constexpr int kBars = 12;
    float eqX = infoX;
    float eqY = py + H - 10.f;
    constexpr float barW = 3.f, barGap = 1.8f;
    for (int i = 0; i < kBars; i++) {
        float phase = anim*(1.6f + i*0.22f) + i*0.55f;
        float bh    = (sinf(phase)*.5f+.5f)*9.f + 2.f;
        float bx    = eqX + i*(barW+barGap);
        ImU32 bc    = (th.accent & 0x00FFFFFF)
                    | ((ImU32)(110 + i*9)<<24);
        dl->AddRectFilled(ImVec2(bx, eqY-bh), ImVec2(bx+barW, eqY), bc, 1.2f);
    }
}

/* ══════════════════════════════════════════════════════════════════
   X9DrawMenuSpotify  —  embedded card inside the menu's Misc tab
   Call signature: X9DrawMenuSpotify(int uiTheme)
   The function reserves its own ImGui Dummy so the caller just
   places the cursor and calls this; no manual Dummy needed after.
   ══════════════════════════════════════════════════════════════════ */
static void X9DrawMenuSpotify(int uiTheme)
{
    X9PollSpotify();

    /* Advance animation */
    float dt = ImGui::GetIO().DeltaTime;
    X9SpotInternal::timer += dt;
    if (g_spotify.playing)
        X9SpotInternal::progress = fmodf(
            X9SpotInternal::progress + dt * 0.018f, 1.f);

    float W  = ImGui::GetContentRegionAvail().x;
    float H  = 88.f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    X9SpotTheme th = X9SpotGetTheme(uiTheme);
    X9SpotDrawCard(dl, p.x, p.y, W, H, th,
        X9SpotInternal::timer, X9SpotInternal::progress);

    ImGui::Dummy(ImVec2(W, H));
    ImGui::Dummy(ImVec2(0, 6));
}

/* ══════════════════════════════════════════════════════════════════
   X9DrawSpotifyOverlay  —  floating draggable widget over the game
   Call from your per-frame overlay/ESP render (after ImGui::NewFrame).
   Renders nothing if g_spotify.playing is false.
   ══════════════════════════════════════════════════════════════════ */
static void X9DrawSpotifyOverlay(int uiTheme)
{
    if (!g_cfg.showSpotify) return;
    X9PollSpotify();
    if (!g_spotify.playing) return;

    float dt = ImGui::GetIO().DeltaTime;
    X9SpotInternal::timer += dt;
    X9SpotInternal::progress = fmodf(
        X9SpotInternal::progress + dt * 0.018f, 1.f);

    float sw = ImGui::GetIO().DisplaySize.x;
    float sh = ImGui::GetIO().DisplaySize.y;

    constexpr float W = 310.f, H = 88.f;
    if (X9SpotInternal::ox < 0.f) {
        X9SpotInternal::ox = 22.f;
        X9SpotInternal::oy = sh - H - 28.f;
    }

    /* Clamp to screen */
    if (X9SpotInternal::ox < 0) X9SpotInternal::ox = 0;
    if (X9SpotInternal::oy < 0) X9SpotInternal::oy = 0;
    if (X9SpotInternal::ox + W > sw) X9SpotInternal::ox = sw - W;
    if (X9SpotInternal::oy + H > sh) X9SpotInternal::oy = sh - H;

    ImGui::SetNextWindowPos(
        ImVec2(X9SpotInternal::ox, X9SpotInternal::oy), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(W, H));
    ImGui::SetNextWindowBgAlpha(0.f);

    ImGuiWindowFlags wf =
        ImGuiWindowFlags_NoTitleBar       |
        ImGuiWindowFlags_NoResize         |
        ImGuiWindowFlags_NoScrollbar      |
        ImGuiWindowFlags_NoScrollWithMouse|
        ImGuiWindowFlags_NoBackground     |
        ImGuiWindowFlags_NoFocusOnAppearing|
        ImGuiWindowFlags_NoNav;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    if (!ImGui::Begin("##x9spot_ov", nullptr, wf)) {
        ImGui::PopStyleVar();
        ImGui::End();
        return;
    }
    ImGui::PopStyleVar();

    /* Drag */
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
        && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        X9SpotInternal::ox += ImGui::GetIO().MouseDelta.x;
        X9SpotInternal::oy += ImGui::GetIO().MouseDelta.y;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    X9SpotTheme th = X9SpotGetTheme(uiTheme);
    X9SpotDrawCard(dl,
        X9SpotInternal::ox, X9SpotInternal::oy, W, H,
        th, X9SpotInternal::timer, X9SpotInternal::progress);

    ImGui::End();
}
