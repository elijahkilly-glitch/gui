/* ════════════════════════════════════════════════════════════════════
 *  menu_theme_ios26.h  —  x9  iOS 26 Liquid Glass Theme
 *
 *  Design language: iOS 26 / Liquid Glass
 *    · Frosted translucent glass panels (lavender-white semi-opaque fills)
 *    · Soft pink-blue layered window gradient with top specular gloss
 *    · iOS-style toggle switches (spring-animated pill + white knob)
 *    · Blue-purple gradient slider tracks with glowing thumb
 *    · Pill-shaped active tabs with inner specular highlight + border
 *    · Vertical icon sidebar with glass active state
 *    · Glass action buttons and keybind pills
 *    · Status bar with Roblox detect + FPS counter
 *
 *  Namespace:  ThemeOriginal
 *  Public API: Toggle() IsVisible() ApplyTheme() Render()
 * ════════════════════════════════════════════════════════════════════ */
#pragma once
#include "x9_shared.h"
#include "config_ui.h"
#include <unordered_map>

namespace ThemeOriginal {

/* ── Visibility / active tab ───────────────────────────────────── */
static bool s_visible = true;
static int  s_tab     = 0;

/* ── UI State Variables ────────────────────────────────────────── */
static float s_gradTop[4] = { 0.91f, 0.84f, 0.96f, 0.72f }; 
static float s_gradBot[4] = { 0.78f, 0.51f, 0.86f, 0.62f }; 
static char  s_luaBuf[1024 * 64] = { 0 };
static char  s_log[1024 * 64]    = { 0 };
static void* s_catTexture = nullptr;

/* ── Dark mode + transparency ──────────────────────────────────── */
static bool  s_darkMode   = false;   /* false=light glass  true=dark osu! */
static float s_glassAlpha = 0.82f;   /* 0.0=fully transparent  1.0=opaque  */
static float s_blurStrength = 0.90f; /* 0=clear  1=heavy frost behind menu  */
static bool  s_blurEnabled  = true;  /* enable frosted blur simulation       */

/* ── Dark-mode colour accessors — call these instead of constants ─
 * Light: lavender-glass as before.
 * Dark : dark navy surfaces, rose/pink accent tint, white-ish text. */
static ImU32  DynCardBg()  { return s_darkMode ? IM_COL32(30,28,46,(int)(s_glassAlpha*200)) : IM_COL32(255,255,255,(int)(s_glassAlpha*36)); }
static ImU32  DynCardBdr() { return s_darkMode ? IM_COL32(255,255,255,35) : IM_COL32(255,255,255,102); }
static ImU32  DynSecHdr()  { return s_darkMode ? IM_COL32(20,18,36,(int)(s_glassAlpha*220)) : IM_COL32(255,255,255,26); }
static ImU32  DynSep()     { return s_darkMode ? IM_COL32(255,255,255,18) : IM_COL32(255,255,255,64); }
static ImU32  DynTxt()     { return s_darkMode ? IM_COL32(230,226,240,235) : IM_COL32(25,15,45,230); }
static ImU32  DynTxtSec()  { return s_darkMode ? IM_COL32(150,140,180,200) : IM_COL32(70,50,100,145); }
static ImU32  DynTxtDis()  { return s_darkMode ? IM_COL32(100,90,130,160) : IM_COL32(70,50,100,90); }
static ImU32  DynSidebar() { return s_darkMode ? IM_COL32(18,16,30,(int)(s_glassAlpha*230)) : IM_COL32(255,255,255,20); }
static ImVec4 DynWinBg()   { return s_darkMode
    ? ImVec4(22/255.f,20/255.f,36/255.f, s_glassAlpha)
    : ImVec4(225/255.f,215/255.f,245/255.f, s_glassAlpha); }

/* ════════════════════════════════════════════════════════════════
   PALETTE  —  iOS 26 Liquid Glass
   ════════════════════════════════════════════════════════════════ */
/* ── Glass surfaces (Translucency & Alpha Fix) ─────────────────── */
static constexpr ImU32 kGlassBg   = IM_COL32(255, 255, 255, 36);  /* card fill 0.14a */
static constexpr ImU32 kGlassBgSt = IM_COL32(255, 255, 255, 115); /* tab active 0.45a */
static constexpr ImU32 kSidebarBg = IM_COL32(255, 255, 255, 20);  /* sidebar bg 0.08a */
static constexpr ImU32 kSecHdrBg  = IM_COL32(255, 255, 255, 26);  /* section header 0.10a */

/* ── Borders & Highlights ─────────────────────────────────────── */
static constexpr ImU32 kGlassBdr  = IM_COL32(255, 255, 255, 140); /* strong border 0.55a */
static constexpr ImU32 kGlassBdrS = IM_COL32(255, 255, 255, 77);  /* subtle border 0.30a */
static constexpr ImU32 kSpecular  = IM_COL32(255, 255, 255, 230); /* top gloss highlight */
static constexpr ImU32 kSep       = IM_COL32(255, 255, 255, 64);  /* row separator 0.25a */

/* ── UI Element Aliases ───────────────────────────────────────── */
static constexpr ImU32 kBdr       = IM_COL32(255, 255, 255, 102); /* card border */
static constexpr ImU32 kBdrTop    = IM_COL32(255, 255, 255, 230); /* top specular */
static constexpr ImU32 kBdrMid    = IM_COL32(255, 255, 255, 64);  /* column divider */

/* ── Surfaces (Fov/Esp/Lua) ───────────────────────────────────── */
static constexpr ImU32 kSurf1     = IM_COL32(255, 255, 255, 26);  /* light glass */
static constexpr ImU32 kSurf2     = IM_COL32(255, 255, 255, 46);  /* medium glass */
static constexpr ImU32 kSurf3     = IM_COL32(255, 255, 255, 71);  /* stronger glass */

/* ── Text (Purple-Tinted for readability) ─────────────────────── */
static constexpr ImU32 kTxt       = IM_COL32( 25,  15,  45, 230); /* primary text */
static constexpr ImU32 kTxtSec    = IM_COL32( 70,  50, 100, 145); /* secondary text */
static constexpr ImU32 kTxtDis    = IM_COL32( 70,  50, 100,  90); /* disabled text */
static constexpr ImU32 kTxtHint   = IM_COL32( 70,  50, 100,  65); /* hint text */

/* ── Interaction Overlays ─────────────────────────────────────── */
static constexpr ImU32 kHov       = IM_COL32(255, 255, 255, 30);
static constexpr ImU32 kHovStr    = IM_COL32(255, 255, 255, 50);
static constexpr ImU32 kPress     = IM_COL32(255, 255, 255, 65);
static constexpr ImU32 kToggleOff = IM_COL32(185, 165, 215, 100);

/* ── ImGui Style Colors (Main Transparency Fix) ───────────────── */
// Critical: 0.82f alpha enables the translucent background.
static constexpr ImVec4 kV4WinBg  = { 225/255.f, 215/255.f, 245/255.f, 0.82f };
static constexpr ImVec4 kV4S2     = { 1.00f, 1.00f, 1.00f, 0.18f };
static constexpr ImVec4 kV4S3     = { 1.00f, 1.00f, 1.00f, 0.28f };
static constexpr ImVec4 kV4Txt    = {  25/255.f,  15/255.f,  45/255.f, 0.90f };
static constexpr ImVec4 kV4TxtS   = {  70/255.f,  50/255.f, 100/255.f, 0.57f };

/* ── Layout & Rounding (Rounding Fix) ─────────────────────────── */
static constexpr float kWR   = 24.0f; /* window corner rounding */
static constexpr float kCR   = 18.0f; /* card corner rounding */
static constexpr float kCR2  = 10.0f; /* control corner rounding */
static constexpr float kPad  = 14.0f; /* padding */
static constexpr float kRH   = 44.0f; /* row height */
static constexpr float kSRH  = 54.0f; /* slider height */
static constexpr float kSecH = 34.0f; /* section header height */
static constexpr float kHdrH = 52.0f; /* header height */
static constexpr float kTabH =  0.0f; 
static constexpr float kBarH = 28.0f; /* status bar height */
static constexpr float kSbW  = 56.0f; /* sidebar width

 *//* ════════════════════════════════════════════════════════════════
   ACCENT  —  iOS blue-purple defaults
   ════════════════════════════════════════════════════════════════ */
struct AccentDef { const char* name; float r,g,b; };
static AccentDef kAccents[] = {
    { "Iris",      0.357f,0.549f,1.000f }, /* #5b8cff default iOS blue-purple */
    { "Lavender",  0.580f,0.420f,1.000f }, /* #946bff purple                  */
    { "Rose",      1.000f,0.380f,0.580f }, /* #ff6194 pink                    */
    { "Teal",      0.200f,0.820f,0.820f }, /* #33d1d1 teal                    */
    { "Mint",      0.200f,0.820f,0.520f }, /* #33d185 green                   */
    { "Tangerine", 1.000f,0.580f,0.200f }, /* #ff9433 orange                  */
    { "Gold",      1.000f,0.780f,0.200f }, /* #ffc733 gold                    */
    { "Steel",     0.450f,0.500f,0.650f }, /* #7380a6 muted blue              */
};
static int   s_acIdx    = 0;
static float s_acR=0.357f,s_acG=0.549f,s_acB=1.000f;
static bool  s_acCustom = false;
static float s_acCust[3]= {0.357f,0.549f,1.000f};

static void SyncAccent(float r,float g,float b){s_acR=r;s_acG=g;s_acB=b;}

static ImU32 AC(int a=255){
    a=a<0?0:a>255?255:a;
    return IM_COL32((int)(s_acR*255),(int)(s_acG*255),(int)(s_acB*255),a);
}
static ImVec4 AC4(){return {s_acR,s_acG,s_acB,1.f};}

/* Purple-shifted end of accent gradient (for toggle/slider fills) */
static ImU32 ACPurp(int a=255){
    a=a<0?0:a>255?255:a;
    int r=(int)(fminf(s_acR+0.27f,1.f)*255);
    int g=(int)(fmaxf(s_acG-0.13f,0.f)*255);
    int b=(int)(s_acB*255);
    return IM_COL32(r,g,b,a);
}

/* Lighter hover shade */
static ImU32 ACH(int a=255){
    a=a<0?0:a>255?255:a;
    return IM_COL32(
        (int)(fminf(s_acR*1.3f,1.f)*255),
        (int)(fminf(s_acG*1.25f,1.f)*255),
        (int)(fminf(s_acB*1.1f,1.f)*255),a);
}

static void ApplyAccentToStyle(){
    auto& c=ImGui::GetStyle().Colors;
    c[ImGuiCol_SliderGrab]           ={s_acR,s_acG,s_acB,1.f};
    c[ImGuiCol_SliderGrabActive]     ={fminf(s_acR*1.2f,1.f),fminf(s_acG*1.2f,1.f),fminf(s_acB*1.1f,1.f),1.f};
    c[ImGuiCol_CheckMark]            ={s_acR,s_acG,s_acB,1.f};
    c[ImGuiCol_Header]               ={s_acR*.15f,s_acG*.15f,s_acB*.25f,0.9f};
    c[ImGuiCol_HeaderHovered]        ={s_acR*.22f,s_acG*.22f,s_acB*.35f,0.9f};
    c[ImGuiCol_HeaderActive]         ={s_acR*.30f,s_acG*.30f,s_acB*.45f,0.9f};
    c[ImGuiCol_ScrollbarGrab]        ={1,1,1,0.25f};
    c[ImGuiCol_ScrollbarGrabHovered] ={1,1,1,0.40f};
    c[ImGuiCol_ScrollbarGrabActive]  ={s_acR,s_acG,s_acB,0.8f};
    Loader::SetAccent(s_acR,s_acG,s_acB);
}

/* ════════════════════════════════════════════════════════════════
   APPLY THEME  —  ImGui style setup (glass / iOS)
   ════════════════════════════════════════════════════════════════ */
void ApplyTheme()
{
    ImGuiStyle& st=ImGui::GetStyle();
    st.WindowRounding    = kWR;
    st.ChildRounding     = kCR;
    st.FrameRounding     = kCR2;
    st.GrabRounding      = kCR;
    st.TabRounding       = kCR;
    st.ScrollbarRounding = kCR;
    st.PopupRounding     = kCR;
    st.WindowBorderSize  = 0.f;
    st.ChildBorderSize   = 0.f;
    st.FrameBorderSize   = 0.f;
    st.WindowPadding     = {0.f,0.f};
    st.FramePadding      = {10.f,6.f};
    st.ItemSpacing       = {8.f,4.f};
    st.ItemInnerSpacing  = {6.f,4.f};
    st.IndentSpacing     = 14.f;
    st.GrabMinSize       = 10.f;
    st.ScrollbarSize     = 3.f;

    auto& c=st.Colors;
    c[ImGuiCol_WindowBg]          = DynWinBg();
    c[ImGuiCol_ChildBg]           = {0,0,0,0};
    c[ImGuiCol_PopupBg]           = {0.90f,0.86f,0.97f,0.96f};
    c[ImGuiCol_Border]            = {1,1,1,0.30f};
    c[ImGuiCol_BorderShadow]      = {0,0,0,0};
    c[ImGuiCol_TitleBg]           = DynWinBg();
    c[ImGuiCol_TitleBgActive]     = DynWinBg();
    c[ImGuiCol_FrameBg]           = kV4S2;
    c[ImGuiCol_FrameBgHovered]    = kV4S3;
    c[ImGuiCol_FrameBgActive]     = {1,1,1,0.40f};
    c[ImGuiCol_Button]            = kV4S2;
    c[ImGuiCol_ButtonHovered]     = kV4S3;
    c[ImGuiCol_ButtonActive]      = {1,1,1,0.45f};
    c[ImGuiCol_ScrollbarBg]       = {0,0,0,0};
    c[ImGuiCol_Separator]         = {1,1,1,0.25f};
    c[ImGuiCol_Text]              = s_darkMode
        ? ImVec4(230/255.f,226/255.f,240/255.f,0.92f)
        : kV4Txt;
    c[ImGuiCol_TextDisabled]      = s_darkMode
        ? ImVec4(150/255.f,140/255.f,180/255.f,0.78f)
        : kV4TxtS;
    st.AntiAliasedLines = true;
    st.AntiAliasedFill = true;
    ApplyAccentToStyle();
}

/* ════════════════════════════════════════════════════════════════
   LABEL HELPER  —  strip ImGui ## id suffix for DrawList text
   ════════════════════════════════════════════════════════════════ */
static const char* LabelEnd(const char* label)
{
    const char* h=strstr(label,"##");
    return h ? h : nullptr; /* nullptr = render to null terminator */
}

/* ════════════════════════════════════════════════════════════════
   CARD HELPERS  —  glass panel, channel-split so bg draws behind
   ════════════════════════════════════════════════════════════════ */
static float       s_cardTopY = 0.f;
static float       s_cardX    = 0.f;
static float       s_cardW    = 0.f;
static bool        s_inCard   = false;
static ImDrawList* s_dl       = nullptr;

static void CardBegin(float x, float w)
{
    s_cardTopY = ImGui::GetCursorScreenPos().y;
    s_cardX    = x;
    s_cardW    = w;
    s_inCard   = true;
    s_dl       = ImGui::GetWindowDrawList();
    s_dl->ChannelsSplit(2);
    s_dl->ChannelsSetCurrent(1);
    /* No leading dummy — first SecHdr call becomes the card header row */
}

static void CardEnd()
{
    if(!s_inCard||!s_dl) return;
    float botY=ImGui::GetCursorScreenPos().y+2.f;
    s_dl->ChannelsSetCurrent(0);

    /* Glass fill — respects dark mode + alpha */
    s_dl->AddRectFilled({s_cardX,s_cardTopY},{s_cardX+s_cardW,botY},DynCardBg(),kCR);

    /* Card border */
    s_dl->AddRect({s_cardX,s_cardTopY},{s_cardX+s_cardW,botY},
        DynCardBdr(),kCR,0,1.f);

    /* Top specular line */
    s_dl->AddLine(
        {s_cardX+kCR,  s_cardTopY+0.5f},
        {s_cardX+s_cardW-kCR, s_cardTopY+0.5f},
        IM_COL32(255,255,255,140),1.f);

    /* Soft outer shadow */
    for(int i=1;i<=3;i++){
        float s=(float)i;
        s_dl->AddRect(
            {s_cardX-s, s_cardTopY-s},
            {s_cardX+s_cardW+s, botY+s},
            IM_COL32(80,60,120,(int)(12/i)),kCR+s,0,1.f);
    }

    s_dl->ChannelsMerge();
    s_inCard=false;
    ImGui::Dummy({0,10.f});
}

/* ════════════════════════════════════════════════════════════════
   SECTION HEADER  —  iOS glass card sub-header row
   ════════════════════════════════════════════════════════════════ */
static void SecHdr(const char* label)
{
    ImVec2 p  =ImGui::GetCursorScreenPos();
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w  =ImGui::GetContentRegionAvail().x;
    float  lh =ImGui::GetTextLineHeight();

    /* Section header bg — dynamic */
    dl->AddRectFilled(p,{p.x+w,p.y+kSecH},DynSecHdr());
    dl->AddLine({p.x,p.y+kSecH-0.5f},{p.x+w,p.y+kSecH-0.5f},DynSep(),0.5f);

    /* Title — accent colour in dark mode (like ref image pink headers) */
    ImU32 hdrTxtCol = s_darkMode ? AC(220) : DynTxtSec();
    dl->AddText({p.x+kPad,p.y+(kSecH-lh)*0.5f},hdrTxtCol,label);

    /* Chevron ^ */
    dl->AddText({p.x+w-kPad-7.f,p.y+(kSecH-lh)*0.5f},kTxtDis,"^");

    ImGui::Dummy({0,kSecH});
}

/* ════════════════════════════════════════════════════════════════
   TOGGLE SWITCH  —  iOS 26 animated pill + spring knob
   ════════════════════════════════════════════════════════════════ */
static std::unordered_map<const void*,float> s_toggleAnim;

static void WinToggle(const char* label, bool& val,
    int* key=nullptr, const char* kb_id=nullptr)
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w =ImGui::GetContentRegionAvail().x;
    ImVec2 p =ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    /* Toggle dimensions */
    constexpr float TW=44.f,TH=26.f,KS=20.f;
    constexpr float kbW=46.f,kbGap=6.f;
    float hitW=(key&&kb_id)?w-kbW-kbGap:w;

