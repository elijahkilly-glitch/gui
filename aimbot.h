#pragma once
/*
 * aimbot.h
 */
#include <Windows.h>

struct PlayerData;
enum class TargetPart  { Head = 0, Torso = 1, Legs = 2 };
enum class AimbotMethod { Mouse = 0, Memory = 1 };   /* Mouse=SendInput  Memory=write camera angles */

namespace Aimbot {
    struct Settings {
        bool  enabled           = false;
        int   key               = VK_RBUTTON;
        AimbotMethod method     = AimbotMethod::Mouse;

        float fov               = 150.f;
        bool  drawFov           = true;
        float fovColor[3]       = { 1,1,1 };
        bool  dynamicFov        = false;
        float dynamicFovMin     = 30.f;

        float smooth            = 5.f;
        bool  adaptiveSmooth    = false;
        bool  randomSmooth      = false;
        float randomSmoothVar   = 0.2f;

        TargetPart part         = TargetPart::Head;
        bool  sticky            = false;
        bool  ignoreTeammates   = true;
        float maxAimDist        = 500.f;
        int   priorityMode      = 0;       // 0=Crosshair 1=LowestHP 2=Nearest
        bool  visibleOnly       = false;
        bool  rageMode          = false;
        float reactionTime      = 0.f;
        bool  switchDelay       = false;

        bool  jitter            = false;
        float jitterStr         = 1.5f;
        bool  jitterBothAxes    = false;
        float jitterFreq        = 1.f;

        bool  drawDeadzone      = false;
        float deadzoneSize      = 20.f;

        bool  prediction        = false;
        float predictionScale   = 1.f;
        bool  gravityComp       = false;
        float gravityStr        = 1.f;
        bool  latencyComp       = false;
        float estimatedPingMs   = 50.f;

        bool  recoilControl     = false;
        float recoilY           = 1.f;
        float recoilX           = 0.f;

        bool  silentAim         = false;
        int   silentAimKey      = VK_LBUTTON;  /* hold to activate silent aim */
        float silentFov         = 150.f;        /* silent aim FOV radius (px)   */
        int   silentPriorityMode= 0;            /* 0=Crosshair 1=LowestHP 2=Nearest */
        int   silentPartMode    = 0;            /* 0=Head 1=Torso 2=Random 3=Closest */
        bool  silentVisCheck    = false;        /* silent aim visible-only       */
        bool  hitboxExpand      = false;        /* expand FOV acceptance zone    */
        float hitboxExpandPx    = 20.f;         /* pixels to inflate             */
        bool  wallCheck         = false;        /* depth-heuristic wall filter   */
        bool  triggerVisCheck   = false;        /* triggerbot visible-only check */
        bool  showKeyBind       = false;

        bool  triggerbot        = false;
        float hitchance         = 100.f;
        int   triggerbotKey     = VK_XBUTTON1;
        float triggerbotDelay   = 0.05f;
        bool  triggerbotBurst   = false;
        int   burstCount        = 3;
        float burstDelay        = 0.08f;
        bool  triggerOnlyInFov  = true;
        float triggerFov        = 15.f;
        /* ── Triggerbot extras ────────────────────────────────── */
        bool  triggerHoldMode   = false;   /* hold LMB while on target vs click */
        float triggerPreFire    = 0.f;     /* ms to wait after entering FOV     */
        bool  triggerHeadOnly   = false;   /* only fire on head target part      */
        bool  triggerNoMove     = false;   /* don't fire while player is moving  */
        bool  triggerDrawFov    = false;   /* draw own FOV circle                */
        float triggerFovCol[3]  = { 1.f, 0.8f, 0.1f }; /* yellow default       */

        bool  autoClick         = false;
        float autoClickDelay    = 0.1f;
        float autoClickMinDist  = 5.f;
        bool  autoClickHold     = false;

        bool  hitmarker         = false;
        float hitmarkerSize     = 10.f;
        float hitmarkerCol[3]   = { 1,1,0 };
        float hitmarkerDuration = 0.3f;

        /* ── Spinbot / Anti-Aim ───────────────────────────────── */
        bool  spinbot           = false;
        float spinSpeed         = 720.f;   /* degrees per second */
        bool  antiAim           = false;
        int   antiAimMode       = 0;       /* 0=Flip 1=Jitter 2=Spin+Flip */
        bool  antiAimPitch      = true;    /* flip pitch (look down) */
        float antiAimJitterAmp  = 90.f;    /* jitter amplitude degrees */
    };

    extern Settings settings;
    void Update();
    const PlayerData* GetTarget();
    bool  HasActiveHitmarker();
    void  GetHitmarkerPos(float& x, float& y);
    float GetHitmarkerSettings(float& size, float col[3], float& duration);
}
