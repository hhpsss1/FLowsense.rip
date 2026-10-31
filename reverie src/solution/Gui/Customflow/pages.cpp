// Customflow-Remake - made by kdt255 (https://github.com/kdt255/Customflow-Remake)
// Free software: MIT + Commons Clause, selling is not permitted. If you paid for this, you got scammed.
// Page layouts. Each page fills the category card and lays panels into two columns; everything else is widgets.
#include "draw.h"
#include "menu.h"
#include "preview.h"
#include "ui.h"
#include "../Settings/Functions.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace customflow {

namespace {

using ui::PanelHeight;

struct ItemList {
    const char* const* items;
    int count;
};

#define CUSTOMFLOW_LIST(name, ...)                                    \
    constexpr const char* name##_items[] = { __VA_ARGS__ };       \
    constexpr ItemList name{ name##_items, IM_ARRAYSIZE(name##_items) }

CUSTOMFLOW_LIST(kGroups, "Pistol", "Heavy pistol", "SMG", "Rifle", "Shotgun", "Sniper", "Machine gun");
CUSTOMFLOW_LIST(kRageCategories, "General", "Pistol", "Heavy pistol", "SMG", "Rifle", "Shotgun", "Sniper", "Machine gun");
CUSTOMFLOW_LIST(kPistols, "Glock-18", "USP-S", "P2000", "P250", "Five-SeveN", "Tec-9", "CZ75-Auto", "Dual Berettas");
CUSTOMFLOW_LIST(kHeavyPistols, "Desert Eagle", "R8 Revolver");
CUSTOMFLOW_LIST(kSmgs, "MAC-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon");
CUSTOMFLOW_LIST(kRifles, "AK-47", "M4A4", "M4A1-S", "Galil AR", "FAMAS", "AUG", "SG 553");
CUSTOMFLOW_LIST(kShotguns, "Nova", "XM1014", "Sawed-Off", "MAG-7");
CUSTOMFLOW_LIST(kSnipers, "SSG 08", "AWP", "G3SG1", "SCAR-20");
CUSTOMFLOW_LIST(kMachineGuns, "M249", "Negev");
constexpr ItemList kWeapons[kGroupCount] = { kPistols, kHeavyPistols, kSmgs, kRifles, kShotguns, kSnipers, kMachineGuns };

CUSTOMFLOW_LIST(kHitboxes, "Head", "Body", "Arms", "Legs", "Feet");
CUSTOMFLOW_LIST(kIgnoreIf, "Flashed", "In smoke", "High speed", "In air", "Jumping");
CUSTOMFLOW_LIST(kAssistModes, "Passive", "Aggressive");
CUSTOMFLOW_LIST(kFovStyles, "2D", "3D", "Dynamic");
CUSTOMFLOW_LIST(kSpreadModes, "Compensate", "Ignore");
CUSTOMFLOW_LIST(kSmoothCurves, "Linear", "Ease out", "Bezier");
CUSTOMFLOW_LIST(kPitch, "Default", "Up", "Down", "Fake up", "Fake fake up", "Custom");
CUSTOMFLOW_LIST(kYawBase, "Local view", "At targets");
CUSTOMFLOW_LIST(kRemoveSpread, "Full", "Partial");
CUSTOMFLOW_LIST(kDoubleTap, "Offensive", "Defensive");
CUSTOMFLOW_LIST(kQuickStop, "Early", "Between shots", "Force accuracy");
CUSTOMFLOW_LIST(kAutoScope, "Always", "On hit chance fail", "Before shot");

CUSTOMFLOW_LIST(kVisualTargets, "Enemy", "Team", "Local", "Other");
CUSTOMFLOW_LIST(kWeaponEsp, "Name", "Icon", "Ammo");
CUSTOMFLOW_LIST(kFlags, "Armor type", "C4 carrier", "Location", "Ping", "Reload", "Grenade pin", "Scope", "Flashed", "Defusing");
CUSTOMFLOW_LIST(kAnimations, "Smoothing", "Death fade");
CUSTOMFLOW_LIST(kMaterials, "Flat", "Shaded", "Glass", "Metallic");
CUSTOMFLOW_LIST(kSkyboxes, "Night", "Vertigo", "Cloudy", "Sunset", "Space");
CUSTOMFLOW_LIST(kWeather, "Rain", "Snow", "Ash");

CUSTOMFLOW_LIST(kPaintKits, "Default", "Asiimov", "Redline", "Vulcan", "Fire Serpent", "Hyper Beast", "Neo-Noir", "Printstream",
            "The Empress", "Bloodsport", "Slate");
CUSTOMFLOW_LIST(kKnives, "Default", "Karambit", "Butterfly", "M9 Bayonet", "Talon", "Skeleton", "Stiletto", "Ursus");
CUSTOMFLOW_LIST(kKnifeFinishes, "Vanilla", "Fade", "Doppler", "Tiger Tooth", "Marble Fade", "Slaughter", "Crimson Web", "Case Hardened");
CUSTOMFLOW_LIST(kGloves, "Default", "Sport", "Driver", "Specialist", "Moto", "Hand wraps");
CUSTOMFLOW_LIST(kGloveFinishes, "Vice", "Pandora's Box", "Crimson Kimono", "Emerald Web", "Fade");
CUSTOMFLOW_LIST(kAgentsT, "Default", "The Elite Mr. Muhlik", "Sir Bloody Darryl", "Number K", "Dragomir");
CUSTOMFLOW_LIST(kAgentsCt, "Default", "Special Agent Ava", "Lt. Commander Ricksaw", "Cmdr. Mae Jamison", "B Squadron Officer");

CUSTOMFLOW_LIST(kMiscPages, "Miscellaneous", "Automation", "Interface");
CUSTOMFLOW_LIST(kWalkSpeed, "Accurate", "Fast", "Silent");
CUSTOMFLOW_LIST(kRemove, "Post processing", "Flashbang overlay", "Smoke", "Scope overlay", "Shadows", "Fog");
CUSTOMFLOW_LIST(kReveal, "Sniper crosshair", "Enemy scoreboard", "Money", "Overwatch");
CUSTOMFLOW_LIST(kAntiCheat, "Valve MM", "Community", "Disabled");
CUSTOMFLOW_LIST(kRoundAlert, "Disabled", "Sound", "Flash taskbar");
CUSTOMFLOW_LIST(kPrimaries, "None", "AWP", "SSG 08", "AK-47 / M4", "Auto sniper");
CUSTOMFLOW_LIST(kSecondaries, "None", "Desert Eagle", "Dual Berettas", "P250");
CUSTOMFLOW_LIST(kUtility, "Armor", "Helmet", "Defuser", "HE grenade", "Smoke", "Molotov", "Flashbang");
CUSTOMFLOW_LIST(kAccents, "Salmon", "Crimson", "Sunset", "Custom");
CUSTOMFLOW_LIST(kScales, "Auto", "100%", "125%", "150%", "175%", "200%");
CUSTOMFLOW_LIST(kHitSounds, "None", "Bell", "Click", "Bubble");
CUSTOMFLOW_LIST(kConfigSort, "Recently modified", "Name");
CUSTOMFLOW_LIST(kScriptFilter, "All scripts", "Autoload only");

#undef CUSTOMFLOW_LIST

[[nodiscard]] float S(float v) { return draw::Snap(ui::Px(v)); }

bool Combo(const char* label, int* v, const ItemList& list) { return ui::Combo(label, v, list.items, list.count); }
bool Multi(const char* label, unsigned* v, const ItemList& list) { return ui::MultiCombo(label, v, list.items, list.count); }

ImRect CardFrame(const ImRect& card)
{
    const float inset = S(ui::Ctx().m.card_inset);
    return ImRect(ImVec2(card.Min.x + inset, card.Min.y + inset), ImVec2(card.Max.x - inset, card.Max.y - inset));
}

// Two combos in one card, split on the column boundary below.
void CardSplit(const ImRect& card, const ImRect& area, ImRect& left, ImRect& right)
{
    const ui::Context& c = ui::Ctx();
    const float inset = S(c.m.card_inset);
    const float gap = S(c.m.gap);
    const float col_w = draw::Snap((area.GetWidth() - gap) * 0.5f);
    const float mid = area.Min.x + col_w + gap * 0.5f;
    left = ImRect(ImVec2(card.Min.x + inset, card.Min.y + inset), ImVec2(draw::Snap(mid - S(6.0f)), card.Max.y - inset));
    right = ImRect(ImVec2(draw::Snap(mid + S(6.0f)), card.Min.y + inset), ImVec2(card.Max.x - inset, card.Max.y - inset));
}

int FmtAimTime(char* buf, int size, float v)
{
    return v >= 1999.5f ? ImFormatString(buf, static_cast<size_t>(size), "Unlimited")
                        : ImFormatString(buf, static_cast<size_t>(size), "%.0fms", static_cast<double>(v));
}

int FmtMinDamage(char* buf, int size, float v)
{
    const int d = static_cast<int>(std::lround(v));
    if (d <= 0) return ImFormatString(buf, static_cast<size_t>(size), "Auto");
    if (d > 100) return ImFormatString(buf, static_cast<size_t>(size), "hp + %d", d - 100);
    return ImFormatString(buf, static_cast<size_t>(size), "%dhp", d);
}

int FmtYaw(char* buf, int size, float v)
{
    const int d = static_cast<int>(std::lround(v));
    if (d == 180 || d == -180) return ImFormatString(buf, static_cast<size_t>(size), "Backward");
    if (d == 0) return ImFormatString(buf, static_cast<size_t>(size), "Forward");
    return ImFormatString(buf, static_cast<size_t>(size), "%d\xC2\xB0", d);
}

int FmtSeed(char* buf, int size, float v) { return ImFormatString(buf, static_cast<size_t>(size), "%d", static_cast<int>(std::lround(v))); }

void StatusLine(const char* text, bool error, float alpha)
{
    if (text == nullptr || text[0] == '\0' || alpha <= 0.01f) return;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ui::Context& c = ui::Ctx();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(window->WorkRect.Max.x, pos.y + S(c.m.row_h)));
    ImGui::ItemSize(bb, 0.0f);
    if (!ImGui::ItemAdd(bb, 0)) return;
    draw::TextCapCentered(window->DrawList, c.fonts.regular, ui::FontPx(c.m.font_body), pos.x, bb.GetCenter().y,
                          draw::Col(error ? c.pal.accent : c.pal.text_soft, alpha), text);
}

} // namespace

