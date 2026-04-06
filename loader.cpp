/*
 * loader.cpp  —  x9  Login / Loading screen
 *
 * Designed to match the Matcha UI aesthetic:
 *   · Same near-black palette as the menu (#0D0D10)
 *   · Same header breadcrumb format: "x9 · Login · Pro"
 *   · Same status bar at bottom: green dot + count + build
 *   · Same monochrome text hierarchy, same border radius
 *   · Login form: square checkbox style, flat fields with thin border
 *   · Loading: clean checklist with step indicators + slim progress bar
 */

#include "loader.h"
#include "imgui.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <cmath>
#include <cstdio>
#include <cstring>

/* ═════════════════════════════════════════════════════════════
   STATE
   ═════════════════════════════════════════════════════════════ */
static Loader::State s_state    = Loader::State::Select;
static float         s_anim     = 0.f;
static float         s_progress = 0.f;
static float         s_fade     = 1.f;
static bool          s_fading   = false;

static char  s_user[64]  = {};
static char  s_pass[64]  = {};
static bool  s_badCreds  = false;
static float s_errTimer  = 0.f;
static float s_pollTimer = 0.f;
static bool  s_robloxOk  = false;
static bool  s_userFocus = false;
static bool  s_passFocus = false;

/* Accent synced from menu */
static float s_aR=0.47f, s_aG=0.34f, s_aB=0.72f;

/* Loading steps */
static const char* kSteps[] = {
    "Verifying credentials",
    "Connecting to server",
    "Validating session",
    "Locating game process",
    "Mapping memory",
    "Loading modules",
    "Ready",
};
static constexpr int kStepCount = 7;

/* ═════════════════════════════════════════════════════════════
   PALETTE  — mirrors menu_theme_industrial.h exactly
   ═════════════════════════════════════════════════════════════ */
static constexpr ImU32 kBg       = IM_COL32( 13,  13,  16, 255);
static constexpr ImU32 kSurface  = IM_COL32( 18,  18,  24, 255);
static constexpr ImU32 kRaised   = IM_COL32( 22,  22,  30, 255);
static constexpr ImU32 kBorder   = IM_COL32( 38,  38,  54, 255);
static constexpr ImU32 kBorderDim= IM_COL32( 26,  26,  38, 255);
static constexpr ImU32 kSep      = IM_COL32( 20,  20,  30, 255);
static constexpr ImU32 kTxtHi    = IM_COL32(228, 228, 236, 255);
static constexpr ImU32 kTxtMid   = IM_COL32(175, 175, 188, 255);
static constexpr ImU32 kTxtDim   = IM_COL32( 90,  90, 108, 255);
static constexpr ImU32 kTxtFaint = IM_COL32( 52,  52,  66, 255);
static constexpr float kR        = 8.f;   /* window corner radius */

static ImU32 AC(int a=255) {
    a=a<0?0:a>255?255:a;
    return IM_COL32((int)(s_aR*255),(int)(s_aG*255),(int)(s_aB*255),a);
}

static bool IsRobloxRunning() {
    HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snap==INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{}; pe.dwSize=sizeof(pe); bool found=false;
    if(Process32FirstW(snap,&pe)) do {
        if(_wcsicmp(pe.szExeFile,L"RobloxPlayerBeta.exe")==0){found=true;break;}
    } while(Process32NextW(snap,&pe));
    CloseHandle(snap);
    return found;
}

/* ═════════════════════════════════════════════════════════════
   PUBLIC API
   ═════════════════════════════════════════════════════════════ */
void Loader::Init() {
    s_state=State::Select; s_anim=s_progress=0.f;
    s_fade=1.f; s_fading=false;
    memset(s_user,0,sizeof(s_user));
    memset(s_pass,0,sizeof(s_pass));
    s_badCreds=false; s_errTimer=0.f;
    s_pollTimer=0.f; s_robloxOk=false;
    s_userFocus=s_passFocus=false;
}
void          Loader::SetAccent(float r,float g,float b){s_aR=r;s_aG=g;s_aB=b;}
bool          Loader::IsDone()   {return s_state==State::Done;}
Loader::State Loader::GetState() {return s_state;}

