/* ═════════════════════════════════════════════════════════════════
 * x9  ·  MATCHA-EXACT THEME
 *
 * Pixel-matched to the Matcha executor reference screenshot.
 *
 * Layout   : header breadcrumb → tab capsules → two-col content → status bar
 * Widgets  : square checkbox left · label · keybind pill right
 * slider label+value line then slim track line
 * full-width action buttons with dark bg + border
 * Palette  : near-black, monochrome grays, purple accent (subtle)
 * ═════════════════════════════════════════════════════════════════ */
#include "x9_shared.h"
#include "config_ui.h"

namespace ThemeIndustrial {

/* ── Visibility / tab ─────────────────────────────────────────── */
static bool s_visible = true;
static int  s_tab     = 0;
/* 0=Aimbot 1=Visuals 2=World 3=Movement 4=Options 5=Config 6=Lua */

void Toggle()    { s_visible = !s_visible; }
bool IsVisible() { return s_visible; }

/* ═══════════════════════════════════════════════════════════════
 * PALETTE — premium dark with subtle cool tint
 * ═══════════════════════════════════════════════════════════════ */
static constexpr ImU32 kBg       = IM_COL32(  8,   8,  10, 255);  /* deepest bg    */
static constexpr ImU32 kSurface  = IM_COL32( 13,  13,  17, 255);  /* cards         */
static constexpr ImU32 kRaised   = IM_COL32( 18,  18,  24, 255);  /* elevated      */
static constexpr ImU32 kBorder   = IM_COL32( 32,  32,  48, 255);  /* visible edges */
static constexpr ImU32 kBorderDim= IM_COL32( 20,  20,  30, 255);  /* subtle        */
static constexpr ImU32 kSep      = IM_COL32( 14,  14,  20, 255);  /* row dividers  */
static constexpr ImU32 kGlass    = IM_COL32(255, 255, 255,   4);  /* glass sheen   */

/* Text */
static constexpr ImU32 kTxtHi    = IM_COL32(215, 215, 228, 255);
static constexpr ImU32 kTxtMid   = IM_COL32(155, 155, 168, 255);
static constexpr ImU32 kTxtDim   = IM_COL32( 92,  92, 108, 255);
static constexpr ImU32 kTxtFaint = IM_COL32( 55,  55,  70, 255);

static constexpr float kWinR  = 6.f;   /* softer corners */
static constexpr ImU32 kHov   = IM_COL32(255, 255, 255,  10);
static constexpr ImU32 kActiv = IM_COL32(255, 255, 255,  18);

/* ImVec4 versions */
static constexpr ImVec4 kVBg    = {  8/255.f,  8/255.f, 10/255.f, 1.f};
static constexpr ImVec4 kVSurf  = { 13/255.f, 13/255.f, 17/255.f, 1.f};
static constexpr ImVec4 kVRaise = { 18/255.f, 18/255.f, 24/255.f, 1.f};
static constexpr ImVec4 kVRaH   = { 24/255.f, 24/255.f, 32/255.f, 1.f};
static constexpr ImVec4 kVRaA   = { 30/255.f, 30/255.f, 40/255.f, 1.f};
static constexpr ImVec4 kVTxt   = {155/255.f,155/255.f,168/255.f, 1.f};
static constexpr ImVec4 kVDim   = { 92/255.f, 92/255.f,108/255.f, 1.f};

/* ═══════════════════════════════════════════════════════════════
 * ACCENT  (used sparingly — checkbox fill, slider fill, dot)
 * ═══════════════════════════════════════════════════════════════ */
struct AccentPreset { const char* name; ImVec4 col; };
static AccentPreset kAccents[] = {
    { "Violet",  {0.47f,0.34f,0.72f,1.f} },
    { "Blue",    {0.20f,0.42f,0.82f,1.f} },
    { "Cyan",    {0.08f,0.60f,0.74f,1.f} },
    { "Emerald", {0.12f,0.64f,0.42f,1.f} },
    { "Rose",    {0.80f,0.20f,0.36f,1.f} },
    { "Amber",   {0.82f,0.50f,0.10f,1.f} },
    { "Pink",    {0.74f,0.24f,0.60f,1.f} },
    { "Mono",    {0.40f,0.40f,0.48f,1.f} },
};
static int   s_acIdx   = 0;
static float s_acCust[3] = {0.47f,0.34f,0.72f};
static bool  s_acCustMode = false;

static ImVec4 AC4() {
    if(s_acCustMode) return {s_acCust[0],s_acCust[1],s_acCust[2],1.f};
    return kAccents[s_acIdx].col;
}
static ImU32 AC(int a=255) {
    ImVec4 v=AC4();
    return IM_COL32((int)(v.x*255),(int)(v.y*255),(int)(v.z*255),a);
}

static void SyncAccent() {
    ImVec4 a=AC4();
    ImVec4 h={fminf(a.x*1.3f,1.f),fminf(a.y*1.3f,1.f),fminf(a.z*1.3f,1.f),1.f};
    auto* c=ImGui::GetStyle().Colors;
    c[ImGuiCol_SliderGrab]           = {a.x*.6f,a.y*.6f,a.z*.6f,1.f};
    c[ImGuiCol_SliderGrabActive]     = a;
    c[ImGuiCol_CheckMark]            = a;
    c[ImGuiCol_ScrollbarGrab]        = {a.x*.4f,a.y*.4f,a.z*.4f,1.f};
    c[ImGuiCol_ScrollbarGrabHovered] = {a.x*.6f,a.y*.6f,a.z*.6f,1.f};
    c[ImGuiCol_ScrollbarGrabActive]  = a;
    c[ImGuiCol_Header]               = {a.x*.2f,a.y*.2f,a.z*.2f,1.f};
    c[ImGuiCol_HeaderHovered]        = {a.x*.3f,a.y*.3f,a.z*.3f,1.f};
    c[ImGuiCol_HeaderActive]         = {a.x*.4f,a.y*.4f,a.z*.4f,1.f};
    Loader::SetAccent(a.x,a.y,a.z);
}

/* ═══════════════════════════════════════════════════════════════
 * APPLY THEME
 * ═══════════════════════════════════════════════════════════════ */
void ApplyTheme()
{
    ImGuiStyle& st=ImGui::GetStyle();
    st.WindowRounding    =  3.f;   /* reference is nearly flat */
    st.ChildRounding     =  2.f;
    st.FrameRounding     =  3.f;
    st.GrabRounding      =  2.f;
    st.TabRounding       =  2.f;
    st.ScrollbarRounding =  2.f;
    st.PopupRounding     =  3.f;
    st.WindowBorderSize  =  0.f;
    st.ChildBorderSize   =  0.f;
    st.FrameBorderSize   =  0.f;
    st.WindowPadding     = {0.f,0.f};
    st.FramePadding      = {6.f,3.f};
    st.ItemSpacing       = {4.f,2.f};
    st.ItemInnerSpacing  = {4.f,2.f};
    st.IndentSpacing     = 10.f;
    st.GrabMinSize       =  4.f;
    st.ScrollbarSize     =  3.f;

    auto* c=st.Colors;
    c[ImGuiCol_WindowBg]          = kVBg;
    c[ImGuiCol_ChildBg]           = {0.f,0.f,0.f,0.f};
    c[ImGuiCol_PopupBg]           = kVSurf;
    c[ImGuiCol_Border]            = kVRaise;
    c[ImGuiCol_BorderShadow]      = {0,0,0,0};
    c[ImGuiCol_TitleBg]           = kVBg;
    c[ImGuiCol_TitleBgActive]     = kVBg;
    c[ImGuiCol_TitleBgCollapsed]  = kVBg;
    c[ImGuiCol_FrameBg]           = kVSurf;
    c[ImGuiCol_FrameBgHovered]    = kVRaH;
    c[ImGuiCol_FrameBgActive]     = kVRaA;
    c[ImGuiCol_Button]            = kVRaise;
    c[ImGuiCol_ButtonHovered]     = kVRaH;
    c[ImGuiCol_ButtonActive]      = kVRaA;
    c[ImGuiCol_ScrollbarBg]       = {0.f,0.f,0.f,0.f};
    c[ImGuiCol_Separator]         = {20/255.f,20/255.f,30/255.f,1.f};
    c[ImGuiCol_Text]              = kVTxt;
    c[ImGuiCol_TextDisabled]      = kVDim;
    SyncAccent();
    c[ImGuiCol_Tab]               = kVBg;
    c[ImGuiCol_TabHovered]        = kVSurf;
    c[ImGuiCol_TabActive]         = kVSurf;
    c[ImGuiCol_TabUnfocused]      = kVBg;
    c[ImGuiCol_TabUnfocusedActive]= kVSurf;
}

/* ═══════════════════════════════════════════════════════════════
 * LAYOUT CONSTANTS
 * ═══════════════════════════════════════════════════════════════ */
static constexpr float kPad   = 10.f;  /* tighter horizontal padding   */
static constexpr float kRH    = 19.f;  /* compact row height like ref   */
static constexpr float kCBSz  = 11.f;  /* smaller checkbox like ref     */
static constexpr float kHdrH  = 28.f;  /* slim header                   */
static constexpr float kTabH  = 26.f;  /* flat tab bar like ref         */
static constexpr float kBarH  = 20.f;  /* slim status bar               */

/* ═══════════════════════════════════════════════════════════════
 * SQUARE CHECKBOX
 * ═══════════════════════════════════════════════════════════════ */
static void DrawSquareCB(ImDrawList* dl, float x, float y, bool on)
{
    constexpr float r=2.5f;
    if(on){
        /* Outer glow */
        dl->AddRectFilled({x-2.f,y-2.f},{x+kCBSz+2.f,y+kCBSz+2.f},AC(20),r+2.f);
        dl->AddRectFilled({x,y},{x+kCBSz,y+kCBSz},AC(245),r);
        /* Glass highlight on top half */
        dl->AddRectFilledMultiColor(
            {x+1.f,y+1.f},{x+kCBSz-1.f,y+kCBSz*.5f},
            IM_COL32(255,255,255,55),IM_COL32(255,255,255,55),
            IM_COL32(255,255,255,0),IM_COL32(255,255,255,0));
        /* Checkmark */
        float cx=x+kCBSz*.5f, cy=y+kCBSz*.5f;
        dl->AddLine({x+2.5f,cy+.5f},{cx-1.f,y+kCBSz-2.5f},IM_COL32(255,255,255,240),1.6f);
        dl->AddLine({cx-1.f,y+kCBSz-2.5f},{x+kCBSz-2.f,y+2.f},IM_COL32(255,255,255,240),1.6f);
    } else {
        dl->AddRectFilled({x,y},{x+kCBSz,y+kCBSz},IM_COL32(14,14,20,255),r);
        dl->AddRect({x,y},{x+kCBSz,y+kCBSz},kBorderDim,r,0,1.f);
        /* Subtle inner shadow */
        dl->AddRectFilled({x+1.f,y+1.f},{x+kCBSz-1.f,y+3.f},IM_COL32(0,0,0,40),r);
    }
}

/* ═══════════════════════════════════════════════════════════════
 * KEYBIND WIDGET  (small inline pill)
 * ═══════════════════════════════════════════════════════════════ */
static void KeyBind(const char* id, int& key)
{
    static const char* s_lid=nullptr;
    bool ls=(s_lid&&strcmp(s_lid,id)==0);
    if(ls){
        if(GetAsyncKeyState(VK_ESCAPE)&0x8000){s_lid=nullptr;ls=false;}
        else{
            const int kS[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2,
                VK_LSHIFT,VK_RSHIFT,VK_LCONTROL,VK_RCONTROL,VK_MENU,
                VK_CAPITAL,VK_TAB,VK_SPACE,VK_DELETE,VK_INSERT,VK_HOME,VK_END,
                'A','B','C','D','E','F','G','H','I','J','K','L','M',
                'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
                '0','1','2','3','4','5','6','7','8','9',
                VK_F1,VK_F2,VK_F3,VK_F4,VK_F5,VK_F6,
                VK_F7,VK_F8,VK_F9,VK_F10,VK_F11,VK_F12,
                VK_NUMPAD0,VK_NUMPAD1,VK_NUMPAD2,VK_NUMPAD3,VK_NUMPAD4,
                VK_NUMPAD5,VK_NUMPAD6,VK_NUMPAD7,VK_NUMPAD8,VK_NUMPAD9};
            for(int k:kS)
                if(GetAsyncKeyState(k)&0x8000){key=k;s_lid=nullptr;ls=false;break;}
        }
    }
    char lbl[32];
    if(ls){snprintf(lbl,sizeof(lbl),"...##%s",id);}
    else{
        const char* kn=
            key==VK_RBUTTON?"rmb":key==VK_LBUTTON?"lmb":key==VK_MBUTTON?"mmb":
            key==VK_LSHIFT?"lsh":key==VK_RSHIFT?"rsh":
            key==VK_LCONTROL?"lctrl":key==VK_RCONTROL?"rctrl":
            key==VK_MENU?"alt":key==VK_SPACE?"spc":key==VK_TAB?"tab":
            key==VK_DELETE?"del":key==VK_INSERT?"ins":key==0?"-":nullptr;
        if(kn)snprintf(lbl,sizeof(lbl),"%s##%s",kn,id);
        else if(key>='A'&&key<='Z')snprintf(lbl,sizeof(lbl),"%c##%s",(char)(key+32),id);
        else if(key>=VK_F1&&key<=VK_F12)snprintf(lbl,sizeof(lbl),"f%d##%s",key-VK_F1+1,id);
        else if(key>=VK_NUMPAD0&&key<=VK_NUMPAD9)snprintf(lbl,sizeof(lbl),"num%d##%s",key-VK_NUMPAD0,id);
        else snprintf(lbl,sizeof(lbl),"k%d##%s",key,id);
    }
    
    ImVec4 bg=ls
        ?ImVec4(AC4().x*.25f,AC4().y*.25f,AC4().z*.4f,1.f)
        :kVRaise;
    ImGui::PushStyleColor(ImGuiCol_Button,        bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kVRaH);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  kVRaA);
    ImGui::PushStyleColor(ImGuiCol_Text,
        ls?ImVec4(1,1,1,1):ImVec4(200/255.f,200/255.f,210/255.f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,12.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{5.f,2.f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(38/255.f,38/255.f,54/255.f,1.f));

    if(ImGui::SmallButton(lbl))s_lid=ls?nullptr:id;
    if(ImGui::IsItemClicked(ImGuiMouseButton_Middle))key=0;
    if(ImGui::IsItemHovered())ImGui::SetTooltip("Click to bind · MMB to clear");

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

/* ═══════════════════════════════════════════════════════════════
 * ROW: CHECKBOX  (with optional right keybind)
 * [pad] [13px box] [6px] [label ............... keybind?]
 * ═══════════════════════════════════════════════════════════════ */
static void RowCB(const char* label, bool& val,
    int* key=nullptr, const char* kb_id=nullptr)
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();

    constexpr float kbW=44.f, kbGap=6.f;
    float hitW = (key && kb_id) ? w-kbW-kbGap : w;

    char bid[80]; snprintf(bid,sizeof(bid),"##cb_%s",label);
    ImGui::InvisibleButton(bid,{hitW,kRH});
    bool hov=ImGui::IsItemHovered();
    if(ImGui::IsItemClicked()) val=!val;
    /* Hover / active background */
    if(hov && !val) dl->AddRectFilled(p,{p.x+hitW,p.y+kRH},kHov,3.f);
    if(val){
        /* Subtle accent tint on enabled rows */
        ImVec4 av=AC4();
        dl->AddRectFilled(p,{p.x+hitW,p.y+kRH},
            IM_COL32((int)(av.x*255*.12f),(int)(av.y*255*.12f),(int)(av.z*255*.18f),255),3.f);
        /* Left accent stripe */
        dl->AddRectFilled(p,{p.x+2.f,p.y+kRH},AC(180),0.f);
    }

    /* Box */
    float bY=p.y+(kRH-kCBSz)*.5f;
    DrawSquareCB(dl, p.x+kPad, bY, val);

    /* Label */
    float lh=ImGui::GetTextLineHeight();
    float tY=p.y+(kRH-lh)*.5f;
    dl->AddText({p.x+kPad+kCBSz+6.f,tY},
        val ? kTxtHi : hov ? kTxtMid : IM_COL32(120,120,134,255), label);

    /* Keybind pill */
    if(key && kb_id){
        ImGui::SetCursorScreenPos({p.x+w-kbW-kbGap+1.f, p.y+(kRH-16.f)*.5f});
        KeyBind(kb_id,*key);
    }

    /* Separator */
    ImGui::SetCursorScreenPos({p.x,p.y+kRH});
    dl->AddLine({p.x+kPad*.5f,p.y+kRH-1.f},{p.x+w,p.y+kRH-1.f},kSep,1.f);
    ImGui::Dummy({0,1.f});
}

/* ═══════════════════════════════════════════════════════════════
 * ROW: SLIDER  — label + value on line1, track on line2
 * ═══════════════════════════════════════════════════════════════ */
static void RowSlideF(const char* label, float& v,
    float lo, float hi, const char* fmt="%.1f")
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    /* Label row */
    dl->AddText({p.x+kPad, p.y+(kRH-lh)*.5f}, kTxtMid, label);
    char vb[24]; snprintf(vb,sizeof(vb),fmt,v);
    ImVec2 vs=ImGui::CalcTextSize(vb);
    /* Value in accent colour */
    dl->AddText({p.x+w-kPad-vs.x, p.y+(kRH-lh)*.5f}, AC(190), vb);
    ImGui::Dummy({0,kRH-2.f});

    /* Custom drawn track + fill under the invisible slider */
    {
        float trX=p.x+kPad, trW=w-kPad*2.f;
        float trY2=ImGui::GetCursorScreenPos().y+5.f;
        float trH=3.f;
        float frac=(hi>lo)?(v-lo)/(hi-lo):0.f;
        frac=frac<0.f?0.f:frac>1.f?1.f:frac;
        /* Track bg */
        dl->AddRectFilled({trX,trY2},{trX+trW,trY2+trH},
            IM_COL32(20,20,30,255),1.5f);
        /* Filled portion */
        if(frac>0.f)
            dl->AddRectFilled({trX,trY2},{trX+trW*frac,trY2+trH},
                AC(160),1.5f);
        /* Knob */
        float kx=trX+trW*frac;
        float ky=trY2+trH*.5f;
        dl->AddCircleFilled({kx,ky},4.5f,IM_COL32(8,8,12,255),12);
        dl->AddCircleFilled({kx,ky},3.5f,AC(255),12);
        dl->AddCircleFilled({kx,ky},4.5f,AC(30),12);  /* glow */
    }

    /* Invisible slider over the visual */
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0.f,5.f});
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,1.f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,    ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,ImVec4(0,0,0,0));
    ImGui::SetNextItemWidth(w-kPad*2.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    char sid[64]; snprintf(sid,sizeof(sid),"##sf_%s",label);
    ImGui::SliderFloat(sid,&v,lo,hi,"");
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);

    ImVec2 p2=ImGui::GetCursorScreenPos();
    dl->AddLine({p.x+kPad*.5f,p2.y},{p.x+w,p2.y},kSep,1.f);
    ImGui::Dummy({0,3.f});
}