    /* Hit target */
    char bid[80]; snprintf(bid,sizeof(bid),"##tog_%s",label);
    ImGui::InvisibleButton(bid,{hitW,kRH});
    bool hov=ImGui::IsItemHovered();
    bool clk=ImGui::IsItemClicked();
    if(clk) val=!val;

    /* Spring-physics animation */
    float& t=s_toggleAnim[&val];
    float  tgt=val?1.f:0.f;
    t+=(tgt-t)*ImGui::GetIO().DeltaTime*12.f;
    t=t<0.f?0.f:t>1.f?1.f:t;

    /* Row hover highlight */
    if(hov) dl->AddRectFilled(p,{p.x+hitW,p.y+kRH},kHov);

    /* ── Label — left side ────────────────────────────────── */
    dl->AddText({p.x+kPad, p.y+(kRH-lh)*0.5f}, kTxt, label, LabelEnd(label));

    /* ── Toggle pill — right side ─────────────────────────── */
    float tx=p.x+hitW-kPad-TW;
    float ty=p.y+(kRH-TH)*0.5f;

    if(val){
        /* ON: accent blue base + purple gradient overlay */
        dl->AddRectFilled({tx,ty},{tx+TW,ty+TH},AC(255),TH*0.5f);
        dl->AddRectFilledMultiColor(
            {tx,ty},{tx+TW,ty+TH},
            IM_COL32(0,0,0,0), ACPurp(180),
            ACPurp(180),       IM_COL32(0,0,0,0));
        /* Glassy border */
        dl->AddRect({tx,ty},{tx+TW,ty+TH},IM_COL32(255,255,255,153),TH*0.5f,0,1.f);
        /* Outer accent glow rings */
        dl->AddRect({tx-2.f,ty-2.f},{tx+TW+2.f,ty+TH+2.f},AC(28),TH*0.5f+2.f,0,1.f);
        dl->AddRect({tx-4.f,ty-4.f},{tx+TW+4.f,ty+TH+4.f},AC(10),TH*0.5f+4.f,0,1.f);
    } else {
        /* OFF: glass gray-purple */
        dl->AddRectFilled({tx,ty},{tx+TW,ty+TH},kToggleOff,TH*0.5f);
        dl->AddRect({tx,ty},{tx+TW,ty+TH},IM_COL32(255,255,255,77),TH*0.5f,0,1.f);
    }

    /* Knob — white circle with drop shadow + inner highlight */
    float kCX=tx+3.f+KS*0.5f+(TW-KS-6.f)*t;
    float kCY=ty+TH*0.5f;
    dl->AddCircleFilled({kCX+0.5f,kCY+1.5f},KS*0.5f,IM_COL32(0,0,0,45),20); /* shadow */
    dl->AddCircleFilled({kCX,kCY},KS*0.5f,IM_COL32(255,255,255,255),20);      /* fill   */
    dl->AddCircleFilled({kCX,kCY-1.5f},KS*0.5f*0.55f,IM_COL32(255,255,255,50),16); /* inner gloss */

    /* ── Keybind pill ─────────────────────────────────────── */
    if(key&&kb_id){
        static const void* s_bindId=nullptr;
        bool listening=(s_bindId==key);
        if(listening&&GetAsyncKeyState(VK_ESCAPE)&0x8000){s_bindId=nullptr;listening=false;}
        if(listening){
            const int kS[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,
                VK_LSHIFT,VK_RSHIFT,VK_LCONTROL,VK_RCONTROL,
                VK_DELETE,VK_INSERT,VK_MENU,VK_SPACE,VK_TAB,
                'A','B','C','D','E','F','G','H','I','J','K','L','M',
                'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
                VK_F1,VK_F2,VK_F3,VK_F4,VK_F5,VK_F6,
                VK_F7,VK_F8,VK_F9,VK_F10,VK_F11,VK_F12};
            for(int k:kS)if(GetAsyncKeyState(k)&0x8000){*key=k;s_bindId=nullptr;listening=false;break;}
        }
        char kl[24];
        if(listening){snprintf(kl,sizeof(kl),"...");}
        else{
            const char* kn=
                *key==VK_RBUTTON?"RMB":*key==VK_LBUTTON?"LMB":*key==VK_MBUTTON?"MMB":
                *key==VK_LSHIFT?"LShift":*key==VK_RSHIFT?"RShift":
                *key==VK_LCONTROL?"LCtrl":*key==VK_RCONTROL?"RCtrl":
                *key==VK_DELETE?"Del":*key==VK_INSERT?"Ins":
                *key==VK_MENU?"Alt":*key==VK_SPACE?"Space":
                *key==VK_TAB?"Tab":*key==0?"-":nullptr;
            if(kn)snprintf(kl,sizeof(kl),"%s",kn);
            else if(*key>='A'&&*key<='Z')snprintf(kl,sizeof(kl),"%c",(char)(*key+32));
            else if(*key>=VK_F1&&*key<=VK_F12)snprintf(kl,sizeof(kl),"F%d",*key-VK_F1+1);
            else snprintf(kl,sizeof(kl),"K%d",*key);
        }
        ImVec2 kls=ImGui::CalcTextSize(kl);
        float  kpx=p.x+w-kbGap-kls.x-14.f;
        float  kpy=p.y+(kRH-22.f)*0.5f;
        ImGui::SetCursorScreenPos({kpx-1.f,kpy-1.f});
        ImGui::InvisibleButton(kb_id,{kls.x+16.f,24.f});
        if(ImGui::IsItemClicked()) s_bindId=listening?nullptr:key;
        if(ImGui::IsItemClicked(ImGuiMouseButton_Middle)) *key=0;
        bool kh=ImGui::IsItemHovered();
        /* Glass pill */
        dl->AddRectFilled({kpx-1.f,kpy-1.f},{kpx+kls.x+15.f,kpy+23.f},
            listening?AC(40):(kh?IM_COL32(255,255,255,55):IM_COL32(255,255,255,28)),kCR2);
        dl->AddRect({kpx-1.f,kpy-1.f},{kpx+kls.x+15.f,kpy+23.f},
            listening?AC(130):IM_COL32(255,255,255,90),kCR2,0,1.f);
        dl->AddText({kpx+7.f,kpy+4.f},listening?AC(255):kTxtSec,kl);
    }

    /* Row separator */
    ImGui::SetCursorScreenPos({p.x,p.y+kRH});
    dl->AddLine({p.x+kPad,p.y+kRH-0.5f},{p.x+w,p.y+kRH-0.5f},kSep,0.5f);
    ImGui::Dummy({0,1.f});
}

/* Public alias */
static void RowCB(const char* label, bool& val,
    int* key=nullptr, const char* kb_id=nullptr)
{ WinToggle(label,val,key,kb_id); }

