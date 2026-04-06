#pragma once
/*
 * chams.h  —  x9 mesh chams
 *
 * Two modes controlled by ESPSettings:
 *   chamsMode == 0  →  Flat chams: filled convex hull over the player bounding box.
 *                       Works even when hasLimbs is false.
 *   chamsMode == 1  →  Mesh chams: filled polygon that traces the actual skeleton
 *                       joints (head, neck, shoulders, elbows, hands, hips, knees,
 *                       feet) to produce a silhouette that molds to the player's
 *                       pose. Falls back to flat if hasLimbs is false.
 *
 * Both modes support:
 *   • chamsPulse        — sinusoidal alpha breathing
 *   • chamsRimLight     — brighter rim / outline drawn with AddPolyline
 *   • chamsWireframe    — outline only, no fill
 *   • chamsRainbow      — hue-cycled colour per player
 *
 * Include this header inside the ESP theme's namespace AFTER
 * WorldToScreen and HsvToImU32 are defined.
 *
 * Usage:
 *     DrawChams(draw, player, vm, sw, sh, s, hueTimer);
 */

#include <cmath>

/* ── New settings fields (add to ESPSettings in settings.h) ──────
 *   int   chamsMode     = 0;   // 0=flat 1=mesh
 *   bool  chamsRimLight = true;
 *   bool  chamsWireframe= false;
 *   bool  chamsRainbow  = false;
 * ---------------------------------------------------------------- */

/* ── Helpers ──────────────────────────────────────────────────── */
static inline ImVec2 ChamsLerp2(const float* a, const float* b, float t)
{
    return ImVec2(a[0]+(b[0]-a[0])*t, a[1]+(b[1]-a[1])*t);
}

static inline void ChamsLerp3(const float* a, const float* b, float t, float out[3])
{
    out[0]=a[0]+(b[0]-a[0])*t;
    out[1]=a[1]+(b[1]-a[1])*t;
    out[2]=a[2]+(b[2]-a[2])*t;
}

/* Project world→screen — self-contained so chams.h has no dependency
 * on the ESP theme's WorldToScreen function.
 * vm is a row-major 4x4 view-projection matrix (standard Roblox layout). */
static inline bool ChamsW2S(const float* w, const float* vm, float sw, float sh, float s2[2])
{
    float x = vm[0]*w[0] + vm[1]*w[1] + vm[2]*w[2]  + vm[3];
    float y = vm[4]*w[0] + vm[5]*w[1] + vm[6]*w[2]  + vm[7];
    float z = vm[8]*w[0] + vm[9]*w[1] + vm[10]*w[2] + vm[11];
    float w2= vm[12]*w[0]+ vm[13]*w[1]+ vm[14]*w[2] + vm[15];
    if (w2 < 0.001f) return false;
    float inv = 1.f / w2;
    s2[0] = sw * .5f + x * inv * sw * .5f;
    s2[1] = sh * .5f - y * inv * sh * .5f;
    (void)z;
    return s2[0] > -sw && s2[0] < sw*2.f && s2[1] > -sh && s2[1] < sh*2.f;
}

/* ── ImGui polygon fill helper ────────────────────────────────── */
/* AddConvexPolyFilled only works for convex shapes. For mesh chams
 * we build a concave outline and fill it using AddTriangleFilled    */

/* Ear-clipping fill for an arbitrary simple polygon (no holes).
 * Points must be in screen-space order (CW or CCW both ok).        */
/* Sort polygon points by angle from centroid so they always form a
 * non-self-intersecting convex/star-shaped outline, then fill via fan.
 * This prevents the "X cross" artifact when arm joints are at chest height. */
static void FillConcavePolygon(ImDrawList* dl, ImVec2* pts, int n, ImU32 col)
{
    if (n < 3) return;
    /* Centroid */
    float cx=0.f, cy=0.f;
    for(int i=0;i<n;i++){ cx+=pts[i].x; cy+=pts[i].y; }
    cx/=(float)n; cy/=(float)n;

    /* Sort by angle around centroid — simple insertion sort (n<=48, fast enough) */
    float angles[48];
    for(int i=0;i<n;i++)
        angles[i]=atan2f(pts[i].y-cy, pts[i].x-cx);
    for(int i=1;i<n;i++){
        ImVec2 kp=pts[i]; float ka=angles[i]; int j=i-1;
        while(j>=0&&angles[j]>ka){ pts[j+1]=pts[j]; angles[j+1]=angles[j]; j--; }
        pts[j+1]=kp; angles[j+1]=ka;
    }

    /* Fan fill from centroid */
    ImVec2 cen(cx,cy);
    for(int i=0;i<n;i++){
        int j=(i+1)%n;
        dl->AddTriangleFilled(cen, pts[i], pts[j], col);
    }
}