/* ═════════════════════════════════════════════════════════════
   RENDER
   ═════════════════════════════════════════════════════════════ */
void Loader::Render()
{
    float dt=ImGui::GetIO().DeltaTime;
    if(dt>0.1f) dt=0.1f;
    s_anim+=dt;
    if(s_errTimer>0.f) s_errTimer-=dt;

    /* Fade-out transition to main */
    if(s_fading){
        s_fade-=dt*2.2f;
        if(s_fade<=0.f){s_state=State::Done;return;}
    }

    /* Loading animation drives progress */
    if(s_state==State::Loading){
        float spd=(1.f-s_progress)*0.45f+0.05f;
        s_progress+=dt*spd;
        if(s_progress>=1.f){s_progress=1.f;s_fading=true;}
    }

    /* Poll Roblox */
    if(s_state==State::Select){
        s_pollTimer-=dt;
        if(s_pollTimer<=0.f){s_pollTimer=1.8f;s_robloxOk=IsRobloxRunning();}
    }

    float f  = s_fade;
    float sw = ImGui::GetIO().DisplaySize.x;
    float sh = ImGui::GetIO().DisplaySize.y;

    /* Fullscreen transparent host window */
    ImGui::SetNextWindowPos({0,0},ImGuiCond_Always);
    ImGui::SetNextWindowSize({sw,sh},ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
    ImGui::Begin("##x9ldr",nullptr,
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoBringToFrontOnFocus|
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar(2);
    ImDrawList* dl=ImGui::GetWindowDrawList();

    /* ── Card geometry ──────────────────────────────────────── */
    constexpr float CW=540.f, CH=420.f;
    float cx = sw*.5f - CW*.5f;
    float cy = sh*.5f - CH*.5f;

    /* Very subtle fullscreen dark backdrop */
    dl->AddRectFilled({0,0},{sw,sh}, IM_COL32(0,0,0,(int)(f*140)));

    /* Drop shadow */
    dl->AddRectFilled({cx+4.f,cy+8.f},{cx+CW+4.f,cy+CH+8.f},
        IM_COL32(0,0,0,(int)(f*55)), kR+2.f);
    dl->AddRectFilled({cx+2.f,cy+4.f},{cx+CW+2.f,cy+CH+4.f},
        IM_COL32(0,0,0,(int)(f*35)), kR+1.f);

    /* Card body — exactly same bg as menu */
    dl->AddRectFilled({cx,cy},{cx+CW,cy+CH},
        IM_COL32(13,13,16,(int)(f*255)), kR);
    dl->AddRect({cx,cy},{cx+CW,cy+CH},
        IM_COL32(26,26,38,(int)(f*255)), kR, 0, 1.f);

    /* ── Header bar — "x9 · Login · Pro" ───────────────────── */
    constexpr float HDR=36.f;
    /* Header bg — top-rounded only */
    dl->AddRectFilled({cx,cy},{cx+CW,cy+HDR},
        IM_COL32(13,13,16,(int)(f*255)), kR, ImDrawFlags_RoundCornersTop);
    dl->AddRectFilled({cx,cy+HDR-kR},{cx+CW,cy+HDR},
        IM_COL32(13,13,16,(int)(f*255)));
    dl->AddLine({cx,cy+HDR},{cx+CW,cy+HDR},
        IM_COL32(26,26,38,(int)(f*255)),1.f);

    float fsS  = ImGui::GetFontSize()*.86f;
    float lh   = ImGui::GetTextLineHeight();
    float hty  = cy+(HDR-lh)*.5f+1.f;
    float hx   = cx+14.f;

    /* x9 */
    dl->AddText(ImGui::GetFont(),fsS,{hx,hty},
        IM_COL32(228,228,236,(int)(f*255)),"x9");
    hx+=ImGui::CalcTextSize("x9").x*(fsS/ImGui::GetFontSize())+5.f;
    /* · */
    dl->AddText(ImGui::GetFont(),fsS,{hx,hty},
        IM_COL32(52,52,66,(int)(f*255)),"\xC2\xB7");
    hx+=ImGui::CalcTextSize("\xC2\xB7").x*(fsS/ImGui::GetFontSize())+5.f;
    /* Login */
    dl->AddText(ImGui::GetFont(),fsS,{hx,hty},
        IM_COL32(90,90,108,(int)(f*255)),
        s_state==State::Select?"Login":"Authenticating");
    hx+=ImGui::CalcTextSize("Login").x*(fsS/ImGui::GetFontSize())+5.f;
    /* · */
    dl->AddText(ImGui::GetFont(),fsS,{hx,hty},
        IM_COL32(52,52,66,(int)(f*255)),"\xC2\xB7");
    hx+=ImGui::CalcTextSize("\xC2\xB7").x*(fsS/ImGui::GetFontSize())+5.f;
    /* Pro badge */
    float bW=ImGui::CalcTextSize("Pro").x*(fsS/ImGui::GetFontSize())+10.f;
    float bY=cy+(HDR-15.f)*.5f;
    dl->AddRect({hx,bY},{hx+bW,bY+15.f},
        IM_COL32(38,38,54,(int)(f*255)), 7.f, 0, 1.f);
    dl->AddText(ImGui::GetFont(),fsS,{hx+5.f,hty},
        IM_COL32(175,175,188,(int)(f*255)),"Pro");

    /* ── Status bar — bottom-rounded only ──────────────────── */
    constexpr float BAR=24.f;
    float barY=cy+CH-BAR;
    dl->AddRectFilled({cx,barY},{cx+CW,cy+CH},
        IM_COL32(13,13,16,(int)(f*255)), kR, ImDrawFlags_RoundCornersBottom);
    dl->AddRectFilled({cx,barY},{cx+CW,barY+kR},
        IM_COL32(13,13,16,(int)(f*255)));
    dl->AddLine({cx,barY},{cx+CW,barY},
        IM_COL32(26,26,38,(int)(f*255)),1.f);

    float fsB=ImGui::GetFontSize()*.74f;
    float bty=barY+(BAR-fsB)*.5f;

    /* Green dot + Roblox status */
    ImU32 dotC = s_robloxOk
        ? IM_COL32(60,210,80,(int)(f*220))
        : IM_COL32(80,80,100,(int)(f*180));
    dl->AddCircleFilled({cx+11.f,barY+BAR*.5f},3.5f,dotC,10);
    dl->AddText(ImGui::GetFont(),fsB,{cx+20.f,bty},
        IM_COL32(90,90,108,(int)(f*255)),
        s_robloxOk?"Roblox detected":"Roblox not found");

    /* Center */
    const char* site="x9.gg/discord";
    float sw2=ImGui::CalcTextSize(site).x*(fsB/ImGui::GetFontSize());
    dl->AddText(ImGui::GetFont(),fsB,{cx+CW*.5f-sw2*.5f,bty},
        IM_COL32(52,52,66,(int)(f*255)),site);

    /* Right: build */
    const char* build="Build: x9 rebuild";
    float bfw=ImGui::CalcTextSize(build).x*(fsB/ImGui::GetFontSize());
    dl->AddText(ImGui::GetFont(),fsB,{cx+CW-bfw-10.f,bty},
        IM_COL32(52,52,66,(int)(f*255)),build);

    /* ── Content area ───────────────────────────────────────── */
    constexpr float PAD=36.f;
    float rx=cx+PAD, rw=CW-PAD*2.f;
    float ry=cy+HDR+PAD;
    float lh2=ImGui::GetTextLineHeight();

    if(s_state==State::Select)
    {
        /* ── Title group ────────────────────────────────────── */
        /* Welcome label */
        float fsT=ImGui::GetFontSize()*.74f;
        dl->AddText(ImGui::GetFont(),fsT,{rx,ry},
            IM_COL32(90,90,108,(int)(f*255)),"WELCOME BACK");
        ry+=fsT+8.f;

        /* Main title */
        dl->AddText({rx,ry},
            IM_COL32(228,228,236,(int)(f*240)),"Sign in to x9");
        ry+=lh2+4.f;

        /* Subtitle */
        dl->AddText(ImGui::GetFont(),fsT,{rx,ry},
            IM_COL32(90,90,108,(int)(f*200)),"Enter your credentials below");
        ry+=fsT+16.f;

        /* Thin divider */
        dl->AddLine({rx,ry},{rx+rw,ry},
            IM_COL32(26,26,38,(int)(f*255)),1.f);
        ry+=14.f;

        /* ── Fields ─────────────────────────────────────────── */
        constexpr float FH=34.f, FLblSz=0.74f;
        float uLblY=ry;
        float uFldY=ry+lh2*FLblSz+5.f;
        float pLblY=uFldY+FH+14.f;
        float pFldY=pLblY+lh2*FLblSz+5.f;
        float errY =pFldY+FH+8.f;
        float btnY =errY+lh2+10.f;
        constexpr float BH=36.f;

        /* Field labels */
        ImU32 lblC=IM_COL32(90,90,108,(int)(f*255));
        dl->AddText(ImGui::GetFont(),ImGui::GetFontSize()*FLblSz,{rx,uLblY},lblC,"USERNAME");
        dl->AddText(ImGui::GetFont(),ImGui::GetFontSize()*FLblSz,{rx,pLblY},lblC,"PASSWORD");

        /* Field backgrounds — match kSurface exactly */
        ImU32 fBg  = IM_COL32(18,18,24,(int)(f*255));
        ImU32 fBdr = IM_COL32(38,38,54,(int)(f*180));
        dl->AddRectFilled({rx,uFldY},{rx+rw,uFldY+FH},fBg,5.f);
        dl->AddRect({rx,uFldY},{rx+rw,uFldY+FH},fBdr,5.f,0,1.f);
        dl->AddRectFilled({rx,pFldY},{rx+rw,pFldY+FH},fBg,5.f);
        dl->AddRect({rx,pFldY},{rx+rw,pFldY+FH},fBdr,5.f,0,1.f);

        /* Active field accent border */
        if(s_userFocus)
            dl->AddRect({rx,uFldY},{rx+rw,uFldY+FH},AC((int)(f*180)),5.f,0,1.2f);
        if(s_passFocus)
            dl->AddRect({rx,pFldY},{rx+rw,pFldY+FH},AC((int)(f*180)),5.f,0,1.2f);

        /* Error border */
        if(s_badCreds&&s_errTimer>0.f){
            float ef=s_errTimer/2.2f;
            ImU32 ec=IM_COL32(190,45,45,(int)(ef*f*160));
            dl->AddRect({rx,uFldY},{rx+rw,uFldY+FH},ec,5.f,0,1.2f);
            dl->AddRect({rx,pFldY},{rx+rw,pFldY+FH},ec,5.f,0,1.2f);
        }

        /* Error message */
        if(s_badCreds&&s_errTimer>0.f){
            float ef=s_errTimer/2.2f;
            dl->AddText({rx,errY},
                IM_COL32(200,60,60,(int)(ef*f*230)),
                "Incorrect username or password.");
        }

        /* Login button — uses accent fill, matches action buttons in menu */
        {
            ImVec2 mp=ImGui::GetIO().MousePos;
            bool hov=(mp.x>=rx&&mp.x<=rx+rw&&mp.y>=btnY&&mp.y<=btnY+BH);

            ImU32 btnFill=IM_COL32(
                (int)(s_aR*255*0.35f),(int)(s_aG*255*0.35f),(int)(s_aB*255*0.55f),
                (int)(f*(hov?220:170)));
            dl->AddRectFilled({rx,btnY},{rx+rw,btnY+BH},btnFill,5.f);
            dl->AddRect({rx,btnY},{rx+rw,btnY+BH},
                AC((int)(f*(hov?140:80))),5.f,0,1.f);

            const char* blt="Sign in";
            float tx2=rx+rw*.5f-ImGui::CalcTextSize(blt).x*.5f;
            float ty2=btnY+(BH-lh2)*.5f;
            dl->AddText({tx2,ty2},
                IM_COL32(228,228,236,(int)(f*(hov?255:220))),blt);
        }

        /* ── ImGui input widgets ────────────────────────────── */
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  5.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,   {10.f,8.f});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,           {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,    {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,     {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_Text,
            ImVec4(175/255.f,175/255.f,188/255.f,f));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,
            ImVec4(s_aR*.4f,s_aG*.4f,s_aB*.7f,0.55f));
        ImGui::PushStyleColor(ImGuiCol_Border,{0,0,0,0});

        /* Username */
        ImGui::SetCursorScreenPos({rx,uFldY});
        ImGui::SetNextItemWidth(rw);
        if(ImGui::InputText("##ldusr",s_user,sizeof(s_user),
            ImGuiInputTextFlags_AutoSelectAll))
            s_badCreds=false;
        s_userFocus=ImGui::IsItemActive();
        if(!s_user[0]&&!s_userFocus)
            dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
                {rx+10.f,uFldY+8.f},IM_COL32(52,52,66,(int)(f*255)),
                "enter username");

        /* Password */
        ImGui::SetCursorScreenPos({rx,pFldY});
        ImGui::SetNextItemWidth(rw);
        if(ImGui::InputText("##ldpwd",s_pass,sizeof(s_pass),
            ImGuiInputTextFlags_Password|ImGuiInputTextFlags_AutoSelectAll))
            s_badCreds=false;
        s_passFocus=ImGui::IsItemActive();
        if(!s_pass[0]&&!s_passFocus)
            dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
                {rx+10.f,pFldY+8.f},IM_COL32(52,52,66,(int)(f*255)),
                "enter password");

        /* Login button hit area */
        ImGui::SetCursorScreenPos({rx,btnY});
        auto TryLogin=[&](){
            if(strcmp(s_user,"admin")==0&&strcmp(s_pass,"admin")==0){
                s_state=State::Loading; s_progress=0.f;
            } else {
                s_badCreds=true; s_errTimer=2.2f;
                memset(s_pass,0,sizeof(s_pass));
            }
        };
        if(ImGui::InvisibleButton("##ldbtn",{rw,BH})) TryLogin();
        if(ImGui::IsKeyPressed(ImGuiKey_Enter)||
           ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) TryLogin();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(6);
    }
    else /* Loading state */
    {
        /* ── Header ─────────────────────────────────────────── */
        dl->AddText({rx,ry},
            IM_COL32(228,228,236,(int)(f*230)),"Authenticating");
        ry+=lh2+4.f;

        int dots=((int)(s_anim*2.5f))%4;
        char sub[24]; snprintf(sub,sizeof(sub),"Please wait%.*s",dots,"...");
        dl->AddText(ImGui::GetFont(),ImGui::GetFontSize()*.80f,
            {rx,ry},IM_COL32(90,90,108,(int)(f*200)),sub);
        ry+=lh2*.85f+16.f;

        /* Thin separator */
        dl->AddLine({rx,ry},{rx+rw,ry},kSep,1.f);
        ry+=12.f;

        /* ── Step checklist ─────────────────────────────────── */
        int stepIdx=(int)(s_progress*(kStepCount-1));
        if(stepIdx<0)stepIdx=0; if(stepIdx>=kStepCount)stepIdx=kStepCount-1;
        constexpr float icR=5.f;

        for(int i=0;i<kStepCount;i++){
            bool done=(i<stepIdx), curr=(i==stepIdx);
            float sy=ry+i*(lh2+9.f);
            float icx=rx+icR, icy=sy+lh2*.5f;

            /* Connector line */
            if(i>0){
                float prevCy=ry+(i-1)*(lh2+9.f)+lh2*.5f;
                dl->AddLine({icx,prevCy+icR+1.f},{icx,icy-icR-1.f},
                    done?AC((int)(f*90)):IM_COL32(38,38,54,(int)(f*255)),1.f);
            }

            if(done){
                /* Filled accent circle + checkmark */
                dl->AddCircleFilled({icx,icy},icR,AC((int)(f*190)),14);
                float s2=icR*.42f;
                dl->AddLine({icx-s2,icy},{icx-s2*.2f,icy+s2*.7f},
                    IM_COL32(228,228,236,(int)(f*230)),1.5f);
                dl->AddLine({icx-s2*.2f,icy+s2*.7f},{icx+s2*.8f,icy-s2*.6f},
                    IM_COL32(228,228,236,(int)(f*230)),1.5f);
            } else if(curr){
                /* Spinning arc */
                dl->AddCircle({icx,icy},icR,kBorderDim,18,1.f);
                float ta=s_anim*5.f;
                for(int j=0;j<8;j++){
                    float a0=ta+j*(1.9f/8.f), a1=ta+(j+1)*(1.9f/8.f);
                    dl->AddLine({icx+cosf(a0)*icR,icy+sinf(a0)*icR},
                                {icx+cosf(a1)*icR,icy+sinf(a1)*icR},
                                AC((int)((float)(j+1)/8.f*f*220)),2.f);
                }
            } else {
                /* Empty dim circle */
                dl->AddCircle({icx,icy},icR,kBorderDim,14,1.f);
            }

            /* Step label */
            ImU32 tc = done  ? AC((int)(f*160))
                     : curr  ? kTxtHi
                     :          kTxtDim;
            float fs2=curr?ImGui::GetFontSize():ImGui::GetFontSize()*.88f;
            dl->AddText(ImGui::GetFont(),fs2,{rx+icR*2.f+10.f,sy},tc,kSteps[i]);
        }

        /* ── Progress bar ───────────────────────────────────── */
        {
            float pbY=cy+CH-BAR-28.f, pbX=rx, pbW=rw;
            /* Percentage label right-aligned above bar */
            char pct[10]; snprintf(pct,sizeof(pct),"%.0f%%",s_progress*100.f);
            float pctW=ImGui::CalcTextSize(pct).x*(.76f);
            dl->AddText(ImGui::GetFont(),ImGui::GetFontSize()*.76f,
                {pbX+pbW-pctW,pbY-lh2-.5f},AC((int)(f*150)),pct);

            /* Track */
            dl->AddRectFilled({pbX,pbY},{pbX+pbW,pbY+3.f},
                IM_COL32(22,22,30,255),1.5f);
            /* Fill with gradient */
            float fw=pbW*s_progress;
            if(fw>2.f){
                dl->AddRectFilledMultiColor(
                    {pbX,pbY},{pbX+fw,pbY+3.f},
                    AC((int)(f*100)),AC((int)(f*240)),
                    AC((int)(f*240)),AC((int)(f*100)));
                /* Knob */
                dl->AddCircleFilled({pbX+fw,pbY+1.5f},5.f,
                    IM_COL32(228,228,236,(int)(f*220)),12);
                dl->AddCircle({pbX+fw,pbY+1.5f},5.f,
                    AC((int)(f*80)),12,1.f);
            }
        }
    }

    ImGui::End();
}