static void RowSlideI(const char* label, int& v, int lo, int hi)
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    dl->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,label);
    char vb[12]; snprintf(vb,sizeof(vb),"%d",v);
    ImVec2 vs=ImGui::CalcTextSize(vb);
    dl->AddText({p.x+w-kPad-vs.x,p.y+(kRH-lh)*.5f},kTxtDim,vb);
    ImGui::Dummy({0,kRH-2.f});

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0.f,2.f});
    ImGui::SetNextItemWidth(w-kPad*2.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    char sid[64]; snprintf(sid,sizeof(sid),"##si_%s",label);
    ImGui::SliderInt(sid,&v,lo,hi,"");
    ImGui::PopStyleVar();

    ImVec2 p2=ImGui::GetCursorScreenPos();
    dl->AddLine({p.x,p2.y},{p.x+w,p2.y},kSep,1.f);
    ImGui::Dummy({0,4.f});
}

/* ═══════════════════════════════════════════════════════════════
 * ROW: COMBO
 * ═══════════════════════════════════════════════════════════════ */
static bool RowCombo(const char* label, int& v,
    const char** items, int n)
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    if(label && label[0]!='\0'){
        dl->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,label);
        ImGui::Dummy({0,kRH-2.f});
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{6.f,4.f});
    ImGui::SetNextItemWidth(w-kPad*2.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    char cid[64]; snprintf(cid,sizeof(cid),"##cbo_%s",label);
    bool ch=ImGui::Combo(cid,&v,items,n);
    ImGui::PopStyleVar();

    ImVec2 p2=ImGui::GetCursorScreenPos();
    dl->AddLine({p.x,p2.y},{p.x+w,p2.y},kSep,1.f);
    ImGui::Dummy({0,4.f});
    return ch;
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION HEADER  
 * ═══════════════════════════════════════════════════════════════ */
static void SecHdr(const char* label)
{
    ImGui::Dummy({0,6.f});
    ImVec2 p=ImGui::GetCursorScreenPos();
    float  w2=ImGui::GetContentRegionAvail().x;
    float  lh=ImGui::GetTextLineHeight();
    ImDrawList* sdl=ImGui::GetWindowDrawList();

    /* Subtle bg */
    sdl->AddRectFilled({p.x,p.y},{p.x+w2,p.y+lh+6.f},
        IM_COL32(255,255,255,4));

    /* 2px accent left bar */
    sdl->AddRectFilled({p.x,p.y},{p.x+2.f,p.y+lh+6.f},AC(160));

    /* Label */
    sdl->AddText({p.x+kPad, p.y+3.f}, kTxtDim, label);

    /* Dim hairline to the right of label */
    float lw2=ImGui::CalcTextSize(label).x;
    float ry2=p.y+lh*.55f+3.f;
    sdl->AddLine({p.x+kPad+lw2+6.f,ry2},{p.x+w2-kPad,ry2},kSep,1.f);

    ImGui::Dummy({0,lh+8.f});
}

/* ═══════════════════════════════════════════════════════════════
 * ACTION BUTTON  — full width, dark raised, centred text
 * ═══════════════════════════════════════════════════════════════ */
static bool ActBtn(const char* label)
{
    float w=ImGui::GetContentRegionAvail().x-kPad*2.f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImGui::PushStyleColor(ImGuiCol_Button,        kVRaise);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kVRaH);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  kVRaA);
    ImGui::PushStyleColor(ImGuiCol_Text,          kVTxt);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
    ImGui::PushStyleColor(ImGuiCol_Border,
        ImVec4(38/255.f,38/255.f,54/255.f,1.f));
    bool clicked=ImGui::Button(label,{w,22.f});
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
    ImGui::Dummy({0,3.f});
    return clicked;
}

