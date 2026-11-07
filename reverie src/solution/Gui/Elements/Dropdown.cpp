#include <shared.h>
#include "../Data/FontsAwesome.h"

extern void GuiMarkPopupOpen();
extern bool GuiIsPopupHovered();
extern ImU32 GuiPopupBg(float alpha);
extern void RenderGearPopupFn(ImDrawList* dl, const ImRect& gear_bb, const char* label_uid, std::function<void()> render_fn);

static ImGuiID s_open_combo_id = 0;

static const ImU32 k_dd_box_bg = IM_COL32(29, 31, 32, 250);
static const ImU32 k_dd_box_bg_hov = IM_COL32(36, 38, 40, 250);
static const ImU32 k_dd_item_sel = IM_COL32(48, 50, 56, 255);
static const ImU32 k_dd_text_grey = IM_COL32(150, 150, 154, 255);
static const ImU32 k_dd_text_white = IM_COL32(232, 232, 235, 255);

struct dropdown_state {
    float alpha{ 0.f };
    float hover{ 0.f };
    bool opened{ false };
    bool hovered{ false };
    bool need_focus{ false };
    int last_frame{ 0 };
};

static void RenderCheckmark(ImDrawList* dl, ImVec2 center, float anim, ImU32 col) {
    if (anim <= 0.005f)
        return;

    const float thick = 1.8f;
    const ImVec2 p0 = ImVec2(center.x - 3.8f, center.y);
    const ImVec2 p1 = ImVec2(center.x - 1.4f, center.y + 2.4f);
    const ImVec2 p2 = ImVec2(center.x + 3.8f, center.y - 2.8f);

    const float split = 0.32f;
    const float t = ImClamp(anim, 0.f, 1.f);

    if (t <= split) {
        float seg_t = t / split;
        ImVec2 cur = ImLerp(p0, p1, seg_t);
        dl->AddLine(p0, cur, col, thick);
        dl->AddCircleFilled(p0, thick * 0.5f, col, 8);
        dl->AddCircleFilled(cur, thick * 0.5f, col, 8);
    } else {
        float seg_t = (t - split) / (1.f - split);
        ImVec2 cur = ImLerp(p1, p2, seg_t);
        ImVec2 pts[3] = { p0, p1, cur };
        dl->AddPolyline(pts, 3, col, 0, thick);
        dl->AddCircleFilled(p0, thick * 0.5f, col, 8);
        dl->AddCircleFilled(p1, thick * 0.5f, col, 8);
        dl->AddCircleFilled(cur, thick * 0.5f, col, 8);
    }
}

static void draw_box_shadow(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding, float alpha_mult) {
    if (alpha_mult <= 0.01f)
        return;

    const float base_a = 158.f * alpha_mult;
    const int steps = 8;
    const float blur = 22.f;
    const ImVec2 offset(0.f, 8.f);

    for (int i = 0; i < steps; ++i) {
        float factor = static_cast<float>(i + 1) / static_cast<float>(steps);
        float cur_spread = blur * factor;
        float a = (base_a / static_cast<float>(steps)) * (1.0f - factor * 0.4f);
        ImU32 shadow_col = IM_COL32(0, 0, 0, static_cast<int>(a));
        ImVec2 s_min(min.x + offset.x - cur_spread * 0.5f, min.y + offset.y - cur_spread * 0.5f);
        ImVec2 s_max(max.x + offset.x + cur_spread * 0.5f, max.y + offset.y + cur_spread * 0.5f);
        dl->AddRectFilled(s_min, s_max, shadow_col, rounding + cur_spread * 0.35f);
    }
}

static bool is_any_picker_open_or_hovered() {
    ImGuiContext& g = *GImGui;
    for (ImGuiWindow* w : g.Windows) {
        if (w->WasActive && w->Name && (strstr(w->Name, xs("picker")) || strstr(w->Name, xs("sw_edit")) || strstr(w->Name, xs("swatch_overlay")))) {
            if (ImGui::IsMouseHoveringRect(w->Pos, w->Pos + w->Size, false))
                return true;
        }
    }
    return false;
}

