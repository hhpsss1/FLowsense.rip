#include "../Settings/Functions.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "../../Configurations/Configurations.h"
#include <xorstring/xorstring.h>

static const char* k_key_names[] = {
    "None",
    "Mouse 1",
    "Mouse 2",
    "Cancel",
    "Mouse 3",
    "Mouse 4",
    "Mouse 5",
    "-",
    "Back",
    "Tab",
    "-",
    "-",
    "Clear",
    "Enter",
    "-",
    "-",
    "Shift",
    "Ctrl",
    "Alt",
    "Pause",
    "Caps Lock",
    "Kana",
    "-",
    "Junja",
    "Final",
    "Kanji",
    "-",
    "Escape",
    "Convert",
    "Nonconvert",
    "Accept",
    "Mode",
    "Space",
    "Page Up",
    "Page Down",
    "End",
    "Home",
    "Left",
    "Up",
    "Right",
    "Down",
    "Select",
    "Print",
    "Execute",
    "Print Screen",
    "Insert",
    "Delete",
    "Help",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "Win",
    "Win",
    "App",
    "-",
    "Sleep",
    "Num 0",
    "Num 1",
    "Num 2",
    "Num 3",
    "Num 4",
    "Num 5",
    "Num 6",
    "Num 7",
    "Num 8",
    "Num 9",
    "Num *",
    "Num +",
    "Num Sep",
    "Num -",
    "Num Del",
    "Num /",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "Num Lock",
    "Scroll Lock",
    "Equ",
    "Mas",
    "Toy",
    "Oya",
    "Oya",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "Shift",
    "Shift",
    "Ctrl",
    "Ctrl",
    "Alt",
    "Alt"
};

namespace PopupUi {
    static std::unordered_map<ImGuiID, float> g_ani_width;

    static inline float AnimateWidth(ImGuiID id, float target, float speed = 22.0f) {
        float& current = g_ani_width[id];
        if (current <= 0.0f)
            current = target * 0.82f;

        const float delta = ImClamp(Gui->FixedSpeed(speed), 0.0f, 1.0f);
        current = ImLerp(current, target, delta);
        return current;
    }

    static inline float Ease(float t) {
        t = ImClamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    static inline float AnimateTo(const char* key, float target, float speed = 20.0f, float initial = 0.0f) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImGuiID id = window->GetID(key);
        float value = window->StateStorage.GetFloat(id, initial);
        const float delta = ImClamp(Gui->FixedSpeed(speed), 0.0f, 1.0f);
        value = ImLerp(value, target, delta);
        window->StateStorage.SetFloat(id, value);
        return value;
    }
}

namespace BindUi {
    constexpr float k_win_rounding = 12.0f;
    constexpr float k_list_pad = 8.0f;
    constexpr float k_edit_pad = 10.0f;
    constexpr float k_list_w = 148.0f;
    constexpr float k_edit_w = 200.0f;
    constexpr float k_row_h = 28.0f;
    constexpr float k_key_h = 40.0f;
    constexpr float k_seg_h = 30.0f;
    constexpr float k_item_h = 24.0f;
    constexpr float k_box_w = 100.0f;
    constexpr float k_box_h = 24.0f;
    constexpr float k_slider_w = 96.0f;
    constexpr float k_check_sz = 14.0f;
    constexpr float k_inner_pad = 8.0f;
    constexpr float k_icon_col = 16.0f;
    constexpr float k_icon_gap = 10.0f;
    constexpr float k_popup_gap = 8.0f;

    static const ImColor col_bg(17, 18, 18);
    static const ImColor col_border(36, 37, 38);
    static const ImColor col_row_active(32, 36, 45);
    static const ImColor col_row_hover(40, 42, 47);
    static const ImColor col_box_bg(27, 28, 30);
    static const ImColor col_box_hover(38, 39, 42);
    static const ImColor col_seg_bg(0, 0, 0);
    static const ImColor col_red(226, 104, 104);
    static const ImColor col_text(218, 219, 220);
    static const ImColor col_text_dim(150, 152, 156);

    struct Context {
        ImDrawList* draw_list = nullptr;
        ImFont* text_font = nullptr;
        ImFont* icon_font = nullptr;
        float alpha = 1.0f;
    };

    struct Segment {
        const char* label;
        int value;
    };

    static const ImGuiWindowFlags k_popup_flags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_Popup |
        ImGuiWindowFlags_AlwaysAutoResize;

    static inline ImVec2 CalcTextSize(ImFont* font, const char* str) {
        return font->CalcTextSizeA(font->LegacySize, FLT_MAX, -1.0f, str);
    }

    static inline void RenderText(const Context& ctx, ImFont* font, const ImVec2& pos, ImU32 col, const char* str) {
        ctx.draw_list->AddText(font, font->LegacySize, pos, col, str);
    }

    static inline float ContentX(const ImVec2& pos) {
        return pos.x + k_inner_pad + k_icon_col + k_icon_gap;
    }

    static inline ImU32 Mix(const ImColor& a, const ImColor& b, float t, float alpha) {
        return Draw->GetClr(ImColor(ImLerp(a.Value, b.Value, t)), alpha);
    }

    static inline void Icon(const Context& ctx, ImFont* font, const ImVec2& pos, const ImVec2& size, const char* str, ImU32 col) {
        const ImVec2 icon_sz = CalcTextSize(font, str);
        RenderText(ctx, font, ImVec2(pos.x + k_inner_pad + (k_icon_col - icon_sz.x) * 0.5f, pos.y + (size.y - icon_sz.y) * 0.5f), col, str);
    }

    static inline void Label(const Context& ctx, const ImVec2& pos, const ImVec2& size, const char* str, ImU32 col) {
        const ImVec2 label_sz = CalcTextSize(ctx.text_font, str);
        RenderText(ctx, ctx.text_font, ImVec2(ContentX(pos), pos.y + (size.y - label_sz.y) * 0.5f), col, str);
    }

    static inline void IconLabel(const Context& ctx, const ImVec2& pos, const ImVec2& size, ImFont* icon_font, const char* icon, const char* label) {
        Icon(ctx, icon_font, pos, size, icon, Draw->GetClr(col_text_dim, ctx.alpha));
        Label(ctx, pos, size, label, Draw->GetClr(col_text, ctx.alpha));
    }

    static inline void Window(const Context& ctx, const ImVec2& pos, const ImVec2& size, float rounding = k_win_rounding) {
        Draw->RectFilled(ctx.draw_list, pos, pos + size, Draw->GetClr(col_bg, ctx.alpha * 0.96f), rounding);
        ctx.draw_list->AddRect(pos + ImVec2(0.5f, 0.5f), pos + size - ImVec2(0.5f, 0.5f), Draw->GetClr(col_border, ctx.alpha), rounding, 0, 1.0f);
    }

