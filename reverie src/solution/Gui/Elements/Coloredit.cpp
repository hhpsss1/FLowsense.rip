#include "../settings/Functions.h"
#include "../Data/FontsAwesome.h"
#include <vector>
#include <Windows.h>

extern void GuiMarkPopupOpen();

struct PickerState {
    float h = 0.f;
    float s = 0.f;
    float v = 0.f;
    float a = 1.f;
    float pre_rainbow_h = 0.f;
    float pre_rainbow_s = 0.f;
    float pre_rainbow_v = 0.f;
    float pre_pulse_a = 1.f;
    bool rainbow_active = false;
    bool pulse_active = false;
    int rainbow = 0;
    int pulse = 0;
    bool init = false;
    bool synced = false;
    int mode = 0;
    bool mode_open = false;
    float mode_a = 0.f;
    float mode_hover = 0.f;
    float item_hover[3]{};
    int edit_idx = -1;
    int edit_frame = 0;
    bool edit_focus = false;
    char edit_buf[32]{};
    float box_hover[4]{};
    bool picking = false;
    bool pick_lmb = false;
    bool pick_rmb = false;
    bool pick_esc = false;
    float pick_r = 0.f;
    float pick_g = 0.f;
    float pick_b = 0.f;
    int pick_frame = -10;
    float pick_a = 0.f;
    float eye_hover = 0.f;
    int drag = 0;
};

static std::vector<std::pair<PickerState*, Color_t*>> g_pickers;
static ImGuiWindow* g_blocker_window = nullptr;
static bool g_picking = false;
static int g_picking_frame = -10;

bool GuiColorPicking() {
    return g_picking || (GImGui && GImGui->FrameCount - g_picking_frame <= 2);
}

static void RegisterPicker(PickerState* st, Color_t* col) {
    if (!st || !col)
        return;

    for (auto& p : g_pickers) {
        if (p.first == st) {
            p.second = col;
            return;
        }
    }
    g_pickers.push_back({ st, col });
}

void CGui::Update() {
    if (!GImGui)
        return;

    ImGuiContext& g = *GImGui;
    float dt = g.IO.DeltaTime;

    for (auto& [st, col_obj] : g_pickers) {
        if (!st || !col_obj)
            continue;

        float col[4] = { col_obj->Rf(), col_obj->Gf(), col_obj->Bf(), col_obj->Af() };

        if (!st->init) {
            ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], st->h, st->s, st->v);
            st->a = col[3];
            st->pre_rainbow_h = st->h;
            st->pre_rainbow_s = st->s;
            st->pre_rainbow_v = st->v;
            st->pre_pulse_a = st->a;
            st->init = true;
            st->synced = true;
        }

        bool changed = false;

        if (st->rainbow > 0) {
            if (!st->rainbow_active) {
                st->pre_rainbow_h = st->h;
                st->pre_rainbow_s = st->s;
                st->pre_rainbow_v = st->v;
                st->rainbow_active = true;
            }
            st->h = fmodf(st->h + dt * (st->rainbow * 0.15f), 1.0f);
            if (st->s < 0.25f)
                st->s = 0.85f;

            if (st->v < 0.25f)
                st->v = 1.0f;

            ImGui::ColorConvertHSVtoRGB(st->h, st->s, st->v, col[0], col[1], col[2]);
            changed = true;
        }
        else if (st->rainbow_active) {
            st->h = st->pre_rainbow_h;
            st->s = st->pre_rainbow_s;
            st->v = st->pre_rainbow_v;
            st->rainbow_active = false;
            ImGui::ColorConvertHSVtoRGB(st->h, st->s, st->v, col[0], col[1], col[2]);
            changed = true;
        }

        if (st->pulse > 0) {
            if (!st->pulse_active) {
                st->pre_pulse_a = st->a;
                st->pulse_active = true;
            }
            st->a = (sinf((float)g.Time * st->pulse * 2.0f) * 0.5f + 0.5f);
            col[3] = st->a;
            changed = true;
        }
        else if (st->pulse_active) {
            st->a = st->pre_pulse_a;
            st->pulse_active = false;
            col[3] = st->a;
            changed = true;
        }

        if (changed)
            *col_obj = Color_t(col[0], col[1], col[2], col[3]);
    }
}

static ImU32 GetPickerBgColor(float alpha = 0.98f) {
    ImColor c = Colors->child.background;
    if (c.Value.w < 0.05f)
        c = Colors->window.background;

    if (c.Value.w < 0.05f)
        c = ImColor(18, 18, 22, 255);

    return Draw->GetClr(c, alpha);
}

static ImU32 AlphaCol(ImU32 col, float alpha) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(col);
    c.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}