bool begin_combo(const char* label, const char* preview_value, int val, const std::vector<std::string>& items, bool multi, float right_reserve = 0.f, const char* bind_popup_name = "", bool keep_open = false, float custom_popup_w = 0.f) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    g.NextWindowData.ClearFlags();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    dropdown_state* state = Gui->AnimContainer(&state, id);

    const bool is_in_about = (Gui->about_active && Gui->about_draw_list && window->DrawList == Gui->about_draw_list);
    if (Gui->about_active && !is_in_about) {
        state->opened = false;
        state->alpha = 0.f;

        if (s_open_combo_id == id)
            s_open_combo_id = 0;

        return false;
    }

    const float avail = GetContentRegionAvail().x;
    const float row_h = 26.f;
    const float box_h = 26.f;

    ImFont* font = Var->font.inter[0];
    ImFont* icon_font = Var->font.fa[0];

    const float text_pad = 10.f;
    const float icon_pad = 10.f;
    const float icon_w = 10.f;

    const float pad_x = 7.f;
    const float pad_y = 7.f;

    const bool has_label = (label && label[0] && label[0] != '#');

    float max_text_w = 0.f;
    if (font) {
        for (auto& it : items) {
            ImVec2 sz = font->CalcTextSizeA(font->LegacySize, FLT_MAX, -1.f, it.c_str());
            max_text_w = ImMax(max_text_w, sz.x);
        }
    }

    const float item_left_pad = 10.f;
    const float item_right_reserve = 30.f;
    const float needed_w = max_text_w + item_left_pad + item_right_reserve + pad_x * 2.f;
    const float default_min_w = ImMin(145.f, avail * 0.48f);

    float total_box_w;
    if (custom_popup_w > 0.f) {
        total_box_w = custom_popup_w;
    } else if (has_label) {
        const float label_min_w = 60.f;
        const float upper_bound = ImMax(default_min_w, avail - label_min_w - 6.f);
        total_box_w = ImClamp(needed_w, default_min_w, upper_bound);
    } else {
        const float upper_bound = ImMax(default_min_w, avail - right_reserve);
        total_box_w = ImClamp(needed_w, default_min_w, upper_bound);
    }

    const ImVec2 pos = window->DC.CursorPos;

    const ImVec2 box_min(pos.x + avail - total_box_w, pos.y + (row_h - box_h) * 0.5f);
    const ImVec2 box_max(pos.x + avail - right_reserve, box_min.y + box_h);

    const ImRect label_rect(pos, ImVec2(box_min.x - 6.f, pos.y + row_h));
    const ImRect value_rect(box_min, box_max);
    const ImRect row_rect(pos, ImVec2(pos.x + avail, pos.y + row_h));

    const ImRect value_text_rect(ImVec2(value_rect.Min.x + text_pad, value_rect.Min.y), ImVec2(value_rect.Max.x - icon_pad - icon_w, value_rect.Max.y));
    const ImRect icon_rect(ImVec2(value_rect.Max.x - icon_pad - icon_w, value_rect.Min.y), ImVec2(value_rect.Max.x - icon_pad, value_rect.Max.y));

    ItemSize(row_rect, style.FramePadding.y);
    if (!ItemAdd(row_rect, id))
        return false;

    const bool hovered_row = ItemHoverable(row_rect, id, g.LastItemData.ItemFlags);
    if (hovered_row && g.IO.MouseClicked[1] && bind_popup_name && bind_popup_name[0])
        ImGui::OpenPopup(bind_popup_name);

    if (state->last_frame < g.FrameCount - 1 || !Draw->opened) {
        state->opened = false;
        state->alpha = 0.f;
        if (s_open_combo_id == id)
            s_open_combo_id = 0;
    }

    state->last_frame = g.FrameCount;

    bool hovered_value = hovered_row && value_rect.Contains(g.IO.MousePos);
    bool pressed = hovered_value && g.IO.MouseClicked[0];

    std::string popup_win_name = (std::stringstream{} << GetCurrentWindow()->Name << label << "dropdown_window").str();
    ImGuiWindow* popup_win_ptr = ImGui::FindWindowByName(popup_win_name.c_str());

    bool hovered_popup_win = false;
    if (popup_win_ptr && (popup_win_ptr->WasActive || state->opened))
        hovered_popup_win = ImGui::IsMouseHoveringRect(popup_win_ptr->Pos, popup_win_ptr->Pos + popup_win_ptr->Size, false);

    bool click_outside_combo = g.IO.MouseClicked[0] && state->opened && !hovered_value && !hovered_popup_win;

    if (click_outside_combo && is_any_picker_open_or_hovered())
        click_outside_combo = false;

    if (pressed) {
        state->opened = !state->opened;
        if (state->opened) {
            if (s_open_combo_id != 0 && s_open_combo_id != id) {
                dropdown_state* prev = static_cast<dropdown_state*>(ImGui::GetStateStorage()->GetVoidPtr(s_open_combo_id));
                if (prev)
                    prev->opened = false;
            }
            s_open_combo_id = id;
            state->need_focus = true;
        } else {
            if (s_open_combo_id == id)
                s_open_combo_id = 0;
        }
    } else if (click_outside_combo) {
        state->opened = false;
        if (s_open_combo_id == id)
            s_open_combo_id = 0;
    }

    state->alpha = ImClamp(state->alpha + Gui->FixedSpeed(14.f) * (state->opened ? 1.f : -1.f), 0.f, 1.f);
    state->hover = ImClamp(state->hover + Gui->FixedSpeed(12.f) * ((hovered_value || state->opened) ? 1.f : -1.f), 0.f, 1.f);

    const float anim_ease = state->alpha * state->alpha * (3.f - 2.f * state->alpha);

    if (state->opened)
        GuiMarkPopupOpen();

    if (font && has_label) {
        ImColor lc = IM_COL32(160, 160, 164, hovered_row ? 255 : 210);
        Draw->TextClipped(window->DrawList, font, label_rect.Min, label_rect.Max, Draw->GetClr(lc), label, nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    ImVec4 bg_a = ImGui::ColorConvertU32ToFloat4(k_dd_box_bg);
    ImVec4 bg_b = ImGui::ColorConvertU32ToFloat4(k_dd_box_bg_hov);
    ImVec4 bg_f(ImLerp(bg_a.x, bg_b.x, state->hover), ImLerp(bg_a.y, bg_b.y, state->hover), ImLerp(bg_a.z, bg_b.z, state->hover), bg_a.w);
    window->DrawList->AddRectFilled(value_rect.Min, value_rect.Max, ImGui::ColorConvertFloat4ToU32(bg_f), 8.f);

    if (font)
        Draw->TextClipped(window->DrawList, font, value_text_rect.Min, value_text_rect.Max, k_dd_text_white, preview_value ? preview_value : "", nullptr, nullptr, ImVec2(0.f, 0.5f));

    if (icon_font) {
        const float fs = icon_font->LegacySize * 0.6f;
        const char* icon_str = state->opened ? ICON_FA_CHEVRON_UP : ICON_FA_CHEVRON_DOWN;
        ImVec2 isz = icon_font->CalcTextSizeA(fs, FLT_MAX, -1.f, icon_str);
        ImVec2 ipos = icon_rect.GetCenter() - isz * 0.5f;
        window->DrawList->AddText(icon_font, fs, ipos, k_dd_text_grey, icon_str);
    }

    if (!IsRectVisible(row_rect.Min, row_rect.Max + ImVec2(0, 2))) {
        state->opened = false;
        state->alpha = 0.f;
        if (s_open_combo_id == id)
            s_open_combo_id = 0;
    }

    if (!state->opened && state->alpha <= 0.01f)
        return false;

    const float item_h = 28.f;
    const float item_gap = 3.f;
    const int max_visible_items = 5;
    const int total_items = static_cast<int>(items.size());
    const int visible_count = ImMax(1, ImMin(total_items, max_visible_items));

    const float full_target_h = pad_y * 2.f + (static_cast<float>(visible_count) * item_h) + (static_cast<float>(visible_count - 1) * item_gap);
    const float current_h = ImMax(1.f, full_target_h * anim_ease);

    const float popup_w = value_rect.GetWidth();
    const ImVec2 popup_pos(value_rect.Min.x, value_rect.Max.y + 4.f);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
    if (total_items <= max_visible_items)
        window_flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, anim_ease);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad_x, pad_y));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, item_gap));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(255, 255, 255, 28));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(255, 255, 255, 45));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(255, 255, 255, 60));

    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popup_w, current_h), ImGuiCond_Always);

    ImGui::Begin(popup_win_name.c_str(), NULL, window_flags);

    ImGuiWindow* cur_popup_win = ImGui::GetCurrentWindow();
    if (cur_popup_win) {
        ImGui::BringWindowToDisplayFront(cur_popup_win);

        if (is_in_about) {
            Gui->about_extra_draw_lists.push_back(cur_popup_win->DrawList);
            Gui->about_extra_windows.push_back(cur_popup_win);
        }

        ImVec2 p_min = cur_popup_win->Pos;
        ImVec2 p_max = p_min + cur_popup_win->Size;

        cur_popup_win->DrawList->PushClipRectFullScreen();
        draw_box_shadow(cur_popup_win->DrawList, p_min, p_max, 12.f, anim_ease);
        cur_popup_win->DrawList->PopClipRect();

        cur_popup_win->DrawList->AddRectFilled(p_min, p_max, k_dd_box_bg, 12.f);
    }

    if (state->need_focus) {
        ImGui::SetWindowFocus();
        state->need_focus = false;
    }

    state->hovered = ImGui::IsWindowHovered();

    return true;
}

