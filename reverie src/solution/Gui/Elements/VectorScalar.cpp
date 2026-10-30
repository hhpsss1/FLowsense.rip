#include <shared.h>
#include "../settings/Functions.h"
#include <string>

struct VectorScalarState {
    int edit_idx = -1;
    int edit_frame = 0;
    bool edit_focus = false;
    char edit_buf[32]{};
    float box_hover[3]{};
    float canvas_hover = 0.f;
    float axis_hover[3]{};
    float axis_active_anim[3]{};
    int drag_axis = 0;
    float cam_yaw = -0.75f;
    float cam_pitch = 0.48f;
    bool init = false;
};

static ImU32 AlphaCol(ImU32 col, float alpha) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(col);
    c.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}

static float NormToVal(float norm, float v_min, float v_max) {
    return v_min + (norm + 1.f) * 0.5f * (v_max - v_min);
}

static float ValToNorm(float val, float v_min, float v_max) {
    if (fabsf(v_max - v_min) < 0.0001f)
        return 0.f;
    return ImClamp((val - v_min) / (v_max - v_min) * 2.f - 1.f, -1.f, 1.f);
}

static void Draw3DArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 col, float thickness = 1.5f, float arrow_size = 6.5f) {
    ImVec2 dir = to - from;
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.001f)
        return;

    dir.x /= len;
    dir.y /= len;

    ImVec2 normal(-dir.y, dir.x);
    ImVec2 base = to - dir * arrow_size;

    dl->AddLine(from, base, col, thickness);

    ImVec2 p1 = to;
    ImVec2 p2 = base + normal * (arrow_size * 0.55f);
    ImVec2 p3 = base - normal * (arrow_size * 0.55f);
    dl->AddTriangleFilled(p1, p2, p3, col);
}

static bool VectorValueBox(VectorScalarState* st, int idx, const ImRect& bb, const char* label_prefix, ImU32 axis_col, float* val, float v_min, float v_max, ImFont* font, bool in_window, float win_a) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImDrawList* dl = window->DrawList;

    PushID(idx);

    const bool hovered = in_window && bb.Contains(g.IO.MousePos) && st->drag_axis == 0;
    const bool editing = (st->edit_idx == idx);

    st->box_hover[idx] = ImClamp(st->box_hover[idx] + Gui->FixedSpeed(14.f) * ((hovered || editing) ? 1.f : -1.f), 0.f, 1.f);

    dl->AddRectFilled(bb.Min, bb.Max, AlphaCol(IM_COL32(24, 25, 29, 255), win_a), 5.f);

    ImVec4 bc = ImLerp(ImVec4(1.f, 1.f, 1.f, 0.08f), ImVec4(Colors->accent.Value.x, Colors->accent.Value.y, Colors->accent.Value.z, 0.85f), editing ? 1.f : st->box_hover[idx] * 0.4f);
    bc.w *= win_a;
    dl->AddRect(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(bc), 5.f, 0, 1.f);

    const float dot_r = 2.5f;
    const ImVec2 dot_pos(bb.Min.x + 10.f, bb.GetCenter().y);
    dl->AddCircleFilled(dot_pos, dot_r, AlphaCol(axis_col, win_a));

    bool confirmed = false;
    const float text_start_x = bb.Min.x + 18.f;

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

        SetCursorScreenPos(ImVec2(text_start_x, bb.GetCenter().y - font->LegacySize * 0.5f));
        SetNextItemWidth(bb.Max.x - text_start_x - 4.f);

        ImGuiInputTextFlags itf = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CharsDecimal;

        if (st->edit_focus) {
            SetKeyboardFocusHere();
            st->edit_focus = false;
        }

        bool enter = InputText(xs("##edit"), st->edit_buf, IM_ARRAYSIZE(st->edit_buf), itf);
        bool active = IsItemActive();

        PopFont();
        PopStyleVar(3);
        PopStyleColor(6);

        window->DC.CursorPos = backup_cursor_pos;

        const bool settled = g.FrameCount > st->edit_frame + 1;
        const bool clicked_outside = settled && !active && g.IO.MouseClicked[0] && !bb.Contains(g.IO.MousePos);

        if (enter) {
            *val = ImClamp((float)atof(st->edit_buf), v_min, v_max);
            confirmed = true;
            st->edit_idx = -1;
        } else if (IsKeyPressed(ImGuiKey_Escape) || clicked_outside) {
            st->edit_idx = -1;
        }
    } else {
        char text[32];
        snprintf(text, sizeof(text), xs("%s: %.1f"), label_prefix, *val);

        ImVec4 tc = Colors->color_edit.text.Value;
        tc.w = (0.75f + st->box_hover[idx] * 0.25f) * win_a;
        Draw->TextClipped(dl, font, ImVec2(text_start_x, bb.Min.y), ImVec2(bb.Max.x - 2.f, bb.Max.y), ImGui::ColorConvertFloat4ToU32(tc), text, nullptr, nullptr, ImVec2(0.f, 0.5f));

        if (hovered && g.IO.MouseClicked[0]) {
            snprintf(st->edit_buf, sizeof(st->edit_buf), xs("%.2f"), *val);
            st->edit_idx = idx;
            st->edit_frame = g.FrameCount;
            st->edit_focus = true;
        }
    }

    PopID();
    return confirmed;
}