/* ── Flat chams ────────────────────────────────────────────────── */
static void DrawChamsFlat(ImDrawList* draw,
    const PlayerData& pl,
    const float* vm, float sw, float sh,
    ImU32 fillCol, ImU32 rimCol,
    bool wireframe, bool rimLight)
{
    float hs[2], fs[2];
    if (!ChamsW2S(pl.headPos, vm, sw, sh, hs)) return;
    if (!ChamsW2S(pl.rootPos, vm, sw, sh, fs)) return;

    float h   = fs[1] - hs[1];
    if (h < 2.f) return;
    float w2  = h * 0.30f;   /* half-width */
    float cx  = hs[0];

    /* 8-point rounded-rect shape (trapezoid body) */
    /* Shoulders wider than feet for a more body-like silhouette */
    float shW = w2 * 1.10f;  /* shoulder half-width */
    float hipY = hs[1] + h * 0.52f;
    float hipW = w2 * 0.85f;

    ImVec2 pts[8] = {
        /* head top — narrow */
        ImVec2(cx - w2*0.55f, hs[1] - h*0.08f),
        ImVec2(cx + w2*0.55f, hs[1] - h*0.08f),
        /* shoulders — wide */
        ImVec2(cx + shW,      hs[1] + h*0.20f),
        /* hip right */
        ImVec2(cx + hipW,     hipY),
        /* feet right */
        ImVec2(cx + w2*0.55f, fs[1]),
        /* feet left */
        ImVec2(cx - w2*0.55f, fs[1]),
        /* hip left */
        ImVec2(cx - hipW,     hipY),
        /* shoulder left */
        ImVec2(cx - shW,      hs[1] + h*0.20f),
    };

    if (!wireframe)
        FillConcavePolygon(draw, pts, 8, fillCol);
    if (rimLight || wireframe)
        draw->AddPolyline(pts, 8, rimCol, ImDrawFlags_Closed, wireframe ? 1.5f : 1.f);
}

/* ── Mesh chams ────────────────────────────────────────────────── */
/* Builds a 2D silhouette polygon tracing the skeleton joints.
 * The polygon visits points in a CCW order that outlines the body:
 *
 *    head top → left ear → left shoulder → left elbow → left hand
 *    → (back up) left hip → left knee → left foot
 *    → right foot → right knee → right hip
 *    → right hand → right elbow → right shoulder → right ear → head top
 *
 * Any joint that didn't project successfully is replaced by the
 * nearest valid neighbour so the polygon stays closed.             */