/* ════════════════════════════════════════════════════════════════
   SLIDER FLOAT  —  glass track + gradient fill + glowing thumb
   ════════════════════════════════════════════════════════════════ */
static void WinSliderF(const char* label, float& v, float lo, float hi, const char* fmt="%.1f")
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w =ImGui::GetContentRegionAvail().x;
    ImVec2 p =ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    /* ── Label row ────────────────────────────────────────── */
    dl->AddText({p.x+kPad,p.y+(kRH*0.5f-lh*0.5f)},kTxt,label,LabelEnd(label));
    ImGui::Dummy({0,kRH});

    /* ── Track row ────────────────────────────────────────── */
    constexpr float kTH=5.f, kThumb=22.f;
    float tx=p.x+kPad, tw=w-kPad*2.f;
    /* ty = cursor_y + FramePadding.y + GrabSize/2 - kTH/2
     * FramePadding.y = kTH/2, so: cursor + kTH/2 + kThumb/2 - kTH/2 = cursor + kThumb/2 */
    float ty = p.y + kRH + kThumb * 0.5f - kTH * 0.5f;

    /* Invisible slider widget */
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,kThumb);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0.f,kTH*0.5f});
    ImGui::PushStyleColor(ImGuiCol_FrameBg,{0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,{0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,{0,0,0,0});
    ImGui::SetNextItemWidth(tw);
    char sid[72]; snprintf(sid,sizeof(sid),"##wsf_%s",label);
    ImGui::SliderFloat(sid,&v,lo,hi,"");
    bool act=ImGui::IsItemActive(), hov=ImGui::IsItemHovered();
    ImGui::PopStyleColor(3); ImGui::PopStyleVar(2);

    float norm=(v-lo)/(hi-lo);
    norm=norm<0.f?0.f:norm>1.f?1.f:norm;
    float fillX=tx+tw*norm;

    /* Track background — frosted glass */
    dl->AddRectFilled({tx,ty},{tx+tw,ty+kTH},IM_COL32(255,255,255,56),kTH*0.5f);
    dl->AddRect({tx,ty},{tx+tw,ty+kTH},IM_COL32(255,255,255,90),kTH*0.5f,0,0.5f);

    /* Fill — accent gradient */
    if(norm>0.f){
        dl->AddRectFilled({tx,ty},{fillX,ty+kTH},AC(220+(act?35:hov?15:0)),kTH*0.5f);
        dl->AddRectFilledMultiColor({tx,ty},{fillX,ty+kTH},
            IM_COL32(0,0,0,0), ACPurp(160), ACPurp(160), IM_COL32(0,0,0,0));
        /* Fill glow */
        dl->AddRect({tx,ty-1.f},{fillX,ty+kTH+1.f},AC(18),kTH*0.5f+1.f,0,1.f);
    }

    /* Thumb — white with shadow + optional glow */
    if(act||hov) dl->AddCircleFilled({fillX,ty+kTH*0.5f},kThumb*0.5f+2.5f,AC(22),22);
    dl->AddCircleFilled({fillX,ty+kTH*0.5f+1.5f},kThumb*0.5f,IM_COL32(0,0,0,38),20);
    dl->AddCircleFilled({fillX,ty+kTH*0.5f},kThumb*0.5f,IM_COL32(255,255,255,255),20);
    dl->AddCircleFilled({fillX,ty+kTH*0.5f-1.5f},kThumb*0.5f*0.50f,IM_COL32(255,255,255,48),16);

    /* Value display — top-right of label row */
    char vb[20]; snprintf(vb,sizeof(vb),fmt,v);
    ImVec2 vs=ImGui::CalcTextSize(vb);
    dl->AddText({p.x+w-kPad-vs.x,p.y+(kRH*0.5f-lh*0.5f)},kTxtSec,vb);

    ImVec2 p2=ImGui::GetCursorScreenPos();
    dl->AddLine({p.x+kPad,p2.y},{p.x+w,p2.y},kSep,0.5f);
    ImGui::Dummy({0,5.f});
}

/* ════════════════════════════════════════════════════════════════
   SLIDER INT  —  same glass style
   ════════════════════════════════════════════════════════════════ */
static void WinSliderI(const char* label, int& v, int lo, int hi)
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w =ImGui::GetContentRegionAvail().x;
    ImVec2 p =ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    /* Label + value row */
    char vb[12]; snprintf(vb,sizeof(vb),"%d",v);
    ImVec2 vs=ImGui::CalcTextSize(vb);
    dl->AddText({p.x+kPad,p.y+(kRH*0.5f-lh*0.5f)},kTxt,label,LabelEnd(label));
    dl->AddText({p.x+w-kPad-vs.x,p.y+(kRH*0.5f-lh*0.5f)},kTxtSec,vb);
    ImGui::Dummy({0,kRH});

    constexpr float kTH=5.f,kThumb=22.f;
    float tx=p.x+kPad,tw=w-kPad*2.f;
    float ty = p.y + kRH + kThumb * 0.5f - kTH * 0.5f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,kThumb);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0.f,kTH*0.5f});
    ImGui::PushStyleColor(ImGuiCol_FrameBg,{0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,{0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,{0,0,0,0});
    ImGui::SetNextItemWidth(tw);
    char sid[72]; snprintf(sid,sizeof(sid),"##wsi_%s",label);
    ImGui::SliderInt(sid,&v,lo,hi,"");
    bool act=ImGui::IsItemActive(),hov=ImGui::IsItemHovered();
    ImGui::PopStyleColor(3); ImGui::PopStyleVar(2);

    float norm=(float)(v-lo)/(float)(hi-lo);
    norm=norm<0.f?0.f:norm>1.f?1.f:norm;
    float fillX=tx+tw*norm;

    dl->AddRectFilled({tx,ty},{tx+tw,ty+kTH},IM_COL32(255,255,255,56),kTH*0.5f);
    dl->AddRect({tx,ty},{tx+tw,ty+kTH},IM_COL32(255,255,255,90),kTH*0.5f,0,0.5f);
    if(norm>0.f){
        dl->AddRectFilled({tx,ty},{fillX,ty+kTH},AC(220+(act?35:hov?15:0)),kTH*0.5f);
        dl->AddRectFilledMultiColor({tx,ty},{fillX,ty+kTH},
            IM_COL32(0,0,0,0), ACPurp(160), ACPurp(160), IM_COL32(0,0,0,0));
        dl->AddRect({tx,ty-1.f},{fillX,ty+kTH+1.f},AC(18),kTH*0.5f+1.f,0,1.f);
    }
    if(act||hov) dl->AddCircleFilled({fillX,ty+kTH*0.5f},kThumb*0.5f+2.5f,AC(22),22);
    dl->AddCircleFilled({fillX,ty+kTH*0.5f+1.5f},kThumb*0.5f,IM_COL32(0,0,0,38),20);
    dl->AddCircleFilled({fillX,ty+kTH*0.5f},kThumb*0.5f,IM_COL32(255,255,255,255),20);
    dl->AddCircleFilled({fillX,ty+kTH*0.5f-1.5f},kThumb*0.5f*0.5f,IM_COL32(255,255,255,48),16);

    ImVec2 p2=ImGui::GetCursorScreenPos();
    dl->AddLine({p.x+kPad,p2.y},{p.x+w,p2.y},kSep,0.5f);
    ImGui::Dummy({0,5.f});
}

/* Public aliases */
static void RowSlideF(const char* l,float& v,float lo,float hi,const char* f="%.1f")
{ WinSliderF(l,v,lo,hi,f); }
static void RowSlideI(const char* l,int& v,int lo,int hi)
{ WinSliderI(l,v,lo,hi); }

/* ════════════════════════════════════════════════════════════════
   COMBO  —  glass dropdown
   ════════════════════════════════════════════════════════════════ */
static bool RowCombo(const char* label, int& v, const char** items, int n)
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    if(label&&label[0]){
        dl->AddText({p.x+kPad,p.y+(kRH*0.5f-lh*0.5f)},kTxtSec,label,LabelEnd(label));
        ImGui::Dummy({0,kRH-4.f});
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,kCR2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{10.f,6.f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,    kV4S2);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,kV4S3);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,{1,1,1,0.40f});
    ImGui::PushStyleColor(ImGuiCol_Border,     {1,1,1,0.40f});
    ImGui::PushStyleColor(ImGuiCol_Button,     kV4S2);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,kV4S3);
    ImGui::SetNextItemWidth(w-kPad*2.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    char cid[72]; snprintf(cid,sizeof(cid),"##cbo_%s",label);
    bool ch=ImGui::Combo(cid,&v,items,n);
    ImGui::PopStyleColor(6); ImGui::PopStyleVar(3);

    ImVec2 p2=ImGui::GetCursorScreenPos();
    dl->AddLine({p.x+kPad,p2.y},{p.x+w,p2.y},kSep,0.5f);
    ImGui::Dummy({0,5.f});
    return ch;
}

/* ════════════════════════════════════════════════════════════════
   COLOR ROW  —  glass swatch
   ════════════════════════════════════════════════════════════════ */
static void RowColor(const char* label, float col[3])
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    char bid[80]; snprintf(bid,sizeof(bid),"##colrow_%s",label);
    ImGui::InvisibleButton(bid,{w,kRH});
    bool hov=ImGui::IsItemHovered();
    if(hov) dl->AddRectFilled(p,{p.x+w,p.y+kRH},kHov);

    dl->AddText({p.x+kPad,p.y+(kRH-lh)*0.5f},kTxt,label,LabelEnd(label));

    /* Color swatch — glass rounded square */
    constexpr float sw=20.f;
    float sx=p.x+w-kPad-sw, sy=p.y+(kRH-sw)*0.5f;
    ImU32 fc=IM_COL32((int)(col[0]*255),(int)(col[1]*255),(int)(col[2]*255),255);
    dl->AddRectFilled({sx,sy},{sx+sw,sy+sw},fc,6.f);
    dl->AddRect({sx,sy},{sx+sw,sy+sw},IM_COL32(255,255,255,120),6.f,0,1.f);
    /* Inner gloss on swatch */
    dl->AddLine({sx+4.f,sy+2.f},{sx+sw-4.f,sy+2.f},IM_COL32(255,255,255,80),1.f);

    if(ImGui::IsItemClicked()) ImGui::OpenPopup(label);
    if(ImGui::BeginPopup(label)){
        ImGui::ColorPicker3(label,col,ImGuiColorEditFlags_NoSmallPreview);
        ImGui::EndPopup();
    }
    dl->AddLine({p.x+kPad,p.y+kRH-0.5f},{p.x+w,p.y+kRH-0.5f},kSep,0.5f);
    ImGui::SetCursorScreenPos({p.x,p.y+kRH});
    ImGui::Dummy({0,1.f});
}

/* ════════════════════════════════════════════════════════════════
   ACTION BUTTON  —  glass pill button
   ════════════════════════════════════════════════════════════════ */
static bool ActBtn(const char* label, bool accent=false)
{
    float bw=ImGui::GetContentRegionAvail().x-kPad*2.f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);

    ImVec4 bg,bgH,bgA;
    if(accent){
        bg  ={s_acR*0.85f,s_acG*0.85f,s_acB*0.85f,0.75f};
        bgH ={s_acR*0.95f,s_acG*0.95f,s_acB*0.95f,0.85f};
        bgA ={s_acR,s_acG,s_acB,1.f};
    } else {
        bg  =kV4S2;
        bgH =kV4S3;
        bgA ={1,1,1,0.45f};
    }
    ImVec4 textCol = accent ? ImVec4(1,1,1,1) : kV4Txt;

    ImGui::PushStyleColor(ImGuiCol_Button,       bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,bgH);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgA);
    ImGui::PushStyleColor(ImGuiCol_Text,         textCol);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,kCR2);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
    ImGui::PushStyleColor(ImGuiCol_Border,{1,1,1,0.40f});
    bool clicked=ImGui::Button(label,{bw,28.f});
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
    ImGui::Dummy({0,4.f});
    return clicked;
}

/* ════════════════════════════════════════════════════════════════
   SIDEBAR ICON DRAWING
   ════════════════════════════════════════════════════════════════ */
