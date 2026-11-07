#include "../settings/Functions.h"
#include <xorstring/xorstring.h>

#ifndef RenderNavHighlight
#define RenderNavHighlight ImGui::RenderNavCursor
#endif

#ifndef ImGuiNavHighlightFlags_Compact
#ifdef ImGuiNavRenderHighlightFlags_Compact
#define ImGuiNavHighlightFlags_Compact ImGuiNavRenderHighlightFlags_Compact
#else
#define ImGuiNavHighlightFlags_Compact (1 << 0)
#endif
#endif

bool BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags, const char* fa_icon) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    IM_ASSERT(id != 0);

    const ImGuiChildFlags ImGuiChildFlags_SupportedMask_ = ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_FrameStyle;
    IM_UNUSED(ImGuiChildFlags_SupportedMask_);

    if (child_flags & ImGuiChildFlags_AutoResizeX)
        child_flags &= ~ImGuiChildFlags_ResizeX;
    if (child_flags & ImGuiChildFlags_AutoResizeY)
        child_flags &= ~ImGuiChildFlags_ResizeY;

    window_flags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar;
    window_flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);
    if (child_flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize))
        window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    if ((child_flags & (ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY)) == 0)
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    if (child_flags & ImGuiChildFlags_FrameStyle) {
        PushStyleColor(ImGuiCol_ChildBg, g.Style.Colors[ImGuiCol_FrameBg]);
        PushStyleVar(ImGuiStyleVar_ChildRounding, g.Style.FrameRounding);
        PushStyleVar(ImGuiStyleVar_ChildBorderSize, g.Style.FrameBorderSize);
        PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.FramePadding);
        child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
        window_flags |= ImGuiWindowFlags_NoMove;
    }

    g.NextWindowData.HasFlags |= ImGuiNextWindowDataFlags_HasChildFlags;
    g.NextWindowData.ChildFlagsSet = child_flags;

    float* state = Gui->AnimContainer(&state, id);

    ImVec2 size = size_arg;
    if (size.x <= 0)
        size.x = 211.f;
    if (size.y <= 0)
        size.y = *state;

    ImFont* title_font = Var->font.inter[1] ? Var->font.inter[1] : Var->font.inter[0];
    const float label_padding_x = 10.0f;
    const float header_top = 5.0f;
    const float header_total = header_top;

    ImVec2 label_min = parent_window->DC.CursorPos;
    ImVec2 card_pos = ImVec2(label_min.x, label_min.y + header_top);

    Gui->SetNextWindowSize(size);
    Gui->SetNextWindowPos(card_pos);

    const char* temp_window_name;
    if (name) ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);
    else ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);

    const float backup_border_size = 0.0f;
    const float backup_child_rounding = g.Style.ChildRounding;
    g.Style.ChildRounding = 0.f;

    const float a = g.Style.Alpha;
    const ImVec2 box_min = card_pos;
    const ImVec2 box_max(label_min.x + size.x + 1.f, card_pos.y + size.y + 1.f);
    const char* title = name ? name : "";
    const float title_x = label_min.x + label_padding_x;
    const float title_w = title_font->CalcTextSizeA(title_font->LegacySize, FLT_MAX, -1.f, title).x;

    ImDrawList* pdl = parent_window->DrawList;
    pdl->AddRectFilled(box_min + ImVec2(1.f, 1.f), box_max, OtcStyle::GroupFace(a));
    pdl->AddRectFilled(box_min, ImVec2(title_x - 2.f, box_min.y + 1.f), OtcStyle::GroupLine(a));
    pdl->AddRectFilled(ImVec2(title_x + title_w + 2.f, box_min.y), ImVec2(box_max.x + 1.f, box_min.y + 1.f), OtcStyle::GroupLine(a));
    pdl->AddRectFilled(box_min, ImVec2(box_min.x + 1.f, box_max.y + 1.f), OtcStyle::GroupLine(a));
    pdl->AddRectFilled(ImVec2(box_max.x, box_min.y), ImVec2(box_max.x + 1.f, box_max.y + 1.f), OtcStyle::GroupLine(a));
    pdl->AddRectFilled(ImVec2(box_max.x + 1.f, box_min.y), ImVec2(box_max.x + 2.f, box_max.y + 1.f), OtcStyle::Border(a));
    pdl->AddRectFilled(ImVec2(box_min.x, box_max.y), ImVec2(box_max.x + 1.f, box_max.y + 1.f), OtcStyle::GroupLine(a));
    pdl->AddRectFilled(ImVec2(box_min.x, box_max.y + 1.f), ImVec2(box_max.x + 2.f, box_max.y + 2.f), OtcStyle::Border(a));

    pdl->AddText(title_font, title_font->LegacySize, ImVec2(title_x, ImFloor(box_min.y - title_font->LegacySize * 0.5f + 1.f)), OtcStyle::Text(a), title);

    const bool ret = Gui->Begin(temp_window_name, NULL, window_flags);

    g.Style.ChildRounding = backup_child_rounding;
    g.Style.ChildBorderSize = backup_border_size;

    if (child_flags & ImGuiChildFlags_FrameStyle) {
        PopStyleVar(3);
        PopStyleColor();
    }

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;

    *state = child_window->ContentSize.y + Elements->child.padding.y * 2;

    if (child_window->BeginCount == 1)
        parent_window->DC.CursorPos = child_window->Pos - ImVec2(0.f, header_total);

    const ImGuiID temp_id_for_activation = ImHashStr("##Child", 0, id);
    if (g.ActiveId == temp_id_for_activation)
        ClearActiveID();

    if (g.NavActivateId == id && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY)) {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(temp_id_for_activation, child_window);
        g.ActiveIdSource = g.NavInputSource;
    }

    return ret;
}

void CGui::BeginChild(std::string_view name, const char* fa_icon, const ImVec2& size) {
    g_child_stack.push_back(std::string(name));

    ImGuiID id = GetCurrentWindow()->GetID(name.data());
    Gui->PushStyleVar(ImGuiStyleVar_WindowPadding, Elements->child.padding);
    BeginChildEx(name.data(), id, size, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse, fa_icon);
    Gui->PushStyleVar(ImGuiStyleVar_ItemSpacing, Elements->child.spacing);
}

void CGui::EndChild() {
    if (!g_child_stack.empty())
        g_child_stack.pop_back();

    ImGuiContext& g = *GImGui;
    ImGuiWindow* child_window = g.CurrentWindow;

    IM_ASSERT(child_window->Flags & ImGuiWindowFlags_ChildWindow);

    Gui->PopStyleVar();

    g.WithinEndChildID = child_window->ID;
    ImVec2 child_size = child_window->Size;
    Gui->End();

    if (child_window->BeginCount == 1) {
        ImGuiWindow* parent_window = g.CurrentWindow;

        const float header_total = 5.0f;

        ImVec2 full_size = child_size + ImVec2(0.f, header_total);
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + full_size);
        ItemSize(full_size);
        if ((child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY)) {
            ItemAdd(bb, child_window->ChildId);
            RenderNavHighlight(bb, child_window->ChildId);
            if (child_window->DC.NavLayersActiveMask == 0 && child_window == g.NavWindow)
                RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_Compact);
        } else {
            ItemAdd(bb, 0);
            if (child_window->Flags & ImGuiChildFlags_NavFlattened)
                parent_window->DC.NavLayersActiveMaskNext |= child_window->DC.NavLayersActiveMaskNext;
        } 
        if (g.HoveredWindow == child_window)
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
    }

    g.WithinEndChildID = 0;
    Gui->PopStyleVar();
    g.LogLinePosY = -FLT_MAX;
}