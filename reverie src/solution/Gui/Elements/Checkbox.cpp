#include <shared.h>

#include "../settings/Share.h"
#include "../Settings/Functions.h"

static int s_last_popup_frame = -1;

void GuiMarkPopupOpen() {
    s_last_popup_frame = ImGui::GetFrameCount();
}

bool GuiIsPopupOpen() {
    return s_last_popup_frame >= ImGui::GetFrameCount() - 1;
}

bool GuiIsPopupHovered() {
    ImGuiContext& g = *GImGui;
    if (!g.HoveredWindow || !g.HoveredWindow->Name)
        return false;

    const char* name = g.HoveredWindow->Name;
    if (strstr(name, xs("dropdown")) || strstr(name, xs("cwd_dd_")) || strstr(name, xs("picker")) || strstr(name, xs("sw_edit")) || strstr(name, xs("binds_popup")) || strstr(name, xs("swatch_overlay")) || strstr(name, xs("gear_popup")) || strstr(name, xs("ColorPicker")) || strstr(name, xs("btip_")) || strstr(name, xs("popup")))
        return true;

    if (g.HoveredWindow->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip))
        return true;

    return false;
}

ImU32 GuiPopupBg(float alpha) {
    ImColor c = Colors->child.background;
    if (c.Value.w < 0.05f)
        c = Colors->window.background;

    return Draw->GetClr(c, alpha);
}

static void DrawPopupWindowLayers(ImDrawList* dl, const ImVec2& min, const ImVec2& max) {
    dl->AddRectFilled(min, max, OtcStyle::WindowShadow());
    dl->AddRectFilled(min + ImVec2(1.f, 1.f), max - ImVec2(1.f, 1.f), OtcStyle::WindowLine());
    dl->AddRectFilled(min + ImVec2(2.f, 2.f), max - ImVec2(2.f, 2.f), OtcStyle::WindowBetween());
    dl->AddRectFilled(min + ImVec2(5.f, 5.f), max - ImVec2(5.f, 5.f), IM_COL32(57, 57, 57, 190));
    dl->AddRectFilled(min + ImVec2(6.f, 6.f), max - ImVec2(6.f, 6.f), OtcStyle::WindowFace());
}