/* ═══════════════════════════════════════════════════════════════
 * COLOR SWATCH ROW
 * ═══════════════════════════════════════════════════════════════ */
static void RowColor(const char* label, float col[3])
{
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float  w=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();
    float  lh=ImGui::GetTextLineHeight();

    char bid[72]; snprintf(bid,sizeof(bid),"##colrow_%s",label);
    ImGui::InvisibleButton(bid,{w,kRH});
    bool hov=ImGui::IsItemHovered();
    if(hov) dl->AddRectFilled(p,{p.x+w,p.y+kRH},kHov,3.f);

    dl->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,label);

    /* Larger gradient swatch — shows the colour prominently */
    float sw2=22.f, sh2=kRH-6.f;
    float sx2=p.x+w-kPad-sw2, sy2=p.y+3.f;
    ImU32 colu=IM_COL32((int)(col[0]*255),(int)(col[1]*255),(int)(col[2]*255),255);
    dl->AddRectFilled({sx2,sy2},{sx2+sw2,sy2+sh2},colu,3.f);
    /* Gloss on swatch */
    dl->AddRectFilledMultiColor(
        {sx2+1.f,sy2+1.f},{sx2+sw2-1.f,sy2+sh2*.5f},
        IM_COL32(255,255,255,50),IM_COL32(255,255,255,50),
        IM_COL32(0,0,0,0),IM_COL32(0,0,0,0));
    dl->AddRect({sx2,sy2},{sx2+sw2,sy2+sh2},
        hov?AC(100):kBorderDim,3.f,0,1.f);

    if(ImGui::IsItemClicked()) ImGui::OpenPopup(label);
    if(ImGui::BeginPopup(label)){
        ImGui::ColorPicker3(label,col,
            ImGuiColorEditFlags_NoSmallPreview|ImGuiColorEditFlags_DisplayRGB);
        ImGui::EndPopup();
    }

    dl->AddLine({p.x+kPad*.5f,p.y+kRH-1.f},{p.x+w,p.y+kRH-1.f},kSep,1.f);
    ImGui::SetCursorScreenPos({p.x,p.y+kRH});
    ImGui::Dummy({0,1.f});
}

/* ═══════════════════════════════════════════════════════════════
 * TAB BAR  
 * ═══════════════════════════════════════════════════════════════ */
static void DrawTabBar(const char** labels, int count, int& active)
{
    float totalW=ImGui::GetContentRegionAvail().x;
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 bs=ImGui::GetCursorScreenPos();

    /* Tab bar background with top gradient sheen */
    dl->AddRectFilled(bs,{bs.x+totalW,bs.y+kTabH},IM_COL32(10,10,13,255));
    dl->AddRectFilledMultiColor(
        bs,{bs.x+totalW,bs.y+3.f},
        IM_COL32(255,255,255,6),IM_COL32(255,255,255,6),
        IM_COL32(0,0,0,0),IM_COL32(0,0,0,0));
    /* Bottom separator */
    dl->AddLine({bs.x,bs.y+kTabH},{bs.x+totalW,bs.y+kTabH},kBorderDim,1.f);

    float padX=12.f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0.f,0.f});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0.f,0.f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0.f);

    float curX=bs.x+5.f;
    for(int i=0;i<count;i++){
        bool act=(active==i);
        ImVec2 ts=ImGui::CalcTextSize(labels[i]);
        float  bW=ts.x+padX*2.f;

        ImGui::SetCursorScreenPos({curX,bs.y});
        char bid[24]; snprintf(bid,sizeof(bid),"##tab%d",i);
        ImGui::InvisibleButton(bid,{bW,kTabH});
        bool hov=ImGui::IsItemHovered();
        if(ImGui::IsItemClicked()) active=i;

        /* Active background pill */
        if(act){
            float pY=bs.y+3.f;
            dl->AddRectFilled({curX+2.f,pY},{curX+bW-2.f,bs.y+kTabH-1.f},
                IM_COL32(22,22,30,255),3.f);
            /* Top accent glow line */
            dl->AddRectFilled({curX+4.f,pY},{curX+bW-4.f,pY+1.5f},AC(180),1.f);
            /* Subtle inner glow */
            dl->AddRectFilled({curX+2.f,pY},{curX+bW-2.f,pY+6.f},
                IM_COL32((int)(AC4().x*255*.15f),(int)(AC4().y*255*.15f),
                         (int)(AC4().z*255*.22f),255),3.f);
        } else if(hov){
            dl->AddRectFilled({curX,bs.y},{curX+bW,bs.y+kTabH},kHov);
        }

        /* Label */
        float ty=bs.y+(kTabH-ts.y)*.5f;
        ImU32 tc = act ? kTxtHi : hov ? kTxtMid : kTxtDim;
        dl->AddText({curX+padX,ty}, tc, labels[i]);

        /* Active accent dot below label */
        if(act)
            dl->AddCircleFilled({curX+bW*.5f,bs.y+kTabH-3.f},
                2.f, AC(200), 8);

        curX+=bW;
    }

    ImGui::PopStyleVar(3);
    ImGui::SetCursorScreenPos({bs.x,bs.y+kTabH});
    ImGui::Dummy({0,1.f});
}

/* ═══════════════════════════════════════════════════════════════
 * FOV PREVIEW
 * ═══════════════════════════════════════════════════════════════ */
static void FovPreview(float& fov)
{
    float sz=ImGui::GetContentRegionAvail().x-kPad*2.f;
    float pH=sz*.5f; if(pH<60.f)pH=60.f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImVec2 p=ImGui::GetCursorScreenPos();
    ImDrawList* dl=ImGui::GetWindowDrawList();

    ImGui::InvisibleButton("##fovdrag",{sz,pH});
    bool hov=ImGui::IsItemHovered(),act=ImGui::IsItemActive();
    if(act){fov+=ImGui::GetIO().MouseDelta.x*2.5f;
            fov=fov<10.f?10.f:fov>800.f?800.f:fov;}
    if(hov||act) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    dl->AddRectFilled(p,{p.x+sz,p.y+pH},kSurface,5.f);
    dl->AddRect(p,{p.x+sz,p.y+pH},(hov||act)?AC(50):kBorderDim,5.f,0,1.f);
    for(float x=16.f;x<sz;x+=16.f)
        dl->AddLine({p.x+x,p.y+1.f},{p.x+x,p.y+pH-1.f},IM_COL32(20,20,30,255),.5f);
    for(float y=16.f;y<pH;y+=16.f)
        dl->AddLine({p.x+1.f,p.y+y},{p.x+sz-1.f,p.y+y},IM_COL32(20,20,30,255),.5f);

    float cx=p.x+sz*.5f,cy=p.y+pH*.5f;
    float r=(fov/800.f)*(fminf(sz,pH)*.5f-6.f);r=r<2.f?2.f:r;
    dl->AddCircle({cx,cy},r,AC(15),72,18.f);
    dl->AddCircle({cx,cy},r,AC(40),72, 8.f);
    dl->AddCircle({cx,cy},r,AC(90),72, 2.f);
    dl->AddCircle({cx,cy},r,AC(200),72,1.f);
    dl->AddLine({cx-4.f,cy},{cx+4.f,cy},kTxtFaint);
    dl->AddLine({cx,cy-4.f},{cx,cy+4.f},kTxtFaint);
    dl->AddCircleFilled({cx,cy},2.f,AC(255),8);

    if(act){
        char buf[16];snprintf(buf,sizeof(buf),"%.0f px",fov);
        ImVec2 ts=ImGui::CalcTextSize(buf);float lx=cx-ts.x*.5f,ly=cy+r+7.f;
        if(ly+ts.y<p.y+pH-4.f){
            dl->AddRectFilled({lx-4.f,ly-2.f},{lx+ts.x+4.f,ly+ts.y+2.f},kSurface,3.f);
            dl->AddText({lx,ly},AC(255),buf);}}

    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy({sz,pH});
    ImGui::Dummy({0,4.f});
}

/* ═══════════════════════════════════════════════════════════════
 * ESP PREVIEW
 * ═══════════════════════════════════════════════════════════════ */
