#pragma once
/*
 * config.h  —  x9 JSON config system
 *
 * Zero external dependencies — hand-rolled JSON writer/reader.
 * Configs are saved to  <exe_dir>/configs/<name>.json
 * and can be enumerated, loaded, saved, and deleted at runtime.
 *
 * Covers every field in Config (esp, movement, world, fx, cat extras)
 * and Aimbot::Settings.
 */

#include "settings.h"
#include "aimbot.h"

#include <Windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

/* ─────────────────────────────────────────────────────────────────
   Tiny self-contained JSON builder
───────────────────────────────────────────────────────────────── */
struct JsonWriter {
    std::string buf;
    int depth = 0;
    bool needComma = false;

    void raw(const char* s) { buf += s; }
    void indent() { for(int i=0;i<depth;i++) buf += "  "; }
    void comma()  { if(needComma){ buf+=",\n"; } else { buf+="\n"; } needComma=false; }

    void beginObj() { comma(); indent(); raw("{"); depth++; needComma=false; buf+="\n"; }
    void endObj()   { depth--; buf+="\n"; indent(); raw("}"); needComma=true; }
    void beginArr() { raw("[\n"); depth++; needComma=false; }
    void endArr()   { depth--; buf+="\n"; indent(); raw("]"); needComma=true; }

    /* key helpers */
    void key(const char* k) {
        comma(); indent();
        buf += "\""; buf += k; buf += "\": ";
        needComma = false;
    }

    void val(bool v)  { buf += v?"true":"false"; needComma=true; }
    void val(int v)   { char t[32]; snprintf(t,sizeof(t),"%d",v); buf+=t; needComma=true; }
    void val(float v) {
        char t[32];
        /* avoid -0.0 and NaN */
        if(!std::isfinite(v)) v=0.f;
        snprintf(t,sizeof(t),"%.6g",v);
        buf+=t; needComma=true;
    }
    void val(const char* s) {
        buf+="\"";
        for(const char* p=s;*p;p++){
            if(*p=='"') buf+="\\\"";
            else if(*p=='\\') buf+="\\\\";
            else buf+=*p;
        }
        buf+="\""; needComma=true;
    }

    void kbool (const char* k, bool v)        { key(k); val(v); }
    void kint  (const char* k, int v)         { key(k); val(v); }
    void kfloat(const char* k, float v)       { key(k); val(v); }
    void kstr  (const char* k, const char* v) { key(k); val(v); }

    void kfloat3(const char* k, const float v[3]) {
        key(k); raw("[");
        char t[96]; snprintf(t,sizeof(t),"%.6g,%.6g,%.6g",v[0],v[1],v[2]);
        raw(t); raw("]"); needComma=true;
    }

    std::string str() { return buf; }
};

/* ─────────────────────────────────────────────────────────────────
   Tiny self-contained JSON parser (enough for our schema)
   Parses a flat key→value map; nested objects via prefix.
───────────────────────────────────────────────────────────────── */
struct JsonParser {
    struct KV { std::string key; std::string val; };
    std::vector<KV> pairs;

    /* ---- tokeniser ---- */
    static std::string unescape(const char* s, int len) {
        std::string out; out.reserve(len);
        for(int i=0;i<len;i++){
            if(s[i]=='\\'&&i+1<len){
                i++;
                if(s[i]=='"')  out+='"';
                else if(s[i]=='\\') out+='\\';
                else if(s[i]=='n')  out+='\n';
                else out+=s[i];
            } else out+=s[i];
        }
        return out;
    }

    bool parse(const std::string& json) {
        pairs.clear();
        /* Extract all "key": value pairs recursively with dotted path */
        parseObj(json.c_str(), (int)json.size(), "");
        return true;
    }

    /* Recursive object parser — builds dotted paths */
    void parseObj(const char* p, int len, const std::string& prefix) {
        int i = 0;
        /* skip leading whitespace / '{' */
        while(i<len && (p[i]==' '||p[i]=='\t'||p[i]=='\n'||p[i]=='\r'||p[i]=='{')) i++;

        while(i<len) {
            /* skip whitespace & commas */
            while(i<len && (p[i]==' '||p[i]=='\t'||p[i]=='\n'||p[i]=='\r'||p[i]==',')) i++;
            if(i>=len||p[i]=='}') break;

            /* read key */
            if(p[i]!='"') { i++; continue; }
            i++;
            int ks=i;
            while(i<len&&p[i]!='"') { if(p[i]=='\\') i++; i++; }
            std::string rawKey = unescape(p+ks, i-ks);
            if(i<len) i++;  /* skip closing " */

            /* skip whitespace + ':' */
            while(i<len&&(p[i]==' '||p[i]=='\t'||p[i]=='\n'||p[i]=='\r'||p[i]==':')) i++;

            std::string fullKey = prefix.empty() ? rawKey : prefix+"."+rawKey;

            /* read value */
            if(i<len && p[i]=='{') {
                /* nested object — recurse */
                int depth=1, start=i+1;
                i++;
                while(i<len&&depth>0){
                    if(p[i]=='{') depth++;
                    else if(p[i]=='}') depth--;
                    i++;
                }
                parseObj(p+start, i-start-1, fullKey);
            } else if(i<len && p[i]=='[') {
                /* array — read as raw string */
                int start=i;
                int depth2=1; i++;
                while(i<len&&depth2>0){
                    if(p[i]=='[') depth2++;
                    else if(p[i]==']') depth2--;
                    i++;
                }
                KV kv; kv.key=fullKey;
                kv.val = std::string(p+start, i-start);
                pairs.push_back(kv);
            } else if(i<len && p[i]=='"') {
                /* string value */
                i++;
                int vs=i;
                while(i<len&&p[i]!='"'){if(p[i]=='\\')i++;i++;}
                KV kv; kv.key=fullKey;
                kv.val = unescape(p+vs, i-vs);
                if(i<len) i++;
                pairs.push_back(kv);
            } else {
                /* number / bool / null */
                int vs=i;
                while(i<len&&p[i]!=','&&p[i]!='}'&&p[i]!=']'&&p[i]!='\n'&&p[i]!='\r') i++;
                KV kv; kv.key=fullKey;
                kv.val = std::string(p+vs, i-vs);
                /* trim trailing whitespace */
                while(!kv.val.empty()&&(kv.val.back()==' '||kv.val.back()=='\t')) kv.val.pop_back();
                pairs.push_back(kv);
            }
        }
    }