bool DrawOtcColorPicker(ImDrawList* dl, const ImVec2& popup_min, Color_t& col, OtcPickerState& state) {
    constexpr float picker_size = 179.f;
    constexpr float bar_h = 8.f;
    constexpr float pad = 10.f;
    const float popup_w = picker_size + 2.f + pad * 2.f;
    const float popup_h = picker_size + bar_h + 3.f + pad * 2.f;
    const ImVec2 popup_max(popup_min.x + popup_w, popup_min.y + popup_h);

    dl->AddRectFilled(popup_min, popup_max, IM_COL32(35, 35, 35, 255));
    DrawPopupWindowLayers(dl, popup_min, popup_max);
    dl->AddRectFilled(popup_min + ImVec2(pad, pad), popup_max - ImVec2(pad, pad), IM_COL32(0, 0, 0, 255));

    const ImVec2 palette_min(popup_min.x + pad + 1.f, popup_min.y + pad + 1.f);
    const ImVec2 palette_max(palette_min.x + picker_size, palette_min.y + picker_size);
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const float half = picker_size * 0.5f;
    bool changed = false;

    ImGui::SetCursorScreenPos(palette_min);
    ImGui::InvisibleButton("palette_area", ImVec2(picker_size, picker_size));
    if (ImGui::IsItemClicked())
        state.held = true;

    if (!state.held) {
        float h = 0.f, s = 0.f, v = 0.f;
        ImGui::ColorConvertRGBtoHSV(col.Rf(), col.Gf(), col.Bf(), h, s, v);
        state.marker = ImVec2(h * picker_size, v < 1.f ? half + (1.f - v) * half : s * half);
    }

    if (state.held && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float sample_x = std::clamp(mouse.x, palette_min.x, palette_max.x);
        const float sample_y = std::clamp(mouse.y, palette_min.y, palette_max.y);
        state.marker = ImVec2(sample_x - palette_min.x, sample_y - palette_min.y);

        const float hue = std::clamp((sample_x - palette_min.x) / picker_size, 0.f, 1.f);
        const float dy = sample_y - palette_min.y;
        const float sat = dy > half ? 1.f : std::clamp(dy / half, 0.f, 1.f);
        const float val = dy > half ? 1.f - std::clamp((dy - half) / half, 0.f, 1.f) : 1.f;

        float r = 0.f, g = 0.f, b = 0.f;
        ImGui::ColorConvertHSVtoRGB(hue, sat, val, r, g, b);
        col = Color_t(r, g, b, col.Af());
        changed = true;
    }
    if (state.held && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        state.held = false;

    for (int i = 0; i < static_cast<int>(picker_size); ++i) {
        ImVec4 hue_color;
        ImGui::ColorConvertHSVtoRGB(static_cast<float>(i) / picker_size, 1.f, 1.f, hue_color.x, hue_color.y, hue_color.z);
        const ImU32 top = IM_COL32(255, 255, 255, 255);
        const ImU32 middle = IM_COL32(static_cast<int>(hue_color.x * 255.f), static_cast<int>(hue_color.y * 255.f), static_cast<int>(hue_color.z * 255.f), 255);
        const float px = palette_min.x + static_cast<float>(i);
        dl->AddRectFilledMultiColor(ImVec2(px, palette_min.y), ImVec2(px + 1.f, palette_min.y + half), top, top, middle, middle);
        dl->AddRectFilledMultiColor(ImVec2(px, palette_min.y + half), ImVec2(px + 1.f, palette_max.y), middle, middle, IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 255));
    }

    const ImVec2 mark(palette_min.x + state.marker.x, palette_min.y + state.marker.y);
    dl->PushClipRect(palette_min, palette_max, true);
    dl->AddCircle(mark, 4.5f, IM_COL32(0, 0, 0, 255), 0, 3.f);
    dl->AddCircle(mark, 4.5f, IM_COL32(255, 255, 255, 255), 0, 1.f);
    dl->PopClipRect();

    const ImVec2 bar_min(palette_min.x, palette_max.y + 1.f);
    const ImVec2 bar_max(palette_max.x, bar_min.y + bar_h);
    ImGui::SetCursorScreenPos(bar_min);
    ImGui::InvisibleButton("alpha_area", ImVec2(picker_size, bar_h));
    if (ImGui::IsItemClicked())
        state.held_alpha = true;
    if (state.held_alpha) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float new_alpha = std::clamp((mouse.x - bar_min.x - 1.5f) / (picker_size - 3.f), 0.f, 1.f);
            col = Color_t(col.Rf(), col.Gf(), col.Bf(), new_alpha);
            changed = true;
        } else {
            state.held_alpha = false;
        }
    }

    for (int i = 0; i * 4 < static_cast<int>(picker_size); ++i) {
        for (int j = 0; j < 2; ++j) {
            const ImVec2 cell_min(bar_min.x + i * 4.f, bar_min.y + j * 4.f);
            dl->AddRectFilled(cell_min, ImVec2(std::min(cell_min.x + 4.f, bar_max.x), cell_min.y + 4.f), (i + j) & 1 ? IM_COL32(153, 153, 153, 255) : IM_COL32(255, 255, 255, 255));
        }
    }

    const ImU32 opaque = ImGui::ColorConvertFloat4ToU32(ImVec4(col.Rf(), col.Gf(), col.Bf(), 1.f));
    const ImU32 transparent = opaque & ~IM_COL32_A_MASK;
    dl->AddRectFilledMultiColor(bar_min, bar_max, transparent, opaque, opaque, transparent);

    const float handle_x = std::round(bar_min.x + col.Af() * (picker_size - 3.f));
    dl->AddRectFilled(ImVec2(handle_x, bar_min.y), ImVec2(handle_x + 3.f, bar_max.y), IM_COL32(0, 0, 0, 255));
    dl->AddRectFilled(ImVec2(handle_x + 1.f, bar_min.y), ImVec2(handle_x + 2.f, bar_max.y), IM_COL32(255, 255, 255, 255));

    return changed;
}

