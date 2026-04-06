#pragma once
#include <Windows.h>
#include <unordered_set>
#include <string>

/* ── ESP ──────────────────────────────────────────────────────── */
struct ESPSettings {
    bool  enabled           = false;
    bool  showBox           = true;
    int   boxType           = 0;
    float boxCol[3]         = { 0.70f, 0.40f, 1.00f };
    float boxThickness      = 1.0f;
    bool  rainbowBox        = false;
    bool  filledBox         = false;
    float fillAlpha         = 0.08f;
    bool  boxOutline        = true;
    float cornerLen         = 0.25f;
    bool  dynBoxColor       = false;
    bool  pulseBox          = false;
    bool  showNames         = true;
    bool  showDisplayName   = false;
    float nameCol[3]        = { 1, 1, 1 };
    bool  nameBg            = false;
    float nameBgAlpha       = 0.4f;
    bool  showHealth        = true;
    bool  showHealthNum     = false;
    int   healthBarSide     = 0;
    bool  showDistance      = true;
    float maxDistance       = 1000.0f;
    bool  showSkeleton      = false;
    float skeletonCol[3]    = { 1, 1, 1 };
    float skeletonThick     = 1.5f;
    bool  skeletonRainbow   = false;
    bool  showSnapline      = false;
    int   snaplineStyle     = 0;
    float snaplineCol[3]    = { 1, 1, 1 };
    bool  snaplinePulse     = false;
    bool  showHeadDot       = false;
    float headDotRadius     = 4.0f;
    bool  headDotFill       = true;
    bool  visCheck          = false;
    float visCol[3]         = { 0, 1, 0.3f };
    float visRange          = 200.0f;
    bool  lowHpFlash        = false;
    float lowHpThresh       = 30.0f;
    bool  offscreenArrows   = false;
    float offscreenMargin   = 60.0f;
    float arrowCol[3]       = { 1, 0.3f, 0.3f };
    float arrowSize         = 10.0f;
    bool  chams             = false;
    float chamsCol[3]       = { 1, 0, 0.5f };
    float chamsAlpha        = 0.18f;
    bool  chamsPulse        = false;
    int   chamsMode         = 0;       /* 0=Flat 1=Mesh                   */
    bool  chamsRimLight     = false;   /* brighter edge outline            */
    bool  chamsWireframe    = false;   /* outline only, no fill            */
    bool  chamsRainbow      = false;   /* per-player hue-cycle             */
    bool  showRadar         = false;
    float radarSize         = 160.0f;
    float radarRange        = 300.0f;
    float radarX            = 20.0f;
    float radarY            = 20.0f;
    float radarDotSize      = 4.0f;
    bool  radarRotate       = false;
    bool  showPlayerCount   = false;
    bool  showDead          = false;
    bool  customCrosshair   = false;
    int   crosshairStyle    = 0;
    float crosshairSize     = 10.0f;
    float crosshairThick    = 1.5f;
    float crosshairGap      = 4.0f;
    float crosshairCol[3]   = { 1, 1, 1 };
    bool  crosshairDynamic  = false;
    float crosshairAlpha    = 1.0f;
    bool  spectatorAlert    = false;
    bool  showFovCircle     = false;
    float fovCircleRadius   = 100.0f;
    float fovCircleCol[3]   = { 1, 1, 1 };

    /* ── Target HUD ───────────────────────────────────────────── */
    bool  showTargetHud     = true;
    bool  tgtHudShowHp      = true;
    bool  tgtHudShowHpNum   = true;
    bool  tgtHudShowDist    = true;
    float tgtHudAlpha       = 1.0f;
    float tgtHudOffsetX     = 0.f;
    float tgtHudOffsetY     = 0.f;
};

/* ── Movement ─────────────────────────────────────────────────── */
struct MovementSettings {
    bool  useSpeed          = false;
    float walkSpeed         = 16.0f;
    bool  useJump           = false;
    float jumpPower         = 50.0f;
    bool  noclip            = false;
    bool  antiAFK           = false;
    bool  alwaysSprint      = false;
    bool  infiniteJump      = false;
    bool  antiVoid          = false;
    float antiVoidY         = -500.0f;
    bool  fly               = false;
    float flySpeed          = 50.0f;
    bool  lowGravity        = false;
    float gravityScale      = 0.3f;
    bool  godMode           = false;
    bool  bunnyHop          = false;
    bool  noSlip            = false;
    bool  regenHP           = false;
    float regenRate         = 1.0f;
};