    static inline void Separator(const Context& ctx, float width) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(width, 4.0f));
        ctx.draw_list->AddLine(ImVec2(pos.x, pos.y + 0.5f), ImVec2(pos.x + width, pos.y + 0.5f), Draw->GetClr(col_border, ctx.alpha), 1.0f);
    }

    static inline void Header(const Context& ctx, float width, const char* icon, const char* label) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);
        ImGui::Dummy(size);
        IconLabel(ctx, pos, size, ctx.icon_font, icon, label);
    }

    static inline bool ItemRow(const Context& ctx, const char* id, float width, ImFont* icon_font, const char* icon, const char* label) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);

        ImGui::InvisibleButton(id, size);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        const float anim = PopupUi::AnimateTo(id, hovered ? 1.0f : 0.0f, 18.0f);

        if (anim > 0.01f)
            Draw->RectFilled(ctx.draw_list, pos, pos + size, Draw->GetClr(col_row_hover, ctx.alpha * anim), 5.0f);

        IconLabel(ctx, pos, size, icon_font, icon, label);
        return clicked;
    }

    static inline bool BindRow(const Context& ctx, const char* id, float width, const char* label, bool bound, bool active) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);

        ImGui::InvisibleButton(id, size);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        const float anim_hover = PopupUi::AnimateTo("##hover", hovered ? 1.0f : 0.0f, 18.0f);
        const float anim_active = PopupUi::AnimateTo("##active", active ? 1.0f : 0.0f, 18.0f, active ? 1.0f : 0.0f);
        const float t = ImMax(anim_active, anim_hover);

        Draw->RectFilled(ctx.draw_list, pos, pos + size, Draw->GetClr(col_row_active, ctx.alpha * (0.5f + 0.5f * t)), 8.0f);
        IconLabel(ctx, pos, size, ctx.icon_font, bound ? ICON_FA_LOCK : ICON_FA_LOCK_OPEN, label);

        const ImVec2 p_sz = CalcTextSize(ctx.icon_font, ICON_FA_PENCIL);
        RenderText(ctx, ctx.icon_font, ImVec2(pos.x + size.x - k_inner_pad - p_sz.x, pos.y + (size.y - p_sz.y) * 0.5f), Mix(col_text_dim, col_text, t, ctx.alpha), ICON_FA_PENCIL);

        return clicked;
    }

    static inline bool KeyRow(const Context& ctx, const char* id, float width, const char* label, bool has_value, bool capturing) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_key_h);

        ImGui::InvisibleButton(id, size);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        const float anim = PopupUi::AnimateTo(id, hovered ? 1.0f : 0.0f, 18.0f);

        Draw->RectFilled(ctx.draw_list, pos, pos + size, Mix(col_box_bg, col_box_hover, anim * 0.5f, ctx.alpha), 8.0f);

        if (capturing) {
            const float pulse = (sinf(static_cast<float>(ImGui::GetTime()) * 4.0f) * 0.5f + 0.5f) * 0.4f + 0.6f;
            Draw->RectFilled(ctx.draw_list, pos, pos + size, Draw->GetClr(Colors->accent, ctx.alpha * 0.10f), 8.0f);
            ctx.draw_list->AddRect(pos + ImVec2(0.5f, 0.5f), pos + size - ImVec2(0.5f, 0.5f), Draw->GetClr(Colors->accent, ctx.alpha * pulse), 8.0f, 0, 1.0f);
        }

        Icon(ctx, ctx.icon_font, pos, size, ICON_FA_KEY, Draw->GetClr(col_text_dim, ctx.alpha));
        Label(ctx, pos, size, label, Draw->GetClr(capturing ? Colors->accent : (has_value ? col_text : col_text_dim), ctx.alpha));

        const ImVec2 box_sz(26.0f, 26.0f);
        const ImVec2 box_pos(pos.x + size.x - 7.0f - box_sz.x, pos.y + (size.y - box_sz.y) * 0.5f);
        Draw->RectFilled(ctx.draw_list, box_pos, box_pos + box_sz, Mix(col_box_hover, col_row_hover, anim, ctx.alpha), 6.0f);

        const ImVec2 icon_sz = CalcTextSize(ctx.icon_font, ICON_FA_PENCIL);
        RenderText(ctx, ctx.icon_font, ImVec2(box_pos.x + (box_sz.x - icon_sz.x) * 0.5f, box_pos.y + (box_sz.y - icon_sz.y) * 0.5f), Draw->GetClr(col_text, ctx.alpha), ICON_FA_PENCIL);

        return clicked;
    }

    static inline void Segments(const Context& ctx, const char* id, float width, int* value, const Segment* items, int count) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_seg_h);

        ImGui::InvisibleButton(id, size);
        const bool clicked = ImGui::IsItemClicked();
        const float seg_w = size.x / static_cast<float>(count);

        int selected = 0;
        for (int i = 0; i < count; ++i) {
            if (items[i].value == *value)
                selected = i;
        }

        if (clicked) {
            selected = ImClamp(static_cast<int>((ImGui::GetIO().MousePos.x - pos.x) / seg_w), 0, count - 1);
            *value = items[selected].value;
        }

        const float anim_x = PopupUi::AnimateTo(id, static_cast<float>(selected), 20.0f, static_cast<float>(selected));

        Draw->RectFilled(ctx.draw_list, pos, pos + size, Draw->GetClr(col_seg_bg, ctx.alpha * 0.6f), 8.0f);

        const ImVec2 hover_pos(pos.x + anim_x * seg_w + 2.0f, pos.y + 2.0f);
        const ImVec2 hover_sz(seg_w - 4.0f, size.y - 4.0f);
        Draw->RectFilled(ctx.draw_list, hover_pos, hover_pos + hover_sz, Draw->GetClr(col_row_active, ctx.alpha), 6.0f);

        for (int i = 0; i < count; ++i) {
            const float t = 1.0f - ImClamp(fabsf(anim_x - static_cast<float>(i)), 0.0f, 1.0f);
            const ImVec2 label_sz = CalcTextSize(ctx.text_font, items[i].label);
            const ImVec2 label_pos(pos.x + seg_w * static_cast<float>(i) + (seg_w - label_sz.x) * 0.5f, pos.y + (size.y - label_sz.y) * 0.5f);
            RenderText(ctx, ctx.text_font, label_pos, Mix(col_text_dim, col_text, t, ctx.alpha), items[i].label);
        }
    }

    static inline void CheckboxDraw(const Context& ctx, const char* id, const ImVec2& pos, bool value, bool hovered) {
        const float sz = k_check_sz;
        const float r = 3.5f;

        ImGui::PushID(id);
        const float fill = PopupUi::AnimateTo("##fill", value ? 1.0f : 0.0f, 14.0f, value ? 1.0f : 0.0f);
        const float hover = PopupUi::AnimateTo("##hover", hovered ? 1.0f : 0.0f, 12.0f);
        ImGui::PopID();

        const ImVec4 bg_off = Colors->button.background.Value;
        const ImVec4 bg_on = Colors->accent.Value;
        ImVec4 bg(ImLerp(bg_off.x, bg_on.x, fill), ImLerp(bg_off.y, bg_on.y, fill), ImLerp(bg_off.z, bg_on.z, fill), ImLerp(bg_off.w, 1.0f, fill));

        if (hover > 0.01f && fill < 0.99f) {
            bg.x = ImMin(bg.x + 0.04f * hover, 1.0f);
            bg.y = ImMin(bg.y + 0.04f * hover, 1.0f);
            bg.z = ImMin(bg.z + 0.04f * hover, 1.0f);
        }

        const ImVec2 bmin = pos;
        const ImVec2 bmax = pos + ImVec2(sz, sz);

        ctx.draw_list->AddRectFilled(bmin, bmax, Draw->GetClr(ImColor(bg), ctx.alpha), r);

        if (fill < 0.99f)
            ctx.draw_list->AddRect(bmin, bmax, Draw->GetClr(Colors->divider, ctx.alpha * ImLerp(0.04f, 0.12f, hover)), r, 0, 1.0f);

        if (fill > 0.01f) {
            const ImVec2 pts[3] = {
                ImVec2(bmin.x + sz * 0.28f, bmin.y + sz * 0.50f),
                ImVec2(bmin.x + sz * 0.44f, bmin.y + sz * 0.68f),
                ImVec2(bmin.x + sz * 0.72f, bmin.y + sz * 0.32f)
            };
            ctx.draw_list->AddPolyline(pts, 3, Draw->GetClr(ImColor(1.0f, 1.0f, 1.0f, fill), ctx.alpha), 0, 1.8f);
        }
    }

    static inline bool CheckboxRow(const Context& ctx, const char* id, float width, const char* icon, const char* label, bool* value) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);

        ImGui::InvisibleButton(id, size);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        if (clicked)
            *value = !*value;

        IconLabel(ctx, pos, size, ctx.icon_font, icon, label);
        CheckboxDraw(ctx, id, ImVec2(pos.x + size.x - k_inner_pad - k_check_sz + 1.0f, pos.y + (size.y - k_check_sz) * 0.5f), *value, hovered);

        return clicked;
    }

    static inline bool SliderRow(const Context& ctx, const char* id, float width, const char* icon, const char* label, ImGuiDataType data_type, void* value, const void* v_min, const void* v_max, const char* fmt) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);

        IconLabel(ctx, pos, size, ctx.icon_font, icon, label);
        const bool changed = Gui->SliderTrack(id, ImVec2(pos.x + width - k_inner_pad - k_slider_w, pos.y), ImVec2(pos.x + width - k_inner_pad, pos.y + k_row_h), data_type, value, v_min, v_max, fmt, ctx.alpha);

        ImGui::SetCursorScreenPos(pos);
        ImGui::Dummy(size);
        return changed;
    }

    static inline bool BeginList(const char* id, const ImVec2& pos, float width) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(width, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, 220.0f));

        const bool open = ImGui::BeginPopup(id, k_popup_flags | ImGuiWindowFlags_NoScrollbar);
        if (!open) {
            ImGui::PopStyleVar(5);
            ImGui::PopStyleColor();
        }

        return open;
    }

    static inline void EndList() {
        ImGui::EndPopup();
        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor();
    }

    static inline Context ListCtx(const Context& ctx) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (ImGui::IsWindowAppearing())
            window->StateStorage.SetFloat(window->GetID("##alpha"), 0.0f);

        const float a = PopupUi::AnimateTo("##alpha", 1.0f, 16.0f);
        Context list_ctx{ window->DrawList, ctx.text_font, ctx.icon_font, ctx.alpha * a };
        Window(list_ctx, window->Pos, window->Size, 8.0f);
        return list_ctx;
    }

    static inline bool ListItem(const Context& ctx, const char* id, float width, const char* label, bool selected, bool check) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_item_h);

        ImGui::InvisibleButton(id, size);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        const float anim = PopupUi::AnimateTo(id, hovered ? 1.0f : 0.0f, 18.0f);

        if (anim > 0.01f)
            Draw->RectFilled(ctx.draw_list, pos, pos + size, Draw->GetClr(col_row_hover, ctx.alpha * anim), 5.0f);

        float right = pos.x + size.x - k_inner_pad;
        if (check) {
            const ImVec2 check_sz = CalcTextSize(ctx.icon_font, ICON_FA_CHECK);
            if (selected)
                RenderText(ctx, ctx.icon_font, ImVec2(right - check_sz.x, pos.y + (size.y - check_sz.y) * 0.5f), Draw->GetClr(Colors->accent, ctx.alpha), ICON_FA_CHECK);

            right -= check_sz.x + 6.0f;
        }

        Draw->TextClipped(ctx.draw_list, ctx.text_font, ImVec2(pos.x + k_inner_pad, pos.y), ImVec2(right, pos.y + size.y), Mix(col_text_dim, col_text, ImMax(selected ? 1.0f : 0.0f, anim), ctx.alpha), label, nullptr, nullptr, ImVec2(0.0f, 0.5f));
        return clicked;
    }

    static inline bool ValueBox(const Context& ctx, const char* id, const ImVec2& box_pos, const char* text, bool open) {
        const ImVec2 box_sz(k_box_w, k_box_h);

        ImGui::SetCursorScreenPos(box_pos);
        ImGui::InvisibleButton(id, box_sz);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        const float anim = PopupUi::AnimateTo(id, (hovered || open) ? 1.0f : 0.0f, 18.0f);

        Draw->RectFilled(ctx.draw_list, box_pos, box_pos + box_sz, Mix(col_box_bg, col_box_hover, anim, ctx.alpha), 6.0f);

        const ImVec2 chev_sz = CalcTextSize(ctx.icon_font, ICON_FA_CHEVRON_DOWN);
        RenderText(ctx, ctx.icon_font, ImVec2(box_pos.x + box_sz.x - k_inner_pad - chev_sz.x, box_pos.y + (box_sz.y - chev_sz.y) * 0.5f), Draw->GetClr(col_text_dim, ctx.alpha), ICON_FA_CHEVRON_DOWN);
        Draw->TextClipped(ctx.draw_list, ctx.text_font, ImVec2(box_pos.x + k_inner_pad, box_pos.y), ImVec2(box_pos.x + box_sz.x - k_inner_pad - chev_sz.x - 6.0f, box_pos.y + box_sz.y), Draw->GetClr(col_text, ctx.alpha), text, nullptr, nullptr, ImVec2(0.0f, 0.5f));

        return clicked;
    }

    static inline bool Dropdown(const Context& ctx, const char* id, float width, const char* icon, const char* label, int* idx, const std::vector<std::string>& items) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);

        char popup_id[64];
        ImFormatString(popup_id, sizeof(popup_id), "%s_list", id);
        const bool is_open = ImGui::IsPopupOpen(popup_id);

        IconLabel(ctx, pos, size, ctx.icon_font, icon, label);

        const ImVec2 box_pos(pos.x + width - k_box_w, pos.y + (size.y - k_box_h) * 0.5f);
        const char* current = (*idx >= 0 && *idx < static_cast<int>(items.size())) ? items[*idx].c_str() : "None";

        if (ValueBox(ctx, id, box_pos, current, is_open))
            ImGui::OpenPopup(popup_id);

        ImGui::SetCursorScreenPos(pos);
        ImGui::Dummy(size);

        bool changed = false;
        if (BeginList(popup_id, ImVec2(box_pos.x, box_pos.y + k_box_h + 4.0f), k_box_w)) {
            const Context list_context = ListCtx(ctx);

            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                ImGui::PushID(i);
                if (ListItem(list_context, "##item", k_box_w - 8.0f, items[i].c_str(), i == *idx, false)) {
                    *idx = i;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
            }

            EndList();
        }

        return changed;
    }

    static inline bool MultiDropdown(const Context& ctx, const char* id, float width, const char* icon, const char* label, std::vector<bool>& selected, const std::vector<std::string>& items) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);

        char popup_id[64];
        ImFormatString(popup_id, sizeof(popup_id), "%s_list", id);
        const bool is_open = ImGui::IsPopupOpen(popup_id);

        IconLabel(ctx, pos, size, ctx.icon_font, icon, label);

        int count = 0;
        int last = -1;
        for (int i = 0; i < static_cast<int>(selected.size()) && i < static_cast<int>(items.size()); ++i) {
            if (!selected[i])
                continue;

            ++count;
            last = i;
        }

        char buffer[64];
        if (count == 0)
            ImFormatString(buffer, sizeof(buffer), "None");
        else if (count == 1)
            ImFormatString(buffer, sizeof(buffer), "%s", items[last].c_str());
        else
            ImFormatString(buffer, sizeof(buffer), "%d selected", count);

        const ImVec2 box_pos(pos.x + width - k_box_w, pos.y + (size.y - k_box_h) * 0.5f);
        if (ValueBox(ctx, id, box_pos, buffer, is_open))
            ImGui::OpenPopup(popup_id);

        ImGui::SetCursorScreenPos(pos);
        ImGui::Dummy(size);

        bool changed = false;
        if (BeginList(popup_id, ImVec2(box_pos.x, box_pos.y + k_box_h + 4.0f), k_box_w)) {
            const Context list_context = ListCtx(ctx);

            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                const bool enabled = i < static_cast<int>(selected.size()) && selected[i];

                ImGui::PushID(i);
                if (ListItem(list_context, "##item", k_box_w - 8.0f, items[i].c_str(), enabled, true)) {
                    if (i < static_cast<int>(selected.size()))
                        selected[i] = !enabled;

                    changed = true;
                }
                ImGui::PopID();
            }

            EndList();
        }

        return changed;
    }

    static inline bool DangerButton(const Context& ctx, const char* id, float width, float rounding, const char* icon, const char* label) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(width, k_row_h);

        ImGui::InvisibleButton(id, size);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        const float anim = PopupUi::AnimateTo(id, hovered ? 1.0f : 0.0f, 18.0f);

        Draw->RectFilled(ctx.draw_list, pos, pos + size, Draw->GetClr(col_red, ctx.alpha * (0.08f + 0.07f * anim)), rounding);

        const ImVec2 icon_sz = CalcTextSize(ctx.icon_font, icon);
        const ImVec2 label_sz = CalcTextSize(ctx.text_font, label);

        const float total_w = icon_sz.x + 8.0f + label_sz.x;
        const float start_x = pos.x + (size.x - total_w) * 0.5f;
        const ImU32 text_col = Draw->GetClr(col_red, ctx.alpha);

        RenderText(ctx, ctx.icon_font, ImVec2(start_x, pos.y + (size.y - icon_sz.y) * 0.5f), text_col, icon);
        RenderText(ctx, ctx.text_font, ImVec2(start_x + icon_sz.x + 8.0f, pos.y + (size.y - label_sz.y) * 0.5f), text_col, label);

        return clicked;
    }
}