static void DrawSidebarIcon(ImDrawList* dl, float cx, float cy, int idx, bool active)
{
    ImU32 c = active ? kTxt : kTxtSec;
    constexpr float s=7.f;
    switch(idx){
    case 0: /* Combat — crosshair */
        dl->AddCircle({cx,cy},s,c,18,1.1f);
        dl->AddCircleFilled({cx,cy},2.f,c,8);
        dl->AddLine({cx-s-3.f,cy},{cx-3.f,cy},c,1.f);
        dl->AddLine({cx+3.f,cy},{cx+s+3.f,cy},c,1.f);
        dl->AddLine({cx,cy-s-3.f},{cx,cy-3.f},c,1.f);
        dl->AddLine({cx,cy+3.f},{cx,cy+s+3.f},c,1.f);
        break;
    case 1: /* Visuals — eye (lens + iris + pupil) */
        for(int a=0;a<24;a++){
            float a1=(float)a/24.f*6.28318f;
            float a2=(float)(a+1)/24.f*6.28318f;
            dl->AddLine({cx+cosf(a1)*(s+1.5f),cy+sinf(a1)*(s-2.f)},
                        {cx+cosf(a2)*(s+1.5f),cy+sinf(a2)*(s-2.f)},c,1.f);
        }
        dl->AddCircle({cx,cy},3.5f,c,12,1.f);
        dl->AddCircleFilled({cx,cy},1.5f,c,8);
        break;
    case 2: /* World — globe */
        dl->AddCircle({cx,cy},s,c,20,1.f);
        dl->AddLine({cx-s,cy},{cx+s,cy},c,0.8f);
        dl->AddLine({cx,cy-s},{cx,cy+s},c,0.8f);
        /* Latitude arc */
        for(int a=-6;a<6;a++){
            float t1=(float)a/6.f, t2=(float)(a+1)/6.f;
            float x1=t1*s*0.85f, y1=-3.5f+sinf(t1*3.14159f)*2.f;
            float x2=t2*s*0.85f, y2=-3.5f+sinf(t2*3.14159f)*2.f;
            dl->AddLine({cx+x1,cy+y1},{cx+x2,cy+y2},c,0.7f);
        }
        break;
    case 3: /* Movement — arrow up */
        dl->AddLine({cx,cy-s-1.f},{cx-s+1.f,cy+1.f},c,1.3f);
        dl->AddLine({cx,cy-s-1.f},{cx+s-1.f,cy+1.f},c,1.3f);
        dl->AddLine({cx,cy-s-1.f},{cx,cy+s+1.f},c,1.3f);
        break;
    case 4: /* Options — gear (inner circle + outer teeth) */
        dl->AddCircle({cx,cy},4.5f,c,14,1.1f);
        for(int i=0;i<8;i++){
            float a=(float)i/8.f*6.28318f;
            float r1=5.5f,r2=s+1.f;
            dl->AddLine({cx+cosf(a)*r1,cy+sinf(a)*r1},{cx+cosf(a)*r2,cy+sinf(a)*r2},c,1.5f);
        }
        break;
    case 5: /* Config — document with lines */
        dl->AddRect({cx-s+1.f,cy-s},{cx+s-1.f,cy+s},c,2.f,0,1.f);
        dl->AddLine({cx-4.f,cy-3.f},{cx+4.f,cy-3.f},c,1.f);
        dl->AddLine({cx-4.f,cy},{cx+4.f,cy},c,1.f);
        dl->AddLine({cx-4.f,cy+3.f},{cx+3.f,cy+3.f},c,1.f);
        break;
    case 6: /* Lua — code brackets < / > */
        dl->AddLine({cx-4.f,cy-4.f},{cx-8.f,cy},{c},1.2f);
        dl->AddLine({cx-8.f,cy},{cx-4.f,cy+4.f},{c},1.2f);
        dl->AddLine({cx+4.f,cy-4.f},{cx+8.f,cy},{c},1.2f);
        dl->AddLine({cx+8.f,cy},{cx+4.f,cy+4.f},{c},1.2f);
        dl->AddLine({cx+2.f,cy-5.f},{cx-2.f,cy+5.f},{c},1.1f);
        break;
    }
}

/* ════════════════════════════════════════════════════════════════
   SIDEBAR  —  icon strip on left edge
   ════════════════════════════════════════════════════════════════ */
static void DrawSidebar(ImDrawList* dl, ImVec2 wp, float wh)
{
    constexpr int kN=7;

    /* Sidebar background */
    dl->AddRectFilled(wp,{wp.x+kSbW,wp.y+wh},DynSidebar(),kWR,ImDrawFlags_RoundCornersLeft);
    dl->AddRectFilled({wp.x+kSbW-1.f,wp.y},{wp.x+kSbW,wp.y+wh},kGlassBdrS);

    /* Tab icon buttons — start near top, no avatar above them */
    constexpr float iconSz=36.f, iconR=10.f;
    float startY=wp.y+10.f;

    ImGui::SetCursorScreenPos({wp.x,wp.y});
    ImGui::Dummy({kSbW,10.f});

    for(int i=0;i<kN;i++){
        float ix=wp.x+(kSbW-iconSz)*0.5f;
        float iy=startY+(float)i*(iconSz+4.f);
        bool  act=(s_tab==i);

        /* Active background — glass pill */
        if(act){
            dl->AddRectFilled({ix,iy},{ix+iconSz,iy+iconSz},
                IM_COL32(255,255,255,90),iconR);
            dl->AddRect({ix,iy},{ix+iconSz,iy+iconSz},
                IM_COL32(255,255,255,160),iconR,0,1.f);
            dl->AddLine({ix+iconR,iy+0.5f},{ix+iconSz-iconR,iy+0.5f},
                IM_COL32(255,255,255,180),1.f);
        }

        /* Draw icon shape */
        DrawSidebarIcon(dl, ix+iconSz*0.5f, iy+iconSz*0.5f, i, act);

        /* Hit area */
        ImGui::SetCursorScreenPos({ix,iy});
        char sbid[16]; snprintf(sbid,sizeof(sbid),"##sb%d",i);
        ImGui::InvisibleButton(sbid,{iconSz,iconSz});
        if(ImGui::IsItemHovered()&&!act)
            dl->AddRectFilled({ix,iy},{ix+iconSz,iy+iconSz},kHov,iconR);
        if(ImGui::IsItemClicked()) s_tab=i;
    }
}

/* ════════════════════════════════════════════════════════════════
   TAB BAR  —  horizontal pill tabs in 52 px header strip
   ════════════════════════════════════════════════════════════════ */
static const char* kTabLabels[]={"Combat","Visuals","World","Movement","Options","Config","Lua"};
static constexpr int kTabCount=7;

static void DrawTabBar(ImDrawList* dl, float barX, float barW, float barY)
{
    float  lh=ImGui::GetTextLineHeight();

    /* Tab bar glass background */
    dl->AddRectFilled({barX,barY},{barX+barW,barY+kHdrH},kSidebarBg);
    /* Bottom separator */
    dl->AddLine({barX,barY+kHdrH-0.5f},{barX+barW,barY+kHdrH-0.5f},kGlassBdrS,0.5f);

    /* Tab pills */
    float tabStartX=barX+kPad;
    float tabH=30.f;
    float tabY=barY+(kHdrH-tabH)*0.5f;

    /* Pre-measure all tab widths */
    float tabWidths[kTabCount];
    float totalW=0.f;
    for(int i=0;i<kTabCount;i++){
        tabWidths[i]=ImGui::CalcTextSize(kTabLabels[i]).x+24.f;
        totalW+=tabWidths[i]+(i<kTabCount-1?4.f:0.f);
    }

    float tx=tabStartX;
    ImGui::SetCursorScreenPos({tabStartX,tabY});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{4.f,0.f});

    for(int i=0;i<kTabCount;i++){
        float tw=tabWidths[i];
        bool  act=(s_tab==i);

        if(act){
            /* Active: glass pill */
            dl->AddRectFilled({tx,tabY},{tx+tw,tabY+tabH},kGlassBgSt,tabH*0.5f);
            dl->AddRect({tx,tabY},{tx+tw,tabY+tabH},IM_COL32(255,255,255,179),tabH*0.5f,0,1.f);
            /* Inner specular */
            dl->AddLine({tx+tabH*0.5f,tabY+0.5f},{tx+tw-tabH*0.5f,tabY+0.5f},
                IM_COL32(255,255,255,200),1.f);
        }

        /* Label */
        ImVec2 ts=ImGui::CalcTextSize(kTabLabels[i]);
        dl->AddText({tx+tw*0.5f-ts.x*0.5f,tabY+(tabH-lh)*0.5f},
            act?kTxt:(ImGui::IsMouseHoveringRect({tx,tabY},{tx+tw,tabY+tabH})?kTxtSec:kTxtDis),
            kTabLabels[i]);

        /* Click target */
        ImGui::SetCursorScreenPos({tx,tabY});
        char bid[16]; snprintf(bid,sizeof(bid),"##tab%d",i);
        ImGui::InvisibleButton(bid,{tw,tabH});
        if(ImGui::IsItemClicked()) s_tab=i;
        if(ImGui::IsItemHovered()&&!act)
            dl->AddRectFilled({tx,tabY},{tx+tw,tabY+tabH},kHov,tabH*0.5f);

        tx+=tw+4.f;
    }
    ImGui::PopStyleVar();

    /* Search pill — right side */
    float spW=88.f, spH=28.f;
    float spX=barX+barW-kPad-spW;
    float spY=barY+(kHdrH-spH)*0.5f;
    dl->AddRectFilled({spX,spY},{spX+spW,spY+spH},IM_COL32(255,255,255,45),spH*0.5f);
    dl->AddRect({spX,spY},{spX+spW,spY+spH},IM_COL32(255,255,255,115),spH*0.5f,0,1.f);
    /* Magnifier dot */
    dl->AddCircle({spX+14.f,spY+spH*0.5f},4.5f,kTxtDis,10,1.f);
    dl->AddLine({spX+17.f,spY+spH*0.5f+3.f},{spX+20.f,spY+spH*0.5f+6.f},kTxtDis,1.f);
    dl->AddText({spX+24.f,spY+(spH-lh)*0.5f},kTxtDis,"Search");
}

/* ════════════════════════════════════════════════════════════════
   STATUS BAR  —  glass bottom strip
   ════════════════════════════════════════════════════════════════ */
static void DrawStatusBar(ImDrawList* dl, ImVec2 wp, float ww, float wh)
{
    float y0=wp.y+wh-kBarH;
    /* Glass bg */
    dl->AddRectFilled({wp.x,y0},{wp.x+ww,wp.y+wh},kSidebarBg,kWR,ImDrawFlags_RoundCornersBottom);
    dl->AddRectFilled({wp.x,y0},{wp.x+ww,y0+kWR},kSidebarBg); /* square top edge */
    dl->AddLine({wp.x,y0},{wp.x+ww,y0},kGlassBdrS,0.5f);

    float fsS=ImGui::GetFontSize()*0.74f;
    float ty=y0+(kBarH-fsS)*0.5f;

    /* Roblox status */
    bool robloxOk=(Roblox::GetPID()!=0);
    ImU32 dotC=robloxOk?IM_COL32(80,220,120,220):IM_COL32(180,160,200,90);
    /* Status dot with glow */
    if(robloxOk){
        dl->AddCircleFilled({wp.x+11.f,y0+kBarH*0.5f},5.5f,IM_COL32(80,220,120,40),10);
        dl->AddCircleFilled({wp.x+11.f,y0+kBarH*0.5f},3.f,IM_COL32(80,220,120,220),10);
    } else {
        dl->AddCircleFilled({wp.x+11.f,y0+kBarH*0.5f},3.f,dotC,10);
    }
    dl->AddText(ImGui::GetFont(),fsS,{wp.x+20.f,ty},
        robloxOk?IM_COL32(100,220,140,200):kTxtDis,
        robloxOk?"Injected":"Not detected");

    /* Center — site */
    const char* site="x9.gg";
    float sw2=ImGui::CalcTextSize(site).x*(fsS/ImGui::GetFontSize());
    dl->AddText(ImGui::GetFont(),fsS,{wp.x+ww*0.5f-sw2*0.5f,ty},kTxtHint,site);

    /* Right — version + fps */
    static int s_fps=0,s_fc=0; static DWORD s_ft=0;
    DWORD now=GetTickCount(); s_fc++;
    if(now-s_ft>=1000){s_fps=s_fc;s_fc=0;s_ft=now;}
    char fb[48]; snprintf(fb,sizeof(fb),"version-b8771a  ·  %d FPS",s_fps);
    float fw=ImGui::CalcTextSize(fb).x*(fsS/ImGui::GetFontSize());
    dl->AddText(ImGui::GetFont(),fsS,{wp.x+ww-fw-10.f,ty},kTxtHint,fb);
}

/* ════════════════════════════════════════════════════════════════
   FOV PREVIEW  —  glass-styled interactive circle
   ════════════════════════════════════════════════════════════════ */