// ---- Legit -----------------------------------------------------------------------------------------------------

void Menu::PageLegit(const ImRect& card, const ImRect& area)
{
    ImGui::SetCursorScreenPos(area.Min);
    ImGui::TextUnformatted("Legit tab");
}

void Menu::PageRage(const ImRect& card, const ImRect& area)
{
    ImGui::SetCursorScreenPos(area.Min);
    if (::Gui) ::Gui->RenderRagebotContent();
}

// ---- Visuals ---------------------------------------------------------------------------------------------------

void Menu::PageVisuals(const ImRect& card, const ImRect& area)
{
    ImGui::SetCursorScreenPos(area.Min);
    if (::Gui) ::Gui->RenderVisualsContent();
}

void Menu::PageSkins(const ImRect& card, const ImRect& area)
{
    ImGui::SetCursorScreenPos(area.Min);
    if (::Gui) ::Gui->RenderSkinsContent();
}

void Menu::PageMisc(const ImRect& card, const ImRect& area)
{
    ImGui::SetCursorScreenPos(area.Min);
    if (::Gui) ::Gui->RenderMiscContent();
}

void Menu::RefreshConfigs()
{
    configs_ = persist::ListConfigs(config_sort_ == 0 ? persist::SortOrder::Modified : persist::SortOrder::Name);
    if (config_selected_ >= static_cast<int>(configs_.size())) config_selected_ = static_cast<int>(configs_.size()) - 1;
    configs_loaded_ = true;
}