static void DrawChamsMesh(ImDrawList* draw,
    const PlayerData& pl,
    const float* vm, float sw, float sh,
    ImU32 fillCol, ImU32 rimCol,
    bool wireframe, bool rimLight)
{
    if (!pl.hasLimbs) {
        /* Fall back to flat if we have no limb data */
        DrawChamsFlat(draw, pl, vm, sw, sh, fillCol, rimCol, wireframe, rimLight);
        return;
    }

    const float* wH  = pl.headPos;
    const float* wR  = pl.rootPos;
    const float* wLH = pl.lHand;
    const float* wRH = pl.rHand;
    const float* wLF = pl.lFoot;
    const float* wRF = pl.rFoot;

    /* ── Derive intermediate skeleton joints ─────────────────── */
    float wChest[3], wNeck[3];
    float wLS[3], wRS[3];    /* shoulders */
    float wLE[3], wRE[3];    /* elbows    */
    float wLHip[3], wRHip[3];
    float wLK[3], wRK[3];   /* knees     */

    ChamsLerp3(wR, wH, 0.42f, wChest);
    ChamsLerp3(wH, wR, 0.28f, wNeck);

    /* Shoulders: branch off chest toward each hand */
    ChamsLerp3(wChest, wLH, 0.24f, wLS);
    ChamsLerp3(wChest, wRH, 0.24f, wRS);

    /* Elbows: 52% of shoulder→hand */
    ChamsLerp3(wLS, wLH, 0.52f, wLE);
    ChamsLerp3(wRS, wRH, 0.52f, wRE);

    /* Hips: branch off root toward each foot */
    ChamsLerp3(wR, wLF, 0.18f, wLHip);
    ChamsLerp3(wR, wRF, 0.18f, wRHip);

    /* Knees: 50% root→foot */
    ChamsLerp3(wR, wLF, 0.50f, wLK);
    ChamsLerp3(wR, wRF, 0.50f, wRK);

    /* ── Project all joints ───────────────────────────────────── */
    float sH[2],  sNeck[2],  sChest[2], sR[2];
    float sLS[2], sRS[2],    sLE[2],    sRE[2];
    float sLH[2], sRH[2];
    float sLHip[2], sRHip[2];
    float sLK[2],   sRK[2];
    float sLF[2],   sRF[2];

    bool vH     = ChamsW2S(wH,     vm,sw,sh,sH);
    bool vNeck  = ChamsW2S(wNeck,  vm,sw,sh,sNeck);
    bool vChest = ChamsW2S(wChest, vm,sw,sh,sChest);
    bool vR     = ChamsW2S(wR,     vm,sw,sh,sR);
    bool vLS    = ChamsW2S(wLS,    vm,sw,sh,sLS);
    bool vRS    = ChamsW2S(wRS,    vm,sw,sh,sRS);
    bool vLE    = ChamsW2S(wLE,    vm,sw,sh,sLE);
    bool vRE    = ChamsW2S(wRE,    vm,sw,sh,sRE);
    bool vLH    = ChamsW2S(wLH,    vm,sw,sh,sLH);
    bool vRH    = ChamsW2S(wRH,    vm,sw,sh,sRH);
    bool vLHip  = ChamsW2S(wLHip,  vm,sw,sh,sLHip);
    bool vRHip  = ChamsW2S(wRHip,  vm,sw,sh,sRHip);
    bool vLK    = ChamsW2S(wLK,    vm,sw,sh,sLK);
    bool vRK    = ChamsW2S(wRK,    vm,sw,sh,sRK);
    bool vLF    = ChamsW2S(wLF,    vm,sw,sh,sLF);
    bool vRF    = ChamsW2S(wRF,    vm,sw,sh,sRF);

    /* Require at least head + root to be visible */
    if (!vH || !vR) return;

    /* Substitute any missing joint with the nearest visible neighbour */
    auto Sub = [](float* dst, bool& valid, const float* fallback, bool fbValid) {
        if (!valid && fbValid) {
            dst[0]=fallback[0]; dst[1]=fallback[1]; valid=true;
        }
    };
    Sub(sNeck,  vNeck,  sH,     vH);
    Sub(sChest, vChest, sR,     vR);
    Sub(sLS,    vLS,    sChest, vChest);
    Sub(sRS,    vRS,    sChest, vChest);
    Sub(sLE,    vLE,    sLS,    vLS);
    Sub(sRE,    vRE,    sRS,    vRS);
    Sub(sLH,    vLH,    sLE,    vLE);
    Sub(sRH,    vRH,    sRE,    vRE);
    Sub(sLHip,  vLHip,  sR,     vR);
    Sub(sRHip,  vRHip,  sR,     vR);
    Sub(sLK,    vLK,    sLHip,  vLHip);
    Sub(sRK,    vRK,    sRHip,  vRHip);
    Sub(sLF,    vLF,    sLK,    vLK);
    Sub(sRF,    vRF,    sRK,    vRK);

    /* ── Sizing ──────────────────────────────────────────────────
     * Generate a CLOUD of bounding points around every body part.
     * FillConcavePolygon will sort them by angle from the centroid
     * to form a correct non-self-intersecting convex hull,
     * eliminating the "X cross" artifact entirely.               */
    float playerH = sR[1] - sH[1];
    if (playerH < 4.f) return;

    float bw  = playerH * 0.15f;   /* arm/leg half-thickness    */
    float hw  = playerH * 0.13f;   /* head radius               */
    float tw  = playerH * 0.20f;   /* torso half-width          */
    float fw  = playerH * 0.10f;   /* foot half-width           */

    ImVec2 pts[48];
    int n = 0;
    auto Pt = [&](float x, float y) {
        if (n < 48) pts[n++] = ImVec2(x, y);
    };

    /* Head — circle of 10 points */
    for (int i = 0; i < 10; i++) {
        float a = (float)i / 10.f * 6.2832f;
        Pt(sH[0] + cosf(a)*hw, sH[1] + sinf(a)*hw*1.05f);
    }

    /* Torso box — 4 corners centred on chest/hips */
    Pt(sChest[0]-tw,  sChest[1]);
    Pt(sChest[0]+tw,  sChest[1]);
    Pt(sR[0]-tw,      sR[1]);
    Pt(sR[0]+tw,      sR[1]);

    /* Arms — 4 points per arm (top/bottom of upper/lower arm) */
    Pt(sLS[0]-bw, sLS[1]);  Pt(sLS[0]+bw, sLS[1]);
    Pt(sLE[0]-bw, sLE[1]);  Pt(sLE[0]+bw, sLE[1]);
    Pt(sLH[0]-bw, sLH[1]);  Pt(sLH[0]+bw, sLH[1]+bw);

    Pt(sRS[0]-bw, sRS[1]);  Pt(sRS[0]+bw, sRS[1]);
    Pt(sRE[0]-bw, sRE[1]);  Pt(sRE[0]+bw, sRE[1]);
    Pt(sRH[0]-bw, sRH[1]);  Pt(sRH[0]+bw, sRH[1]+bw);

    /* Legs — 4 points per leg */
    Pt(sLHip[0]-bw, sLHip[1]);  Pt(sLHip[0]+bw, sLHip[1]);
    Pt(sLK[0]-bw,   sLK[1]);    Pt(sLK[0]+bw,   sLK[1]);
    Pt(sLF[0]-fw,   sLF[1]);    Pt(sLF[0]+fw,   sLF[1]+fw);

    Pt(sRHip[0]-bw, sRHip[1]);  Pt(sRHip[0]+bw, sRHip[1]);
    Pt(sRK[0]-bw,   sRK[1]);    Pt(sRK[0]+bw,   sRK[1]);
    Pt(sRF[0]-fw,   sRF[1]);    Pt(sRF[0]+fw,   sRF[1]+fw);

    if (n < 4) return;

    if (!wireframe)
        FillConcavePolygon(draw, pts, n, fillCol);

    if (rimLight || wireframe)
        draw->AddPolyline(pts, n, rimCol, ImDrawFlags_Closed,
            wireframe ? 1.8f : 1.1f);
}