static void FovPreview(float& fov)
{
    float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;
    float ch=cw*0.38f; if(ch<70.f)ch=70.f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImVec2 p=ImGui::GetCursorScreenPos();
    ImDrawList* dl=ImGui::GetWindowDrawList();

    ImGui::InvisibleButton("##fovprev",{cw,ch});
    bool act=ImGui::IsItemActive(),hov=ImGui::IsItemHovered();
    if(act){fov+=ImGui::GetIO().MouseDelta.x*2.f;fov=fov<10.f?10.f:fov>800.f?800.f:fov;}
    if(hov||act) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    /* Glass preview bg */
    dl->AddRectFilled(p,{p.x+cw,p.y+ch},IM_COL32(255,255,255,20),kCR);
    dl->AddRect(p,{p.x+cw,p.y+ch},(hov||act)?AC(50):IM_COL32(255,255,255,70),kCR,0,1.f);
    /* Subtle dot grid */
    for(float gx=18.f;gx<cw;gx+=18.f)
        for(float gy=18.f;gy<ch;gy+=18.f)
            dl->AddRectFilled({p.x+gx-.5f,p.y+gy-.5f},{p.x+gx+.5f,p.y+gy+.5f},
                IM_COL32(255,255,255,18));

    float cx2=p.x+cw*0.5f, cy2=p.y+ch*0.5f;
    float r=(fov/800.f)*(fminf(cw,ch)*0.45f-8.f);r=r<2.f?2.f:r;

    /* Glow rings */
    for(int i=3;i>=1;i--){
        float ri=r+(float)i*5.f;
        dl->AddCircle({cx2,cy2},ri,AC((int)(7/i)),80,(float)(7/i));
    }
    dl->AddCircle({cx2,cy2},r,AC(130),80,1.5f);
    dl->AddCircle({cx2,cy2},r,AC(200),80,1.f);
    /* Crosshair */
    dl->AddLine({cx2-5.f,cy2},{cx2+5.f,cy2},kTxtDis,1.f);
    dl->AddLine({cx2,cy2-5.f},{cx2,cy2+5.f},kTxtDis,1.f);
    dl->AddCircleFilled({cx2,cy2},2.5f,AC(255),10);

    if(act){
        char buf[16]; snprintf(buf,sizeof(buf),"%.0f px",fov);
        ImVec2 ts=ImGui::CalcTextSize(buf);float lx=cx2-ts.x*0.5f,ly=cy2+r+10.f;
        if(ly+ts.y<p.y+ch-4.f){
            dl->AddRectFilled({lx-5.f,ly-2.f},{lx+ts.x+5.f,ly+ts.y+2.f},
                IM_COL32(255,255,255,40),6.f);
            dl->AddRect({lx-5.f,ly-2.f},{lx+ts.x+5.f,ly+ts.y+2.f},
                IM_COL32(255,255,255,90),6.f,0,1.f);
            dl->AddText({lx,ly},AC(230),buf);
        }
    }
    dl->AddText(ImGui::GetFont(),ImGui::GetFontSize()*0.68f,{p.x+8.f,p.y+6.f},kTxtHint,"Preview");
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy({cw,ch});
    ImGui::Dummy({0,4.f});
}

/* ════════════════════════════════════════════════════════════════
   ESP PREVIEW  —  glass-framed player mockup
   ════════════════════════════════════════════════════════════════ */
static void EspPreview()
{
    ESPSettings& e=g_cfg.esp;
    float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;
    float ch=180.f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImVec2 p=ImGui::GetCursorScreenPos();
    ImDrawList* dl=ImGui::GetWindowDrawList();

    dl->AddRectFilled(p,{p.x+cw,p.y+ch},IM_COL32(255,255,255,20),kCR);
    dl->AddRect(p,{p.x+cw,p.y+ch},IM_COL32(255,255,255,70),kCR,0,1.f);
    /* Dot grid */
    for(float gx=14.f;gx<cw;gx+=14.f)
        for(float gy=14.f;gy<ch;gy+=14.f)
            dl->AddRectFilled({p.x+gx-.4f,p.y+gy-.4f},{p.x+gx+.4f,p.y+gy+.4f},
                IM_COL32(255,255,255,14));

    /* Player mock */
    static float s_hp=0.72f,s_hpT=0.f; s_hpT+=ImGui::GetIO().DeltaTime*0.3f;
    s_hp=0.5f+sinf(s_hpT)*0.22f;
    float bW=cw*0.22f,bH=ch*0.68f;
    float bL=p.x+cw*0.5f-bW*0.5f,bTop=p.y+ch*0.12f,bBot=bTop+bH,bR=bL+bW;
    float cx2=p.x+cw*0.5f;
    float hR=bH*0.10f,hCY=bTop-hR-2.f;

    if(e.showBox){
        ImU32 boxC=IM_COL32((int)(e.boxCol[0]*255),(int)(e.boxCol[1]*255),(int)(e.boxCol[2]*255),200);
        float th=e.boxThickness;
        if(e.boxType==1){
            float cs=0.22f;
            auto C=[&](float x0,float y0,float dx,float dy){
                dl->AddLine({x0,y0},{x0+dx*cs,y0},boxC,th);
                dl->AddLine({x0,y0},{x0,y0+dy*cs},boxC,th);};
            C(bL,bTop,1,1);C(bR,bTop,-1,1);C(bL,bBot,1,-1);C(bR,bBot,-1,-1);
        } else {
            dl->AddRectFilled({bL,bTop},{bR,bBot},
                IM_COL32((int)(e.boxCol[0]*255),(int)(e.boxCol[1]*255),(int)(e.boxCol[2]*255),18),kCR2);
            dl->AddRect({bL,bTop},{bR,bBot},boxC,kCR2,0,th);
        }
    }
    if(e.showSkeleton){
        ImU32 sc=IM_COL32((int)(e.skeletonCol[0]*255),(int)(e.skeletonCol[1]*255),(int)(e.skeletonCol[2]*255),180);
        float st=e.skeletonThick;
        float nY=bTop+bH*.04f,shY=bTop+bH*.28f,hipY=bTop+bH*.56f;
        float shX=bW*.72f,elbX=bW*.90f,elbY=bTop+bH*.42f;
        float hanX=bW*.88f,hanY=bTop+bH*.54f;
        float knY=bTop+bH*.76f,knX=bW*.36f;
        dl->AddLine({cx2,nY},{cx2,hipY},sc,st);
        dl->AddLine({cx2-shX,shY},{cx2+shX,shY},sc,st);
        dl->AddLine({cx2-shX,shY},{cx2-elbX,elbY},sc,st);
        dl->AddLine({cx2+shX,shY},{cx2+elbX,elbY},sc,st);
        dl->AddLine({cx2-elbX,elbY},{cx2-hanX,hanY},sc,st);
        dl->AddLine({cx2+elbX,elbY},{cx2+hanX,hanY},sc,st);
        dl->AddLine({cx2,hipY},{cx2-knX,knY},sc,st);
        dl->AddLine({cx2,hipY},{cx2+knX,knY},sc,st);
        dl->AddLine({cx2-knX,knY},{cx2-bW*.32f,bBot},sc,st);
        dl->AddLine({cx2+knX,knY},{cx2+bW*.32f,bBot},sc,st);
        auto jnt=[&](float jx,float jy){
            dl->AddCircleFilled({jx,jy},1.6f,IM_COL32(0,0,0,80),8);
            dl->AddCircleFilled({jx,jy},1.1f,sc,8);};
        jnt(cx2,nY);jnt(cx2-shX,shY);jnt(cx2+shX,shY);
        jnt(cx2-elbX,elbY);jnt(cx2+elbX,elbY);
        jnt(cx2-hanX,hanY);jnt(cx2+hanX,hanY);jnt(cx2,hipY);
        jnt(cx2-knX,knY);jnt(cx2+knX,knY);
    }
    dl->AddCircleFilled({cx2,hCY},hR,IM_COL32(200,190,220,120));
    dl->AddCircle({cx2,hCY},hR,IM_COL32(255,255,255,60),20,1.f);

    if(e.showHealth){
        constexpr float bw2=3.5f; float bx=bL-6.f;
        float fillH=(bBot-bTop)*s_hp;
        int hr=(int)((1.f-s_hp)*2.f*255.f); if(hr>255)hr=255;
        int hg=(int)(s_hp*2.f*255.f); if(hg>255)hg=255;
        dl->AddRectFilled({bx,bTop},{bx+bw2,bBot},IM_COL32(255,255,255,18),1.5f);
        dl->AddRectFilledMultiColor({bx,bBot-fillH},{bx+bw2,bBot},
            IM_COL32(hr,hg,20,220),IM_COL32(hr,hg,20,220),
            IM_COL32(hg/2,hg,20,230),IM_COL32(hg/2,hg,20,230));
        dl->AddRect({bx,bTop},{bx+bw2,bBot},IM_COL32(255,255,255,50),1.5f);
    }
    if(e.showNames){
        ImU32 nc=IM_COL32((int)(e.nameCol[0]*255),(int)(e.nameCol[1]*255),(int)(e.nameCol[2]*255),230);
        const char* fn="Player"; ImVec2 nt=ImGui::CalcTextSize(fn);
        float fsN=ImGui::GetFontSize()*.84f,nw=nt.x*(fsN/ImGui::GetFontSize());
        float ny=hCY-hR-fsN-4.f;
        dl->AddText(ImGui::GetFont(),fsN,{cx2-nw*.5f+.5f,ny+.5f},IM_COL32(0,0,0,80),fn);
        dl->AddText(ImGui::GetFont(),fsN,{cx2-nw*.5f,ny},nc,fn);
    }
    if(e.showDistance){
        const char* ds="42m"; float fsD=ImGui::GetFontSize()*.76f;
        ImVec2 dt=ImGui::CalcTextSize(ds); float dw=dt.x*(fsD/ImGui::GetFontSize());
        dl->AddText(ImGui::GetFont(),fsD,{cx2-dw*.5f,bBot+5.f},kTxtDis,ds);
    }
    dl->AddText(ImGui::GetFont(),ImGui::GetFontSize()*.68f,{p.x+8.f,p.y+6.f},kTxtHint,"Preview");
    ImGui::Dummy({cw,ch});
    ImGui::Dummy({0,4.f});
}

/* ════════════════════════════════════════════════════════════════
   CONTENT HELPERS
   ════════════════════════════════════════════════════════════════ */
static void TwoColBegin(float& lW,float& rW,float totalW)
{ lW=totalW*0.5f; rW=totalW-lW; }

/* ════════════════════════════════════════════════════════════════
   TAB: COMBAT
   ════════════════════════════════════════════════════════════════ */