void RenderColorRectWithAlphaCheckboard(ImDrawList* dl, ImVec2 pmin, ImVec2 pmax, ImU32 col, float gs, ImVec2 goff, float rounding, ImDrawFlags flags) {
    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags = ImDrawFlags_RoundCornersDefault_;

    float win_a = ImGui::GetStyle().Alpha;

    if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF) {
        ImU32 c1 = GetColorU32(ImAlphaBlendColors(AlphaCol(IM_COL32(18, 21, 26, 255), win_a), col));
        ImU32 c2 = GetColorU32(ImAlphaBlendColors(AlphaCol(IM_COL32(69, 72, 77, 255), win_a), col));
        dl->AddRectFilled(pmin, pmax, c1, rounding, flags);

        int yi = 0;
        for (float y = pmin.y + goff.y; y < pmax.y; y += gs, yi++) {
            float y1 = ImClamp(y, pmin.y, pmax.y);
            float y2 = ImMin(y + gs, pmax.y);
            if (y2 <= y1)
                continue;

            for (float x = pmin.x + goff.x + (yi & 1) * gs; x < pmax.x; x += gs * 2.0f) {
                float x1 = ImClamp(x, pmin.x, pmax.x);
                float x2 = ImMin(x + gs, pmax.x);
                if (x2 <= x1)
                    continue;

                ImDrawFlags cf = ImDrawFlags_RoundCornersNone;
                if (y1 <= pmin.y) {
                    if (x1 <= pmin.x)
                        cf |= ImDrawFlags_RoundCornersTopLeft;
                    if (x2 >= pmax.x)
                        cf |= ImDrawFlags_RoundCornersTopRight;
                }

                if (y2 >= pmax.y) {
                    if (x1 <= pmin.x)
                        cf |= ImDrawFlags_RoundCornersBottomLeft;
                    if (x2 >= pmax.x)
                        cf |= ImDrawFlags_RoundCornersBottomRight;
                }

                cf = (flags == ImDrawFlags_RoundCornersNone || cf == ImDrawFlags_RoundCornersNone) ? ImDrawFlags_RoundCornersNone : (cf & flags);
                dl->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), c2, rounding, cf);
            }
        }
    }
    else {
        dl->AddRectFilled(pmin, pmax, col, rounding, flags);
    }
}

static void PickerSyncHsv(PickerState* st, const float col[4]) {
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], h, s, v);

    if (s > 0.f && v > 0.f)
        st->h = h;
    if (v > 0.f)
        st->s = s;
    st->v = v;
    st->a = col[3];

    st->pre_rainbow_h = st->h;
    st->pre_rainbow_s = st->s;
    st->pre_rainbow_v = st->v;
    st->pre_pulse_a = st->a;
    st->synced = true;
}

static bool PickerOutOfSync(PickerState* st, const float col[4]) {
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(st->h, st->s, st->v, r, g, b);
    const float e = 1.5f / 255.f;
    return fabsf(r - col[0]) > e || fabsf(g - col[1]) > e || fabsf(b - col[2]) > e || fabsf(st->a - col[3]) > e;
}

static void PickerFormatHex(char* buf, size_t sz, const float col[4], bool has_alpha) {
    if (has_alpha)
        snprintf(buf, sz, "#%02X%02X%02X%02X", IM_F32_TO_INT8_SAT(col[0]), IM_F32_TO_INT8_SAT(col[1]), IM_F32_TO_INT8_SAT(col[2]), IM_F32_TO_INT8_SAT(col[3]));
    else
        snprintf(buf, sz, "#%02X%02X%02X", IM_F32_TO_INT8_SAT(col[0]), IM_F32_TO_INT8_SAT(col[1]), IM_F32_TO_INT8_SAT(col[2]));
}

static bool PickerParseHex(const char* text, float col[4], bool has_alpha) {
    if (!text || !text[0])
        return false;

    const char* p = (text[0] == '#') ? text + 1 : text;
    unsigned v = 0;
    int len = (int)strlen(p);

    if (len == 6 && sscanf_s(p, "%06X", &v) == 1) {
        col[0] = ((v >> 16) & 0xFF) / 255.f;
        col[1] = ((v >> 8) & 0xFF) / 255.f;
        col[2] = (v & 0xFF) / 255.f;
        return true;
    }

    if (len == 8 && sscanf_s(p, "%08X", &v) == 1) {
        col[0] = ((v >> 24) & 0xFF) / 255.f;
        col[1] = ((v >> 16) & 0xFF) / 255.f;
        col[2] = ((v >> 8) & 0xFF) / 255.f;
        if (has_alpha)
            col[3] = (v & 0xFF) / 255.f;
        return true;
    }

    return false;
}

static bool KeyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static void EyedropStart(PickerState* st) {
    st->picking = true;
    st->pick_lmb = true;
    st->pick_rmb = KeyDown(VK_RBUTTON);
    st->pick_esc = KeyDown(VK_ESCAPE);
    st->pick_frame = GImGui->FrameCount;
    st->mode_open = false;
    st->edit_idx = -1;
    st->drag = 0;
    g_picking = true;
    g_picking_frame = GImGui->FrameCount;
}

static void EyedropStop(PickerState* st) {
    st->picking = false;
    st->pick_frame = GImGui->FrameCount;
    g_picking = false;
    g_picking_frame = GImGui->FrameCount;
}

static COLORREF EyedropRead(POINT pt) {
    HDC screen = GetDC(NULL);
    COLORREF c = GetPixel(screen, pt.x, pt.y);

    if (c == CLR_INVALID) {
        HDC mem = CreateCompatibleDC(screen);
        HBITMAP bmp = CreateCompatibleBitmap(screen, 1, 1);
        HGDIOBJ old = SelectObject(mem, bmp);
        BitBlt(mem, 0, 0, 1, 1, screen, pt.x, pt.y, SRCCOPY | CAPTUREBLT);
        c = GetPixel(mem, 0, 0);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
    }

    ReleaseDC(NULL, screen);

    if (c == CLR_INVALID)
        c = RGB(0, 0, 0);

    return c;
}