static bool IsAnyChildPopupOrDropdownHovered() {
    return GuiIsPopupHovered();
}

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

static void RenderInlineSwatch(ImDrawList* dl, const ImRect& bb, Color_t& col, bool alpha, const char* uid) {
    struct SwState {
        bool open = false;
        float a = 0.f;
        ImVec2 anchor = {};
        bool picker_hov = false;
        bool just_toggled = false;
        bool just_opened = false;
        int last_frame = 0;
        bool was_picking = false;
        bool refocus = false;
        OtcPickerState picker{};
    };

    extern bool GuiColorPicking();

    ImGuiContext& g = *GImGui;
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (!w || w->SkipItems)
        return;

    ImGuiID id = w->GetID(uid);
    if (!ImGui::ItemAdd(bb, id))
        return;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImVec4 cv(col.Rf(), col.Gf(), col.Bf(), alpha ? col.Af() : 1.f);
    ImU32 col_u32 = ImGui::ColorConvertFloat4ToU32(cv);

    dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(0, 0, 0, 255));
    const ImVec2 inner_min(bb.Min.x + 1.f, bb.Min.y + 1.f);
    const ImVec2 inner_max(bb.Max.x - 1.f, bb.Max.y - 1.f);
    for (int i = 0; i * 3 < static_cast<int>(inner_max.x - inner_min.x); ++i) {
        for (int j = 0; j * 3 < static_cast<int>(inner_max.y - inner_min.y); ++j) {
            const ImVec2 cell_min(inner_min.x + i * 3.f, inner_min.y + j * 3.f);
            dl->AddRectFilled(cell_min, ImVec2(ImMin(cell_min.x + 3.f, inner_max.x), ImMin(cell_min.y + 3.f, inner_max.y)), (i + j) & 1 ? IM_COL32(153, 153, 153, 255) : IM_COL32(255, 255, 255, 255));
        }
    }
    dl->AddRectFilled(inner_min, inner_max, col_u32);

    SwState* st = Gui->AnimContainer(&st, id);

    if (st->last_frame < g.FrameCount - 1) {
        st->open = false;
        st->a = 0.f;
    }

    st->last_frame = g.FrameCount;

    bool swatch_just_toggled = false;
    bool swatch_just_opened = false;

    if (pressed) {
        st->open = !st->open;
        swatch_just_toggled = true;
        swatch_just_opened = st->open;
    }

    if (st->open) {
        st->anchor = ImVec2(bb.Max.x, bb.Max.y + 6.f);
        GuiMarkPopupOpen();
    }

    st->a = ImClamp(st->a + Gui->FixedSpeed(12.f) * (st->open ? 1.f : -1.f), 0.f, 1.f);

    if (!ImGui::IsRectVisible(bb.Min, bb.Max + ImVec2(0, 2))) {
        st->open = false;
        st->a = 0.f;
    }

    if (st->a < 0.01f)
        return;

    const bool picking_now = GuiColorPicking();
    if (st->open && st->was_picking && !picking_now)
        st->refocus = true;

    st->was_picking = picking_now;

    if (st->open && !swatch_just_toggled && g.IO.MouseClicked[0] && !st->picker_hov && !hovered && !picking_now) {
        st->open = false;
        return;
    }

    const float anim_ease = st->a * st->a * (3.f - 2.f * st->a);
    constexpr float picker_size = 179.f;
    constexpr float bar_h = 8.f;
    constexpr float picker_pad = 10.f;
    const float popup_w = picker_size + 2.f + picker_pad * 2.f;
    const float popup_h = picker_size + bar_h + 3.f + picker_pad * 2.f;

    Gui->PushStyleVar(ImGuiStyleVar_Alpha, anim_ease);
    Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.f, 1.f));

    Gui->PushStyleColor(ImGuiCol_WindowBg, IM_COL32(16, 16, 16, 255));
    Gui->PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));

    ImVec2 popup_pos = st->anchor + ImVec2(0.f, (1.f - anim_ease) * -6.f);
    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(popup_w, popup_h), ImGuiCond_Always);

    std::string overlay_name = std::string("##swatch_overlay_") + uid;

    if (ImGui::Begin(overlay_name.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
        ImGuiWindow* popup_win = ImGui::GetCurrentWindow();
        ImDrawList* pdl = popup_win->DrawList;

        ImGui::BringWindowToDisplayFront(popup_win);

        if (Gui->about_active && Gui->about_draw_list && w->DrawList == Gui->about_draw_list) {
            Gui->about_extra_draw_lists.push_back(popup_win->DrawList);
            Gui->about_extra_windows.push_back(popup_win);
        }

        if (swatch_just_opened || st->refocus) {
            ImGui::SetWindowFocus();
            st->refocus = false;
        }

        st->picker_hov = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows);

        DrawOtcColorPicker(pdl, popup_win->Pos, col, st->picker);
    }
    ImGui::End();

    Gui->PopStyleVar(4);
    Gui->PopStyleColor(2);
}