static void TabCombat()
{
    Aimbot::Settings& a=Aimbot::settings;
    float tw=ImGui::GetContentRegionAvail().x;
    float lW=tw*.5f;

    ImGui::BeginChild("##cl",{lW,0},false,ImGuiWindowFlags_NoScrollbar);
    float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;
    float cx=ImGui::GetCursorScreenPos().x;

    CardBegin(cx,cw+kPad*2.f);
    SecHdr("Aimbot");
    RowCB("Enable##ab",a.enabled,&a.key,"aim_kb");
    RowCB("Sticky Target",a.sticky);
    RowCB("Visible Only",a.visibleOnly);
    RowCB("Rage Mode",a.rageMode);
    RowSlideF("FOV Radius",a.fov,10.f,800.f,"%.0f px");
    {const char* mi[]={"Mouse (SendInput)","Camera (Memory)"};
     int mx=(int)a.method;if(RowCombo("Method",mx,mi,2))a.method=(AimbotMethod)mx;}
    {const char* bi[]={"Head","Torso","Legs"};
     int bx=(int)a.part;if(RowCombo("Target Part",bx,bi,3))a.part=(TargetPart)bx;}
    {const char* pi[]={"Crosshair","Lowest HP","Nearest"};
     RowCombo("Priority",a.priorityMode,pi,3);}
    RowSlideF("Max Distance",a.maxAimDist,50.f,2000.f,"%.0f");
    RowSlideF("Smoothness",a.smooth,1.f,30.f,"%.1f");
    CardEnd();

    CardBegin(cx,cw+kPad*2.f);
    SecHdr("Prediction");
    RowCB("Enable##pred",a.prediction);
    if(a.prediction){
        RowSlideF("Scale",a.predictionScale,.1f,3.f,"%.2f");
        RowCB("Latency Comp",a.latencyComp);
        if(a.latencyComp)RowSlideF("Est. Ping",a.estimatedPingMs,0.f,300.f,"%.0f ms");
        RowCB("Gravity Comp",a.gravityComp);
        if(a.gravityComp)RowSlideF("Gravity Str",a.gravityStr,0.f,5.f,"%.2f");}
    CardEnd();

    CardBegin(cx,cw+kPad*2.f);
    SecHdr("Recoil Control");
    RowCB("Enable##rcs",a.recoilControl);
    if(a.recoilControl){
        RowSlideF("Vertical",a.recoilY,-5.f,5.f,"%.2f");
        RowSlideF("Horizontal",a.recoilX,-5.f,5.f,"%.2f");}
    RowCB("Jitter##jt",a.jitter);
    if(a.jitter){
        RowSlideF("Strength",a.jitterStr,.1f,5.f,"%.1f");
        RowCB("Both Axes",a.jitterBothAxes);}
    CardEnd();

    ImGui::EndChild();
    ImGui::SameLine();
    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBdrMid,1.f);
     ImGui::SetCursorScreenPos({dp.x+1.f,dp.y});}

    ImGui::BeginChild("##cr",{0,0},false,ImGuiWindowFlags_NoScrollbar);
    float cx2=ImGui::GetCursorScreenPos().x;
    float cw2=ImGui::GetContentRegionAvail().x-kPad*2.f;

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("FOV Preview");
    FovPreview(a.fov);
    RowCB("Draw FOV Circle",a.drawFov);
    CardEnd();

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("Silent Aim");
    RowCB("Enable##sa",a.silentAim,&a.silentAimKey,"sa_kb");
    if(a.silentAim){
        {const char* spm[]={"Crosshair","Lowest HP","Nearest"};
         RowCombo("Priority##sp",a.silentPriorityMode,spm,3);}
        {const char* spt[]={"Head","Torso","Random","Closest"};
         RowCombo("Target Part##spt",a.silentPartMode,spt,4);}
        RowCB("Visible Only##sv",a.silentVisCheck);
        RowCB("Auto Fire##saf",a.autoClick);
        if(a.autoClick){
            RowSlideF("Fire Delay",a.autoClickDelay,.01f,1.f,"%.2f s");
            RowCB("Hold Click",a.autoClickHold);}}
    CardEnd();

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("Triggerbot");
    RowCB("Enable##tb",a.triggerbot,&a.triggerbotKey,"tb_kb");
    if(a.triggerbot){
        RowCB("Only In FOV",a.triggerOnlyInFov);
        if(a.triggerOnlyInFov)RowSlideF("FOV##tf",a.triggerFov,1.f,300.f,"%.0f");
        RowCB("Head Only",a.triggerHeadOnly);
        RowCB("No Move##tnm",a.triggerNoMove);
        RowSlideF("Delay##td",a.triggerbotDelay,0.f,.5f,"%.2f s");
        RowSlideF("Hitchance",a.hitchance,1.f,100.f,"%.0f %%");}
    CardEnd();

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("Anti-Aim");
    RowCB("Spinbot##spb",a.spinbot);
    if(a.spinbot)RowSlideF("Speed##sps",a.spinSpeed,60.f,3600.f,"%.0f deg/s");
    RowCB("Anti-Aim##aa",a.antiAim);
    if(a.antiAim){
        {const char* aaM[]={"Flip","Jitter","Spin+Flip"};RowCombo("Mode##aam",a.antiAimMode,aaM,3);}
        RowCB("Flip Pitch",a.antiAimPitch);}
    CardEnd();

    ImGui::EndChild();
}

/* ════════════════════════════════════════════════════════════════
   TAB: VISUALS
   ════════════════════════════════════════════════════════════════ */
static void TabVisuals()
{
    ESPSettings& e=g_cfg.esp;
    float tw=ImGui::GetContentRegionAvail().x;
    float lW=tw*.5f;

    ImGui::BeginChild("##vl",{lW,0},false,ImGuiWindowFlags_NoScrollbar);
    float cx=ImGui::GetCursorScreenPos().x;
    float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;

    CardBegin(cx,cw+kPad*2.f);
    SecHdr("ESP");
    RowCB("Enable##esp",e.enabled);
    SecHdr("Bounding Box");
    RowCB("Show Box",e.showBox);
    if(e.showBox){
        {const char* bt[]={"Rounded","Corners","Filled"};RowCombo("Type",e.boxType,bt,3);}
        RowColor("Box Color",e.boxCol);
        RowSlideF("Thickness",e.boxThickness,.5f,4.f,"%.1f");
        RowCB("Outline",e.boxOutline);RowCB("Rainbow",e.rainbowBox);
        RowCB("Dynamic Color",e.dynBoxColor);}
    SecHdr("Names");
    RowCB("Show Name",e.showNames);
    if(e.showNames){
        RowCB("Display Name",e.showDisplayName);
        RowColor("Name Color",e.nameCol);
        RowCB("Background##nbg",e.nameBg);
        if(e.nameBg)RowSlideF("BG Alpha",e.nameBgAlpha,0.f,1.f,"%.2f");}
    CardEnd();

    CardBegin(cx,cw+kPad*2.f);
    SecHdr("Health");
    RowCB("Health Bar",e.showHealth);
    RowCB("HP Number",e.showHealthNum);
    {const char* hs[]={"Left","Right","Top","Bottom"};
     RowCombo("Bar Side",e.healthBarSide,hs,4);}
    RowCB("Low HP Flash",e.lowHpFlash);
    if(e.lowHpFlash)RowSlideF("Threshold",e.lowHpThresh,1.f,100.f,"%.0f");
    SecHdr("Skeleton");
    RowCB("Show Skeleton",e.showSkeleton);
    if(e.showSkeleton){
        RowColor("Skeleton Color",e.skeletonCol);
        RowSlideF("Thickness##skt",e.skeletonThick,.5f,4.f,"%.1f");
        RowCB("Rainbow##skr",e.skeletonRainbow);}
    CardEnd();

    CardBegin(cx,cw+kPad*2.f);
    SecHdr("Extra Overlays");
    RowCB("Head Dot",e.showHeadDot);
    if(e.showHeadDot){
        RowSlideF("Radius",e.headDotRadius,1.f,12.f,"%.1f");
        RowCB("Filled##hdf",e.headDotFill);}
    RowCB("Distance",e.showDistance);
    RowCB("Snap Lines",e.showSnapline);
    if(e.showSnapline){
        {const char* ss[]={"Bottom","Centre","Top"};RowCombo("Origin",e.snaplineStyle,ss,3);}
        RowColor("Line Color",e.snaplineCol);}
    RowCB("Offscreen Arrows",e.offscreenArrows);
    if(e.offscreenArrows){
        RowColor("Arrow Color",e.arrowCol);
        RowSlideF("Arrow Size",e.arrowSize,4.f,24.f,"%.0f");}
    RowCB("Show Dead",e.showDead);
    RowCB("Spectator Alert",e.spectatorAlert);
    CardEnd();

    ImGui::EndChild();
    ImGui::SameLine();
    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBdrMid,1.f);
     ImGui::SetCursorScreenPos({dp.x+1.f,dp.y});}
    ImGui::BeginChild("##vr",{0,0},false,ImGuiWindowFlags_NoScrollbar);
    float cx2=ImGui::GetCursorScreenPos().x;
    float cw2=ImGui::GetContentRegionAvail().x-kPad*2.f;

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("ESP Preview");
    EspPreview();
    CardEnd();

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("Chams");
    RowCB("Enable##chm",e.chams);
    if(e.chams){
        {const char* cm[]={"Flat","Mesh"};RowCombo("Mode##chm2",e.chamsMode,cm,2);}
        RowColor("Chams Color",e.chamsCol);
        RowSlideF("Alpha##cha",e.chamsAlpha,0.f,1.f,"%.2f");
        RowCB("Pulse##chp",e.chamsPulse);
        RowCB("Rim Light",e.chamsRimLight);
        RowCB("Wireframe",e.chamsWireframe);
        RowCB("Rainbow##chrbw",e.chamsRainbow);}
    CardEnd();

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("Crosshair");
    RowCB("Custom Crosshair",e.customCrosshair);
    if(e.customCrosshair){
        {const char* cs[]={"Cross","Circle","Dot","T-Shape"};
         RowCombo("Style##chs",e.crosshairStyle,cs,4);}
        RowSlideF("Size##chs2",e.crosshairSize,2.f,40.f,"%.0f");
        RowSlideF("Gap##chg",e.crosshairGap,0.f,20.f,"%.0f");
        RowSlideF("Thickness##cht",e.crosshairThick,.5f,4.f,"%.1f");
        RowColor("Color##chc",e.crosshairCol);
        RowCB("Dynamic##chd",e.crosshairDynamic);}
    SecHdr("Radar");
    RowCB("Enable##rdr",e.showRadar);
    if(e.showRadar){
        RowSlideF("Size##rds",e.radarSize,80.f,300.f,"%.0f");
        RowSlideF("Range##rdr2",e.radarRange,100.f,1000.f,"%.0f");
        RowSlideF("Dot Size",e.radarDotSize,2.f,12.f,"%.1f");}
    CardEnd();

    ImGui::EndChild();
}

/* ════════════════════════════════════════════════════════════════
   TAB: WORLD
   ════════════════════════════════════════════════════════════════ */
static void TabWorld()
{
    WorldSettings& w=g_cfg.world;
    float tw=ImGui::GetContentRegionAvail().x;
    float lW=tw*.5f;

    ImGui::BeginChild("##wl",{lW,0},false,ImGuiWindowFlags_NoScrollbar);
    float cx=ImGui::GetCursorScreenPos().x;
    float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;

    CardBegin(cx,cw+kPad*2.f);
    SecHdr("Fog Override");
    RowCB("Enable##fe",w.fogEnabled);
    if(w.fogEnabled){
        RowSlideF("End##fge",w.fogEnd,0.f,100000.f,"%.0f");
        RowSlideF("Start##fgs",w.fogStart,0.f,10000.f,"%.0f");
        RowColor("Fog Color",w.fogColor);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        if(ActBtn("Preset: Dense")){w.fogEnd=500.f;w.fogStart=0.f;w.fogColor[0]=.6f;w.fogColor[1]=.6f;w.fogColor[2]=.6f;}
        if(ActBtn("Preset: Clear")){w.fogEnd=100000.f;w.fogStart=0.f;}
        if(ActBtn("Preset: Night")){w.fogEnd=1500.f;w.fogStart=0.f;w.fogColor[0]=.04f;w.fogColor[1]=.04f;w.fogColor[2]=.10f;}}
    CardEnd();

    ImGui::EndChild();
    ImGui::SameLine();
    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBdrMid,1.f);
     ImGui::SetCursorScreenPos({dp.x+1.f,dp.y});}
    ImGui::BeginChild("##wr",{0,0},false,ImGuiWindowFlags_NoScrollbar);
    float cx2=ImGui::GetCursorScreenPos().x;
    float cw2=ImGui::GetContentRegionAvail().x-kPad*2.f;

    CardBegin(cx2,cw2+kPad*2.f);
    SecHdr("Lighting Override");
    RowCB("Enable##le",w.lightingEnabled);
    if(w.lightingEnabled){
        RowSlideF("Brightness",w.brightness,0.f,5.f,"%.2f");
        RowSlideF("Exposure",w.exposureComp,-5.f,5.f,"%.2f");
        RowSlideF("Clock Time",w.clockTime,0.f,24.f,"%.1f h");
        RowColor("Ambient",w.ambient);
        RowColor("Outdoor Ambient",w.outdoorAmbient);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        if(ActBtn("Preset: Dawn")){w.clockTime=6.f;w.brightness=.8f;w.exposureComp=-.5f;}
        if(ActBtn("Preset: Noon")){w.clockTime=14.f;w.brightness=1.f;w.exposureComp=0.f;}
        if(ActBtn("Preset: Midnight")){w.clockTime=0.f;w.brightness=.1f;w.exposureComp=-2.f;}}
    CardEnd();

    ImGui::EndChild();
}

/* ════════════════════════════════════════════════════════════════
   TAB: MOVEMENT
   ════════════════════════════════════════════════════════════════ */
