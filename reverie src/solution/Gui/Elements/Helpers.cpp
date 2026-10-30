#include <shared.h>

void CGui::PushStyleColor(ImGuiCol idx, ImU32 col) { ImGui::PushStyleColor(idx, col); }
void CGui::PopStyleColor(int count) { ImGui::PopStyleColor(count); }
void CGui::PushStyleVar(ImGuiStyleVar idx, float val) { ImGui::PushStyleVar(idx, val); }
void CGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val) { ImGui::PushStyleVar(idx, val); }
void CGui::PopStyleVar(int count) { ImGui::PopStyleVar(count); }
void CGui::PushFont(ImFont* font) { ImGui::PushFont(font); }
void CGui::PopFont() { ImGui::PopFont(); }
void CGui::SetCursorPos(const ImVec2& local_pos) { ImGui::SetCursorPos(local_pos); }
void CGui::BeginGroup() { ImGui::BeginGroup(); }
void CGui::EndGroup() { ImGui::EndGroup(); }
void CGui::Sameline() { ImGui::SameLine(0.f, -1.f); }

void CGui::SetNextWindowPos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot) { ImGui::SetNextWindowPos(pos, cond, pivot); }
void CGui::SetNextWindowSize(const ImVec2& size, ImGuiCond cond) { ImGui::SetNextWindowSize(size, cond); }

void CGui::BeginContent() {
    Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, Elements->content.padding);
    Gui->BeginDefChild(xs("content area"), ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    Gui->PushStyleVar(ImGuiStyleVar_ItemSpacing, Elements->content.spacing);
}

void CGui::EndContent() {
    Gui->PopStyleVar();
    Gui->EndDefChild();
    Gui->PopStyleVar();
}

bool BeginDefChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    if (!parent_window || id == 0)
        return false;

    return ImGui::BeginChild(id, size_arg, child_flags, window_flags);
}

bool CGui::BeginDefChild(std::string_view name, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags) {
    ImGuiID id = ImGui::GetCurrentWindow()->GetID(name.data());
    return BeginDefChildEx(name.data(), id, size_arg, child_flags, window_flags);
}

void CGui::EndDefChild() {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!window || !(window->Flags & ImGuiWindowFlags_ChildWindow))
        return;

    ImGui::EndChild();
}

bool CGui::SelectableEx(std::string_view label, bool active, const ImVec2& size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + size);
    ImGui::ItemSize(rect, style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered = ImGui::IsItemHovered();
    bool pressed = hovered && g.IO.MouseClicked[0];
    if (pressed)
        ImGui::MarkItemEdited(id);

    ImVec4* state = nullptr;
    state = Gui->AnimContainer(&state, id);

    *state = ImLerp(*state, active ? Colors->selectable.text : Colors->selectable.text_inactive, Gui->FixedSpeed(12.f));

    Draw->TextClipped(window->DrawList, Var->font.inter[2], rect.Min - ImVec2(0, 1), rect.Max - ImVec2(0, 1), Draw->GetClr(*state), label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label.data(), g.LastItemData.StatusFlags);
    return pressed;
}

bool CGui::Selectable(std::string_view label, bool* p_selected, const ImVec2& size) {
    if (SelectableEx(label, *p_selected, size)) {
        *p_selected = !*p_selected;
        return true;
    }

    return false;
}

bool CGui::BeginListBox(const char* label, const ImVec2& size_arg) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(), ImGui::GetTextLineHeightWithSpacing() * 7.25f + style.FramePadding.y * 2.0f);
    ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
    ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    g.NextItemData.ClearFlags();

    if (!ImGui::IsRectVisible(bb.Min, bb.Max)) {
        ImGui::ItemSize(bb.GetSize(), style.FramePadding.y);
        ImGui::ItemAdd(bb, 0, &frame_bb);
        g.NextWindowData.ClearFlags();
        return false;
    }

    Draw->Rect(window->DrawList, frame_bb.Min, frame_bb.Max, Draw->GetClr(Colors->listbox.border), Elements->listbox.rounding, 0, Elements->listbox.border_size);

    if (label_size.x > 0.0f) {
        ImVec2 label_pos = ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y);
        Draw->TextClipped(window->DrawList, Var->font.inter[2], label_pos, label_pos + label_size, Draw->GetClr(Colors->listbox.text), label, NULL, NULL, ImVec2(0.f, 0.f));
        window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, label_pos + label_size);
    }

    Gui->PushStyleVar(ImGuiStyleVar_ChildRounding, Elements->listbox.rounding);
    Gui->PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Elements->listbox.item_padding, 0.0f));
    Gui->PushStyleColor(ImGuiCol_ChildBg, Draw->GetClr(Colors->listbox.background));

    if (!Gui->BeginDefChild(label, frame_bb.GetSize(), ImGuiChildFlags_None)) {
        Gui->EndDefChild();
        Gui->PopStyleColor();
        Gui->PopStyleVar(3);
        return false;
    }

    return true;
}

