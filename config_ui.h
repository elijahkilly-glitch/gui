#pragma once
/*
 * config_ui.h  —  x9 Config Manager UI
 *
 * Include this header ONCE inside any menu theme that wants a
 * config tab.  Call  ConfigUI::Draw()  from the tab body.
 *
 * Features:
 *  • List all saved configs with load / delete buttons
 *  • Save-new: type a name, press Save
 *  • Overwrite: select existing, press Overwrite
 *  • Status toast (green/red, fades after 2.5s)
 *  • "Reset to defaults" resets g_cfg + Aimbot::settings in-place
 *  • Auto-saves to  <exe_dir>/configs/<name>.json
 */

#include "config.h"
#include "imgui.h"
#include <cstring>
#include <cstdio>

namespace ConfigUI {

/* ── Internal state ─────────────────────────────────────────── */
static char  s_saveName[64]  = {};
static int   s_selected      = -1;   /* index into listed configs  */
static std::string s_status  = "";
static float s_statusTimer   = 0.f;
static bool  s_statusOk      = true;
static std::vector<std::string> s_configs;

static void Refresh() {
    s_configs = ConfigIO::ListConfigs();
    if (s_selected >= (int)s_configs.size())
        s_selected = (int)s_configs.size() - 1;
}

static void SetStatus(const std::string& msg, bool ok) {
    s_status      = msg;
    s_statusTimer = 2.5f;
    s_statusOk    = ok;
}

/* ── Draw ─────────────────────────────────────────────────────── */
static void Draw(float dt = 0.016f) {
    /* Tick status timer */
    if (s_statusTimer > 0.f) s_statusTimer -= dt;

    /* Refresh list if empty (first frame) */
    if (s_configs.empty()) Refresh();

    float W = ImGui::GetContentRegionAvail().x;

    /* ── Header ─────────────────────────────────────────────── */
    ImGui::TextDisabled("Configs are saved as JSON in:");
    ImGui::SameLine();
    {
        char dir[MAX_PATH]={};
        GetModuleFileNameA(NULL,dir,MAX_PATH);
        /* trim to exe dir */
        for(int i=(int)strlen(dir)-1;i>=0;i--)
            if(dir[i]=='\\'||dir[i]=='/'){dir[i]='\0';break;}
        char label[MAX_PATH+16];
        snprintf(label,sizeof(label),"%s\\configs\\",dir);
        if(ImGui::SmallButton("Open Folder")) {
            char cmd[MAX_PATH+32];
            snprintf(cmd,sizeof(cmd),"explorer \"%s\\configs\"",dir);
            system(cmd);
        }
    }
    ImGui::Separator();
    ImGui::Spacing();

    /* ── Saved configs list ──────────────────────────────────── */
    ImGui::Text("Saved configs  (%d)", (int)s_configs.size());
    ImGui::SameLine(W - 60.f);
    if (ImGui::SmallButton("Refresh##cfgref")) Refresh();

    float listH = ImGui::GetTextLineHeightWithSpacing() * 7.f + 6.f;
    ImGui::BeginChild("##cfglist", ImVec2(W, listH), true);
    for (int i = 0; i < (int)s_configs.size(); i++) {
        bool sel = (s_selected == i);
        char label[96];
        snprintf(label, sizeof(label), " %s  %s",
            sel ? ">" : " ", s_configs[i].c_str());
        if (ImGui::Selectable(label, sel)) {
            s_selected = i;
            /* auto-fill save name with selected */
            strncpy_s(s_saveName, sizeof(s_saveName),
                s_configs[i].c_str(), sizeof(s_saveName)-1);
        }
    }
    if (s_configs.empty())
        ImGui::TextDisabled("  No configs yet");
    ImGui::EndChild();

    ImGui::Spacing();

    /* ── Action buttons for selected config ─────────────────── */
    bool hasSelection = (s_selected >= 0 && s_selected < (int)s_configs.size());

    /* Load */
    if (!hasSelection) ImGui::BeginDisabled();
    if (ImGui::Button("Load##cfgload", ImVec2((W-8.f)*.5f, 0))) {
        std::string err = ConfigIO::Load(s_configs[s_selected]);
        if (err.empty()) SetStatus("Loaded: " + s_configs[s_selected], true);
        else             SetStatus("Error: " + err, false);
    }
    if (!hasSelection) ImGui::EndDisabled();

    ImGui::SameLine();

    /* Delete */
    if (!hasSelection) ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(.55f,.12f,.12f,.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.75f,.18f,.18f,.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(.90f,.22f,.22f,1.f));
    if (ImGui::Button("Delete##cfgdel", ImVec2((W-8.f)*.5f, 0))) {
        std::string name = s_configs[s_selected];
        if (ConfigIO::Delete(name)) {
            SetStatus("Deleted: " + name, true);
            Refresh();
        } else {
            SetStatus("Delete failed", false);
        }
    }
    ImGui::PopStyleColor(3);
    if (!hasSelection) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /* ── Save / Overwrite ────────────────────────────────────── */
    ImGui::Text("Save current config as:");
    ImGui::SetNextItemWidth(W - 90.f);
    ImGui::InputText("##cfgname", s_saveName, sizeof(s_saveName));
    ImGui::SameLine();

    bool nameOk = s_saveName[0] != '\0';
    if (!nameOk) ImGui::BeginDisabled();
    if (ImGui::Button("Save##cfgsave", ImVec2(-1, 0))) {
        std::string err = ConfigIO::Save(s_saveName);
        if (err.empty()) {
            SetStatus("Saved: " + std::string(s_saveName), true);
            Refresh();
            /* select the newly saved entry */
            for (int i=0;i<(int)s_configs.size();i++) {
                std::string n = s_saveName;
                /* strip .json if typed */
                if(n.size()>5&&n.substr(n.size()-5)==".json") n=n.substr(0,n.size()-5);
                if(s_configs[i]==n){ s_selected=i; break; }
            }
        } else {
            SetStatus("Error: " + err, false);
        }
    }
    if (!nameOk) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /* ── Reset to defaults ───────────────────────────────────── */
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(.25f,.25f,.08f,.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.40f,.40f,.12f,.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(.55f,.55f,.15f,1.f));
    if (ImGui::Button("Reset to Defaults##cfgrst", ImVec2(W, 0))) {
        g_cfg             = Config{};
        Aimbot::settings  = Aimbot::Settings{};
        SetStatus("Reset to defaults", true);
    }
    ImGui::PopStyleColor(3);

    /* ── Status toast ────────────────────────────────────────── */
    if (s_statusTimer > 0.f && !s_status.empty()) {
        float alpha = s_statusTimer > .5f ? 1.f : s_statusTimer / .5f;
        ImGui::Spacing();
        float a8 = alpha * 255.f;
        ImU32 col = s_statusOk
            ? IM_COL32(80,220,100,(int)a8)
            : IM_COL32(220,80,80,(int)a8);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(
            s_statusOk?0.35f:0.9f,
            s_statusOk?0.9f :0.35f,
            0.35f, alpha));
        ImGui::TextWrapped("%s %s",
            s_statusOk ? "[OK]" : "[ERR]",
            s_status.c_str());
        ImGui::PopStyleColor();
        (void)col;
    }
}

} // namespace ConfigUI
