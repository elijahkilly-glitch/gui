#pragma once
/*
 * roblox.h - Roblox data model traversal
 */
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>


struct PlayerData {
    std::string name;
    std::string displayName;   /* DisplayName property — separate from username */
    float headPos[3]  = {};
    float rootPos[3]  = {};
    float health      = 100.f;
    float maxHealth   = 100.f;
    float lFoot[3]    = {};
    float rFoot[3]    = {};
    float lHand[3]    = {};
    float rHand[3]    = {};
    bool  hasLimbs    = false;
    bool  isLocalPlayer = false;
    uintptr_t ptr     = 0;
    uintptr_t humanoid= 0;
    long long userId  = 0;
    bool  valid       = false;
};

namespace Roblox {
    bool Init();
    void Update();
    void StartScanner();
    void StopScanner();
    void SetWalkSpeed(float speed);
    void SetJumpPower(float power);

    /* ── Camera control (memory aimbot) ─────────────────────── */
    /* yawRad / pitchRad are ABSOLUTE angles in radians           */
    void AdjustCameraAngles(float yawRad, float pitchRad);

    /* ── Local character control (spinbot / anti-aim) ────────── */
    void SetLocalYaw(float yawDeg);
    void SetLocalPitch(float pitchDeg);

    DWORD  GetPID();
    uintptr_t GetBase();
    uintptr_t GetWorkspace();
    bool   GetLocalPlayerPos(float out[3]); /* world position of local HRP */
    const float* GetViewMatrix();
    const std::vector<PlayerData>& GetPlayers();
    std::vector<PlayerData> GetPlayersCopy();
    int    GetScreenWidth();
    int    GetScreenHeight();
    HWND   GetWindow();

    /* ── Lighting / Fog / World FX ───────────────────────────── */
    /* All writes go directly to the Lighting service instance.
     * Call these from the menu slider callbacks — no per-frame cost. */
    void SetFogEnd(float end);           /* studs where fog is fully opaque  */
    void SetFogStart(float start);       /* studs where fog begins            */
    void SetFogColor(float r,float g2,float b); /* RGB 0-1                   */
    void SetBrightness(float b);         /* Lighting.Brightness 0-2          */
    void SetClockTime(float t);          /* 0-24 hour time of day            */
    void SetAmbient(float r,float g2,float b);  /* Lighting.Ambient RGB 0-1  */
    void SetOutdoorAmbient(float r,float g2,float b);
    void SetExposureCompensation(float e); /* -5..5                          */
    uintptr_t GetLightingService();      /* cached Lighting instance ptr     */
    void ApplyWorldSettings();             /* call every frame to persist writes */
    void HitboxExpanderTick();             /* call every frame — expands/resets HRP sizes */
}
