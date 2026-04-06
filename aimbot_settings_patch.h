#pragma once
/*
 * aimbot_settings_patch.h
 * ────────────────────────────────────────────────────────────
 * Add these fields to the Aimbot::Settings struct in aimbot.h,
 * inside the struct body alongside the existing members.
 *
 * PASTE INTO aimbot.h  →  struct Settings { ... } block:
 *
 *   // ── Silent Aim ──────────────────────────────────────────
 *   float silentFov          = 150.f; // separate FOV for silent aim
 *   int   silentPriorityMode = 0;     // 0=crosshair 1=lowestHP 2=nearest
 *   int   silentPartMode     = 0;     // 0=head 1=torso 2=random 3=closest
 *   bool  silentVisCheck     = false; // visible-only for silent aim
 *
 *   // ── Hitbox Expand ───────────────────────────────────────
 *   bool  hitboxExpand    = false;
 *   float hitboxExpandPx  = 0.f;   // extra px added to FOV acceptance
 *
 *   // ── Wall Check ──────────────────────────────────────────
 *   bool  wallCheck       = false; // depth-heuristic occlusion filter
 */