void Menu::PageConfigs(const ImRect& card, const ImRect& area)
{
    const ui::Context& c = ui::Ctx();
    if (!configs_loaded_) RefreshConfigs();
    if (ui::CardCombo("##sort", &config_sort_, kConfigSort.items, kConfigSort.count, CardFrame(card))) RefreshConfigs();

    auto report = [this](const std::string& text, bool error) {
        config_status_ = text;
        config_status_error_ = error;
        config_status_time_ = ui::Ctx().time;
    };
    auto selected_name = [this]() -> std::string {
        return config_selected_ >= 0 && config_selected_ < static_cast<int>(configs_.size()) ? configs_[config_selected_].name : std::string();
    };

    ui::BeginColumns("##configs", area, 2);
    ui::BeginPanel("Configs", PanelHeight::Fill);
    if (configs_.empty()) ui::Caption("No configs yet");
    for (int i = 0; i < static_cast<int>(configs_.size()); ++i) {
        ImGui::PushID(i);
        if (ui::ListRow(configs_[i].name.c_str(), configs_[i].modified.c_str(), i == config_selected_)) {
            config_selected_ = i;
            delete_armed_ = false;
            ImFormatString(config_name_, sizeof(config_name_), "%s", configs_[i].name.c_str());
        }
        ImGui::PopID();
    }
    ui::EndPanel();

    ui::NextColumn();
    ui::BeginPanel("Actions", PanelHeight::Fill);
    ui::Caption("Name");
    ui::InputText("##config_name", "Config name", config_name_, sizeof(config_name_));

    std::string error;
    if (ui::Button("Create", 0.0f, true)) {
        const std::string name = config_name_;
        if (persist::SaveConfig(cfg_, name, error)) {
            report("Created " + name, false);
            RefreshConfigs();
            for (int i = 0; i < static_cast<int>(configs_.size()); ++i)
                if (configs_[i].name == name) config_selected_ = i;
        } else {
            report(error, true);
        }
    }
    if (ui::Button("Save")) {
        const std::string name = selected_name();
        if (name.empty()) report("Select a config first", true);
        else if (persist::SaveConfig(cfg_, name, error)) report("Saved " + name, false), RefreshConfigs();
        else report(error, true);
    }
    if (ui::Button("Load")) {
        const std::string name = selected_name();
        if (name.empty()) report("Select a config first", true);
        else if (persist::LoadConfig(cfg_, name, error)) report("Loaded " + name, false);
        else report(error, true);
    }
    if (ui::Button(delete_armed_ ? "Click again to delete##delete" : "Delete##delete")) {
        const std::string name = selected_name();
        if (name.empty()) {
            report("Select a config first", true);
        } else if (!delete_armed_) {
            delete_armed_ = true;
        } else {
            delete_armed_ = false;
            if (persist::DeleteConfig(name, error)) report("Deleted " + name, false), RefreshConfigs();
            else report(error, true);
        }
    }
    ui::Separator();
    if (ui::Button("Export to clipboard")) {
        ImGui::SetClipboardText(persist::ExportConfig(cfg_).c_str());
        report("Config copied to clipboard", false);
    }
    if (ui::Button("Import from clipboard")) {
        if (persist::ImportConfig(cfg_, ImGui::GetClipboardText(), error)) report("Imported from clipboard", false);
        else report(error, true);
    }
    if (ui::Button("Open folder")) persist::OpenFolder(false);

    const float age = static_cast<float>(c.time - config_status_time_);
    StatusLine(config_status_.c_str(), config_status_error_, ImSaturate((5.0f - age) / 0.6f));
    ui::EndPanel();
    ui::EndColumns();
}