static void TabMovement()
{
    MovementSettings& m = g_cfg.movement;
    float tw = ImGui::GetContentRegionAvail().x;
    float lW = tw * .5f;

    ImGui::BeginChild("##ml", { lW, 0 }, false, ImGuiWindowFlags_NoScrollbar);
    float cx = ImGui::GetCursorScreenPos().x;
    float cw = ImGui::GetContentRegionAvail().x - kPad * 2.f;

    CardBegin(cx, cw + kPad * 2.f);
    SecHdr("Speed");
    RowCB("Override Walk Speed", m.useSpeed);
    if (m.useSpeed) RowSlideF("Walk Speed", m.walkSpeed, 1.f, 200.f, "%.1f");
    RowCB("Always Sprint", m.alwaysSprint);
    SecHdr("Jump");
    RowCB("Override Jump Power", m.useJump);
    if (m.useJump) RowSlideF("Jump Power", m.jumpPower, 1.f, 500.f, "%.1f");
    RowCB("Infinite Jump", m.infiniteJump);
    RowCB("Bunny Hop", m.bunnyHop);
    CardEnd();

    CardBegin(cx, cw + kPad * 2.f);
    SecHdr("Physics");
    RowCB("No Slip", m.noSlip);
    RowCB("Low Gravity", m.lowGravity);
    if (m.lowGravity) RowSlideF("Gravity Scale", m.gravityScale, .01f, 1.f, "%.2f");
    RowCB("Fly Mode", m.fly);
    if (m.fly) RowSlideF("Fly Speed", m.flySpeed, 1.f, 500.f, "%.0f");
    RowCB("Noclip", m.noclip);
    SecHdr("Survival");
    RowCB("God Mode", m.godMode);
    RowCB("HP Regen", m.regenHP);
    if (m.regenHP) RowSlideF("Regen Rate", m.regenRate, .1f, 20.f, "%.1f hp/s");
    CardEnd();

    ImGui::EndChild();
    ImGui::SameLine();
    {
        ImVec2 dp = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(dp, { dp.x, dp.y + ImGui::GetContentRegionAvail().y }, kBdrMid, 1.f);
        ImGui::SetCursorScreenPos({ dp.x + 1.f, dp.y });
    }

    ImGui::BeginChild("##mr", { 0,0 }, false, ImGuiWindowFlags_NoScrollbar);
    float cx2 = ImGui::GetCursorScreenPos().x;
    float cw2 = ImGui::GetContentRegionAvail().x - kPad * 2.f;

    CardBegin(cx2, cw2 + kPad * 2.f);
    SecHdr("Utility");
    RowCB("Anti-AFK", m.antiAFK);
    RowCB("Anti-Void", m.antiVoid);
    if (m.antiVoid) RowSlideF("Floor Y", m.antiVoidY, -2000.f, 0.f, "%.0f");
    SecHdr("Actions");
    if (ActBtn("Apply Walk Speed", true)) Roblox::SetWalkSpeed(m.walkSpeed);
    if (ActBtn("Reset Walk Speed")) { m.walkSpeed = 16.f; Roblox::SetWalkSpeed(16.f); }
    if (ActBtn("Apply Jump Power", true)) Roblox::SetJumpPower(m.jumpPower);
    if (ActBtn("Reset Jump Power")) { m.jumpPower = 50.f; Roblox::SetJumpPower(50.f); }
    CardEnd();
    ImGui::EndChild();
} // <--- This closing brace was missing!

/* ════════════════════════════════════════════════════════════════
   TAB: OPTIONS
   ════════════════════════════════════════════════════════════════ */static void TabOptions()
{
    float tw = ImGui::GetContentRegionAvail().x;
    float lW = tw * .5f;

    /* --- LEFT COLUMN --- */
    ImGui::BeginChild("##ol", { lW, 0 }, false, ImGuiWindowFlags_NoScrollbar);
    float cx = ImGui::GetCursorScreenPos().x;
    float cw = ImGui::GetContentRegionAvail().x - kPad * 2.f;

    CardBegin(cx, cw + kPad * 2.f);
    SecHdr("Accent Color");
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float  rowH = 40.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        constexpr int kN = 8; float swSz = 18.f, swGap = 6.f;
        float swStartX = p.x + kPad;
        ImGui::Dummy({ 0, rowH });
        for (int i = 0; i < kN; i++) {
            float sx = swStartX + i * (swSz + swGap);
            float sy = p.y + (rowH - swSz) * 0.5f;
            bool sel = (!s_acCustom && s_acIdx == i);
            if (sel) {
                dl->AddRectFilled({ sx - 4.f, sy - 4.f }, { sx + swSz + 4.f, sy + swSz + 4.f }, AC(30), kCR2 + 4.f);
                dl->AddRect({ sx - 4.f, sy - 4.f }, { sx + swSz + 4.f, sy + swSz + 4.f }, AC(160), kCR2 + 4.f, 0, 1.5f);
            }
            auto& ac = kAccents[i];
            ImU32 sc = IM_COL32((int)(ac.r * 255), (int)(ac.g * 255), (int)(ac.b * 255), 255);
            dl->AddRectFilled({ sx, sy }, { sx + swSz, sy + swSz }, sc, kCR2);
            dl->AddRect({ sx, sy }, { sx + swSz, sy + swSz }, IM_COL32(255, 255, 255, 100), kCR2, 0, 1.f);
            dl->AddLine({ sx + 3.f, sy + 2.f }, { sx + swSz - 3.f, sy + 2.f }, IM_COL32(255, 255, 255, 80), 1.f);
            ImGui::SetCursorScreenPos({ sx, sy });
            char acId[16]; snprintf(acId, sizeof(acId), "##ac%d", i);
            ImGui::InvisibleButton(acId, { swSz, swSz });
            if (ImGui::IsItemClicked()) { s_acIdx = i; s_acCustom = false; SyncAccent(ac.r, ac.g, ac.b); ApplyAccentToStyle(); }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", kAccents[i].name);
        }
        ImGui::Dummy({ 0, 4.f });
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPad);
        if (ImGui::ColorEdit3("##cac", s_acCust, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder))
        {
            s_acCustom = true; SyncAccent(s_acCust[0], s_acCust[1], s_acCust[2]); ApplyAccentToStyle();
        }
        ImGui::SameLine(0, 6.f); ImGui::TextDisabled("Custom Accent");
        ImGui::Dummy({ 0, 4.f });
    }
    CardEnd();

    CardBegin(cx, cw + kPad * 2.f);
    SecHdr("Menu & Background");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPad);
    ImGui::ColorEdit4("Top Color##bg", s_gradTop, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::SameLine(0, 8.f); ImGui::Text("Glass Top");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPad);
    ImGui::ColorEdit4("Bottom Color##bg", s_gradBot, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::SameLine(0, 8.f); ImGui::Text("Glass Bottom");
    ImGui::Dummy({0, 5.f});
    /* Dark mode toggle — switches from light glass to dark osu! style */
    RowCB("Dark Mode", s_darkMode);
    /* Transparency slider — 0=fully transparent 1=fully opaque */
    RowSlideF("Glass Transparency", s_glassAlpha, 0.10f, 1.0f, "%.2f");
    /* Blur controls */
    RowCB("Background Blur", s_blurEnabled);
    if (s_blurEnabled) RowSlideF("Blur Strength", s_blurStrength, 0.f, 1.f, "%.2f");
    RowCB("Lock Position", g_cfg.menuLocked);
    RowSlideF("Menu Alpha", g_cfg.menuAlpha, .3f, 1.f, "%.2f");
    RowCB("Menu Glow", g_cfg.menuGlow);
    if (g_cfg.menuGlow) RowSlideF("Glow Intensity", g_cfg.menuGlowIntens, .1f, 1.f, "%.2f");
    
    SecHdr("ESP Effects");
    RowCB("Box Glow", g_cfg.espBoxGlow);
    if (g_cfg.espBoxGlow) RowSlideF("Glow Intensity##ebl", g_cfg.espBoxGlowIntens, .1f, 1.f, "%.2f");
    CardEnd();

    CardBegin(cx, cw + kPad * 2.f);
    SecHdr("Overlay Effects");
    RowCB("Dark Background", g_cfg.fx.darken);
    if (g_cfg.fx.darken) RowSlideF("Strength##fxd", g_cfg.fx.darkenAlpha, 0.f, 1.f, "%.2f");
    RowCB("Vignette", g_cfg.fx.vignette);
    if (g_cfg.fx.vignette) RowSlideF("Strength##fxv", g_cfg.fx.vignetteStr, 0.f, 1.f, "%.2f");
    RowCB("Scanlines", g_cfg.fx.scanlines);
    if (g_cfg.fx.scanlines) RowSlideF("Alpha##fxsl", g_cfg.fx.scanlineAlpha, 0.f, .5f, "%.2f");
    RowCB("Snow Effect", g_cfg.fx.snow);
    if (g_cfg.fx.snow) {
        RowSlideI("Count##snc", g_cfg.fx.snowCount, 20, 400);
        RowSlideF("Speed##sns", g_cfg.fx.snowSpeed, .1f, 4.f, "%.1f");
    }
    RowCB("RGB Border", g_cfg.fx.rgbBorder);
    CardEnd();
    ImGui::EndChild();

    /* --- RIGHT COLUMN --- */
    ImGui::SameLine();
    {
        ImVec2 dp = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(dp, { dp.x, dp.y + ImGui::GetContentRegionAvail().y }, kBdrMid, 1.f);
        ImGui::SetCursorScreenPos({ dp.x + 1.f, dp.y });
    }

    ImGui::BeginChild("##or", { 0, 0 }, false, ImGuiWindowFlags_NoScrollbar);
    float cx2 = ImGui::GetCursorScreenPos().x;
    float cw2 = ImGui::GetContentRegionAvail().x - kPad * 2.f;

    CardBegin(cx2, cw2 + kPad * 2.f);
    SecHdr("Session Info");
    {
        const auto& plrs = Roblox::GetPlayers();
        int alive = 0; for (auto& p : plrs) if (p.valid && p.health > 0 && !p.isLocalPlayer) alive++;
        auto row = [&](const char* l, const char* v) {
            ImVec2 p = ImGui::GetCursorScreenPos(); float lh = ImGui::GetTextLineHeight();
            ImGui::GetWindowDrawList()->AddText({ p.x + kPad, p.y + (kRH * .5f - lh * .5f) }, kTxt, l);
            ImVec2 vs = ImGui::CalcTextSize(v);
            ImGui::GetWindowDrawList()->AddText({ p.x + ImGui::GetContentRegionAvail().x - kPad - vs.x, p.y + (kRH * .5f - lh * .5f) }, kTxtSec, v);
            ImGui::GetWindowDrawList()->AddLine({ p.x + kPad, p.y + kRH - .5f }, { p.x + ImGui::GetContentRegionAvail().x, p.y + kRH - .5f }, kSep, 0.5f);
            ImGui::Dummy({ 0, kRH });
        };
        char lb[32]; snprintf(lb, sizeof(lb), "%d", alive); row("Players visible", lb);
        snprintf(lb, sizeof(lb), "%d", (int)plrs.size()); row("Total players", lb);
        row("Roblox version", "b8771a24f36a4ef8");
    }
    CardEnd();

    CardBegin(cx2, cw2 + kPad * 2.f);
    SecHdr("Target Info");
    {
        const PlayerData* tgt = Aimbot::GetTarget();
        if (tgt) {
            auto row = [&](const char* l, const char* v) {
                ImVec2 p = ImGui::GetCursorScreenPos(); float lh = ImGui::GetTextLineHeight();
                ImGui::GetWindowDrawList()->AddText({ p.x + kPad, p.y + (kRH * .5f - lh * .5f) }, kTxt, l);
                ImVec2 vs = ImGui::CalcTextSize(v);
                ImGui::GetWindowDrawList()->AddText({ p.x + ImGui::GetContentRegionAvail().x - kPad - vs.x, p.y + (kRH * .5f - lh * .5f) }, kTxtSec, v);
                ImGui::GetWindowDrawList()->AddLine({ p.x + kPad, p.y + kRH - .5f }, { p.x + ImGui::GetContentRegionAvail().x, p.y + kRH - .5f }, kSep, 0.5f);
                ImGui::Dummy({ 0, kRH });
            };
            char lb[48];
            row("Name", tgt->name.c_str());
            snprintf(lb, sizeof(lb), "%.0f / %.0f", tgt->health, tgt->maxHealth); row("Health", lb);
            float d = sqrtf(tgt->rootPos[0] * tgt->rootPos[0] + tgt->rootPos[1] * tgt->rootPos[1] + tgt->rootPos[2] * tgt->rootPos[2]);
            snprintf(lb, sizeof(lb), "%.1f studs", d); row("Distance", lb);
        }
        else { ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPad); ImGui::TextDisabled("No target locked"); }
    }
    CardEnd();

    CardBegin(cx2, cw2 + kPad * 2.f);
    SecHdr("Danger Zone");
    if (ActBtn("Reset All To Defaults##rst")) { g_cfg = Config{}; Aimbot::settings = Aimbot::Settings{}; }
    CardEnd();
    ImGui::EndChild();
}