static bool EyedropTick(PickerState* st, float col[4], const ImRect& eye_bb) {
    ImGuiContext& g = *GImGui;

    g_picking = true;
    g_picking_frame = g.FrameCount;
    GuiMarkPopupOpen();

    POINT pt;
    GetCursorPos(&pt);
    COLORREF px = EyedropRead(pt);

    st->pick_r = GetRValue(px) / 255.f;
    st->pick_g = GetGValue(px) / 255.f;
    st->pick_b = GetBValue(px) / 255.f;

    const ImVec2 mp = g.IO.MousePos;
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    ImVec2 pc(mp.x + 18.f, mp.y + 18.f);
    fg->AddCircleFilled(ImVec2(pc.x, pc.y + 1.f), 11.f, IM_COL32(0, 0, 0, 90), 32);
    fg->AddCircleFilled(pc, 10.f, IM_COL32(255, 255, 255, 255), 32);
    fg->AddCircleFilled(pc, 7.5f, ImGui::ColorConvertFloat4ToU32(ImVec4(st->pick_r, st->pick_g, st->pick_b, 1.f)), 32);

    const bool settled = g.FrameCount > st->pick_frame + 1;

    const bool a_lmb = KeyDown(VK_LBUTTON);
    const bool a_rmb = KeyDown(VK_RBUTTON);
    const bool a_esc = KeyDown(VK_ESCAPE);

    const bool lmb_click = settled && ((a_lmb && !st->pick_lmb) || g.IO.MouseClicked[0]);
    const bool rmb_click = settled && ((a_rmb && !st->pick_rmb) || g.IO.MouseClicked[1]);
    const bool esc_click = settled && ((a_esc && !st->pick_esc) || IsKeyPressed(ImGuiKey_Escape, false));

    st->pick_lmb = a_lmb;
    st->pick_rmb = a_rmb;
    st->pick_esc = a_esc;

    if (rmb_click || esc_click) {
        EyedropStop(st);
        return false;
    }

    if (!lmb_click)
        return false;

    if (eye_bb.Contains(mp)) {
        EyedropStop(st);
        return false;
    }

    col[0] = st->pick_r;
    col[1] = st->pick_g;
    col[2] = st->pick_b;
    PickerSyncHsv(st, col);
    EyedropStop(st);
    return true;
}

static void PickerDrawKnob(ImDrawList* dl, ImVec2 c, ImU32 fill, ImU32 ring, float win_a) {
    dl->AddCircleFilled(ImVec2(c.x, c.y + 1.f), 7.5f, AlphaCol(IM_COL32(0, 0, 0, 70), win_a), 24);
    dl->AddCircleFilled(c, 7.f, ring, 24);
    dl->AddCircleFilled(c, 4.5f, fill, 24);
}

static void PickerDrawHueBar(ImDrawList* dl, const ImRect& bb, float win_a) {
    static const ImU32 stops[7] = {
        IM_COL32(255, 0, 0, 255),
        IM_COL32(255, 255, 0, 255),
        IM_COL32(0, 255, 0, 255),
        IM_COL32(0, 255, 255, 255),
        IM_COL32(0, 0, 255, 255),
        IM_COL32(255, 0, 255, 255),
        IM_COL32(255, 0, 0, 255)
    };

    const float r = bb.GetHeight() * 0.5f;
    const float x0 = bb.Min.x + r;
    const float x1 = bb.Max.x - r;
    const float cy = bb.GetCenter().y;

    dl->AddCircleFilled(ImVec2(x0, cy), r, AlphaCol(stops[0], win_a), 16);
    dl->AddCircleFilled(ImVec2(x1, cy), r, AlphaCol(stops[6], win_a), 16);

    const float seg = (x1 - x0) / 6.f;
    for (int i = 0; i < 6; i++) {
        ImU32 c0 = AlphaCol(stops[i], win_a);
        ImU32 c1 = AlphaCol(stops[i + 1], win_a);
        dl->AddRectFilledMultiColor(ImVec2(x0 + seg * i, bb.Min.y), ImVec2(x0 + seg * (i + 1), bb.Max.y), c0, c1, c1, c0);
    }
}

static bool PickerValueBox(PickerState* st, int idx, const ImRect& bb, const char* text, ImFont* font, bool hex, bool allow, bool in_window) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    float win_a = g.Style.Alpha;

    PushID(idx);

    const bool hovered = allow && in_window && bb.Contains(g.IO.MousePos);
    const bool editing = (st->edit_idx == idx);

    st->box_hover[idx] = ImClamp(st->box_hover[idx] + Gui->FixedSpeed(14.f) * ((hovered || editing) ? 1.f : -1.f), 0.f, 1.f);

    dl->AddRectFilled(bb.Min, bb.Max, AlphaCol(IM_COL32(24, 25, 29, 255), win_a), 5.f);

    ImVec4 bc = ImLerp(ImVec4(1.f, 1.f, 1.f, 0.08f), ImVec4(Colors->accent.Value.x, Colors->accent.Value.y, Colors->accent.Value.z, 0.85f), editing ? 1.f : st->box_hover[idx] * 0.4f);
    bc.w *= win_a;
    dl->AddRect(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(bc), 5.f, 0, 1.f);

    bool confirmed = false;

    if (editing) {
        ImVec2 backup_cursor_pos = window->DC.CursorPos;

        PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.f));
        PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
        PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.f, 0.f, 0.f, 0.f));
        PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
        PushStyleColor(ImGuiCol_TextSelectedBg, Draw->GetClr(Colors->accent, 0.35f));
        PushStyleColor(ImGuiCol_Text, Colors->color_edit.text.Value);

        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
        PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

        PushFont(font);

        SetCursorScreenPos(ImVec2(bb.Min.x + 5.f, bb.GetCenter().y - font->LegacySize * 0.5f));
        SetNextItemWidth(bb.GetWidth() - 10.f);

        ImGuiInputTextFlags itf = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll;
        if (!hex)
            itf |= ImGuiInputTextFlags_CharsDecimal;

        if (st->edit_focus) {
            SetKeyboardFocusHere();
            st->edit_focus = false;
        }

        bool enter = InputText("##edit", st->edit_buf, IM_ARRAYSIZE(st->edit_buf), itf);
        bool active = IsItemActive();

        PopFont();
        PopStyleVar(3);
        PopStyleColor(6);

        window->DC.CursorPos = backup_cursor_pos;

        const bool settled = g.FrameCount > st->edit_frame + 1;
        const bool clicked_outside = settled && !active && g.IO.MouseClicked[0] && !bb.Contains(g.IO.MousePos);

        if (enter) {
            confirmed = true;
            st->edit_idx = -1;
        }
        else if (IsKeyPressed(ImGuiKey_Escape) || clicked_outside) {
            st->edit_idx = -1;
        }
    }
    else {
        ImVec4 tc = Colors->color_edit.text.Value;
        tc.w = (0.80f + st->box_hover[idx] * 0.20f) * win_a;
        Draw->TextClipped(dl, font, bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(tc), text, nullptr, nullptr, ImVec2(0.5f, 0.5f));

        if (hovered && g.IO.MouseClicked[0]) {
            strncpy_s(st->edit_buf, text, sizeof(st->edit_buf) - 1);
            st->edit_idx = idx;
            st->edit_frame = g.FrameCount;
            st->edit_focus = true;
        }
    }

    PopID();
    return confirmed;
}