void end_combo() {
    ImGui::End();
    ImGui::PopStyleVar(8);
    ImGui::PopStyleColor(7);
}

bool begin_config_combo(const char* label, const char* preview_value, ImVec2 box_min, float box_width, float popup_width, float popup_height, bool* request_close) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    g.NextWindowData.ClearFlags();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float box_h = 26.f;
    const ImVec2 box_max(box_min.x + box_width, box_min.y + box_h);
    const ImRect value_rect(box_min, box_max);
    const ImRect row_rect(box_min, box_max);

    ItemSize(row_rect, style.FramePadding.y);
    if (!ItemAdd(row_rect, id))
        return false;

    const bool hovered_row = ItemHoverable(row_rect, id, g.LastItemData.ItemFlags);

    dropdown_state* state = Gui->AnimContainer(&state, id);

    if (state->last_frame < g.FrameCount - 1 || !Draw->opened) {
        state->opened = false;
        state->alpha = 0.f;
    }

    state->last_frame = g.FrameCount;

    if (request_close && *request_close) {
        state->opened = false;
        *request_close = false;
    }

    bool hovered_value = hovered_row && value_rect.Contains(g.IO.MousePos);
    bool pressed = hovered_value && g.IO.MouseClicked[0];

    std::string popup_win_name = (std::stringstream{} << GetCurrentWindow()->Name << label << "config_dropdown_window").str();
    ImGuiWindow* popup_win_ptr = ImGui::FindWindowByName(popup_win_name.c_str());

    bool hovered_popup_win = false;
    if (popup_win_ptr && (popup_win_ptr->WasActive || state->opened))
        hovered_popup_win = ImGui::IsMouseHoveringRect(popup_win_ptr->Pos, popup_win_ptr->Pos + popup_win_ptr->Size, false);

    bool click_outside_combo = g.IO.MouseClicked[0] && state->opened && !hovered_value && !hovered_popup_win;

    if (pressed) {
        state->opened = !state->opened;
        if (state->opened)
            state->need_focus = true;
    } else if (click_outside_combo) {
        state->opened = false;
    }

    state->alpha = ImClamp(state->alpha + Gui->FixedSpeed(14.f) * (state->opened ? 1.f : -1.f), 0.f, 1.f);
    const float anim_ease = state->alpha * state->alpha * (3.f - 2.f * state->alpha);

    if (state->opened)
        GuiMarkPopupOpen();

    if (!state->opened && state->alpha <= 0.01f)
        return false;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, anim_ease);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.f, 4.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(255, 255, 255, 28));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(255, 255, 255, 45));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(255, 255, 255, 60));

    float current_h = ImMax(1.f, popup_height * anim_ease);
    ImVec2 popup_pos(box_max.x - popup_width, box_max.y + 4.f);
    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popup_width, current_h), ImGuiCond_Always);

    ImGui::Begin(popup_win_name.c_str(), NULL, window_flags);

    ImGuiWindow* cur_popup_win = ImGui::GetCurrentWindow();
    if (cur_popup_win) {
        ImGui::BringWindowToDisplayFront(cur_popup_win);

        ImVec2 p_min = cur_popup_win->Pos;
        ImVec2 p_max = p_min + cur_popup_win->Size;

        cur_popup_win->DrawList->PushClipRectFullScreen();
        draw_box_shadow(cur_popup_win->DrawList, p_min, p_max, 12.f, anim_ease);
        cur_popup_win->DrawList->PopClipRect();

        cur_popup_win->DrawList->AddRectFilled(p_min, p_max, k_dd_box_bg, 12.f);
    }

    if (state->need_focus) {
        ImGui::SetWindowFocus();
        state->need_focus = false;
    }

    state->hovered = ImGui::IsWindowHovered();

    return true;
}

