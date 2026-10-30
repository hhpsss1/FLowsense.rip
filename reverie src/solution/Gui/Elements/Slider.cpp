#include "../Settings/Functions.h"
#include "../Data/FontsAwesome.h"

static float SliderValueRatio(ImGuiDataType data_type, const void* p_data, const void* p_min, const void* p_max) {
    auto calc = [](double v, double mn, double mx) -> float {
        if (mx == mn)
            return 0.f;
        return ImSaturate((float)((v - mn) / (mx - mn)));
    };

    switch (data_type) {
    case ImGuiDataType_S8: return calc(*(const ImS8*)p_data, *(const ImS8*)p_min, *(const ImS8*)p_max);
    case ImGuiDataType_U8: return calc(*(const ImU8*)p_data, *(const ImU8*)p_min, *(const ImU8*)p_max);
    case ImGuiDataType_S16: return calc(*(const ImS16*)p_data, *(const ImS16*)p_min, *(const ImS16*)p_max);
    case ImGuiDataType_U16: return calc(*(const ImU16*)p_data, *(const ImU16*)p_min, *(const ImU16*)p_max);
    case ImGuiDataType_S32: return calc(*(const ImS32*)p_data, *(const ImS32*)p_min, *(const ImS32*)p_max);
    case ImGuiDataType_U32: return calc(*(const ImU32*)p_data, *(const ImU32*)p_min, *(const ImU32*)p_max);
    case ImGuiDataType_S64: return calc((double)*(const ImS64*)p_data, (double)*(const ImS64*)p_min, (double)*(const ImS64*)p_max);
    case ImGuiDataType_U64: return calc((double)*(const ImU64*)p_data, (double)*(const ImU64*)p_min, (double)*(const ImU64*)p_max);
    case ImGuiDataType_Float: return calc(*(const float*)p_data, *(const float*)p_min, *(const float*)p_max);
    case ImGuiDataType_Double: return calc(*(const double*)p_data, *(const double*)p_min, *(const double*)p_max);
    default: return 0.5f;
    }
}

static bool ApplySliderStep(ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, float steps) {
    if (steps == 0.f)
        return false;

    if (data_type == ImGuiDataType_S32) {
        int* v = (int*)p_data;
        int mn = *(const int*)p_min;
        int mx = *(const int*)p_max;
        int delta = (int)(steps > 0.f ? ceilf(steps) : floorf(steps));
        int old_v = *v;
        *v = ImClamp(*v + delta, mn, mx);
        return *v != old_v;
    } else if (data_type == ImGuiDataType_Float) {
        float* v = (float*)p_data;
        float mn = *(const float*)p_min;
        float mx = *(const float*)p_max;
        float range = mx - mn;
        float step_size = (range > 0.f) ? (range * 0.01f) : 0.1f;
        float old_v = *v;
        *v = ImClamp(*v + step_size * steps, mn, mx);
        return *v != old_v;
    }

    return false;
}

static bool ApplyMouseStep(ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, int direction) {
    if (direction == 0)
        return false;

    if (data_type == ImGuiDataType_S32) {
        int* v = (int*)p_data;
        int mn = *(const int*)p_min;
        int mx = *(const int*)p_max;
        int old_v = *v;
        *v = ImClamp(*v + direction, mn, mx);
        return *v != old_v;
    } else if (data_type == ImGuiDataType_Float) {
        float* v = (float*)p_data;
        float mn = *(const float*)p_min;
        float mx = *(const float*)p_max;
        float old_v = *v;
        *v = ImClamp(*v + (float)direction, mn, mx);
        return *v != old_v;
    }

    return false;
}

bool SliderBehavior(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb) {
    return ImGui::SliderBehavior(bb, id, data_type, p_v, p_min, p_max, format, flags, out_grab_bb);
}

namespace slider_cfg {
    constexpr float row_h = 30.f;
    constexpr float label_frac = 0.46f;
    constexpr float track_h = 4.f;
    constexpr float knob_r = 5.f;
    constexpr float knob_r_hover = 6.5f;
    constexpr float gear_sz = 16.f;
    constexpr float gear_pad = 14.f;
    constexpr float bubble_h = 20.f;
    constexpr float bubble_gap = 7.f;
    constexpr float show_after_change = 0.9f;
}

struct SliderState {
    float hover{ 0.f };
    float active{ 0.f };
    float bubble{ 0.f };
    float edit_alpha{ 0.f };
    float last_change{ -100.f };
    bool editing{ false };
    char edit_buf[64]{};
};

bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, bool has_gear, const char* bind_popup_name, ImRect* out_gear_bb = nullptr) {
    using namespace slider_cfg;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    if (format == NULL)
        format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

    char value_buf[64];
    ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    const char* value_buf_end = value_buf + strlen(value_buf);

    ImFont* font = Var->font.inter[5] ? Var->font.inter[5] : Var->font.inter[2];

    float avail = ImGui::GetContentRegionAvail().x;
    if (window->Flags & ImGuiWindowFlags_Popup)
        avail = 220.f;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, ImVec2(pos.x + avail, pos.y + row_h));

    const float label_w  = ImFloor(avail * label_frac);
    const float track_y  = ImFloor(pos.y + row_h * 0.5f);
    const float track_x0 = pos.x + label_w + knob_r_hover;
    const float track_x1 = pos.x + avail - knob_r_hover - (has_gear ? (gear_sz + gear_pad) : 0.f);
    const float track_w  = ImMax(track_x1 - track_x0, 1.f);

    const ImRect slider_bb(ImVec2(track_x0 - 2.f, track_y - 9.f), ImVec2(track_x1 + 2.f, track_y + 9.f));
    const ImRect hover_bb(ImVec2(track_x0 - knob_r_hover - 2.f, track_y - 10.f), ImVec2(track_x1 + knob_r_hover + 2.f, track_y + 10.f));

    ImGui::ItemSize(rect, style.FramePadding.y);
    const ImVec2 next_item_cursor_pos = window->DC.CursorPos;

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    if (!ImGui::ItemAdd(rect, id, &slider_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0)) {
        window->DC.CursorPos = next_item_cursor_pos;
        return false;
    }

    SliderState* state = Gui->AnimContainer(&state, id);

    const bool hovered_row = ImGui::IsMouseHoveringRect(rect.Min, rect.Max) && g.HoveredWindow == window;
    const bool hovered = ImGui::ItemHoverable(hover_bb, id, g.LastItemData.ItemFlags);
    const bool is_active = (g.ActiveId == id);

    bool value_changed = false;
    bool just_opened_edit = false;

    if (!state->editing) {
        const bool want_edit = (hovered && g.IO.MouseClicked[1]) || (hovered && g.IO.KeyCtrl && g.IO.MouseClicked[0]);
        if (want_edit) {
            strncpy_s(state->edit_buf, value_buf, sizeof(state->edit_buf) - 1);
            state->editing = true;
            just_opened_edit = true;
            g.IO.MouseClicked[0] = false;
            g.IO.MouseClicked[1] = false;
        } else if (hovered_row && g.IO.MouseClicked[1] && bind_popup_name) {
            ImGui::OpenPopup(bind_popup_name);
        }
    }

    ImRect grab_bb;
    if (!state->editing) {
        const bool clicked = hovered && ImGui::IsMouseClicked(0, 0, id);
        if (clicked) {
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 0.0f);
        value_changed = ImGui::SliderBehavior(slider_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
        ImGui::PopStyleVar();

        if (hovered && !is_active && g.IO.MouseWheel != 0.f) {
            if (ApplyMouseStep(data_type, p_data, p_min, p_max, (int)g.IO.MouseWheel))
                value_changed = true;
        }

        if (is_active || (hovered && g.ActiveId == 0)) {
            float step_dir = 0.f;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
                step_dir -= 1.f;

            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
                step_dir += 1.f;

            if (step_dir != 0.f && ApplySliderStep(data_type, p_data, p_min, p_max, step_dir))
                value_changed = true;
        }

        if (value_changed) {
            ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
            value_buf_end = value_buf + strlen(value_buf);
            ImGui::MarkItemEdited(id);
            state->last_change = (float)g.Time;
        }
    }

    const bool recently_changed = ((float)g.Time - state->last_change) < show_after_change;
    const bool show_bubble = !state->editing && (hovered || is_active || recently_changed);

    state->hover = ImClamp(state->hover + Gui->FixedSpeed(14.f) * (hovered ? 1.f : -1.f), 0.f, 1.f);
    state->active = ImClamp(state->active + Gui->FixedSpeed(14.f) * (is_active ? 1.f : -1.f), 0.f, 1.f);
    state->bubble = ImClamp(state->bubble + Gui->FixedSpeed(12.f) * (show_bubble ? 1.f : -1.f), 0.f, 1.f);
    state->edit_alpha = ImClamp(state->edit_alpha + Gui->FixedSpeed(14.f) * (state->editing ? 1.f : -1.f), 0.f, 1.f);

    ImDrawList* dl = window->DrawList;
    const float glow_t = ImMax(state->hover, state->active);

    if (font && label && label[0]) {
        ImVec4 tc = Colors->slider.text.Value;
        tc.w = 0.65f + glow_t * 0.35f;
        Draw->TextClipped(dl, font, ImVec2(pos.x, pos.y), ImVec2(pos.x + label_w - 8.f, pos.y + row_h), Draw->GetClr(ImColor(tc)), label, nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    const float value_ratio = SliderValueRatio(data_type, p_data, p_min, p_max);
    const float knob_x = track_x0 + value_ratio * track_w;

    const ImVec2 track_min(track_x0, track_y - track_h * 0.5f);
    const ImVec2 track_max(track_x1, track_y + track_h * 0.5f);
    const float  track_rnd = track_h * 0.5f;

    {
        const float track_alpha = 1.f - state->edit_alpha * 0.85f;
        Draw->RectFilled(dl, track_min, track_max, Draw->GetClr(Colors->slider.background, track_alpha), track_rnd);

        if (knob_x - track_x0 > 0.5f) {
            const ImVec2 fill_max(knob_x, track_max.y);

            if (glow_t > 0.001f) {
                ImVec4 gc = Colors->accent.Value;
                for (int i = 1; i <= 3; ++i) {
                    float off = (float)i * 1.5f;
                    gc.w = (0.20f / (float)i) * glow_t * track_alpha;
                    dl->AddRect(ImVec2(track_min.x - off, track_min.y - off), ImVec2(fill_max.x + off, fill_max.y + off), Draw->GetClr(ImColor(gc)), track_rnd + off, 0, 1.f);
                }
            }

            Draw->RectFilled(dl, track_min, fill_max, Draw->GetClr(Colors->accent, track_alpha), track_rnd);
        }

        const float kr = ImLerp(knob_r, knob_r_hover, glow_t);
        const ImVec2 kc(knob_x, track_y);

        dl->AddCircleFilled(ImVec2(kc.x, kc.y + 1.f), kr + 1.5f, IM_COL32(0, 0, 0, (int)(70.f * track_alpha)), 24);
        if (glow_t > 0.001f)
            dl->AddCircleFilled(kc, kr + 3.f, Draw->GetClr(Colors->accent, 0.25f * glow_t * track_alpha), 24);

        dl->AddCircleFilled(kc, kr, IM_COL32(236, 236, 240, (int)(255.f * track_alpha)), 24);
    }

    if (font && state->editing) {
        ImVec2 backup_cursor_pos = window->DC.CursorPos;

        const float edit_h = 22.f;
        const ImVec2 edit_min(track_x0 - knob_r_hover, track_y - edit_h * 0.5f);
        const ImVec2 edit_max(track_x1 + knob_r_hover, track_y + edit_h * 0.5f);

        Draw->RectFilled(dl, edit_min, edit_max, Draw->GetClr(Colors->slider.background, state->edit_alpha), 5.f);
        dl->AddRect(edit_min, edit_max, Draw->GetClr(Colors->accent, 0.55f * state->edit_alpha), 5.f, 0, 1.f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, Draw->GetClr(Colors->accent, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_Text, Colors->slider.value.Value);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

        ImGui::PushFont(font);

        const float edit_w = edit_max.x - edit_min.x;
        ImGui::SetCursorScreenPos(ImVec2(edit_min.x, track_y - font->LegacySize * 0.5f));
        ImGui::SetNextItemWidth(edit_w);

        ImGuiInputTextFlags itf = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CharsDecimal;

        ImGui::PushID(id);
        if (just_opened_edit)
            ImGui::SetKeyboardFocusHere();

        bool confirmed = ImGui::InputText("##sv_edit_inline", state->edit_buf, IM_ARRAYSIZE(state->edit_buf), itf);
        bool escaped = ImGui::IsKeyPressed(ImGuiKey_Escape);
        bool clicked_outside = !just_opened_edit && !ImGui::IsItemActive() && g.IO.MouseClicked[0] && !ImGui::IsMouseHoveringRect(edit_min, edit_max);
        ImGui::PopID();

        ImGui::PopFont();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(6);

        window->DC.CursorPos = backup_cursor_pos;

        if (confirmed) {
            char* end_ptr = nullptr;
            double parsed = strtod(state->edit_buf, &end_ptr);

            if (end_ptr != state->edit_buf) {
                if (data_type == ImGuiDataType_S32) {
                    int imin = *(const int*)p_min;
                    int imax = *(const int*)p_max;
                    *(int*)p_data = (int)ImClamp(parsed, (double)imin, (double)imax);
                } else {
                    float fmin = *(const float*)p_min;
                    float fmax = *(const float*)p_max;
                    *(float*)p_data = ImClamp((float)parsed, fmin, fmax);
                }

                value_changed = true;
                ImGui::MarkItemEdited(id);
                state->last_change = (float)g.Time;
            }
            state->editing = false;
        }

        if (escaped || clicked_outside)
            state->editing = false;
    }

    if (font && state->bubble > 0.01f) {
        const float a = state->bubble;
        const float ease = 1.f - (1.f - a) * (1.f - a);

        ImVec2 ts = font->CalcTextSizeA(font->LegacySize, FLT_MAX, -1.f, value_buf, value_buf_end);
        const float bw = ImMax(ts.x + 16.f, 30.f);
        const float bh = bubble_h;

        const float slide = (1.f - ease) * 4.f;
        const float y1 = track_y - knob_r_hover - bubble_gap + slide;
        const float y0 = y1 - bh;
        float x0 = ImFloor(knob_x - bw * 0.5f);
        float x1 = x0 + bw;

        if (x0 < pos.x) {
            x0 = pos.x;
            x1 = x0 + bw;
        }

        if (x1 > pos.x + avail) {
            x1 = pos.x + avail;
            x0 = x1 - bw;
        }

        dl->PushClipRectFullScreen();

        dl->AddRectFilled(ImVec2(x0 - 1.f, y0), ImVec2(x1 + 1.f, y1 + 2.f), IM_COL32(0, 0, 0, (int)(60.f * ease)), 6.f);
        Draw->RectFilled(dl, ImVec2(x0, y0), ImVec2(x1, y1), Draw->GetClr(Colors->slider.background, ease), 5.f);
        dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), Draw->GetClr(Colors->accent, 0.45f * ease), 5.f, 0, 1.f);

        {
            const float px = ImClamp(knob_x, x0 + 6.f, x1 - 6.f);
            dl->AddTriangleFilled(ImVec2(px - 4.f, y1), ImVec2(px + 4.f, y1), ImVec2(px, y1 + 4.f), Draw->GetClr(Colors->slider.background, ease));
            dl->AddLine(ImVec2(px - 4.f, y1), ImVec2(px, y1 + 4.f), Draw->GetClr(Colors->accent, 0.45f * ease), 1.f);
            dl->AddLine(ImVec2(px + 4.f, y1), ImVec2(px, y1 + 4.f), Draw->GetClr(Colors->accent, 0.45f * ease), 1.f);
        }

        ImVec4 vc = Colors->slider.value.Value;
        vc.w = ease;
        Draw->TextClipped(dl, font, ImVec2(x0, y0), ImVec2(x1, y1), Draw->GetClr(ImColor(vc)), value_buf, value_buf_end, nullptr, ImVec2(0.5f, 0.5f));

        dl->PopClipRect();
    }

    if (out_gear_bb) {
        const float gx = track_x1 + knob_r_hover + gear_pad;
        *out_gear_bb = ImRect(ImVec2(gx, track_y - gear_sz * 0.5f), ImVec2(gx + gear_sz, track_y + gear_sz * 0.5f));
    }

    window->DC.CursorPos = next_item_cursor_pos;
    return value_changed;
}

bool CGui::SliderTrack(const char* str_id, const ImVec2 & bb_min, const ImVec2 & bb_max, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, float alpha) {
    using namespace slider_cfg;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(str_id);

    if (format == NULL)
        format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

    char value_buf[64];
    ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    const char* value_buf_end = value_buf + strlen(value_buf);

    ImFont* font = Var->font.inter[5] ? Var->font.inter[5] : Var->font.inter[2];

    const float track_y = ImFloor((bb_min.y + bb_max.y) * 0.5f);
    const float track_x0 = bb_min.x + knob_r_hover;
    const float track_x1 = bb_max.x - knob_r_hover;
    const float track_w = ImMax(track_x1 - track_x0, 1.f);

    const ImRect rect(bb_min, bb_max);
    const ImRect slider_bb(ImVec2(track_x0 - 2.f, track_y - 9.f), ImVec2(track_x1 + 2.f, track_y + 9.f));
    const ImRect hover_bb(ImVec2(bb_min.x, track_y - 10.f), ImVec2(bb_max.x, track_y + 10.f));

    if (!ImGui::ItemAdd(rect, id, &slider_bb, ImGuiItemFlags_Inputable))
        return false;

    SliderState* state = Gui->AnimContainer(&state, id);

    const bool hovered = ImGui::ItemHoverable(hover_bb, id, g.LastItemData.ItemFlags);
    const bool is_active = (g.ActiveId == id);

    bool value_changed = false;
    bool just_opened_edit = false;

    if (!state->editing && hovered && (g.IO.MouseClicked[1] || (g.IO.KeyCtrl && g.IO.MouseClicked[0]))) {
        strncpy_s(state->edit_buf, value_buf, sizeof(state->edit_buf) - 1);
        state->editing = true;
        just_opened_edit = true;
        g.IO.MouseClicked[0] = false;
        g.IO.MouseClicked[1] = false;
    }

    ImRect grab_bb;
    if (!state->editing) {
        if (hovered && ImGui::IsMouseClicked(0, 0, id)) {
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 0.0f);
        value_changed = ImGui::SliderBehavior(slider_bb, id, data_type, p_data, p_min, p_max, format, 0, &grab_bb);
        ImGui::PopStyleVar();

        if (hovered && !is_active && g.IO.MouseWheel != 0.f && ApplyMouseStep(data_type, p_data, p_min, p_max, (int)g.IO.MouseWheel))
            value_changed = true;

        if (is_active || (hovered && g.ActiveId == 0)) {
            float step_dir = 0.f;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
                step_dir -= 1.f;

            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
                step_dir += 1.f;

            if (step_dir != 0.f && ApplySliderStep(data_type, p_data, p_min, p_max, step_dir))
                value_changed = true;
        }

        if (value_changed) {
            ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
            value_buf_end = value_buf + strlen(value_buf);
            ImGui::MarkItemEdited(id);
            state->last_change = (float)g.Time;
        }
    }

    const bool recently_changed = ((float)g.Time - state->last_change) < show_after_change;
    const bool show_bubble = !state->editing && (hovered || is_active || recently_changed);

    state->hover = ImClamp(state->hover + Gui->FixedSpeed(14.f) * (hovered ? 1.f : -1.f), 0.f, 1.f);
    state->active = ImClamp(state->active + Gui->FixedSpeed(14.f) * (is_active ? 1.f : -1.f), 0.f, 1.f);
    state->bubble = ImClamp(state->bubble + Gui->FixedSpeed(12.f) * (show_bubble ? 1.f : -1.f), 0.f, 1.f);
    state->edit_alpha = ImClamp(state->edit_alpha + Gui->FixedSpeed(14.f) * (state->editing ? 1.f : -1.f), 0.f, 1.f);

    ImDrawList* dl = window->DrawList;
    const float glow_t = ImMax(state->hover, state->active);

    const float value_ratio = SliderValueRatio(data_type, p_data, p_min, p_max);
    const float knob_x = track_x0 + value_ratio * track_w;

    const ImVec2 track_min(track_x0, track_y - track_h * 0.5f);
    const ImVec2 track_max(track_x1, track_y + track_h * 0.5f);
    const float track_rnd = track_h * 0.5f;

    {
        const float track_alpha = (1.f - state->edit_alpha * 0.85f) * alpha;
        Draw->RectFilled(dl, track_min, track_max, Draw->GetClr(Colors->slider.background, track_alpha), track_rnd);

        if (knob_x - track_x0 > 0.5f) {
            const ImVec2 fill_max(knob_x, track_max.y);

            if (glow_t > 0.001f) {
                ImVec4 gc = Colors->accent.Value;
                for (int i = 1; i <= 3; ++i) {
                    float off = (float)i * 1.5f;
                    gc.w = (0.20f / (float)i) * glow_t * track_alpha;
                    dl->AddRect(ImVec2(track_min.x - off, track_min.y - off), ImVec2(fill_max.x + off, fill_max.y + off), Draw->GetClr(ImColor(gc)), track_rnd + off, 0, 1.f);
                }
            }

            Draw->RectFilled(dl, track_min, fill_max, Draw->GetClr(Colors->accent, track_alpha), track_rnd);
        }

        const float kr = ImLerp(knob_r, knob_r_hover, glow_t);
        const ImVec2 kc(knob_x, track_y);

        dl->AddCircleFilled(ImVec2(kc.x, kc.y + 1.f), kr + 1.5f, IM_COL32(0, 0, 0, (int)(70.f * track_alpha)), 24);
        if (glow_t > 0.001f)
            dl->AddCircleFilled(kc, kr + 3.f, Draw->GetClr(Colors->accent, 0.25f * glow_t * track_alpha), 24);

        dl->AddCircleFilled(kc, kr, IM_COL32(236, 236, 240, (int)(255.f * track_alpha)), 24);
    }

    if (font && state->editing) {
        ImVec2 backup_cursor_pos = window->DC.CursorPos;

        const float edit_h = 22.f;
        const ImVec2 edit_min(track_x0 - knob_r_hover, track_y - edit_h * 0.5f);
        const ImVec2 edit_max(track_x1 + knob_r_hover, track_y + edit_h * 0.5f);

        Draw->RectFilled(dl, edit_min, edit_max, Draw->GetClr(Colors->slider.background, state->edit_alpha * alpha), 5.f);
        dl->AddRect(edit_min, edit_max, Draw->GetClr(Colors->accent, 0.55f * state->edit_alpha * alpha), 5.f, 0, 1.f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, Draw->GetClr(Colors->accent, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_Text, Colors->slider.value.Value);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

        ImGui::PushFont(font);

        ImGui::SetCursorScreenPos(ImVec2(edit_min.x, track_y - font->LegacySize * 0.5f));
        ImGui::SetNextItemWidth(edit_max.x - edit_min.x);

        ImGuiInputTextFlags itf = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CharsDecimal;

        ImGui::PushID(id);
        if (just_opened_edit)
            ImGui::SetKeyboardFocusHere();

        bool confirmed = ImGui::InputText("##st_edit_inline", state->edit_buf, IM_ARRAYSIZE(state->edit_buf), itf);
        bool escaped = ImGui::IsKeyPressed(ImGuiKey_Escape);
        bool clicked_outside = !just_opened_edit && !ImGui::IsItemActive() && g.IO.MouseClicked[0] && !ImGui::IsMouseHoveringRect(edit_min, edit_max);
        ImGui::PopID();

        ImGui::PopFont();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(6);

        window->DC.CursorPos = backup_cursor_pos;

        if (confirmed) {
            char* end_ptr = nullptr;
            double parsed = strtod(state->edit_buf, &end_ptr);

            if (end_ptr != state->edit_buf) {
                if (data_type == ImGuiDataType_S32) {
                    int imin = *(const int*)p_min;
                    int imax = *(const int*)p_max;
                    *(int*)p_data = (int)ImClamp(parsed, (double)imin, (double)imax);
                } else {
                    float fmin = *(const float*)p_min;
                    float fmax = *(const float*)p_max;
                    *(float*)p_data = ImClamp((float)parsed, fmin, fmax);
                }

                value_changed = true;
                ImGui::MarkItemEdited(id);
                state->last_change = (float)g.Time;
            }
            state->editing = false;
        }

        if (escaped || clicked_outside)
            state->editing = false;
    }

    if (font && state->bubble > 0.01f) {
        const float a = state->bubble;
        const float ease = (1.f - (1.f - a) * (1.f - a)) * alpha;

        ImVec2 ts = font->CalcTextSizeA(font->LegacySize, FLT_MAX, -1.f, value_buf, value_buf_end);
        const float bw = ImMax(ts.x + 16.f, 30.f);
        const float bh = bubble_h;

        const float slide = (1.f - ease) * 4.f;
        const float y1 = track_y - knob_r_hover - bubble_gap + slide;
        const float y0 = y1 - bh;
        float x0 = ImFloor(knob_x - bw * 0.5f);
        float x1 = x0 + bw;

        if (x0 < bb_min.x) {
            x0 = bb_min.x;
            x1 = x0 + bw;
        }

        if (x1 > bb_max.x) {
            x1 = bb_max.x;
            x0 = x1 - bw;
        }

        dl->PushClipRectFullScreen();

        dl->AddRectFilled(ImVec2(x0 - 1.f, y0), ImVec2(x1 + 1.f, y1 + 2.f), IM_COL32(0, 0, 0, (int)(60.f * ease)), 6.f);
        Draw->RectFilled(dl, ImVec2(x0, y0), ImVec2(x1, y1), Draw->GetClr(Colors->slider.background, ease), 5.f);
        dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), Draw->GetClr(Colors->accent, 0.45f * ease), 5.f, 0, 1.f);

        {
            const float px = ImClamp(knob_x, x0 + 6.f, x1 - 6.f);
            dl->AddTriangleFilled(ImVec2(px - 4.f, y1), ImVec2(px + 4.f, y1), ImVec2(px, y1 + 4.f), Draw->GetClr(Colors->slider.background, ease));
            dl->AddLine(ImVec2(px - 4.f, y1), ImVec2(px, y1 + 4.f), Draw->GetClr(Colors->accent, 0.45f * ease), 1.f);
            dl->AddLine(ImVec2(px + 4.f, y1), ImVec2(px, y1 + 4.f), Draw->GetClr(Colors->accent, 0.45f * ease), 1.f);
        }

        ImVec4 vc = Colors->slider.value.Value;
        vc.w = ease;
        Draw->TextClipped(dl, font, ImVec2(x0, y0), ImVec2(x1, y1), Draw->GetClr(ImColor(vc)), value_buf, value_buf_end, nullptr, ImVec2(0.5f, 0.5f));

        dl->PopClipRect();
    }

    return value_changed;
}