bool ColorPicker(const char* label, float col[4], ImGuiColorEditFlags flags, const char* display_name, PickerState* st) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    PushID(label);

    if (!st)
        st = Gui->AnimContainer(&st, GetID("##PickerState"));

    if (!st->synced || (st->drag == 0 && !st->picking && st->rainbow == 0 && st->pulse == 0 && PickerOutOfSync(st, col)))
        PickerSyncHsv(st, col);

    const bool has_alpha = !(flags & ImGuiColorEditFlags_NoAlpha);
    ImDrawList* dl = window->DrawList;
    const float win_a = g.Style.Alpha;
    const ImVec2 mp = g.IO.MousePos;
    const bool in_window = window->Rect().Contains(mp) && (g.HoveredWindow == window || g.HoveredWindow == g_blocker_window || g.HoveredWindow == nullptr);

    ImFont* font = Var->font.inter[5] ? Var->font.inter[5] : Var->font.inter[2];
    ImFont* small_font = Var->font.inter[2] ? Var->font.inter[2] : font;
    ImFont* icon_font = Var->font.fa[0];

    const char* mode_names[3] = { "RGBA", "HEX", "Float" };

    const float total_w = 200.f;
    const float header_h = 22.f;
    const float bar_h = 10.f;
    const float bar_r = bar_h * 0.5f;
    const float row_h = 26.f;
    const float mode_w = 58.f;
    const float gap = 4.f;
    const float item_h = 20.f;

    const ImVec2 origin = window->DC.CursorPos;

    const ImRect eye_bb(ImVec2(origin.x + total_w - header_h, origin.y), ImVec2(origin.x + total_w, origin.y + header_h));
    const ImRect sv_bb(ImVec2(origin.x, origin.y + header_h + 8.f), ImVec2(origin.x + total_w, origin.y + header_h + 8.f + total_w));
    const ImRect hue_bb(ImVec2(origin.x, sv_bb.Max.y + 12.f), ImVec2(origin.x + total_w, sv_bb.Max.y + 12.f + bar_h));
    const ImRect alpha_bb(ImVec2(origin.x, hue_bb.Max.y + 10.f), ImVec2(origin.x + total_w, hue_bb.Max.y + 10.f + bar_h));
    const float row_y = (has_alpha ? alpha_bb.Max.y : hue_bb.Max.y) + 12.f;
    const ImRect mode_bb(ImVec2(origin.x, row_y), ImVec2(origin.x + mode_w, row_y + row_h));
    const float total_h = row_y + row_h - origin.y;

    const ImRect hue_hit(ImVec2(hue_bb.Min.x - 4.f, hue_bb.Min.y - 5.f), ImVec2(hue_bb.Max.x + 4.f, hue_bb.Max.y + 5.f));
    const ImRect alpha_hit(ImVec2(alpha_bb.Min.x - 4.f, alpha_bb.Min.y - 5.f), ImVec2(alpha_bb.Max.x + 4.f, alpha_bb.Max.y + 5.f));

    const float hue_x0 = hue_bb.Min.x + bar_r;
    const float hue_x1 = hue_bb.Max.x - bar_r;
    const float alpha_x0 = alpha_bb.Min.x + bar_r;
    const float alpha_x1 = alpha_bb.Max.x - bar_r;

    const float list_h = item_h * 3.f + 6.f;
    const ImRect list_bb(ImVec2(mode_bb.Min.x, mode_bb.Min.y - 4.f - list_h), ImVec2(mode_bb.Min.x + mode_w + 16.f, mode_bb.Min.y - 4.f));
    ImRect item_bb[3];
    for (int i = 0; i < 3; i++)
        item_bb[i] = ImRect(ImVec2(list_bb.Min.x + 3.f, list_bb.Min.y + 3.f + i * item_h), ImVec2(list_bb.Max.x - 3.f, list_bb.Min.y + 3.f + (i + 1) * item_h));

    Dummy(ImVec2(total_w, total_h));

    bool changed = false;
    bool consumed = false;

    if (st->picking) {
        if (EyedropTick(st, col, eye_bb))
            changed = true;
        consumed = true;
    }

    const bool pick_grace = st->picking || (g.FrameCount - st->pick_frame) <= 2;
    const bool lmb_click = g.IO.MouseClicked[0] && !pick_grace;
    const bool rmb_click = g.IO.MouseClicked[1] && !pick_grace;
    const bool esc = IsKeyPressed(ImGuiKey_Escape) && !pick_grace;

    const bool eye_hovered = (in_window || st->picking) && eye_bb.Contains(mp);

    if (!consumed && !st->picking && eye_hovered && lmb_click && !st->mode_open) {
        EyedropStart(st);
        consumed = true;
    }

    const bool mode_hovered = in_window && mode_bb.Contains(mp) && !st->picking;

    if (st->mode_open) {
        GuiMarkPopupOpen();

        if (!consumed && lmb_click) {
            for (int i = 0; i < 3; i++) {
                if (item_bb[i].Contains(mp)) {
                    st->mode = i;
                    st->edit_idx = -1;
                }
            }
            st->mode_open = false;
            consumed = true;
        }
        else if (rmb_click || esc) {
            st->mode_open = false;
            consumed = true;
        }
    }
    else if (!consumed && mode_hovered && lmb_click) {
        st->mode_open = true;
        st->edit_idx = -1;
        st->drag = 0;
        consumed = true;
    }

    const bool blocked = st->mode_open || pick_grace;

    if (!g.IO.MouseDown[0] || st->picking)
        st->drag = 0;

    if (!consumed && !blocked && lmb_click && in_window && st->drag == 0) {
        if (sv_bb.Contains(mp))
            st->drag = 1;
        else if (hue_hit.Contains(mp))
            st->drag = 2;
        else if (has_alpha && alpha_hit.Contains(mp))
            st->drag = 3;

        if (st->drag) {
            st->edit_idx = -1;
            consumed = true;
        }
    }

    if (!consumed && !blocked && rmb_click && in_window) {
        if (hue_hit.Contains(mp)) {
            st->rainbow = st->rainbow ? 0 : 3;
            consumed = true;
        }
        else if (has_alpha && alpha_hit.Contains(mp)) {
            st->pulse = st->pulse ? 0 : 3;
            consumed = true;
        }
    }

    if (st->drag == 1 && st->rainbow == 0) {
        st->s = ImClamp((mp.x - sv_bb.Min.x) / sv_bb.GetWidth(), 0.f, 1.f);
        st->v = 1.f - ImClamp((mp.y - sv_bb.Min.y) / sv_bb.GetHeight(), 0.f, 1.f);
        st->pre_rainbow_s = st->s;
        st->pre_rainbow_v = st->v;
        ImGui::ColorConvertHSVtoRGB(st->h, st->s, st->v, col[0], col[1], col[2]);
        changed = true;
    }

    if (st->drag == 2 && st->rainbow == 0) {
        st->h = ImClamp((mp.x - hue_x0) / (hue_x1 - hue_x0), 0.f, 0.9999f);
        st->pre_rainbow_h = st->h;
        ImGui::ColorConvertHSVtoRGB(st->h, st->s, st->v, col[0], col[1], col[2]);
        changed = true;
    }

    if (st->drag == 3 && has_alpha && st->pulse == 0) {
        st->a = ImClamp((mp.x - alpha_x0) / (alpha_x1 - alpha_x0), 0.f, 1.f);
        st->pre_pulse_a = st->a;
        col[3] = st->a;
        changed = true;
    }

    st->eye_hover = ImClamp(st->eye_hover + Gui->FixedSpeed(14.f) * (eye_hovered ? 1.f : -1.f), 0.f, 1.f);
    st->pick_a = ImClamp(st->pick_a + Gui->FixedSpeed(14.f) * (st->picking ? 1.f : -1.f), 0.f, 1.f);
    st->mode_hover = ImClamp(st->mode_hover + Gui->FixedSpeed(14.f) * ((mode_hovered || st->mode_open) ? 1.f : -1.f), 0.f, 1.f);
    st->mode_a = ImClamp(st->mode_a + Gui->FixedSpeed(12.f) * (st->mode_open ? 1.f : -1.f), 0.f, 1.f);

    if (font) {
        ImVec4 tc = Colors->color_edit.text.Value;
        tc.w *= win_a;
        Draw->TextClipped(dl, font, origin, ImVec2(eye_bb.Min.x - 6.f, origin.y + header_h), ImGui::ColorConvertFloat4ToU32(tc), display_name ? display_name : "Color", nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    {
        ImVec4 bg = ImLerp(ImVec4(1.f, 1.f, 1.f, 0.06f + st->eye_hover * 0.05f), ImVec4(Colors->accent.Value.x, Colors->accent.Value.y, Colors->accent.Value.z, 0.25f), st->pick_a);
        bg.w *= win_a;
        dl->AddRectFilled(eye_bb.Min, eye_bb.Max, ImGui::ColorConvertFloat4ToU32(bg), 5.f);

        if (icon_font) {
            const float fs = icon_font->LegacySize * 0.6f;
            ImVec2 isz = icon_font->CalcTextSizeA(fs, FLT_MAX, -1.f, ICON_FA_EYE_DROPPER);
            ImVec4 ic = ImLerp(ImVec4(0.72f, 0.72f, 0.76f, 1.f), Colors->accent.Value, ImMax(st->eye_hover * 0.6f, st->pick_a));
            ic.w = win_a;
            dl->AddText(icon_font, fs, eye_bb.GetCenter() - isz * 0.5f, ImGui::ColorConvertFloat4ToU32(ic), ICON_FA_EYE_DROPPER);
        }
    }

    ImVec4 hue_col_f(1.f, 1.f, 1.f, 1.f);
    ImGui::ColorConvertHSVtoRGB(st->h, 1.f, 1.f, hue_col_f.x, hue_col_f.y, hue_col_f.z);
    const ImU32 hue32 = AlphaCol(ImGui::ColorConvertFloat4ToU32(hue_col_f), win_a);
    const ImU32 white32 = AlphaCol(IM_COL32(255, 255, 255, 255), win_a);
    const ImU32 black32 = AlphaCol(IM_COL32(0, 0, 0, 255), win_a);
    const ImU32 rgb32 = AlphaCol(ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 1.f)), win_a);

    Draw->RectFilledMultiColor(dl, sv_bb.Min, sv_bb.Max, white32, hue32, hue32, white32, 6.f);
    Draw->RectFilledMultiColor(dl, sv_bb.Min, sv_bb.Max, 0, 0, black32, black32, 6.f);

    {
        ImVec2 cur_p(sv_bb.Min.x + st->s * sv_bb.GetWidth(), sv_bb.Min.y + (1.f - st->v) * sv_bb.GetHeight());
        dl->PushClipRectFullScreen();
        dl->AddCircle(cur_p, 7.f, AlphaCol(IM_COL32(0, 0, 0, 70), win_a), 24, 4.f);
        dl->AddCircle(cur_p, 7.f, white32, 24, 2.5f);
        dl->PopClipRect();
    }

    {
        PickerDrawHueBar(dl, hue_bb, win_a);

        ImVec2 hk(hue_x0 + st->h * (hue_x1 - hue_x0), hue_bb.GetCenter().y);
        ImU32 ring = st->rainbow ? AlphaCol(Draw->GetClr(Colors->accent), win_a) : white32;
        dl->PushClipRectFullScreen();
        PickerDrawKnob(dl, hk, hue32, ring, win_a);
        dl->PopClipRect();
    }

    if (has_alpha) {
        ImU32 base = AlphaCol(IM_COL32(22, 24, 30, 255), win_a);
        ImU32 c0 = AlphaCol(ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0.f)), win_a);
        ImU32 c1 = AlphaCol(ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 1.f)), win_a);

        dl->AddRectFilled(alpha_bb.Min, alpha_bb.Max, base, bar_r);
        dl->AddCircleFilled(ImVec2(alpha_x1, alpha_bb.GetCenter().y), bar_r, c1, 16);
        dl->AddRectFilledMultiColor(ImVec2(alpha_x0, alpha_bb.Min.y), ImVec2(alpha_x1, alpha_bb.Max.y), c0, c1, c1, c0);

        ImVec2 ak(alpha_x0 + st->a * (alpha_x1 - alpha_x0), alpha_bb.GetCenter().y);
        ImU32 ring = st->pulse ? AlphaCol(Draw->GetClr(Colors->accent), win_a) : white32;
        dl->PushClipRectFullScreen();
        PickerDrawKnob(dl, ak, rgb32, ring, win_a);
        dl->PopClipRect();
    }

    {
        dl->AddRectFilled(mode_bb.Min, mode_bb.Max, AlphaCol(IM_COL32(255, 255, 255, (int)(10.f + st->mode_hover * 10.f)), win_a), 5.f);
        dl->AddRect(mode_bb.Min, mode_bb.Max, AlphaCol(IM_COL32(255, 255, 255, 20), win_a), 5.f, 0, 1.f);

        if (small_font) {
            ImVec4 tc = Colors->color_edit.text.Value;
            tc.w = (0.75f + st->mode_hover * 0.25f) * win_a;
            Draw->TextClipped(dl, small_font, ImVec2(mode_bb.Min.x + 7.f, mode_bb.Min.y), ImVec2(mode_bb.Max.x - 16.f, mode_bb.Max.y), ImGui::ColorConvertFloat4ToU32(tc), mode_names[st->mode], nullptr, nullptr, ImVec2(0.f, 0.5f));
        }

        if (icon_font) {
            const float fs = icon_font->LegacySize * 0.4f;
            ImVec2 isz = icon_font->CalcTextSizeA(fs, FLT_MAX, -1.f, ICON_FA_CHEVRON_DOWN);
            ImVec2 ip(mode_bb.Max.x - 7.f - isz.x, mode_bb.GetCenter().y - isz.y * 0.5f);
            dl->AddText(icon_font, fs, ip, AlphaCol(IM_COL32(150, 150, 156, 255), win_a), ICON_FA_CHEVRON_DOWN);
        }
    }

    char vals[4][32]{};
    int box_count = 0;

    if (st->mode == 0) {
        box_count = has_alpha ? 4 : 3;
        for (int i = 0; i < box_count; i++)
            snprintf(vals[i], sizeof(vals[i]), "%d", IM_F32_TO_INT8_SAT(col[i]));
    }
    else if (st->mode == 1) {
        box_count = 1;
        PickerFormatHex(vals[0], sizeof(vals[0]), col, has_alpha);
    }
    else {
        box_count = has_alpha ? 4 : 3;
        for (int i = 0; i < box_count; i++)
            snprintf(vals[i], sizeof(vals[i]), "%.2f", col[i]);
    }

    if (small_font) {
        const float boxes_x = mode_bb.Max.x + gap;
        const float boxes_w = origin.x + total_w - boxes_x;
        const float box_w = (boxes_w - gap * (box_count - 1)) / (float)box_count;
        const bool allow = !consumed && !blocked && st->drag == 0;

        for (int i = 0; i < box_count; i++) {
            float bx = boxes_x + i * (box_w + gap);
            ImRect bb(ImVec2(bx, row_y), ImVec2(bx + box_w, row_y + row_h));

            if (PickerValueBox(st, i, bb, vals[i], small_font, st->mode == 1, allow, in_window)) {
                bool ok = false;

                if (st->mode == 0) {
                    col[i] = ImClamp(atoi(st->edit_buf), 0, 255) / 255.f;
                    ok = true;
                }
                else if (st->mode == 1) {
                    ok = PickerParseHex(st->edit_buf, col, has_alpha);
                }
                else {
                    col[i] = ImClamp((float)atof(st->edit_buf), 0.f, 1.f);
                    ok = true;
                }

                if (ok) {
                    PickerSyncHsv(st, col);
                    changed = true;
                }
            }
        }
    }

    if (in_window && g.IO.KeyCtrl && st->edit_idx < 0 && !blocked) {
        if (IsKeyPressed(ImGuiKey_C)) {
            char hex[32];
            PickerFormatHex(hex, sizeof(hex), col, has_alpha);
            ImGui::SetClipboardText(hex);
        }

        if (IsKeyPressed(ImGuiKey_V)) {
            if (PickerParseHex(ImGui::GetClipboardText(), col, has_alpha)) {
                PickerSyncHsv(st, col);
                changed = true;
            }
        }
    }

    if (st->mode_a > 0.01f) {
        const float ease = st->mode_a * st->mode_a * (3.f - 2.f * st->mode_a);
        const ImVec2 off(0.f, (1.f - ease) * 4.f);

        dl->PushClipRectFullScreen();
        dl->AddRectFilled(list_bb.Min + off - ImVec2(1.f, 0.f), list_bb.Max + off + ImVec2(1.f, 3.f), IM_COL32(0, 0, 0, (int)(70.f * ease * win_a)), 8.f);
        dl->AddRectFilled(list_bb.Min + off, list_bb.Max + off, GetPickerBgColor(0.98f * ease * win_a), 6.f);
        dl->AddRect(list_bb.Min + off, list_bb.Max + off, AlphaCol(IM_COL32(255, 255, 255, 18), ease * win_a), 6.f, 0, 1.f);

        for (int i = 0; i < 3; i++) {
            const ImRect ib(item_bb[i].Min + off, item_bb[i].Max + off);
            const bool ih = st->mode_open && in_window && ib.Contains(mp);

            st->item_hover[i] = ImClamp(st->item_hover[i] + Gui->FixedSpeed(14.f) * (ih ? 1.f : -1.f), 0.f, 1.f);

            if (st->item_hover[i] > 0.01f)
                dl->AddRectFilled(ib.Min, ib.Max, AlphaCol(IM_COL32(255, 255, 255, (int)(14.f * st->item_hover[i])), ease * win_a), 4.f);

            if (small_font) {
                ImVec4 tc = (st->mode == i) ? Colors->accent.Value : Colors->color_edit.text.Value;
                tc.w = ((st->mode == i) ? 1.f : (0.65f + st->item_hover[i] * 0.35f)) * ease * win_a;
                Draw->TextClipped(dl, small_font, ImVec2(ib.Min.x + 6.f, ib.Min.y), ib.Max, ImGui::ColorConvertFloat4ToU32(tc), mode_names[i], nullptr, nullptr, ImVec2(0.f, 0.5f));
            }
        }
        dl->PopClipRect();
    }

    SetCursorScreenPos(ImVec2(origin.x, origin.y + total_h));

    PopID();
    return changed;
}