bool CGui::VectorScalar(std::string_view label, Vector* v, float min_x, float max_x, float min_y, float max_y, float min_z, float max_z) {
    if (!v)
        return false;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const float avail = GetContentRegionAvail().x;
    const float win_a = style.Alpha;

    const float header_h = 20.f;
    const float canvas_h = 180.f;
    const float gap = 6.f;
    const float row_h = 24.f;
    const float total_h = header_h + 4.f + canvas_h + gap + row_h;

    const ImRect total_bb(pos, ImVec2(pos.x + avail, pos.y + total_h));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
        return false;

    VectorScalarState* st = Gui->AnimContainer(&st, id);
    if (!st->init) {
        st->cam_yaw = -0.75f;
        st->cam_pitch = 0.48f;
        st->init = true;
    }

    ImDrawList* dl = window->DrawList;
    ImFont* font_regular = Var->font.inter[5] ? Var->font.inter[5] : Var->font.inter[0];
    ImFont* font_small = Var->font.inter[2] ? Var->font.inter[2] : font_regular;

    const ImRect canvas_bb(ImVec2(pos.x, pos.y + header_h + 4.f), ImVec2(pos.x + avail, pos.y + header_h + 4.f + canvas_h));
    const ImVec2 mp = g.IO.MousePos;
    const bool in_window = window->Rect().Contains(mp) && g.HoveredWindow == window;
    const bool canvas_hovered = in_window && canvas_bb.Contains(mp) && st->edit_idx < 0;

    const ImVec2 center = ImVec2(canvas_bb.Min.x + canvas_bb.GetWidth() * 0.5f, canvas_bb.Min.y + canvas_bb.GetHeight() * 0.52f);
    const float scale = canvas_bb.GetHeight() * 0.40f;

    auto Project = [&](float x, float y, float z) -> ImVec2 {
        float x1 = x * cosf(st->cam_yaw) - y * sinf(st->cam_yaw);
        float y1 = x * sinf(st->cam_yaw) + y * cosf(st->cam_yaw);
        float z1 = z;

        float x2 = x1;
        float z2 = y1 * sinf(st->cam_pitch) + z1 * cosf(st->cam_pitch);

        return ImVec2(center.x + x2 * scale, center.y - z2 * scale);
    };

    float norm_x = ValToNorm(v->x, min_x, max_x);
    float norm_y = ValToNorm(v->y, min_y, max_y);
    float norm_z = ValToNorm(v->z, min_z, max_z);

    const ImVec2 pt_origin = Project(0.f, 0.f, 0.f);
    const ImVec2 pt_dir_x = Project(1.f, 0.f, 0.f) - pt_origin;
    const ImVec2 pt_dir_y = Project(0.f, 1.f, 0.f) - pt_origin;
    const ImVec2 pt_dir_z = Project(0.f, 0.f, 1.f) - pt_origin;

    const ImVec2 pt_knob_x = pt_origin + pt_dir_x * norm_x;
    const ImVec2 pt_knob_y = pt_origin + pt_dir_y * norm_y;
    const ImVec2 pt_knob_z = pt_origin + pt_dir_z * norm_z;
    const ImVec2 pt_tip = Project(norm_x, norm_y, norm_z);

    auto GetAxisParam = [&](const ImVec2& dir, const ImVec2& point) -> float {
        float len_sq = dir.x * dir.x + dir.y * dir.y;
        if (len_sq < 0.0001f)
            return 0.f;

        ImVec2 delta = point - pt_origin;
        return ImClamp((delta.x * dir.x + delta.y * dir.y) / len_sq, -1.f, 1.f);
    };

    auto DistToAxisLine = [&](const ImVec2& dir, const ImVec2& point) -> float {
        float t = GetAxisParam(dir, point);
        ImVec2 proj = pt_origin + dir * t;
        return sqrtf(ImLengthSqr(point - proj));
    };

    int hovered_axis = 0;
    if (canvas_hovered) {
        float d_kx = sqrtf(ImLengthSqr(mp - pt_knob_x));
        float d_ky = sqrtf(ImLengthSqr(mp - pt_knob_y));
        float d_kz = sqrtf(ImLengthSqr(mp - pt_knob_z));

        float d_lx = DistToAxisLine(pt_dir_x, mp);
        float d_ly = DistToAxisLine(pt_dir_y, mp);
        float d_lz = DistToAxisLine(pt_dir_z, mp);

        float score_x = ImMin(d_kx * 0.75f, d_lx);
        float score_y = ImMin(d_ky * 0.75f, d_ly);
        float score_z = ImMin(d_kz * 0.75f, d_lz);

        float min_score = ImMin(score_x, ImMin(score_y, score_z));
        if (min_score <= 11.f) {
            if (min_score == score_x)
                hovered_axis = 1;
            else if (min_score == score_y)
                hovered_axis = 2;
            else
                hovered_axis = 3;
        }
    }

    bool changed = false;

    if (canvas_hovered && st->drag_axis == 0) {
        if (g.IO.MouseClicked[1]) {
            st->drag_axis = 4;
            SetActiveID(id, window);
            SetFocusID(id, window);
        } else if (g.IO.MouseClicked[0]) {
            if (hovered_axis != 0) {
                st->drag_axis = hovered_axis;
                if (st->drag_axis == 1) {
                    norm_x = GetAxisParam(pt_dir_x, mp);
                    v->x = NormToVal(norm_x, min_x, max_x);
                    changed = true;
                } else if (st->drag_axis == 2) {
                    norm_y = GetAxisParam(pt_dir_y, mp);
                    v->y = NormToVal(norm_y, min_y, max_y);
                    changed = true;
                } else if (st->drag_axis == 3) {
                    norm_z = GetAxisParam(pt_dir_z, mp);
                    v->z = NormToVal(norm_z, min_z, max_z);
                    changed = true;
                }
            } else {
                st->drag_axis = 4;
            }
            SetActiveID(id, window);
            SetFocusID(id, window);
        }
    }

    if (g.ActiveId == id) {
        if (!g.IO.MouseDown[0] && !g.IO.MouseDown[1]) {
            st->drag_axis = 0;
            ClearActiveID();
        } else {
            if (st->drag_axis == 4) {
                const ImVec2 d = g.IO.MouseDelta;
                st->cam_yaw += d.x * 0.012f;
                st->cam_pitch = ImClamp(st->cam_pitch + d.y * 0.012f, 0.12f, 1.40f);
            } else if (st->drag_axis == 1) {
                norm_x = GetAxisParam(pt_dir_x, mp);
                v->x = NormToVal(norm_x, min_x, max_x);
                changed = true;
            } else if (st->drag_axis == 2) {
                norm_y = GetAxisParam(pt_dir_y, mp);
                v->y = NormToVal(norm_y, min_y, max_y);
                changed = true;
            } else if (st->drag_axis == 3) {
                norm_z = GetAxisParam(pt_dir_z, mp);
                v->z = NormToVal(norm_z, min_z, max_z);
                changed = true;
            }
        }
    }

    st->canvas_hover = ImClamp(st->canvas_hover + FixedSpeed(12.f) * (canvas_hovered ? 1.f : -1.f), 0.f, 1.f);
    for (int i = 0; i < 3; i++) {
        bool is_hov = (hovered_axis == (i + 1)) && (st->drag_axis == 0);
        bool is_act = (st->drag_axis == (i + 1));
        st->axis_hover[i] = ImClamp(st->axis_hover[i] + FixedSpeed(14.f) * ((is_hov || is_act) ? 1.f : -1.f), 0.f, 1.f);
        st->axis_active_anim[i] = ImClamp(st->axis_active_anim[i] + FixedSpeed(16.f) * (is_act ? 1.f : -1.f), 0.f, 1.f);
    }

    if (font_regular) {
        ImVec4 tc = Colors->color_edit.text.Value;
        tc.w = (0.70f + st->canvas_hover * 0.30f) * win_a;
        Draw->TextClipped(dl, font_regular, pos, ImVec2(pos.x + avail, pos.y + header_h), ImGui::ColorConvertFloat4ToU32(tc), label.data(), nullptr, nullptr, ImVec2(0.f, 0.5f));
    }

    dl->AddRectFilled(canvas_bb.Min, canvas_bb.Max, AlphaCol(IM_COL32(20, 21, 25, 255), win_a), 6.f);

    ImVec4 border_col = ImLerp(ImVec4(1.f, 1.f, 1.f, 0.06f), ImVec4(Colors->accent.Value.x, Colors->accent.Value.y, Colors->accent.Value.z, 0.45f), st->canvas_hover);
    border_col.w *= win_a;
    dl->AddRect(canvas_bb.Min, canvas_bb.Max, ImGui::ColorConvertFloat4ToU32(border_col), 6.f, 0, 1.f);

    dl->PushClipRect(canvas_bb.Min + ImVec2(1.f, 1.f), canvas_bb.Max - ImVec2(1.f, 1.f), true);

    ImVec2 poly[4] = {
        Project(-1.f, -1.f, 0.f),
        Project(1.f, -1.f, 0.f),
        Project(1.f, 1.f, 0.f),
        Project(-1.f, 1.f, 0.f)
    };
    dl->AddConvexPolyFilled(poly, 4, AlphaCol(IM_COL32(26, 28, 35, 120), win_a));
    dl->AddPolyline(poly, 4, AlphaCol(IM_COL32(255, 255, 255, 16), win_a), ImDrawFlags_Closed, 1.f);

    const int grid_cnt = 2;
    for (int i = -grid_cnt; i <= grid_cnt; i++) {
        float f = (float)i / (float)grid_cnt;
        dl->AddLine(Project(-1.f, f, 0.f), Project(1.f, f, 0.f), AlphaCol(IM_COL32(255, 255, 255, 12), win_a), 1.f);
        dl->AddLine(Project(f, -1.f, 0.f), Project(f, 1.f, 0.f), AlphaCol(IM_COL32(255, 255, 255, 12), win_a), 1.f);
    }

    auto DrawDashedAxis = [&](ImVec2 from, ImVec2 to, ImU32 col) {
        const int segs = 6;
        for (int i = 0; i < segs; i += 2) {
            float t0 = (float)i / (float)segs;
            float t1 = (float)(i + 1) / (float)segs;
            dl->AddLine(ImLerp(from, to, t0), ImLerp(from, to, t1), col, 1.f);
        }
    };

    DrawDashedAxis(pt_origin, Project(-1.f, 0.f, 0.f), AlphaCol(IM_COL32(235, 75, 75, 50), win_a));
    DrawDashedAxis(pt_origin, Project(0.f, -1.f, 0.f), AlphaCol(IM_COL32(80, 210, 110, 50), win_a));
    DrawDashedAxis(pt_origin, Project(0.f, 0.f, -1.f), AlphaCol(IM_COL32(70, 150, 245, 50), win_a));

    const ImVec2 end_x = pt_origin + pt_dir_x;
    const ImVec2 end_y = pt_origin + pt_dir_y;
    const ImVec2 end_z = pt_origin + pt_dir_z;

    const ImU32 col_axis_x = IM_COL32(235, 75, 75, 255);
    const ImU32 col_axis_y = IM_COL32(80, 210, 110, 255);
    const ImU32 col_axis_z = IM_COL32(70, 150, 245, 255);

    auto DrawAxisTrack = [&](const ImVec2& end, ImU32 col, float hover, float act) {
        float thick = 1.6f + hover * 1.4f;
        float arrow_sz = 6.5f + hover * 1.5f;
        ImU32 c = AlphaCol(col, (0.50f + hover * 0.45f) * win_a);
        if (act > 0.01f)
            dl->AddLine(pt_origin, end, AlphaCol(col, 0.35f * act * win_a), thick + 3.f);
        Draw3DArrow(dl, pt_origin, end, c, thick, arrow_sz);
    };

    DrawAxisTrack(end_x, col_axis_x, st->axis_hover[0], st->axis_active_anim[0]);
    DrawAxisTrack(end_y, col_axis_y, st->axis_hover[1], st->axis_active_anim[1]);
    DrawAxisTrack(end_z, col_axis_z, st->axis_hover[2], st->axis_active_anim[2]);

    if (font_small) {
        dl->AddText(font_small, font_small->LegacySize * 0.85f, end_x + ImVec2(5.f, -4.f), AlphaCol(col_axis_x, (0.80f + st->axis_hover[0] * 0.20f) * win_a), xs("+X"));
        dl->AddText(font_small, font_small->LegacySize * 0.85f, end_y + ImVec2(5.f, -4.f), AlphaCol(col_axis_y, (0.80f + st->axis_hover[1] * 0.20f) * win_a), xs("+Y"));
        dl->AddText(font_small, font_small->LegacySize * 0.85f, end_z + ImVec2(-6.f, -15.f), AlphaCol(col_axis_z, (0.80f + st->axis_hover[2] * 0.20f) * win_a), xs("+Z"));
    }

    const ImVec2 pt_ground = Project(norm_x, norm_y, 0.f);
    dl->AddLine(pt_knob_x, pt_ground, AlphaCol(col_axis_x, 60), 1.f);
    dl->AddLine(pt_knob_y, pt_ground, AlphaCol(col_axis_y, 60), 1.f);

    dl->AddCircleFilled(pt_ground, 3.5f, AlphaCol(IM_COL32(0, 0, 0, 140), win_a));
    dl->AddCircleFilled(pt_ground, 2.5f, AlphaCol(Draw->GetClr(Colors->accent), 0.70f * win_a));

    if (norm_z >= 0.f)
        dl->AddLine(pt_ground, pt_tip, AlphaCol(IM_COL32(255, 255, 255, 65), win_a), 1.f);
    else
        DrawDashedAxis(pt_ground, pt_tip, AlphaCol(IM_COL32(255, 255, 255, 50), win_a));

    const ImU32 stem_glow = AlphaCol(Draw->GetClr(Colors->accent), 0.25f * win_a);
    const ImU32 stem_col = AlphaCol(Draw->GetClr(Colors->accent), 0.85f * win_a);
    dl->AddLine(pt_origin, pt_tip, stem_glow, 3.5f);
    Draw3DArrow(dl, pt_origin, pt_tip, stem_col, 2.0f, 8.f);

    auto RenderAxisKnob = [&](const ImVec2& pt, ImU32 col, float hover, float act) {
        float r = 4.2f + hover * 1.5f + act * 0.8f;
        dl->AddCircleFilled(pt + ImVec2(0.f, 1.f), r + 2.f, AlphaCol(IM_COL32(0, 0, 0, 110), win_a), 16);
        dl->AddCircleFilled(pt, r + 1.f, AlphaCol(IM_COL32(255, 255, 255, 255), win_a), 16);
        dl->AddCircleFilled(pt, r, AlphaCol(col, win_a), 16);
    };

    RenderAxisKnob(pt_knob_x, col_axis_x, st->axis_hover[0], st->axis_active_anim[0]);
    RenderAxisKnob(pt_knob_y, col_axis_y, st->axis_hover[1], st->axis_active_anim[1]);
    RenderAxisKnob(pt_knob_z, col_axis_z, st->axis_hover[2], st->axis_active_anim[2]);

    dl->AddCircleFilled(pt_tip + ImVec2(0.f, 1.f), 6.5f, AlphaCol(IM_COL32(0, 0, 0, 110), win_a), 16);
    dl->AddCircleFilled(pt_tip, 5.5f, AlphaCol(IM_COL32(255, 255, 255, 255), win_a), 16);
    dl->AddCircleFilled(pt_tip, 4.0f, AlphaCol(Draw->GetClr(Colors->accent), win_a), 16);

    dl->PopClipRect();

    const float row_y = canvas_bb.Max.y + gap;
    const float box_w = (avail - gap * 2.f) / 3.f;

    const ImU32 axis_clrs[3] = { col_axis_x, col_axis_y, col_axis_z };
    const char* axis_labels[3] = { xs("X"), xs("Y"), xs("Z") };

    float* vals[3] = { &v->x, &v->y, &v->z };
    float v_mins[3] = { min_x, min_y, min_z };
    float v_maxs[3] = { max_x, max_y, max_z };

    for (int i = 0; i < 3; i++) {
        const float bx = pos.x + i * (box_w + gap);
        const ImRect box_bb(ImVec2(bx, row_y), ImVec2(bx + box_w, row_y + row_h));
        if (VectorValueBox(st, i, box_bb, axis_labels[i], axis_clrs[i], vals[i], v_mins[i], v_maxs[i], font_small, in_window, win_a))
            changed = true;
    }

    if (changed && g.ActiveId == id)
        MarkItemEdited(id);

    return changed;
}