/* ── Public entry point ───────────────────────────────────────── */
/* Call once per player inside the ESP render loop.
 * Reads ESPSettings fields:
 *   chams, chamsMode, chamsCol[3], chamsAlpha, chamsPulse,
 *   chamsRimLight, chamsWireframe, chamsRainbow               */
static void DrawChams(ImDrawList* draw,
    const PlayerData& pl,
    const float* vm, float sw, float sh,
    const ESPSettings& s,
    float hueTimer)
{
    if (!s.chams) return;
    if (pl.isLocalPlayer || !pl.valid || pl.health <= 0.1f) return;

    static float chPulseT = 0.f;
    chPulseT += 0.016f;

    float alpha = s.chamsPulse
        ? s.chamsAlpha * ((sinf(chPulseT * 2.2f) + 1.f) * 0.5f * 0.75f + 0.25f)
        : s.chamsAlpha;
    alpha = alpha > 1.f ? 1.f : (alpha < 0.f ? 0.f : alpha);

    ImU32 fillCol, rimCol;

    /* Colour source */
    bool rainbow = s.chamsRainbow;
    if (rainbow) {
        /* Per-player hue offset using ptr so each player has a unique colour */
        float hue = fmodf(hueTimer + (float)(pl.ptr & 0xFF) / 255.f, 1.f);
        /* Inline HSV→RGB so chams.h doesn't depend on theme's HsvToImU32 */
        {
            float h6=hue*6.f, s2=1.f, v=1.f;
            int   hi=(int)h6%6;
            float ff=h6-(int)h6;
            float p=v*(1.f-s2), q=v*(1.f-s2*ff), t2=v*(1.f-s2*(1.f-ff));
            float r2,g2,b2;
            switch(hi){
                case 0:r2=v;g2=t2;b2=p;break;
                case 1:r2=q;g2=v;b2=p;break;
                case 2:r2=p;g2=v;b2=t2;break;
                case 3:r2=p;g2=q;b2=v;break;
                case 4:r2=t2;g2=p;b2=v;break;
                default:r2=v;g2=p;b2=q;break;
            }
            fillCol=IM_COL32((int)(r2*255),(int)(g2*255),(int)(b2*255),(int)(alpha*255.f));
        }
    } else {
        fillCol = IM_COL32(
            (int)(s.chamsCol[0] * 255.f),
            (int)(s.chamsCol[1] * 255.f),
            (int)(s.chamsCol[2] * 255.f),
            (int)(alpha * 255.f));
    }

    /* Rim: same hue but brighter and semi-opaque */
    float rimR = (std::min)(1.f, s.chamsCol[0] * 1.4f);
    float rimG = (std::min)(1.f, s.chamsCol[1] * 1.4f);
    float rimB = (std::min)(1.f, s.chamsCol[2] * 1.4f);
    if (rainbow) {
        rimCol = (fillCol & 0x00FFFFFF) | (ImU32(200) << 24);
    } else {
        rimCol = IM_COL32((int)(rimR*255),(int)(rimG*255),(int)(rimB*255), 200);
    }

    /* Wireframe mode: fill is fully transparent */
    if (s.chamsWireframe) fillCol = fillCol & 0x00FFFFFF;

    /* Dispatch */
    if (s.chamsMode == 1) {
        DrawChamsMesh(draw, pl, vm, sw, sh, fillCol, rimCol,
            s.chamsWireframe, s.chamsRimLight);
    } else {
        DrawChamsFlat(draw, pl, vm, sw, sh, fillCol, rimCol,
            s.chamsWireframe, s.chamsRimLight);
    }
}