static ImGuiID s_open_gear_id = 0;

void RenderGearPopupFn(ImDrawList* dl, const ImRect& gear_bb, const char* label_uid, std::function<void()> render_fn) {
    struct GearState {
        bool open = false;
        bool hov = false;
        float a = 0.f;
        float anim = 0.f;
        bool need_focus = false;
        int last_frame = 0;
    };

    ImGuiContext& g = *GImGui;
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (!w || w->SkipItems)
        return;

    ImGuiID id = w->GetID((std::string("##gear_") + label_uid).c_str());
    ImGui::ItemAdd(gear_bb, id, nullptr, ImGuiItemFlags_NoNav);

    bool hovered = ImGui::IsMouseHoveringRect(gear_bb.Min, gear_bb.Max);
    bool pressed = hovered && g.IO.MouseClicked[0];
    if (pressed)
        g.IO.MouseClicked[0] = false;

    GearState* st = Gui->AnimContainer(&st, id);

    if (st->last_frame < g.FrameCount - 1) {
        st->open = false;
        st->a = 0.f;
        st->anim = 0.f;
        if (s_open_gear_id == id)
            s_open_gear_id = 0;
    }

    st->last_frame = g.FrameCount;

    if (pressed) {
        bool will_open = !st->open;
        if (will_open) {
            if (s_open_gear_id != 0 && s_open_gear_id != id) {
                GearState* prev = static_cast<GearState*>(ImGui::GetStateStorage()->GetVoidPtr(s_open_gear_id));
                if (prev)
                    prev->open = false;
            }

            s_open_gear_id = id;
            st->need_focus = true;
        } else {
            if (s_open_gear_id == id)
                s_open_gear_id = 0;
        }

        st->open = will_open;
    }

    extern bool GuiColorPicking();

    if (st->open && g.IO.MouseClicked[0] && !st->hov && !IsAnyChildPopupOrDropdownHovered() && !GuiColorPicking()) {
        st->open = false;
        if (s_open_gear_id == id)
            s_open_gear_id = 0;
    }

    if (st->open)
        GuiMarkPopupOpen();

    st->a = ImClamp(st->a + 12.f * g.IO.DeltaTime * (st->open ? 1.f : -1.f), 0.f, 1.f);
    st->anim = ImClamp(st->anim + 14.f * g.IO.DeltaTime * ((hovered || st->open) ? 1.f : -1.f), 0.f, 1.f);

    const float gear_alpha = ImLerp(0.35f, 0.90f, st->anim);
    ImColor gear_clr = ImColor(Colors->text_secondary.Value);
    gear_clr.Value.w = gear_alpha;

    ImFont* icon_font = Var->font.fa[0];
    if (icon_font) {
        ImVec2 isz = icon_font->CalcTextSizeA(icon_font->LegacySize, FLT_MAX, -1.f, ICON_FA_GEAR);
        ImVec2 ipos = gear_bb.GetCenter() - isz * 0.5f;
        dl->AddText(icon_font, icon_font->LegacySize, ipos, Draw->GetClr(gear_clr), ICON_FA_GEAR);
    }

    if (!ImGui::IsRectVisible(gear_bb.Min, gear_bb.Max + ImVec2(0, 2))) {
        st->open = false;
        st->a = 0.f;
        if (s_open_gear_id == id)
            s_open_gear_id = 0;
    }

    if (st->a >= 0.01f) {
        constexpr float popup_w = 230.f;
        const float anim_ease = st->a * st->a * (3.f - 2.f * st->a);
        const ImVec2 popup_pos = ImVec2(gear_bb.Max.x, gear_bb.Max.y + 4.f + (1.f - anim_ease) * -6.f);

        Gui->PushStyleVar(ImGuiStyleVar_Alpha, anim_ease);
        Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 12.f));
        Gui->PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 6.f));
        Gui->PushStyleColor(ImGuiCol_WindowBg, IM_COL32(16, 16, 16, 255));
        Gui->PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));

        ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        std::string wname = std::string("##gear_popup_") + label_uid;
        ImGui::SetNextWindowSizeConstraints(ImVec2(popup_w, 0.f), ImVec2(popup_w, FLT_MAX));

        if (ImGui::Begin(wname.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing)) {
            if (Gui->about_active && Gui->about_draw_list && w->DrawList == Gui->about_draw_list) {
                Gui->about_extra_draw_lists.push_back(ImGui::GetWindowDrawList());
                Gui->about_extra_windows.push_back(ImGui::GetCurrentWindow());
            }

            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

            ImDrawList* wdl = ImGui::GetWindowDrawList();
            const ImVec2 wmin = ImGui::GetWindowPos();
            const ImVec2 wmax = wmin + ImGui::GetWindowSize();
            wdl->AddRectFilled(wmin, wmax, IM_COL32(35, 35, 35, 255));
            DrawPopupWindowLayers(wdl, wmin, wmax);

            if (st->need_focus) {
                ImGui::SetWindowFocus();
                st->need_focus = false;
            }

            st->hov = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows);

            ImFont* font_regular = Var->font.inter[0];
            if (font_regular)
                ImGui::PushFont(font_regular);

            render_fn();

            if (font_regular)
                ImGui::PopFont();
        }
        ImGui::End();
        Gui->PopStyleVar(5);
        Gui->PopStyleColor(2);
    }
}