static void EspPreview(float W, float H)
{
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImVec2 p=ImGui::GetCursorScreenPos();
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ESPSettings& e=g_cfg.esp;

    /* ── Background ─────────────────────────────────────────── */
    dl->AddRectFilled(p,{p.x+W,p.y+H},kSurface,5.f);
    dl->AddRect(p,{p.x+W,p.y+H},kBorderDim,5.f,0,1.f);
    for(float gx=12.f;gx<W;gx+=12.f)
        for(float gy=12.f;gy<H;gy+=12.f)
            dl->AddRectFilled({p.x+gx-.5f,p.y+gy-.5f},{p.x+gx+.5f,p.y+gy+.5f},
                IM_COL32(30,30,44,255));

    /* ── Animated HP ────────────────────────────────────────── */
    static float s_hp=0.72f, s_hd=-0.0002f;
    s_hp+=s_hd;
    if(s_hp<=0.18f) s_hd= 0.0002f;
    if(s_hp>=0.96f) s_hd=-0.0002f;

    /* ── Geometry ───────────────────────────────────────────── */
    float cx    = p.x + W*.5f;
    float groundY = p.y + H - 12.f;
    float bH    = H * 0.56f;
    float bW    = fmaxf(bH * 0.26f, 15.f);
    float bBot  = groundY;
    float bTop  = bBot - bH;
    float bL    = cx - bW;
    float bR    = cx + bW;
    float hR    = 9.5f;
    float hCY   = bTop - hR - 2.f;

    ImU32 boxCol = IM_COL32(
        (int)(e.boxCol[0]*255),(int)(e.boxCol[1]*255),(int)(e.boxCol[2]*255), 210);

    /* ── Ground line ────────────────────────────────────────── */
    dl->AddLine({p.x+6.f,groundY},{p.x+W-6.f,groundY},IM_COL32(28,28,44,255),1.f);

    /* ── ESP Box ────────────────────────────────────────────── */
    if(e.showBox){
        float th=e.boxThickness;
        if(e.boxType==0){
            if(e.boxOutline) dl->AddRect({bL-1.f,bTop-1.f},{bR+1.f,bBot+1.f},
                IM_COL32(0,0,0,140),0.f,0,th+1.f);
            dl->AddRect({bL,bTop},{bR,bBot},boxCol,0.f,0,th);
        } else if(e.boxType==1){
            float cs=bW*0.32f;
            auto corner=[&](float x0,float y0,float dx,float dy){
                dl->AddLine({x0,y0},{x0+dx*cs,y0},boxCol,th);
                dl->AddLine({x0,y0},{x0,y0+dy*cs},boxCol,th);};
            corner(bL,bTop, 1, 1); corner(bR,bTop,-1, 1);
            corner(bL,bBot, 1,-1); corner(bR,bBot,-1,-1);
        } else {
            dl->AddRectFilled({bL,bTop},{bR,bBot},
                IM_COL32((int)(e.boxCol[0]*255),(int)(e.boxCol[1]*255),(int)(e.boxCol[2]*255),22));
            dl->AddRect({bL,bTop},{bR,bBot},boxCol,0.f,0,th);
        }
    }

    /* ── Skeleton ───────────────────────────────────────────── */
    if(e.showSkeleton){
        ImU32 sc=IM_COL32(
            (int)(e.skeletonCol[0]*255),(int)(e.skeletonCol[1]*255),
            (int)(e.skeletonCol[2]*255),190);
        float st=e.skeletonThick;
        float neckY  = bTop + bH*0.04f;
        float shY    = bTop + bH*0.28f;
        float hipY   = bTop + bH*0.56f;
        float shX    = bW  * 0.72f;
        float elbX   = bW  * 0.90f, elbY = bTop + bH*0.42f;
        float handX  = bW  * 0.88f, handY= bTop + bH*0.54f;
        float kneeX  = bW  * 0.36f, kneeY= bTop + bH*0.76f;
        float footX  = bW  * 0.32f;

        dl->AddLine({cx,neckY},{cx,hipY},sc,st);                              
        dl->AddLine({cx-shX,shY},{cx+shX,shY},sc,st);                        
        dl->AddLine({cx-shX,shY},{cx-elbX,elbY},sc,st);                      
        dl->AddLine({cx+shX,shY},{cx+elbX,elbY},sc,st);                      
        dl->AddLine({cx-elbX,elbY},{cx-handX,handY},sc,st);                  
        dl->AddLine({cx+elbX,elbY},{cx+handX,handY},sc,st);                  
        dl->AddLine({cx,hipY},{cx-kneeX,kneeY},sc,st);                       
        dl->AddLine({cx,hipY},{cx+kneeX,kneeY},sc,st);                       
        dl->AddLine({cx-kneeX,kneeY},{cx-footX,bBot},sc,st);                 
        dl->AddLine({cx+kneeX,kneeY},{cx+footX,bBot},sc,st);                 

        auto jnt=[&](float jx,float jy){
            dl->AddCircleFilled({jx,jy},1.8f,IM_COL32(0,0,0,120),8);
            dl->AddCircleFilled({jx,jy},1.2f,sc,8);};
        jnt(cx,neckY); jnt(cx-shX,shY); jnt(cx+shX,shY);
        jnt(cx-elbX,elbY); jnt(cx+elbX,elbY);
        jnt(cx-handX,handY); jnt(cx+handX,handY);
        jnt(cx,hipY);
        jnt(cx-kneeX,kneeY); jnt(cx+kneeX,kneeY);
    }

    /* ── Head ───────────────────────────────────────────────── */
    dl->AddCircleFilled({cx,hCY},hR,IM_COL32(22,22,34,255));
    dl->AddCircle({cx,hCY},hR,kBorderDim,32,1.f);
    if(e.showHeadDot){
        if(e.headDotFill) dl->AddCircleFilled({cx,hCY},hR,boxCol);
        else              dl->AddCircle({cx,hCY},hR,boxCol,32,1.2f);
    }

    /* ── Health bar ─────────────────────────────────────────── */
    if(e.showHealth){
        constexpr float bw=3.5f;
        float bx = bL - 6.f;
        float fillH = (bBot-bTop)*s_hp;

        int hr=(int)((1.f-s_hp)*2.f*255.f); if(hr>255)hr=255;
        int hg=(int)(s_hp*2.f*255.f);         if(hg>255)hg=255;

        dl->AddRectFilled({bx,bTop},{bx+bw,bBot},IM_COL32(14,14,22,220),1.5f);
        dl->AddRectFilledMultiColor(
            {bx, bBot-fillH}, {bx+bw, bBot},
            IM_COL32(hr,hg,30,220), IM_COL32(hr,hg,30,220),
            IM_COL32(hg/2,hg,30,230), IM_COL32(hg/2,hg,30,230));
        dl->AddRect({bx,bTop},{bx+bw,bBot},IM_COL32(38,38,54,200),1.5f);

        if(e.showHealthNum){
            char hpb[8]; snprintf(hpb,sizeof(hpb),"%d",(int)(s_hp*100.f));
            float fsT=ImGui::GetFontSize()*.66f;
            ImVec2 hts=ImGui::CalcTextSize(hpb);
            dl->AddText(ImGui::GetFont(),fsT,
                {bx+bw*.5f-hts.x*.5f*(fsT/ImGui::GetFontSize()),
                 bBot-fillH-fsT-2.f},
                IM_COL32(hr,hg,30,220),hpb);}
    }

    /* ── Name label ─────────────────────────────────────────── */
    if(e.showNames){
        ImU32 nc=IM_COL32(
            (int)(e.nameCol[0]*255),(int)(e.nameCol[1]*255),
            (int)(e.nameCol[2]*255),230);
        const char* fn="Player";
        ImVec2 nt=ImGui::CalcTextSize(fn);
        float fsN=ImGui::GetFontSize()*.84f;
        float nw=nt.x*(fsN/ImGui::GetFontSize());
        float ny=hCY-hR-fsN-4.f;
        if(e.nameBg) dl->AddRectFilled(
            {cx-nw*.5f-4.f,ny-1.f},{cx+nw*.5f+4.f,ny+fsN+1.f},
            IM_COL32(0,0,0,(int)(e.nameBgAlpha*210)),3.f);
        dl->AddText(ImGui::GetFont(),fsN,{cx-nw*.5f+.5f,ny+.5f},IM_COL32(0,0,0,120),fn);
        dl->AddText(ImGui::GetFont(),fsN,{cx-nw*.5f,ny},nc,fn);
    }

    /* ── Distance ───────────────────────────────────────────── */
    if(e.showDistance){
        const char* ds="42m";
        float fsD=ImGui::GetFontSize()*.76f;
        ImVec2 dt=ImGui::CalcTextSize(ds);
        float dw=dt.x*(fsD/ImGui::GetFontSize());
        dl->AddText(ImGui::GetFont(),fsD,{cx-dw*.5f,bBot+5.f},kTxtDim,ds);
    }

    /* ── Snapline ───────────────────────────────────────────── */
    if(e.showSnapline){
        float sy=(e.snaplineStyle==2)?p.y:(e.snaplineStyle==1)?p.y+H*.5f:p.y+H;
        ImU32 slc=IM_COL32(
            (int)(e.snaplineCol[0]*255),(int)(e.snaplineCol[1]*255),
            (int)(e.snaplineCol[2]*255),100);
        dl->AddLine({cx,sy},{cx,bBot},slc,1.f);
    }

    /* ── Preview watermark ──────────────────────────────────── */
    dl->AddText(ImGui::GetFont(),ImGui::GetFontSize()*.68f,
        {p.x+5.f,p.y+4.f},kTxtFaint,"ESP Preview");

    ImGui::Dummy({W,H});
    ImGui::Dummy({0,4.f});
}

/* ═══════════════════════════════════════════════════════════════
 * HEADER BAR
 * ═══════════════════════════════════════════════════════════════ */
static void DrawHeader(ImDrawList* dl, ImVec2 wp, float ww,
    const char* tabName)
{
    static float s_glow=0.f;
    s_glow+=ImGui::GetIO().DeltaTime*0.4f;

    /* Base */
    dl->AddRectFilled(wp,{wp.x+ww,wp.y+kHdrH},
        IM_COL32(8,8,10,255),kWinR,ImDrawFlags_RoundCornersTop);
    dl->AddRectFilled({wp.x,wp.y+kHdrH-kWinR},{wp.x+ww,wp.y+kHdrH},
        IM_COL32(8,8,10,255));

    /* Subtle top glass highlight */
    dl->AddRectFilledMultiColor(
        wp,{wp.x+ww,wp.y+2.f},
        IM_COL32(255,255,255,14),IM_COL32(255,255,255,14),
        IM_COL32(0,0,0,0),IM_COL32(0,0,0,0));

    /* Animated accent glow sweep across bottom edge */
    {
        float t=fmodf(s_glow,1.f);
        float cx=wp.x+t*ww;
        float gW=ww*.35f;
        dl->PushClipRect(wp,{wp.x+ww,wp.y+kHdrH},true);
        dl->AddRectFilledMultiColor(
            {cx-gW,wp.y+kHdrH-1.f},{cx+gW,wp.y+kHdrH},
            IM_COL32(0,0,0,0),
            IM_COL32((int)(AC4().x*255),(int)(AC4().y*255),(int)(AC4().z*255),90),
            IM_COL32((int)(AC4().x*255),(int)(AC4().y*255),(int)(AC4().z*255),90),
            IM_COL32(0,0,0,0));
        dl->PopClipRect();
    }

    /* Bottom separator */
    dl->AddLine({wp.x,wp.y+kHdrH},{wp.x+ww,wp.y+kHdrH},kBorderDim,1.f);

    float lh=ImGui::GetTextLineHeight();
    float ty=wp.y+(kHdrH-lh)*.5f;

    /* Left: logo + breadcrumb */
    float x=wp.x+kPad+2.f;
    /* Small accent square before logo */
    dl->AddRectFilled({x,ty+2.f},{x+4.f,ty+lh-2.f},AC(220),1.f);
    x+=8.f;
    dl->AddText({x,ty},AC(230),"x9");
    x+=ImGui::CalcTextSize("x9").x+5.f;
    dl->AddText({x,ty},kTxtFaint,"·");
    x+=ImGui::CalcTextSize("·").x+5.f;
    dl->AddText({x,ty},kTxtDim,tabName);

    /* Right: user + live indicator */
    static char s_un[64]={};
    if(!s_un[0]){DWORD sz=(DWORD)sizeof(s_un);if(!GetUserNameA(s_un,&sz))snprintf(s_un,sizeof(s_un),"user");}

    /* Pulsing online dot */
    float pp=sinf(s_glow*3.2f)*.5f+.5f;
    float dotX=wp.x+ww-kPad-ImGui::CalcTextSize(s_un).x-18.f;
    float dotY=ty+lh*.5f;
    dl->AddCircleFilled({dotX,dotY},3.5f,IM_COL32(50,(int)(180+pp*50),80,255),10);
    dl->AddCircleFilled({dotX,dotY},3.5f+pp*2.f,IM_COL32(50,200,80,(int)(18+pp*12)),10);
    /* Username */
    float uw=ImGui::CalcTextSize(s_un).x;
    dl->AddText({wp.x+ww-uw-kPad,ty},kTxtDim,s_un);
}