/* ── World / Lighting / Fog ───────────────────────────────────── */
struct WorldSettings {
    bool  fogEnabled    = false;   /* writes fog every frame when true   */
    bool  lightingEnabled=false;   /* writes lighting every frame        */
    /* Fog */
    float fogEnd        = 100000.f;
    float fogStart      = 0.f;
    float fogColor[3]   = { 0.75f, 0.75f, 0.75f };
    /* Lighting */
    float brightness    = 1.0f;
    float clockTime     = 14.f;
    float ambient[3]    = { 0.f, 0.f, 0.f };
    float outdoorAmbient[3] = { 0.5f, 0.5f, 0.5f };
    float exposureComp  = 0.f;
};


/* ── Overlay effects (drawn when menu/loader is open) ────────── */
struct OverlayEffects {
    bool  darken        = false;   /* dim the screen behind menu    */
    float darkenAlpha   = 0.45f;  /* 0-1                           */
    bool  vignette      = false;  /* dark edge vignette            */
    float vignetteStr   = 0.6f;
    bool  snow          = false;  /* animated snowfall             */
    int   snowCount     = 120;    /* particle count 20-400         */
    float snowSpeed     = 1.0f;   /* fall speed multiplier         */
    float snowOpacity   = 0.7f;
    bool  scanlines     = false;  /* subtle horizontal scan lines  */
    float scanlineAlpha = 0.15f;
    bool  rgbBorder     = false;  /* cycling colour outer rim      */
    float rgbSpeed      = 1.0f;
};

/* ── Hitbox Expander ──────────────────────────────────────────── */
struct HitboxSettings {
    bool  enabled    = false;
    float sizeX      = 6.0f;
    float sizeY      = 6.0f;
    float sizeZ      = 6.0f;
    bool  teamCheck  = true;   /* skip teammates                   */
    bool  canCollide = false;  /* whether expanded part has collide */
};

/* ── Global config ────────────────────────────────────────────── */
struct Config {
    ESPSettings      esp;
    MovementSettings movement;
    bool  showMenu   = true;
    bool  menuLocked = false;
    float menuAlpha  = 0.95f;
    int   menuX      = 80;
    int   menuY      = 80;
    /* ── Glow ──────────────────────────────────────────────────── */
    bool  menuGlow       = true;
    float menuGlowIntens = 1.0f;
    bool  espBoxGlow     = true;
    float espBoxGlowIntens = 0.6f;
    bool  catDarkMode    = false;  /* dark cat theme vs pink */
    int   uiTheme        = 1;      /* 0=Neko 1=Industrial 2=Original */
    WorldSettings    world;
    HitboxSettings   hitbox;
    OverlayEffects   fx;

    /* ── Cat theme extras ─────────────────────────────────────── */
    bool  catCrosshair   = true;   /* cat-face crosshair at cursor  */
    bool  showSpotify    = true;   /* spotify mini-player widget    */
    bool  showSession    = true;   /* session timer + kill counter  */
    bool  showHearts     = true;   /* floating heart particles      */
    bool  showPawTrail   = true;   /* walking paw prints at bottom  */
    float catEarSize     = 1.0f;   /* 0.5–2.0 scale for box ears   */
    bool  catBoxFace     = true;   /* cat face inside ESP boxes     */
    bool  catBoxPaws     = true;   /* paw prints at box corners     */
};

extern Config g_cfg;

/* ── Friendly list ────────────────────────────────────────────── *
 * Stores player names marked as friendly.  Checked by aimbot     *
 * (skip targeting) and ESP (force blue color).                   */
namespace FriendlyList {
    extern std::unordered_set<std::string> entries;
    inline bool Has(const std::string& name) { return entries.count(name) > 0; }
    inline void Add(const std::string& name) { entries.insert(name); }
    inline void Remove(const std::string& name) { entries.erase(name); }
    inline void Toggle(const std::string& name) { Has(name) ? Remove(name) : Add(name); }
}
