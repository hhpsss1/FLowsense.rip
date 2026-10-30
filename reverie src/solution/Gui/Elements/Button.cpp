#include "../settings/Functions.h"

bool CGui::Buttton(std::string_view label, const ImVec2& size_arg) {
    struct C_ButtonState {
        bool clicked{ false };
        float alpha{ 0 };
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = size_arg;

    if (size_arg.x == 0)
        size.x = GetContentRegionAvail().x;
    if (size_arg.y == 0)
        size.y = Elements->button.height;

    const ImRect rect(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);

    C_ButtonState* state = Gui->AnimContainer(&state, id);

    if (pressed)
        state->clicked = true;

    state->alpha = ImClamp(state->alpha + (Gui->FixedSpeed(8.f) * (state->clicked ? 1.f : -1.f)), 0.f, 1.f);

    if (state->alpha > 0.9f)
        state->clicked = false;

    Draw->RectFilled(window->DrawList, rect.Min, rect.Max, Draw->GetClr(Colors->button.background), Elements->button.rounding);
    Draw->RectFilled(window->DrawList, rect.Min, rect.Max, Draw->GetClr(Colors->accent, state->alpha), Elements->button.rounding);
    Draw->TextClipped(window->DrawList, Var->font.inter[2], rect.Min - ImVec2(0, 1), rect.Max - ImVec2(0, 1), Draw->GetClr(Colors->button.text), label.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    return pressed;
}