/* ═══════════════════════════════════════════════════════════════
 * STATUS BAR
 * ═══════════════════════════════════════════════════════════════ */
static void DrawStatusBar(ImDrawList* dl,ImVec2 wp,float ww,float wh)
{
    float y0=wp.y+wh-kBarH;

    /* Bar bg */
    dl->AddRectFilled({wp.x,y0},{wp.x+ww,wp.y+wh},
        IM_COL32(7,7,9,255),kWinR,ImDrawFlags_RoundCornersBottom);
    dl->AddRectFilled({wp.x,y0},{wp.x+ww,y0+kWinR},IM_COL32(7,7,9,255));

    /* Top separator */
    dl->AddLine({wp.x,y0},{wp.x+ww,y0},kBorderDim,1.f);
    /* Subtle accent glow on separator */
    dl->AddLine({wp.x,y0},{wp.x+ww*.3f,y0},AC(28),1.f);

    float fsS=ImGui::GetFontSize()*.72f;
    float ty=y0+(kBarH-fsS)*.5f;

    /* FPS counter + mini sparkline */
    static int  s_fps=0,s_fc=0;
    static DWORD s_ft=0;
    static float s_spark[24]={};
    static int   s_si=0;
    DWORD n2=GetTickCount();s_fc++;
    if(n2-s_ft>=1000){
        s_fps=s_fc; s_fc=0; s_ft=n2;
        s_spark[s_si%24]=(float)s_fps;
        s_si++;
    }

    /* Sparkline — 24 samples, rightmost = latest */
    {
        float spW=40.f, spH=kBarH-4.f;
        float spX=wp.x+ww-spW-kPad;
        float spY=y0+2.f;
        float maxFps=1.f;
        for(int i=0;i<24;i++) if(s_spark[i]>maxFps)maxFps=s_spark[i];
        for(int i=0;i<23;i++){
            float fa=s_spark[(s_si+i)%24]/maxFps;
            float fb2=s_spark[(s_si+i+1)%24]/maxFps;
            dl->AddLine(
                {spX+i*(spW/23.f), spY+spH*(1.f-fa)},
                {spX+(i+1)*(spW/23.f), spY+spH*(1.f-fb2)},
                AC(60),1.f);
        }
    }

    /* FPS text */
    char fb[24];snprintf(fb,sizeof(fb),"%d fps",s_fps);
    float fw2=ImGui::CalcTextSize(fb).x*(fsS/ImGui::GetFontSize());
    dl->AddText(ImGui::GetFont(),fsS,
        {wp.x+ww-fw2-kPad-44.f,ty},kTxtDim,fb);

    /* Centre: link */
    const char* site="x9.gg";
    float sw2=ImGui::CalcTextSize(site).x*(fsS/ImGui::GetFontSize());
    dl->AddText(ImGui::GetFont(),fsS,{wp.x+ww*.5f-sw2*.5f,ty},kTxtFaint,site);

    /* Left: status pill */
    {
        static float s_pp=0.f; s_pp+=ImGui::GetIO().DeltaTime;
        float pp=sinf(s_pp*2.8f)*.5f+.5f;
        /* Online dot */
        dl->AddCircleFilled({wp.x+11.f,y0+kBarH*.5f},
            3.f,IM_COL32(45,(int)(170+pp*40),65,255),10);
        /* Glow ring */
        dl->AddCircle({wp.x+11.f,y0+kBarH*.5f},
            3.f+pp*2.f,IM_COL32(45,200,65,(int)(12+pp*10)),10,1.f);
    }
    dl->AddText(ImGui::GetFont(),fsS,{wp.x+20.f,ty},kTxtDim,"online");
}

/* ═══════════════════════════════════════════════════════════════
 * TAB: AIMBOT
 * ═══════════════════════════════════════════════════════════════ */
static void TabAimbot()
{
    Aimbot::Settings& a=Aimbot::settings;
    float lW=ImGui::GetContentRegionAvail().x*.5f;
    ImGui::BeginChild("##al",{lW,0},false);

    SecHdr("Assist");
    RowCB("Aimbot",      a.enabled, &a.key, "aim_kb");
    RowCB("Sticky",      a.sticky);
    RowCB("Visible Only",a.visibleOnly);
    RowCB("Rage Mode",   a.rageMode);

    SecHdr("Field Of View");
    RowSlideF("Radius",  a.fov,   10.f,800.f,"%.0f");
    RowCB("Dynamic FOV", a.dynamicFov);
    if(a.dynamicFov) RowSlideF("Min Radius",a.dynamicFovMin,10.f,a.fov,"%.0f");
    RowCB("Draw FOV",    a.drawFov);

    SecHdr("Method");
    {const char* mi[]={"Mouse (SendInput)","Memory (Camera)"};
     int mx=(int)a.method;if(RowCombo("",mx,mi,2))a.method=(AimbotMethod)mx;}

    SecHdr("Target");
    {const char* bi[]={"None","Head","Torso","Legs"};
     int bx=(int)a.part;if(RowCombo("Hit Box",bx,bi,4))a.part=(TargetPart)bx;}
    {const char* pi[]={"Crosshair","Lowest HP","Nearest"};
     RowCombo("Priority",a.priorityMode,pi,3);}
    RowSlideF("Max Distance",a.maxAimDist,50.f,2000.f,"%.0f");

    SecHdr("Smoothness");
    RowSlideF("Smooth",a.smooth,1.f,30.f,"%.1f");
    {const char* si[]={"Default","Adaptive","Random"};
     int si2=a.randomSmooth?2:a.adaptiveSmooth?1:0;
     if(RowCombo("Mode",si2,si,3)){a.adaptiveSmooth=(si2==1);a.randomSmooth=(si2==2);}}
    if(a.randomSmooth) RowSlideF("Variance",a.randomSmoothVar,0.f,1.f,"%.2f");

    SecHdr("Prediction");
    RowCB("Enablepred",a.prediction);
    if(a.prediction){
        RowSlideF("Scale",a.predictionScale,.1f,3.f,"%.2f");
        RowCB("Latency Comp",a.latencyComp);
        if(a.latencyComp) RowSlideF("Ping (ms)",a.estimatedPingMs,0.f,300.f,"%.0f");
        RowCB("Gravity Comp",a.gravityComp);
        if(a.gravityComp) RowSlideF("Strength",a.gravityStr,0.f,5.f,"%.2f");}

    SecHdr("Recoil");
    RowCB("RCS Enable",a.recoilControl);
    if(a.recoilControl){
        RowSlideF("RCS Y",a.recoilY,-5.f,5.f,"%.2f");
        RowSlideF("RCS X",a.recoilX,-5.f,5.f,"%.2f");}
    RowCB("Stutter",a.jitter);
    if(a.jitter){
        RowSlideF("Strength",a.jitterStr,.1f,5.f,"%.1f");
        RowCB("Both Axes",a.jitterBothAxes);
        RowSlideF("Frequency",a.jitterFreq,.1f,10.f,"%.1f");}

    SecHdr("Reaction");
    RowSlideF("Delay",a.reactionTime,0.f,1.f,"%.2f s");
    RowCB("Switch Delay",a.switchDelay);

    ImGui::EndChild();
    ImGui::SameLine();

    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBorderDim,1.f);
     ImGui::SetCursorScreenPos({dp.x+2.f,dp.y});}
    ImGui::BeginChild("##ar",{0,0},false);

    SecHdr("FOV Preview");
    FovPreview(a.fov);

    SecHdr("Triggerbot");
    RowCB("Enable",a.triggerbot,&a.triggerbotKey,"tb_kb");
    if(a.triggerbot){
        RowCB("Only In FOV",a.triggerOnlyInFov);
        if(a.triggerOnlyInFov) RowSlideF("FOV",a.triggerFov,1.f,300.f,"%.0f");
        RowCB("Head Only",a.triggerHeadOnly);
        RowCB("Visible Only",a.triggerVisCheck);
        RowCB("No Move",a.triggerNoMove);
        RowSlideF("Delay (ms)",a.triggerbotDelay,0.f,500.f,"%.0f");}

    SecHdr("Silent Aim");
    RowCB("Enable",a.silentAim,&a.silentAimKey,"sa_kb");
    if(a.silentAim){
        /* FOV */
        RowSlideF("FOV",a.silentFov,10.f,800.f,"%.0f px");
        /* Priority mode */
        {const char* spm[]={"Crosshair","Lowest HP","Nearest"};
         RowCombo("Priority",a.silentPriorityMode,spm,3);}
        /* Part */
        {const char* spt[]={"Head","Torso","Random","Closest"};
         RowCombo("Target Part",a.silentPartMode,spt,4);}
        /* Vis / wall */
        RowCB("Visible Only",a.silentVisCheck);
        /* Auto fire */
        RowCB("Auto Fire",a.autoClick);
        if(a.autoClick){RowSlideF("Fire Delay",a.autoClickDelay,.01f,1.f,"%.2f");
            RowCB("Hold",a.autoClickHold);}}

    SecHdr("Hitbox Expand");
    RowCB("Enable",a.hitboxExpand);
    if(a.hitboxExpand){
        RowSlideF("Radius (px)##hbepx",a.hitboxExpandPx,0.f,120.f,"%.0f");
        ImGui::PushStyleColor(ImGuiCol_TextDisabled,ImVec4(0.38f,0.36f,0.44f,1.f));
        ImGui::TextDisabled("  Inflates FOV acceptance zone.");
        ImGui::TextDisabled("  0 = off, 30-80 = typical.");
        ImGui::PopStyleColor();}

    SecHdr("Wall Check");
    RowCB("Enable",a.wallCheck);
    if(a.wallCheck){
        ImGui::PushStyleColor(ImGuiCol_TextDisabled,ImVec4(0.38f,0.36f,0.44f,1.f));
        ImGui::TextDisabled("  Depth-heuristic occlusion filter.");
        ImGui::TextDisabled("  Blocks targets behind geometry.");
        ImGui::PopStyleColor();}

    SecHdr("Auto-Click");
    RowCB("Enable",a.autoClick,&a.key,"ac_kb2");
    if(a.autoClick){
        RowSlideF("Delay",a.autoClickDelay,.01f,1.f,"%.2f");
        RowSlideF("Min Dist",a.autoClickMinDist,1.f,50.f,"%.1f");
        RowCB("Hold Click",a.autoClickHold);}

    SecHdr("Hitmarker");
    RowCB("Enable##hm",a.hitmarker);
    if(a.hitmarker){
        RowSlideF("Size",a.hitmarkerSize,2.f,30.f,"%.0f");
        RowSlideF("Duration",a.hitmarkerDuration,.05f,1.f,"%.2f");}

    SecHdr("Spinbot");
    RowCB("Enable",a.spinbot);
    if(a.spinbot) RowSlideF("Speed",a.spinSpeed,60.f,3600.f,"%.0f deg/s");

    SecHdr("Anti-Aim");
    RowCB("Enable",a.antiAim);
    if(a.antiAim){
        {const char* aaM[]={"Flip","Jitter","Spin+Flip"};
         RowCombo("Mode",a.antiAimMode,aaM,3);}
        RowCB("Flip Pitch",a.antiAimPitch);
        if(a.antiAimMode==1)RowSlideF("Jitter Amp",a.antiAimJitterAmp,10.f,180.f,"%.0f");
        if(a.antiAimMode==2)RowSlideF("Spin Speed",a.spinSpeed,60.f,3600.f,"%.0f");}

    ImGui::EndChild();
}