struct combo_item_anim {
    float hover{ 0.f };
    float select{ 0.f };
};

static void render_combo_item(ImGuiWindow* popup_window, const ImVec2& p_min, const ImVec2& p_max, const char* text, bool selected, float hover, float select, float right_reserve) {
    ImFont* pop_font = Var->font.inter[0];
    ImFont* icon_font = Var->font.fa[0];

    if (select > 0.01f)
        popup_window->DrawList->AddRectFilled(p_min, p_max, IM_COL32(48, 50, 56, static_cast<int>(255.f * select)), 8.f);

    if (hover > 0.01f && !selected)
        popup_window->DrawList->AddRectFilled(p_min, p_max, IM_COL32(255, 255, 255, static_cast<int>(10.f * hover)), 8.f);

    ImVec4 grey = ImGui::ColorConvertU32ToFloat4(k_dd_text_grey);
    ImVec4 acc = Colors->accent.Value;
    ImVec4 tc(ImLerp(grey.x, acc.x, select), ImLerp(grey.y, acc.y, select), ImLerp(grey.z, acc.z, select), 1.f);

    if (!selected && hover > 0.01f) {
        tc.x = ImLerp(tc.x, 0.92f, hover * 0.6f);
        tc.y = ImLerp(tc.y, 0.92f, hover * 0.6f);
        tc.z = ImLerp(tc.z, 0.92f, hover * 0.6f);
    }

    const float text_left = p_min.x + 10.f;
    const float base_reserve = selected ? right_reserve : 10.f;

    bool text_overflows = false;
    if (pop_font) {
        ImVec2 text_sz = pop_font->CalcTextSizeA(pop_font->LegacySize, FLT_MAX, -1.f, text);
        float reserved_avail = (p_max.x - base_reserve) - text_left;
        text_overflows = text_sz.x > reserved_avail;
    }

    const float check_hide = (selected && text_overflows) ? hover : 0.f;
    const float check_alpha = select * (1.f - check_hide);
    const float actual_right_reserve = ImLerp(base_reserve, 10.f, check_hide);

    if (pop_font)
        Draw->TextClipped(popup_window->DrawList, pop_font, ImVec2(text_left, p_min.y), ImVec2(p_max.x - actual_right_reserve, p_max.y), ImGui::ColorConvertFloat4ToU32(tc), text, nullptr, nullptr, ImVec2(0.f, 0.5f));

    if (check_alpha > 0.01f && icon_font) {
        const float fs = icon_font->LegacySize * 0.65f;
        ImVec2 isz = icon_font->CalcTextSizeA(fs, FLT_MAX, -1.f, ICON_FA_CHECK);
        ImVec2 ipos(p_max.x - 12.f - isz.x, p_min.y + ((p_max.y - p_min.y) - isz.y) * 0.5f);
        popup_window->DrawList->AddText(icon_font, fs, ipos, Draw->GetClr(Colors->accent, check_alpha), ICON_FA_CHECK);
    }
}

bool combo(const char* label, const char* desc, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items = -1, float right_reserve = 0.f, const char* bind_popup_name = "") {
    ImGuiContext& g = *GImGui;

    auto getter = [](const std::vector<std::string>& vec, int idx) -> const char* { return idx >= 0 && idx < static_cast<int>(vec.size()) ? vec[idx].c_str() : nullptr; };

    const char* preview_value = nullptr;
    if (*current_item >= 0 && *current_item < static_cast<int>(items.size()))
        preview_value = getter(items, *current_item);

    if (!begin_combo(label, preview_value, static_cast<int>(items.size()), items, false, right_reserve, bind_popup_name))
        return false;

    bool value_changed = false;
    ImGuiWindow* popup_window = GetCurrentWindow();

    const float item_h = 28.f;

    for (int i = 0; i < static_cast<int>(items.size()); i++) {
        const char* itetext = getter(items, i);
        if (itetext == nullptr)
            itetext = "*unknown item*";

        PushID(i);
        const bool iteselected = (i == *current_item);

        ImVec2 item_size(GetContentRegionAvail().x, item_h);
        ImVec2 p_min = popup_window->DC.CursorPos;
        ImVec2 p_max = p_min + item_size;
        ImRect item_rect(p_min, p_max);

        ItemSize(item_size);
        if (ItemAdd(item_rect, popup_window->GetID(i))) {
            bool item_hovered = ItemHoverable(item_rect, popup_window->GetID(i), g.LastItemData.ItemFlags);
            bool item_pressed = item_hovered && g.IO.MouseClicked[0];

            combo_item_anim* item_st = Gui->AnimContainer(&item_st, popup_window->GetID(i));
            item_st->hover = ImClamp(item_st->hover + Gui->FixedSpeed(14.f) * (item_hovered ? 1.f : -1.f), 0.f, 1.f);
            item_st->select = ImClamp(item_st->select + Gui->FixedSpeed(14.f) * (iteselected ? 1.f : -1.f), 0.f, 1.f);

            render_combo_item(popup_window, p_min, p_max, itetext, iteselected, item_st->hover, item_st->select, 28.f);

            if (item_pressed && *current_item != i) {
                value_changed = true;
                *current_item = i;
            }
        }

        PopID();
    }

    end_combo();

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool CGui::Dropdown(std::string_view label, int* current_item, const std::vector<std::string>& items) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label.data());
    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    const bool value_changed = combo(label.data(), nullptr, current_item, items, -1, 0.f, bind_popup_name);
    Binds::RenderDropdownPopup(bind_popup_name, label, current_item, items);
    return value_changed;
}