    /* ---- lookup ---- */
    const std::string* get(const char* k) const {
        for(auto& kv:pairs) if(kv.key==k) return &kv.val;
        return nullptr;
    }

    bool   getBool (const char* k, bool   def) const {
        auto* v=get(k); if(!v) return def;
        return *v=="true"||*v=="1";
    }
    int    getInt  (const char* k, int    def) const {
        auto* v=get(k); if(!v) return def;
        return atoi(v->c_str());
    }
    float  getFloat(const char* k, float  def) const {
        auto* v=get(k); if(!v) return def;
        return (float)atof(v->c_str());
    }
    void   getFloat3(const char* k, float out[3]) const {
        auto* v=get(k); if(!v) return;
        /* parse [r,g,b] */
        float a=0,b=0,c=0;
        sscanf_s(v->c_str(),"[%f,%f,%f]",&a,&b,&c);
        out[0]=a; out[1]=b; out[2]=c;
    }
};

/* ─────────────────────────────────────────────────────────────────
   Config directory helpers
───────────────────────────────────────────────────────────────── */
namespace ConfigIO {

    static std::string GetConfigDir() {
        char path[MAX_PATH]={};
        GetModuleFileNameA(NULL, path, MAX_PATH);
        PathRemoveFileSpecA(path);
        std::string dir = std::string(path) + "\\configs";
        CreateDirectoryA(dir.c_str(), NULL);
        return dir;
    }

    static std::string GetConfigPath(const std::string& name) {
        std::string n = name;
        /* strip .json if user typed it */
        if(n.size()>5 && n.substr(n.size()-5)==".json") n=n.substr(0,n.size()-5);
        /* sanitise — allow alphanum, space, dash, underscore */
        std::string safe;
        for(char c:n)
            safe += (isalnum((unsigned char)c)||c==' '||c=='-'||c=='_') ? c : '_';
        if(safe.empty()) safe="config";
        return GetConfigDir() + "\\" + safe + ".json";
    }