/* ═══════════════════════════════════════════════════════════════
 * TAB: VISUALS
 * ═══════════════════════════════════════════════════════════════ */
static void TabVisuals()
{
    ESPSettings& e=g_cfg.esp;
    float lW=ImGui::GetContentRegionAvail().x*.5f;
    ImGui::BeginChild("##vl",{lW,0},false);

    SecHdr("ESP");
    RowCB("Enabled",e.enabled);

    SecHdr("Boxes");
    RowCB("Show Box",e.showBox);
    if(e.showBox){
        {const char* bt[]={"2D Full","Corner","Filled"};
         RowCombo("Type",e.boxType,bt,3);}
        RowColor("Box Color",e.boxCol);
        RowSlideF("Thickness",e.boxThickness,.5f,4.f,"%.1f");
        RowCB("Outline",e.boxOutline);RowCB("Rainbow",e.rainbowBox);
        RowCB("Dynamic Color",e.dynBoxColor);RowCB("Pulse",e.pulseBox);}

    SecHdr("Names");
    RowCB("Show Name",e.showNames);RowCB("root",e.showDisplayName);
    if(e.showNames){RowColor("Name Color",e.nameCol);
        RowCB("Background",e.nameBg);
        if(e.nameBg)RowSlideF("BG Alpha",e.nameBgAlpha,0.f,1.f,"%.2f");}

    SecHdr("Health");
    RowCB("Health Bar",e.showHealth);RowCB("HP Number",e.showHealthNum);
    {const char* hs[]={"Left","Right","Bottom"};
     RowCombo("Bar Side",e.healthBarSide,hs,3);}
    RowCB("Low HP Flash",e.lowHpFlash);
    if(e.lowHpFlash)RowSlideF("Threshold",e.lowHpThresh,1.f,100.f,"%.0f");

    SecHdr("Skeleton");
    RowCB("Show Skeleton",e.showSkeleton);
    if(e.showSkeleton){RowColor("Color",e.skeletonCol);
        RowSlideF("Thickness",e.skeletonThick,.5f,4.f,"%.1f");
        RowCB("Rainbow",e.skeletonRainbow);}

    SecHdr("Misc Overlays");
    RowCB("Head Dot",e.showHeadDot);
    if(e.showHeadDot){RowSlideF("Radius",e.headDotRadius,1.f,10.f,"%.1f");
        RowCB("Fill",e.headDotFill);}
    RowCB("Distance",e.showDistance);
    RowCB("Snap Lines",e.showSnapline);
    if(e.showSnapline){
        {const char* ss[]={"Bottom","Centre","Top"};
         RowCombo("Origin",e.snaplineStyle,ss,3);}
        RowColor("Color",e.snaplineCol);}
    RowCB("Offscreen Arrows",e.offscreenArrows);
    if(e.offscreenArrows){RowColor("Arrow Color",e.arrowCol);
        RowSlideF("Size",e.arrowSize,4.f,24.f,"%.0f");}
    RowCB("Show Dead",e.showDead);RowCB("Spectator Alert",e.spectatorAlert);

    ImGui::EndChild();
    ImGui::SameLine();
    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBorderDim,1.f);
     ImGui::SetCursorScreenPos({dp.x+2.f,dp.y});}
    ImGui::BeginChild("##vr",{0,0},false);

    {float pw=ImGui::GetContentRegionAvail().x-kPad*2.f;
     EspPreview(pw,180.f);}

    SecHdr("Visibility");
    RowCB("Vis Check",e.visCheck);
    if(e.visCheck){RowColor("Vis Color",e.visCol);
        RowSlideF("Range",e.visRange,50.f,500.f,"%.0f");}
    RowSlideF("Max Render Dist",e.maxDistance,50.f,2000.f,"%.0f");

    SecHdr("Radar");
    RowCB("Enable",e.showRadar);
    if(e.showRadar){RowSlideF("Size",e.radarSize,80.f,400.f,"%.0f");
        RowSlideF("Range",e.radarRange,100.f,1000.f,"%.0f");
        RowSlideF("Pos X",e.radarX,0.f,1600.f,"%.0f");
        RowSlideF("Pos Y",e.radarY,0.f,900.f,"%.0f");
        RowSlideF("Dot Size",e.radarDotSize,2.f,12.f,"%.1f");
        RowCB("Rotate",e.radarRotate);}

    SecHdr("Chams");
    RowCB("Enable",e.chams);
    if(e.chams){
        {const char* cm[]={"Flat","Mesh"};RowCombo("Mode",e.chamsMode,cm,2);}
        RowColor("Color",e.chamsCol);
        RowSlideF("Alpha",e.chamsAlpha,0.f,1.f,"%.2f");
        RowCB("Pulse",e.chamsPulse);RowCB("Rim Light",e.chamsRimLight);
        RowCB("Wireframe",e.chamsWireframe);RowCB("Rainbow",e.chamsRainbow);}

    SecHdr("Crosshair");
    RowCB("Enable",e.customCrosshair);
    if(e.customCrosshair){
        {const char* cs[]={"Cross","Circle","Dot","T-Shape"};
         RowCombo("Style",e.crosshairStyle,cs,4);}
        RowSlideF("Size",e.crosshairSize,2.f,40.f,"%.0f");
        RowSlideF("Thick",e.crosshairThick,.5f,4.f,"%.1f");
        RowSlideF("Gap",e.crosshairGap,0.f,20.f,"%.0f");
        RowColor("Color",e.crosshairCol);
        RowSlideF("Alpha",e.crosshairAlpha,0.f,1.f,"%.2f");
        RowCB("Dynamic",e.crosshairDynamic);}

    SecHdr("Target HUD");
    RowCB("Enable",e.showTargetHud);
    if(e.showTargetHud){RowCB("Distance",e.tgtHudShowDist);
        RowCB("HP Bar",e.tgtHudShowHp);RowCB("HP Number",e.tgtHudShowHpNum);
        RowSlideF("Alpha",e.tgtHudAlpha,.1f,1.f,"%.2f");
        RowSlideF("Offset X",e.tgtHudOffsetX,-200.f,200.f,"%.0f");
        RowSlideF("Offset Y",e.tgtHudOffsetY,-200.f,200.f,"%.0f");}

    ImGui::EndChild();
}

/* ═══════════════════════════════════════════════════════════════
 * TAB: WORLD
 * ═══════════════════════════════════════════════════════════════ */
static void TabWorld()
{
    WorldSettings& w=g_cfg.world;
    float lW=ImGui::GetContentRegionAvail().x*.5f;
    ImGui::BeginChild("##wl",{lW,0},false);

    SecHdr("Fog");
    RowCB("Enable Override",w.fogEnabled);
    if(w.fogEnabled){RowSlideF("End",w.fogEnd,0.f,100000.f,"%.0f");
        RowSlideF("Start",w.fogStart,0.f,10000.f,"%.0f");
        RowColor("Fog Color",w.fogColor);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        if(ImGui::SmallButton("Dense"))
            {w.fogEnd=500.f;w.fogStart=0.f;w.fogColor[0]=.6f;w.fogColor[1]=.6f;w.fogColor[2]=.6f;}
        ImGui::SameLine(0,4.f);
        if(ImGui::SmallButton("Clear")){w.fogEnd=100000.f;w.fogStart=0.f;}
        ImGui::SameLine(0,4.f);
        if(ImGui::SmallButton("Night"))
            {w.fogEnd=2000.f;w.fogStart=0.f;w.fogColor[0]=.05f;w.fogColor[1]=.05f;w.fogColor[2]=.12f;}
        ImGui::Dummy({0,6.f});}

    SecHdr("Lighting");
    RowCB("Enable Override",w.lightingEnabled);
    if(w.lightingEnabled){
        RowSlideF("Exposure",w.exposureComp,-5.f,5.f,"%.2f");
        RowSlideF("Brightness",w.brightness,0.f,5.f,"%.2f");
        RowColor("Ambient",w.ambient);RowColor("Outdoor Ambient",w.outdoorAmbient);
        RowSlideF("Clock Time (0-24)",w.clockTime,0.f,24.f,"%.1f");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        if(ImGui::SmallButton("Dawn")){w.clockTime=6.f;w.brightness=.8f;w.exposureComp=-.5f;}
        ImGui::SameLine(0,4.f);
        if(ImGui::SmallButton("Noon")){w.clockTime=14.f;w.brightness=1.f;w.exposureComp=0.f;}
        ImGui::SameLine(0,4.f);
        if(ImGui::SmallButton("Midnight")){w.clockTime=0.f;w.brightness=.1f;w.exposureComp=-2.f;}
        ImGui::Dummy({0,6.f});}

    ImGui::EndChild();
    ImGui::SameLine();
    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBorderDim,1.f);
     ImGui::SetCursorScreenPos({dp.x+2.f,dp.y});}
    ImGui::BeginChild("##wr",{0,0},false);

    if(w.fogEnabled){
        ImVec2 cp=ImGui::GetCursorScreenPos();
        float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        ImGui::GetWindowDrawList()->AddRectFilled(cp,{cp.x+cw,cp.y+20.f},
            IM_COL32((int)(w.fogColor[0]*255),(int)(w.fogColor[1]*255),(int)(w.fogColor[2]*255),180),5.f);
        ImGui::Dummy({0,20.f});
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        ImGui::TextDisabled("Fog color preview");ImGui::Dummy({0,8.f});}
    if(w.lightingEnabled){
        {ImVec2 cp=ImGui::GetCursorScreenPos();float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;
         ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
         ImGui::GetWindowDrawList()->AddRectFilled(cp,{cp.x+cw,cp.y+16.f},
            IM_COL32((int)(w.ambient[0]*255),(int)(w.ambient[1]*255),(int)(w.ambient[2]*255),160),4.f);
         ImGui::Dummy({0,16.f});}
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        ImGui::TextDisabled("Ambient");ImGui::Dummy({0,6.f});
        {ImVec2 cp=ImGui::GetCursorScreenPos();float cw=ImGui::GetContentRegionAvail().x-kPad*2.f;
         ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
         ImGui::GetWindowDrawList()->AddRectFilled(cp,{cp.x+cw,cp.y+16.f},
            IM_COL32((int)(w.outdoorAmbient[0]*255),(int)(w.outdoorAmbient[1]*255),(int)(w.outdoorAmbient[2]*255),160),4.f);
         ImGui::Dummy({0,16.f});}
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        ImGui::TextDisabled("Outdoor ambient");ImGui::Dummy({0,6.f});}

    ImGui::EndChild();
}