bool CGui::DropdownNew(const char* label, int* current_item, const char* const items[], int items_count) {
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiID id = window->GetID(label);

    std::vector<std::string> items_vec;
    items_vec.reserve(items_count);
    for (int i = 0; i < items_count; ++i) {
        if (items[i] != nullptr)
            items_vec.emplace_back(items[i]);
    }

    ImVec2 pos_before = window->DC.CursorPos;
    float full_width = GetContentRegionAvail().x;
    float row_height = 26.f;

    ImRect element_rect(pos_before, ImVec2(pos_before.x + full_width, pos_before.y + row_height));

    bool result = this->Dropdown(label, current_item, items_vec);

    ImGuiIO& io = ImGui::GetIO();
    bool mouse_over_element = ImGui::IsMouseHoveringRect(element_rect.Min, element_rect.Max) && ImGui::GetCurrentWindow() == window;

    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##dropdown_binds_%p", (void*)current_item);

    if (mouse_over_element && io.MouseClicked[1])
        ImGui::OpenPopup(bind_popup_name);

    return result;
}

void CGui::MultiDropdown(std::string_view label, std::vector<bool>* variable, const std::vector<std::string>& items, std::function<void()> gear_fn) {
    if (!variable)
        return;

    if (variable->size() < items.size())
        variable->resize(items.size(), false);

    std::string preview = "";
    int count = 0;

    for (size_t i = 0; i < items.size(); i++) {
        if (variable->at(i)) {
            if (count == 0)
                preview = items[i];
            else if (count == 1)
                preview += ", " + items[i];
            count++;
        }
    }

    if (count == 0)
        preview = xs("None");
    else if (count > 2)
        preview = std::to_string(count) + xs(" selected");

    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiID id = window->GetID(label.data());

    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    const ImVec2 row_pos = window->DC.CursorPos;
    const float row_avail = GetContentRegionAvail().x;
    const float reserve = gear_fn ? 24.f : 0.f;

    if (begin_combo(label.data(), preview.c_str(), static_cast<int>(items.size()), items, true, reserve, bind_popup_name, true)) {
        ImGuiWindow* popup_window = GetCurrentWindow();
        ImGuiContext& g = *GImGui;

        const float item_h = 28.f;

        for (size_t i = 0; i < items.size(); ++i) {
            PushID(static_cast<int>(i));
            bool selected = variable->at(i);

            ImVec2 item_size(GetContentRegionAvail().x, item_h);
            ImVec2 p_min = popup_window->DC.CursorPos;
            ImVec2 p_max = p_min + item_size;
            ImRect item_rect(p_min, p_max);

            ItemSize(item_size);
            if (ItemAdd(item_rect, popup_window->GetID(static_cast<int>(i)))) {
                bool item_hovered = ItemHoverable(item_rect, popup_window->GetID(static_cast<int>(i)), g.LastItemData.ItemFlags);
                bool item_pressed = item_hovered && g.IO.MouseClicked[0];

                combo_item_anim* item_st = Gui->AnimContainer(&item_st, popup_window->GetID(static_cast<int>(i)));
                item_st->hover = ImClamp(item_st->hover + Gui->FixedSpeed(14.f) * (item_hovered ? 1.f : -1.f), 0.f, 1.f);
                item_st->select = ImClamp(item_st->select + Gui->FixedSpeed(14.f) * (selected ? 1.f : -1.f), 0.f, 1.f);

                render_combo_item(popup_window, p_min, p_max, items[i].c_str(), selected, item_st->hover, item_st->select, 28.f);

                if (item_pressed)
                    variable->at(i) = !selected;
            }

            PopID();
        }

        end_combo();
    }

    if (gear_fn) {
        ImRect rect(ImVec2(row_pos.x + row_avail - 18.f, row_pos.y),
            ImVec2(row_pos.x + row_avail, row_pos.y + 26.f));
        RenderGearPopupFn(window->DrawList, rect, label.data(), gear_fn);
    }

    Binds::RenderMultiDropdownPopup(bind_popup_name, label, variable, items);
}

void CGui::MultiDropdown(std::string_view label, std::vector<int>* variable, const std::vector<std::string>& items) {
    if (!variable)
        return;

    if (variable->size() < items.size())
        variable->resize(items.size(), 0);

    std::string preview = "";
    int count = 0;

    for (size_t i = 0; i < items.size(); i++) {
        if (variable->at(i) != 0) {
            if (count == 0)
                preview = items[i];
            else if (count == 1)
                preview += ", " + items[i];
            count++;
        }
    }

    if (count == 0)
        preview = xs("None");
    else if (count > 2)
        preview = std::to_string(count) + xs(" selected");

    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiID id = window->GetID(label.data());

    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    if (begin_combo(label.data(), preview.c_str(), static_cast<int>(items.size()), items, true, 0.f, bind_popup_name, true)) {
        ImGuiWindow* popup_window = GetCurrentWindow();
        ImGuiContext& g = *GImGui;

        const float item_h = 28.f;

        for (size_t i = 0; i < items.size(); ++i) {
            PushID(static_cast<int>(i));
            bool selected = (variable->at(i) != 0);

            ImVec2 item_size(GetContentRegionAvail().x, item_h);
            ImVec2 p_min = popup_window->DC.CursorPos;
            ImVec2 p_max = p_min + item_size;
            ImRect item_rect(p_min, p_max);

            ItemSize(item_size);
            if (ItemAdd(item_rect, popup_window->GetID(static_cast<int>(i)))) {
                bool item_hovered = ItemHoverable(item_rect, popup_window->GetID(static_cast<int>(i)), g.LastItemData.ItemFlags);
                bool item_pressed = item_hovered && g.IO.MouseClicked[0];

                combo_item_anim* item_st = Gui->AnimContainer(&item_st, popup_window->GetID(static_cast<int>(i)));
                item_st->hover = ImClamp(item_st->hover + Gui->FixedSpeed(14.f) * (item_hovered ? 1.f : -1.f), 0.f, 1.f);
                item_st->select = ImClamp(item_st->select + Gui->FixedSpeed(14.f) * (selected ? 1.f : -1.f), 0.f, 1.f);

                render_combo_item(popup_window, p_min, p_max, items[i].c_str(), selected, item_st->hover, item_st->select, 28.f);

                if (item_pressed)
                    variable->at(i) = selected ? 0 : 1;
            }

            PopID();
        }

        end_combo();
    }
}

