#include <shared.h>

bool CGui::Section(std::string_view label, std::string_view icon, int section_id, int& section_variable, bool last) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());
    const bool selected = section_id == section_variable;

    float icon_size = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize, FLT_MAX, -1.f, icon.data()).x;
    float text_size = Var->font.inter[1]->CalcTextSizeA(Var->font.inter[1]->LegacySize, FLT_MAX, -1.f, label.data()).x;
    float total_width = icon_size + text_size + Elements->section.icon_padding;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + Elements->section.size);
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        section_variable = section_id;

    const ImU32 icon_color = selected ? OtcStyle::Accent() : (hovered ? OtcStyle::Text() : OtcStyle::TextDim());
    const ImU32 text_color = selected ? OtcStyle::Accent() : (hovered ? OtcStyle::Text() : OtcStyle::TextDim());

    float start_x = rect.GetCenter().x - total_width / 2;
    ImVec2 icon_pos(start_x, rect.GetCenter().y);
    ImVec2 text_pos(start_x + icon_size + Elements->section.icon_padding, rect.GetCenter().y);

    if (selected)
        window->DrawList->AddRectFilled(ImVec2(rect.Min.x, rect.Max.y - 2.f), ImVec2(rect.Max.x, rect.Max.y), OtcStyle::Accent());

    Draw->TextClipped(window->DrawList, Var->font.fa[1], ImVec2(rect.Min.x, icon_pos.y), ImVec2(rect.Max.x, icon_pos.y), icon_color, icon.data(), NULL, NULL, ImVec2(0.f, 0.5f));
    Draw->TextClipped(window->DrawList, Var->font.inter[1], ImVec2(rect.Min.x, text_pos.y), ImVec2(rect.Max.x, text_pos.y), text_color, label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    return pressed;
}

bool CGui::SubSection(std::string_view label, std::string_view icon, int section_id, int& section_variable) {
    struct sub_section_state {
        float alpha{ 0 };
        ImVec4 text_col{ ImGui::ColorConvertU32ToFloat4(OtcStyle::TextDim()) };
        ImVec4 icon_col{ ImGui::ColorConvertU32ToFloat4(OtcStyle::TextDim()) };
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());
    const bool selected = section_id == section_variable;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(Elements->sub_section.size));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        section_variable = section_id;

    sub_section_state* state = Gui->AnimContainer(&state, id);

    state->alpha = ImClamp(state->alpha + (Gui->FixedSpeed(8.f) * (selected ? 1.f : -1.f)), 0.f, 1.f);
    state->text_col = ImLerp(state->text_col, selected ? ImGui::ColorConvertU32ToFloat4(OtcStyle::Accent()) : ImGui::ColorConvertU32ToFloat4(OtcStyle::TextDim()), Gui->FixedSpeed(12.f));
    state->icon_col = ImLerp(state->icon_col, selected ? ImGui::ColorConvertU32ToFloat4(OtcStyle::Accent()) : ImGui::ColorConvertU32ToFloat4(OtcStyle::TextDim()), Gui->FixedSpeed(12.f));

    Draw->RectFilled(window->DrawList, rect.Min, rect.Max, OtcStyle::Control(0.35f * state->alpha));

    float icon_size = Var->font.fa[0]->CalcTextSizeA(Var->font.fa[0]->LegacySize, FLT_MAX, -1.f, icon.data()).x;
    float text_size = Var->font.inter[0]->CalcTextSizeA(Var->font.inter[0]->LegacySize, FLT_MAX, -1.f, label.data()).x;
    float total_width = icon_size + text_size + 8.f;

    float start_x = rect.GetCenter().x - total_width / 2;

    Draw->TextClipped(window->DrawList, Var->font.fa[0], ImVec2(start_x, rect.GetCenter().y), ImVec2(start_x + icon_size, rect.GetCenter().y), Draw->GetClr(state->icon_col), icon.data(), NULL, NULL, ImVec2(0.f, 0.5f));
    Draw->TextClipped(window->DrawList, Var->font.inter[0], ImVec2(start_x + icon_size + 8.f, rect.GetCenter().y), ImVec2(start_x + icon_size + 8.f + text_size, rect.GetCenter().y), Draw->GetClr(state->text_col), label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    return pressed;
}