bool CGui::Checkbox(std::string_view label, bool* callback) {
    return CheckboxEx(label, callback, false, nullptr, 0, nullptr);
}

bool CGui::CheckboxEx(std::string_view label, bool* callback, bool insecure, Color_t** colors, int color_count, std::function<void()> gear_fn) {
    struct CheckboxState {
        float hover{ 0.f };
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const float avail = GetContentRegionAvail().x;

    const float box_sz = 8.f;
    const float row_h = 15.f;
    const float swatch_w = 20.f;
    const float swatch_h = 8.f;
    const float swatch_gap = 4.f;
    const float gear_sz = 14.f;
    const float badge_sz = 10.f;

    color_count = ImClamp(color_count, 0, 4);

    float right_side_w = 0.f;
    if (color_count > 0 && colors)
        right_side_w += color_count * (swatch_w + swatch_gap);
    if (gear_fn)
        right_side_w += gear_sz + swatch_gap;
    if (insecure)
        right_side_w += badge_sz + 6.f;

    const ImRect rect(pos, ImVec2(pos.x + avail, pos.y + row_h));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    const bool hovered = ImGui::IsMouseHoveringRect(rect.Min, rect.Max) && g.HoveredWindow == window;
    const float right_side_min_x = pos.x + avail - right_side_w;
    const bool in_right_side = io.MousePos.x >= right_side_min_x && io.MousePos.x < pos.x + avail;

    bool pressed = false;
    if (hovered && !in_right_side && io.MouseClicked[0]) {
        pressed = true;
        *callback = !(*callback);
        MarkItemEdited(id);
    }

    char bind_popup_name[64];
    sprintf_s(bind_popup_name, xs("##binds_popup_%u"), id);
    if (hovered && io.MouseClicked[1])
        ImGui::OpenPopup(bind_popup_name);

    CheckboxState* state = Gui->AnimContainer(&state, id);
    state->hover = ImClamp(state->hover + FixedSpeed(12.f) * (hovered ? 1.f : -1.f), 0.f, 1.f);

    ImDrawList* dl = window->DrawList;
    const float bx = pos.x;
    const float by = pos.y;

    dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + box_sz, by + box_sz), OtcStyle::Border());
    dl->AddRectFilled(ImVec2(bx + 1.f, by + 1.f), ImVec2(bx + box_sz - 1.f, by + box_sz - 1.f), OtcStyle::CheckOff());
    if (*callback) {
        dl->AddRectFilledMultiColor(ImVec2(bx + 1.f, by + 1.f), ImVec2(bx + box_sz - 1.f, by + box_sz - 1.f),
            OtcStyle::Accent(), OtcStyle::Accent(), OtcStyle::AccentShade(), OtcStyle::AccentShade());
    }
    if (state->hover > 0.01f && !*callback)
        dl->AddRect(ImVec2(bx, by), ImVec2(bx + box_sz, by + box_sz), OtcStyle::Accent(0.45f * state->hover), 0.f, 0, 1.f);

    ImFont* font_regular = Var->font.inter[0];
    if (font_regular) {
        const ImU32 text_clr = OtcStyle::Text();
        Draw->TextClipped(dl, font_regular, ImVec2(pos.x + 20.f, pos.y - 4.f), ImVec2(right_side_min_x - 8.f, pos.y + 9.f), text_clr, label.data(), nullptr, nullptr, ImVec2(0.f, 0.f));
    }

    float rx = pos.x + avail;

    if (gear_fn) {
        rx -= gear_sz;
        ImRect gear_bb(ImVec2(rx, pos.y + (row_h - gear_sz) * 0.5f), ImVec2(rx + gear_sz, pos.y + (row_h + gear_sz) * 0.5f));
        std::string uid = std::string(label.data()) + "_gear";
        RenderGearPopupFn(dl, gear_bb, uid.c_str(), gear_fn);
        rx -= swatch_gap;
    }

    if (color_count > 0 && colors) {
        const float sw_y = pos.y + (row_h - swatch_h) * 0.5f;
        for (int i = color_count - 1; i >= 0; --i) {
            if (!colors[i])
                continue;

            rx -= swatch_w;
            ImRect sw_bb(ImVec2(rx, sw_y), ImVec2(rx + swatch_w, sw_y + swatch_h));

            std::string sw_uid = std::string(label.data()) + "_sw_" + std::to_string(i);
            RenderInlineSwatch(dl, sw_bb, *colors[i], true, sw_uid.c_str());
            rx -= swatch_gap;
        }
    }

    if (insecure) {
        ImFont* fa = Var->font.fa[0];
        if (fa) {
            ImVec2 isz = fa->CalcTextSizeA(fa->LegacySize, FLT_MAX, -1.f, ICON_FA_DIAMOND);
            rx -= (isz.x + 6.f);
            float iy = pos.y + (row_h - isz.y) * 0.5f;

            dl->AddText(fa, fa->LegacySize, ImVec2(rx, iy), Draw->GetClr(ImColor(1.f, 0.58f, 0.f, 0.85f)), ICON_FA_DIAMOND);

            ImRect badge_bb(ImVec2(rx, pos.y), ImVec2(rx + isz.x, pos.y + row_h));
            bool badge_hov = ImGui::IsMouseHoveringRect(badge_bb.Min, badge_bb.Max);

            struct BadgeTip { float a = 0.f; };
            ImGuiID tip_id = window->GetID((std::string(label.data()) + "_btip").c_str());
            BadgeTip* bt = Gui->AnimContainer(&bt, tip_id);
            bt->a = ImClamp(bt->a + 10.f * g.IO.DeltaTime * (badge_hov ? 1.f : -1.f), 0.f, 1.f);

            if (bt->a >= 0.01f) {
                Gui->PushStyleVar(ImGuiStyleVar_Alpha, bt->a);
                Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 12.f));
                Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
                Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
                Gui->PushStyleColor(ImGuiCol_WindowBg, IM_COL32(36, 36, 36, 255));
                Gui->PushStyleColor(ImGuiCol_Border, OtcStyle::Border());

                ImGui::SetNextWindowPos(badge_bb.GetBL() + ImVec2(0, 4.f));
                if (ImGui::Begin((std::string("##btip_") + label.data()).c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing)) {
                    if (font_regular)
                        ImGui::PushFont(font_regular);

                    ImVec4 warn(1.f, 0.58f, 0.f, 1.f);
                    ImGui::TextColored(warn, xs("flagged as insecure"));
                    ImVec4 dim = Colors->checkbox.text.Value;
                    dim.w = 0.45f;
                    ImGui::TextColored(dim, xs("using on official valve servers may result in ban"));

                    if (font_regular)
                        ImGui::PopFont();
                }
                ImGui::End();

                Gui->PopStyleColor(2);
                Gui->PopStyleVar(4);
            }
        }
    }

    Binds::RenderCheckboxPopup(bind_popup_name, label, callback);

    return pressed;
}

