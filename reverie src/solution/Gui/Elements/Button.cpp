#include "../settings/Functions.h"

bool CGui::Buttton(std::string_view label, const ImVec2& size_arg) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;

    ImVec2 size = size_arg;
    if (size.x == 0.f)
        size.x = 150.f;
    if (size.y == 0.f)
        size.y = 20.f;

    const float bx = pos.x + 20.f;
    const float by = pos.y;
    const ImRect rect(ImVec2(bx, by), ImVec2(bx + size.x, by + size.y));

    ItemSize(ImVec2(size.x + 20.f, 25.f), style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);

    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(rect.Min - ImVec2(1.f, 1.f), rect.Max + ImVec2(1.f, 1.f), OtcStyle::Border());
    dl->AddRectFilled(rect.Min, rect.Max, OtcStyle::Control(1.f));

    ImFont* font = Var->font.inter[2] ? Var->font.inter[2] : Var->font.inter[0];
    if (font) {
        const ImVec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, -1.f, label.data());
        const ImVec2 text_pos(ImFloor(rect.Min.x + (size.x - text_size.x) * 0.5f), ImFloor(rect.Min.y + (size.y - text_size.y) * 0.5f));
        dl->AddText(font, font->LegacySize, text_pos, OtcStyle::Text(), label.data());
    }

    return pressed;
}
