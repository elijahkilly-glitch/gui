/* ═══════════════════════════════════════════════════════════════
   ESP THEME: NORMAL  —  clean dark boxes with glow
   Used when g_cfg.uiTheme == 1 (Industrial) or 2 (Original)
   Default theme — loads automatically for non-Neko themes.
   ═══════════════════════════════════════════════════════════════ */
namespace EspNormal {

ESPSettings& Settings() { return g_cfg.esp; }

/* ── World to Screen using 4x4 ViewProjection matrix ─────────── */

static bool WorldToScreen(const float pos[3], const float* vm,
    float screenW, float screenH, float out[2]) {
    float x = pos[0] * vm[0] + pos[1] * vm[1] + pos[2] * vm[2] + vm[3];
    float y = pos[0] * vm[4] + pos[1] * vm[5] + pos[2] * vm[6] + vm[7];
    float w = pos[0] * vm[12] + pos[1] * vm[13] + pos[2] * vm[14] + vm[15];
    if (w < 0.1f) return false;
    float inv = 1.0f / w;
    out[0] = (screenW * 0.5f) + (x * inv * screenW * 0.5f);
    out[1] = (screenH * 0.5f) - (y * inv * screenH * 0.5f);
    return (out[0] > -1000.0f && out[0] < screenW + 1000.0f &&
        out[1] > -1000.0f && out[1] < screenH + 1000.0f);
}

static float Distance3D(const float a[3], const float b[3]) {
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static ImU32 HealthColor(float hp, float maxHp) {
    if (maxHp <= 0.0f) return IM_COL32(255, 255, 255, 255);
    float pct = hp / maxHp;
    if (pct < 0) pct = 0; if (pct > 1) pct = 1;
    return IM_COL32((int)((1.0f - pct) * 255), (int)(pct * 255), 0, 255);
}

static ImU32 HsvToImU32(float hue) {
    int hi = (int)(hue * 6.0f) % 6;
    float f = hue * 6.0f - (int)(hue * 6.0f), q = 1.0f - f;
    float r, g, b;
    switch (hi) {
    case 0:r = 1;g = f;b = 0;break; case 1:r = q;g = 1;b = 0;break;
    case 2:r = 0;g = 1;b = f;break; case 3:r = 0;g = q;b = 1;break;
    case 4:r = f;g = 0;b = 1;break; default:r = 1;g = 0;b = q;break;
    }
    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
}

/* ── Teleport / stretch prevention ───────────────────────────────
 * Tracks each player's world position from the previous frame.
 * If a player jumps more than kTeleportThresh studs in one frame
 * we skip drawing them for that frame so the box never stretches
 * between the old and new position.                               */
struct PlayerPosCache {
    float pos[3] = {};
    bool  valid  = false;
};
static std::unordered_map<uintptr_t, PlayerPosCache> s_posCache;

// Maximum studs a player can travel in one frame before it's
// treated as a teleport.  16 studs/s walk, 2x sprint ~32, with
// a 30Hz scanner update = up to ~1.07 studs per scan tick at sprint.
// Set to 60 so only actual teleports (not fast legit movement) skip.
static constexpr float kTeleportThresh = 60.0f;

/* ── Render ──────────────────────────────────────────────────── */

void Render() {
    ESPSettings& s = g_cfg.esp;   /* single alias – no split refs */
    if (!s.enabled) return;

    const float* vm = Roblox::GetViewMatrix();
    const auto& players = Roblox::GetPlayers();
    float sw = (float)Roblox::GetScreenWidth();
    float sh = (float)Roblox::GetScreenHeight();
    if (sw <= 0.0f || sh <= 0.0f) return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    if (!draw) return;

    float localPos[3] = {}; uintptr_t localID = 0;
    for (auto& p : players) {
        if (p.isLocalPlayer) { memcpy(localPos, p.rootPos, 12); localID = p.ptr; break; }
    }

    static float hueTimer = 0.0f;
    hueTimer += 0.001f;

    /* ════════════════════════════════════════════════════════════
       Per-player rendering
       ════════════════════════════════════════════════════════════ */
    for (auto& player : players) {
        if (player.isLocalPlayer || player.ptr == localID) continue;
        if (!player.valid) continue;
        if (!s.showDead && player.health <= 0.1f) continue;
        if (player.rootPos[0] == 0 && player.rootPos[1] == 0 && player.rootPos[2] == 0) continue;
        if (fabsf(player.rootPos[0]) > 50000 || fabsf(player.rootPos[1]) > 50000 || fabsf(player.rootPos[2]) > 50000) continue;

        float dist = Distance3D(localPos, player.rootPos);
        if (dist > s.maxDistance) continue;

        /* ── Teleport / stretch guard ──────────────────────────────
         * Compare this frame's position against last frame's.
         * On a large jump: update the cache and skip for one frame.
         * The box will appear correctly on the very next frame.    */
        {
            auto& cache = s_posCache[player.ptr];
            float dx = player.rootPos[0] - cache.pos[0];
            float dy = player.rootPos[1] - cache.pos[1];
            float dz = player.rootPos[2] - cache.pos[2];
            float distSq = dx * dx + dy * dy + dz * dz;
            bool teleported = cache.valid && (distSq > kTeleportThresh * kTeleportThresh);
            cache.pos[0] = player.rootPos[0];
            cache.pos[1] = player.rootPos[1];
            cache.pos[2] = player.rootPos[2];
            cache.valid  = true;
            if (teleported) continue;
        }

        /* ── Color ─────────────────────────────────────────────── */
        ImU32 color;
        if (FriendlyList::Has(player.name)) {
            /* Friendlies always render in solid blue regardless of other settings */
            color = IM_COL32(60, 140, 255, 255);
        }
        else if (s.rainbowBox) {
            color = HsvToImU32(fmodf(hueTimer + (float)(player.ptr & 0xFF) / 255.0f, 1.0f));
        }
        else if (s.dynBoxColor) {
            color = HealthColor(player.health, player.maxHealth);
        }
        else if (s.visCheck && dist < s.visRange) {
            color = IM_COL32((int)(s.visCol[0] * 255), (int)(s.visCol[1] * 255), (int)(s.visCol[2] * 255), 255);
        }
        else if (s.lowHpFlash && player.health < s.lowHpThresh) {
            float flash = (sinf((float)GetTickCount() * 0.01f) + 1.0f) * 0.5f;
            color = IM_COL32(255, (int)(flash * 80), (int)(flash * 80), 255);
        }
        else {
            color = IM_COL32((int)(s.boxCol[0] * 255), (int)(s.boxCol[1] * 255), (int)(s.boxCol[2] * 255), 255);
        }

        /* ── Box geometry — fixed world-space size ─────────────────
         * Project only the root position to get screen centre + depth.
         * Use a constant world-space player height (5.5 studs covers
         * every R6 / R15 rig) scaled by perspective W so the box
         * stays a consistent size regardless of limb read noise.     */
        float boxX, boxY, boxW, boxH;
        float headScreen[2], feetScreen[2];

        {
            /* Project root to screen to get the perspective scale */
            float rootScr[2];
            if (!WorldToScreen(player.rootPos, vm, sw, sh, rootScr)) continue;

            /* Compute perspective W for the root position directly —
             * same calculation as WorldToScreen row 3.               */
            float perspW = player.rootPos[0]*vm[12]
                         + player.rootPos[1]*vm[13]
                         + player.rootPos[2]*vm[14]
                         + vm[15];
            if (perspW < 0.1f) continue;

            /* Fixed world-space dimensions (studs) — +20% for visibility:
             *   height = 7.2  (was 6.0 * 1.2)
             *   width  = height * 0.6                                   */
            constexpr float kPlayerH = 7.2f;
            constexpr float kPlayerW = kPlayerH * 0.6f;

            /* Convert world units → pixels via perspective divide.
             * halfPx = (worldUnits / perspW) * (screenHalf)          */
            float halfH = (kPlayerH * 0.5f / perspW) * (sh * 0.5f);
            float halfW = (kPlayerW * 0.5f / perspW) * (sh * 0.5f);

            /* Root sits at hip level — shift centre up so box covers full body */
            float centreY = rootScr[1] - halfH * 0.20f + halfH * 0.34f;

            boxX = rootScr[0] - halfW;
            boxY = centreY    - halfH;
            boxW = halfW * 2.f;
            boxH = halfH * 2.f;

            headScreen[0] = rootScr[0]; headScreen[1] = boxY;
            feetScreen[0] = rootScr[0]; feetScreen[1] = boxY + boxH;
        }
        if (boxW > sw * 0.9f || boxH > sh * 0.9f || boxW < 0.1f || boxH < 0.1f) continue;
        if (boxX<-sw || boxX>sw * 2 || boxY<-sh || boxY>sh * 2) continue;

        float bx = boxX, by = boxY, bw = boxW, bh = boxH, thick = s.boxThickness;

        /* ── Bounding box — modern rounded style ──────────────── */
        if (s.showBox) {
            /* Rounding: 0=rounded rect(6px)  1=corner brackets  2=filled */
            float boxR = (s.boxType == 0) ? 6.f : 0.f;

            if (s.filledBox) {
                ImU32 fc = (color & 0x00FFFFFF) | ((int)(s.fillAlpha * 255) << 24);
                draw->AddRectFilled(ImVec2(bx,by),ImVec2(bx+bw,by+bh),fc,boxR);
            }

            if (s.boxType == 0) {
                /* ── Rounded rect box ─────────────────────────── */
                ImU32 c = color;
                /* Subtle dark outline underneath for contrast */
                if (s.boxOutline)
                    draw->AddRect(ImVec2(bx-1,by-1),ImVec2(bx+bw+1,by+bh+1),
                        IM_COL32(0,0,0,160), boxR+1.f, 0, thick+1.2f);
                /* Glow bloom */
                if (g_cfg.espBoxGlow) {
                    float gi = g_cfg.espBoxGlowIntens;
                    auto GA=[&](ImU32 b,float a)->ImU32{return (b&0x00FFFFFF)|((ImU32)(a*gi)<<24);};
                    float gs=(std::min)(bh*0.035f,3.5f)*gi;
                    if (gs>0.3f) {
                        draw->AddRect(ImVec2(bx-gs*2,by-gs*2),ImVec2(bx+bw+gs*2,by+bh+gs*2),GA(c,0x0E),boxR+gs*2,0,thick+gs*2);
                        draw->AddRect(ImVec2(bx-gs,  by-gs  ),ImVec2(bx+bw+gs,  by+bh+gs  ),GA(c,0x30),boxR+gs,  0,thick+gs);
                        draw->AddRect(ImVec2(bx-gs*.5f,by-gs*.5f),ImVec2(bx+bw+gs*.5f,by+bh+gs*.5f),GA(c,0x70),boxR+gs*.5f,0,thick+gs*.3f);
                    }
                }
                draw->AddRect(ImVec2(bx,by),ImVec2(bx+bw,by+bh),color,boxR,0,thick);
            }
            else if (s.boxType == 1) {
                /* ── Corner bracket box ───────────────────────── */
                float clx = bw * s.cornerLen, cly = bh * s.cornerLen;
                auto C = [&](float ox2, float oy2, float lx2, float ly2) {
                    if (s.boxOutline) {
                        draw->AddLine(ImVec2(ox2-1,oy2),ImVec2(ox2+lx2-1,oy2),IM_COL32(0,0,0,160),thick+1.2f);
                        draw->AddLine(ImVec2(ox2,oy2-1),ImVec2(ox2,oy2+ly2-1),IM_COL32(0,0,0,160),thick+1.2f);
                    }
                    if (g_cfg.espBoxGlow) {
                        float gi=g_cfg.espBoxGlowIntens;
                        auto GA=[&](ImU32 b,float a)->ImU32{return (b&0x00FFFFFF)|((ImU32)(a*gi)<<24);};
                        float gs=(std::min)(bh*0.035f,3.5f)*gi;
                        if (gs>0.3f) {
                            draw->AddLine(ImVec2(ox2,oy2),ImVec2(ox2+lx2,oy2),GA(color,0x18),thick+gs*1.5f);
                            draw->AddLine(ImVec2(ox2,oy2),ImVec2(ox2,oy2+ly2),GA(color,0x18),thick+gs*1.5f);
                            draw->AddLine(ImVec2(ox2,oy2),ImVec2(ox2+lx2,oy2),GA(color,0x55),thick+gs*.5f);
                            draw->AddLine(ImVec2(ox2,oy2),ImVec2(ox2,oy2+ly2),GA(color,0x55),thick+gs*.5f);
                        }
                    }
                    draw->AddLine(ImVec2(ox2,oy2),ImVec2(ox2+lx2,oy2),color,thick);
                    draw->AddLine(ImVec2(ox2,oy2),ImVec2(ox2,oy2+ly2),color,thick);
                };
                C(bx,by,clx,cly); C(bx+bw,by,-clx,cly);
                C(bx,by+bh,clx,-cly); C(bx+bw,by+bh,-clx,-cly);
            }
            else {
                /* ── Filled box ───────────────────────────────── */
                draw->AddRectFilled(ImVec2(bx,by),ImVec2(bx+bw,by+bh),
                    (color&0x00FFFFFF)|(0x44<<24),4.f);
                draw->AddRect(ImVec2(bx,by),ImVec2(bx+bw,by+bh),color,4.f,0,thick);
            }
        }

        /* ── Snapline ──────────────────────────────────────────── */
        if (s.showSnapline) {
            ImU32 sc = IM_COL32((int)(s.snaplineCol[0] * 255), (int)(s.snaplineCol[1] * 255), (int)(s.snaplineCol[2] * 255), 200);
            float oy = (s.snaplineStyle == 1) ? sh * 0.5f : (s.snaplineStyle == 2) ? 0.0f : sh;
            draw->AddLine(ImVec2(sw * 0.5f, oy), ImVec2(headScreen[0], feetScreen[1]), sc, 1.0f);
        }

        /* ── Name — pill badge ─────────────────────────────────── */
        if (s.showNames && !player.name.empty()) {
            const char* nameStr = (s.showDisplayName && !player.displayName.empty())
                                  ? player.displayName.c_str()
                                  : player.name.c_str();
            ImVec2 tsz = ImGui::CalcTextSize(nameStr);
            float tx = headScreen[0] - tsz.x * 0.5f;
            float ty = boxY - tsz.y - 7.f;
            float pr = 5.f;  /* pill horizontal padding */
            float pv = 2.f;  /* pill vertical padding   */
            /* Pill bg */
            ImU32 pillBg = s.nameBg
                ? IM_COL32(0,0,0,(int)(s.nameBgAlpha*255))
                : IM_COL32(0,0,0,130);
            draw->AddRectFilled(ImVec2(tx-pr,ty-pv),ImVec2(tx+tsz.x+pr,ty+tsz.y+pv),
                pillBg, tsz.y*.5f+pv);
            /* Pill border in box colour */
            draw->AddRect(ImVec2(tx-pr,ty-pv),ImVec2(tx+tsz.x+pr,ty+tsz.y+pv),
                (color&0x00FFFFFF)|(0x70u<<24), tsz.y*.5f+pv, 0, 0.8f);
            /* Text */
            draw->AddText(ImVec2(tx,ty),
                IM_COL32((int)(s.nameCol[0]*255),(int)(s.nameCol[1]*255),(int)(s.nameCol[2]*255),235),
                nameStr);
        }

        /* ── Health bar — slim rounded pill ────────────────────── */
        if (s.showHealth) {
            float pct = (player.maxHealth > 0.1f) ? (player.health / player.maxHealth) : 0;
            if (pct < 0)pct = 0; if (pct > 1)pct = 1;
            float brX, brY, brW, brH;
            if      (s.healthBarSide == 0) { brX=bx-5.f;   brY=by;    brW=3.f; brH=bh; }
            else if (s.healthBarSide == 1) { brX=bx+bw+2.f;brY=by;    brW=3.f; brH=bh; }
            else if (s.healthBarSide == 2) { brX=bx;       brY=by-5.f;brW=bw;  brH=3.f;}
            else                           { brX=bx;       brY=by+bh+2.f;brW=bw;brH=3.f;}
            float hbR = (brW < brH) ? brW*.5f : brH*.5f;
            /* Track */
            draw->AddRectFilled(ImVec2(brX,brY),ImVec2(brX+brW,brY+brH),
                IM_COL32(0,0,0,160), hbR);
            ImU32 hc = HealthColor(player.health, player.maxHealth);
            /* Fill */
            if (pct > 0.f) {
                if (s.healthBarSide < 2) {
                    float fh=brH*pct;
                    draw->AddRectFilled(ImVec2(brX,brY+brH-fh),ImVec2(brX+brW,brY+brH),hc,hbR);
                } else {
                    draw->AddRectFilled(ImVec2(brX,brY),ImVec2(brX+brW*pct,brY+brH),hc,hbR);
                }
            }
        }

        /* ── HP number ─────────────────────────────────────────── */
        if (s.showHealthNum && player.maxHealth > 0) {
            char buf[32]; snprintf(buf, sizeof(buf), "%.0f/%.0f", player.health, player.maxHealth);
            ImVec2 sz = ImGui::CalcTextSize(buf);
            float tx = bx + bw + 4, ty = by + (bh - sz.y) * 0.5f;
            draw->AddText(ImVec2(tx + 1, ty + 1), IM_COL32(0, 0, 0, 180), buf);
            draw->AddText(ImVec2(tx, ty), HealthColor(player.health, player.maxHealth), buf);
        }

        /* ── Distance — small pill below box ───────────────────── */
        if (s.showDistance) {
            char buf[32]; snprintf(buf, sizeof(buf), "%.0fm", dist);
            ImVec2 tsz = ImGui::CalcTextSize(buf);
            float tx = headScreen[0] - tsz.x*.5f - 4.f;
            float ty = boxY + boxH + 3.f;
            draw->AddRectFilled(ImVec2(tx-1,ty-1),ImVec2(tx+tsz.x+8,ty+tsz.y+1),
                IM_COL32(0,0,0,120), tsz.y*.5f);
            draw->AddText(ImVec2(tx+3.f,ty),IM_COL32(180,180,185,220),buf);
        }

        /* ── Skeleton ──────────────────────────────────────────── */
        if (s.showSkeleton && player.hasLimbs) {
            const float* wH  = player.headPos;
            const float* wR  = player.rootPos;
            const float* wLH = player.lHand;
            const float* wRH = player.rHand;
            const float* wLF = player.lFoot;
            const float* wRF = player.rFoot;

            /* Estimate intermediate joints by lerping between known positions */
            auto Lerp3 = [](const float* a, const float* b, float t, float out[3]) {
                out[0]=a[0]+(b[0]-a[0])*t; out[1]=a[1]+(b[1]-a[1])*t; out[2]=a[2]+(b[2]-a[2])*t;
            };
            float wNeck[3],wLS[3],wRS[3],wLE[3],wRE[3],wHips[3],wLK[3],wRK[3];
            /* Root = pelvis/hip in Roblox. Head is above.
             * Chest = 40% of the way from root UP toward head.
             * Shoulders branch OFF the chest, not from root.
             * We fake shoulder width by pushing them outward using
             * the hand direction but only 20% of the way.           */
            float wChest[3];
            Lerp3(wR, wH, 0.40f, wChest);   /* chest: 40% root→head */
            Lerp3(wH, wR, 0.30f, wNeck);    /* neck:  30% head→root */
            /* Shoulders: start from chest, go 20% toward each hand */
            Lerp3(wChest, wLH, 0.22f, wLS);
            Lerp3(wChest, wRH, 0.22f, wRS);
            /* Elbows: 55% from shoulder toward hand */
            Lerp3(wLS, wLH, 0.55f, wLE);
            Lerp3(wRS, wRH, 0.55f, wRE);
            /* Hips: root IS the hip, split L/R slightly */
            Lerp3(wR, wLF, 0.15f, wHips);   /* hip centre (slight L bias) */
            /* Knees: 52% from root toward foot */
            Lerp3(wR, wLF, 0.52f, wLK);
            Lerp3(wR, wRF, 0.52f, wRK);

            float sH[2],sNeck[2],sChest[2],sR[2],sLS[2],sRS[2],sLE[2],sRE[2];
            float sLH[2],sRH[2],sHips[2],sLK[2],sRK[2],sLF[2],sRF[2];
            bool vH    =WorldToScreen(wH,     vm,sw,sh,sH);
            bool vNeck =WorldToScreen(wNeck,  vm,sw,sh,sNeck);
            bool vChest=WorldToScreen(wChest, vm,sw,sh,sChest);
            bool vR    =WorldToScreen(wR,     vm,sw,sh,sR);
            bool vLS   =WorldToScreen(wLS,    vm,sw,sh,sLS);
            bool vRS   =WorldToScreen(wRS,    vm,sw,sh,sRS);
            bool vLE   =WorldToScreen(wLE,    vm,sw,sh,sLE);
            bool vRE   =WorldToScreen(wRE,    vm,sw,sh,sRE);
            bool vLH   =WorldToScreen(wLH,    vm,sw,sh,sLH);
            bool vRH   =WorldToScreen(wRH,    vm,sw,sh,sRH);
            bool vHips =WorldToScreen(wHips,  vm,sw,sh,sHips);
            bool vLK   =WorldToScreen(wLK,    vm,sw,sh,sLK);
            bool vRK   =WorldToScreen(wRK,    vm,sw,sh,sRK);
            bool vLF   =WorldToScreen(wLF,    vm,sw,sh,sLF);
            bool vRF   =WorldToScreen(wRF,    vm,sw,sh,sRF);

            ImU32 bC = s.skeletonRainbow ? HsvToImU32(fmodf(hueTimer,1.f)) :
                IM_COL32((int)(s.skeletonCol[0]*255),(int)(s.skeletonCol[1]*255),(int)(s.skeletonCol[2]*255),220);
            ImU32 bO = IM_COL32(0,0,0,130); float bT = s.skeletonThick;

            auto Bone = [&](const float* a, bool va, const float* b, bool vb) {
                if (!va||!vb) return;
                draw->AddLine(ImVec2(a[0]+1,a[1]+1),ImVec2(b[0]+1,b[1]+1),bO,bT+1.f);
                draw->AddLine(ImVec2(a[0],a[1]),ImVec2(b[0],b[1]),bC,bT);
            };
            auto Joint = [&](const float* s2, bool v) {
                if (!v) return;
                draw->AddCircleFilled(ImVec2(s2[0],s2[1]),2.8f,bO);
                draw->AddCircleFilled(ImVec2(s2[0],s2[1]),1.8f,bC);
            };

            /* Spine: feet→hips→root→chest→neck→head */
            Bone(sH,vH,         sNeck, vNeck);
            Bone(sNeck,vNeck,   sChest,vChest);
            Bone(sChest,vChest, sR,    vR);
            Bone(sR,vR,         sHips, vHips);
            /* Arms: chest→shoulder→elbow→hand */
            Bone(sChest,vChest, sLS,vLS);
            Bone(sChest,vChest, sRS,vRS);
            Bone(sLS,vLS,       sLE,vLE);
            Bone(sRS,vRS,       sRE,vRE);
            Bone(sLE,vLE,       sLH,vLH);
            Bone(sRE,vRE,       sRH,vRH);
            /* Legs: root→knee→foot (root = pelvis) */
            Bone(sR,vR,   sLK,vLK);
            Bone(sR,vR,   sRK,vRK);
            Bone(sLK,vLK, sLF,vLF);
            Bone(sRK,vRK, sRF,vRF);

            Joint(sH,vH); Joint(sNeck,vNeck); Joint(sChest,vChest); Joint(sR,vR);
            Joint(sLS,vLS); Joint(sRS,vRS); Joint(sLE,vLE); Joint(sRE,vRE);
            Joint(sLH,vLH); Joint(sRH,vRH);
            Joint(sLK,vLK); Joint(sRK,vRK); Joint(sLF,vLF); Joint(sRF,vRF);
        }

        /* ── Head dot ──────────────────────────────────────────── */
        if (s.showHeadDot) {
            float hs[2];
            if (WorldToScreen(player.headPos, vm, sw, sh, hs)) {
                float r = s.headDotRadius;
                ImU32 c = color;
                if (g_cfg.espBoxGlow) {
                    float gi = g_cfg.espBoxGlowIntens;
                    auto GA = [&](ImU32 b, float a) -> ImU32 { return (b&0x00FFFFFF)|((ImU32)(a*gi)<<24); };
                    float gs = (std::min)(r * 0.5f, 4.f) * gi;
                    if (gs > 0.2f) {
                        draw->AddCircleFilled(ImVec2(hs[0],hs[1]), r+gs*1.2f, GA(c,0x12));
                        draw->AddCircleFilled(ImVec2(hs[0],hs[1]), r+gs*0.8f, GA(c,0x30));
                        draw->AddCircleFilled(ImVec2(hs[0],hs[1]), r+gs*0.4f, GA(c,0x60));
                    }
                }
                draw->AddCircleFilled(ImVec2(hs[0], hs[1]), r + 1.0f, IM_COL32(0, 0, 0, 200));
                draw->AddCircleFilled(ImVec2(hs[0], hs[1]), r, color);
            }
        }
    } /* end player loop */

    /* ── Purge stale cache entries ─────────────────────────────────
     * Remove any cached ptr that no longer exists in the player list
     * so memory doesn't grow when players leave.                    */
    {
        std::unordered_map<uintptr_t, bool> alive;
        for (auto& pl : players) if (pl.valid) alive[pl.ptr] = true;
        for (auto it = s_posCache.begin(); it != s_posCache.end(); ) {
            it = alive.count(it->first) ? std::next(it) : s_posCache.erase(it);
        }
    }

    /* ── Offscreen arrows ──────────────────────────────────────── */
    if (s.offscreenArrows) {
        ImU32 aC = IM_COL32((int)(s.arrowCol[0] * 255), (int)(s.arrowCol[1] * 255), (int)(s.arrowCol[2] * 255), 220);
        float margin = s.offscreenMargin, hcx = sw * 0.5f, hcy = sh * 0.5f;
        for (auto& pl : players) {
            if (pl.isLocalPlayer || !pl.valid || pl.health <= 0.1f) continue;
            float scr[2]; WorldToScreen(pl.rootPos, vm, sw, sh, scr);
            if (scr[0] >= 0 && scr[0] <= sw && scr[1] >= 0 && scr[1] <= sh) continue;
            float dx = scr[0] - hcx, dy = scr[1] - hcy, len = sqrtf(dx * dx + dy * dy);
            if (len < 0.1f) continue;
            float nx = dx / len, ny = dy / len;
            float ax = hcx + nx * (sw * 0.5f - margin), ay = hcy + ny * (sh * 0.5f - margin);
            ax = (ax < margin) ? margin : (ax > sw - margin) ? sw - margin : ax;
            ay = (ay < margin) ? margin : (ay > sh - margin) ? sh - margin : ay;
            float sz = s.arrowSize, px = -ny, py = nx;
            draw->AddTriangleFilled(ImVec2(ax + nx * sz, ay + ny * sz),
                ImVec2(ax + px * (sz * 0.5f), ay + py * (sz * 0.5f)),
                ImVec2(ax - px * (sz * 0.5f), ay - py * (sz * 0.5f)), aC);
        }
    }

    /* ── Aimbot FOV circle ─────────────────────────────────────── */
    if (Aimbot::settings.enabled && Aimbot::settings.drawFov) {
        POINT p; GetCursorPos(&p);
        float px = (float)p.x, py = (float)p.y, fr = Aimbot::settings.fov;
        ImU32 fc = IM_COL32((int)(Aimbot::settings.fovColor[0] * 255), (int)(Aimbot::settings.fovColor[1] * 255), (int)(Aimbot::settings.fovColor[2] * 255), 200);
        if (g_cfg.espBoxGlow) {
            float gi = g_cfg.espBoxGlowIntens;
            auto GA=[&](ImU32 b,float a)->ImU32{return(b&0x00FFFFFF)|((ImU32)(a*gi)<<24);};
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x12),128,10.f);
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x28),128,6.f);
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x50),128,3.5f);
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x90),128,1.5f);
        }
        draw->AddCircle(ImVec2(px,py),fr,fc,128,1.0f);
    }

    /* ── ESP FOV circle ────────────────────────────────────────── */
    if (s.showFovCircle) {
        POINT cp; GetCursorPos(&cp);
        float px = (float)cp.x, py = (float)cp.y, fr = s.fovCircleRadius;
        ImU32 fc = IM_COL32((int)(s.fovCircleCol[0]*255),(int)(s.fovCircleCol[1]*255),(int)(s.fovCircleCol[2]*255),200);
        if (g_cfg.espBoxGlow) {
            float gi = g_cfg.espBoxGlowIntens;
            auto GA=[&](ImU32 b,float a)->ImU32{return(b&0x00FFFFFF)|((ImU32)(a*gi)<<24);};
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x12),128,10.f);
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x28),128,6.f);
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x50),128,3.5f);
            draw->AddCircle(ImVec2(px,py),fr,GA(fc,0x90),128,1.5f);
        }
        draw->AddCircle(ImVec2(px,py),fr,fc,128,1.0f);
    }

    /* ── 2D Radar ──────────────────────────────────────────────── */
    if (s.showRadar) {
        float rSz = s.radarSize, rRng = s.radarRange;
        float rX = s.radarX, rY = s.radarY, rCx = rX + rSz * 0.5f, rCy = rY + rSz * 0.5f;
        draw->AddRectFilled(ImVec2(rX, rY), ImVec2(rX + rSz, rY + rSz), IM_COL32(0, 0, 0, 160));
        draw->AddRect(ImVec2(rX, rY), ImVec2(rX + rSz, rY + rSz), IM_COL32(180, 180, 180, 200));
        draw->AddLine(ImVec2(rCx, rY), ImVec2(rCx, rY + rSz), IM_COL32(50, 50, 50, 180));
        draw->AddLine(ImVec2(rX, rCy), ImVec2(rX + rSz, rCy), IM_COL32(50, 50, 50, 180));
        draw->AddText(ImVec2(rX + 3, rY + 3), IM_COL32(180, 180, 180, 255), "RADAR");
        for (auto& pl : players) {
            if (pl.isLocalPlayer || !pl.valid || pl.health <= 0.1f) continue;
            float dx = pl.rootPos[0] - localPos[0], dz = pl.rootPos[2] - localPos[2];
            float px = rCx + (dx / rRng) * (rSz * 0.5f), py = rCy + (dz / rRng) * (rSz * 0.5f);
            px = (px < rX + 2) ? rX + 2 : (px > rX + rSz - 2) ? rX + rSz - 2 : px;
            py = (py < rY + 2) ? rY + 2 : (py > rY + rSz - 2) ? rY + rSz - 2 : py;
            float ds = s.radarDotSize;
            /* Fixed: was incorrectly using radarDotSize as color channel — now proper red dots */
            draw->AddCircleFilled(ImVec2(px, py), ds, IM_COL32(255, 80, 80, 230));
            draw->AddCircle(ImVec2(px, py), ds, IM_COL32(0, 0, 0, 200));
        }
        draw->AddCircleFilled(ImVec2(rCx, rCy), 4.5f, IM_COL32(255, 255, 255, 255));
    }

    /* ── Chams ─────────────────────────────────────────────────── */
    if (s.chams) {
        static float chPulseT = 0.0f; chPulseT += 0.002f;
        for (auto& pl : players) {
            if (pl.isLocalPlayer || !pl.valid || pl.health <= 0.1f) continue;
            float hs[2], fs[2];
            if (!WorldToScreen(pl.headPos, vm, sw, sh, hs)) continue;
            if (!WorldToScreen(pl.rootPos, vm, sw, sh, fs)) continue;
            float cbh = fs[1] - hs[1], cbw = cbh * 0.55f, cbx = hs[0] - cbw * 0.5f;
            if (cbw < 1 || cbh < 1) continue;
            float alpha = s.chamsPulse ? s.chamsAlpha * ((sinf(chPulseT) + 1) * 0.5f * 0.8f + 0.2f) : s.chamsAlpha;
            draw->AddRectFilled(ImVec2(cbx, hs[1]), ImVec2(cbx + cbw, fs[1]),
                IM_COL32((int)(s.chamsCol[0] * 255), (int)(s.chamsCol[1] * 255), (int)(s.chamsCol[2] * 255), (int)(alpha * 255)));
        }
    }

    /* ── Player count ──────────────────────────────────────────── */
    if (s.showPlayerCount) {
        int cnt = 0;
        for (auto& p2 : players) if (!p2.isLocalPlayer && p2.valid && p2.health > 0) cnt++;
        char buf[32]; snprintf(buf, sizeof(buf), "Players: %d", cnt);
        draw->AddText(ImVec2(sw - 115.0f, 10.0f), IM_COL32(255, 255, 255, 220), buf);
    }

    /* ── Aimbot keybind ────────────────────────────────────────── */
    if (Aimbot::settings.enabled && Aimbot::settings.showKeyBind) {
        const char* kn = (Aimbot::settings.key == VK_RBUTTON) ? "RMB" :
            (Aimbot::settings.key == VK_LBUTTON) ? "LMB" :
            (Aimbot::settings.key == VK_XBUTTON1) ? "M4" :
            (Aimbot::settings.key == VK_XBUTTON2) ? "M5" : "KEY";
        bool held = (GetAsyncKeyState(Aimbot::settings.key) & 0x8000) != 0;
        char buf[32]; snprintf(buf, sizeof(buf), "AIM [%s]", kn);
        draw->AddText(ImVec2(sw - 115.0f, 28.0f), held ? IM_COL32(100, 255, 100, 255) : IM_COL32(180, 180, 180, 200), buf);
    }

    /* ── Custom crosshair ──────────────────────────────────────── */
    if (s.customCrosshair) {
        float cx = sw * 0.5f, cy = sh * 0.5f, sz = s.crosshairSize, gap = s.crosshairGap, th = s.crosshairThick;
        ImU32 cc = IM_COL32((int)(s.crosshairCol[0] * 255), (int)(s.crosshairCol[1] * 255), (int)(s.crosshairCol[2] * 255), (int)(s.crosshairAlpha * 255));
        ImU32 ol = IM_COL32(0, 0, 0, (int)(s.crosshairAlpha * 180));
        if (s.crosshairStyle == 0) {
            draw->AddLine(ImVec2(cx - sz - 1, cy), ImVec2(cx - gap - 1, cy), ol, th + 1.5f);
            draw->AddLine(ImVec2(cx + gap + 1, cy), ImVec2(cx + sz + 1, cy), ol, th + 1.5f);
            draw->AddLine(ImVec2(cx, cy - sz - 1), ImVec2(cx, cy - gap - 1), ol, th + 1.5f);
            draw->AddLine(ImVec2(cx, cy + gap + 1), ImVec2(cx, cy + sz + 1), ol, th + 1.5f);
            draw->AddLine(ImVec2(cx - sz, cy), ImVec2(cx - gap, cy), cc, th);
            draw->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + sz, cy), cc, th);
            draw->AddLine(ImVec2(cx, cy - sz), ImVec2(cx, cy - gap), cc, th);
            draw->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + sz), cc, th);
        }
        else if (s.crosshairStyle == 1) {
            draw->AddCircleFilled(ImVec2(cx, cy), th + 1.5f, ol);
            draw->AddCircleFilled(ImVec2(cx, cy), th, cc);
        }
        else if (s.crosshairStyle == 2) {
            draw->AddCircle(ImVec2(cx, cy), sz, ol, 32, th + 1.5f);
            draw->AddCircle(ImVec2(cx, cy), sz, cc, 32, th);
        }
        else {
            draw->AddLine(ImVec2(cx - sz, cy), ImVec2(cx - gap, cy), cc, th);
            draw->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + sz, cy), cc, th);
            draw->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + sz), cc, th);
        }
    }

    /* ── Hitmarker ─────────────────────────────────────────────── */
    if (Aimbot::settings.hitmarker && Aimbot::HasActiveHitmarker()) {
        float hx, hy; Aimbot::GetHitmarkerPos(hx, hy);
        float sz = Aimbot::settings.hitmarkerSize;
        ImU32 hmc = IM_COL32((int)(Aimbot::settings.hitmarkerCol[0] * 255), (int)(Aimbot::settings.hitmarkerCol[1] * 255), (int)(Aimbot::settings.hitmarkerCol[2] * 255), 220);
        draw->AddLine(ImVec2(hx - sz, hy - sz), ImVec2(hx + sz, hy + sz), IM_COL32(0, 0, 0, 180), 2.5f);
        draw->AddLine(ImVec2(hx + sz, hy - sz), ImVec2(hx - sz, hy + sz), IM_COL32(0, 0, 0, 180), 2.5f);
        draw->AddLine(ImVec2(hx - sz, hy - sz), ImVec2(hx + sz, hy + sz), hmc, 1.5f);
        draw->AddLine(ImVec2(hx + sz, hy - sz), ImVec2(hx - sz, hy + sz), hmc, 1.5f);
    }

    /* ── Spectator alert ───────────────────────────────────────── */
    if (s.spectatorAlert) {
        int sc = 0;
        for (auto& p : players) { if (!p.isLocalPlayer && p.valid && Distance3D(localPos, p.rootPos) < 3.0f) sc++; }
        if (sc > 0) {
            float fl = (sinf((float)GetTickCount() * 0.008f) + 1.0f) * 0.5f;
            char buf[64]; snprintf(buf, sizeof(buf), "! SPECTATOR DETECTED (%d)", sc);
            ImVec2 tsz = ImGui::CalcTextSize(buf);
            float tx = (sw - tsz.x) * 0.5f, ty = sh * 0.15f;
            draw->AddRectFilled(ImVec2(tx - 8, ty - 4), ImVec2(tx + tsz.x + 8, ty + tsz.y + 4), IM_COL32(0, 0, 0, 160));
            draw->AddText(ImVec2(tx, ty), IM_COL32(255, (int)(fl * 80), (int)(fl * 80), 230), buf);
        }
    }


    /* ════════════════════════════════════════════════════════════
       TARGET HUD — designed to match the industrial menu / miniplayer
       aesthetic: near-black card, left vinyl-art panel (animated),
       hairline divider, accent-coloured HP bar, clean typography.
       Floats near cursor, respects user offset + alpha settings.
       ════════════════════════════════════════════════════════════ */
    const PlayerData* tgt = Aimbot::GetTarget();
    if (tgt && s.showTargetHud) {
        POINT tp; GetCursorPos(&tp);

        /* ── Dimensions ──────────────────────────────────────── */
        constexpr float kW    = 220.f;
        constexpr float kArtW = 42.f;   /* left art panel */
        constexpr float kR    = 8.f;    /* card radius     */
        constexpr float kPad  = 10.f;
        constexpr float kBarH = 3.f;    /* progress bar    */
        float lhH = ImGui::GetTextLineHeight();

        /* Measure total height */
        float kH = kPad;
        kH += lhH + 2.f;                           /* name */
        kH += lhH * 0.8f + 4.f;                    /* subtitle (dist/team) */
        if (s.tgtHudShowHp) kH += kBarH + 8.f;    /* HP bar */
        if (s.tgtHudShowHpNum) kH += lhH * 0.82f + 4.f;
        kH += kPad;

        /* Position */
        float ox = (float)tp.x + 18.f + s.tgtHudOffsetX;
        float oy = (float)tp.y + 18.f + s.tgtHudOffsetY;
        if (ox + kW > sw) ox = (float)tp.x - kW - 4.f + s.tgtHudOffsetX;
        if (oy + kH > sh) oy = (float)tp.y - kH - 4.f + s.tgtHudOffsetY;

        int aScale = (int)(s.tgtHudAlpha * 255.f);
        auto A = [&](int base)->int { int v=(base*aScale)/255; return v>255?255:v; };

        ImDrawList* fg = ImGui::GetForegroundDrawList();

        /* Accent from box colour */
        int acR=(int)(s.boxCol[0]*255),acG=(int)(s.boxCol[1]*255),acB=(int)(s.boxCol[2]*255);
        auto AC=[&](int a)->ImU32{return IM_COL32(acR,acG,acB,A(a));};

        /* ── Card shadow ─────────────────────────────────────── */
        fg->AddRectFilled(ImVec2(ox+3,oy+4),ImVec2(ox+kW+3,oy+kH+4),
            IM_COL32(0,0,0,A(70)), kR+2.f);

        /* ── Card body — same near-black as industrial menu ──── */
        fg->AddRectFilled(ImVec2(ox,oy),ImVec2(ox+kW,oy+kH),
            IM_COL32(9,9,12,A(245)), kR);
        /* 1px white border */
        fg->AddRect(ImVec2(ox,oy),ImVec2(ox+kW,oy+kH),
            IM_COL32(255,255,255,A(12)), kR, 0, 1.f);
        /* Accent top hairline */
        fg->AddLine(ImVec2(ox+kR,oy+0.5f),ImVec2(ox+kW-kR,oy+0.5f),
            AC(A(50)), 1.f);

        /* ── Left art panel (vinyl style matching menu header) ── */
        fg->AddRectFilled(ImVec2(ox,oy),ImVec2(ox+kArtW,oy+kH),
            IM_COL32(5,5,8,A(255)), kR);
        /* Square off right corners */
        fg->AddRectFilled(ImVec2(ox+kArtW-kR,oy),ImVec2(ox+kArtW,oy+kH),
            IM_COL32(5,5,8,A(255)));
        /* Hairline divider */
        fg->AddLine(ImVec2(ox+kArtW,oy+kR),ImVec2(ox+kArtW,oy+kH-kR),
            IM_COL32(255,255,255,A(8)), 1.f);

        /* Vinyl concentric rings */
        {
            static float s_hudAnim = 0.f;
            s_hudAnim += ImGui::GetIO().DeltaTime * 0.2f;
            float vcx = ox+kArtW*.5f, vcy = oy+kH*.5f;
            int rings = 4;
            for (int i=rings;i>=1;i--) {
                float r2 = (float)i * (kArtW*.11f);
                /* Alternate rotation direction per ring */
                float rot = s_hudAnim * (i%2 ? 1.f : -1.f) * (1.f + i*.08f);
                int segs = 16;
                for (int j=0;j<segs;j++) {
                    float a0=rot+(float)j/segs*6.2832f;
                    float a1=rot+(float)(j+1)/segs*6.2832f;
                    int al = (i==rings&&j%2==0) ? A(2) : A(4+i*3);
                    fg->AddLine(
                        ImVec2(vcx+cosf(a0)*r2, vcy+sinf(a0)*r2),
                        ImVec2(vcx+cosf(a1)*r2, vcy+sinf(a1)*r2),
                        IM_COL32(255,255,255,al), 1.f);
                }
            }
            /* Centre disc */
            fg->AddCircleFilled(ImVec2(vcx,vcy), 4.f, AC(A(70)), 12);
            fg->AddCircleFilled(ImVec2(vcx,vcy), 2.f,
                IM_COL32(200,200,210,A(180)), 8);
            /* Pulsing outer ring */
            float rp = sinf(s_hudAnim*8.f)*.5f+.5f;
            fg->AddCircle(ImVec2(vcx,vcy), kArtW*.42f+rp*2.f,
                AC(A((int)(6+rp*14))), 20, 1.f);
        }

        /* ── Text content (right of art panel) ──────────────── */
        float tx2 = ox + kArtW + kPad;
        float tw2 = kW - kArtW - kPad * 2.f;
        float cy2 = oy + kPad;

        /* Name — white */
        const char* tName = (s.showDisplayName && !tgt->displayName.empty())
                           ? tgt->displayName.c_str() : tgt->name.c_str();
        fg->AddText(ImVec2(tx2, cy2), IM_COL32(220,220,228,A(240)), tName);
        cy2 += lhH + 2.f;

        /* Distance subtitle — dim */
        if (s.tgtHudShowDist) {
            float dx2=tgt->rootPos[0]-localPos[0];
            float dy2=tgt->rootPos[1]-localPos[1];
            float dz2=tgt->rootPos[2]-localPos[2];
            char distBuf[24];
            snprintf(distBuf,sizeof(distBuf),"%.0f studs",sqrtf(dx2*dx2+dy2*dy2+dz2*dz2));
            fg->AddText(ImGui::GetFont(), ImGui::GetFontSize()*.80f,
                ImVec2(tx2, cy2), IM_COL32(55,55,65,A(200)), distBuf);
            cy2 += lhH*.8f + 4.f;
        }

        /* HP bar — thin gradient fill like the vinyl progress bar */
        if (s.tgtHudShowHp) {
            float hpPct = (tgt->maxHealth > 0.f)
                        ? (tgt->health / tgt->maxHealth) : 0.f;
            if (hpPct < 0.f) hpPct = 0.f;
            if (hpPct > 1.f) hpPct = 1.f;
            /* Track */
            fg->AddRectFilled(ImVec2(tx2,cy2),ImVec2(tx2+tw2,cy2+kBarH),
                IM_COL32(22,22,28,A(220)), kBarH*.5f);
            /* Fill */
            if (hpPct > 0.f) {
                /* HP colour: green→yellow→red */
                int hpR=(int)((1.f-hpPct)*2.f*255.f); if(hpR>255)hpR=255;
                int hpG=(int)(hpPct      *2.f*255.f); if(hpG>255)hpG=255;
                fg->AddRectFilled(ImVec2(tx2,cy2),
                    ImVec2(tx2+tw2*hpPct,cy2+kBarH),
                    IM_COL32(hpR,hpG,28,A(230)), kBarH*.5f);
            }
            /* Knob dot */
            fg->AddCircleFilled(ImVec2(tx2+tw2*hpPct,cy2+kBarH*.5f),
                3.f, IM_COL32(220,220,228,A(200)), 8);
            cy2 += kBarH + 8.f;
        }

        /* HP number */
        if (s.tgtHudShowHpNum && tgt->maxHealth > 0.f) {
            char hpBuf[24];
            snprintf(hpBuf,sizeof(hpBuf),"%.0f / %.0f",tgt->health,tgt->maxHealth);
            ImVec2 hpSz = ImGui::CalcTextSize(hpBuf);
            float fs = ImGui::GetFontSize()*.82f;
            fg->AddText(ImGui::GetFont(),fs,ImVec2(tx2,cy2),
                IM_COL32(55,55,65,A(200)),"hp");
            fg->AddText(ImGui::GetFont(),fs,
                ImVec2(ox+kW-kPad-hpSz.x*.82f,cy2),
                IM_COL32(180,180,188,A(220)),hpBuf);
        }
    }

} // namespace EspNormal
} // namespace EspNormal  /* closes namespace — was missing */