bool ColorPicker(const char* label, float col[4], ImGuiColorEditFlags flags, const char* display_name) {
    return ColorPicker(label, col, flags, display_name, nullptr);
}

bool CGui::ColorEdit(std::string_view label, Color_t** colors, int color_count, bool alpha) {
    struct SwState {
        bool open = false;
        float a = 0.f;
        ImVec2 anchor = {};
        ImRect popup_rect = ImRect(0, 0, 0, 0);
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const float avail = GetContentRegionAvail().x;
    const float row_h = ImMax(Elements->color_edit.height, 22.f);

    const float swatch_sz = 18.f;
    const float swatch_gap = 6.f;
    const float swatch_r = 5.f;
    const float right_pad = 0.f;

    color_count = ImClamp(color_count, 0, 3);
    float right_used = right_pad + color_count * (swatch_sz + swatch_gap);

    const ImRect rect(pos, ImVec2(pos.x + avail, pos.y + row_h));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    const bool hovered = ImGui::IsMouseHoveringRect(rect.Min, rect.Max) && g.HoveredWindow == window;

    if (Var->font.inter[5]) {
        float label_right = pos.x + avail - right_used;
        ImVec4 tc = Colors->color_edit.text.Value;
        tc.w = hovered ? 1.0f : 0.65f;
        Draw->TextClipped(window->DrawList, Var->font.inter[5], ImVec2(pos.x, pos.y), ImVec2(label_right, pos.y + row_h), Draw->GetClr(ImColor(tc)), label.data(), nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    float rx = pos.x + avail - right_pad;
    float sw_y = pos.y + (row_h - swatch_sz) * 0.5f;
    bool changed = false;

    const bool is_about_owner = Gui->about_active && Gui->about_draw_list && window->DrawList == Gui->about_draw_list;

    for (int i = color_count - 1; i >= 0; --i) {
        if (!colors[i])
            continue;

        rx -= swatch_sz;
        ImRect sw_bb(ImVec2(rx, sw_y), ImVec2(rx + swatch_sz, sw_y + swatch_sz));

        std::string sw_uid = std::string(label.data()) + "_sw_edit_" + std::to_string(i);
        ImGuiID sw_id = window->GetID(sw_uid.c_str());

        if (!ItemAdd(sw_bb, sw_id)) {
            rx -= swatch_gap;
            continue;
        }

        bool sw_hovered, sw_held;
        bool sw_pressed = ImGui::ButtonBehavior(sw_bb, sw_id, &sw_hovered, &sw_held);

        ImVec4 cv(colors[i]->Rf(), colors[i]->Gf(), colors[i]->Bf(), alpha ? colors[i]->Af() : 1.f);
        ImU32 col_u32 = ImGui::ColorConvertFloat4ToU32(cv);

        if (alpha && colors[i]->Af() < 0.99f)
            RenderColorRectWithAlphaCheckboard(window->DrawList, sw_bb.Min, sw_bb.Max, IM_COL32(0, 0, 0, 0), sw_bb.GetWidth() / 4.f, ImVec2(0, 0), swatch_r, 0);

        window->DrawList->AddRectFilled(sw_bb.Min, sw_bb.Max, col_u32, swatch_r);

        if (sw_hovered)
            window->DrawList->AddRect(sw_bb.Min, sw_bb.Max, IM_COL32(255, 255, 255, 35), swatch_r, 0, 1.f);

        SwState* st = Gui->AnimContainer(&st, sw_id);
        PickerState* p_st = Gui->AnimContainer(&p_st, sw_id + 0x1337);

        RegisterPicker(p_st, colors[i]);

        const bool picking = p_st->picking;
        const bool pick_lock = picking || (g.FrameCount - p_st->pick_frame) <= 2;

        bool swatch_just_toggled = false;

        if (picking)
            st->open = true;

        if (sw_pressed && !st->open && !pick_lock) {
            st->open = true;
            swatch_just_toggled = true;
            st->anchor = ImVec2(sw_bb.Max.x, sw_bb.Max.y + 6.f);
        }

        if (st->open) {
            GuiMarkPopupOpen();

            Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
            Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

            ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(g.IO.DisplaySize, ImGuiCond_Always);

            std::string blocker_name = std::string("##swatch_blocker_") + sw_uid;
            ImGuiWindowFlags bf = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing;

            if (ImGui::Begin(blocker_name.c_str(), nullptr, bf)) {
                ImGuiWindow* blocker = GetCurrentWindow();
                g_blocker_window = blocker;
                ImGui::BringWindowToDisplayFront(blocker);

                if (!swatch_just_toggled && !pick_lock && g.IO.MouseClicked[0] && g.HoveredWindow == blocker && !st->popup_rect.Contains(g.IO.MousePos))
                    st->open = false;
            }
            ImGui::End();

            Gui->PopStyleVar(2);
        }

        if (!ImGui::IsRectVisible(sw_bb.Min, sw_bb.Max + ImVec2(0, 2)) && !pick_lock) {
            st->open = false;
            st->a = 0.f;
        }

        st->a = ImClamp(st->a + Gui->FixedSpeed(12.f) * (st->open ? 1.f : -1.f), 0.f, 1.f);

        if (!st->open) {
            p_st->mode_open = false;
            p_st->edit_idx = -1;
            p_st->drag = 0;
            if (p_st->picking)
                EyedropStop(p_st);
        }

        if (st->a >= 0.01f) {
            const float anim_ease = st->a * st->a * (3.f - 2.f * st->a);

            Gui->PushStyleVar(ImGuiStyleVar_Alpha, anim_ease);
            Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
            Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
            Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 12.f));

            Gui->PushStyleColor(ImGuiCol_WindowBg, GetPickerBgColor(0.98f));
            Gui->PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 24));

            ImVec2 popup_pos = st->anchor + ImVec2(0.f, (1.f - anim_ease) * -6.f);
            ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

            std::string overlay_name = std::string("##swatch_overlay_") + sw_uid;

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

            if (swatch_just_toggled && !picking)
                ImGui::SetNextWindowFocus();

            if (!st->open)
                flags |= ImGuiWindowFlags_NoInputs;

            if (ImGui::Begin(overlay_name.c_str(), nullptr, flags)) {
                ImGuiWindow* popup_win = GetCurrentWindow();
                st->popup_rect = ImRect(popup_win->Pos, popup_win->Pos + popup_win->Size);

                ImGui::BringWindowToDisplayFront(popup_win);

                if (is_about_owner) {
                    Gui->about_extra_windows.push_back(popup_win);
                    Gui->about_extra_draw_lists.push_back(popup_win->DrawList);
                }

                float tmp[4] = { colors[i]->Rf(), colors[i]->Gf(), colors[i]->Bf(), alpha ? colors[i]->Af() : 1.f };
                ImGuiColorEditFlags fl = ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview;

                if (alpha)
                    fl |= ImGuiColorEditFlags_AlphaBar;
                else
                    fl |= ImGuiColorEditFlags_NoAlpha;

                if (ColorPicker("##sp", tmp, fl, "Color", p_st)) {
                    *colors[i] = Color_t(tmp[0], tmp[1], tmp[2], alpha ? tmp[3] : colors[i]->A());
                    changed = true;
                }
            }
            ImGui::End();

            Gui->PopStyleVar(4);
            Gui->PopStyleColor(2);
        }

        rx -= swatch_gap;
    }

    return changed;
}