static void TabConfig()
{
    float cx = ImGui::GetCursorScreenPos().x;
    float cw = ImGui::GetContentRegionAvail().x - kPad * 2.f;
    CardBegin(cx, cw + kPad * 2.f);
    SecHdr("Saved Configs");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPad);
    ConfigUI::Draw(ImGui::GetIO().DeltaTime);
    CardEnd();
}

static void TabLua()
{
    float cx = ImGui::GetCursorScreenPos().x;
    float cw = ImGui::GetContentRegionAvail().x - kPad * 2.f;

    CardBegin(cx, cw + kPad * 2.f);
    SecHdr("Script Editor");
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPad);
    ImGui::SetNextItemWidth(w - kPad * 2.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, kCR2);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, kV4S2);
    ImGui::InputTextMultiline("##lua", s_luaBuf, sizeof(s_luaBuf), { w - kPad * 2.f, 140.f }, ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopStyleColor(); ImGui::PopStyleVar();
    ImGui::Dummy({ 0, 5.f });
    if (ActBtn("Execute", true)) {}
    
    SecHdr("Console");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPad);
    ImGui::SetNextItemWidth(w - kPad * 2.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, kCR2);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, kV4S2);
    ImGui::InputTextMultiline("##lualog", s_log, sizeof(s_log), { w - kPad * 2.f, 70.f }, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor(); ImGui::PopStyleVar();
    ImGui::Dummy({ 0, 5.f });
    if (ActBtn("Clear Console")) memset(s_log, 0, sizeof(s_log));
    CardEnd();
}

void Render()
{
    if (!s_visible) return;

    constexpr float WW = 740.f, WH = 540.f;
    
    /* Always transparent ImGui bg so our drawn gradient is the background */
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_Border,   { 0, 0, 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, kWR);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });

    ImGui::SetNextWindowSize({ WW, WH }, ImGuiCond_Once);
    ImGui::SetNextWindowPos({ (float)g_cfg.menuX, (float)g_cfg.menuY }, ImGuiCond_Once);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("##LiquidGlassMenu", &s_visible, flags)) {
        ImVec2 p = ImGui::GetWindowPos();
        g_cfg.menuX = (int)p.x;
        g_cfg.menuY = (int)p.y;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        /* ── Frosted glass backdrop blur ───────────────────────────────────
         *
         * GOAL: match the reference screenshot exactly —
         *   • Background colours (red/pink, blue-gray) are VISIBLE but
         *     completely smeared into soft blobs, not sharp edges.
         *   • The frost is NOT a solid opaque layer — it is semi-transparent
         *     so the game background bleeds through at ~50-55 % opacity.
         *   • A top-edge specular gloss makes it read as glass, not plastic.
         *
         * WHY previous versions failed:
         *   • The heavy frost coat (alpha 185) completely hid the background,
         *     making the effect look like a solid tinted rectangle.
         *   • The spread offsets were too small (max ±6 px) so colors didn't
         *     visibly smear.
         *
         * TECHNIQUE — three conceptual stages:
         *
         *  A)  SPREAD SMEAR  — 44 passes at ±1…±10 px
         *      Each pass draws the menu-area rect offset by a different
         *      amount using a near-white neutral colour at low individual
         *      alpha.  Passes at large offsets expand the "bleed radius" of
         *      each background colour, making sharp edges bleed outward just
         *      like a Gaussian kernel averages adjacent pixels toward their
         *      neighbours.  Near-white colour (not a fixed tint) lets the
         *      actual game colours show through averaged toward white.
         *
         *  B)  FROST COAT  — single pass, alpha ≈ 110 at str = 1.0
         *      Alpha 110 blocks ~57 % of the background, letting ~43 %
         *      bleed through.  That ratio matches the reference image where
         *      the flag colours (pinkish-red centre, blue-gray left) are
         *      clearly visible but muted.  Higher alpha = too opaque (solid
         *      box look).  Lower alpha = background too sharp (no blur feel).
         *
         *  C)  TOP SPECULAR  — gradient from top, alpha ≈ 55
         *      Bright top edge sells the curved-glass surface reading.
         *
         * All passes are clipped to the window's rounded rect boundary.    */
        if (s_blurEnabled && s_blurStrength > 0.01f) {
            ImDrawList* bg = ImGui::GetBackgroundDrawList();
            float bx = p.x, by = p.y, bw = WW, bh = WH;
            float str = s_blurStrength;   /* 0 .. 1 */

            /* Clip every draw call to the rounded menu area */
            bg->PushClipRect({bx, by}, {bx+bw, by+bh}, true);

            /* ── A: Spread smear passes ─────────────────────────────── */
            /* 44 offsets covering ±1 px … ±10 px in cardinal + diagonal  *
             * directions.  Near-white (245,242,248) so background colours  *
             * average toward bright-neutral rather than toward a fixed hue. *
             * Per-pass alpha 9 × 44 passes = ~396 weighted smear alpha;    *
             * clamped to visible range by the frost coat above.             */
            static constexpr float kOff[][2] = {
                /* ± 1 px */
                {-1.f, 0.f},{1.f, 0.f},{0.f,-1.f},{0.f, 1.f},
                /* ± 2 px */
                {-2.f, 0.f},{2.f, 0.f},{0.f,-2.f},{0.f, 2.f},
                {-2.f,-2.f},{2.f,-2.f},{-2.f,2.f},{2.f, 2.f},
                /* ± 4 px */
                {-4.f, 0.f},{4.f, 0.f},{0.f,-4.f},{0.f, 4.f},
                {-3.f,-3.f},{3.f,-3.f},{-3.f,3.f},{3.f, 3.f},
                /* ± 6 px */
                {-6.f, 0.f},{6.f, 0.f},{0.f,-6.f},{0.f, 6.f},
                {-5.f,-4.f},{5.f,-4.f},{-5.f,4.f},{5.f, 4.f},
                /* ± 8 px */
                {-8.f, 0.f},{8.f, 0.f},{0.f,-8.f},{0.f, 8.f},
                {-6.f,-6.f},{6.f,-6.f},{-6.f,6.f},{6.f, 6.f},
                /* ±10 px */
                {-10.f, 0.f},{10.f, 0.f},{0.f,-10.f},{0.f,10.f},
                {-8.f,-7.f},{8.f,-7.f},{-8.f,7.f},{8.f, 7.f},
                /* diagonal extremes */
                {-10.f,-6.f},{10.f,-6.f},{-10.f,6.f},{10.f,6.f},
            };
            int smearA = (int)(str * 9.f);
            if (smearA > 0) {
                for (const auto& off : kOff) {
                    bg->AddRectFilled(
                        {bx + off[0], by + off[1]},
                        {bx + bw + off[0], by + bh + off[1]},
                        IM_COL32(245, 242, 248, smearA), kWR);
                }
            }

            /* ── B: Frost coat ──────────────────────────────────────── */
            /* Alpha 110 at str=1.0 → 43 % of background bleeds through.  *
             * Slight cool-lavender tint (230,224,240) matches the glass    *
             * panel colour in the reference image.                         */
            int frostA = (int)(str * 110.f);
            if (frostA > 0)
                bg->AddRectFilled(
                    {bx, by}, {bx+bw, by+bh},
                    IM_COL32(230, 224, 240, frostA), kWR);

            /* ── C: Top specular gloss ──────────────────────────────── */
            /* Bright-white gradient top→bottom over top 40 %.             *
             * Peak alpha 55 at str=1.0 — same ratio as the visible bright  *
             * upper band in the reference.                                  */
            int glossA = (int)(str * 55.f);
            if (glossA > 0)
                bg->AddRectFilledMultiColor(
                    {bx, by},          {bx+bw, by+bh*0.40f},
                    IM_COL32(255,255,255,glossA), IM_COL32(255,255,255,glossA),
                    IM_COL32(255,255,255,0),      IM_COL32(255,255,255,0));

            /* ── D: Thin bottom accent shadow ──────────────────────── */
            /* Subtle darker band at the bottom ~10 % helps "ground" the   *
             * panel so it doesn't float.  Matches the slightly darker      *
             * lower portion visible in the reference image.                */
            int shadowA = (int)(str * 30.f);
            if (shadowA > 0)
                bg->AddRectFilledMultiColor(
                    {bx, by+bh*0.85f}, {bx+bw, by+bh},
                    IM_COL32(80, 70, 100, 0),        IM_COL32(80, 70, 100, 0),
                    IM_COL32(80, 70, 100, shadowA),  IM_COL32(80, 70, 100, shadowA));

            bg->PopClipRect();
        }

        /* In dark mode: deep navy gradient; light mode: user gradient */
        ImU32 colTop, colBot;
        if (s_darkMode) {
            int aa = (int)(s_glassAlpha * 255.f);
            colTop = IM_COL32(22,20,38, aa);
            colBot = IM_COL32(18,16,30, aa);
        } else {
            ImVec4 topV = *(ImVec4*)s_gradTop; topV.w *= s_glassAlpha;
            ImVec4 botV = *(ImVec4*)s_gradBot; botV.w *= s_glassAlpha;
            colTop = ImGui::GetColorU32(topV);
            colBot = ImGui::GetColorU32(botV);
        }
        /* ── Translucent rounded background ─────────────────────────────
         * AddRectFilledMultiColor has no rounding param, so to get a
         * gradient that respects kWR=24 corners we:
         *  1. Flood the rounded shape with colTop via AddRectFilled
         *  2. Gradient-blend the safe inner strip (no corner pixels)
         *  3. Fill the corner-safe top/bottom strips with their solid colour
         * Result: smooth top→bot gradient, fully rounded, no square corners */

        /* 1. Rounded base */
        dl->AddRectFilled(p, { p.x + WW, p.y + WH }, colTop, kWR);

        /* 2. Gradient over the vertical centre (left/right full width, top/bot inset) */
        dl->AddRectFilledMultiColor(
            { p.x,        p.y + kWR  },
            { p.x + WW,   p.y + WH - kWR },
            colTop, colTop, colBot, colBot);

        /* 3. Top strip: inset by kWR horizontally so corners stay from step 1 */
        dl->AddRectFilledMultiColor(
            { p.x + kWR,      p.y       },
            { p.x + WW - kWR, p.y + kWR },
            colTop, colTop, colTop, colTop);

        /* 4. Bottom strip: same inset, filled with colBot */
        dl->AddRectFilledMultiColor(
            { p.x + kWR,      p.y + WH - kWR },
            { p.x + WW - kWR, p.y + WH       },
            colBot, colBot, colBot, colBot);

        /* 5. Glass border + top specular line */
        dl->AddRect(p, { p.x + WW, p.y + WH }, IM_COL32(255, 255, 255, 95), kWR, 0, 1.5f);
        dl->AddLine(
            { p.x + kWR,      p.y + 1.f },
            { p.x + WW - kWR, p.y + 1.f },
            IM_COL32(255, 255, 255, 170), 1.2f);

        /* 6. Soft drop shadow (layered) */
        for (int _si = 1; _si <= 4; _si++) {
            float _ss = (float)_si * 1.6f;
            dl->AddRect(
                { p.x - _ss, p.y - _ss },
                { p.x + WW + _ss, p.y + WH + _ss },
                IM_COL32(80, 50, 120, (int)(20 / _si)), kWR + _ss, 0, 1.f);
        }

        ImGui::SetCursorPos({ 0, 0 });
        ImGui::InvisibleButton("##drag", { WW, kHdrH });
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            p.x += ImGui::GetIO().MouseDelta.x; p.y += ImGui::GetIO().MouseDelta.y;
            ImGui::SetWindowPos(p);
        }

        DrawSidebar(dl, p, WH);
        float mainX = p.x + kSbW; float mainW = WW - kSbW;
        DrawTabBar(dl, mainX, mainW, p.y);

        ImGui::SetCursorPos({ kSbW, kHdrH });
        ImGui::BeginChild("##tabcontent", { mainW, WH - kHdrH - kBarH }, false, ImGuiWindowFlags_NoScrollbar);
        switch(s_tab) {
            case 0: TabCombat(); break;
            case 1: TabVisuals(); break;
            case 2: TabWorld(); break;
            case 3: TabMovement(); break;
            case 4: TabOptions(); break;
            case 5: TabConfig(); break;
            case 6: TabLua(); break;
        }
        ImGui::EndChild();

        DrawStatusBar(dl, { mainX, p.y }, mainW, WH);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

} // namespace ThemeOriginal