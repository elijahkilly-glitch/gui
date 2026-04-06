/*
 * esp_displayname_patch.h
 * ────────────────────────────────────────────────────────────────
 * APPLY THIS PATCH to esp_theme_normal.h and esp_theme_cat.h
 *
 * In EVERY place you render a player name, replace:
 *
 *     const char* nameStr = p.name.c_str();
 *
 * with:
 *
 *     const char* nameStr = (g_cfg.esp.showDisplayName && !p.displayName.empty())
 *                         ? p.displayName.c_str()
 *                         : p.name.c_str();
 *
 * That's the entire fix. The rest is already wired up:
 *   - PlayerData::displayName is now populated in roblox.cpp
 *   - ESPSettings::showDisplayName toggle already exists in settings.h
 *   - The menu checkbox for "Display Name" already exists in all themes
 *
 * EXAMPLE — typical name rendering block before patch:
 *
 *     if (e.showNames) {
 *         std::string label = p.name;                     // ← OLD
 *         ImVec2 ts = ImGui::CalcTextSize(label.c_str());
 *         dl->AddText({cx - ts.x*.5f, headY - ts.y - 4}, nameCol, label.c_str());
 *     }
 *
 * EXAMPLE — after patch:
 *
 *     if (e.showNames) {
 *         const char* nameStr = (e.showDisplayName && !p.displayName.empty())
 *                             ? p.displayName.c_str()
 *                             : p.name.c_str();           // ← NEW
 *         ImVec2 ts = ImGui::CalcTextSize(nameStr);
 *         dl->AddText({cx - ts.x*.5f, headY - ts.y - 4}, nameCol, nameStr);
 *     }
 *
 * ────────────────────────────────────────────────────────────────
 * If your ESP renders names using a helper like this:
 *
 *     void DrawName(const PlayerData& p, ...) {
 *         dl->AddText(..., p.name.c_str());
 *     }
 *
 * Change to:
 *
 *     void DrawName(const PlayerData& p, ...) {
 *         const ESPSettings& e = g_cfg.esp;
 *         const char* n = (e.showDisplayName && !p.displayName.empty())
 *                       ? p.displayName.c_str() : p.name.c_str();
 *         dl->AddText(..., n);
 *     }
 * ────────────────────────────────────────────────────────────────
 */

/* Convenience macro you can use anywhere in ESP rendering: */
#define ESP_NAME(player) \
    ((g_cfg.esp.showDisplayName && !(player).displayName.empty()) \
     ? (player).displayName.c_str() \
     : (player).name.c_str())

/*
 * Usage:
 *     dl->AddText(pos, col, ESP_NAME(p));
 *     ImVec2 ts = ImGui::CalcTextSize(ESP_NAME(p));
 */