bool CGui::DropdownEx(std::string_view label, int* current_item, const std::vector<std::string>& items, std::function<void()> gear_fn) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label.data());
    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    const float row_h = 26.f;
    const float gear_sz = 18.f;
    const float gear_gap = 6.f;
    const float right_res = gear_fn ? (gear_sz + gear_gap) : 0.f;

    const ImVec2 row_pos = window->DC.CursorPos;
    const float row_avail = GetContentRegionAvail().x;

    const bool value_changed = combo(label.data(), nullptr, current_item, items, -1, right_res, bind_popup_name);

    if (gear_fn) {
        ImDrawList* dl = window->DrawList;
        const float gx = row_pos.x + row_avail - gear_sz;
        ImRect gear_bb(ImVec2(gx, row_pos.y), ImVec2(gx + gear_sz, row_pos.y + row_h));
        RenderGearPopupFn(dl, gear_bb, label.data(), gear_fn);
    }

    Binds::RenderDropdownPopup(bind_popup_name, label, current_item, items);
    return value_changed;
}

bool CGui::CheckboxWithDropdown(std::string_view label, bool* callback, int* current_item, const std::vector<std::string>& items, bool insecure, std::function<void()> gear_fn) {
    struct CwdState {
        float fill{ 0.f };
        float hover{ 0.f };
        float dd_alpha{ 0.f };
        float dd_hover{ 0.f };
        bool dd_opened{ false };
        bool dd_hovered{ false };
        bool dd_need_focus{ false };
        int last_frame{ 0 };
    };

    if (!callback || !current_item || items.empty())
        return false;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());
    const ImGuiID dd_id = window->GetID((std::string(label.data()) + xs("##cwd_dd")).c_str());

    const ImVec2 pos = window->DC.CursorPos;
    const float avail = GetContentRegionAvail().x;

    const float row_h = 26.f;
    const float box_sz = 18.f;
    const float box_r = 5.f;
    const float gear_sz = 18.f;
    const float gear_gap = 6.f;

    const float box_x = pos.x + avail - box_sz;
    const float box_y = pos.y + (row_h - box_sz) * 0.5f;
    const ImRect box_bb(ImVec2(box_x, box_y), ImVec2(box_x + box_sz, box_y + box_sz));

    float gear_anchor = box_x;
    ImRect gear_bb(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    if (gear_fn) {
        gear_anchor = box_x - gear_gap - gear_sz;
        gear_bb = ImRect(ImVec2(gear_anchor, pos.y + (row_h - gear_sz) * 0.5f), ImVec2(gear_anchor + gear_sz, pos.y + (row_h + gear_sz) * 0.5f));
    }

    ImFont* font_regular = Var->font.inter[0];
    ImFont* icon_font = Var->font.fa[0];

    float max_text_w = 0.f;
    if (font_regular) {
        for (const auto& it : items) {
            ImVec2 sz = font_regular->CalcTextSizeA(font_regular->LegacySize, FLT_MAX, -1.f, it.c_str());
            max_text_w = ImMax(max_text_w, sz.x);
        }
    }

    const float pad_x = 7.f;
    const float item_left_pad = 10.f;
    const float item_right_reserve = 30.f;
    const float needed_w = max_text_w + item_left_pad + item_right_reserve + pad_x * 2.f;
    const float default_min_w = ImMin(135.f, avail * 0.40f);
    const float dd_w = ImClamp(needed_w, default_min_w, ImMin(avail * 0.45f, 175.f));

    const float dd_right_limit = gear_fn ? (gear_anchor - gear_gap) : (box_x - 6.f);
    const float dd_x = dd_right_limit - dd_w;
    const ImRect dd_rect(ImVec2(dd_x, pos.y + (row_h - 26.f) * 0.5f), ImVec2(dd_x + dd_w, pos.y + (row_h + 26.f) * 0.5f));
    const float label_rx = dd_x - 8.f;

    const ImRect row_rect(pos, ImVec2(pos.x + avail, pos.y + row_h));

    ItemSize(row_rect, style.FramePadding.y);
    if (!ItemAdd(row_rect, id))
        return false;

    const bool row_hov = ItemHoverable(row_rect, id, g.LastItemData.ItemFlags);
    const bool dd_hov_r = row_hov && dd_rect.Contains(io.MousePos);
    const bool gear_hov_r = gear_fn && row_hov && gear_bb.Contains(io.MousePos);
    const bool cb_hov = row_hov && !dd_hov_r && !gear_hov_r;

    bool pressed = false;
    if (cb_hov && io.MouseClicked[0]) {
        pressed = true;
        *callback = !(*callback);
        MarkItemEdited(id);
    }

    char bind_popup_name[64];
    sprintf_s(bind_popup_name, xs("##binds_popup_%u"), id);
    if (row_hov && io.MouseClicked[1])
        ImGui::OpenPopup(bind_popup_name);

    CwdState* state = Gui->AnimContainer(&state, id);

    if (state->last_frame < g.FrameCount - 1 || !Draw->opened) {
        state->dd_opened = false;
        state->dd_alpha = 0.f;
        if (s_open_combo_id == dd_id)
            s_open_combo_id = 0;
    }
    state->last_frame = g.FrameCount;

    if (state->dd_opened && s_open_combo_id != dd_id)
        state->dd_opened = false;
    
    std::string popup_win_name = (std::stringstream{} << window->Name << label << xs("##cwd_dd_popup_win")).str();
    ImGuiWindow* popup_win_ptr = ImGui::FindWindowByName(popup_win_name.c_str());

    bool hovered_popup_win = false;
    if (popup_win_ptr && (popup_win_ptr->WasActive || state->dd_opened))
        hovered_popup_win = ImGui::IsMouseHoveringRect(popup_win_ptr->Pos, popup_win_ptr->Pos + popup_win_ptr->Size, false);

    bool click_outside_combo = io.MouseClicked[0] && state->dd_opened && !dd_hov_r && !hovered_popup_win;
    if (click_outside_combo && is_any_picker_open_or_hovered())
        click_outside_combo = false;

    if (dd_hov_r && io.MouseClicked[0]) {
        io.MouseClicked[0] = false;
        state->dd_opened = !state->dd_opened;
        if (state->dd_opened) {
            if (s_open_combo_id != 0 && s_open_combo_id != dd_id) {
                dropdown_state* prev = static_cast<dropdown_state*>(ImGui::GetStateStorage()->GetVoidPtr(s_open_combo_id));
                if (prev)
                    prev->opened = false;
            }
            s_open_combo_id = dd_id;
            state->dd_need_focus = true;
        } else {
            if (s_open_combo_id == dd_id)
                s_open_combo_id = 0;
        }
    } else if (click_outside_combo) {
        state->dd_opened = false;
        if (s_open_combo_id == dd_id)
            s_open_combo_id = 0;
    }

    state->fill = ImClamp(state->fill + Gui->FixedSpeed(14.f) * (*callback ? 1.f : -1.f), 0.f, 1.f);
    state->hover = ImClamp(state->hover + Gui->FixedSpeed(12.f) * (cb_hov ? 1.f : -1.f), 0.f, 1.f);
    state->dd_alpha = ImClamp(state->dd_alpha + Gui->FixedSpeed(14.f) * (state->dd_opened ? 1.f : -1.f), 0.f, 1.f);
    state->dd_hover = ImClamp(state->dd_hover + Gui->FixedSpeed(12.f) * ((dd_hov_r || state->dd_opened) ? 1.f : -1.f), 0.f, 1.f);

    const float anim_ease = state->dd_alpha * state->dd_alpha * (3.f - 2.f * state->dd_alpha);

    if (state->dd_opened)
        GuiMarkPopupOpen();

    ImDrawList* dl = window->DrawList;

    ImVec4 bg_off = Colors->button.background.Value;
    ImVec4 bg_on = Colors->accent.Value;
    ImVec4 bg_col(ImLerp(bg_off.x, bg_on.x, state->fill), ImLerp(bg_off.y, bg_on.y, state->fill), ImLerp(bg_off.z, bg_on.z, state->fill), ImLerp(bg_off.w, 1.f, state->fill));

    if (state->hover > 0.01f && state->fill < 0.99f) {
        bg_col.x = ImMin(bg_col.x + 0.03f * state->hover, 1.f);
        bg_col.y = ImMin(bg_col.y + 0.03f * state->hover, 1.f);
        bg_col.z = ImMin(bg_col.z + 0.03f * state->hover, 1.f);
    }

    dl->AddRectFilled(box_bb.Min, box_bb.Max, Draw->GetClr(ImColor(bg_col)), box_r);

    if (state->fill < 0.99f) {
        float b_alpha = ImLerp(0.04f, 0.10f, state->hover);
        dl->AddRect(box_bb.Min, box_bb.Max, Draw->GetClr(Colors->divider, b_alpha), box_r, 0, 1.f);
    }

    if (state->fill > 0.005f) {
        const float lum = 0.299f * bg_on.x + 0.587f * bg_on.y + 0.114f * bg_on.z;
        const ImU32 ck_base_col = lum > 0.45f ? IM_COL32(18, 20, 26, 255) : IM_COL32(255, 255, 255, 255);
        const ImU32 ck_col = Draw->GetClr(ImColor(ck_base_col), state->fill);
        RenderCheckmark(dl, box_bb.GetCenter(), state->fill, ck_col);
    }

    if (gear_fn) {
        std::string gear_uid = std::string(label.data()) + xs("_cwd_gear");
        RenderGearPopupFn(dl, gear_bb, gear_uid.c_str(), gear_fn);
    }

    if (font_regular) {
        const float text_alpha = ImLerp(0.55f, 1.0f, ImMax(state->fill, state->hover * 0.5f));
        ImColor tc = Colors->checkbox.text;
        tc.Value.w = text_alpha;
        Draw->TextClipped(dl, font_regular, ImVec2(pos.x, pos.y), ImVec2(label_rx, pos.y + row_h), Draw->GetClr(tc), label.data(), nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    ImVec4 dd_bg_a = ImGui::ColorConvertU32ToFloat4(k_dd_box_bg);
    ImVec4 dd_bg_b = ImGui::ColorConvertU32ToFloat4(k_dd_box_bg_hov);
    ImVec4 dd_bg_f(ImLerp(dd_bg_a.x, dd_bg_b.x, state->dd_hover), ImLerp(dd_bg_a.y, dd_bg_b.y, state->dd_hover), ImLerp(dd_bg_a.z, dd_bg_b.z, state->dd_hover), dd_bg_a.w);
    dl->AddRectFilled(dd_rect.Min, dd_rect.Max, ImGui::ColorConvertFloat4ToU32(dd_bg_f), 8.f);

    const char* preview_value = (*current_item >= 0 && *current_item < static_cast<int>(items.size())) ? items[*current_item].c_str() : "";

    const float text_pad = 10.f;
    const float icon_pad = 10.f;
    const float icon_w = 10.f;

    const ImRect value_text_rect(ImVec2(dd_rect.Min.x + text_pad, dd_rect.Min.y), ImVec2(dd_rect.Max.x - icon_pad - icon_w, dd_rect.Max.y));
    const ImRect icon_rect(ImVec2(dd_rect.Max.x - icon_pad - icon_w, dd_rect.Min.y), ImVec2(dd_rect.Max.x - icon_pad, dd_rect.Max.y));

    if (font_regular)
        Draw->TextClipped(dl, font_regular, value_text_rect.Min, value_text_rect.Max, k_dd_text_white, preview_value, nullptr, nullptr, ImVec2(0.f, 0.5f));

    if (icon_font) {
        const float fs = icon_font->LegacySize * 0.6f;
        const char* icon_str = state->dd_opened ? ICON_FA_CHEVRON_UP : ICON_FA_CHEVRON_DOWN;
        ImVec2 isz = icon_font->CalcTextSizeA(fs, FLT_MAX, -1.f, icon_str);
        ImVec2 ipos = icon_rect.GetCenter() - isz * 0.5f;
        dl->AddText(icon_font, fs, ipos, k_dd_text_grey, icon_str);
    }

    if (!IsRectVisible(row_rect.Min, row_rect.Max + ImVec2(0, 2))) {
        state->dd_opened = false;
        state->dd_alpha = 0.f;
        if (s_open_combo_id == dd_id)
            s_open_combo_id = 0;
    }

    if (state->dd_opened || state->dd_alpha > 0.01f) {
        const float item_h = 28.f;
        const float item_gap = 3.f;
        const int max_visible_items = 5;
        const int total_items = static_cast<int>(items.size());
        const int visible_count = ImMax(1, ImMin(total_items, max_visible_items));
        const float pad_y = 7.f;

        const float full_target_h = pad_y * 2.f + (static_cast<float>(visible_count) * item_h) + (static_cast<float>(visible_count - 1) * item_gap);
        const float current_h = ImMax(1.f, full_target_h * anim_ease);

        const float popup_w = dd_rect.GetWidth();
        const ImVec2 popup_pos(dd_rect.Min.x, dd_rect.Max.y + 4.f);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
        if (total_items <= max_visible_items)
            window_flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, anim_ease);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad_x, pad_y));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, item_gap));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(255, 255, 255, 28));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(255, 255, 255, 45));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(255, 255, 255, 60));

        ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(popup_w, current_h), ImGuiCond_Always);

        if (ImGui::Begin(popup_win_name.c_str(), NULL, window_flags)) {
            ImGuiWindow* cur_popup_win = ImGui::GetCurrentWindow();
            if (cur_popup_win) {
                ImGui::BringWindowToDisplayFront(cur_popup_win);

                const bool is_in_about = (Gui->about_active && Gui->about_draw_list && window->DrawList == Gui->about_draw_list);
                if (is_in_about) {
                    Gui->about_extra_draw_lists.push_back(cur_popup_win->DrawList);
                    Gui->about_extra_windows.push_back(cur_popup_win);
                }

                ImVec2 p_min = cur_popup_win->Pos;
                ImVec2 p_max = p_min + cur_popup_win->Size;

                cur_popup_win->DrawList->PushClipRectFullScreen();
                draw_box_shadow(cur_popup_win->DrawList, p_min, p_max, 12.f, anim_ease);
                cur_popup_win->DrawList->PopClipRect();

                cur_popup_win->DrawList->AddRectFilled(p_min, p_max, k_dd_box_bg, 12.f);
            }

            if (state->dd_need_focus) {
                ImGui::SetWindowFocus();
                state->dd_need_focus = false;
            }

            state->dd_hovered = ImGui::IsWindowHovered();

            for (int i = 0; i < total_items; i++) {
                PushID(i);
                const bool iteselected = (i == *current_item);

                ImVec2 item_size(GetContentRegionAvail().x, item_h);
                ImVec2 p_min = cur_popup_win->DC.CursorPos;
                ImVec2 p_max = p_min + item_size;
                ImRect item_rect(p_min, p_max);

                ItemSize(item_size);
                if (ItemAdd(item_rect, cur_popup_win->GetID(i))) {
                    bool item_hovered = ItemHoverable(item_rect, cur_popup_win->GetID(i), g.LastItemData.ItemFlags);
                    bool item_pressed = item_hovered && io.MouseClicked[0];

                    combo_item_anim* item_st = Gui->AnimContainer(&item_st, cur_popup_win->GetID(i));
                    item_st->hover = ImClamp(item_st->hover + Gui->FixedSpeed(14.f) * (item_hovered ? 1.f : -1.f), 0.f, 1.f);
                    item_st->select = ImClamp(item_st->select + Gui->FixedSpeed(14.f) * (iteselected ? 1.f : -1.f), 0.f, 1.f);

                    render_combo_item(cur_popup_win, p_min, p_max, items[i].c_str(), iteselected, item_st->hover, item_st->select, 28.f);

                    if (item_pressed && *current_item != i) {
                        *current_item = i;
                        state->dd_opened = false;
                        if (s_open_combo_id == dd_id)
                            s_open_combo_id = 0;
                        MarkItemEdited(id);
                    }
                }

                PopID();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(8);
        ImGui::PopStyleColor(7);
    }

    Binds::RenderCheckboxPopup(bind_popup_name, label, callback);

    return pressed;
}