    /* List all .json files in configs dir */
    static std::vector<std::string> ListConfigs() {
        std::vector<std::string> out;
        std::string pattern = GetConfigDir() + "\\*.json";
        WIN32_FIND_DATAA fd{};
        HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
        if(h==INVALID_HANDLE_VALUE) return out;
        do {
            std::string n = fd.cFileName;
            /* strip .json */
            if(n.size()>5) n = n.substr(0, n.size()-5);
            out.push_back(n);
        } while(FindNextFileA(h, &fd));
        FindClose(h);
        std::sort(out.begin(), out.end());
        return out;
    }

/* ─────────────────────────────────────────────────────────────────
   SERIALIZE  g_cfg + Aimbot::settings → JSON string
───────────────────────────────────────────────────────────────── */
    static std::string Serialize() {
        JsonWriter w;
        w.raw("{");
        w.depth=1; w.needComma=false; w.buf+="\n";

        /* ── ESP ─────────────────────────────────────────────── */
        w.key("esp"); w.raw("{"); w.depth++; w.needComma=false; w.buf+="\n";
        {
            const ESPSettings& e = g_cfg.esp;
            w.kbool ("enabled",          e.enabled);
            w.kbool ("showBox",          e.showBox);
            w.kint  ("boxType",          e.boxType);
            w.kfloat3("boxCol",          e.boxCol);
            w.kfloat("boxThickness",     e.boxThickness);
            w.kbool ("rainbowBox",       e.rainbowBox);
            w.kbool ("filledBox",        e.filledBox);
            w.kfloat("fillAlpha",        e.fillAlpha);
            w.kbool ("boxOutline",       e.boxOutline);
            w.kfloat("cornerLen",        e.cornerLen);
            w.kbool ("dynBoxColor",      e.dynBoxColor);
            w.kbool ("pulseBox",         e.pulseBox);
            w.kbool ("showNames",        e.showNames);
            w.kbool ("showDisplayName",  e.showDisplayName);
            w.kfloat3("nameCol",         e.nameCol);
            w.kbool ("nameBg",           e.nameBg);
            w.kfloat("nameBgAlpha",      e.nameBgAlpha);
            w.kbool ("showHealth",       e.showHealth);
            w.kbool ("showHealthNum",    e.showHealthNum);
            w.kint  ("healthBarSide",    e.healthBarSide);
            w.kbool ("showDistance",     e.showDistance);
            w.kfloat("maxDistance",      e.maxDistance);
            w.kbool ("showSkeleton",     e.showSkeleton);
            w.kfloat3("skeletonCol",     e.skeletonCol);
            w.kfloat("skeletonThick",    e.skeletonThick);
            w.kbool ("skeletonRainbow",  e.skeletonRainbow);
            w.kbool ("showSnapline",     e.showSnapline);
            w.kint  ("snaplineStyle",    e.snaplineStyle);
            w.kfloat3("snaplineCol",     e.snaplineCol);
            w.kbool ("snaplinePulse",    e.snaplinePulse);
            w.kbool ("showHeadDot",      e.showHeadDot);
            w.kfloat("headDotRadius",    e.headDotRadius);
            w.kbool ("headDotFill",      e.headDotFill);
            w.kbool ("visCheck",         e.visCheck);
            w.kfloat3("visCol",          e.visCol);
            w.kfloat("visRange",         e.visRange);
            w.kbool ("lowHpFlash",       e.lowHpFlash);
            w.kfloat("lowHpThresh",      e.lowHpThresh);
            w.kbool ("offscreenArrows",  e.offscreenArrows);
            w.kfloat("offscreenMargin",  e.offscreenMargin);
            w.kfloat3("arrowCol",        e.arrowCol);
            w.kfloat("arrowSize",        e.arrowSize);
            w.kbool ("chams",            e.chams);
            w.kfloat3("chamsCol",        e.chamsCol);
            w.kfloat("chamsAlpha",       e.chamsAlpha);
            w.kbool ("chamsPulse",       e.chamsPulse);
            w.kbool ("showRadar",        e.showRadar);
            w.kfloat("radarSize",        e.radarSize);
            w.kfloat("radarRange",       e.radarRange);
            w.kfloat("radarX",           e.radarX);
            w.kfloat("radarY",           e.radarY);
            w.kfloat("radarDotSize",     e.radarDotSize);
            w.kbool ("radarRotate",      e.radarRotate);
            w.kbool ("showPlayerCount",  e.showPlayerCount);
            w.kbool ("showDead",         e.showDead);
            w.kbool ("customCrosshair",  e.customCrosshair);
            w.kint  ("crosshairStyle",   e.crosshairStyle);
            w.kfloat("crosshairSize",    e.crosshairSize);
            w.kfloat("crosshairThick",   e.crosshairThick);
            w.kfloat("crosshairGap",     e.crosshairGap);
            w.kfloat3("crosshairCol",    e.crosshairCol);
            w.kbool ("crosshairDynamic", e.crosshairDynamic);
            w.kfloat("crosshairAlpha",   e.crosshairAlpha);
            w.kbool ("spectatorAlert",   e.spectatorAlert);
            w.kbool ("showFovCircle",    e.showFovCircle);
            w.kfloat("fovCircleRadius",  e.fovCircleRadius);
            w.kfloat3("fovCircleCol",    e.fovCircleCol);
            /* target hud */
            w.kbool ("showTargetHud",    e.showTargetHud);
            w.kbool ("tgtHudShowHp",     e.tgtHudShowHp);
            w.kbool ("tgtHudShowHpNum",  e.tgtHudShowHpNum);
            w.kbool ("tgtHudShowDist",   e.tgtHudShowDist);
            w.kfloat("tgtHudAlpha",      e.tgtHudAlpha);
            w.kfloat("tgtHudOffsetX",    e.tgtHudOffsetX);
            w.kfloat("tgtHudOffsetY",    e.tgtHudOffsetY);
        }
        w.depth--; w.buf+="\n"; w.indent(); w.raw("}"); w.needComma=true;

        /* ── Aimbot ──────────────────────────────────────────── */
        w.key("aimbot"); w.raw("{"); w.depth++; w.needComma=false; w.buf+="\n";
        {
            const Aimbot::Settings& a = Aimbot::settings;
            w.kbool ("enabled",          a.enabled);
            w.kint  ("key",              a.key);
            w.kint  ("method",           (int)a.method);
            w.kfloat("fov",              a.fov);
            w.kbool ("drawFov",          a.drawFov);
            w.kfloat3("fovColor",        a.fovColor);
            w.kbool ("dynamicFov",       a.dynamicFov);
            w.kfloat("dynamicFovMin",    a.dynamicFovMin);
            w.kfloat("smooth",           a.smooth);
            w.kbool ("adaptiveSmooth",   a.adaptiveSmooth);
            w.kbool ("randomSmooth",     a.randomSmooth);
            w.kfloat("randomSmoothVar",  a.randomSmoothVar);
            w.kint  ("part",             (int)a.part);
            w.kbool ("sticky",           a.sticky);
            w.kbool ("ignoreTeammates",  a.ignoreTeammates);
            w.kfloat("maxAimDist",       a.maxAimDist);
            w.kint  ("priorityMode",     a.priorityMode);
            w.kbool ("visibleOnly",      a.visibleOnly);
            w.kbool ("rageMode",         a.rageMode);
            w.kfloat("reactionTime",     a.reactionTime);
            w.kbool ("switchDelay",      a.switchDelay);
            w.kbool ("jitter",           a.jitter);
            w.kfloat("jitterStr",        a.jitterStr);
            w.kbool ("jitterBothAxes",   a.jitterBothAxes);
            w.kfloat("jitterFreq",       a.jitterFreq);
            w.kbool ("drawDeadzone",     a.drawDeadzone);
            w.kfloat("deadzoneSize",     a.deadzoneSize);
            w.kbool ("prediction",       a.prediction);
            w.kfloat("predictionScale",  a.predictionScale);
            w.kbool ("gravityComp",      a.gravityComp);
            w.kfloat("gravityStr",       a.gravityStr);
            w.kbool ("latencyComp",      a.latencyComp);
            w.kfloat("estimatedPingMs",  a.estimatedPingMs);
            w.kbool ("recoilControl",    a.recoilControl);
            w.kfloat("recoilY",          a.recoilY);
            w.kfloat("recoilX",          a.recoilX);
            w.kbool ("silentAim",        a.silentAim);
            w.kint  ("silentAimKey",     a.silentAimKey);
            w.kbool ("triggerVisCheck",  a.triggerVisCheck);
            w.kbool ("showKeyBind",      a.showKeyBind);
            w.kbool ("triggerbot",       a.triggerbot);
            w.kfloat("hitchance",        a.hitchance);
            w.kint  ("triggerbotKey",    a.triggerbotKey);
            w.kfloat("triggerbotDelay",  a.triggerbotDelay);
            w.kbool ("triggerbotBurst",  a.triggerbotBurst);
            w.kint  ("burstCount",       a.burstCount);
            w.kfloat("burstDelay",       a.burstDelay);
            w.kbool ("triggerOnlyInFov", a.triggerOnlyInFov);
            w.kfloat("triggerFov",       a.triggerFov);
            w.kbool ("triggerHoldMode",  a.triggerHoldMode);
            w.kfloat("triggerPreFire",   a.triggerPreFire);
            w.kbool ("triggerHeadOnly",  a.triggerHeadOnly);
            w.kbool ("triggerNoMove",    a.triggerNoMove);
            w.kbool ("triggerDrawFov",   a.triggerDrawFov);
            w.kfloat3("triggerFovCol",   a.triggerFovCol);
            w.kbool ("autoClick",        a.autoClick);
            w.kfloat("autoClickDelay",   a.autoClickDelay);
            w.kfloat("autoClickMinDist", a.autoClickMinDist);
            w.kbool ("autoClickHold",    a.autoClickHold);
            w.kbool ("hitmarker",        a.hitmarker);
            w.kfloat("hitmarkerSize",    a.hitmarkerSize);
            w.kfloat3("hitmarkerCol",    a.hitmarkerCol);
            w.kfloat("hitmarkerDuration",a.hitmarkerDuration);
            w.kbool ("spinbot",          a.spinbot);
            w.kfloat("spinSpeed",        a.spinSpeed);
            w.kbool ("antiAim",          a.antiAim);
            w.kint  ("antiAimMode",      a.antiAimMode);
            w.kbool ("antiAimPitch",     a.antiAimPitch);
            w.kfloat("antiAimJitterAmp", a.antiAimJitterAmp);
        }
        w.depth--; w.buf+="\n"; w.indent(); w.raw("}"); w.needComma=true;

        /* ── Movement ────────────────────────────────────────── */
        w.key("movement"); w.raw("{"); w.depth++; w.needComma=false; w.buf+="\n";
        {
            const MovementSettings& m = g_cfg.movement;
            w.kbool ("useSpeed",     m.useSpeed);
            w.kfloat("walkSpeed",    m.walkSpeed);
            w.kbool ("useJump",      m.useJump);
            w.kfloat("jumpPower",    m.jumpPower);
            w.kbool ("noclip",       m.noclip);
            w.kbool ("antiAFK",      m.antiAFK);
            w.kbool ("alwaysSprint", m.alwaysSprint);
            w.kbool ("infiniteJump", m.infiniteJump);
            w.kbool ("antiVoid",     m.antiVoid);
            w.kfloat("antiVoidY",    m.antiVoidY);
            w.kbool ("fly",          m.fly);
            w.kfloat("flySpeed",     m.flySpeed);
            w.kbool ("lowGravity",   m.lowGravity);
            w.kfloat("gravityScale", m.gravityScale);
            w.kbool ("godMode",      m.godMode);
            w.kbool ("bunnyHop",     m.bunnyHop);
            w.kbool ("noSlip",       m.noSlip);
            w.kbool ("regenHP",      m.regenHP);
            w.kfloat("regenRate",    m.regenRate);
        }
        w.depth--; w.buf+="\n"; w.indent(); w.raw("}"); w.needComma=true;

        /* ── World ───────────────────────────────────────────── */
        w.key("world"); w.raw("{"); w.depth++; w.needComma=false; w.buf+="\n";
        {
            const WorldSettings& wr = g_cfg.world;
            w.kfloat("fogEnd",       wr.fogEnd);
            w.kfloat("fogStart",     wr.fogStart);
            w.kfloat3("fogColor",    wr.fogColor);
            w.kfloat("brightness",   wr.brightness);
            w.kfloat("clockTime",    wr.clockTime);
            w.kfloat3("ambient",     wr.ambient);
            w.kfloat3("outdoorAmbient", wr.outdoorAmbient);
            w.kfloat("exposureComp", wr.exposureComp);
        }
        w.depth--; w.buf+="\n"; w.indent(); w.raw("}"); w.needComma=true;

        /* ── Overlay FX ──────────────────────────────────────── */
        w.key("fx"); w.raw("{"); w.depth++; w.needComma=false; w.buf+="\n";
        {
            const OverlayEffects& fx = g_cfg.fx;
            w.kbool ("darken",        fx.darken);
            w.kfloat("darkenAlpha",   fx.darkenAlpha);
            w.kbool ("vignette",      fx.vignette);
            w.kfloat("vignetteStr",   fx.vignetteStr);
            w.kbool ("snow",          fx.snow);
            w.kint  ("snowCount",     fx.snowCount);
            w.kfloat("snowSpeed",     fx.snowSpeed);
            w.kfloat("snowOpacity",   fx.snowOpacity);
            w.kbool ("scanlines",     fx.scanlines);
            w.kfloat("scanlineAlpha", fx.scanlineAlpha);
            w.kbool ("rgbBorder",     fx.rgbBorder);
            w.kfloat("rgbSpeed",      fx.rgbSpeed);
        }
        w.depth--; w.buf+="\n"; w.indent(); w.raw("}"); w.needComma=true;

        /* ── UI / Cat extras ─────────────────────────────────── */
        w.key("ui"); w.raw("{"); w.depth++; w.needComma=false; w.buf+="\n";
        {
            w.kint  ("uiTheme",       g_cfg.uiTheme);
            w.kbool ("catDarkMode",   g_cfg.catDarkMode);
            w.kfloat("menuAlpha",     g_cfg.menuAlpha);
            w.kbool ("menuLocked",    g_cfg.menuLocked);
            w.kbool ("menuGlow",      g_cfg.menuGlow);
            w.kfloat("menuGlowIntens",g_cfg.menuGlowIntens);
            w.kbool ("espBoxGlow",    g_cfg.espBoxGlow);
            w.kfloat("espBoxGlowIntens",g_cfg.espBoxGlowIntens);
            w.kbool ("catCrosshair",  g_cfg.catCrosshair);
            w.kbool ("showSpotify",   g_cfg.showSpotify);
            w.kbool ("showSession",   g_cfg.showSession);
            w.kbool ("showHearts",    g_cfg.showHearts);
            w.kbool ("showPawTrail",  g_cfg.showPawTrail);
            w.kfloat("catEarSize",    g_cfg.catEarSize);
            w.kbool ("catBoxFace",    g_cfg.catBoxFace);
            w.kbool ("catBoxPaws",    g_cfg.catBoxPaws);
        }
        w.depth--; w.buf+="\n"; w.indent(); w.raw("}"); w.needComma=true;

        /* close root */
        w.buf += "\n}\n";
        return w.buf;
    }

/* ─────────────────────────────────────────────────────────────────
   DESERIALIZE  JSON string → g_cfg + Aimbot::settings
───────────────────────────────────────────────────────────────── */
    static void Deserialize(const std::string& json) {
        JsonParser p;
        p.parse(json);

        /* ── ESP ─────────────────────────────────────────────── */
        ESPSettings& e = g_cfg.esp;
        #define GBOOL(k,f)   f = p.getBool ("esp." k, f)
        #define GINT(k,f)    f = p.getInt  ("esp." k, f)
        #define GFLOAT(k,f)  f = p.getFloat("esp." k, f)
        #define GFLOAT3(k,f) p.getFloat3   ("esp." k, f)
        GBOOL("enabled",         e.enabled);
        GBOOL("showBox",         e.showBox);
        GINT ("boxType",         e.boxType);
        GFLOAT3("boxCol",        e.boxCol);
        GFLOAT("boxThickness",   e.boxThickness);
        GBOOL("rainbowBox",      e.rainbowBox);
        GBOOL("filledBox",       e.filledBox);
        GFLOAT("fillAlpha",      e.fillAlpha);
        GBOOL("boxOutline",      e.boxOutline);
        GFLOAT("cornerLen",      e.cornerLen);
        GBOOL("dynBoxColor",     e.dynBoxColor);
        GBOOL("pulseBox",        e.pulseBox);
        GBOOL("showNames",       e.showNames);
        GBOOL("showDisplayName", e.showDisplayName);
        GFLOAT3("nameCol",       e.nameCol);
        GBOOL("nameBg",          e.nameBg);
        GFLOAT("nameBgAlpha",    e.nameBgAlpha);
        GBOOL("showHealth",      e.showHealth);
        GBOOL("showHealthNum",   e.showHealthNum);
        GINT ("healthBarSide",   e.healthBarSide);
        GBOOL("showDistance",    e.showDistance);
        GFLOAT("maxDistance",    e.maxDistance);
        GBOOL("showSkeleton",    e.showSkeleton);
        GFLOAT3("skeletonCol",   e.skeletonCol);
        GFLOAT("skeletonThick",  e.skeletonThick);
        GBOOL("skeletonRainbow", e.skeletonRainbow);
        GBOOL("showSnapline",    e.showSnapline);
        GINT ("snaplineStyle",   e.snaplineStyle);
        GFLOAT3("snaplineCol",   e.snaplineCol);
        GBOOL("snaplinePulse",   e.snaplinePulse);
        GBOOL("showHeadDot",     e.showHeadDot);
        GFLOAT("headDotRadius",  e.headDotRadius);
        GBOOL("headDotFill",     e.headDotFill);
        GBOOL("visCheck",        e.visCheck);
        GFLOAT3("visCol",        e.visCol);
        GFLOAT("visRange",       e.visRange);
        GBOOL("lowHpFlash",      e.lowHpFlash);
        GFLOAT("lowHpThresh",    e.lowHpThresh);
        GBOOL("offscreenArrows", e.offscreenArrows);
        GFLOAT("offscreenMargin",e.offscreenMargin);
        GFLOAT3("arrowCol",      e.arrowCol);
        GFLOAT("arrowSize",      e.arrowSize);
        GBOOL("chams",           e.chams);
        GFLOAT3("chamsCol",      e.chamsCol);
        GFLOAT("chamsAlpha",     e.chamsAlpha);
        GBOOL("chamsPulse",      e.chamsPulse);
        GBOOL("showRadar",       e.showRadar);
        GFLOAT("radarSize",      e.radarSize);
        GFLOAT("radarRange",     e.radarRange);
        GFLOAT("radarX",         e.radarX);
        GFLOAT("radarY",         e.radarY);
        GFLOAT("radarDotSize",   e.radarDotSize);
        GBOOL("radarRotate",     e.radarRotate);
        GBOOL("showPlayerCount", e.showPlayerCount);
        GBOOL("showDead",        e.showDead);
        GBOOL("customCrosshair", e.customCrosshair);
        GINT ("crosshairStyle",  e.crosshairStyle);
        GFLOAT("crosshairSize",  e.crosshairSize);
        GFLOAT("crosshairThick", e.crosshairThick);
        GFLOAT("crosshairGap",   e.crosshairGap);
        GFLOAT3("crosshairCol",  e.crosshairCol);
        GBOOL("crosshairDynamic",e.crosshairDynamic);
        GFLOAT("crosshairAlpha", e.crosshairAlpha);
        GBOOL("spectatorAlert",  e.spectatorAlert);
        GBOOL("showFovCircle",   e.showFovCircle);
        GFLOAT("fovCircleRadius",e.fovCircleRadius);
        GFLOAT3("fovCircleCol",  e.fovCircleCol);
        GBOOL("showTargetHud",   e.showTargetHud);
        GBOOL("tgtHudShowHp",    e.tgtHudShowHp);
        GBOOL("tgtHudShowHpNum", e.tgtHudShowHpNum);
        GBOOL("tgtHudShowDist",  e.tgtHudShowDist);
        GFLOAT("tgtHudAlpha",    e.tgtHudAlpha);
        GFLOAT("tgtHudOffsetX",  e.tgtHudOffsetX);
        GFLOAT("tgtHudOffsetY",  e.tgtHudOffsetY);
        #undef GBOOL
        #undef GINT
        #undef GFLOAT
        #undef GFLOAT3

        /* ── Aimbot ──────────────────────────────────────────── */
        Aimbot::Settings& a = Aimbot::settings;
        #define ABOOL(k,f)   f = p.getBool ("aimbot." k, f)
        #define AINT(k,f)    f = p.getInt  ("aimbot." k, f)
        #define AFLOAT(k,f)  f = p.getFloat("aimbot." k, f)
        #define AFLOAT3(k,f) p.getFloat3   ("aimbot." k, f)
        ABOOL("enabled",          a.enabled);
        AINT ("key",              a.key);
        { int m=p.getInt("aimbot.method",(int)a.method); a.method=(AimbotMethod)m; }
        AFLOAT("fov",             a.fov);
        ABOOL("drawFov",          a.drawFov);
        AFLOAT3("fovColor",       a.fovColor);
        ABOOL("dynamicFov",       a.dynamicFov);
        AFLOAT("dynamicFovMin",   a.dynamicFovMin);
        AFLOAT("smooth",          a.smooth);
        ABOOL("adaptiveSmooth",   a.adaptiveSmooth);
        ABOOL("randomSmooth",     a.randomSmooth);
        AFLOAT("randomSmoothVar", a.randomSmoothVar);
        { int pt=p.getInt("aimbot.part",(int)a.part); a.part=(TargetPart)pt; }
        ABOOL("sticky",           a.sticky);
        ABOOL("ignoreTeammates",  a.ignoreTeammates);
        AFLOAT("maxAimDist",      a.maxAimDist);
        AINT ("priorityMode",     a.priorityMode);
        ABOOL("visibleOnly",      a.visibleOnly);
        ABOOL("rageMode",         a.rageMode);
        AFLOAT("reactionTime",    a.reactionTime);
        ABOOL("switchDelay",      a.switchDelay);
        ABOOL("jitter",           a.jitter);
        AFLOAT("jitterStr",       a.jitterStr);
        ABOOL("jitterBothAxes",   a.jitterBothAxes);
        AFLOAT("jitterFreq",      a.jitterFreq);
        ABOOL("drawDeadzone",     a.drawDeadzone);
        AFLOAT("deadzoneSize",    a.deadzoneSize);
        ABOOL("prediction",       a.prediction);
        AFLOAT("predictionScale", a.predictionScale);
        ABOOL("gravityComp",      a.gravityComp);
        AFLOAT("gravityStr",      a.gravityStr);
        ABOOL("latencyComp",      a.latencyComp);
        AFLOAT("estimatedPingMs", a.estimatedPingMs);
        ABOOL("recoilControl",    a.recoilControl);
        AFLOAT("recoilY",         a.recoilY);
        AFLOAT("recoilX",         a.recoilX);
        ABOOL("silentAim",        a.silentAim);
        AINT ("silentAimKey",     a.silentAimKey);
        ABOOL("triggerVisCheck",  a.triggerVisCheck);
        ABOOL("showKeyBind",      a.showKeyBind);
        ABOOL("triggerbot",       a.triggerbot);
        AFLOAT("hitchance",       a.hitchance);
        AINT ("triggerbotKey",    a.triggerbotKey);
        AFLOAT("triggerbotDelay", a.triggerbotDelay);
        ABOOL("triggerbotBurst",  a.triggerbotBurst);
        AINT ("burstCount",       a.burstCount);
        AFLOAT("burstDelay",      a.burstDelay);
        ABOOL("triggerOnlyInFov", a.triggerOnlyInFov);
        AFLOAT("triggerFov",      a.triggerFov);
        ABOOL("triggerHoldMode",  a.triggerHoldMode);
        AFLOAT("triggerPreFire",  a.triggerPreFire);
        ABOOL("triggerHeadOnly",  a.triggerHeadOnly);
        ABOOL("triggerNoMove",    a.triggerNoMove);
        ABOOL("triggerDrawFov",   a.triggerDrawFov);
        AFLOAT3("triggerFovCol",  a.triggerFovCol);
        ABOOL("autoClick",        a.autoClick);
        AFLOAT("autoClickDelay",  a.autoClickDelay);
        AFLOAT("autoClickMinDist",a.autoClickMinDist);
        ABOOL("autoClickHold",    a.autoClickHold);
        ABOOL("hitmarker",        a.hitmarker);
        AFLOAT("hitmarkerSize",   a.hitmarkerSize);
        AFLOAT3("hitmarkerCol",   a.hitmarkerCol);
        AFLOAT("hitmarkerDuration",a.hitmarkerDuration);
        ABOOL("spinbot",          a.spinbot);
        AFLOAT("spinSpeed",       a.spinSpeed);
        ABOOL("antiAim",          a.antiAim);
        AINT ("antiAimMode",      a.antiAimMode);
        ABOOL("antiAimPitch",     a.antiAimPitch);
        AFLOAT("antiAimJitterAmp",a.antiAimJitterAmp);
        #undef ABOOL
        #undef AINT
        #undef AFLOAT
        #undef AFLOAT3

        /* ── Movement ────────────────────────────────────────── */
        MovementSettings& mv = g_cfg.movement;
        #define MBOOL(k,f)   f = p.getBool ("movement." k, f)
        #define MFLOAT(k,f)  f = p.getFloat("movement." k, f)
        MBOOL("useSpeed",     mv.useSpeed);
        MFLOAT("walkSpeed",   mv.walkSpeed);
        MBOOL("useJump",      mv.useJump);
        MFLOAT("jumpPower",   mv.jumpPower);
        MBOOL("noclip",       mv.noclip);
        MBOOL("antiAFK",      mv.antiAFK);
        MBOOL("alwaysSprint", mv.alwaysSprint);
        MBOOL("infiniteJump", mv.infiniteJump);
        MBOOL("antiVoid",     mv.antiVoid);
        MFLOAT("antiVoidY",   mv.antiVoidY);
        MBOOL("fly",          mv.fly);
        MFLOAT("flySpeed",    mv.flySpeed);
        MBOOL("lowGravity",   mv.lowGravity);
        MFLOAT("gravityScale",mv.gravityScale);
        MBOOL("godMode",      mv.godMode);
        MBOOL("bunnyHop",     mv.bunnyHop);
        MBOOL("noSlip",       mv.noSlip);
        MBOOL("regenHP",      mv.regenHP);
        MFLOAT("regenRate",   mv.regenRate);
        #undef MBOOL
        #undef MFLOAT

        /* ── World ───────────────────────────────────────────── */
        WorldSettings& wr = g_cfg.world;
        wr.fogEnd      = p.getFloat("world.fogEnd",      wr.fogEnd);
        wr.fogStart    = p.getFloat("world.fogStart",    wr.fogStart);
        p.getFloat3("world.fogColor",       wr.fogColor);
        wr.brightness  = p.getFloat("world.brightness",  wr.brightness);
        wr.clockTime   = p.getFloat("world.clockTime",   wr.clockTime);
        p.getFloat3("world.ambient",        wr.ambient);
        p.getFloat3("world.outdoorAmbient", wr.outdoorAmbient);
        wr.exposureComp= p.getFloat("world.exposureComp",wr.exposureComp);

        /* ── FX ──────────────────────────────────────────────── */
        OverlayEffects& fx = g_cfg.fx;
        fx.darken       = p.getBool ("fx.darken",        fx.darken);
        fx.darkenAlpha  = p.getFloat("fx.darkenAlpha",   fx.darkenAlpha);
        fx.vignette     = p.getBool ("fx.vignette",      fx.vignette);
        fx.vignetteStr  = p.getFloat("fx.vignetteStr",   fx.vignetteStr);
        fx.snow         = p.getBool ("fx.snow",          fx.snow);
        fx.snowCount    = p.getInt  ("fx.snowCount",     fx.snowCount);
        fx.snowSpeed    = p.getFloat("fx.snowSpeed",     fx.snowSpeed);
        fx.snowOpacity  = p.getFloat("fx.snowOpacity",   fx.snowOpacity);
        fx.scanlines    = p.getBool ("fx.scanlines",     fx.scanlines);
        fx.scanlineAlpha= p.getFloat("fx.scanlineAlpha", fx.scanlineAlpha);
        fx.rgbBorder    = p.getBool ("fx.rgbBorder",     fx.rgbBorder);
        fx.rgbSpeed     = p.getFloat("fx.rgbSpeed",      fx.rgbSpeed);

        /* ── UI ──────────────────────────────────────────────── */
        g_cfg.uiTheme        = p.getInt  ("ui.uiTheme",        g_cfg.uiTheme);
        g_cfg.catDarkMode    = p.getBool ("ui.catDarkMode",     g_cfg.catDarkMode);
        g_cfg.menuAlpha      = p.getFloat("ui.menuAlpha",       g_cfg.menuAlpha);
        g_cfg.menuLocked     = p.getBool ("ui.menuLocked",      g_cfg.menuLocked);
        g_cfg.menuGlow       = p.getBool ("ui.menuGlow",        g_cfg.menuGlow);
        g_cfg.menuGlowIntens = p.getFloat("ui.menuGlowIntens",  g_cfg.menuGlowIntens);
        g_cfg.espBoxGlow     = p.getBool ("ui.espBoxGlow",      g_cfg.espBoxGlow);
        g_cfg.espBoxGlowIntens=p.getFloat("ui.espBoxGlowIntens",g_cfg.espBoxGlowIntens);
        g_cfg.catCrosshair   = p.getBool ("ui.catCrosshair",    g_cfg.catCrosshair);
        g_cfg.showSpotify    = p.getBool ("ui.showSpotify",     g_cfg.showSpotify);
        g_cfg.showSession    = p.getBool ("ui.showSession",     g_cfg.showSession);
        g_cfg.showHearts     = p.getBool ("ui.showHearts",      g_cfg.showHearts);
        g_cfg.showPawTrail   = p.getBool ("ui.showPawTrail",    g_cfg.showPawTrail);
        g_cfg.catEarSize     = p.getFloat("ui.catEarSize",      g_cfg.catEarSize);
        g_cfg.catBoxFace     = p.getBool ("ui.catBoxFace",      g_cfg.catBoxFace);
        g_cfg.catBoxPaws     = p.getBool ("ui.catBoxPaws",      g_cfg.catBoxPaws);
    }

/* ─────────────────────────────────────────────────────────────────
   Public API
───────────────────────────────────────────────────────────────── */

    /* Returns "" on success, error string on failure */
    static std::string Save(const std::string& name) {
        std::string path = GetConfigPath(name);
        std::string json = Serialize();
        FILE* f = nullptr;
        fopen_s(&f, path.c_str(), "wb");
        if (!f) return "Could not open file for writing: " + path;
        fwrite(json.c_str(), 1, json.size(), f);
        fclose(f);
        return "";
    }

    static std::string Load(const std::string& name) {
        std::string path = GetConfigPath(name);
        FILE* f = nullptr;
        fopen_s(&f, path.c_str(), "rb");
        if (!f) return "Config not found: " + path;
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz <= 0 || sz > 1024*1024) { fclose(f); return "Invalid file size"; }
        std::string json(sz, '\0');
        fread(&json[0], 1, sz, f);
        fclose(f);
        Deserialize(json);
        return "";
    }

    static bool Delete(const std::string& name) {
        return DeleteFileA(GetConfigPath(name).c_str()) != 0;
    }

} // namespace ConfigIO
