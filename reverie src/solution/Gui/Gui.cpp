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

    auto draw_widgets = [&]() {
        ImGuiIO& wio = ImGui::GetIO();
        ImDrawList* wdl = ImGui::GetForegroundDrawList();
        if (!Draw || !Draw->initialized || !Var || !Colors)
            return;

        ImFont* wfont = Var->font.inter[0] ? Var->font.inter[0] : nullptr;
        if (!wfont)
            return;

        if (s_widget_watermark) {
            std::string text = xs("CustomFlow");
            if (s_watermark_username)
                text += xs("  |  user");
            if (s_watermark_ping)
                text += xs("  |  0 ms");
            if (s_watermark_fps)
                text += xs("  |  ") + std::to_string(static_cast<int>(wio.Framerate)) + xs(" fps");

            ImVec2 pos(10.f, 10.f);
            wdl->AddText(wfont, wfont->LegacySize, pos, Draw->GetClr(Colors->accent, 1.f), text.c_str());
        }

        if (s_widget_keybind_list) {
            auto active = Binds::GetActiveList();
            if (!active.empty()) {
                float x = wio.DisplaySize.x - 220.f;
                float y = 10.f;
                float line_h = wfont->LegacySize + 4.f;
                wdl->AddRectFilled(ImVec2(x - 8.f, y - 4.f), ImVec2(x + 200.f, y + active.size() * line_h + 4.f), IM_COL32(14, 15, 17, 200), 8.f);
                for (const auto& bind : active) {
                    std::string line = bind.name + xs("  [") + bind.key_name + xs("]");
                    ImU32 col = bind.active ? Draw->GetClr(Colors->accent, 1.f) : Draw->GetClr(Colors->text, 0.8f);
                    wdl->AddText(wfont, wfont->LegacySize, ImVec2(x, y), col, line.c_str());
                    y += line_h;
                }
            }
        }
    };

    if (!Draw->opened && !s_closing && this->menu_anim <= 0.001f) {
        // draw_widgets(); // temporarily disabled for crash test
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

        // Oldschool flat look: no blur, no bloom, sharp corners, thin border
        dl->AddRectFilled(pos, pos + size, IM_COL32(18, 18, 18, static_cast<int>(255.f * window_ease)), 0.f);
        dl->AddRectFilled(pos, ImVec2(pos.x + sidebar_w, pos.y + size.y), IM_COL32(22, 22, 22, static_cast<int>(255.f * window_ease)), 0.f);
        DrawOutline(dl, pos, pos + size, 0.f, window_ease);

        // Oldschool: brand text top-left, small, accent color
        if (font_regular) {
            const char* brand = xs("CustomFlow");
            dl->AddText(font_regular, font_regular->LegacySize, ImVec2(pos.x + 12.f, pos.y + 10.f), Draw->GetClr(Colors->accent, window_ease), brand);
        }

        const float top_sep_y = pos.y + 30.f;
        dl->AddLine(ImVec2(pos.x, top_sep_y), ImVec2(pos.x + sidebar_w, top_sep_y), IM_COL32(40, 40, 40, static_cast<int>(255.f * window_ease)), 1.f);

        float side_y = top_sep_y + 8.f;
        const float tab_pad_x = 6.f;
        const float tab_w = sidebar_w - (tab_pad_x * 2.f);

        ImGui::BeginDisabled(about_blocking);

        for (auto& cat : Var->gui.categories) {
            if (font_sub) {
                dl->AddText(font_sub, font_sub->LegacySize, ImVec2(pos.x + tab_pad_x + 6.f, side_y), Draw->GetClr(Colors->text_secondary, 0.35f * window_ease), cat.label.c_str());
                side_y += font_sub->LegacySize + 8.f;
            }

            for (auto& tab : cat.tabs) {
                const bool is_tab_sel = (Var->gui.active_tab_id == tab.id);
                const float tab_h = 32.f;
                ImVec2 t_min(pos.x + tab_pad_x, side_y);
                ImVec2 t_max(t_min.x + tab_w, t_min.y + tab_h);

                ImGui::SetCursorScreenPos(t_min);
                ImGui::PushID(tab.id * 100);
                ImGui::InvisibleButton(xs("##side_tab"), ImVec2(tab_w, tab_h));

                const bool t_hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked())
                    Var->gui.active_tab_id = tab.id;

                ImGuiID anim_id = ImGui::GetID(xs("##side_t_anim"));
                CSideAnim* ta = this->AnimContainer(&ta, anim_id);
                ta->hover = ImClamp(ta->hover + this->FixedSpeed(14.f) * ((t_hov && !is_tab_sel) ? 1.f : -1.f), 0.f, 1.f);
                ta->select = ImClamp(ta->select + this->FixedSpeed(14.f) * (is_tab_sel ? 1.f : -1.f), 0.f, 1.f);
                ta->expand = ImClamp(ta->expand + this->FixedSpeed(14.f) * (is_tab_sel ? 1.f : -1.f), 0.f, 1.f);

                if (ta->select > 0.01f) {
                    // Oldschool: solid accent bar on left, no rounded bg
                    dl->AddRectFilled(t_min, ImVec2(t_min.x + 3.f, t_max.y), Draw->GetClr(Colors->accent, window_ease), 0.f);
                }
                if (ta->hover > 0.01f && !is_tab_sel) {
                    dl->AddRectFilled(t_min, t_max, Draw->GetClr(Colors->text, 0.04f * ta->hover * window_ease), 0.f);
                }

                const float alpha_factor = ImLerp(0.40f, 1.0f, ImMax(ta->select, ta->hover * 0.7f));
                ImColor item_clr = ImColor(alpha_factor, alpha_factor, alpha_factor, window_ease);

                if (icon_font) {
                    ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, tab.icon.c_str());
                    ImVec2 ip(t_min.x + 12.f, t_min.y + (tab_h - isz.y) * 0.5f);
                    dl->AddText(icon_font, icon_font->LegacySize, ip, Draw->GetClr(item_clr), tab.icon.c_str());
                }

                if (font_regular) {
                    ImVec2 lsz = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, tab.label.c_str());
                    ImVec2 lp(t_min.x + 36.f, t_min.y + (tab_h - lsz.y) * 0.5f);
                    dl->AddText(font_regular, font_regular->LegacySize, lp, Draw->GetClr(item_clr), tab.label.c_str());
                }

                ImGui::PopID();
                side_y += tab_h + 3.f;

                if (!tab.subtabs.empty() && ta->expand > 0.001f) {
                    const float expand_ease = ta->expand * ta->expand * (3.f - 2.f * ta->expand);
                    const float sub_h = 24.f;
                    const float sub_gap = 2.f;
                    const float total_subtabs_h = static_cast<float>(tab.subtabs.size()) * (sub_h + sub_gap);
                    const float animated_block_h = total_subtabs_h * expand_ease;

                    ImGui::PushClipRect(ImVec2(pos.x, side_y), ImVec2(pos.x + sidebar_w, side_y + animated_block_h), true);

                    const float tree_x = pos.x + tab_pad_x + 14.f;
                    const float line_y_start = side_y + sub_h * 0.5f;
                    const float line_y_end = side_y + (static_cast<float>(tab.subtabs.size()) - 1.f) * (sub_h + sub_gap) + sub_h * 0.5f;

                    if (tab.subtabs.size() > 1)
                        dl->AddLine(ImVec2(tree_x, line_y_start), ImVec2(tree_x, line_y_end), Draw->GetClr(Colors->divider, 0.25f * expand_ease * window_ease), 1.f);

                    for (size_t s = 0; s < tab.subtabs.size(); ++s) {
                        CTab* cur_active_tab = FindTabByID(Var->gui.active_tab_id);
                        const bool is_sub_sel = (cur_active_tab->active_subtab == static_cast<int>(s));
                        const float sub_item_y = side_y + static_cast<float>(s) * (sub_h + sub_gap);
                        ImVec2 s_min(pos.x + tab_pad_x, sub_item_y);
                        ImVec2 s_max(s_min.x + tab_w, s_min.y + sub_h);

                        ImGui::SetCursorScreenPos(s_min);
                        ImGui::PushID(static_cast<int>(tab.id * 1000 + s + 500));
                        ImGui::InvisibleButton(xs("##side_sub"), ImVec2(tab_w, sub_h));

                        const bool s_hov = ImGui::IsItemHovered();
                        if (ImGui::IsItemClicked()) {
                            Var->gui.active_tab_id = tab.id;
                            tab.active_subtab = static_cast<int>(s);
                        }

                        ImGuiID s_anim_id = ImGui::GetID(xs("##sub_anim"));
                        CSideAnim* sta = this->AnimContainer(&sta, s_anim_id);
                        sta->hover = ImClamp(sta->hover + this->FixedSpeed(14.f) * ((s_hov && !is_sub_sel) ? 1.f : -1.f), 0.f, 1.f);
                        sta->select = ImClamp(sta->select + this->FixedSpeed(14.f) * (is_sub_sel ? 1.f : -1.f), 0.f, 1.f);

                        const float sub_dot_y = sub_item_y + sub_h * 0.5f;
                        const float dot_radius = is_sub_sel ? 2.5f : 1.5f;
                        const ImColor dot_color = is_sub_sel ? ImColor(Colors->accent.Value) : ImColor(Colors->text_secondary.Value);
                        dl->AddCircleFilled(ImVec2(tree_x, sub_dot_y), dot_radius, Draw->GetClr(dot_color, (is_sub_sel ? 0.9f : 0.4f) * expand_ease * window_ease), 16);

                        const float sub_alpha = ImLerp(0.35f, 1.0f, ImMax(sta->select, sta->hover * 0.7f)) * expand_ease;
                        ImColor sub_clr = is_sub_sel ? ImColor(Colors->accent.Value) : ImColor(Colors->text_secondary.Value);
                        sub_clr.Value.w = sub_alpha * window_ease;

                        if (icon_font) {
                            ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, tab.subtabs[s].icon.c_str());
                            ImVec2 ip(tree_x + 10.f, sub_item_y + (sub_h - isz.y) * 0.5f);
                            dl->AddText(icon_font, icon_font->LegacySize, ip, Draw->GetClr(sub_clr), tab.subtabs[s].icon.c_str());
                        }

                        if (font_regular) {
                            ImVec2 lsz = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, tab.subtabs[s].label.c_str());
                            ImVec2 lp(tree_x + 30.f, sub_item_y + (sub_h - lsz.y) * 0.5f);
                            dl->AddText(font_regular, font_regular->LegacySize, lp, Draw->GetClr(sub_clr), tab.subtabs[s].label.c_str());
                        }

                        ImGui::PopID();
                    }

                    ImGui::PopClipRect();
                    side_y += animated_block_h + 4.f * expand_ease;
                }
            }
            side_y += 8.f;
        }

        const float bot_sep_y = pos.y + size.y - 46.f;
        dl->AddLine(ImVec2(pos.x + 14.f, bot_sep_y), ImVec2(pos.x + sidebar_w - 14.f, bot_sep_y), IM_COL32(36, 37, 38, static_cast<int>(255.f * window_ease)), 1.f);

        const float profile_y = bot_sep_y + 12.f;
        const float av_sz = 26.f;
        const ImVec2 av_min(pos.x + 12.f, profile_y);

        dl->AddRectFilled(av_min, av_min + ImVec2(av_sz, av_sz), IM_COL32(38, 40, 44, static_cast<int>(255.f * window_ease)), 0.f);
        Draw->Rect(dl, av_min, av_min + ImVec2(av_sz, av_sz), IM_COL32(58, 60, 66, static_cast<int>(255.f * window_ease)), 0.f, 0, 1.f);

        const float uname_x = av_min.x + av_sz + 10.f;
        dl->AddText(font_regular, font_regular->LegacySize, ImVec2(uname_x, profile_y + 4.f), Draw->GetClr(Colors->text, window_ease), xs("Guest"));
        dl->AddText(font_sub, font_sub->LegacySize, ImVec2(uname_x, profile_y + 18.f), Draw->GetClr(Colors->text_secondary, 0.65f * window_ease), xs("Lifetime"));

        const float dot_btn_sz = 20.f;
        const ImVec2 dots_min(pos.x + sidebar_w - 16.f - dot_btn_sz, profile_y + av_sz * 0.5f - dot_btn_sz * 0.5f);

        ImGui::SetCursorScreenPos(dots_min);
        ImGui::InvisibleButton(xs("##side_profile_dots"), ImVec2(dot_btn_sz, dot_btn_sz));
        const bool dots_hover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            s_about_open = !s_about_open;

        if (icon_font) {
            ImVec2 d_sz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_ELLIPSIS_VERTICAL);
            ImVec2 dp = dots_min + (ImVec2(dot_btn_sz, dot_btn_sz) - d_sz) * 0.5f;
            dl->AddText(icon_font, icon_font->LegacySize, dp, Draw->GetClr(dots_hover ? Colors->text : Colors->text_secondary, (dots_hover ? 1.0f : 0.45f) * window_ease), ICON_FA_ELLIPSIS_VERTICAL);
        }

        const float top_btn_sz = 22.f;
        const float top_btn_y = pos.y + 6.f;
        float header_cur_x = pos.x + size.x - 22.f;

        header_cur_x -= top_btn_sz;
        ImVec2 lua_btn_min(header_cur_x, top_btn_y);
        ImVec2 lua_btn_max(lua_btn_min.x + top_btn_sz, lua_btn_min.y + top_btn_sz);

        Draw->RectFilled(dl, lua_btn_min, lua_btn_max, Draw->GetClr(Colors->button.background, 0.4f * window_ease), 0.f);
        DrawOutline(dl, lua_btn_min, lua_btn_max, 0.f, window_ease);
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

        Draw->RectFilled(dl, cfg_btn_min, cfg_btn_max, Draw->GetClr(cfg_btn_hov || cfg_popup_open ? Colors->listbox.hovered : Colors->button.background, window_ease), 0.f);
        DrawOutline(dl, cfg_btn_min, cfg_btn_max, 0.f, window_ease);

        if (icon_font) {
            ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_FOLDER);
            dl->AddText(icon_font, icon_font->LegacySize, cfg_btn_min + (ImVec2(top_btn_sz, top_btn_sz) - isz) * 0.5f, Draw->GetClr(cfg_btn_hov || cfg_popup_open ? Colors->text : Colors->text_secondary, window_ease), ICON_FA_FOLDER);
        }

        if (cfg_btn_hov)
            this->s_deferred_tooltip = xs("Presets Manager");

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
            float h_sub_x = pos.x + sidebar_w + 10.f;
            const float h_sub_y = pos.y + 8.f;

            for (size_t s = 0; s < cur_active_tab->subtabs.size(); ++s) {
                const auto& sub = cur_active_tab->subtabs[s];
                CTab* cur_active_tab = FindTabByID(Var->gui.active_tab_id);
                const bool is_sub_sel = (cur_active_tab->active_subtab == static_cast<int>(s));

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
        const float cfg_rounding = 0.f;
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

            ImGui::SetCursorScreenPos(ImVec2(pos.x + sidebar_w + 8.f, pos.y + 32.f));
            ImGui::PushClipRect(ImVec2(pos.x + sidebar_w, pos.y + 28.f), ImVec2(pos.x + size.x, pos.y + size.y - 4.f), true);

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

        ImGui::EndDisabled();

        if (this->about_active) {
            const float about_w = 330.f;
            const float about_h = s_about_content_h;
            const float about_rounding = 0.f;
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
                    adl->AddText(title_font, 19.f, ImVec2(content_left, apos.y + 14.f), Draw->GetClr(Colors->accent, about_ease), xs("CustomFlow"));

                const float close_sz = 26.f;
                ImVec2 close_min(content_right - close_sz, apos.y + 12.f);
                ImVec2 close_max(close_min.x + close_sz, close_min.y + close_sz);

                ImGui::SetCursorScreenPos(close_min);
                ImGui::InvisibleButton(xs("##about_close"), ImVec2(close_sz, close_sz));
                const bool close_hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked())
                    s_about_open = false;

                adl->AddRectFilled(close_min, close_max, IM_COL32(26, 28, 32, static_cast<int>((close_hov ? 255.f : 160.f) * about_ease)), 0.f);
                if (icon_font) {
                    ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_XMARK);
                    adl->AddText(icon_font, icon_font->LegacySize, close_min + (ImVec2(close_sz, close_sz) - isz) * 0.5f, Draw->GetClr(close_hov ? Colors->text : Colors->text_secondary, about_ease), ICON_FA_XMARK);
                }

                const float card_y = apos.y + 46.f;
                const float card_w = 300.f;
                const float card_h = 83.f;
                const ImVec2 card_min(apos.x + (asize.x - card_w) * 0.5f, card_y);
                const ImVec2 card_max(card_min.x + card_w, card_min.y + card_h);
                adl->AddRectFilled(card_min, card_max, IM_COL32(20, 21, 23, static_cast<int>(230.f * about_ease)), 0.f);

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

    // draw_widgets();

    ImGui::GetIO().FontGlobalScale = 1.f;
}