extern void RenderGearPopupFn(ImDrawList*, const ImRect&, const char*, std::function<void()>);

bool CGui::SliderInt(std::string_view label, int* v, int v_min, int v_max, const char* format) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label.data());
    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    bool changed = SliderScalar(label.data(), ImGuiDataType_S32, v, &v_min, &v_max, format, 0, false, bind_popup_name);
    Binds::RenderSliderIntPopup(bind_popup_name, label, v, v_min, v_max, format);
    return changed;
}

bool CGui::SliderFloat(std::string_view label, float* v, float v_min, float v_max, const char* format, float scale) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label.data());
    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    bool changed;
    if (scale == 1.f) {
        changed = SliderScalar(label.data(), ImGuiDataType_Float, v, &v_min, &v_max, format, 0, false, bind_popup_name);
    } else {
        float mn = v_min * scale;
        float mx = v_max * scale;
        float val = *v * scale;
        changed = SliderScalar(label.data(), ImGuiDataType_Float, &val, &mn, &mx, format, 0, false, bind_popup_name);
        if (changed)
            *v = val / scale;
    }

    Binds::RenderSliderFloatPopup(bind_popup_name, label, v, v_min, v_max, format, scale);
    return changed;
}

bool CGui::SliderIntEx(std::string_view label, int* v, int v_min, int v_max, std::function<void()> gear_fn, const char* format) {
    const bool has_gear = (gear_fn != nullptr);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label.data());
    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    ImRect gear_bb;
    bool changed = SliderScalar(label.data(), ImGuiDataType_S32, v, &v_min, &v_max, format, 0, has_gear, bind_popup_name, &gear_bb);

    if (has_gear)
        RenderGearPopupFn(window->DrawList, gear_bb, label.data(), gear_fn);

    Binds::RenderSliderIntPopup(bind_popup_name, label, v, v_min, v_max, format);
    return changed;
}

bool CGui::SliderFloatEx(std::string_view label, float* v, float v_min, float v_max, std::function<void()> gear_fn, const char* format, float scale) {
    const bool has_gear = (gear_fn != nullptr);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label.data());
    char bind_popup_name[64];
    sprintf_s(bind_popup_name, "##binds_popup_%u", id);

    ImRect gear_bb;
    bool changed;
    if (scale == 1.f) {
        changed = SliderScalar(label.data(), ImGuiDataType_Float, v, &v_min, &v_max, format, 0, has_gear, bind_popup_name, &gear_bb);
    } else {
        float mn = v_min * scale, mx = v_max * scale, val = *v * scale;
        changed = SliderScalar(label.data(), ImGuiDataType_Float, &val, &mn, &mx, format, 0, has_gear, bind_popup_name, &gear_bb);
        if (changed)
            *v = val / scale;
    }

    if (has_gear)
        RenderGearPopupFn(window->DrawList, gear_bb, label.data(), gear_fn);

    Binds::RenderSliderFloatPopup(bind_popup_name, label, v, v_min, v_max, format, scale);
    return changed;
}