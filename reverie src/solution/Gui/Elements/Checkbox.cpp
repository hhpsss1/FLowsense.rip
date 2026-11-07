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

    const float r = 5.f;
    ImVec4 cv(col.Rf(), col.Gf(), col.Bf(), alpha ? col.Af() : 1.f);
    ImU32 col_u32 = ImGui::ColorConvertFloat4ToU32(cv);

    if (alpha && col.Af() < 0.99f)
        RenderColorRectWithAlphaCheckboard(dl, bb.Min, bb.Max, IM_COL32(0, 0, 0, 0), bb.GetWidth() / 4.f, ImVec2(0, 0), r, 0);

    dl->AddRectFilled(bb.Min, bb.Max, col_u32, r);

    if (hovered)
        dl->AddRect(bb.Min, bb.Max, IM_COL32(255, 255, 255, 35), r, 0, 1.f);

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

    Gui->PushStyleVar(ImGuiStyleVar_Alpha, anim_ease);
    Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
    Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 12.f));

    Gui->PushStyleColor(ImGuiCol_WindowBg, GuiPopupBg(0.98f));
    Gui->PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 24));

    ImVec2 popup_pos = st->anchor + ImVec2(0.f, (1.f - anim_ease) * -6.f);
    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    std::string overlay_name = std::string("##swatch_overlay_") + uid;

    if (ImGui::Begin(overlay_name.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGuiWindow* popup_win = ImGui::GetCurrentWindow();

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

        float tmp[4] = { col.Rf(), col.Gf(), col.Bf(), alpha ? col.Af() : 1.f };

        ImGuiColorEditFlags fl = ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview;

        if (alpha) {
            fl |= ImGuiColorEditFlags_AlphaBar;
        } else {
            fl |= ImGuiColorEditFlags_NoAlpha;
        }

        if (ColorPicker(xs("##sp"), tmp, fl, nullptr))
            col = Color_t(tmp[0], tmp[1], tmp[2], alpha ? tmp[3] : col.A());
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
        Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
        Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
        Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 12.f));
        Gui->PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 6.f));
        Gui->PushStyleColor(ImGuiCol_WindowBg, GuiPopupBg(0.98f));
        Gui->PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 24));

        ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        std::string wname = std::string("##gear_popup_") + label_uid;
        ImGui::SetNextWindowSizeConstraints(ImVec2(popup_w, 0.f), ImVec2(popup_w, FLT_MAX));

        if (ImGui::Begin(wname.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing)) {
            if (Gui->about_active && Gui->about_draw_list && w->DrawList == Gui->about_draw_list) {
                Gui->about_extra_draw_lists.push_back(ImGui::GetWindowDrawList());
                Gui->about_extra_windows.push_back(ImGui::GetCurrentWindow());
            }

            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

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
        float fill{ 0.f };
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

    const float box_sz = 14.f;
    const float row_h = 20.f;
    const float box_r = 0.f;
    const float swatch_sz = 14.f;
    const float swatch_gap = 4.f;
    const float gear_sz = 14.f;
    const float badge_sz = 10.f;
    const float right_pad = 0.f;

    color_count = ImClamp(color_count, 0, 4);

    const float box_x = pos.x + avail - right_pad - box_sz;
    const float box_y = pos.y + (row_h - box_sz) * 0.5f;
    const ImRect box_bb(ImVec2(box_x, box_y), ImVec2(box_x + box_sz, box_y + box_sz));

    float right_side_w = box_sz + right_pad;
    if (color_count > 0 && colors)
        right_side_w += color_count * (swatch_sz + swatch_gap);
    if (gear_fn)
        right_side_w += gear_sz + swatch_gap;
    if (insecure)
        right_side_w += badge_sz + 6.f;

    const ImRect rect(pos, ImVec2(pos.x + avail, pos.y + row_h));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    const bool hovered = ImGui::IsMouseHoveringRect(rect.Min, rect.Max) && g.HoveredWindow == window;
    const float interactive_min_x = box_x - (right_side_w - box_sz - right_pad);
    const bool in_swatch_or_gear = (color_count > 0 || gear_fn) && io.MousePos.x >= interactive_min_x && io.MousePos.x < box_x;

    bool pressed = false;
    if (hovered && !in_swatch_or_gear && io.MouseClicked[0]) {
        pressed = true;
        *callback = !(*callback);
        MarkItemEdited(id);
    }

    char bind_popup_name[64];
    sprintf_s(bind_popup_name, xs("##binds_popup_%u"), id);
    if (hovered && io.MouseClicked[1])
        ImGui::OpenPopup(bind_popup_name);

    CheckboxState* state = Gui->AnimContainer(&state, id);
    state->fill = ImClamp(state->fill + FixedSpeed(14.f) * (*callback ? 1.f : -1.f), 0.f, 1.f);
    state->hover = ImClamp(state->hover + FixedSpeed(12.f) * (hovered ? 1.f : -1.f), 0.f, 1.f);

    ImDrawList* dl = window->DrawList;

    ImVec4 bg_off = Colors->button.background.Value;
    ImVec4 bg_on = Colors->accent.Value;

    ImVec4 bg_col(
        ImLerp(bg_off.x, bg_on.x, state->fill),
        ImLerp(bg_off.y, bg_on.y, state->fill),
        ImLerp(bg_off.z, bg_on.z, state->fill),
        ImLerp(bg_off.w, 1.f, state->fill)
    );

    if (state->hover > 0.01f && state->fill < 0.99f) {
        bg_col.x = ImMin(bg_col.x + 0.03f * state->hover, 1.f);
        bg_col.y = ImMin(bg_col.y + 0.03f * state->hover, 1.f);
        bg_col.z = ImMin(bg_col.z + 0.03f * state->hover, 1.f);
    }

    dl->AddRectFilled(box_bb.Min, box_bb.Max, Draw->GetClr(ImColor(bg_col)), box_r);

    if (state->fill < 0.99f) {
        float b_alpha = ImLerp(0.30f, 0.60f, state->hover);
        dl->AddRect(box_bb.Min, box_bb.Max, Draw->GetClr(Colors->divider, b_alpha), box_r, 0, 1.f);
    }

    if (state->fill > 0.005f) {
        const float lum = 0.299f * bg_on.x + 0.587f * bg_on.y + 0.114f * bg_on.z;
        const ImColor ck_base_col = lum > 0.45f ? ImColor(18, 20, 26) : ImColor(255, 255, 255);
        const ImU32 ck_col = Draw->GetClr(ck_base_col, state->fill);
        RenderCheckmark(dl, box_bb.GetCenter(), state->fill, ck_col);
    }

    float rx = box_x;

    if (color_count > 0 && colors) {
        const float sw_y = pos.y + (row_h - swatch_sz) * 0.5f;
        for (int i = color_count - 1; i >= 0; --i) {
            if (!colors[i])
                continue;

            rx -= (swatch_sz + swatch_gap);
            ImRect sw_bb(ImVec2(rx, sw_y), ImVec2(rx + swatch_sz, sw_y + swatch_sz));

            std::string sw_uid = std::string(label.data()) + "_sw_" + std::to_string(i);
            RenderInlineSwatch(dl, sw_bb, *colors[i], true, sw_uid.c_str());
        }
    }

    if (gear_fn) {
        rx -= (gear_sz + swatch_gap);
        ImRect gear_bb(ImVec2(rx, pos.y), ImVec2(rx + gear_sz, pos.y + row_h));
        std::string uid = std::string(label.data()) + "_gear";
        RenderGearPopupFn(dl, gear_bb, uid.c_str(), gear_fn);
    }

    ImFont* font_regular = Var->font.inter[0];

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
                Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
                Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
                Gui->PushStyleColor(ImGuiCol_WindowBg, GuiPopupBg(0.98f));
                Gui->PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 24));

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
                Gui->PopStyleVar(4);
                Gui->PopStyleColor(2);
            }
        }
    }

    if (font_regular) {
        const float text_alpha = ImLerp(0.55f, 1.0f, ImMax(state->fill, state->hover * 0.5f));
        ImColor tc = Colors->checkbox.text;
        tc.Value.w = text_alpha;
        Draw->TextClipped(dl, font_regular, ImVec2(pos.x, pos.y), ImVec2(rx - 8.f, pos.y + row_h), Draw->GetClr(tc), label.data(), nullptr, nullptr, ImVec2(0.f, 0.5f));
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
    const float swatch_sz = 18.f;
    const float swatch_gap = 6.f;
    const float gear_sz = 18.f;
    const float right_pad = 0.f;

    color_count = ImClamp(color_count, 0, 4);

    float right_used = right_pad;
    if (gear_fn)
        right_used += gear_sz + swatch_gap;
    if (color_count > 0 && colors)
        right_used += color_count * (swatch_sz + swatch_gap);

    const ImRect rect(pos, ImVec2(pos.x + avail, pos.y + row_h));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    const bool hovered = ImGui::IsMouseHoveringRect(rect.Min, rect.Max) && g.HoveredWindow == window;

    ImDrawList* dl = window->DrawList;

    ImFont* font_regular = Var->font.inter[0];
    if (font_regular) {
        float label_right = pos.x + avail - right_used;
        ImVec4 tc = Colors->checkbox.text.Value;
        tc.w = hovered ? 0.90f : 0.60f;
        Draw->TextClipped(dl, font_regular, pos, ImVec2(label_right, pos.y + row_h), Draw->GetClr(ImColor(tc)), label.data(), nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    float rx = pos.x + avail - right_pad;

    if (gear_fn) {
        rx -= gear_sz;
        ImRect gear_bb(ImVec2(rx, pos.y), ImVec2(rx + gear_sz, pos.y + row_h));
        std::string uid = std::string(label.data()) + "_list_gear";
        RenderGearPopupFn(dl, gear_bb, uid.c_str(), gear_fn);
        rx -= swatch_gap;
    }

    if (color_count > 0 && colors) {
        float sw_y = pos.y + (row_h - swatch_sz) * 0.5f;
        for (int i = color_count - 1; i >= 0; --i) {
            if (!colors[i])
                continue;

            rx -= swatch_sz;
            ImRect sw_bb(ImVec2(rx, sw_y), ImVec2(rx + swatch_sz, sw_y + swatch_sz));

            std::string sw_uid = std::string(label.data()) + "_list_sw_" + std::to_string(i);
            RenderInlineSwatch(dl, sw_bb, *colors[i], true, sw_uid.c_str());
            rx -= swatch_gap;
        }
    }

    return false;
}