namespace Binds {
    struct CheckboxBind {
        bool* value = nullptr;
        int* int_value = nullptr;
        std::string label;
        int key = 0;
        int type = static_cast<int>(EBindType::toggle);
        bool active_on = true;
        bool show_in_list = true;
        bool last_down = false;
        bool hold_active = false;
        bool backup_value = false;
        int backup_int_value = 0;
        uint32_t uid = 0;
        bool enabled = true;
        std::string group;
    };

    struct SliderIntBind {
        int* value = nullptr;
        std::string label;
        int key = 0;
        int type = static_cast<int>(EBindType::toggle);
        int target = 0;
        int min_value = 0;
        int max_value = 0;
        bool show_in_list = true;
        bool last_down = false;
        bool active = false;
        int backup = 0;
        uint32_t uid = 0;
        bool enabled = true;
        std::string group;
        std::string format;
    };

    struct SliderFloatBind {
        float* value = nullptr;
        std::string label;
        int key = 0;
        int type = static_cast<int>(EBindType::toggle);
        float target = 0.0f;
        float min_value = 0.0f;
        float max_value = 0.0f;
        bool show_in_list = true;
        bool last_down = false;
        bool active = false;
        float backup = 0.0f;
        uint32_t uid = 0;
        bool enabled = true;
        std::string group;
        std::string format;
        float scale = 1.0f;
    };

    struct DropdownBind {
        int* value = nullptr;
        std::string label;
        int key = 0;
        int type = static_cast<int>(EBindType::toggle);
        int target_index = 0;
        bool show_in_list = true;
        bool last_down = false;
        bool active = false;
        int backup = 0;
        uint32_t uid = 0;
        bool enabled = true;
        std::string group;
        std::vector<std::string> items;
    };

    struct MultiDropdownBoolBind {
        std::vector<bool>* value = nullptr;
        std::string label;
        int key = 0;
        int type = static_cast<int>(EBindType::toggle);
        std::vector<int> target_indices;
        bool show_in_list = true;
        bool last_down = false;
        bool active = false;
        std::vector<bool> backup;
        uint32_t uid = 0;
        bool enabled = true;
        std::string group;
        std::vector<std::string> items;
    };

    struct MultiDropdownIntBind {
        std::vector<int>* value = nullptr;
        std::string label;
        int key = 0;
        int type = static_cast<int>(EBindType::toggle);
        std::vector<int> target_indices;
        bool show_in_list = true;
        bool last_down = false;
        bool active = false;
        std::vector<int> backup;
        uint32_t uid = 0;
        bool enabled = true;
        std::string group;
        std::vector<std::string> items;
    };

    static std::vector<CheckboxBind> g_checkbox_binds;
    static std::vector<SliderIntBind> g_slider_int_binds;
    static std::vector<SliderFloatBind> g_slider_float_binds;
    static std::vector<DropdownBind> g_dropdown_binds;
    static std::vector<MultiDropdownBoolBind> g_multi_dropdown_bool_binds;
    static std::vector<MultiDropdownIntBind> g_multi_dropdown_int_binds;

    static uint32_t g_next_bind_uid = 1;
    static std::unordered_map<void*, uint32_t> g_active_owner;

    static std::unordered_map<void*, bool> g_backup_checkbox;
    static std::unordered_map<void*, int> g_backup_slider_int;
    static std::unordered_map<void*, float> g_backup_slider_float;
    static std::unordered_map<void*, int> g_backup_dropdown;
    static std::unordered_map<void*, std::vector<bool>> g_backup_multi_bool;
    static std::unordered_map<void*, std::vector<int>> g_backup_multi_int;

    template <typename T>
    static void BindApplyActivate(void* key, uint32_t uid, T& value_ref, const T& target, std::unordered_map<void*, T>& backup_map) {
        if (g_active_owner.find(key) == g_active_owner.end())
            backup_map[key] = value_ref;

        g_active_owner[key] = uid;
        value_ref = target;
    }

    template <typename T>
    static void BindApplyDeactivate(void* key, uint32_t uid, T& value_ref, std::unordered_map<void*, T>& backup_map) {
        const auto owner_it = g_active_owner.find(key);
        if (owner_it == g_active_owner.end() || owner_it->second != uid)
            return;

        const auto backup_it = backup_map.find(key);
        if (backup_it != backup_map.end()) {
            value_ref = backup_it->second;
            backup_map.erase(backup_it);
        }

        g_active_owner.erase(owner_it);
    }

    template <typename BindT, typename ValueT>
    static void BindDeactivateSiblings(std::vector<BindT>& binds, BindT* self, ValueT* value_key) {
        for (auto& other : binds) {
            if (&other == self)
                continue;

            bool same_key = false;
            if constexpr (std::is_same_v<BindT, CheckboxBind>)
                same_key = (other.value == reinterpret_cast<bool*>(value_key) || other.int_value == reinterpret_cast<int*>(value_key));
            else
                same_key = (other.value == value_key);

            if (!same_key)
                continue;

            if constexpr (std::is_same_v<BindT, CheckboxBind>)
                other.hold_active = false;
            else
                other.active = false;
        }
    }