bool CGui::BeginListBox(const char* label, int items_count, int height_in_items) {
    float height = ImGui::GetTextLineHeightWithSpacing() * ((height_in_items < 0 ? ImMin(items_count, 7) : height_in_items) + 0.25f) + ImGui::GetStyle().FramePadding.y * 2.0f;
    return BeginListBox(label, ImVec2(0.0f, height));
}

void CGui::EndListBox() {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!window || !(window->Flags & ImGuiWindowFlags_ChildWindow))
        return;

    Gui->EndDefChild();
    Gui->PopStyleColor();
    Gui->PopStyleVar(3);
}

void CGui::ElementTooltip(const char* text, float delay) {
    if (!text || !text[0])
        return;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiLastItemData& last = g.LastItemData;
    if (last.ID == 0)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStorage* storage = &window->StateStorage;

    ImRect rect = last.Rect;
    bool hovered = ImGui::IsMouseHoveringRect(rect.Min, rect.Max);

    ImGuiID time_id = last.ID ^ 0x19D21C3Fu;
    ImGuiID alpha_id = last.ID ^ 0x8B73265Au;

    float hover_time = storage->GetFloat(time_id, 0.f);
    if (hovered)
        hover_time += io.DeltaTime;
    else
        hover_time = 0.f;

    storage->SetFloat(time_id, hover_time);

    bool want_show = hover_time >= delay;

    float alpha = storage->GetFloat(alpha_id, 0.f);
    float target = want_show ? 1.f : 0.f;
    float speed = 12.f;
    float t = 1.f - powf(0.001f, io.DeltaTime * speed);

    alpha = ImLerp(alpha, target, t);
    alpha = ImClamp(alpha, 0.f, 1.f);
    storage->SetFloat(alpha_id, alpha);

    if (alpha <= 0.01f)
        return;

    const float rounding = 12.f;
    const ImVec2 padding = ImVec2(10.f, 7.f);
    const float max_width = 230.f;
    const float offset_y = 8.f;

    ImFont* font = Var->font.inter[2];
    if (!font)
        font = ImGui::GetFont();

    ImGui::PushFont(font);
    ImVec2 text_size = ImGui::CalcTextSize(text, nullptr, false, max_width);
    ImGui::PopFont();

    ImVec2 tooltip_size = text_size + padding * 2.0f;

    ImVec2 tooltip_pos;
    tooltip_pos.x = rect.Min.x + (rect.GetWidth() - tooltip_size.x) * 0.5f;
    tooltip_pos.y = rect.Min.y - tooltip_size.y - offset_y;

    ImVec2 display_size = io.DisplaySize;
    tooltip_pos.x = ImClamp(tooltip_pos.x, 4.f, display_size.x - tooltip_size.x - 4.f);

    if (tooltip_pos.y < 4.f)
        tooltip_pos.y = rect.Max.y + offset_y;

    ImVec2 p1 = tooltip_pos;
    ImVec2 p2 = tooltip_pos + tooltip_size;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    ImU32 bg = Draw->GetClr(ImColor(0, 0, 0), alpha * 0.80f);
    ImU32 br = Draw->GetClr(Colors->listbox.border, alpha * 0.70f);
    ImU32 tx = Draw->GetClr(ImColor(255, 255, 255), alpha);

    Draw->RectFilled(dl, p1, p2, bg, rounding, ImDrawFlags_RoundCornersAll);
    dl->AddRect(p1 + ImVec2(0.5f, 0.5f), p2 - ImVec2(0.5f, 0.5f), br, rounding, ImDrawFlags_RoundCornersAll, 1.f);

    ImGui::PushFont(font);
    dl->AddText(font, font->LegacySize, p1 + padding, tx, text, nullptr, max_width);
    ImGui::PopFont();
}