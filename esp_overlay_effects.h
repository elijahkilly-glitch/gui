#pragma once
/*
 * esp_overlay_effects.h  —  x9  Menu/loader overlay effects
 *
 * Called from ESP::Render() after all ESP, before Spotify overlay.
 * Effects are gated on (Menu::IsVisible() || !Loader::IsDone()).
 * Each effect is individually toggled via g_cfg.fx.
 */

#include "settings.h"
#include "menu.h"
#include "loader.h"
#include "imgui.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>

namespace X9Effects {

/* ── Snow particles ─────────────────────────────────────────────── */
struct SnowFlake { float x,y,r,speed,wobble,phase; };
static constexpr int kMaxSnow = 400;
static SnowFlake s_snow[kMaxSnow];
static bool      s_snowInit = false;
static float     s_snowTime = 0.f;

static void InitSnow(float sw, float sh) {
    for(int i=0;i<kMaxSnow;i++){
        s_snow[i].x      = (float)(rand()%((int)sw+1));
        s_snow[i].y      = (float)(rand()%((int)sh+1));
        s_snow[i].r      = 1.5f + (rand()%30)*0.08f;
        s_snow[i].speed  = 25.f + (rand()%80)*0.4f;
        s_snow[i].wobble = 8.f  + (rand()%60)*0.15f;
        s_snow[i].phase  = (float)(rand()%628)/100.f;
    }
    s_snowInit = true;
}

static void DrawEffects(bool menuOpen) {
    if (!menuOpen) { s_snowTime=0.f; return; }

    OverlayEffects& fx = g_cfg.fx;
    if (!fx.darken&&!fx.vignette&&!fx.snow&&!fx.scanlines&&!fx.rgbBorder) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    float sw = ImGui::GetIO().DisplaySize.x;
    float sh = ImGui::GetIO().DisplaySize.y;
    float dt = ImGui::GetIO().DeltaTime;
    if(dt>0.1f)dt=0.1f;
    s_snowTime += dt;

    /* ── Darken overlay ─────────────────────────────────────── */
    if (fx.darken) {
        int a = (int)(fx.darkenAlpha * 200.f);
        if(a>200)a=200;
        dl->AddRectFilled({0,0},{sw,sh}, IM_COL32(0,0,0,a));
    }

    /* ── Vignette — dark radial fade from edges ─────────────── */
    if (fx.vignette) {
        float cx=sw*.5f, cy=sh*.5f;
        float str=(int)(fx.vignetteStr*180.f);
        /* 4 gradient rects from each edge */
        float edge=0.35f; /* how far in vignette reaches */
        /* Top */
        dl->AddRectFilledMultiColor({0,0},{sw,sh*edge},
            IM_COL32(0,0,0,(int)str),IM_COL32(0,0,0,(int)str),
            IM_COL32(0,0,0,0),IM_COL32(0,0,0,0));
        /* Bottom */
        dl->AddRectFilledMultiColor({0,sh*(1.f-edge)},{sw,sh},
            IM_COL32(0,0,0,0),IM_COL32(0,0,0,0),
            IM_COL32(0,0,0,(int)str),IM_COL32(0,0,0,(int)str));
        /* Left */
        dl->AddRectFilledMultiColor({0,0},{sw*edge,sh},
            IM_COL32(0,0,0,(int)str),IM_COL32(0,0,0,0),
            IM_COL32(0,0,0,0),IM_COL32(0,0,0,(int)str));
        /* Right */
        dl->AddRectFilledMultiColor({sw*(1.f-edge),0},{sw,sh},
            IM_COL32(0,0,0,0),IM_COL32(0,0,0,(int)str),
            IM_COL32(0,0,0,(int)str),IM_COL32(0,0,0,0));
    }

    /* ── Scanlines ───────────────────────────────────────────── */
    if (fx.scanlines) {
        int a=(int)(fx.scanlineAlpha*80.f);
        for(float y=0.f;y<sh;y+=4.f)
            dl->AddLine({0,y},{sw,y},IM_COL32(0,0,0,a),1.f);
    }

    /* ── Snowfall ────────────────────────────────────────────── */
    if (fx.snow) {
        if(!s_snowInit||(int)sw==0) InitSnow(sw,sh);
        int count=fx.snowCount; if(count<1)count=1; if(count>kMaxSnow)count=kMaxSnow;
        int alpha=(int)(fx.snowOpacity*200.f); if(alpha>200)alpha=200;

        for(int i=0;i<count;i++){
            SnowFlake& p=s_snow[i];
            /* Update position */
            p.y += p.speed * fx.snowSpeed * dt;
            p.x += sinf(s_snowTime*0.8f+p.phase)*p.wobble*dt;
            /* Wrap */
            if(p.y>sh+p.r*2.f){ p.y=-p.r; p.x=(float)(rand()%((int)sw+1)); }
            if(p.x<-p.r)       { p.x=sw+p.r; }
            if(p.x>sw+p.r)     { p.x=-p.r; }
            /* Draw — smaller = dimmer */
            float bright=0.6f+p.r*0.08f;
            int a2=(int)(alpha*bright); if(a2>200)a2=200;
            dl->AddCircleFilled({p.x,p.y},p.r,IM_COL32(220,225,240,a2),8);
            /* Tiny specular glint on larger flakes */
            if(p.r>2.5f)
                dl->AddCircleFilled({p.x-p.r*.3f,p.y-p.r*.3f},
                    p.r*.25f,IM_COL32(255,255,255,(int)(a2*.6f)),6);
        }
    }

    /* ── RGB cycling border ──────────────────────────────────── */
    if (fx.rgbBorder) {
        float t=s_snowTime*fx.rgbSpeed;
        auto HSV=[](float h)->ImU32{
            h=fmodf(h,1.f); int hi=(int)(h*6.f)%6;
            float f=h*6.f-(int)(h*6.f),q=1.f-f;
            float r,g,b;
            switch(hi){
                case 0:r=1;g=f;b=0;break; case 1:r=q;g=1;b=0;break;
                case 2:r=0;g=1;b=f;break; case 3:r=0;g=q;b=1;break;
                case 4:r=f;g=0;b=1;break; default:r=1;g=0;b=q;break;
            }
            return IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),200);
        };
        /* 4-segment border, each a slightly offset hue */
        float seg=1.f/4.f;
        dl->AddLine({0,0},{sw,0},    HSV(t),        3.f);
        dl->AddLine({sw,0},{sw,sh},  HSV(t+seg),    3.f);
        dl->AddLine({sw,sh},{0,sh},  HSV(t+seg*2.f),3.f);
        dl->AddLine({0,sh},{0,0},    HSV(t+seg*3.f),3.f);
        /* Inner glow */
        dl->AddRect({2,2},{sw-2,sh-2},HSV(t+0.5f),0,0,1.f);
    }
}

} // namespace X9Effects
