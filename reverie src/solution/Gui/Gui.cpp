#include <shared.h>
#include "Blur/Blur.h"

struct CSideAnim {
    float hover{ 0.f };
    float select{ 0.f };
    float expand{ 0.f };
};

static void DrawOutline(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, float alpha_mult = 1.f, ImDrawFlags flags = ImDrawFlags_None) {
    const int a = static_cast<int>(255.f * ImClamp(alpha_mult, 0.f, 1.f));
    const ImU32 col = (Var->window.outline_col & 0x00FFFFFF) | (static_cast<ImU32>(a) << 24);
    dl->AddRect(min, max, col, rounding, flags, 1.f);
}

CTab* FindTabByID(int id) {
    for (auto& cat : Var->gui.categories) {
        for (auto& tab : cat.tabs) {
            if (tab.id == id)
                return &tab;
        }
    }
    return nullptr;
}

int GetActiveSubtab(int id) {
    CTab* tab = FindTabByID(id);
    return tab ? tab->active_subtab : 0;
}

void CGui::Render() {
    this->Update();
    Binds::Update();

    this->about_draw_list = nullptr;
    this->about_extra_draw_lists.clear();
    this->about_extra_windows.clear();

    ImDrawList* about_draw_list = nullptr;
    std::vector<ImDrawList*> about_extra_draw_lists;

    static float content_anim = 0.f;
    static bool s_prev_opened = false;
    static bool s_closing = false;
    static bool s_init = false;
    static ImVec2 s_pos(0.f, 0.f);

    static bool s_about_open = false;
    static bool s_about_prev_open = false;
    static float s_about_anim = 0.f;
    static float s_about_content_h = 420.f;

    static int s_about_theme_idx = 0;
    static int s_about_scale_idx = 1;
    static bool s_widget_keybind_list = true;
    static bool s_widget_watermark = true;
    static bool s_widget_spectator_list = false;
    static bool s_widget_shot_logs = true;
    static bool s_watermark_username = true;
    static bool s_watermark_ping = true;
    static bool s_watermark_fps = true;
    static bool s_shot_logs_damage = true;
    static bool s_shot_logs_distance = true;
    static bool s_shot_logs_hitgroup = false;

    static char s_new_config_name[64] = "";

    const bool just_opened = !s_prev_opened && Draw->opened;
    const bool just_closed = s_prev_opened && !Draw->opened;
    s_prev_opened = Draw->opened;

    if (just_opened) {
        s_closing = false;
        this->menu_anim = 0.f;
        content_anim = 0.f;
        //g_presets->Refresh();
    }

    if (just_closed) {
        s_closing = true;
        content_anim = 1.f;
        s_about_open = false;
        Binds::CancelCapture();
    }

    if (!Draw->opened && !s_closing && this->menu_anim <= 0.001f) {
        ImGui::GetIO().FontGlobalScale = 1.f;
        this->ui_alpha = 0.f;
        this->about_active = false;
        this->about_ease = 0.f;
        return;
    }

    if (Draw->opened) {
        this->menu_anim = ImClamp(this->menu_anim + this->FixedSpeed(10.f), 0.f, 1.f);

        if (this->menu_anim > 0.14f)
            content_anim = ImClamp(content_anim + this->FixedSpeed(16.f), 0.f, 1.f);
        else
            content_anim = ImClamp(content_anim - this->FixedSpeed(20.f), 0.f, 1.f);
    } else if (s_closing) {
        content_anim = ImClamp(content_anim - this->FixedSpeed(22.f), 0.f, 1.f);

        if (content_anim <= 0.001f)
            this->menu_anim = ImClamp(this->menu_anim - this->FixedSpeed(10.f), 0.f, 1.f);

        if (this->menu_anim <= 0.001f && content_anim <= 0.001f)
            s_closing = false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 scaled_size(Var->window.size.x, Var->window.size.y);

    if (!s_init) {
        s_pos = ImVec2(vp->GetCenter().x - scaled_size.x * 0.5f, vp->GetCenter().y - scaled_size.y * 0.5f);
        s_init = true;
    }

    const float window_ease = this->menu_anim * this->menu_anim * (3.f - 2.f * this->menu_anim);
    const float content_gate = content_anim * content_anim * (3.f - 2.f * content_anim);

    s_about_anim = ImClamp(s_about_anim + (s_about_open ? 1.f : -1.f) * this->FixedSpeed(12.f), 0.f, 1.f);
    const float about_ease = s_about_anim * s_about_anim * (3.f - 2.f * s_about_anim);
    this->about_active = (s_about_open || s_about_anim > 0.001f);
    this->about_ease = about_ease;

    const bool about_just_opened = !s_about_prev_open && s_about_open;
    s_about_prev_open = s_about_open;

    const bool about_blocking = this->about_active;

    ImGui::SetNextWindowPos(s_pos, ImGuiCond_Always);
    this->SetNextWindowSize(scaled_size);
    ImGui::SetNextWindowBgAlpha(0.f);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, window_ease);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Var->window.rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, Var->window.border_size); 

    this->Begin(xs("menu"), nullptr, Var->window.flags);
    {
        ImGuiContext& g = *GImGui;
        ImGuiStyle* style = &g.Style;
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ConfigPopup::SetMenuRect(pos, size);
        s_pos = pos;
        this->menu_pos = pos;
        this->menu_size = size;
        this->menu_rounding = style->WindowRounding;
        this->ui_alpha = window_ease;

        style->WindowPadding = ImVec2(Var->window.padding.x, Var->window.padding.y);
        style->ItemSpacing = ImVec2(Var->window.spacing.x, Var->window.spacing.y);
        style->ScrollbarSize = 0.f;
        style->WindowBorderSize = Var->window.border_size;
        style->WindowRounding = Var->window.rounding;

        ImFont* icon_font = Var->font.fa[0];
        ImFont* font_logo = Var->font.inter[3];
        ImFont* font_regular = Var->font.inter[0];
        ImFont* font_bold = Var->font.inter[1] ? Var->font.inter[1] : font_regular;
        ImFont* font_sub = Var->font.inter[5] ? Var->font.inter[5] : font_regular;

        const float sidebar_w = Var->gui.sidebar_width;

        static bool dragging = false;
        static ImVec2 drag_offset(0.f, 0.f);
        const bool over_drag = io.MousePos.x >= pos.x && io.MousePos.x <= (pos.x + size.x - 120.f) && io.MousePos.y >= pos.y && io.MousePos.y <= (pos.y + 40.f);

        if (over_drag && ImGui::IsMouseClicked(0) && !about_blocking) {
            dragging = true;
            drag_offset = ImVec2(io.MousePos.x - s_pos.x, io.MousePos.y - s_pos.y);
        }

        if (dragging) {
            if (ImGui::IsMouseDown(0) && !about_blocking) {
                s_pos = ImVec2(io.MousePos.x - drag_offset.x, io.MousePos.y - drag_offset.y);
                s_pos.x = ImClamp(s_pos.x, 0.f, vp->Size.x - size.x);
                s_pos.y = ImClamp(s_pos.y, 0.f, vp->Size.y - size.y);
            }
            else {
                dragging = false;
            }
        }

        Blur::DrawBackgroundBlur(dl, Device, DeviceCtx, style->WindowRounding);

        dl->AddRectFilled(pos, pos + size, IM_COL32(13, 14, 14, static_cast<int>(248.f * window_ease)), style->WindowRounding);
        // sidebar removed - full width content
        DrawOutline(dl, pos, pos + size, style->WindowRounding, window_ease);

        if (font_logo) {
            const char* brand = xs("customflow");
            ImVec2 logo_sz = font_logo->CalcTextSizeA(font_logo->LegacySize, FLT_MAX, -1.f, brand);
            ImVec2 lp = ImVec2(pos.x + 20.f, pos.y + 16.f);
            dl->AddText(font_logo, font_logo->LegacySize, lp, Draw->GetClr(Colors->accent, window_ease), brand);
        }

        const float top_sep_y = pos.y + 52.f;

        ImGui::BeginDisabled(about_blocking);

        float top_tab_x = pos.x + 150.f;
        const float top_tab_y = pos.y + 16.f;
        const float top_tab_h = 28.f;

        for (auto& cat : Var->gui.categories) {
            for (auto& tab : cat.tabs) {
                const bool is_tab_sel = (Var->gui.active_tab_id == tab.id);
                ImVec2 tsz = font_bold ? font_bold->CalcTextSizeA(font_bold->LegacySize, FLT_MAX, -1.f, tab.label.c_str()) : ImVec2(0,0);
                const float btn_w = tsz.x + 24.f;

                ImVec2 t_min(top_tab_x, top_tab_y);
                ImVec2 t_max(top_tab_x + btn_w, top_tab_y + top_tab_h);

                ImGui::SetCursorScreenPos(t_min);
                ImGui::PushID(tab.id * 100);
                ImGui::InvisibleButton(xs("##top_main_tab"), ImVec2(btn_w, top_tab_h));
                const bool t_hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked())
                    Var->gui.active_tab_id = tab.id;
                ImGui::PopID();

                ImColor text_c = is_tab_sel ? ImColor(Colors->text.Value) : ImColor(Colors->text_secondary.Value);
                const float alpha_factor = is_tab_sel ? 1.0f : (t_hov ? 0.85f : 0.55f);
                text_c.Value.w = alpha_factor * window_ease;

                if (font_bold)
                    dl->AddText(font_bold, font_bold->LegacySize, ImVec2(t_min.x + 12.f, t_min.y + (top_tab_h - tsz.y) * 0.5f), Draw->GetClr(text_c), tab.label.c_str());

                if (is_tab_sel) {
                    const float line_w = tsz.x + 8.f;
                    const float line_y = top_tab_y + top_tab_h + 2.f;
                    dl->AddLine(ImVec2(t_min.x + 12.f, line_y), ImVec2(t_min.x + 12.f + line_w, line_y), Draw->GetClr(Colors->accent, window_ease), 2.f);
                }

                top_tab_x += btn_w + 8.f;
            }
        }

        dl->AddLine(ImVec2(pos.x + 14.f, top_sep_y), ImVec2(pos.x + size.x - 14.f, top_sep_y), IM_COL32(36, 37, 38, static_cast<int>(255.f * window_ease)), 1.f);

        const float top_btn_sz = 26.f;
        const float top_btn_y = pos.y + 18.f;
        float header_cur_x = pos.x + size.x - 24.f;

        header_cur_x -= top_btn_sz;
        ImVec2 lua_btn_min(header_cur_x, top_btn_y);
        ImVec2 lua_btn_max(lua_btn_min.x + top_btn_sz, lua_btn_min.y + top_btn_sz);

        Draw->RectFilled(dl, lua_btn_min, lua_btn_max, Draw->GetClr(Colors->button.background, 0.4f * window_ease), 6.f);
        DrawOutline(dl, lua_btn_min, lua_btn_max, 6.f, window_ease);
        if (icon_font) {
            ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_CODE);
            dl->AddText(icon_font, icon_font->LegacySize, lua_btn_min + (ImVec2(top_btn_sz, top_btn_sz) - isz) * 0.5f, Draw->GetClr(Colors->text_secondary, 0.35f * window_ease), ICON_FA_CODE);
        }

        header_cur_x -= (top_btn_sz + 6.f);
        ImVec2 cfg_btn_min(header_cur_x, top_btn_y);
        ImVec2 cfg_btn_max(cfg_btn_min.x + top_btn_sz, cfg_btn_min.y + top_btn_sz);

        ImGui::SetCursorScreenPos(cfg_btn_min);
        ImGui::InvisibleButton(xs("##top_cfg_btn"), ImVec2(top_btn_sz, top_btn_sz));
        const bool cfg_btn_hov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup(xs("##presets_popup"));

        const bool cfg_popup_open = ImGui::IsPopupOpen(xs("##presets_popup"));

        Draw->RectFilled(dl, cfg_btn_min, cfg_btn_max, Draw->GetClr(cfg_btn_hov || cfg_popup_open ? Colors->listbox.hovered : Colors->button.background, window_ease), 6.f);
        DrawOutline(dl, cfg_btn_min, cfg_btn_max, 6.f, window_ease);

        if (icon_font) {
            ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_FOLDER);
            dl->AddText(icon_font, icon_font->LegacySize, cfg_btn_min + (ImVec2(top_btn_sz, top_btn_sz) - isz) * 0.5f, Draw->GetClr(cfg_btn_hov || cfg_popup_open ? Colors->text : Colors->text_secondary, window_ease), ICON_FA_FOLDER);
        }

        if (cfg_btn_hov)
            this->s_deferred_tooltip = xs("Presets Manager");

        header_cur_x -= (top_btn_sz + 6.f);
        ImVec2 about_btn_min(header_cur_x, top_btn_y);
        ImVec2 about_btn_max(about_btn_min.x + top_btn_sz, about_btn_min.y + top_btn_sz);

        ImGui::SetCursorScreenPos(about_btn_min);
        ImGui::InvisibleButton(xs("##top_about_btn"), ImVec2(top_btn_sz, top_btn_sz));
        const bool about_btn_hov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            s_about_open = !s_about_open;

        Draw->RectFilled(dl, about_btn_min, about_btn_max, Draw->GetClr(about_btn_hov || s_about_open ? Colors->listbox.hovered : Colors->button.background, window_ease), 6.f);
        DrawOutline(dl, about_btn_min, about_btn_max, 6.f, window_ease);

        if (icon_font) {
            ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_ELLIPSIS_VERTICAL);
            dl->AddText(icon_font, icon_font->LegacySize, about_btn_min + (ImVec2(top_btn_sz, top_btn_sz) - isz) * 0.5f, Draw->GetClr(about_btn_hov || s_about_open ? Colors->text : Colors->text_secondary, window_ease), ICON_FA_ELLIPSIS_VERTICAL);
        }

        const CTab* cur_active_tab = nullptr;
        for (const auto& cat : Var->gui.categories) {
            for (const auto& tab : cat.tabs) {
                if (tab.id == Var->gui.active_tab_id) {
                    cur_active_tab = &tab;
                    break;
                }
            }
        }

        if (cur_active_tab && !cur_active_tab->subtabs.empty()) {
            float h_sub_x = pos.x + 16.f;
            const float h_sub_y = pos.y + 60.f;

            for (size_t s = 0; s < cur_active_tab->subtabs.size(); ++s) {
                const auto& sub = cur_active_tab->subtabs[s];
                CTab* cur_active_tab = FindTabByID(Var->gui.active_tab_id);
                if (!cur_active_tab)
                    continue;
                const bool is_sub_sel = (cur_active_tab->active_subtab == static_cast<int>(s));

                if (!font_bold)
                    continue;
                ImVec2 tsz = font_bold->CalcTextSizeA(font_bold->LegacySize, FLT_MAX, -1.f, sub.label.c_str());
                ImVec2 s_btn_min(h_sub_x, h_sub_y - 4.f);
                ImVec2 s_btn_max(h_sub_x + tsz.x + 8.f, h_sub_y + tsz.y + 12.f);

                ImGui::SetCursorScreenPos(s_btn_min);
                ImGui::PushID(static_cast<int>(s + 9000));
                ImGui::InvisibleButton(xs("##top_sub_btn"), ImVec2(s_btn_max.x - s_btn_min.x, s_btn_max.y - s_btn_min.y));

                const bool hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked())
                    cur_active_tab->active_subtab = static_cast<int>(s);

                ImGuiID h_anim_id = ImGui::GetID(xs("##top_sub_anim"));
                CSideAnim* hta = this->AnimContainer(&hta, h_anim_id);
                hta->hover = ImClamp(hta->hover + this->FixedSpeed(14.f) * ((hov && !is_sub_sel) ? 1.f : -1.f), 0.f, 1.f);
                hta->select = ImClamp(hta->select + this->FixedSpeed(14.f) * (is_sub_sel ? 1.f : -1.f), 0.f, 1.f);

                const float alpha_factor = ImLerp(0.35f, 1.0f, ImMax(hta->select, hta->hover * 0.7f));
                ImColor text_c = is_sub_sel ? ImColor(Colors->text.Value) : ImColor(Colors->text_secondary.Value);
                text_c.Value.w = alpha_factor * window_ease;

                dl->AddText(font_bold, font_bold->LegacySize, ImVec2(h_sub_x + 4.f, h_sub_y), Draw->GetClr(text_c), sub.label.c_str());

                if (hta->select > 0.01f) {
                    const float line_w = (tsz.x + 8.f) * hta->select;
                    const float line_y = h_sub_y + tsz.y + 8.f;
                    dl->AddLine(ImVec2(h_sub_x + 4.f, line_y), ImVec2(h_sub_x + 4.f + line_w, line_y), Draw->GetClr(Colors->accent, hta->select * window_ease), 2.f);
                }

                ImGui::PopID();
                h_sub_x += tsz.x + 28.f;
            }
        }

        const auto& cfg_items = Config::GetItems();
        const float cfg_row_h = 36.f;
        const float cfg_popup_w = 250.f;
        const float cfg_input_h = 26.f;
        const float cfg_rounding = 12.f;
        const float cfg_list_h = ImMin(static_cast<float>(cfg_items.size()) * (cfg_row_h + 4.f), 220.f);
        const float cfg_popup_h = cfg_input_h + 10.f + cfg_list_h + 8.f;

        ImVec2 popup_pos(cfg_btn_max.x - cfg_popup_w, cfg_btn_max.y + 8.f);
        popup_pos.x = ImClamp(popup_pos.x, pos.x + 8.f, pos.x + size.x - cfg_popup_w - 8.f);

        ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(cfg_popup_w, cfg_popup_h), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(22, 23, 24, 250));
        ImGui::PushStyleColor(ImGuiCol_Border, Var->window.outline_col);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, cfg_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 10.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);

        if (ImGui::BeginPopup(xs("##presets_popup"), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGuiWindow* popup_window = GetCurrentWindow();
            ImDrawList* pop_dl = popup_window->DrawList;

            ImGui::PushStyleColor(ImGuiCol_FrameBg, Draw->GetClr(Colors->button.background, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Draw->GetClr(Colors->button.background, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Draw->GetClr(Colors->button.background, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_Text, Draw->GetClr(Colors->text));
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, Draw->GetClr(Colors->text_secondary));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, Draw->GetClr(Colors->button.background, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Draw->GetClr(Colors->accent, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Draw->GetClr(Colors->accent, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, cfg_rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 5.f));

            const float plus_btn_w = 26.f;
            const float avail_w = cfg_popup_w - 20.f;

            ImGui::PushFont(font_regular);
            ImGui::SetNextItemWidth(avail_w - plus_btn_w - style->ItemSpacing.x);

            const bool enter_pressed = ImGui::InputTextWithHint(xs("##new_cfg"), xs("New Preset Name..."), s_new_config_name, sizeof(s_new_config_name), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopFont();

            ImGui::SameLine();

            ImVec2 plus_min = ImGui::GetCursorScreenPos();
            ImVec2 plus_max = plus_min + ImVec2(plus_btn_w, cfg_input_h);

            ImGui::InvisibleButton(xs("##new_cfg_add"), ImVec2(plus_btn_w, cfg_input_h));

            const bool plus_hov = ImGui::IsItemHovered();
            const bool plus_act = ImGui::IsItemActive();

            pop_dl->AddRectFilled(plus_min, plus_max, Draw->GetClr(plus_act ? Colors->accent : (plus_hov ? Colors->accent : Colors->button.background), plus_act ? 1.f : (plus_hov ? 0.85f : 0.9f)), cfg_rounding);

            const ImVec2 plus_center = ImFloor((plus_min + plus_max) * 0.5f);
            const float plus_half = 4.f;
            const float plus_thick = 1.5f;
            const ImU32 plus_col = Draw->GetClr(Colors->text);

            pop_dl->AddRectFilled(ImVec2(plus_center.x - plus_half, plus_center.y - plus_thick * 0.5f), ImVec2(plus_center.x + plus_half, plus_center.y + plus_thick * 0.5f), plus_col);
            pop_dl->AddRectFilled(ImVec2(plus_center.x - plus_thick * 0.5f, plus_center.y - plus_half), ImVec2(plus_center.x + plus_thick * 0.5f, plus_center.y + plus_half), plus_col);

            if (ImGui::IsItemClicked() || enter_pressed) {
                if (s_new_config_name[0]) {
                    if (Config::Create(s_new_config_name))
                        s_new_config_name[0] = '\0';
                }
            }

            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(9);

            ImGui::Dummy(ImVec2(0.f, 8.f));

            ImGui::BeginChild(xs("##cfg_panel_list"), ImVec2(avail_w, cfg_list_h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            ImVec2 list_min = ImGui::GetCursorScreenPos();
            float cur_y = list_min.y;
            const float list_w = ImGui::GetContentRegionAvail().x;
            const auto& reload_items = Config::GetItems();

            for (size_t i = 0; i < reload_items.size(); ++i) {
                const auto& item = reload_items[i];
                const bool is_act = Config::IsActive(item.name);
                const bool is_start = Config::IsStartup(item.name);

                ImGui::PushID(static_cast<int>(i));

                ImVec2 r_min(list_min.x, cur_y);
                ImVec2 r_max(list_min.x + list_w, cur_y + cfg_row_h);
                ImRect r_rect(r_min, r_max);

                const bool row_hov = r_rect.Contains(io.MousePos);

                if (is_act)
                    pop_dl->AddRectFilled(r_min, r_max, Draw->GetClr(Colors->accent, 0.16f), cfg_rounding);
                else if (row_hov)
                    pop_dl->AddRectFilled(r_min, r_max, Draw->GetClr(Colors->listbox.hovered, 0.5f), cfg_rounding);

                if (font_regular) {
                    ImColor name_clr = is_act ? ImColor(Colors->accent.Value) : (row_hov ? ImColor(Colors->text.Value) : ImColor(Colors->text_secondary.Value));

                    Draw->TextClipped(pop_dl, font_regular, ImVec2(r_min.x + 10.f, r_min.y + 2.f), ImVec2(r_max.x - 115.f, r_min.y + 20.f), Draw->GetClr(name_clr), item.name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
                    Draw->TextClipped(pop_dl, font_regular, ImVec2(r_min.x + 10.f, r_min.y + 18.f), ImVec2(r_max.x - 115.f, r_max.y - 2.f), Draw->GetClr(Colors->text_secondary, 0.6f), item.created_text.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
                }

                float btn_x = r_max.x - 8.f;
                const float icon_hit = 20.f;
                bool hovered_any_button = false;
                bool item_deleted = false;

                auto handle_action_btn = [&](const char* icon_str, const char* tooltip_str, ImColor def_c, ImColor hov_c, std::function<void()> on_click) {
                    btn_x -= icon_hit;
                    ImVec2 b_min(btn_x, r_min.y + (cfg_row_h - icon_hit) * 0.5f);
                    ImVec2 b_max(b_min.x + icon_hit, b_min.y + icon_hit);
                    ImRect b_rect(b_min, b_max);

                    const bool hov = b_rect.Contains(io.MousePos);
                    if (hov) {
                        hovered_any_button = true;
                        this->s_deferred_tooltip = tooltip_str;
                        pop_dl->AddRectFilled(b_min, b_max, Draw->GetClr(Colors->listbox.hovered, 0.7f), 6.f);
                        if (ImGui::IsMouseClicked(0))
                            on_click();
                    }

                    if (icon_font) {
                        ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, icon_str);
                        ImVec2 ipos = b_min + (ImVec2(icon_hit, icon_hit) - isz) * 0.5f;
                        pop_dl->AddText(icon_font, icon_font->LegacySize, ipos, Draw->GetClr(hov ? hov_c : def_c), icon_str);
                    }
                };

                if (!item.is_default) {
                    handle_action_btn(ICON_FA_TRASH_CAN, xs("Delete"), ImColor(Colors->text_secondary.Value), ImColor(230, 60, 60, 255), [&]() {
                        Config::Remove(item.name);
                        item_deleted = true;
                    });
                }

                if (item_deleted) {
                    ImGui::PopID();
                    break;
                }

                handle_action_btn(ICON_FA_FOLDER_OPEN, xs("Show In Explorer"), ImColor(Colors->text_secondary.Value), ImColor(Colors->text.Value), [&]() { Config::ShowInExplorer(item.name); });

                handle_action_btn(ICON_FA_DESKTOP, xs("Set As Startup"), is_start ? ImColor(Colors->accent.Value) : ImColor(Colors->text_secondary.Value), ImColor(Colors->text.Value), [&]() { Config::SetStartupConfig(item.name); });

                if (!item.is_default)
                    handle_action_btn(ICON_FA_FLOPPY_DISK, xs("Save"), ImColor(Colors->text_secondary.Value), ImColor(Colors->text.Value), [&]() { Config::Save(item.name); });

                handle_action_btn(ICON_FA_DOWNLOAD, xs("Load"), ImColor(Colors->text_secondary.Value), ImColor(Colors->text.Value), [&]() {
                    Config::Load(item.name);
                    ImGui::CloseCurrentPopup();
                });

                if (!hovered_any_button && row_hov && ImGui::IsMouseClicked(0)) {
                    Config::Load(item.name);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopID();
                cur_y += cfg_row_h + 4.f;
            }

            ImGui::EndChild();
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);

        static int s_prev_tab = -1;
        static int s_prev_subtab = -1;
        const CTab* cur_active_tab_ptr = nullptr;
        for (const auto& cat : Var->gui.categories) {
            for (const auto& tab : cat.tabs) {
                if (tab.id == Var->gui.active_tab_id) {
                    cur_active_tab_ptr = &tab;
                    break;
                }
            }
        }
        const int cur_subtab = cur_active_tab_ptr ? cur_active_tab_ptr->active_subtab : 0;
        const bool tab_changed = (Var->gui.active_tab_id != s_prev_tab) || (cur_subtab != s_prev_subtab);
        if (tab_changed) {
            if (s_prev_tab == Var->gui.hotkeys_tab_id)
                Binds::CancelCapture();

            Var->gui.content_alpha = 0.f;
            s_prev_tab = Var->gui.active_tab_id;
            s_prev_subtab = cur_subtab;
        }

        Var->gui.content_alpha = ImClamp(Var->gui.content_alpha + this->FixedSpeed(14.f), 0.f, 1.f);
        const float page_ease = Var->gui.content_alpha * Var->gui.content_alpha * (3.f - 2.f * Var->gui.content_alpha);
        const float content_alpha = page_ease * content_gate * window_ease;

        if (content_alpha > 0.01f) {
            this->PushStyleVar(ImGuiStyleVar_Alpha, content_alpha);

            ImGui::SetCursorScreenPos(ImVec2(pos.x + 12.f, pos.y + 88.f));
            ImGui::PushClipRect(ImVec2(pos.x, pos.y + 82.f), ImVec2(pos.x + size.x, pos.y + size.y - 30.f), true);

            this->BeginContent();

            switch (Var->gui.active_tab_id) {
            case 0: RenderRagebotContent(); break;
            case 1: break;
            case 2: RenderVisualsContent(); break;
            case 3: RenderWorldContent(); break;
            case 4: RenderMiscContent(); break;
            case 5: RenderSkinsContent(); break;
            case 6: RenderHotkeysContent(); break;
            default: break;
            }

            this->EndContent();
            ImGui::PopClipRect();
            this->PopStyleVar();
        }

        if (font_sub) {
            const char* brand = xs("customflow");
            ImVec2 bsz = font_sub->CalcTextSizeA(font_sub->LegacySize, FLT_MAX, -1.f, brand);
            ImVec2 bp = ImVec2(pos.x + (size.x - bsz.x) * 0.5f, pos.y + size.y - 22.f);
            dl->AddText(font_sub, font_sub->LegacySize, bp, Draw->GetClr(Colors->text_secondary, 0.35f * window_ease), brand);
        }

        ImGui::EndDisabled();

        if (this->about_active) {
            const float about_w = 330.f;
            const float about_h = s_about_content_h;
            const float about_rounding = 14.f;
            const float pad = 16.f;

            ImVec2 about_min(pos.x + (size.x - about_w) * 0.5f, pos.y + (size.y - about_h) * 0.5f);

            if (about_just_opened)
                ImGui::SetNextWindowFocus();

            ImGui::SetNextWindowPos(about_min, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(about_w, about_h), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, about_ease);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, pad));

            ImGuiWindowFlags about_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;

            ImGui::Begin(xs("##about_panel"), nullptr, about_flags);
            {
                this->about_draw_list = ImGui::GetWindowDrawList();
                ImDrawList* adl = ImGui::GetWindowDrawList();
                ImVec2 apos = ImGui::GetWindowPos();
                ImVec2 asize = ImGui::GetWindowSize();
                const float content_left = apos.x + pad;
                const float content_right = apos.x + asize.x - pad;

                adl->AddRectFilled(apos, apos + asize, IM_COL32(14, 15, 17, static_cast<int>(248.f * about_ease)), about_rounding);
                DrawOutline(adl, apos, apos + asize, about_rounding, about_ease);

                ImFont* title_font = Var->font.inter[1] ? Var->font.inter[1] : (font_logo ? font_logo : font_bold);
                if (title_font)
                    adl->AddText(title_font, 19.f, ImVec2(content_left, apos.y + 14.f), Draw->GetClr(Colors->accent, about_ease), xs("customflow"));

                const float close_sz = 26.f;
                ImVec2 close_min(content_right - close_sz, apos.y + 12.f);
                ImVec2 close_max(close_min.x + close_sz, close_min.y + close_sz);

                ImGui::SetCursorScreenPos(close_min);
                ImGui::InvisibleButton(xs("##about_close"), ImVec2(close_sz, close_sz));
                const bool close_hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked())
                    s_about_open = false;

                adl->AddRectFilled(close_min, close_max, IM_COL32(26, 28, 32, static_cast<int>((close_hov ? 255.f : 160.f) * about_ease)), 6.f);
                if (icon_font) {
                    ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_XMARK);
                    adl->AddText(icon_font, icon_font->LegacySize, close_min + (ImVec2(close_sz, close_sz) - isz) * 0.5f, Draw->GetClr(close_hov ? Colors->text : Colors->text_secondary, about_ease), ICON_FA_XMARK);
                }

                const float card_y = apos.y + 46.f;
                const float card_w = 300.f;
                const float card_h = 83.f;
                const ImVec2 card_min(apos.x + (asize.x - card_w) * 0.5f, card_y);
                const ImVec2 card_max(card_min.x + card_w, card_min.y + card_h);
                adl->AddRectFilled(card_min, card_max, IM_COL32(20, 21, 23, static_cast<int>(230.f * about_ease)), 10.f);

                if (font_regular && font_sub) {
                    const float r1_y = card_min.y + 18.f;
                    adl->AddText(font_sub, font_sub->LegacySize, ImVec2(card_min.x + 16.f, r1_y), Draw->GetClr(Colors->text_secondary, 0.6f * about_ease), xs("Build"));
                    ImVec2 bsz = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, xs("Alpha"));
                    adl->AddText(font_regular, font_regular->LegacySize, ImVec2(card_max.x - 16.f - bsz.x, r1_y), Draw->GetClr(Colors->text, about_ease), xs("Alpha"));

                    const float r2_y = card_min.y + 48.f;
                    adl->AddText(font_sub, font_sub->LegacySize, ImVec2(card_min.x + 16.f, r2_y), Draw->GetClr(Colors->text_secondary, 0.6f * about_ease), xs("Updated"));
                    ImVec2 usz = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, xs("02 Sep 2026"));
                    adl->AddText(font_regular, font_regular->LegacySize, ImVec2(card_max.x - 16.f - usz.x, r2_y), Draw->GetClr(Colors->text, about_ease), xs("02 Sep 2026"));
                }

                ImGui::SetCursorScreenPos(ImVec2(content_left, card_max.y + 14.f));

                if (font_sub)
                    adl->AddText(font_sub, font_sub->LegacySize, ImGui::GetCursorScreenPos(), Draw->GetClr(Colors->text_secondary, 0.45f * about_ease), xs("CUSTOMIZATION"));

                ImGui::Dummy(ImVec2(0.f, font_sub->LegacySize + 8.f));

                this->Dropdown(xs("Theme"), &s_about_theme_idx, { xs("Dark") });

                ImGui::Dummy(ImVec2(0.f, 4.f));

                this->Dropdown(xs("Interface scale"), &s_about_scale_idx, { xs("90%"), xs("100%"), xs("110%"), xs("125%") });

                ImGui::Dummy(ImVec2(0.f, 10.f));
                const float sep1_y = ImGui::GetCursorScreenPos().y;
                adl->AddLine(ImVec2(content_left, sep1_y), ImVec2(content_right, sep1_y), IM_COL32(32, 33, 36, static_cast<int>(255.f * about_ease)), 1.f);
                ImGui::Dummy(ImVec2(0.f, 10.f));

                ImGui::SetCursorScreenPos(ImVec2(content_left, ImGui::GetCursorScreenPos().y));
                if (font_sub)
                    adl->AddText(font_sub, font_sub->LegacySize, ImGui::GetCursorScreenPos(), Draw->GetClr(Colors->text_secondary, 0.45f * about_ease), xs("WIDGETS"));

                ImGui::Dummy(ImVec2(0.f, font_sub->LegacySize + 8.f));

                this->Checkbox(xs("Keybind list"), &s_widget_keybind_list);

                this->Checkbox(xs("Spectator list"), &s_widget_spectator_list);

                this->CheckboxEx(xs("Watermark"), &s_widget_watermark, false, nullptr, 0, [this]() {
                    this->Checkbox(xs("Show Username"), &s_watermark_username);
                    this->Checkbox(xs("Show Ping"), &s_watermark_ping);
                    this->Checkbox(xs("Show FPS"), &s_watermark_fps);
                    });

                this->CheckboxEx(xs("Shot logs"), &s_widget_shot_logs, false, nullptr, 0, [this]() {
                    this->Checkbox(xs("Show Damage"), &s_shot_logs_damage);
                    this->Checkbox(xs("Show Distance"), &s_shot_logs_distance);
                    this->Checkbox(xs("Show Hitgroup"), &s_shot_logs_hitgroup);
                    });

                /*       C_Color* idc[1] = {&g_presets->Get<C_Color>(xs("GUI.INDICATORS_COLOR"))};

                       this->MultiDropdown(xs("Indicators"), &g_presets->Get<std::vector<int>>(xs("GUI.INDICATORS")), { xs("Jump bug"), xs("Force shoot"), xs("Minimum accuracy"), xs("Minimum damage"), xs("Bomb info"), xs("Slow down"), xs("Duck peek assist"), xs("Double tap") });
                       this->ColorEdit(xs("Indicators color"), idc, 1);*/

                const float content_bottom = ImGui::GetCursorScreenPos().y;
                s_about_content_h = (content_bottom - apos.y) + pad;

                bool about_related_hovered = ImRect(apos, apos + asize).Contains(io.MousePos);
                if (!about_related_hovered) {
                    for (void* wptr : this->about_extra_windows) {
                        ImGuiWindow* ew = reinterpret_cast<ImGuiWindow*>(wptr);
                        if (!ew)
                            continue;

                        if (ImRect(ew->Pos, ew->Pos + ew->Size).Contains(io.MousePos)) {
                            about_related_hovered = true;
                            break;
                        }
                    }
                }

                const bool click_outside = ImGui::IsMouseClicked(0) && !about_related_hovered;
                if (click_outside)
                    s_about_open = false;
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }
    }

    this->End();
    ImGui::PopStyleVar(3);

    if (!this->s_deferred_tooltip.empty() && !s_about_open) {
        ImFont* font_regular = Var->font.inter[0];

        ImDrawList* fg_dl = ImGui::GetForegroundDrawList();
        ImVec2 text_sz = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, this->s_deferred_tooltip.c_str());
        ImVec2 padding(8.f, 5.f);
        ImVec2 t_size = text_sz + padding * 2.f;

        ImVec2 t_pos = io.MousePos + ImVec2(12.f, 12.f);
        if (t_pos.x + t_size.x > vp->Size.x)
            t_pos.x = io.MousePos.x - t_size.x - 4.f;

        if (t_pos.y + t_size.y > vp->Size.y)
            t_pos.y = io.MousePos.y - t_size.y - 4.f;

        fg_dl->AddRectFilled(t_pos, t_pos + t_size, Draw->GetClr(Colors->window.background, 0.98f), 8.f);
        fg_dl->AddRect(t_pos, t_pos + t_size, Var->window.outline_col, 8.f, 0, 1.f);
        fg_dl->AddText(font_regular, font_regular->LegacySize, t_pos + padding, Draw->GetClr(Colors->text), this->s_deferred_tooltip.c_str());

        this->s_deferred_tooltip.clear();
    }

    ImGui::GetIO().FontGlobalScale = 1.f;
}