    static void ForceDeactivate(CheckboxBind& b) {
        if (!b.hold_active)
            return;

        b.hold_active = false;
        if (b.value)
            BindApplyDeactivate<bool>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_checkbox);
        else if (b.int_value)
            BindApplyDeactivate<int>(static_cast<void*>(b.int_value), b.uid, *b.int_value, g_backup_slider_int);
    }

    static void ForceDeactivate(SliderIntBind& b) {
        if (!b.active)
            return;

        b.active = false;
        BindApplyDeactivate<int>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_slider_int);
    }

    static void ForceDeactivate(SliderFloatBind& b) {
        if (!b.active)
            return;

        b.active = false;
        BindApplyDeactivate<float>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_slider_float);
    }

    static void ForceDeactivate(DropdownBind& b) {
        if (!b.active)
            return;

        b.active = false;
        BindApplyDeactivate<int>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_dropdown);
    }

    static void ForceDeactivate(MultiDropdownBoolBind& b) {
        if (!b.active)
            return;

        b.active = false;
        BindApplyDeactivate<std::vector<bool>>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_multi_bool);
    }

    static void ForceDeactivate(MultiDropdownIntBind& b) {
        if (!b.active)
            return;

        b.active = false;
        BindApplyDeactivate<std::vector<int>>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_multi_int);
    }

    static bool CheckboxActive(const CheckboxBind& b) {
        if (b.type == static_cast<int>(EBindType::hold))
            return b.hold_active == b.active_on;

        if (b.value)
            return *b.value;

        return b.int_value && *b.int_value != 0;
    }

    template <typename F>
    static bool ForBind(uint32_t uid, F&& fn) {
        auto scan = [&](auto& vec) -> bool {
            for (size_t i = 0; i < vec.size(); ++i) {
                if (vec[i].uid != uid)
                    continue;

                fn(vec, i);
                return true;
            }
            return false;
            };

        return scan(g_checkbox_binds) ||
            scan(g_slider_int_binds) ||
            scan(g_slider_float_binds) ||
            scan(g_dropdown_binds) ||
            scan(g_multi_dropdown_bool_binds) ||
            scan(g_multi_dropdown_int_binds);
    }

    static std::string CurrentGroup() {
        for (const auto& cat : Var->gui.categories) {
            for (const auto& tab : cat.tabs) {
                if (tab.id != Var->gui.active_tab_id)
                    continue;

                std::string res = cat.label;
                if (tab.label != cat.label)
                    res += " / " + tab.label;

                const int sub = tab.active_subtab;
                if (sub >= 0 && sub < static_cast<int>(tab.subtabs.size()))
                    res += " / " + tab.subtabs[sub].label;

                return res;
            }
        }
        return "Other";
    }

    static bool IsVkDown(int vk) {
        if (vk <= 0)
            return false;
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    static int* s_capturing_key = nullptr;
    static bool s_waiting_release = false;
    static bool s_block_activation = false;

    bool IsCapturingBind() {
        return s_capturing_key != nullptr || s_block_activation;
    }

    static void PollKeyCapture() {
        if (s_block_activation) {
            bool any_down = false;
            for (int vk = 0x01; vk <= 0x06; ++vk) {
                if (GetAsyncKeyState(vk) & 0x8000) {
                    any_down = true;
                    break;
                }
            }

            for (int vk = 0x08; vk <= 0xA5; ++vk) {
                if (GetAsyncKeyState(vk) & 0x8000) {
                    any_down = true;
                    break;
                }
            }

            if (!any_down) {
                s_block_activation = false;
                s_waiting_release = false;
            }
            return;
        }

        if (!s_capturing_key)
            return;

        if (s_waiting_release) {
            bool any_down = false;
            for (int vk = 0x01; vk <= 0x06; ++vk) {
                if (GetAsyncKeyState(vk) & 0x8000) {
                    any_down = true;
                    break;
                }
            }

            for (int vk = 0x08; vk <= 0xA5; ++vk) {
                if (GetAsyncKeyState(vk) & 0x8000) {
                    any_down = true;
                    break;
                }
            }

            if (!any_down)
                s_waiting_release = false;

            return;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            *s_capturing_key = 0;
            s_capturing_key = nullptr;
            s_block_activation = true;
            return;
        }

        const int mouse_vks[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
        for (const int vk : mouse_vks) {
            if (!(GetAsyncKeyState(vk) & 0x8000))
                continue;

            *s_capturing_key = vk;
            s_capturing_key = nullptr;
            s_block_activation = true;
            return;
        }

        for (int vk = 0x08; vk <= 0xA5; ++vk) {
            if (!(GetAsyncKeyState(vk) & 0x8000))
                continue;

            *s_capturing_key = vk;
            s_capturing_key = nullptr;
            s_block_activation = true;
            return;
        }
    }

    void Update() {
        if (s_block_activation && !s_capturing_key)
            PollKeyCapture();

        if (IsCapturingBind())
            return;

        static auto is_window_active = []() -> bool {
            const HWND fg = GetForegroundWindow();
            if (!fg)
                return false;

            DWORD pid = 0;
            GetWindowThreadProcessId(fg, &pid);
            return pid == GetCurrentProcessId() && !IsIconic(fg);
            };

        for (auto& b : g_checkbox_binds) {
            if ((!b.value && !b.int_value) || b.key == 0 || !b.enabled)
                continue;

            const bool down = IsVkDown(b.key);
            const bool pressed = down && !b.last_down;
            const bool released = !down && b.last_down;

            if (b.type == static_cast<int>(EBindType::hold)) {
                if (is_window_active() && pressed && !b.hold_active) {
                    if (b.value) {
                        BindDeactivateSiblings(g_checkbox_binds, &b, b.value);
                        b.hold_active = true;
                        BindApplyActivate<bool>(static_cast<void*>(b.value), b.uid, *b.value, b.active_on, g_backup_checkbox);
                    }
                    else if (b.int_value) {
                        BindDeactivateSiblings(g_checkbox_binds, &b, reinterpret_cast<bool*>(b.int_value));
                        b.hold_active = true;
                        BindApplyActivate<int>(static_cast<void*>(b.int_value), b.uid, *b.int_value, b.active_on ? 1 : 0, g_backup_slider_int);
                    }
                }

                if (released && b.hold_active) {
                    b.hold_active = false;
                    if (b.value)
                        BindApplyDeactivate<bool>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_checkbox);
                    else if (b.int_value)
                        BindApplyDeactivate<int>(static_cast<void*>(b.int_value), b.uid, *b.int_value, g_backup_slider_int);
                }
            }
            else {
                if (is_window_active() && pressed) {
                    if (b.value)
                        *b.value = (*b.value == b.active_on) ? !b.active_on : b.active_on;
                    else if (b.int_value)
                        *b.int_value = (*b.int_value == (b.active_on ? 1 : 0)) ? (b.active_on ? 0 : 1) : (b.active_on ? 1 : 0);
                }
            }

            b.last_down = down;
        }

        for (auto& b : g_slider_int_binds) {
            if (!b.value || b.key == 0 || !b.enabled)
                continue;

            const bool down = IsVkDown(b.key);
            const bool pressed = down && !b.last_down;
            const bool released = !down && b.last_down;

            if (b.type == static_cast<int>(EBindType::hold)) {
                if (is_window_active() && pressed && !b.active) {
                    BindDeactivateSiblings(g_slider_int_binds, &b, b.value);
                    b.active = true;
                    BindApplyActivate<int>(static_cast<void*>(b.value), b.uid, *b.value, b.target, g_backup_slider_int);
                }

                if (released && b.active) {
                    b.active = false;
                    BindApplyDeactivate<int>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_slider_int);
                }
            }
            else {
                if (is_window_active() && pressed) {
                    void* key = static_cast<void*>(b.value);
                    const auto owner = g_active_owner.find(key);
                    const bool owns = owner != g_active_owner.end() && owner->second == b.uid;

                    if (owns) {
                        b.active = false;
                        BindApplyDeactivate<int>(key, b.uid, *b.value, g_backup_slider_int);

                        const int index = Config::GetIndexFromPtr(b.value);
                        if (index >= 0 && Config::variables[index].name == xs("YawOffset")) {
                            *b.value = 0;
                        }
                    }
                    else {
                        BindDeactivateSiblings(g_slider_int_binds, &b, b.value);
                        b.active = true;
                        BindApplyActivate<int>(key, b.uid, *b.value, b.target, g_backup_slider_int);
                    }
                }
            }
            b.last_down = down;
        }

        for (auto& b : g_slider_float_binds) {
            if (!b.value || b.key == 0 || !b.enabled)
                continue;

            const bool down = IsVkDown(b.key);
            const bool pressed = down && !b.last_down;
            const bool released = !down && b.last_down;

            if (b.type == static_cast<int>(EBindType::hold)) {
                if (is_window_active() && pressed && !b.active) {
                    BindDeactivateSiblings(g_slider_float_binds, &b, b.value);
                    b.active = true;
                    BindApplyActivate<float>(static_cast<void*>(b.value), b.uid, *b.value, b.target, g_backup_slider_float);
                }

                if (released && b.active) {
                    b.active = false;
                    BindApplyDeactivate<float>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_slider_float);
                }
            }
            else {
                if (is_window_active() && pressed) {
                    if (!b.active) {
                        BindDeactivateSiblings(g_slider_float_binds, &b, b.value);
                        b.active = true;
                        BindApplyActivate<float>(static_cast<void*>(b.value), b.uid, *b.value, b.target, g_backup_slider_float);
                    }
                    else {
                        b.active = false;
                        BindApplyDeactivate<float>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_slider_float);
                    }
                }
            }
            b.last_down = down;
        }

        for (auto& b : g_dropdown_binds) {
            if (!b.value || b.key == 0 || !b.enabled)
                continue;

            const bool down = IsVkDown(b.key);
            const bool pressed = down && !b.last_down;
            const bool released = !down && b.last_down;

            if (b.type == static_cast<int>(EBindType::hold)) {
                if (is_window_active() && pressed && !b.active) {
                    BindDeactivateSiblings(g_dropdown_binds, &b, b.value);
                    b.active = true;
                    BindApplyActivate<int>(static_cast<void*>(b.value), b.uid, *b.value, b.target_index, g_backup_dropdown);
                }

                if (released && b.active) {
                    b.active = false;
                    BindApplyDeactivate<int>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_dropdown);
                }
            }
            else {
                if (is_window_active() && pressed) {
                    if (!b.active) {
                        BindDeactivateSiblings(g_dropdown_binds, &b, b.value);
                        b.active = true;
                        BindApplyActivate<int>(static_cast<void*>(b.value), b.uid, *b.value, b.target_index, g_backup_dropdown);
                    }
                    else {
                        b.active = false;
                        BindApplyDeactivate<int>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_dropdown);
                    }
                }
            }
            b.last_down = down;
        }

        for (auto& b : g_multi_dropdown_bool_binds) {
            if (!b.value || b.key == 0 || !b.enabled)
                continue;

            const bool down = IsVkDown(b.key);
            const bool pressed = down && !b.last_down;
            const bool released = !down && b.last_down;

            auto compute_target = [&]() {
                std::vector<bool> current = *b.value;
                const int index = Config::GetIndexFromPtr(b.value);
                if (index >= 0 && Config::variables[index].name.ends_with("_AutoStop"))
                    std::fill(current.begin(), current.end(), false);
                for (const int idx : b.target_indices) {
                    if (idx >= 0 && idx < static_cast<int>(current.size()))
                        current[idx] = true;
                }
                return current;
                };

            if (b.type == static_cast<int>(EBindType::hold)) {
                if (is_window_active() && pressed && !b.active) {
                    BindDeactivateSiblings(g_multi_dropdown_bool_binds, &b, b.value);
                    b.active = true;
                    BindApplyActivate<std::vector<bool>>(static_cast<void*>(b.value), b.uid, *b.value, compute_target(), g_backup_multi_bool);
                }

                if (released && b.active) {
                    b.active = false;
                    BindApplyDeactivate<std::vector<bool>>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_multi_bool);
                }
            }
            else {
                if (is_window_active() && pressed) {
                    if (!b.active) {
                        BindDeactivateSiblings(g_multi_dropdown_bool_binds, &b, b.value);
                        b.active = true;
                        BindApplyActivate<std::vector<bool>>(static_cast<void*>(b.value), b.uid, *b.value, compute_target(), g_backup_multi_bool);
                    }
                    else {
                        b.active = false;
                        BindApplyDeactivate<std::vector<bool>>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_multi_bool);
                    }
                }
            }
            b.last_down = down;
        }

        for (auto& b : g_multi_dropdown_int_binds) {
            if (!b.value || b.key == 0 || !b.enabled)
                continue;

            const bool down = IsVkDown(b.key);
            const bool pressed = down && !b.last_down;
            const bool released = !down && b.last_down;

            if (b.type == static_cast<int>(EBindType::hold)) {
                if (is_window_active() && pressed && !b.active) {
                    BindDeactivateSiblings(g_multi_dropdown_int_binds, &b, b.value);
                    b.active = true;
                    BindApplyActivate<std::vector<int>>(static_cast<void*>(b.value), b.uid, *b.value, b.target_indices, g_backup_multi_int);
                }

                if (released && b.active) {
                    b.active = false;
                    BindApplyDeactivate<std::vector<int>>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_multi_int);
                }
            }
            else {
                if (is_window_active() && pressed) {
                    if (!b.active) {
                        BindDeactivateSiblings(g_multi_dropdown_int_binds, &b, b.value);
                        b.active = true;
                        BindApplyActivate<std::vector<int>>(static_cast<void*>(b.value), b.uid, *b.value, b.target_indices, g_backup_multi_int);
                    }
                    else {
                        b.active = false;
                        BindApplyDeactivate<std::vector<int>>(static_cast<void*>(b.value), b.uid, *b.value, g_backup_multi_int);
                    }
                }
            }
            b.last_down = down;
        }
    }


    static std::string HexEncode(const std::string& in) {
        static constexpr char hex_chars[] = "0123456789ABCDEF";
        std::string out;
        out.reserve(in.size() * 2);
        for (unsigned char c : in) {
            out.push_back(hex_chars[c >> 4]);
            out.push_back(hex_chars[c & 0x0F]);
        }
        return out;
    }

    static std::string HexDecode(const std::string& in) {
        if (in.size() % 2 != 0)
            return {};

        auto hex_val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
            };
        std::string out;
        out.reserve(in.size() / 2);
        for (size_t i = 0; i + 1 < in.size(); i += 2) {
            const int high = hex_val(in[i]);
            const int low = hex_val(in[i + 1]);
            if (high < 0 || low < 0)
                return {};
            out.push_back(static_cast<char>((high << 4) | low));
        }
        return out;
    }

    static std::string EscapeBindStr(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '\\') out += "\\\\";
            else if (c == ';') out += "\\;";
            else if (c == '\n') out += "\\n";
            else if (c == ',') out += "\\,";
            else out += c;
        }
        return out;
    }

    static std::string UnescapeBindStr(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                char next = s[i + 1];
                if (next == ';') { out += ';'; ++i; }
                else if (next == ',') { out += ','; ++i; }
                else if (next == 'n') { out += '\n'; ++i; }
                else if (next == '\\') { out += '\\'; ++i; }
                else out += s[i];
            }
            else {
                out += s[i];
            }
        }
        return out;
    }

    static std::vector<std::string> SplitEscaped(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::string cur;
        bool escape = false;
        for (size_t i = 0; i < str.size(); ++i) {
            char c = str[i];
            if (escape) {
                cur += '\\';
                cur += c;
                escape = false;
            }
            else if (c == '\\') {
                escape = true;
            }
            else if (c == delim) {
                tokens.push_back(cur);
                cur.clear();
            }
            else {
                cur += c;
            }
        }
        tokens.push_back(cur);
        return tokens;
    }

    static std::string BindTargetReference(const void* ptr, const std::string& type_name) {
        const int index = Config::GetIndexFromPtr(ptr);
        if (index < 0)
            return {};

        const auto& var = Config::variables[index];
        if (Config::GetPtrFromName(var.name, type_name) != ptr)
            return {};

        return "name:" + EscapeBindStr(var.name);
    }

    static bool ParseBindInt(const std::string& text, int& value) {
        try {
            std::size_t consumed = 0;
            value = std::stoi(text, &consumed);
            return consumed == text.size();
        } catch (...) {
            return false;
        }
    }

    static void* ResolveBindTarget(const std::string& reference, const std::string& type_name) {
        if (reference.rfind("name:", 0) == 0)
            return Config::GetPtrFromName(UnescapeBindStr(reference.substr(5)), type_name);

        int index = -1;
        return ParseBindInt(reference, index) && index >= 0 ? Config::GetLegacyBindPtr(index, type_name) : nullptr;
    }

    void SaveToConfig() {
        std::ostringstream ss;

        for (const auto& b : g_checkbox_binds) {
            const void* ptr = b.value ? static_cast<const void*>(b.value) : static_cast<const void*>(b.int_value);
            const std::string target = BindTargetReference(ptr, b.int_value ? "int" : "bool");
            if (target.empty())
                continue;

            ss << "CB;"
                << target << ";"
                << (b.int_value ? 1 : 0) << ";"
                << EscapeBindStr(b.label) << ";"
                << EscapeBindStr(b.group) << ";"
                << b.key << ";"
                << b.type << ";"
                << (b.active_on ? 1 : 0) << ";"
                << (b.show_in_list ? 1 : 0) << ";"
                << (b.enabled ? 1 : 0) << "\n";
        }

        for (const auto& b : g_slider_int_binds) {
            const std::string target = BindTargetReference(b.value, "int");
            if (target.empty())
                continue;

            ss << "SI;"
                << target << ";"
                << EscapeBindStr(b.label) << ";"
                << EscapeBindStr(b.group) << ";"
                << b.key << ";"
                << b.type << ";"
                << b.target << ";"
                << b.min_value << ";"
                << b.max_value << ";"
                << (b.show_in_list ? 1 : 0) << ";"
                << (b.enabled ? 1 : 0) << ";"
                << EscapeBindStr(b.format) << "\n";
        }

        for (const auto& b : g_slider_float_binds) {
            const std::string target = BindTargetReference(b.value, "float");
            if (target.empty())
                continue;

            ss << "SF;"
                << target << ";"
                << EscapeBindStr(b.label) << ";"
                << EscapeBindStr(b.group) << ";"
                << b.key << ";"
                << b.type << ";"
                << b.target << ";"
                << b.min_value << ";"
                << b.max_value << ";"
                << (b.show_in_list ? 1 : 0) << ";"
                << (b.enabled ? 1 : 0) << ";"
                << EscapeBindStr(b.format) << ";"
                << b.scale << "\n";
        }

        for (const auto& b : g_dropdown_binds) {
            const std::string target = BindTargetReference(b.value, "int");
            if (target.empty())
                continue;

            std::string items_joined;
            for (size_t i = 0; i < b.items.size(); ++i) {
                if (i != 0)
                    items_joined += ",";
                items_joined += EscapeBindStr(b.items[i]);
            }

            ss << "DD;"
                << target << ";"
                << EscapeBindStr(b.label) << ";"
                << EscapeBindStr(b.group) << ";"
                << b.key << ";"
                << b.type << ";"
                << b.target_index << ";"
                << (b.show_in_list ? 1 : 0) << ";"
                << (b.enabled ? 1 : 0) << ";"
                << items_joined << "\n";
        }

        for (const auto& b : g_multi_dropdown_bool_binds) {
            const std::string target = BindTargetReference(b.value, "std::vector<bool>");
            if (target.empty())
                continue;

            std::string indices_joined;
            for (size_t i = 0; i < b.target_indices.size(); ++i) {
                if (i != 0)
                    indices_joined += ",";
                indices_joined += std::to_string(b.target_indices[i]);
            }

            std::string items_joined;
            for (size_t i = 0; i < b.items.size(); ++i) {
                if (i != 0)
                    items_joined += ",";
                items_joined += EscapeBindStr(b.items[i]);
            }

            ss << "MDB;"
                << target << ";"
                << EscapeBindStr(b.label) << ";"
                << EscapeBindStr(b.group) << ";"
                << b.key << ";"
                << b.type << ";"
                << indices_joined << ";"
                << (b.show_in_list ? 1 : 0) << ";"
                << (b.enabled ? 1 : 0) << ";"
                << items_joined << "\n";
        }

        for (const auto& b : g_multi_dropdown_int_binds) {
            const std::string target = BindTargetReference(b.value, "std::vector<int>");
            if (target.empty())
                continue;

            std::string indices_joined;
            for (size_t i = 0; i < b.target_indices.size(); ++i) {
                if (i != 0)
                    indices_joined += ",";
                indices_joined += std::to_string(b.target_indices[i]);
            }

            std::string items_joined;
            for (size_t i = 0; i < b.items.size(); ++i) {
                if (i != 0)
                    items_joined += ",";
                items_joined += EscapeBindStr(b.items[i]);
            }

            ss << "MDI;"
                << target << ";"
                << EscapeBindStr(b.label) << ";"
                << EscapeBindStr(b.group) << ";"
                << b.key << ";"
                << b.type << ";"
                << indices_joined << ";"
                << (b.show_in_list ? 1 : 0) << ";"
                << (b.enabled ? 1 : 0) << ";"
                << items_joined << "\n";
        }

        const std::string hex_data = HexEncode(ss.str());
        Config::Set<std::string>(Vars::Binds::Data, hex_data);
    }

    void LoadFromConfig() {
        ResetAll();
        const std::string hex_data = Config::Get<std::string>(Vars::Binds::Data);
        if (hex_data.empty())
            return;

        const std::string raw = HexDecode(hex_data);
        if (raw.empty())
            return;

        std::istringstream in(raw);
        std::string line;

        while (std::getline(in, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();

            if (line.empty())
                continue;

            auto tokens = SplitEscaped(line, ';');
            if (tokens.size() < 2)
                continue;

            std::string kind = tokens[0];
            std::string type_name;
            if (kind == "CB" && tokens.size() >= 10 && (tokens[2] == "0" || tokens[2] == "1"))
                type_name = tokens[2] == "1" ? "int" : "bool";
            else if ((kind == "SI" && tokens.size() >= 12) || (kind == "DD" && tokens.size() >= 9))
                type_name = "int";
            else if (kind == "SF" && tokens.size() >= 13)
                type_name = "float";
            else if (kind == "MDB" && tokens.size() >= 9)
                type_name = "std::vector<bool>";
            else if (kind == "MDI" && tokens.size() >= 9)
                type_name = "std::vector<int>";
            else
                continue;

            const std::size_t key_column = kind == "CB" ? 5 : 4;
            int key = 0;
            int mode = 0;
            if (!ParseBindInt(tokens[key_column], key) || key < 0 || key > 0xA5 ||
                !ParseBindInt(tokens[key_column + 1], mode) ||
                (mode != static_cast<int>(EBindType::hold) && mode != static_cast<int>(EBindType::toggle))) {
                continue;
            }

            void* ptr = ResolveBindTarget(tokens[1], type_name);
            if (!ptr && kind == "DD") {
                void* target = ResolveBindTarget(tokens[1], "std::vector<bool>");
                const int index = Config::GetIndexFromPtr(target);
                if (index >= 0 && Config::variables[index].name.ends_with("_AutoStop")) {
                    int selection = 0;
                    if (!ParseBindInt(tokens[6], selection) || selection < 0 || selection > 2)
                        continue;

                    kind = "MDB";
                    tokens[6] = selection == 1 ? "1" : selection == 2 ? "0" : "";
                    tokens.resize(10);
                    tokens[9] = "Early,Stop Between Shots,In Air,On Land";
                    ptr = target;
                }
            }
            if (!ptr)
                continue;

            if (kind == "CB" && tokens.size() >= 10) {
                CheckboxBind nb{};
                const bool is_int = (tokens[2] == "1");
                if (is_int) {
                    nb.int_value = static_cast<int*>(ptr);
                    nb.backup_int_value = *nb.int_value;
                }
                else {
                    nb.value = static_cast<bool*>(ptr);
                    nb.backup_value = *nb.value;
                }
                nb.label = UnescapeBindStr(tokens[3]);
                nb.group = UnescapeBindStr(tokens[4]);
                try {
                    nb.key = std::stoi(tokens[5]);
                    nb.type = std::stoi(tokens[6]);
                }
                catch (...) { continue; }
                nb.active_on = (tokens[7] == "1");
                nb.show_in_list = (tokens[8] == "1");
                nb.enabled = (tokens[9] == "1");
                nb.uid = g_next_bind_uid++;
                nb.last_down = false;
                nb.hold_active = false;
                g_checkbox_binds.push_back(nb);
            }
            else if (kind == "SI" && tokens.size() >= 12) {
                SliderIntBind nb{};
                nb.value = static_cast<int*>(ptr);
                nb.backup = *nb.value;
                nb.label = UnescapeBindStr(tokens[2]);
                nb.group = UnescapeBindStr(tokens[3]);
                try {
                    nb.key = std::stoi(tokens[4]);
                    nb.type = std::stoi(tokens[5]);
                    nb.target = std::stoi(tokens[6]);
                    nb.min_value = std::stoi(tokens[7]);
                    nb.max_value = std::stoi(tokens[8]);
                }
                catch (...) { continue; }
                nb.show_in_list = (tokens[9] == "1");
                nb.enabled = (tokens[10] == "1");
                nb.format = UnescapeBindStr(tokens[11]);
                nb.uid = g_next_bind_uid++;
                nb.last_down = false;
                nb.active = false;
                g_slider_int_binds.push_back(nb);
            }
            else if (kind == "SF" && tokens.size() >= 13) {
                SliderFloatBind nb{};
                nb.value = static_cast<float*>(ptr);
                nb.backup = *nb.value;
                nb.label = UnescapeBindStr(tokens[2]);
                nb.group = UnescapeBindStr(tokens[3]);
                try {
                    nb.key = std::stoi(tokens[4]);
                    nb.type = std::stoi(tokens[5]);
                    nb.target = std::stof(tokens[6]);
                    nb.min_value = std::stof(tokens[7]);
                    nb.max_value = std::stof(tokens[8]);
                    nb.scale = std::stof(tokens[12]);
                }
                catch (...) { continue; }
                nb.show_in_list = (tokens[9] == "1");
                nb.enabled = (tokens[10] == "1");
                nb.format = UnescapeBindStr(tokens[11]);
                nb.uid = g_next_bind_uid++;
                nb.last_down = false;
                nb.active = false;
                g_slider_float_binds.push_back(nb);
            }
            else if (kind == "DD" && tokens.size() >= 9) {
                DropdownBind nb{};
                nb.value = static_cast<int*>(ptr);
                nb.backup = *nb.value;
                nb.label = UnescapeBindStr(tokens[2]);
                nb.group = UnescapeBindStr(tokens[3]);
                try {
                    nb.key = std::stoi(tokens[4]);
                    nb.type = std::stoi(tokens[5]);
                    nb.target_index = std::stoi(tokens[6]);
                }
                catch (...) { continue; }
                nb.show_in_list = (tokens[7] == "1");
                nb.enabled = (tokens[8] == "1");
                if (tokens.size() > 9 && !tokens[9].empty()) {
                    auto item_tokens = SplitEscaped(tokens[9], ',');
                    for (const auto& itm : item_tokens)
                        nb.items.push_back(UnescapeBindStr(itm));
                }
                nb.uid = g_next_bind_uid++;
                nb.last_down = false;
                nb.active = false;
                g_dropdown_binds.push_back(nb);
            }
            else if (kind == "MDB" && tokens.size() >= 9) {
                MultiDropdownBoolBind nb{};
                nb.value = static_cast<std::vector<bool>*>(ptr);
                nb.backup = *nb.value;
                nb.label = UnescapeBindStr(tokens[2]);
                nb.group = UnescapeBindStr(tokens[3]);
                try {
                    nb.key = std::stoi(tokens[4]);
                    nb.type = std::stoi(tokens[5]);
                }
                catch (...) { continue; }
                if (!tokens[6].empty()) {
                    std::istringstream iss(tokens[6]);
                    std::string idx;
                    while (std::getline(iss, idx, ',')) {
                        try { nb.target_indices.push_back(std::stoi(idx)); }
                        catch (...) {}
                    }
                }
                nb.show_in_list = (tokens[7] == "1");
                nb.enabled = (tokens[8] == "1");
                if (tokens.size() > 9 && !tokens[9].empty()) {
                    auto item_tokens = SplitEscaped(tokens[9], ',');
                    for (const auto& itm : item_tokens)
                        nb.items.push_back(UnescapeBindStr(itm));
                }
                nb.uid = g_next_bind_uid++;
                nb.last_down = false;
                nb.active = false;
                g_multi_dropdown_bool_binds.push_back(nb);
            }
            else if (kind == "MDI" && tokens.size() >= 9) {
                MultiDropdownIntBind nb{};
                nb.value = static_cast<std::vector<int>*>(ptr);
                nb.backup = *nb.value;
                nb.label = UnescapeBindStr(tokens[2]);
                nb.group = UnescapeBindStr(tokens[3]);
                try {
                    nb.key = std::stoi(tokens[4]);
                    nb.type = std::stoi(tokens[5]);
                }
                catch (...) { continue; }
                if (!tokens[6].empty()) {
                    std::istringstream iss(tokens[6]);
                    std::string idx;
                    while (std::getline(iss, idx, ',')) {
                        try { nb.target_indices.push_back(std::stoi(idx)); }
                        catch (...) {}
                    }
                }
                nb.show_in_list = (tokens[7] == "1");
                nb.enabled = (tokens[8] == "1");
                if (tokens.size() > 9 && !tokens[9].empty()) {
                    auto item_tokens = SplitEscaped(tokens[9], ',');
                    for (const auto& itm : item_tokens)
                        nb.items.push_back(UnescapeBindStr(itm));
                }
                nb.uid = g_next_bind_uid++;
                nb.last_down = false;
                nb.active = false;
                g_multi_dropdown_int_binds.push_back(nb);
            }
        }
    }

    static const char* KeyLabel(int key) {
        return (key > 0 && key <= 0xA5) ? k_key_names[key] : "none";
    }

    static bool KeyBound(int key) {
        return key > 0 && key <= 0xA5;
    }

    static void StopCapture(int* key) {
        if (s_capturing_key != key)
            return;

        s_capturing_key = nullptr;
        s_waiting_release = false;
    }

    const char* KeyName(int key) {
        return KeyBound(key) ? k_key_names[key] : "Unbound";
    }

    std::string EntryCategory(const std::string& group) {
        if (group.empty())
            return "Other";

        const size_t pos = group.find(" / ");
        return pos == std::string::npos ? group : group.substr(0, pos);
    }

    void BeginCapture(int* key) {
        if (!key || s_capturing_key)
            return;

        s_capturing_key = key;
        s_waiting_release = true;
        ImGui::ClearActiveID();
    }

    void CancelCapture() {
        s_capturing_key = nullptr;
        s_waiting_release = false;
    }

    bool IsCapturing(int* key) {
        return key && s_capturing_key == key;
    }

    void PollCapture() {
        PollKeyCapture();
    }

    std::vector<Entry> GetEntries() {
        std::vector<Entry> out;

        auto make = [](auto& b, EEntryKind kind, bool active) {
            Entry e{};
            e.uid = b.uid;
            e.label = b.label;
            e.group = b.group;
            e.kind = kind;
            e.key = &b.key;
            e.type = &b.type;
            e.enabled = &b.enabled;
            e.visible = &b.show_in_list;
            e.active = active;
            return e;
            };

        for (auto& b : g_checkbox_binds) {
            Entry e = make(b, EEntryKind::checkbox, CheckboxActive(b));
            e.target_bool = &b.active_on;
            out.push_back(e);
        }

        for (auto& b : g_slider_int_binds) {
            Entry e = make(b, EEntryKind::slider_int, b.active);
            e.target_int = &b.target;
            e.v_min = static_cast<float>(b.min_value);
            e.v_max = static_cast<float>(b.max_value);
            e.format = b.format;
            out.push_back(e);
        }

        for (auto& b : g_slider_float_binds) {
            Entry e = make(b, EEntryKind::slider_float, b.active);
            e.target_float = &b.target;
            e.v_min = b.min_value;
            e.v_max = b.max_value;
            e.scale = b.scale;
            e.format = b.format;
            out.push_back(e);
        }

        for (auto& b : g_dropdown_binds) {
            Entry e = make(b, EEntryKind::dropdown, b.active);
            e.target_int = &b.target_index;
            e.items = &b.items;
            out.push_back(e);
        }

        for (auto& b : g_multi_dropdown_bool_binds) {
            Entry e = make(b, EEntryKind::multi, b.active);
            e.target_indices = &b.target_indices;
            e.items = &b.items;
            out.push_back(e);
        }

        for (auto& b : g_multi_dropdown_int_binds) {
            Entry e = make(b, EEntryKind::multi, b.active);
            e.target_indices = &b.target_indices;
            e.items = &b.items;
            out.push_back(e);
        }

        return out;
    }

    void SetEnabled(uint32_t uid, bool on) {
        ForBind(uid, [&](auto& vec, size_t i) {
            auto& b = vec[i];
            b.enabled = on;
            b.last_down = false;
            if (!on)
                ForceDeactivate(b);
            });
    }

    void Remove(uint32_t uid) {
        ForBind(uid, [&](auto& vec, size_t i) {
            ForceDeactivate(vec[i]);
            StopCapture(&vec[i].key);
            vec.erase(vec.begin() + i);
            });
    }

    void ResetAll() {
        auto wipe = [&](auto& vec) {
            for (auto& b : vec) {
                ForceDeactivate(b);
                StopCapture(&b.key);
            }
            vec.clear();
            };

        wipe(g_checkbox_binds);
        wipe(g_slider_int_binds);
        wipe(g_slider_float_binds);
        wipe(g_dropdown_binds);
        wipe(g_multi_dropdown_bool_binds);
        wipe(g_multi_dropdown_int_binds);

        CancelCapture();
        g_active_owner.clear();
        g_backup_checkbox.clear();
        g_backup_slider_int.clear();
        g_backup_slider_float.clear();
        g_backup_dropdown.clear();
        g_backup_multi_bool.clear();
        g_backup_multi_int.clear();
    }

    static void PushPopupStyle(float pad, float spacing) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, pad));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, spacing));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, BindUi::k_win_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));
    }

    static void PopPopupStyle() {
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor();
    }

    template <typename BindVec, typename ValuePtr>
    static void RenderBindPopupImpl(const char* popup_name, BindVec& all_binds, ValuePtr value_ptr, bool has_extra, auto&& draw_extra, auto&& create_new) {
        ImFont* text_font = Var->font.inter[0] ? Var->font.inter[0] : ImGui::GetFont();
        ImFont* icon_font = Var->font.fa[0] ? Var->font.fa[0] : text_font;

        PushPopupStyle(BindUi::k_list_pad, 4.0f);

        if (!ImGui::BeginPopup(popup_name, BindUi::k_popup_flags)) {
            PopPopupStyle();
            return;
        }

        const ImGuiID popup_id = ImGui::GetID(popup_name);

        static std::unordered_map<ImGuiID, int> s_edit_idx;
        static std::unordered_map<ImGuiID, float> s_list_alpha;
        static std::unordered_map<ImGuiID, float> s_edit_alpha;

        if (ImGui::IsWindowAppearing()) {
            s_edit_idx[popup_id] = -1;
            s_list_alpha[popup_id] = 0.0f;
            s_edit_alpha[popup_id] = 0.0f;
        }

        PollKeyCapture();

        float& alpha = s_list_alpha[popup_id];
        alpha = ImClamp(alpha + Gui->FixedSpeed(16.0f), 0.0f, 1.0f);

        const ImGuiIO& io = ImGui::GetIO();
        const ImVec2 win_pos = ImGui::GetWindowPos();
        const ImVec2 win_size = ImGui::GetWindowSize();
        const ImRect win_rect(win_pos, win_pos + win_size);

        if (alpha > 0.95f && !win_rect.Contains(io.MousePos) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
            if (io.MouseClicked[0] || io.MouseClicked[1]) {
                StopCapture(s_capturing_key);
                ImGui::CloseCurrentPopup();
            }
        }

        auto matches = [&](const auto& b) -> bool {
            if constexpr (std::is_same_v<BindVec, std::vector<CheckboxBind>>) {
                if constexpr (std::is_same_v<ValuePtr, int*>)
                    return b.int_value == value_ptr;
                else
                    return b.value == value_ptr;
            }
            else {
                return b.value == value_ptr;
            }
            };

        std::vector<int> indices;
        for (int i = 0; i < static_cast<int>(all_binds.size()); ++i) {
            if (matches(all_binds[i]))
                indices.push_back(i);
        }

        int& edit_idx = s_edit_idx[popup_id];
        const bool edit_open = ImGui::IsPopupOpen("##edit");

        if (edit_idx >= 0 && (edit_idx >= static_cast<int>(all_binds.size()) || !matches(all_binds[edit_idx])))
            edit_idx = -1;

        int open_edit = -1;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        {
            const float width = BindUi::k_list_w;
            BindUi::Context ctx{ ImGui::GetWindowDrawList(), text_font, icon_font, alpha };
            BindUi::Window(ctx, win_pos, win_size);

            for (const int i : indices) {
                auto& b = all_binds[i];
                const bool bound = KeyBound(b.key);

                ImGui::PushID(i);
                if (BindUi::BindRow(ctx, "##bind", width, bound ? k_key_names[b.key] : "Unbound", bound, edit_open && edit_idx == i))
                    open_edit = i;
                ImGui::PopID();
            }

            if (BindUi::ItemRow(ctx, "##all_hotkeys", width, icon_font, ICON_FA_KEYBOARD, "All Hotkeys")) {
                StopCapture(s_capturing_key);
                Var->gui.active_tab_id = Var->gui.hotkeys_tab_id;

                for (auto& cat : Var->gui.categories) {
                    for (auto& tab : cat.tabs) {
                        if (tab.id == Var->gui.hotkeys_tab_id) {
                            tab.active_subtab = 0;
                            break;
                        }
                    }
                }

                ImGui::CloseCurrentPopup();
            }

            if (BindUi::ItemRow(ctx, "##new_hotkey", width, text_font, "+", "New Hotkey")) {
                create_new();
                open_edit = static_cast<int>(all_binds.size()) - 1;
            }

            BindUi::Separator(ctx, width);

            if (BindUi::DangerButton(ctx, "##reset", width, 6.0f, ICON_FA_ROTATE_LEFT, "Reset")) {
                for (const int i : indices)
                    StopCapture(&all_binds[i].key);

                for (int ii = static_cast<int>(indices.size()) - 1; ii >= 0; --ii)
                    all_binds.erase(all_binds.begin() + indices[ii]);

                edit_idx = -1;
                open_edit = -1;
            }
        }
        ImGui::PopStyleVar(1);

        if (open_edit >= 0) {
            edit_idx = open_edit;
            ImGui::OpenPopup("##edit");
        }

        {
            float& edit_alpha = s_edit_alpha[popup_id];

            PushPopupStyle(BindUi::k_edit_pad, 6.0f);
            ImGui::SetNextWindowPos(ImVec2(win_pos.x + win_size.x + BindUi::k_popup_gap - (1.0f - PopupUi::Ease(edit_alpha)) * 8.0f, win_pos.y), ImGuiCond_Always);

            if (ImGui::BeginPopup("##edit", BindUi::k_popup_flags)) {
                if (ImGui::IsWindowAppearing())
                    edit_alpha = 0.0f;

                edit_alpha = ImClamp(edit_alpha + Gui->FixedSpeed(16.0f), 0.0f, 1.0f);

                if (edit_idx < 0 || edit_idx >= static_cast<int>(all_binds.size())) {
                    ImGui::CloseCurrentPopup();
                }
                else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, edit_alpha);

                    const float width = BindUi::k_edit_w;
                    auto& b = all_binds[edit_idx];

                    BindUi::Context ctx{ ImGui::GetWindowDrawList(), text_font, icon_font, edit_alpha };
                    BindUi::Window(ctx, ImGui::GetWindowPos(), ImGui::GetWindowSize());

                    const bool capturing = (s_capturing_key == &b.key);
                    const bool bound = KeyBound(b.key);
                    const char* key_label = capturing ? "Press key..." : (bound ? k_key_names[b.key] : "Key");

                    if (BindUi::KeyRow(ctx, "##key", width, key_label, bound, capturing) && !s_capturing_key) {
                        s_capturing_key = &b.key;
                        s_waiting_release = true;
                    }

                    BindUi::Header(ctx, width, ICON_FA_SLIDERS, "Mode");

                    const BindUi::Segment modes[] = {
                        { "Toggle", static_cast<int>(EBindType::toggle) },
                        { "Hold", static_cast<int>(EBindType::hold) }
                    };
                    BindUi::Segments(ctx, "##mode", width, &b.type, modes, 2);

                    if (has_extra) {
                        ImGui::PushID("##extra_scope");
                        draw_extra(ctx, width, b);
                        ImGui::PopID();
                    }

                    BindUi::CheckboxRow(ctx, "##visible", width, ICON_FA_EYE, "Visible", &b.show_in_list);
                    BindUi::Separator(ctx, width);

                    const bool remove = BindUi::DangerButton(ctx, "##delete", width, 8.0f, ICON_FA_TRASH, "Delete");

                    ImGui::PopStyleVar(1);

                    if (remove) {
                        StopCapture(&b.key);
                        all_binds.erase(all_binds.begin() + edit_idx);
                        edit_idx = -1;
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }
            else {
                edit_alpha = 0.0f;
                if (edit_idx >= 0 && edit_idx < static_cast<int>(all_binds.size()))
                    StopCapture(&all_binds[edit_idx].key);
            }

            PopPopupStyle();
        }

        ImGui::EndPopup();
        PopPopupStyle();
    }

    std::vector<ActiveBind> GetActiveList() {
        std::vector<ActiveBind> out;

        for (const auto& b : g_checkbox_binds) {
            if (b.key == 0 || !b.show_in_list || b.label.empty())
                continue;

            out.push_back({ b.label, KeyLabel(b.key), CheckboxActive(b) });
        }

        for (const auto& b : g_slider_int_binds) {
            if (b.key == 0 || !b.show_in_list || b.label.empty())
                continue;

            out.push_back({ b.label, KeyLabel(b.key), b.active });
        }

        for (const auto& b : g_slider_float_binds) {
            if (b.key == 0 || !b.show_in_list || b.label.empty())
                continue;

            out.push_back({ b.label, KeyLabel(b.key), b.active });
        }

        for (const auto& b : g_dropdown_binds) {
            if (b.key == 0 || !b.show_in_list || b.label.empty())
                continue;

            out.push_back({ b.label, KeyLabel(b.key), b.active });
        }

        for (const auto& b : g_multi_dropdown_bool_binds) {
            if (b.key == 0 || !b.show_in_list || b.label.empty())
                continue;

            out.push_back({ b.label, KeyLabel(b.key), b.active });
        }

        for (const auto& b : g_multi_dropdown_int_binds) {
            if (b.key == 0 || !b.show_in_list || b.label.empty())
                continue;

            out.push_back({ b.label, KeyLabel(b.key), b.active });
        }

        return out;
    }

    static void DrawCheckboxExtra(const BindUi::Context& ctx, float width, CheckboxBind& b) {
        BindUi::Header(ctx, width, ICON_FA_CODE_BRANCH, "Value");

        int val = b.active_on ? 1 : 0;
        const BindUi::Segment values[] = { { "On", 1 }, { "Off", 0 } };
        BindUi::Segments(ctx, "##value", width, &val, values, 2);
        b.active_on = (val == 1);
    }

    void RenderCheckboxPopup(const char* popup_name, std::string_view label, bool* value) {
        RenderBindPopupImpl(popup_name, g_checkbox_binds, value, true, DrawCheckboxExtra, [value, label]() {
            CheckboxBind nb{};
            nb.value = value;
            nb.label = label;
            nb.key = 0;
            nb.type = static_cast<int>(EBindType::toggle);
            nb.active_on = true;
            nb.show_in_list = true;
            nb.last_down = false;
            nb.hold_active = false;
            nb.backup_value = *value;
            nb.uid = g_next_bind_uid++;
            nb.enabled = true;
            nb.group = CurrentGroup();
            g_checkbox_binds.push_back(nb);
            });
    }

    void RenderCheckboxPopup(const char* popup_name, std::string_view label, int* value) {
        RenderBindPopupImpl(popup_name, g_checkbox_binds, value, true, DrawCheckboxExtra, [value, label]() {
            CheckboxBind nb{};
            nb.int_value = value;
            nb.label = label;
            nb.key = 0;
            nb.type = static_cast<int>(EBindType::toggle);
            nb.active_on = true;
            nb.show_in_list = true;
            nb.last_down = false;
            nb.hold_active = false;
            nb.backup_int_value = *value;
            nb.uid = g_next_bind_uid++;
            nb.enabled = true;
            nb.group = CurrentGroup();
            g_checkbox_binds.push_back(nb);
            });
    }

    void RenderSliderIntPopup(const char* popup_name, std::string_view label, int* value, int v_min, int v_max, const char* format) {
        RenderBindPopupImpl(popup_name, g_slider_int_binds, value, true,
            [v_min, v_max, format](const BindUi::Context& ctx, float width, SliderIntBind& b) {
                if (format && b.format != format)
                    b.format = format;

                BindUi::SliderRow(ctx, "##value", width, ICON_FA_CODE_BRANCH, "Value", ImGuiDataType_S32, &b.target, &v_min, &v_max, format ? format : "%d");
            },
            [value, label, v_min, v_max, format]() {
                SliderIntBind nb{};
                nb.value = value;
                nb.label = label;
                nb.key = 0;
                nb.type = static_cast<int>(EBindType::toggle);
                nb.target = *value;
                nb.min_value = v_min;
                nb.max_value = v_max;
                nb.show_in_list = true;
                nb.last_down = false;
                nb.active = false;
                nb.backup = *value;
                nb.uid = g_next_bind_uid++;
                nb.enabled = true;
                nb.group = CurrentGroup();
                nb.format = format ? format : "%d";
                g_slider_int_binds.push_back(nb);
            }
        );
    }

    void RenderSliderFloatPopup(const char* popup_name, std::string_view label, float* value, float v_min, float v_max, const char* format, float scale) {
        if (scale == 0.0f)
            scale = 1.0f;

        RenderBindPopupImpl(popup_name, g_slider_float_binds, value, true,
            [v_min, v_max, format, scale](const BindUi::Context& ctx, float width, SliderFloatBind& b) {
                if (format && b.format != format)
                    b.format = format;

                b.scale = scale;

                float scaled_val = b.target * scale;
                float scaled_min = v_min * scale;
                float scaled_max = v_max * scale;

                if (BindUi::SliderRow(ctx, "##value", width, ICON_FA_CODE_BRANCH, "Value", ImGuiDataType_Float, &scaled_val, &scaled_min, &scaled_max, format ? format : "%.1f"))
                    b.target = scaled_val / scale;
            },
            [value, label, v_min, v_max, format, scale]() {
                SliderFloatBind nb{};
                nb.value = value;
                nb.label = label;
                nb.key = 0;
                nb.type = static_cast<int>(EBindType::toggle);
                nb.target = *value;
                nb.min_value = v_min;
                nb.max_value = v_max;
                nb.show_in_list = true;
                nb.last_down = false;
                nb.active = false;
                nb.backup = *value;
                nb.uid = g_next_bind_uid++;
                nb.enabled = true;
                nb.group = CurrentGroup();
                nb.format = format ? format : "%.1f";
                nb.scale = scale;
                g_slider_float_binds.push_back(nb);
            }
        );
    }

    void RenderDropdownPopup(const char* popup_name, std::string_view label, int* value, const std::vector<std::string>& items) {
        RenderBindPopupImpl(popup_name, g_dropdown_binds, value, true,
            [&items](const BindUi::Context& ctx, float width, DropdownBind& b) {
                if (b.items != items)
                    b.items = items;

                BindUi::Dropdown(ctx, "##value", width, ICON_FA_CODE_BRANCH, "Value", &b.target_index, items);
            },
            [value, label, &items]() {
                DropdownBind nb{};
                nb.value = value;
                nb.label = label;
                nb.key = 0;
                nb.type = static_cast<int>(EBindType::toggle);
                nb.target_index = *value;
                nb.show_in_list = true;
                nb.last_down = false;
                nb.active = false;
                nb.backup = *value;
                nb.uid = g_next_bind_uid++;
                nb.enabled = true;
                nb.group = CurrentGroup();
                nb.items = items;
                g_dropdown_binds.push_back(nb);
            }
        );
    }

    void RenderMultiDropdownPopup(const char* popup_name, std::string_view label, std::vector<bool>* value, const std::vector<std::string>& items) {
        RenderBindPopupImpl(popup_name, g_multi_dropdown_bool_binds, value, true,
            [&items](const BindUi::Context& ctx, float width, MultiDropdownBoolBind& b) {
                if (b.items != items)
                    b.items = items;

                std::vector<bool> temp(items.size(), false);
                for (const int idx : b.target_indices) {
                    if (idx >= 0 && idx < static_cast<int>(items.size()))
                        temp[idx] = true;
                }

                if (!BindUi::MultiDropdown(ctx, "##value", width, ICON_FA_CODE_BRANCH, "Value", temp, items))
                    return;

                b.target_indices.clear();
                for (int j = 0; j < static_cast<int>(temp.size()); ++j) {
                    if (temp[j])
                        b.target_indices.push_back(j);
                }
            },
            [value, label, &items]() {
                MultiDropdownBoolBind nb{};
                nb.value = value;
                nb.label = label;
                nb.key = 0;
                nb.type = static_cast<int>(EBindType::toggle);
                nb.show_in_list = true;
                nb.last_down = false;
                nb.active = false;
                nb.backup = *value;
                nb.uid = g_next_bind_uid++;
                nb.enabled = true;
                nb.group = CurrentGroup();
                nb.items = items;
                g_multi_dropdown_bool_binds.push_back(nb);
            }
        );
    }

    void RenderMultiDropdownPopup(const char* popup_name, std::string_view label, std::vector<int>* value, const std::vector<std::string>& items) {
        RenderBindPopupImpl(popup_name, g_multi_dropdown_int_binds, value, true,
            [&items](const BindUi::Context& ctx, float width, MultiDropdownIntBind& b) {
                if (b.items != items)
                    b.items = items;

                std::vector<bool> temp(items.size(), false);
                for (const int idx : b.target_indices) {
                    if (idx >= 0 && idx < static_cast<int>(items.size()))
                        temp[idx] = true;
                }

                if (!BindUi::MultiDropdown(ctx, "##value", width, ICON_FA_CODE_BRANCH, "Value", temp, items))
                    return;

                b.target_indices.clear();
                for (int j = 0; j < static_cast<int>(temp.size()); ++j) {
                    if (temp[j])
                        b.target_indices.push_back(j);
                }
            },
            [value, label, &items]() {
                MultiDropdownIntBind nb{};
                nb.value = value;
                nb.label = label;
                nb.key = 0;
                nb.type = static_cast<int>(EBindType::toggle);
                nb.show_in_list = true;
                nb.last_down = false;
                nb.active = false;
                nb.backup = *value;
                nb.uid = g_next_bind_uid++;
                nb.enabled = true;
                nb.group = CurrentGroup();
                nb.items = items;
                g_multi_dropdown_int_binds.push_back(nb);
            }
        );
    }
}