bool CGui::ListEx(std::string_view label, std::function<void()> gear_fn) {
    return ListEx(label, nullptr, 0, gear_fn);
}

bool CGui::ListEx(std::string_view label, Color_t** colors, int color_count, std::function<void()> gear_fn) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());
    const ImVec2 pos = window->DC.CursorPos;
    const float avail = GetContentRegionAvail().x;

    const float row_h = 24.f;
    const float swatch_w = 20.f;
    const float swatch_h = 8.f;
    const float swatch_gap = 6.f;
    const float gear_sz = 14.f;
    const float right_pad = 0.f;

    color_count = ImClamp(color_count, 0, 4);

    float right_used = right_pad;
    if (gear_fn)
        right_used += gear_sz + swatch_gap;
    if (color_count > 0 && colors)
        right_used += color_count * (swatch_w + swatch_gap);

    const ImRect rect(pos, ImVec2(pos.x + avail, pos.y + row_h));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    const bool hovered = ImGui::IsMouseHoveringRect(rect.Min, rect.Max) && g.HoveredWindow == window;

    ImDrawList* dl = window->DrawList;

    ImFont* font_regular = Var->font.inter[0];
    if (font_regular) {
        float label_right = pos.x + avail - right_used;
        Draw->TextClipped(dl, font_regular, ImVec2(pos.x + 20.f, pos.y), ImVec2(label_right, pos.y + row_h), OtcStyle::Text(), label.data(), nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    float rx = pos.x + avail - right_pad;

    if (gear_fn) {
        rx -= gear_sz;
        ImRect gear_bb(ImVec2(rx, pos.y + (row_h - gear_sz) * 0.5f), ImVec2(rx + gear_sz, pos.y + (row_h + gear_sz) * 0.5f));
        std::string uid = std::string(label.data()) + "_list_gear";
        RenderGearPopupFn(dl, gear_bb, uid.c_str(), gear_fn);
        rx -= swatch_gap;
    }

    if (color_count > 0 && colors) {
        float sw_y = pos.y + (row_h - swatch_h) * 0.5f;
        for (int i = color_count - 1; i >= 0; --i) {
            if (!colors[i])
                continue;

            rx -= swatch_w;
            ImRect sw_bb(ImVec2(rx, sw_y), ImVec2(rx + swatch_w, sw_y + swatch_h));

            std::string sw_uid = std::string(label.data()) + "_list_sw_" + std::to_string(i);
            RenderInlineSwatch(dl, sw_bb, *colors[i], true, sw_uid.c_str());
            rx -= swatch_gap;
        }
    }

    return false;
}