/* ═══════════════════════════════════════════════════════════════
 * TAB: MOVEMENT
 * ═══════════════════════════════════════════════════════════════ */
static void TabMovement()
{
    MovementSettings& m=g_cfg.movement;
    float lW=ImGui::GetContentRegionAvail().x*.5f;
    ImGui::BeginChild("##ml",{lW,0},false);

    SecHdr("Walk Speed");
    RowCB("Override",m.useSpeed);
    if(m.useSpeed){RowSlideF("Speed",m.walkSpeed,1.f,200.f,"%.1f");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        if(ImGui::SmallButton("Apply"))Roblox::SetWalkSpeed(m.walkSpeed);
        ImGui::SameLine(0,4.f);
        if(ImGui::SmallButton("Reset")){m.walkSpeed=16.f;Roblox::SetWalkSpeed(16.f);}
        ImGui::Dummy({0,4.f});}
    RowCB("Always Sprint",m.alwaysSprint);

    SecHdr("Jump");
    RowCB("Override",m.useJump);
    if(m.useJump){RowSlideF("Power",m.jumpPower,1.f,500.f,"%.1f");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        if(ImGui::SmallButton("Apply"))Roblox::SetJumpPower(m.jumpPower);
        ImGui::SameLine(0,4.f);
        if(ImGui::SmallButton("Reset")){m.jumpPower=50.f;Roblox::SetJumpPower(50.f);}
        ImGui::Dummy({0,4.f});}
    RowCB("Infinite Jump",m.infiniteJump);RowCB("Bunny Hop",m.bunnyHop);

    SecHdr("Physics");
    RowCB("No Slip",m.noSlip);RowCB("Low Gravity",m.lowGravity);
    if(m.lowGravity)RowSlideF("Gravity Scale",m.gravityScale,.01f,1.f,"%.2f");
    RowCB("Fly",m.fly);
    if(m.fly)RowSlideF("Fly Speed",m.flySpeed,1.f,500.f,"%.0f");
    RowCB("Noclip",m.noclip);

    SecHdr("Survival");
    RowCB("God Mode",m.godMode);RowCB("Regen HP",m.regenHP);
    if(m.regenHP)RowSlideF("Rate (hp/s)",m.regenRate,.1f,20.f,"%.1f");

    SecHdr("Utility");
    RowCB("Anti-AFK",m.antiAFK);RowCB("Anti-Void",m.antiVoid);
    if(m.antiVoid)RowSlideF("Floor Y",m.antiVoidY,-2000.f,0.f,"%.0f");

    SecHdr("Actions");
    ActBtn("Apply Walk Speed"); if(ImGui::IsItemClicked())Roblox::SetWalkSpeed(m.walkSpeed);
    ActBtn("Reset Walk Speed"); if(ImGui::IsItemClicked()){m.walkSpeed=16.f;Roblox::SetWalkSpeed(16.f);}

    ImGui::EndChild();
    ImGui::SameLine();
    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBorderDim,1.f);
     ImGui::SetCursorScreenPos({dp.x+2.f,dp.y});}
    ImGui::BeginChild("##mr",{0,0},false);

    SecHdr("Players");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ImGui::TextDisabled("Click [F] to toggle friendly");
    ImGui::Dummy({0,5.f});

    const auto& players=Roblox::GetPlayers();
    float listW=ImGui::GetContentRegionAvail().x;
    ESPSettings& e=g_cfg.esp;
    for(const auto& pl:players){
        if(pl.isLocalPlayer) continue;
        bool iF=FriendlyList::Has(pl.name);
        ImGui::PushStyleColor(ImGuiCol_Button,       iF?ImVec4(.15f,.35f,.82f,1.f):kVRaise);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,iF?ImVec4(.20f,.44f,1.f,1.f):kVRaH);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, iF?ImVec4(.10f,.28f,.68f,1.f):kVRaA);
        ImGui::PushStyleColor(ImGuiCol_Text,         iF?ImVec4(1,1,1,1):kVDim);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        char bid2[64];snprintf(bid2,sizeof(bid2),"F##fl_%llX",(unsigned long long)pl.ptr);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{4.f,1.f});
        if(ImGui::SmallButton(bid2))FriendlyList::Toggle(pl.name);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);ImGui::SameLine(0,6.f);
        float hp2=(pl.maxHealth>0.f)?(pl.health/pl.maxHealth):0.f;
        hp2=hp2<0.f?0.f:hp2>1.f?1.f:hp2;
        ImU32 ac2=iF?IM_COL32(60,140,255,200):
            IM_COL32((int)(e.boxCol[0]*255),(int)(e.boxCol[1]*255),(int)(e.boxCol[2]*255),200);
        {ImVec2 sp2=ImGui::GetCursorScreenPos();
         ImDrawList* dl2=ImGui::GetWindowDrawList();
         dl2->AddRectFilled(sp2,{sp2.x+14.f,sp2.y+14.f},kSurface);
         dl2->AddCircleFilled({sp2.x+7.f,sp2.y+4.5f},3.f,ac2);
         dl2->AddRectFilled({sp2.x+3.f,sp2.y+9.f},{sp2.x+11.f,sp2.y+13.f},ac2);
         ImGui::Dummy({14.f,14.f});}
        ImGui::SameLine(0,5.f);
        ImGui::TextColored(iF?ImVec4(.4f,.65f,1.f,1.f):ImVec4(.75f,.75f,.82f,1.f),
            "%s",pl.name.c_str());
        float bw2=44.f,bh2=3.f;
        ImVec2 cp2=ImGui::GetCursorScreenPos();
        float bx2=ImGui::GetWindowPos().x+listW-bw2-6.f,by2=cp2.y-ImGui::GetTextLineHeight()-1.f;
        ImDrawList* dl3=ImGui::GetWindowDrawList();
        dl3->AddRectFilled({bx2,by2},{bx2+bw2,by2+bh2},kSurface);
        int r2=(int)((1.f-hp2)*2.f*255.f);if(r2>255)r2=255;
        int g2=(int)(hp2*2.f*255.f);      if(g2>255)g2=255;
        if(hp2>0.f)dl3->AddRectFilled({bx2,by2},{bx2+bw2*hp2,by2+bh2},IM_COL32(r2,g2,30,200));
        ImVec2 sp3=ImGui::GetCursorScreenPos();
        dl3->AddLine(sp3,{sp3.x+listW,sp3.y},kSep,1.f);
        ImGui::Dummy({0,2.f});}
    if(players.empty()||[&]{int c=0;for(auto&pl:players)if(!pl.isLocalPlayer)c++;return c;}()==0){
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
        ImGui::TextDisabled("no players detected");}

    ImGui::EndChild();
}

/* ═══════════════════════════════════════════════════════════════
 * TAB: OPTIONS 
 * ═══════════════════════════════════════════════════════════════ */