// ---- Scripts ---------------------------------------------------------------------------------------------------

void Menu::RefreshScripts()
{
    scripts_ = persist::ListScripts();
    autoload_ = persist::LoadAutoload();
    if (script_selected_ >= static_cast<int>(scripts_.size())) script_selected_ = -1;
    preview_.clear();
    if (script_selected_ >= 0) preview_ = persist::ReadScriptPreview(scripts_[script_selected_].name, 40);
    scripts_loaded_ = true;
}

void Menu::PageScripts(const ImRect& card, const ImRect& area)
{
    const ui::Context& c = ui::Ctx();
    if (!scripts_loaded_) RefreshScripts();
    ui::CardCombo("##filter", &script_filter_, kScriptFilter.items, kScriptFilter.count, CardFrame(card));

    auto is_autoload = [this](const std::string& name) { return std::find(autoload_.begin(), autoload_.end(), name) != autoload_.end(); };

    ui::BeginColumns("##scripts", area, 2);
    ui::BeginPanel("Scripts", PanelHeight::Fill);
    int shown = 0;
    for (int i = 0; i < static_cast<int>(scripts_.size()); ++i) {
        const bool autoload = is_autoload(scripts_[i].name);
        if (script_filter_ == 1 && !autoload) continue;
        ++shown;
        char detail[48];
        if (autoload) ImFormatString(detail, sizeof(detail), "autoload");
        else ImFormatString(detail, sizeof(detail), "%.1f kb", static_cast<double>(scripts_[i].size) / 1024.0);
        ImGui::PushID(i);
        if (ui::ListRow(scripts_[i].name.c_str(), detail, i == script_selected_)) {
            script_selected_ = i;
            preview_ = persist::ReadScriptPreview(scripts_[i].name, 40);
        }
        ImGui::PopID();
    }
    if (shown == 0) ui::Caption(scripts_.empty() ? "Put .lua files into the scripts folder" : "No autoload scripts");
    ui::EndPanel();

    ui::NextColumn();
    ui::BeginPanel("Script", PanelHeight::Fill);
    if (script_selected_ >= 0 && script_selected_ < static_cast<int>(scripts_.size())) {
        const persist::FileEntry& s = scripts_[script_selected_];
        bool autoload = is_autoload(s.name);
        if (ui::Checkbox("Load on startup", &autoload)) {
            if (autoload) autoload_.push_back(s.name);
            else autoload_.erase(std::remove(autoload_.begin(), autoload_.end(), s.name), autoload_.end());
            std::string error;
            persist::SaveAutoload(autoload_, error);
        }
        ui::Checkbox("Reload on change", &cfg_.scripts.reload_on_change);

        // Read-only source preview
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const float fs = ui::FontPx(c.m.font_small + 1.0f);
        const float line_h = draw::Snap(fs * 1.2f);
        const int lines = static_cast<int>(preview_.size());
        const ImVec2 pos = window->DC.CursorPos;
        const float pad = S(8.0f);
        const ImRect block(pos, ImVec2(window->WorkRect.Max.x, pos.y + pad * 2.0f + line_h * static_cast<float>(ImMax(lines, 1))));
        ImGui::ItemSize(block, 0.0f);
        if (ImGui::ItemAdd(block, 0)) {
            ImDrawList* dl = window->DrawList;
            dl->AddRectFilled(block.Min, block.Max, draw::Col(c.pal.frame), ui::Px(c.m.frame_rounding));
            dl->PushClipRect(block.Min, block.Max, true);
            if (lines == 0)
                draw::Text(dl, c.fonts.regular, fs, ImVec2(block.Min.x + pad, block.Min.y + pad), draw::Col(c.pal.text_dim), "(empty file)");
            for (int i = 0; i < lines; ++i) {
                const float y = block.Min.y + pad + line_h * static_cast<float>(i);
                char number[8];
                ImFormatString(number, sizeof(number), "%2d", i + 1);
                draw::Text(dl, c.fonts.regular, fs, ImVec2(block.Min.x + pad, y), draw::Col(c.pal.grip), number);
                draw::Text(dl, c.fonts.regular, fs, ImVec2(block.Min.x + pad + S(22.0f), y), draw::Col(c.pal.text_soft), preview_[i].c_str());
            }
            dl->PopClipRect();
        }
    } else {
        ui::Caption("Select a script to inspect it");
    }
    if (ui::Button("Refresh")) RefreshScripts();
    if (ui::Button("Open folder")) persist::OpenFolder(true);
    ui::EndPanel();
    ui::EndColumns();
}

} // namespace customflow
