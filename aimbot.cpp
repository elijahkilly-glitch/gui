/*
 * aimbot.cpp
 *
 * Mouse aimbot:  SendInput pixel deltas with spring-damper smoothing
 * Memory aimbot: Pure angle-space — computes exact yaw/pitch to target,
 * lerps our locally-tracked angles toward it, writes once.
 * Never reads the CFrame back from memory (eliminates all
 * matrix extraction bugs and layout ambiguity).
 */

#include "aimbot.h"
#include "roblox.h"
#include "console.h"
#include "overlay.h"
#include "settings.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Aimbot {

    Settings settings;

    /* ── High-resolution timer ───────────────────────────────────── */
    static double HRSeconds() {
        LARGE_INTEGER freq, cnt;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&cnt);
        return (double)cnt.QuadPart / (double)freq.QuadPart;
    }

    /* ── State ────────────────────────────────────────────────────── */
    static uintptr_t lockedPtr = 0;
    static std::string lockedName = ""; // Track by name to handle respawns
    static uintptr_t lastLockPtr = 0;
    static float     currentSmooth = 5.f;
    static double    accumX = 0.0;
    static double    accumY = 0.0;
    static float     filtScrX = 0.f, filtScrY = 0.f;
    static bool      filtInit = false;
    static uintptr_t velTrackedPtr = 0;
    static float     velLastPos[3] = {};
    static float     velocity[3] = {};
    static double    velLastTime = 0.0;
    static bool      hitmarkerVisible = false;
    static DWORD     hitmarkerTime = 0;
    static float     hitmarkerX = 0.f, hitmarkerY = 0.f;
    static double    spinYaw = 0.0;
    static double    lastFrameTime = 0.0;

    /* Memory aimbot — we track our own yaw/pitch so we never need to
     * read the CFrame back from memory.                              */
    static float     memYaw = 0.f;
    static float     memPitch = 0.f;
    static bool      memAnglesInit = false;

    /* ── Helpers ──────────────────────────────────────────────────── */
    static inline float Dist2D(float x1, float y1, float x2, float y2) {
        float dx = x2 - x1, dy = y2 - y1;
        return sqrtf(dx * dx + dy * dy);
    }

    static bool W2S(const float pos[3], const float* vm,
        float sw, float sh, float out[2]) {
        float x = pos[0] * vm[0] + pos[1] * vm[1] + pos[2] * vm[2] + vm[3];
        float y = pos[0] * vm[4] + pos[1] * vm[5] + pos[2] * vm[6] + vm[7];
        float w = pos[0] * vm[12] + pos[1] * vm[13] + pos[2] * vm[14] + vm[15];
        if (w < 0.01f) return false;
        float iw = 1.f / w;
        out[0] = sw * .5f + x * iw * sw * .5f;
        out[1] = sh * .5f - y * iw * sh * .5f;
        return true;
    }

    static void Click(bool down) {
        mouse_event(down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }

    static void MoveMouse(double dx, double dy) {
        accumX += dx; accumY += dy;
        /* round() not floor() — prevents systematic under-movement bias */
        int ix = (int)round(accumX);
        int iy = (int)round(accumY);
        if (!ix && !iy) return;
        accumX -= ix; accumY -= iy;
        INPUT in = {};
        in.type = INPUT_MOUSE;
        in.mi.dwFlags = MOUSEEVENTF_MOVE
#ifdef MOUSEEVENTF_MOVE_NOCOALESCE
            | MOUSEEVENTF_MOVE_NOCOALESCE
#endif
            ;
        in.mi.dx = ix; in.mi.dy = iy;
        SendInput(1, &in, sizeof(INPUT));
    }

    static void SpringSmooth(float dx, float dy, float smooth,
        float dt, double& outX, double& outY) {
        /* Clamp dt: reject any spike > 25ms (scanner stall, GC, etc).
         * Use 6ms floor so first-frame alpha isn't 0.               */
        if (dt > 0.025f) dt = 0.025f;
        if (dt <= 0.f)   dt = 0.006f;

        /* Framerate-independent exponential smoothing.
         * half-life = smooth / 144 seconds.
         * alpha = 1 - exp(-dt * 144 / smooth)
         * At smooth=1 → nearly instant; smooth=30 → very gradual.
         * Identical result at 60, 120, 240fps.                      */
        float halfLifeSec = smooth / 144.f;
        if (halfLifeSec < 0.001f) halfLifeSec = 0.001f;
        float alpha = 1.f - expf(-dt / halfLifeSec);
        if (alpha < 0.f) alpha = 0.f;
        if (alpha > 1.f) alpha = 1.f;

        outX = (double)dx * alpha;
        outY = (double)dy * alpha;
    }

    /* ── Hitbox Expand — inflate the FOV acceptance radius ──────────────
     * Adds expandPx screen-space pixels to the FOV threshold for target
     * acquisition. Higher values = easier to target through wider angles.
     * Controlled by settings.hitboxExpandPx (0 = off, 0.1-80 typical).   */
    static float HitboxExpandedFov() {
        return settings.fov + (settings.hitboxExpand ? settings.hitboxExpandPx : 0.f);
    }
    static float SilentExpandedFov() {
        return settings.silentFov + (settings.hitboxExpand ? settings.hitboxExpandPx : 0.f);
    }

    /* ── Wall Check — multi-point W-depth occlusion heuristic ───────────
     * Tests four sample points (head, root, lFoot, rFoot) using the
     * view-projection W value. A player occluded by geometry will have
     * a W value dramatically different from their neighbours.
     *
     * Method:
     *   1. Project head and root via view matrix → get W_head, W_root.
     *   2. If either W < near plane threshold → behind camera, reject.
     *   3. Compare W_head / W_root ratio — a visible player has both
     *      points at a consistent depth (ratio ≈ 1.0).  A player behind
     *      a wall only has one point project normally → ratio spikes.
     *   4. Additionally test foot W values to confirm depth consistency.
     *
     * Returns true if the player appears visible (no wall between us).    */
    static bool WallCheck(const PlayerData& p, const float* vm) {
        auto W = [&](const float pos[3]) -> float {
            return pos[0]*vm[12] + pos[1]*vm[13] + pos[2]*vm[14] + vm[15];
        };

        float wH = W(p.headPos);
        float wR = W(p.rootPos);

        /* Behind near plane = definitely not visible */
        if (wH < 0.1f || wR < 0.1f) return false;

        /* Depth ratio check: visible players have consistent W across body */
        float ratio = wH / wR;
        if (ratio < 0.50f || ratio > 1.80f) return false;

        /* Extra foot consistency check when limb data available */
        if (p.hasLimbs) {
            float wLF = W(p.lFoot);
            float wRF = W(p.rFoot);
            /* Feet must also be in front of camera */
            if (wLF < 0.05f && wRF < 0.05f) return false;
            /* If we have a good foot sample, check it's depth-consistent */
            float footW = (wLF > wRF) ? wLF : wRF;
            float footRatio = footW / wR;
            if (footRatio < 0.30f || footRatio > 2.50f) return false;
        }

        return true;
    }


    /* Normalise angle to [-PI, PI] */
    static float NAngle(float a) {
        const float kPi = (float)M_PI;
        while (a > kPi) a -= 2.f * kPi;
        while (a < -kPi) a += 2.f * kPi;
        return a;
    }

    /* Extract yaw/pitch from view matrix.
     * View matrix rows are the camera axes in world space.
     * Row 2 (vm[8],vm[9],vm[10]) = forward/look vector.             */
    static void VMToAngles(const float* vm, float& yaw, float& pitch)
    {
        float lx = vm[8], ly = vm[9], lz = vm[10];
        float len = sqrtf(lx * lx + ly * ly + lz * lz);
        if (len < 1e-6f) { yaw = 0.f; pitch = 0.f; return; }
        lx /= len; ly /= len; lz /= len;
        yaw = atan2f(lx, lz);
        pitch = asinf((ly > 1.f) ? 1.f : (ly < -1.f) ? -1.f : ly);
    }

    /* World-space direction from camPos to target → yaw/pitch        */
    static void DirToAngles(const float camPos[3], const float tgt[3],
        float& yaw, float& pitch)
    {
        float dx = tgt[0] - camPos[0];
        float dy = tgt[1] - camPos[1];
        float dz = tgt[2] - camPos[2];
        float hd = sqrtf(dx * dx + dz * dz);
        yaw = atan2f(dx, dz);
        pitch = atan2f(dy, hd);
        const float kM = 89.f * (float)(M_PI / 180.0);
        if (pitch > kM) pitch = kM;
        if (pitch < -kM) pitch = -kM;
    }

    /* Camera world position from view matrix (inverse translation)   */
    static void CamPosFromVM(const float* vm, float out[3])
    {
        /* View matrix: upper-left 3x3 is rotation R, column 3 is R*(-pos)
         * So pos = -R^T * t, where t = (vm[3],vm[7],vm[11])          */
        out[0] = -(vm[0] * vm[3] + vm[1] * vm[7] + vm[2] * vm[11]);
        out[1] = -(vm[4] * vm[3] + vm[5] * vm[7] + vm[6] * vm[11]);
        out[2] = -(vm[8] * vm[3] + vm[9] * vm[7] + vm[10] * vm[11]);
    }

    /* ── Public helpers ───────────────────────────────────────────── */
    bool  HasActiveHitmarker() { return hitmarkerVisible; }
    void  GetHitmarkerPos(float& x, float& y) { x = hitmarkerX; y = hitmarkerY; }
    float GetHitmarkerSettings(float& sz, float col[3], float& dur) {
        sz = settings.hitmarkerSize;
        col[0] = settings.hitmarkerCol[0];
        col[1] = settings.hitmarkerCol[1];
        col[2] = settings.hitmarkerCol[2];
        dur = settings.hitmarkerDuration;
        return dur;
    }

    /* ══════════════════════════════════════════════════════════════
       MAIN UPDATE
       ══════════════════════════════════════════════════════════════ */
    void Update() {
        double now = HRSeconds();
        float  dt = (lastFrameTime > 0.0)
            ? (float)(now - lastFrameTime) : (1.f / 144.f);
        /* Reject dt spikes (scanner stall, OS scheduling, GC).
         * Cap at 25ms (40fps equivalent) — anything larger is a hitch,
         * not a real frame, and would cause a visible lurch.         */
        if (dt > 0.025f) dt = 0.025f;
        if (dt < 0.0001f) dt = 0.001f;
        lastFrameTime = now;
        DWORD nowMs = GetTickCount();

        /* ── Spinbot / Anti-aim ───────────────────────────────────── */
        if (settings.spinbot || settings.antiAim) {
            if (settings.spinbot) {
                spinYaw = fmod(spinYaw + settings.spinSpeed * dt, 360.0);
                Roblox::SetLocalYaw((float)spinYaw);
            }
            if (settings.antiAim) {
                switch (settings.antiAimMode) {
                case 0:
                    if (settings.antiAimPitch) Roblox::SetLocalPitch(89.f);
                    Roblox::SetLocalYaw((float)spinYaw + 180.f);
                    break;
                case 1: {
                    float jOff = ((rand() % 200) - 100) / 100.f * settings.antiAimJitterAmp;
                    if (settings.antiAimPitch)
                        Roblox::SetLocalPitch((rand() % 2) ? 89.f : -89.f);
                    Roblox::SetLocalYaw((float)spinYaw + jOff);
                    break;
                }
                case 2:
                    spinYaw = fmod(spinYaw + settings.spinSpeed * dt, 360.0);
                    if (settings.antiAimPitch) Roblox::SetLocalPitch(89.f);
                    Roblox::SetLocalYaw((float)spinYaw + 180.f);
                    break;
                }
            }
        }

        /* ── Triggerbot — runs independently before aimbot gate ──── */
        if (settings.triggerbot &&
            (GetAsyncKeyState(settings.triggerbotKey) & 0x8000)) {

            static DWORD tbCooldown = 0;
            static int   burstLeft = 0;
            static DWORD burstNext = 0;
            static DWORD tbEnteredFovAt = 0; /* for pre-fire delay */
            static bool  tbWasInFov = false;

            /* Gather data */
            const auto& tbPlayers = Roblox::GetPlayers();
            const float* tbVm = Roblox::GetViewMatrix();
            float tbSw = (float)Roblox::GetScreenWidth();
            float tbSh = (float)Roblox::GetScreenHeight();
            POINT tbCur{}; GetCursorPos(&tbCur);

            float tbLocalPos[3] = {};
            bool  tbHasLocal = Roblox::GetLocalPlayerPos(tbLocalPos);

            /* Don't fire while moving */
            bool tbMoving = false;
            if (settings.triggerNoMove) {
                static POINT tbLastCur = {};
                tbMoving = (abs(tbCur.x - tbLastCur.x) + abs(tbCur.y - tbLastCur.y)) > 3;
                tbLastCur = tbCur;
            }

            /* Scan for target in FOV */
            bool tbOnTarget = false;
            float tbClosest = 1e9f;
            float tbHitX = (float)tbCur.x, tbHitY = (float)tbCur.y;
            if (tbSw > 0 && tbSh > 0 && !tbMoving) {
                for (const auto& p : tbPlayers) {
                    if (p.isLocalPlayer || !p.valid) continue;
                    /* Use ratio — a revived/knocked player may have health
                       briefly at 0 while recovering; only skip truly dead   */
                    float _tbHpR = (p.maxHealth > 0.f) ? p.health / p.maxHealth : 1.f;
                    if (_tbHpR < 0.005f) continue;
                    if (FriendlyList::Has(p.name)) continue;

                    /* Real distance check */
                    if (tbHasLocal) {
                        float ddx = p.rootPos[0] - tbLocalPos[0];
                        float ddy = p.rootPos[1] - tbLocalPos[1];
                        float ddz = p.rootPos[2] - tbLocalPos[2];
                        if (sqrtf(ddx * ddx + ddy * ddy + ddz * ddz) > settings.maxAimDist) continue;
                    }

                    /* Target part selection */
                    const float* pos = (settings.triggerHeadOnly)
                        ? p.headPos : ((settings.part == TargetPart::Head) ? p.headPos : p.rootPos);

                    float scr[2];
                    if (!W2S(pos, tbVm, tbSw, tbSh, scr)) continue;
                    float sd = Dist2D((float)tbCur.x, (float)tbCur.y, scr[0], scr[1]);

                    float fovCheck = settings.triggerOnlyInFov
                        ? settings.triggerFov : settings.fov;
                    if (sd > fovCheck) continue;

                    /* Visibility raycasting: compare perspective W of the
                     * target against nearby occluders using depth consistency.
                     *
                     * Roblox doesn't expose a raycast API from usermode, so we
                     * use a VP-matrix depth approximation:
                     * 1. Compute the target's NDC depth (W value).
                     * 2. Check both head and root — if either has W < 0.5
                     * the target is behind the near plane (occluded).
                     * 3. Compare head W vs root W ratio — a player visible
                     * through a wall will have matching W values but a
                     * partially occluded one will not.
                     * This catches walls reliably without game API access.    */
                    /* Triggerbot vis/wall check — unified with WallCheck */
                    if (settings.triggerVisCheck || settings.wallCheck) {
                        if (!WallCheck(p, tbVm)) continue;
                    }

                    if (sd < tbClosest) {
                        tbClosest = sd;
                        tbHitX = scr[0]; tbHitY = scr[1];
                        tbOnTarget = true;
                    }
                }
            }

            /* Pre-fire delay tracking */
            if (tbOnTarget && !tbWasInFov) tbEnteredFovAt = nowMs;
            tbWasInFov = tbOnTarget;
            float preFireMs = settings.triggerPreFire;
            bool preFireReady = (preFireMs <= 0.f ||
                (nowMs - tbEnteredFovAt) >= (DWORD)preFireMs);

            /* Fire */
            if (tbOnTarget && preFireReady && nowMs > tbCooldown) {
                if ((float)(rand() % 100) < settings.hitchance) {
                    if (settings.triggerbotBurst) {
                        if (burstLeft <= 0) { burstLeft = settings.burstCount; burstNext = nowMs; }
                        if (nowMs >= burstNext) {
                            Click(true); Click(false); burstLeft--;
                            burstNext = nowMs + (DWORD)(settings.burstDelay * 1000);
                            if (burstLeft <= 0)
                                tbCooldown = nowMs + (DWORD)(settings.triggerbotDelay * 1000);
                        }
                    }
                    else if (settings.triggerHoldMode) {
                        Click(true);   /* hold down while on target */
                        tbCooldown = nowMs + 16; /* re-check every frame */
                    }
                    else {
                        Click(true); Click(false);
                        tbCooldown = nowMs + (DWORD)(settings.triggerbotDelay * 1000);
                    }
                    if (settings.hitmarker) {
                        hitmarkerVisible = true; hitmarkerTime = nowMs;
                        hitmarkerX = tbHitX; hitmarkerY = tbHitY;
                    }
                }
            }
            else if (settings.triggerHoldMode && !tbOnTarget) {
                Click(false);  /* release hold when no target */
            }
        }
        else {
            /* Key released — release hold if hold mode was active */
            if (settings.triggerHoldMode)
                Click(false);
        }

        /* Hitmarker decay */
        if (hitmarkerVisible &&
            (nowMs - hitmarkerTime) > (DWORD)(settings.hitmarkerDuration * 1000))
            hitmarkerVisible = false;

        /* ── Aimbot gate ──────────────────────────────────────────── */
        if (!settings.enabled) {
            lockedPtr = 0; lockedName = ""; filtInit = false; accumX = accumY = 0.0;
            memAnglesInit = false; return;
        }

        static DWORD aimHoldStart = 0;
        bool keyHeld = (GetAsyncKeyState(settings.key) & 0x8000) != 0;

        if (!keyHeld) {
            lockedPtr = 0; lockedName = ""; filtInit = false; accumX = accumY = 0.0;
            aimHoldStart = 0; memAnglesInit = false; return;
        }
        if (!aimHoldStart) aimHoldStart = nowMs;
        if (settings.reactionTime > 0.f &&
            (nowMs - aimHoldStart) < (DWORD)(settings.reactionTime * 1000.f)) return;

        /* ── Data ────────────────────────────────────────────────── */
        const auto& players = Roblox::GetPlayers();
        const float* vm = Roblox::GetViewMatrix();
        float sw = (float)Roblox::GetScreenWidth();
        float sh = (float)Roblox::GetScreenHeight();
        if (sw == 0.f || sh == 0.f) return;
        POINT cursor = {}; GetCursorPos(&cursor);

        /* Get local player world position for accurate distance checks */
        float localPos[3] = {};
        bool  hasLocalPos = Roblox::GetLocalPlayerPos(localPos);

        /* ── Target selection ────────────────────────────────────── */
        const PlayerData* target = nullptr;

        /* Persistent Re-lock: If we have a locked name, look for the player
           by name instead of address. This survives respawns and knockdowns. */
        if (!lockedName.empty()) {
            for (const auto& p : players) {
                /* Accept the target if valid — even health==0 briefly after
                   being revived. Only drop the lock if they are TRULY gone
                   (invalid ptr) or their health ratio is under 0.5 %        */
                float _rlHpR = (p.maxHealth > 0.f) ? p.health / p.maxHealth : 1.f;
                if (p.name == lockedName && p.valid && _rlHpR >= 0.005f) {
                    target = &p;
                    lockedPtr = p.ptr;
                    break;
                }
            }
            if (!target) {
                lockedPtr = 0; lockedName = "";
                velTrackedPtr = 0; filtInit = false; memAnglesInit = false;
            }
        }

        if (!target) {
            float best = 1e9f;
            for (const auto& p : players) {
                if (p.isLocalPlayer || !p.valid) continue;
                {   /* Only skip truly dead — ratio < 0.5% handles knocked/reviving */
                    float _hpR = (p.maxHealth > 0.f) ? p.health / p.maxHealth : 1.f;
                    if (_hpR < 0.005f) continue;
                }
                if (FriendlyList::Has(p.name)) continue;

                /* Real 3D distance from local player */
                float dx3 = p.rootPos[0] - localPos[0];
                float dy3 = p.rootPos[1] - localPos[1];
                float dz3 = p.rootPos[2] - localPos[2];
                float wd = sqrtf(dx3 * dx3 + dy3 * dy3 + dz3 * dz3);

                if (hasLocalPos && wd > settings.maxAimDist) continue;

                /* Visibility check — use WallCheck when enabled */
                if (settings.visibleOnly || settings.wallCheck) {
                    if (!WallCheck(p, vm)) continue;
                }

                const float* pos = (settings.part == TargetPart::Head) ? p.headPos : p.rootPos;
                float scr[2];
                if (!W2S(pos, vm, sw, sh, scr)) continue;
                float sd = Dist2D((float)cursor.x, (float)cursor.y, scr[0], scr[1]);
                if (sd > HitboxExpandedFov()) continue;
                float score = (settings.priorityMode == 1) ? p.health :
                    (settings.priorityMode == 2) ? wd : sd;
                if (score < best) {
                    best = score;
                    target = &p;
                    lockedPtr = p.ptr;
                    lockedName = p.name; // Remember the name!
                }
            }
            if (target && lockedPtr != lastLockPtr) {
                filtInit = false; accumX = accumY = 0.0;
                memAnglesInit = false;
                lastLockPtr = lockedPtr;
            }
        }

        if (!settings.randomSmooth) {
            currentSmooth = settings.smooth;
        }
        else if (lockedPtr != lastLockPtr) {
            float rv = ((rand() % 200) / 100.f - 1.f) * settings.randomSmoothVar;
            currentSmooth = (std::max)(1.f, settings.smooth * (1.f + rv));
        }

        if (!target) return;

        /* ── Velocity tracking ───────────────────────────────────── */
        const float* rawPos = (settings.part == TargetPart::Head) ? target->headPos : target->rootPos;

        if (velTrackedPtr == lockedPtr && velLastTime > 0.0) {
            float dtV = (float)(now - velLastTime);
            float moved = fabsf(rawPos[0] - velLastPos[0])
                + fabsf(rawPos[1] - velLastPos[1])
                + fabsf(rawPos[2] - velLastPos[2]);
            if (moved > 0.01f && dtV > 0.001f) {
                /* Lower EMA alpha: smoother velocity, less scanner-noise amplification */
                float alpha = (std::min)(0.20f, dtV * 8.f);
                for (int i = 0;i < 3;i++) {
                    float inst = (rawPos[i] - velLastPos[i]) / dtV;
                    velocity[i] = alpha * inst + (1.f - alpha) * velocity[i];
                }
                memcpy(velLastPos, rawPos, 12);
                velLastTime = now;
            }
        }
        else {
            velocity[0] = velocity[1] = velocity[2] = 0.f;
            memcpy(velLastPos, rawPos, 12);
            velLastTime = now;
        }
        velTrackedPtr = lockedPtr;

        /* ── Prediction ──────────────────────────────────────────── */
        float predicted[3]; memcpy(predicted, rawPos, 12);
        if (settings.prediction) {
            float la = 0.066f * settings.predictionScale;
            if (settings.latencyComp) la += settings.estimatedPingMs * 0.001f;
            for (int i = 0;i < 3;i++) predicted[i] += velocity[i] * la;
        }
        if (settings.gravityComp) predicted[1] += settings.gravityStr * 0.5f;

        /* ══════════════════════════════════════════════════════════
           MEMORY AIMBOT — angle space, write-only
           ══════════════════════════════════════════════════════════ */
        static bool s_memWasHeld2 = false;
        bool s_memHeld2 = (GetAsyncKeyState(settings.key) & 0x8000) != 0;
        if (s_memWasHeld2 && !s_memHeld2) { memAnglesInit = false; }
        s_memWasHeld2 = s_memHeld2;

        if (settings.method == AimbotMethod::Memory) {
            float camPos[3];
            CamPosFromVM(vm, camPos);
            if (!memAnglesInit) {
                VMToAngles(vm, memYaw, memPitch);
                memAnglesInit = true;
            }
            float tgtYaw, tgtPitch;
            DirToAngles(camPos, predicted, tgtYaw, tgtPitch);
            float dYaw = NAngle(tgtYaw - memYaw);
            float dPitch = NAngle(tgtPitch - memPitch);
            float angDist = sqrtf(dYaw * dYaw + dPitch * dPitch);
            if (angDist < 0.0002f) {
                if (settings.autoClick) {
                    static DWORD lastAC2 = 0;
                    if (nowMs - lastAC2 > (DWORD)(settings.autoClickDelay * 1000)) {
                        if (settings.autoClickHold) Click(true);
                        else { Click(true); Click(false); }
                        lastAC2 = nowMs;
                    }
                }
                return;
            }
            float safeDt = dt;
            if (safeDt > 0.025f) safeDt = 0.025f;
            if (safeDt <= 0.f)   safeDt = 0.006f;
            float smooth = settings.rageMode ? 1.f : currentSmooth;
            /* Framerate-independent exponential — same formula as SpringSmooth */
            float halfLife = smooth / 144.f;
            if (halfLife < 0.001f) halfLife = 0.001f;
            float alpha = 1.f - expf(-safeDt / halfLife);
            if (alpha < 0.f) alpha = 0.f;
            if (alpha > 1.f) alpha = 1.f;
            /* Per-frame angle velocity cap: 6 rad/s max (~344 deg/s).
             * Prevents camera from snapping even at smooth=1.        */
            float maxStep = 6.f * safeDt;
            float stepYaw   = dYaw   * alpha;
            float stepPitch = dPitch * alpha;
            if (stepYaw   >  maxStep) stepYaw   =  maxStep;
            if (stepYaw   < -maxStep) stepYaw   = -maxStep;
            if (stepPitch >  maxStep) stepPitch =  maxStep;
            if (stepPitch < -maxStep) stepPitch = -maxStep;
            memYaw = NAngle(memYaw + stepYaw);
            memPitch = memPitch + stepPitch;
            const float kM = 89.f * (float)(M_PI / 180.0);
            if (memPitch > kM) memPitch = kM;
            if (memPitch < -kM) memPitch = -kM;
            Roblox::AdjustCameraAngles(memYaw, memPitch);
            return;
        }

        /* ══════════════════════════════════════════════════════════
           SILENT AIM
           ── Design mirrors silent.cpp ──────────────────────────
           · SilentCombatChecks  — team / alive / health / dist / FOV
           · SilentGetBestPart   — head | HRP | random | closest-mouse
           · SilentGetBestTarget — sticky + three priority modes
           · Aim via mouse-warp  (method == Mouse)
           · Aim via camera-write (method == Memory)
           ══════════════════════════════════════════════════════════ */
        {
            /* ── Per-session state ─────────────────────────── */
            static POINT               s_savedCursor  = {};
            static bool                s_hasSaved     = false;
            static bool                s_needRestore  = false;
            /* Sticky lock — mirrors target::entity / target::part */
            static uintptr_t           s_stickyPtr    = 0;
            static std::string         s_stickyName   = "";
            static float               s_stickyPartW[3] = {};  /* world pos of best part */
            static bool                s_stickyValid  = false;

            bool saActive = settings.silentAim &&
                            (GetAsyncKeyState(settings.silentAimKey) & 0x8000);

            /* ── Absolute cursor warp (virtual-desktop coords) ─ */
            auto WarpAbs = [](float px, float py) {
                int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
                int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
                if (vw <= 0 || vh <= 0) return;
                INPUT in{};
                in.type       = INPUT_MOUSE;
                in.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE
                                | MOUSEEVENTF_VIRTUALDESK;
                in.mi.dx = (LONG)(((px - vx) / (float)vw) * 65535.f);
                in.mi.dy = (LONG)(((py - vy) / (float)vh) * 65535.f);
                SendInput(1, &in, sizeof(INPUT));
            };

            /* ── Combat checks (mirrors c_silent::combat_checks) ─
             * Returns true if this player is a valid target.
             * cursor2D is the current FOV reference point.         */
            auto SilentCombatChecks = [&](const PlayerData& p,
                                          float cursor2D[2]) -> bool {
                /* Team check */
                if (FriendlyList::Has(p.name))   return false;
                /* Alive check */
                float hpR = (p.maxHealth > 0.f) ? (p.health / p.maxHealth) : 1.f;
                if (hpR < 0.005f)                return false;
                /* Distance check */
                if (hasLocalPos) {
                    float ddx = p.rootPos[0] - localPos[0];
                    float ddy = p.rootPos[1] - localPos[1];
                    float ddz = p.rootPos[2] - localPos[2];
                    float wd  = sqrtf(ddx*ddx + ddy*ddy + ddz*ddz);
                    if (wd > settings.maxAimDist) return false;
                }
                /* Within-FOV check — uses silentFov, not shared fov */
                TargetPart silentPart = (TargetPart)settings.silentPartMode;
                const float* aimPos = (silentPart == TargetPart::Head)
                                      ? p.headPos : p.rootPos;
                float scr[2];
                if (!W2S(aimPos, vm, sw, sh, scr)) return false;
                float fovDist = Dist2D(cursor2D[0], cursor2D[1], scr[0], scr[1]);
                if (fovDist > SilentExpandedFov()) return false;
                /* Visibility — silentVisCheck + wallCheck */
                if (settings.silentVisCheck || settings.wallCheck) {
                    if (!WallCheck(p, vm)) return false;
                }
                return true;
            };

            /* ── Best-part world position (mirrors get_best_part) ─
             * Returns the world-space position to aim toward.
             * part setting: 0=Head  1=Torso/HRP  2=Random  3=Closest */
            auto SilentGetBestPart = [&](const PlayerData& p,
                                         float cursor2D[2],
                                         float outW[3]) -> bool {
                switch (settings.part) {
                case TargetPart::Head: {
                    memcpy(outW, p.headPos, 12);
                    return true;
                }
                case TargetPart::Torso: {       /* treat as HRP/root */
                    memcpy(outW, p.rootPos, 12);
                    return true;
                }
                case TargetPart::Legs: {        /* random from head/root/feet */
                    /* We don't have every limb, so pick randomly among what we have */
                    const float* opts[4] = { p.headPos, p.rootPos, p.lFoot, p.rFoot };
                    int validN = 0;
                    const float* valid[4] = {};
                    for (int i = 0; i < 4; i++) {
                        if (opts[i][0]!=0.f||opts[i][1]!=0.f||opts[i][2]!=0.f)
                            valid[validN++] = opts[i];
                    }
                    if (!validN) return false;
                    memcpy(outW, valid[rand() % validN], 12);
                    return true;
                }
                default: {
                    /* Closest-to-cursor: pick whichever of head/root is nearer */
                    float scrH[2], scrR[2];
                    bool  vH = W2S(p.headPos, vm, sw, sh, scrH);
                    bool  vR = W2S(p.rootPos, vm, sw, sh, scrR);
                    if (!vH && !vR) return false;
                    float dH = vH ? Dist2D(cursor2D[0],cursor2D[1],scrH[0],scrH[1]) : 1e9f;
                    float dR = vR ? Dist2D(cursor2D[0],cursor2D[1],scrR[0],scrR[1]) : 1e9f;
                    if (dH <= dR) { if (vH) { memcpy(outW, p.headPos, 12); return true; } }
                    if (vR) { memcpy(outW, p.rootPos, 12); return true; }
                    return false;
                }
                }
            };

            /* ── Target selection (mirrors get_best_target) ───────
             * Writes the best target's world aim-pos into outW[3].
             * Priority modes:
             *   0 = closest to cursor screen  (like case 0 in silent.cpp)
             *   1 = closest to camera origin  (like case 1)
             *   2 = closest in-game 3D dist   (like case 2)
             * Sticky aim locks onto the same player across frames.  */
            auto SilentGetBestTarget = [&](float cursor2D[2],
                                            float outW[3],
                                            uintptr_t& outPtr,
                                            std::string& outName) -> bool {

                /* Sticky: if we have a lock, verify it still passes checks */
                if (settings.sticky && s_stickyPtr) {
                    for (const auto& p : players) {
                        if (p.ptr != s_stickyPtr) continue;
                        if (!SilentCombatChecks(p, cursor2D)) break;  /* lost — fall through */
                        float partW[3];
                        if (!SilentGetBestPart(p, cursor2D, partW)) break;
                        memcpy(outW, partW, 12);
                        outPtr  = p.ptr;
                        outName = p.name;
                        return true;
                    }
                    /* Sticky target failed — clear */
                    s_stickyPtr  = 0;
                    s_stickyName = "";
                    s_stickyValid = false;
                }

                /* Scan for new best target */
                const PlayerData* best   = nullptr;
                float             bestScore = 1e9f;
                float             bestPartW[3] = {};

                /* Camera position for camera-distance priority */
                float camPos[3];
                CamPosFromVM(vm, camPos);

                for (const auto& p : players) {
                    if (p.isLocalPlayer || !p.valid) continue;
                    if (!SilentCombatChecks(p, cursor2D)) continue;
                    float partW[3];
                    if (!SilentGetBestPart(p, cursor2D, partW)) continue;

                    float score = 1e9f;
                    /* Silent uses its own priority mode */
                    switch (settings.silentPriorityMode) {
                    case 0: {   /* closest to cursor */
                        float scr[2];
                        if (!W2S(partW, vm, sw, sh, scr)) continue;
                        score = Dist2D(cursor2D[0], cursor2D[1], scr[0], scr[1]);
                        break;
                    }
                    case 1: {   /* lowest health */
                        score = p.health;
                        break;
                    }
                    case 2: {   /* nearest 3D */
                        if (hasLocalPos) {
                            float dx3 = p.rootPos[0]-localPos[0];
                            float dy3 = p.rootPos[1]-localPos[1];
                            float dz3 = p.rootPos[2]-localPos[2];
                            score = sqrtf(dx3*dx3+dy3*dy3+dz3*dz3);
                        }
                        break;
                    }
                    }

                    if (score < bestScore) {
                        bestScore = score;
                        best      = &p;
                        memcpy(bestPartW, partW, 12);
                    }
                }

                if (!best) return false;

                memcpy(outW, bestPartW, 12);
                outPtr  = best->ptr;
                outName = best->name;

                /* Cache for sticky */
                s_stickyPtr   = best->ptr;
                s_stickyName  = best->name;
                memcpy(s_stickyPartW, bestPartW, 12);
                s_stickyValid = true;

                return true;
            };

            /* ── Silent aim frame logic ────────────────────────── */
            if (saActive) {
                /* Build cursor reference point for FOV checks */
                float cursor2D[2] = { (float)cursor.x, (float)cursor.y };

                /* Find target */
                float      aimW[3]  = {};
                uintptr_t  aimPtr   = 0;
                std::string aimName;
                bool hasTarget = SilentGetBestTarget(cursor2D, aimW, aimPtr, aimName);

                /* Project to screen */
                float scr[2] = {};
                bool  onScreen = hasTarget && W2S(aimW, vm, sw, sh, scr)
                    && scr[0] >= 0.f && scr[0] <= sw
                    && scr[1] >= 0.f && scr[1] <= sh;

                if (onScreen) {
                    if (settings.method == AimbotMethod::Memory) {
                        /* ── Camera-write mode ────────────────── */
                        float camPos[3];
                        CamPosFromVM(vm, camPos);
                        if (!memAnglesInit) {
                            VMToAngles(vm, memYaw, memPitch);
                            memAnglesInit = true;
                        }
                        float tgtYaw, tgtPitch;
                        DirToAngles(camPos, aimW, tgtYaw, tgtPitch);
                        /* Instant snap for silent — no smoothing needed */
                        memYaw   = tgtYaw;
                        memPitch = tgtPitch;
                        Roblox::AdjustCameraAngles(memYaw, memPitch);
                    } else {
                        /* ── Mouse-warp mode ─────────────────── */
                        /* Save real cursor once per keypress */
                        if (!s_hasSaved) {
                            GetCursorPos(&s_savedCursor);
                            s_hasSaved    = true;
                            s_needRestore = false;
                        }
                        WarpAbs(scr[0], scr[1]);
                        s_needRestore = true;
                    }

                    /* Auto-click while on target */
                    if (settings.autoClick) {
                        static DWORD lastSAC = 0;
                        if (nowMs - lastSAC > (DWORD)(settings.autoClickDelay * 1000)) {
                            if (settings.autoClickHold) Click(true);
                            else { Click(true); Click(false); }
                            lastSAC = nowMs;
                            if (settings.hitmarker) {
                                hitmarkerVisible = true; hitmarkerTime = nowMs;
                                hitmarkerX = scr[0]; hitmarkerY = scr[1];
                            }
                        }
                    }
                }

                return;   /* don't run normal aimbot while silent aim is active */
            }
            else {
                /* Key released — restore cursor and clear sticky */
                if (s_needRestore && s_hasSaved) {
                    WarpAbs((float)s_savedCursor.x, (float)s_savedCursor.y);
                    s_needRestore = false;
                }
                s_hasSaved    = false;
                memAnglesInit = false;   /* allow re-init on next silent press */
                /* Clear sticky so next press does a fresh target scan */
                if (!settings.sticky) {
                    s_stickyPtr   = 0;
                    s_stickyName  = "";
                    s_stickyValid = false;
                }
            }
        }

        /* ══════════════════════════════════════════════════════════
           MOUSE AIMBOT
           ══════════════════════════════════════════════════════════ */
        float tgt2[2];
        if (!W2S(predicted, vm, sw, sh, tgt2)) { lockedPtr = 0; lockedName = ""; return; }

        float dx = tgt2[0] - (float)cursor.x;
        float dy = tgt2[1] - (float)cursor.y;
        float dist = Dist2D(0, 0, dx, dy);

        if (dist < 1.5f) {
            accumX = accumY = 0.0;
            if (settings.autoClick && dist <= settings.autoClickMinDist) {
                static DWORD lastAC = 0;
                if (nowMs - lastAC > (DWORD)(settings.autoClickDelay * 1000)) {
                    if (settings.autoClickHold) Click(true);
                    else { Click(true); Click(false); }
                    lastAC = nowMs;
                    if (settings.hitmarker) {
                        hitmarkerVisible = true; hitmarkerTime = nowMs;
                        hitmarkerX = tgt2[0]; hitmarkerY = tgt2[1];
                    }
                }
            }
            return;
        }

        float smooth = settings.rageMode ? 1.f : currentSmooth;
        if (settings.adaptiveSmooth) {
            float f = (dist > settings.fov) ? 1.f : (dist / settings.fov);
            smooth = 1.f + (smooth - 1.f) * f;
        }
        double moveX, moveY;
        SpringSmooth(dx, dy, smooth, dt, moveX, moveY);

        if (settings.jitter) {
            static double jt = 0.0;
            jt += dt * settings.jitterFreq * 6.2832;
            float jv = sinf((float)jt) * settings.jitterStr;
            moveX += jv;
            if (settings.jitterBothAxes) moveY += cosf((float)jt) * settings.jitterStr;
        }
        if (settings.recoilControl && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
            moveX += settings.recoilX; moveY += settings.recoilY;
        }

        MoveMouse(moveX, moveY);
    }

    const PlayerData* GetTarget() {
        if (!lockedPtr) return nullptr;
        for (const auto& p : Roblox::GetPlayers())
            if (p.ptr == lockedPtr && p.valid && p.health > 0) return &p;
        return nullptr;
    }

} // namespace Aimbot