static void TabOptions()
{
    float lW=ImGui::GetContentRegionAvail().x*.5f;
    ImGui::BeginChild("##ol",{lW,0},false);

    SecHdr("Theme");
    {ImVec2 p=ImGui::GetCursorScreenPos();
     float  lh=ImGui::GetTextLineHeight();
     ImGui::GetWindowDrawList()->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtDim,"Accent Color");
     ImGui::Dummy({0,kRH});
     ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
     constexpr int kN=8;float swSz=15.f,swGap=4.f;
     for(int i=0;i<kN;i++){
         ImVec4 col=kAccents[i].col;
         bool sel=(!s_acCustMode&&s_acIdx==i);
         ImVec2 sp2=ImGui::GetCursorScreenPos();
         ImDrawList* dl2=ImGui::GetWindowDrawList();
         if(sel)dl2->AddRect({sp2.x-2.f,sp2.y-2.f},{sp2.x+swSz+2.f,sp2.y+swSz+2.f},
             AC(200),3.f,0,1.5f);
         ImGui::PushStyleColor(ImGuiCol_Button,col);
         ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
             {fminf(col.x*1.2f,1.f),fminf(col.y*1.2f,1.f),fminf(col.z*1.2f,1.f),1.f});
         ImGui::PushStyleColor(ImGuiCol_ButtonActive,col);
         ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,3.f);
         char lid[16];snprintf(lid,sizeof(lid),"##sw%d",i);
         if(ImGui::Button(lid,{swSz,swSz}))
             {s_acIdx=i;s_acCustMode=false;SyncAccent();}
         ImGui::PopStyleColor(3);ImGui::PopStyleVar();
         if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",kAccents[i].name);
         if(i<kN-1)ImGui::SameLine(0,swGap);}
     ImGui::Dummy({0,5.f});
     ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
     if(ImGui::ColorEdit3("##cac",s_acCust,
         ImGuiColorEditFlags_NoLabel|ImGuiColorEditFlags_NoBorder))
         {s_acCustMode=true;SyncAccent();}
     ImGui::SameLine(0,6.f);ImGui::TextDisabled("Custom");
     ImGui::Dummy({0,5.f});}

    {ImVec2 p=ImGui::GetCursorScreenPos();float lh=ImGui::GetTextLineHeight();
     ImGui::GetWindowDrawList()->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,"Menu Key");
     ImVec2 vs=ImGui::CalcTextSize("del");
     ImGui::GetWindowDrawList()->AddText(
         {p.x+ImGui::GetContentRegionAvail().x-kPad-vs.x-10.f,p.y+(kRH-lh)*.5f},
         kTxtDim,"del");
     ImGui::Dummy({0,kRH});}

    SecHdr("UI Theme");
    {struct TB{const char*l;const char*d;};
     static constexpr TB kB[3]={{"Neko","Cat paws"},{"Industrial","Dark flat"},{"Original","Light"}};
     float tw=(ImGui::GetContentRegionAvail().x-kPad*2.f-8.f)/3.f;
     ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
     for(int i=0;i<3;i++){
         bool act=(g_cfg.uiTheme==i);
         ImVec4 bg=act?ImVec4(AC4().x*.28f,AC4().y*.28f,AC4().z*.46f,1.f):kVRaise;
         ImGui::PushStyleColor(ImGuiCol_Button,        bg);
         ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kVRaH);
         ImGui::PushStyleColor(ImGuiCol_ButtonActive,  kVRaA);
         ImGui::PushStyleColor(ImGuiCol_Text,
             act?ImVec4(AC4().x*1.5f,AC4().y*1.5f,AC4().z*1.5f,1.f):kVDim);
         ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,5.f);
         ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
         ImGui::PushStyleColor(ImGuiCol_Border,
             act?ImVec4(AC4().x*.5f,AC4().y*.5f,AC4().z*.7f,1.f)
                :ImVec4(38/255.f,38/255.f,54/255.f,1.f));
         char bid[32];snprintf(bid,sizeof(bid),"%s##thsw%d",kB[i].l,i);
         if(ImGui::Button(bid,{tw,22.f}))g_cfg.uiTheme=i;
         if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",kB[i].d);
         ImGui::PopStyleColor(5);ImGui::PopStyleVar(2);
         if(i<2)ImGui::SameLine(0,4.f);}
     ImGui::Dummy({0,6.f});}

    SecHdr("General");
    RowCB("Lock Menu Position",g_cfg.menuLocked);
    RowSlideF("Menu Alpha",g_cfg.menuAlpha,.3f,1.f,"%.2f");
    RowCB("Menu Glow",g_cfg.menuGlow);
    if(g_cfg.menuGlow)RowSlideF("Glow Intensity",g_cfg.menuGlowIntens,.1f,1.f,"%.2f");
    RowCB("ESP Box Glow",g_cfg.espBoxGlow);
    if(g_cfg.espBoxGlow)RowSlideF("ESP Glow Intensity",g_cfg.espBoxGlowIntens,.1f,1.f,"%.2f");

    SecHdr("User Interface");
    RowCB("Dark Background",g_cfg.fx.darken);
    if(g_cfg.fx.darken)RowSlideF("Strength",g_cfg.fx.darkenAlpha,0.f,1.f,"%.2f");
    RowCB("Vignette",g_cfg.fx.vignette);
    if(g_cfg.fx.vignette)RowSlideF("Strength",g_cfg.fx.vignetteStr,0.f,1.f,"%.2f");
    RowCB("Scanlines",g_cfg.fx.scanlines);
    if(g_cfg.fx.scanlines)RowSlideF("Alpha",g_cfg.fx.scanlineAlpha,0.f,.5f,"%.2f");
    RowCB("Snow Effect",g_cfg.fx.snow);
    if(g_cfg.fx.snow){RowSlideI("Count",g_cfg.fx.snowCount,20,400);
        RowSlideF("Speed",g_cfg.fx.snowSpeed,.1f,4.f,"%.1f");
        RowSlideF("Opacity",g_cfg.fx.snowOpacity,.1f,1.f,"%.2f");}
    RowCB("RGB Border",g_cfg.fx.rgbBorder);
    if(g_cfg.fx.rgbBorder)RowSlideF("Speed",g_cfg.fx.rgbSpeed,.1f,5.f,"%.1f");

    if(g_cfg.showSpotify){SecHdr("Spotify");X9DrawMenuSpotify(1);}

    ImGui::EndChild();
    ImGui::SameLine();
    {ImVec2 dp=ImGui::GetCursorScreenPos();
     ImGui::GetWindowDrawList()->AddLine(dp,{dp.x,dp.y+ImGui::GetContentRegionAvail().y},kBorderDim,1.f);
     ImGui::SetCursorScreenPos({dp.x+2.f,dp.y});}
    ImGui::BeginChild("##or",{0,0},false);

    SecHdr("Session");
    {const auto& plrs=Roblox::GetPlayers();
     int alive=0;for(const auto& pl:plrs)if(pl.valid&&pl.health>0&&!pl.isLocalPlayer)alive++;
     char lb[48];
     snprintf(lb,sizeof(lb),"%d",alive);
     {ImVec2 p=ImGui::GetCursorScreenPos();float lh=ImGui::GetTextLineHeight();
      ImGui::GetWindowDrawList()->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,"Players visible");
      ImVec2 vs=ImGui::CalcTextSize(lb);
      ImGui::GetWindowDrawList()->AddText(
          {p.x+ImGui::GetContentRegionAvail().x-kPad-vs.x,p.y+(kRH-lh)*.5f},kTxtDim,lb);
      ImGui::Dummy({0,kRH});}
     snprintf(lb,sizeof(lb),"%d",(int)plrs.size());
     {ImVec2 p=ImGui::GetCursorScreenPos();float lh=ImGui::GetTextLineHeight();
      ImGui::GetWindowDrawList()->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,"Total players");
      ImVec2 vs=ImGui::CalcTextSize(lb);
      ImGui::GetWindowDrawList()->AddText(
          {p.x+ImGui::GetContentRegionAvail().x-kPad-vs.x,p.y+(kRH-lh)*.5f},kTxtDim,lb);
      ImGui::Dummy({0,kRH});}}

    SecHdr("Locked Target");
    {const PlayerData* tgt=Aimbot::GetTarget();
     auto lv=[&](const char* l, const char* v){
         ImVec2 p=ImGui::GetCursorScreenPos();float lh=ImGui::GetTextLineHeight();
         ImGui::GetWindowDrawList()->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,l);
         ImVec2 vs=ImGui::CalcTextSize(v);
         ImGui::GetWindowDrawList()->AddText(
             {p.x+ImGui::GetContentRegionAvail().x-kPad-vs.x,p.y+(kRH-lh)*.5f},kTxtDim,v);
         ImGui::GetWindowDrawList()->AddLine({p.x,p.y+kRH-1.f},
             {p.x+ImGui::GetContentRegionAvail().x,p.y+kRH-1.f},kSep,1.f);
         ImGui::Dummy({0,kRH});};
     if(tgt){char lb[48];
         lv("Name",tgt->name.c_str());
         snprintf(lb,sizeof(lb),"%.0f / %.0f",tgt->health,tgt->maxHealth);lv("HP",lb);
         float d=sqrtf(tgt->rootPos[0]*tgt->rootPos[0]+tgt->rootPos[1]*tgt->rootPos[1]+tgt->rootPos[2]*tgt->rootPos[2]);
         snprintf(lb,sizeof(lb),"%.1f studs",d);lv("Distance",lb);}
     else{ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);ImGui::TextDisabled("none");}}

    SecHdr("Utils");
    if(ActBtn("Reset All To Defaultsrst")){g_cfg=Config{};Aimbot::settings=Aimbot::Settings{};}

    SecHdr("Build");
    {auto lv=[&](const char* l,const char* v){
         ImVec2 p=ImGui::GetCursorScreenPos();float lh=ImGui::GetTextLineHeight();
         ImGui::GetWindowDrawList()->AddText({p.x+kPad,p.y+(kRH-lh)*.5f},kTxtMid,l);
         ImVec2 vs=ImGui::CalcTextSize(v);
         ImGui::GetWindowDrawList()->AddText(
             {p.x+ImGui::GetContentRegionAvail().x-kPad-vs.x,p.y+(kRH-lh)*.5f},kTxtDim,v);
         ImGui::Dummy({0,kRH});};
     lv("Version","x9 rebuild");lv("Roblox","version-b130242ed064436f");}

    ImGui::EndChild();
}

/* ═══════════════════════════════════════════════════════════════
 * TAB: CONFIG
 * ═══════════════════════════════════════════════════════════════ */
static void TabConfig()
{
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    ConfigUI::Draw(ImGui::GetIO().DeltaTime);
}

/* ═══════════════════════════════════════════════════════════════
 * TAB: LUA
 * ═══════════════════════════════════════════════════════════════ */
static void TabLua()
{
    float w=ImGui::GetContentRegionAvail().x;
    SecHdr("Script");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    static char s_luaBuf[4096]="-- x9 Lua\n";
    ImGui::SetNextItemWidth(w-kPad*2.f);
    ImGui::InputTextMultiline("lua",s_luaBuf,sizeof(s_luaBuf),
        {w-kPad*2.f,150.f},ImGuiInputTextFlags_AllowTabInput);
    ImGui::Dummy({0,4.f});
    ActBtn("Executeluaexec");

    SecHdr("Console");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+kPad);
    static char s_log[2048]="";
    ImGui::SetNextItemWidth(w-kPad*2.f);
    ImGui::InputTextMultiline("lualog",s_log,sizeof(s_log),
        {w-kPad*2.f,80.f},ImGuiInputTextFlags_ReadOnly);

    SecHdr("Actions");
    ActBtn("Clear Console");
    ActBtn("Restart Executor");
}

/* ═══════════════════════════════════════════════════════════════
 * MAIN RENDER
 * ═══════════════════════════════════════════════════════════════ */
void Render()
{
    if(!s_visible) return;

    static const char* kTabs[]=
        {"Aimbot","Visuals","World","Movement","Options","Config","Lua"};
    static constexpr int kTabN=7;
    constexpr float WW=660.f, WH=500.f;

    ImGui::PushStyleColor(ImGuiCol_WindowBg,kVBg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,kWinR);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0.f,0.f});

    ImGui::SetNextWindowSize({WW,WH},ImGuiCond_Once);
    ImGui::SetNextWindowPos({(float)g_cfg.menuX,(float)g_cfg.menuY},ImGuiCond_Once);

    ImGuiWindowFlags wf=
        ImGuiWindowFlags_NoScrollbar|
        ImGuiWindowFlags_NoScrollWithMouse|
        ImGuiWindowFlags_NoTitleBar;
    if(g_cfg.menuLocked)wf|=ImGuiWindowFlags_NoMove;

    if(!ImGui::Begin("##x9_root",nullptr,wf)){
        ImGui::PopStyleVar(2);ImGui::PopStyleColor();ImGui::End();return;}
    ImGui::PopStyleVar(2);ImGui::PopStyleColor();

    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 wp=ImGui::GetWindowPos();
    float  ww2=ImGui::GetWindowWidth(),wh2=ImGui::GetWindowHeight();

    DrawHeader(dl,wp,ww2,kTabs[s_tab]);

    ImGui::SetCursorPos({0.f,kHdrH});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0.f,0.f});
    DrawTabBar(kTabs,kTabN,s_tab);
    ImGui::PopStyleVar();

    float cH=wh2-kHdrH-kTabH-kBarH-1.f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0.f,2.f});
    ImGui::BeginChild("##cnt",{ww2,cH},false,ImGuiWindowFlags_NoScrollbar);

    switch(s_tab){
        case 0:TabAimbot();   break;
        case 1:TabVisuals();  break;
        case 2:TabWorld();    break;
        case 3:TabMovement(); break;
        case 4:TabOptions();  break;
        case 5:TabConfig();   break;
        case 6:TabLua();      break;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImVec2 rootPos  = ImGui::GetWindowPos();
    ImVec2 rootSize = ImGui::GetWindowSize();
    g_cfg.menuX = (int)rootPos.x;
    g_cfg.menuY = (int)rootPos.y;

    DrawStatusBar(dl,wp,ww2,wh2);

    ImGui::End();

    if(g_cfg.menuGlow){
        ImVec2 wpos = rootPos;
        ImVec2 wmax = {wpos.x+rootSize.x, wpos.y+rootSize.y};
        float  gi   = g_cfg.menuGlowIntens;
        ImDrawList* fg=ImGui::GetForegroundDrawList();
        fg->AddRect({wpos.x-5.f,wpos.y-5.f},{wmax.x+5.f,wmax.y+5.f},AC((int)( 6*gi)),kWinR+5.f,0,12.f);
        fg->AddRect({wpos.x-2.f,wpos.y-2.f},{wmax.x+2.f,wmax.y+2.f},AC((int)(22*gi)),kWinR+2.f,0, 5.f);
        fg->AddRect({wpos.x-1.f,wpos.y-1.f},{wmax.x+1.f,wmax.y+1.f},AC((int)(55*gi)),kWinR+1.f,0, 2.f);
        fg->AddRect(wpos,wmax,                                          AC((int)(100*gi)),kWinR,  0, 1.f);}
}

} // namespace ThemeIndustrial