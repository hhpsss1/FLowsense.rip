#include <shared.h>
#include "Blur/Blur.h"
#include <cmath>
#include <vector>

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

    static float content_anim = 0.f;
    static bool s_prev_opened = false;
    static bool s_closing = false;
    static bool s_init = false;
    static ImVec2 s_pos(0.f, 0.f);

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
        Binds::CancelCapture();
    }

    if (!Draw->opened && !s_closing && this->menu_anim <= 0.001f) {
        ImGui::GetIO().FontGlobalScale = 1.f;
        this->ui_alpha = 0.f;
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

        static bool dragging = false;
        static ImVec2 drag_offset(0.f, 0.f);
        const bool over_drag = io.MousePos.x >= pos.x && io.MousePos.x <= (pos.x + size.x) && io.MousePos.y >= pos.y && io.MousePos.y <= (pos.y + 12.f);

        if (over_drag && ImGui::IsMouseClicked(0)) {
            dragging = true;
            drag_offset = ImVec2(io.MousePos.x - s_pos.x, io.MousePos.y - s_pos.y);
        }

        if (dragging) {
            if (ImGui::IsMouseDown(0)) {
                s_pos = ImVec2(io.MousePos.x - drag_offset.x, io.MousePos.y - drag_offset.y);
                s_pos.x = ImClamp(s_pos.x, 0.f, vp->Size.x - size.x);
                s_pos.y = ImClamp(s_pos.y, 0.f, vp->Size.y - size.y);
            }
            else {
                dragging = false;
            }
        }

        dl->AddRectFilled(pos, pos + size, OtcStyle::WindowShadow(window_ease), 0.f);
        dl->AddRectFilled(pos + ImVec2(1.f, 1.f), pos + size - ImVec2(1.f, 1.f), OtcStyle::WindowLine(window_ease), 0.f);
        dl->AddRectFilled(pos + ImVec2(2.f, 2.f), pos + size - ImVec2(2.f, 2.f), OtcStyle::WindowBetween(window_ease), 0.f);
        dl->AddRectFilled(pos + ImVec2(5.f, 5.f), pos + size - ImVec2(5.f, 5.f), IM_COL32(57, 57, 57, static_cast<int>(190.f * window_ease)), 0.f);
        dl->AddRectFilled(pos + ImVec2(6.f, 6.f), pos + size - ImVec2(6.f, 6.f), OtcStyle::WindowFace(window_ease), 0.f);

        // brand centered above the window
        {
            const char* brand = xs("customflow");
            const float brand_fs = 15.f;
            ImFont* brand_font = font_logo ? font_logo : font_regular;
            float brand_w = 0.f;
            for (const char* p = brand; *p; ++p) {
                char glyph[2] = { *p, 0 };
                brand_w += brand_font->CalcTextSizeA(brand_fs, FLT_MAX, 0.f, glyph).x;
            }
            const ImVec2 origin(ImFloor(pos.x + size.x * 0.5f - brand_w * 0.5f), ImFloor(pos.y - 7.f));
            const double time = ImGui::GetTime();
            const ImU32 brand_shadow = IM_COL32(0, 0, 0, static_cast<int>(255.f * window_ease));
            dl->PushClipRectFullScreen();
            float bx = origin.x;
            int bi = 0;
            for (const char* p = brand; *p; ++p, ++bi) {
                char glyph[2] = { *p, 0 };
                const float phase = static_cast<float>(time * 3.2 + bi * 0.65);
                const float pulse = (std::sin(phase) + 1.f) * 0.5f;
                const float lift = ImFloor(std::sin(phase) * 3.f);
                const ImU32 col = IM_COL32(static_cast<int>(185 + pulse * 70), static_cast<int>(20 + pulse * 45), static_cast<int>(35 + pulse * 60), static_cast<int>(255.f * window_ease));
                const ImVec2 gp(ImFloor(bx), origin.y + lift);
                dl->AddText(brand_font, brand_fs, ImVec2(gp.x - 1.f, gp.y), brand_shadow, glyph);
                dl->AddText(brand_font, brand_fs, ImVec2(gp.x + 1.f, gp.y), brand_shadow, glyph);
                dl->AddText(brand_font, brand_fs, ImVec2(gp.x, gp.y - 1.f), brand_shadow, glyph);
                dl->AddText(brand_font, brand_fs, ImVec2(gp.x, gp.y + 1.f), brand_shadow, glyph);
                dl->AddText(brand_font, brand_fs, gp, col, glyph);
                bx += brand_font->CalcTextSizeA(brand_fs, FLT_MAX, 0.f, glyph).x;
            }
            dl->PopClipRect();
        }

        const float panel_x = pos.x + 20.f;
        const float panel_y = pos.y + 44.f;
        const float panel_w = size.x - 40.f;
        const float panel_h = size.y - 60.f;

        // top tab bar
        {
            std::vector<CTab*> flat_tabs;
            for (auto& cat : Var->gui.categories)
                for (auto& tab : cat.tabs)
                    flat_tabs.push_back(&tab);

            const float header_btn_area = 92.f;
            const float tabs_w = panel_w - header_btn_area;
            const int tab_count = static_cast<int>(flat_tabs.size());
            const float tab_slot_w = tab_count > 0 ? tabs_w / static_cast<float>(tab_count) : tabs_w;
            const float tab_y = pos.y + 16.f;
            const float tab_h = 24.f;

            for (int i = 0; i < tab_count; ++i) {
                CTab* tab = flat_tabs[i];
                const bool selected = (Var->gui.active_tab_id == tab->id);
                const float slot_x = panel_x + tab_slot_w * static_cast<float>(i);

                ImGui::SetCursorScreenPos(ImVec2(slot_x, tab_y));
                ImGui::PushID(tab->id * 100);
                ImGui::InvisibleButton(xs("##top_tab"), ImVec2(tab_slot_w, tab_h));
                const bool tab_hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked())
                    Var->gui.active_tab_id = tab->id;
                ImGui::PopID();

                if (font_regular) {
                    const float text_w = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, tab->label.c_str()).x;
                    const float text_x = ImFloor(slot_x + tab_slot_w * 0.5f - text_w * 0.5f);
                    const ImU32 text_col = selected ? OtcStyle::Text(window_ease) : OtcStyle::TextDim((tab_hov ? 1.f : 0.85f) * window_ease);
                    dl->AddText(font_regular, font_regular->LegacySize, ImVec2(text_x, tab_y + 1.f), text_col, tab->label.c_str());
                }

                const float line_x = slot_x + (i == 0 ? -3.f : 2.f);
                const float line_y = pos.y + 35.f - (selected ? 1.f : 0.f);
                const float line_r = slot_x + tab_slot_w + (i == tab_count - 1 ? 3.f : -2.f);
                const ImU32 line_top = selected ? OtcStyle::Accent(window_ease) : OtcStyle::TabIdle(window_ease);
                const ImU32 line_bottom = selected ? IM_COL32(160, 16, 37, static_cast<int>(255.f * window_ease)) : OtcStyle::TabIdleBottom(window_ease);
                const float line_h = selected ? 5.f : 3.f;

                if (selected) {
                    const ImU32 highlight = IM_COL32(255, 92, 112, static_cast<int>(255.f * window_ease));
                    dl->AddRectFilledMultiColor(ImVec2(line_x, line_y), ImVec2(line_r, line_y + 1.f), highlight, highlight, line_top, line_top);
                    dl->AddRectFilledMultiColor(ImVec2(line_x, line_y + 1.f), ImVec2(line_r, line_y + line_h - 1.f), line_top, line_top, line_bottom, line_bottom);
                    dl->AddRectFilled(ImVec2(line_x, line_y + line_h - 1.f), ImVec2(line_r, line_y + line_h), IM_COL32(95, 8, 22, static_cast<int>(255.f * window_ease)));
                } else {
                    dl->AddRectFilledMultiColor(ImVec2(line_x, line_y), ImVec2(line_r, line_y + line_h), line_top, line_top, line_bottom, line_bottom);
                }
                dl->AddRectFilledMultiColor(ImVec2(line_x, line_y + line_h), ImVec2(line_r, line_y + line_h + 1.f), OtcStyle::WindowShadow(window_ease), OtcStyle::WindowShadow(window_ease), OtcStyle::Border(window_ease), OtcStyle::Border(window_ease));
            }
        }

        // header button: presets manager
        const float top_btn_sz = 20.f;
        const float top_btn_y = pos.y + 14.f;
        ImVec2 cfg_btn_min(pos.x + size.x - 20.f - top_btn_sz, top_btn_y);
        ImVec2 cfg_btn_max(cfg_btn_min.x + top_btn_sz, cfg_btn_min.y + top_btn_sz);

        ImGui::SetCursorScreenPos(cfg_btn_min);
        ImGui::InvisibleButton(xs("##top_cfg_btn"), ImVec2(top_btn_sz, top_btn_sz));
        const bool cfg_btn_hov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup(xs("##presets_popup"));
        const bool cfg_popup_open = ImGui::IsPopupOpen(xs("##presets_popup"));

        if (icon_font) {
            ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_FOLDER);
            dl->AddText(icon_font, icon_font->LegacySize, cfg_btn_min + (ImVec2(top_btn_sz, top_btn_sz) - isz) * 0.5f, OtcStyle::TextDim((cfg_btn_hov || cfg_popup_open ? 1.f : 0.7f) * window_ease), ICON_FA_FOLDER);
        }
        if (cfg_btn_hov)
            this->s_deferred_tooltip = xs("Presets Manager");

        // inner panel layers
        dl->AddRectFilled(ImVec2(panel_x, panel_y), ImVec2(panel_x + panel_w, panel_y + panel_h), IM_COL32(0, 0, 0, static_cast<int>(145.f * window_ease)));
        dl->AddRectFilled(ImVec2(panel_x + 1.f, panel_y + 1.f), ImVec2(panel_x + panel_w - 1.f, panel_y + panel_h - 1.f), IM_COL32(44, 44, 44, static_cast<int>(145.f * window_ease)));
        dl->AddRectFilled(ImVec2(panel_x + 2.f, panel_y + 2.f), ImVec2(panel_x + panel_w - 2.f, panel_y + panel_h - 2.f), IM_COL32(9, 9, 9, static_cast<int>(120.f * window_ease)));
        dl->AddRectFilled(ImVec2(panel_x + 3.f, panel_y + 3.f), ImVec2(panel_x + panel_w - 3.f, panel_y + panel_h - 3.f), IM_COL32(9, 9, 9, static_cast<int>(82.f * window_ease)));

        const CTab* cur_active_tab = nullptr;
        for (const auto& cat : Var->gui.categories) {
            for (const auto& tab : cat.tabs) {
                if (tab.id == Var->gui.active_tab_id) {
                    cur_active_tab = &tab;
                    break;
                }
            }
            if (cur_active_tab)
                break;
        }

        const bool has_subtabs = (cur_active_tab && !cur_active_tab->subtabs.empty());
        if (has_subtabs) {
            float sub_x = panel_x + 12.f;
            const float sub_y = panel_y + 8.f;
            for (size_t s = 0; s < cur_active_tab->subtabs.size(); ++s) {
                const auto& sub = cur_active_tab->subtabs[s];
                CTab* active_tab = FindTabByID(Var->gui.active_tab_id);
                const bool is_sub_sel = (active_tab && active_tab->active_subtab == static_cast<int>(s));

                ImVec2 tsz = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, sub.label.c_str());
                ImVec2 s_btn_min(sub_x - 4.f, sub_y - 3.f);
                ImVec2 s_btn_max(sub_x + tsz.x + 4.f, sub_y + tsz.y + 6.f);

                ImGui::SetCursorScreenPos(s_btn_min);
                ImGui::PushID(static_cast<int>(s + 9000));
                ImGui::InvisibleButton(xs("##top_sub_btn"), ImVec2(s_btn_max.x - s_btn_min.x, s_btn_max.y - s_btn_min.y));
                const bool sub_hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked() && active_tab)
                    active_tab->active_subtab = static_cast<int>(s);
                ImGui::PopID();

                const ImU32 text_col = is_sub_sel ? OtcStyle::Text(window_ease) : OtcStyle::TextDim((sub_hov ? 1.f : 0.75f) * window_ease);
                dl->AddText(font_regular, font_regular->LegacySize, ImVec2(sub_x, sub_y), text_col, sub.label.c_str());

                if (is_sub_sel)
                    dl->AddRectFilled(ImVec2(sub_x - 4.f, sub_y + tsz.y + 2.f), ImVec2(sub_x + tsz.x + 4.f, sub_y + tsz.y + 4.f), OtcStyle::Accent(window_ease));

                sub_x += tsz.x + 20.f;
            }
        }

        const float content_top = panel_y + (has_subtabs ? 32.f : 10.f);

        const auto& cfg_items = Config::GetItems();
        const float cfg_row_h = 32.f;
        const float cfg_popup_w = 240.f;
        const float cfg_input_h = 24.f;
        const float cfg_rounding = 0.f;
        const float cfg_header_h = 18.f;
        const float cfg_list_h = ImMin(static_cast<float>(cfg_items.size()) * (cfg_row_h + 2.f), 200.f);
        const float cfg_popup_h = 10.f + cfg_header_h + 8.f + cfg_input_h + 10.f + cfg_list_h + 8.f;

        ImVec2 popup_pos(cfg_btn_max.x - cfg_popup_w, cfg_btn_max.y + 8.f);
        popup_pos.x = ImClamp(popup_pos.x, pos.x + 8.f, pos.x + size.x - cfg_popup_w - 8.f);

        ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(cfg_popup_w, cfg_popup_h), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(14, 14, 14, 250));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(44, 44, 44, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, cfg_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 10.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);

        if (ImGui::BeginPopup(xs("##presets_popup"), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGuiWindow* popup_window = GetCurrentWindow();
            ImDrawList* pop_dl = popup_window->DrawList;

            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(24, 24, 24, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(30, 30, 30, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(30, 30, 30, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, Draw->GetClr(Colors->text));
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, Draw->GetClr(Colors->text_secondary, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(24, 24, 24, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Draw->GetClr(Colors->accent, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Draw->GetClr(Colors->accent, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, cfg_rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 4.f));

            {
                const ImVec2 header_pos = ImGui::GetCursorScreenPos();
                pop_dl->AddText(font_regular, font_regular->LegacySize, header_pos, Draw->GetClr(Colors->text_secondary, 0.9f), xs("configs"));

                const std::string cfg_count = std::to_string(cfg_items.size());
                const ImVec2 cfg_count_size = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, cfg_count.c_str());
                pop_dl->AddText(font_regular, font_regular->LegacySize, ImVec2(popup_window->Pos.x + popup_window->Size.x - 10.f - cfg_count_size.x, header_pos.y), Draw->GetClr(Colors->text_secondary, 0.5f), cfg_count.c_str());

                ImGui::Dummy(ImVec2(0.f, cfg_header_h));
            }

            const float plus_btn_w = 24.f;
            const float avail_w = cfg_popup_w - 20.f;

            ImGui::PushFont(font_regular);
            ImGui::SetNextItemWidth(avail_w - plus_btn_w - style->ItemSpacing.x);

            const bool enter_pressed = ImGui::InputTextWithHint(xs("##new_cfg"), xs("new config..."), s_new_config_name, sizeof(s_new_config_name), ImGuiInputTextFlags_EnterReturnsTrue);
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

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
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

                const bool row_hov = r_rect.Contains(io.MousePos) && ImGui::IsWindowHovered();

                if (is_act) {
                    pop_dl->AddRectFilled(r_min, r_max, Draw->GetClr(Colors->accent, 0.12f), cfg_rounding);
                    pop_dl->AddRectFilled(r_min, ImVec2(r_min.x + 2.f, r_max.y), Draw->GetClr(Colors->accent, 1.f));
                }
                else if (row_hov)
                    pop_dl->AddRectFilled(r_min, r_max, IM_COL32(255, 255, 255, 8), cfg_rounding);

                if (font_regular) {
                    ImColor name_clr = is_act ? ImColor(Colors->accent.Value) : (row_hov ? ImColor(Colors->text.Value) : ImColor(Colors->text_secondary.Value));

                    Draw->TextClipped(pop_dl, font_regular, ImVec2(r_min.x + 10.f, r_min.y + 1.f), ImVec2(r_max.x - 95.f, r_min.y + 17.f), Draw->GetClr(name_clr), item.name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
                    Draw->TextClipped(pop_dl, font_regular, ImVec2(r_min.x + 10.f, r_min.y + 15.f), ImVec2(r_max.x - 95.f, r_max.y - 1.f), Draw->GetClr(Colors->text_secondary, 0.45f), item.created_text.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
                }

                float btn_x = r_max.x - 4.f;
                const float icon_hit = 18.f;
                bool hovered_any_button = false;
                bool item_deleted = false;

                auto handle_action_btn = [&](const char* icon_str, const char* tooltip_str, ImColor def_c, ImColor hov_c, std::function<void()> on_click) {
                    btn_x -= icon_hit;
                    ImVec2 b_min(btn_x, r_min.y + (cfg_row_h - icon_hit) * 0.5f);
                    ImVec2 b_max(b_min.x + icon_hit, b_min.y + icon_hit);
                    ImRect b_rect(b_min, b_max);

                    const bool hov = b_rect.Contains(io.MousePos) && ImGui::IsWindowHovered();
                    if (hov) {
                        hovered_any_button = true;
                        this->s_deferred_tooltip = tooltip_str;
                        pop_dl->AddRectFilled(b_min, b_max, IM_COL32(255, 255, 255, 10), 0.f);
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
                cur_y += cfg_row_h + 2.f;
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();
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

            ImGui::SetCursorScreenPos(ImVec2(panel_x + 3.f, content_top));
            ImGui::PushClipRect(ImVec2(panel_x + 3.f, content_top), ImVec2(panel_x + panel_w - 3.f, panel_y + panel_h - 3.f), true);

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


    }

    this->End();
    ImGui::PopStyleVar(3);

    if (!this->s_deferred_tooltip.empty()) {
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