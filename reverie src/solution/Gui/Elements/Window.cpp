#include "../Settings/Functions.h"

#ifndef ImDrawFlags_ShadowCutOutShapeBackground
#define ImDrawFlags_ShadowCutOutShapeBackground (1 << 30)
#endif

#ifndef ImGuiWindowFlags_NavFlattened
#define ImGuiWindowFlags_NavFlattened 0
#endif

#ifndef IM_FLOOR
#define IM_FLOOR(X) ImFloor(X)
#endif

template <typename T, typename = void>
struct NextWindowDataFlagsAccessor {
    static auto get(const T& d) { return d.HasFlags; }
    static void set(T& d, ImGuiNextWindowDataFlags f) { d.HasFlags |= f; }
};

template <typename T>
struct NextWindowDataFlagsAccessor<T, std::void_t<decltype(std::declval<T>().Flags)>> {
    static auto get(const T& d) { return d.Flags; }
    static void set(T& d, ImGuiNextWindowDataFlags f) { d.Flags |= f; }
};

template <typename T>
inline auto GetNextWindowDataFlags(const T& d) {
    return NextWindowDataFlagsAccessor<T>::get(d);
}

template <typename T>
inline ImGuiChildFlags GetNextWindowChildFlags(const T& d) {
    if constexpr (requires { d.ChildFlagsSet; })
        return d.ChildFlagsSet;
    else if constexpr (requires { d.ChildFlags; })
        return d.ChildFlags;
    else
        return 0;
}

template <typename T>
inline void StoreWindowRecoveryState(T& data, ImGuiContext* g) {
    if constexpr (requires { data.StackSizesInBegin; }) {
        auto& s = data.StackSizesInBegin;
        ImGuiWindow* window = g->CurrentWindow;
        if constexpr (requires { s.SizeOfWindowStack; }) s.SizeOfWindowStack = (short)g->CurrentWindowStack.Size;
        if constexpr (requires { s.SizeOfIDStack; }) s.SizeOfIDStack = (short)(window ? window->IDStack.Size : 0);
        if constexpr (requires { s.SizeOfColorStack; }) s.SizeOfColorStack = (short)g->ColorStack.Size;
        if constexpr (requires { s.SizeOfStyleVarStack; }) s.SizeOfStyleVarStack = (short)g->StyleVarStack.Size;
        if constexpr (requires { s.SizeOfFontStack; }) s.SizeOfFontStack = (short)g->FontStack.Size;
        if constexpr (requires { s.SizeOfFocusScopeStack; }) s.SizeOfFocusScopeStack = (short)g->FocusScopeStack.Size;
        if constexpr (requires { s.SizeOfGroupStack; }) s.SizeOfGroupStack = (short)g->GroupStack.Size;
        if constexpr (requires { s.SizeOfItemFlagsStack; }) s.SizeOfItemFlagsStack = (short)g->ItemFlagsStack.Size;
        if constexpr (requires { s.SizeOfBeginPopupStack; }) s.SizeOfBeginPopupStack = (short)g->BeginPopupStack.Size;
        if constexpr (requires { s.SizeOfDisabledStack; }) s.SizeOfDisabledStack = g->DisabledStackSize;
    }
}

static void SetWindowConditionAllowFlag(ImGuiWindow* window, ImGuiCond flags, bool enabled) {
    window->SetWindowPosAllowFlags = enabled ? (window->SetWindowPosAllowFlags | flags) : (window->SetWindowPosAllowFlags & ~flags);
    window->SetWindowSizeAllowFlags = enabled ? (window->SetWindowSizeAllowFlags | flags) : (window->SetWindowSizeAllowFlags & ~flags);
    window->SetWindowCollapsedAllowFlags = enabled ? (window->SetWindowCollapsedAllowFlags | flags) : (window->SetWindowCollapsedAllowFlags & ~flags);
}

static void ApplyWindowSettings(ImGuiWindow* window, ImGuiWindowSettings* settings) {
    window->Pos = ImTrunc(ImVec2(settings->Pos.x, settings->Pos.y));
    if (settings->Size.x > 0 && settings->Size.y > 0)
        window->Size = window->SizeFull = ImTrunc(ImVec2(settings->Size.x, settings->Size.y));
    window->Collapsed = settings->Collapsed;
}

static void UpdateWindowInFocusOrderList(ImGuiWindow* window, bool just_created, ImGuiWindowFlags new_flags) {
    ImGuiContext& g = *GImGui;

    const bool new_is_explicit_child = (new_flags & ImGuiWindowFlags_ChildWindow) != 0 && ((new_flags & ImGuiWindowFlags_Popup) == 0 || (new_flags & ImGuiWindowFlags_ChildMenu) != 0);
    const bool child_flag_changed = new_is_explicit_child != window->IsExplicitChild;
    if ((just_created || child_flag_changed) && !new_is_explicit_child) {
        IM_ASSERT(!g.WindowsFocusOrder.contains(window));
        g.WindowsFocusOrder.push_back(window);
        window->FocusOrder = (short)(g.WindowsFocusOrder.Size - 1);
    }
    else if (!just_created && child_flag_changed && new_is_explicit_child) {
        IM_ASSERT(g.WindowsFocusOrder[window->FocusOrder] == window);
        for (int n = window->FocusOrder + 1; n < g.WindowsFocusOrder.Size; n++)
            g.WindowsFocusOrder[n]->FocusOrder--;
        g.WindowsFocusOrder.erase(g.WindowsFocusOrder.Data + window->FocusOrder);
        window->FocusOrder = -1;
    }
    window->IsExplicitChild = new_is_explicit_child;
}

static void InitOrLoadWindowSettings(ImGuiWindow* window, ImGuiWindowSettings* settings) {
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    window->Pos = main_viewport->Pos + ImVec2(60, 60);
    window->Size = window->SizeFull = ImVec2(0, 0);
    window->SetWindowPosAllowFlags = window->SetWindowSizeAllowFlags = window->SetWindowCollapsedAllowFlags = ImGuiCond_Always | ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing;

    if (settings != NULL) {
        SetWindowConditionAllowFlag(window, ImGuiCond_FirstUseEver, false);
        ApplyWindowSettings(window, settings);
    }
    window->DC.CursorStartPos = window->DC.CursorMaxPos = window->DC.IdealMaxPos = window->Pos;

    if ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0) {
        window->AutoFitFramesX = window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
    else {
        if (window->Size.x <= 0.0f)
            window->AutoFitFramesX = 2;
        if (window->Size.y <= 0.0f)
            window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
    }
}

static void CalcWindowContentSizes(ImGuiWindow* window, ImVec2* content_size_current, ImVec2* content_size_ideal) {
    bool preserve_old_content_sizes = false;
    if (window->Collapsed && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
        preserve_old_content_sizes = true;
    else if (window->Hidden && window->HiddenFramesCannotSkipItems == 0 && window->HiddenFramesCanSkipItems > 0)
        preserve_old_content_sizes = true;
    if (preserve_old_content_sizes) {
        *content_size_current = window->ContentSize;
        *content_size_ideal = window->ContentSizeIdeal;
        return;
    }

    content_size_current->x = (window->ContentSizeExplicit.x != 0.0f) ? window->ContentSizeExplicit.x : ImTrunc(window->DC.CursorMaxPos.x - window->DC.CursorStartPos.x);
    content_size_current->y = (window->ContentSizeExplicit.y != 0.0f) ? window->ContentSizeExplicit.y : ImTrunc(window->DC.CursorMaxPos.y - window->DC.CursorStartPos.y);
    content_size_ideal->x = (window->ContentSizeExplicit.x != 0.0f) ? window->ContentSizeExplicit.x : ImTrunc(ImMax(window->DC.CursorMaxPos.x, window->DC.IdealMaxPos.x) - window->DC.CursorStartPos.x);
    content_size_ideal->y = (window->ContentSizeExplicit.y != 0.0f) ? window->ContentSizeExplicit.y : ImTrunc(ImMax(window->DC.CursorMaxPos.y, window->DC.IdealMaxPos.y) - window->DC.CursorStartPos.y);
}

static void SetCurrentWindow(ImGuiWindow* window) {
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    g.CurrentTable = window && window->DC.CurrentTableIdx != -1 ? g.Tables.GetByIndex(window->DC.CurrentTableIdx) : NULL;
    if (window) {
        g.FontSize = g.DrawListSharedData.FontSize = (window->FontRefSize != 0.0f ? window->FontRefSize : g.FontSizeBase * window->FontWindowScale * window->FontWindowScaleParents);
        ImGui::NavUpdateCurrentWindowIsScrollPushableX();
    }
}

static inline ImVec2 CalcWindowMinSize(ImGuiWindow* window) {
    ImGuiContext& g = *GImGui;
    ImVec2 size_min;
    if (window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_ChildWindow)) {
        size_min.x = (window->ChildFlags & ImGuiChildFlags_ResizeX) ? g.Style.WindowMinSize.x : 4.0f;
        size_min.y = (window->ChildFlags & ImGuiChildFlags_ResizeY) ? g.Style.WindowMinSize.y : 4.0f;
    }
    else {
        ImGuiWindow* window_for_height = window;
        size_min.x = ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) == 0) ? g.Style.WindowMinSize.x : 4.0f;
        size_min.y = ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) == 0) ? g.Style.WindowMinSize.y : 4.0f;
        size_min.y = ImMax(size_min.y, window_for_height->TitleBarHeight + window_for_height->MenuBarHeight + ImMax(0.0f, g.Style.WindowRounding - 1.0f));
    }
    return size_min;
}

static ImVec2 CalcWindowSizeAfterConstraint(ImGuiWindow* window, const ImVec2& size_desired) {
    ImGuiContext& g = *GImGui;
    ImVec2 new_size = size_desired;
    if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasSizeConstraint) {
        ImRect cr = g.NextWindowData.SizeConstraintRect;
        new_size.x = (cr.Min.x >= 0 && cr.Max.x >= 0) ? ImClamp(new_size.x, cr.Min.x, cr.Max.x) : window->SizeFull.x;
        new_size.y = (cr.Min.y >= 0 && cr.Max.y >= 0) ? ImClamp(new_size.y, cr.Min.y, cr.Max.y) : window->SizeFull.y;
        if (g.NextWindowData.SizeCallback) {
            ImGuiSizeCallbackData data;
            data.UserData = g.NextWindowData.SizeCallbackUserData;
            data.Pos = window->Pos;
            data.CurrentSize = window->SizeFull;
            data.DesiredSize = new_size;
            g.NextWindowData.SizeCallback(&data);
            new_size = data.DesiredSize;
        }
        new_size.x = ImTrunc(new_size.x);
        new_size.y = ImTrunc(new_size.y);
    }

    ImVec2 size_min = CalcWindowMinSize(window);
    return ImMax(new_size, size_min);
}

static ImVec2 CalcWindowAutoFitSize(ImGuiWindow* window, const ImVec2& size_contents) {
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    const float decoration_w_without_scrollbars = window->DecoOuterSizeX1 + window->DecoOuterSizeX2 - window->ScrollbarSizes.x;
    const float decoration_h_without_scrollbars = window->DecoOuterSizeY1 + window->DecoOuterSizeY2 - window->ScrollbarSizes.y;
    ImVec2 size_pad = window->WindowPadding * 2.0f;
    ImVec2 size_desired = size_contents + size_pad + ImVec2(decoration_w_without_scrollbars, decoration_h_without_scrollbars);
    if (window->Flags & ImGuiWindowFlags_Tooltip) {
        return size_desired;
    }
    else {
        ImVec2 size_min = CalcWindowMinSize(window);
        ImVec2 size_max = (window->Flags & ImGuiWindowFlags_ChildWindow) ? ImVec2(FLT_MAX, FLT_MAX) : ImGui::GetMainViewport()->WorkSize - style.DisplaySafeAreaPadding * 2.0f;
        ImVec2 size_auto_fit = ImClamp(size_desired, size_min, size_max);
        ImVec2 size_auto_fit_after_constraint = CalcWindowSizeAfterConstraint(window, size_auto_fit);

        bool will_have_scrollbar_x = (size_auto_fit_after_constraint.x - size_pad.x - decoration_w_without_scrollbars < size_contents.x && !(window->Flags & ImGuiWindowFlags_NoScrollbar) && (window->Flags & ImGuiWindowFlags_HorizontalScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        bool will_have_scrollbar_y = (size_auto_fit_after_constraint.y - size_pad.y - decoration_h_without_scrollbars < size_contents.y && !(window->Flags & ImGuiWindowFlags_NoScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysVerticalScrollbar);

        if (will_have_scrollbar_x)
            size_auto_fit.y += style.ScrollbarSize;
        if (will_have_scrollbar_y)
            size_auto_fit.x += style.ScrollbarSize;
        return size_auto_fit;
    }
}

static inline void ClampWindowPos(ImGuiWindow* window, const ImRect& visibility_rect) {
    ImGuiContext& g = *GImGui;
    ImVec2 size_for_clamping = window->Size;
    if (g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
        size_for_clamping.y = window->TitleBarHeight;

    window->Pos = ImClamp(window->Pos, visibility_rect.Min - size_for_clamping, visibility_rect.Max);
}

static const float WINDOWS_HOVER_PADDING = 4.0f;
static const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;

static void CalcResizePosSizeFromAnyCorner(ImGuiWindow* window, const ImVec2& corner_target, const ImVec2& corner_norm, ImVec2* out_pos, ImVec2* out_size) {
    ImVec2 pos_min = ImLerp(corner_target, window->Pos, corner_norm);
    ImVec2 pos_max = ImLerp(window->Pos + window->Size, corner_target, corner_norm);
    ImVec2 size_expected = pos_max - pos_min;
    ImVec2 size_constrained = CalcWindowSizeAfterConstraint(window, size_expected);
    *out_pos = pos_min;
    if (corner_norm.x == 0.0f)
        out_pos->x -= (size_constrained.x - size_expected.x);
    if (corner_norm.y == 0.0f)
        out_pos->y -= (size_constrained.y - size_expected.y);
    *out_size = size_constrained;
}

struct ImGuiResizeGripDef {
    ImVec2 corner_pos_n;
    ImVec2 inner_dir;
    int    angle_min12, angle_max12;
};

static const ImGuiResizeGripDef resize_grip_def[4] = {
    { ImVec2(1, 1), ImVec2(-1, -1), 0, 3 },
    { ImVec2(0, 1), ImVec2(+1, -1), 3, 6 },
    { ImVec2(0, 0), ImVec2(+1, +1), 6, 9 },
    { ImVec2(1, 0), ImVec2(-1, +1), 9, 12 }
};

struct ImGuiResizeBorderDef {
    ImVec2 inner_dir;
    ImVec2 segment_n1, segment_n2;
    float  outer_angle;
};

static const ImGuiResizeBorderDef resize_border_def[4] = {
    { ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f },
    { ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f },
    { ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f },
    { ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f }
};

static ImRect GetResizeBorderRect(ImGuiWindow* window, int border_n, float perp_padding, float thickness) {
    ImRect rect = window->Rect();
    if (thickness == 0.0f)
        rect.Max -= ImVec2(1, 1);

    if (border_n == ImGuiDir_Left) { return ImRect(rect.Min.x - thickness, rect.Min.y + perp_padding, rect.Min.x + thickness, rect.Max.y - perp_padding); }
    if (border_n == ImGuiDir_Right) { return ImRect(rect.Max.x - thickness, rect.Min.y + perp_padding, rect.Max.x + thickness, rect.Max.y - perp_padding); }
    if (border_n == ImGuiDir_Up) { return ImRect(rect.Min.x + perp_padding, rect.Min.y - thickness, rect.Max.x - perp_padding, rect.Min.y + thickness); }
    if (border_n == ImGuiDir_Down) { return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness, rect.Max.x - perp_padding, rect.Max.y + thickness); }
    IM_ASSERT(0);
    return ImRect();
}

static int UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_hovered, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect) {
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;

    if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        return false;
    if (window->WasActive == false)
        return false;

    int ret_auto_fit_mask = 0x00;
    const float grip_draw_size = ImTrunc(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
    const float grip_hover_inner_size = ImTrunc(grip_draw_size * 0.75f);
    const float grip_hover_outer_size = g.IO.ConfigWindowsResizeFromEdges ? WINDOWS_HOVER_PADDING : 0.0f;

    ImRect clamp_rect = visibility_rect;
    const bool window_move_from_title_bar = g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar);
    if (window_move_from_title_bar)
        clamp_rect.Min.y -= window->TitleBarHeight;

    ImVec2 pos_target(FLT_MAX, FLT_MAX);
    ImVec2 size_target(FLT_MAX, FLT_MAX);

    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    PushID("#RESIZE");
    for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++) {
        const ImGuiResizeGripDef& def = resize_grip_def[resize_grip_n];
        const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, def.corner_pos_n);

        bool hovered, held;
        ImRect resize_rect(corner - def.inner_dir * grip_hover_outer_size, corner + def.inner_dir * grip_hover_inner_size);
        if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
        if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
        ImGuiID resize_grip_id = window->GetID(resize_grip_n);
        ItemAdd(resize_rect, resize_grip_id, NULL, ImGuiItemFlags_NoNav);
        ButtonBehavior(resize_rect, resize_grip_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        if (hovered || held)
            g.MouseCursor = (resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;

        if (held && g.IO.MouseDoubleClicked[0]) {
            size_target = CalcWindowSizeAfterConstraint(window, size_auto_fit);
            ret_auto_fit_mask = 0x03;
            ClearActiveID();
        }
        else if (held) {
            ImVec2 clamp_min = ImVec2(def.corner_pos_n.x == 1.0f ? clamp_rect.Min.x : -FLT_MAX, (def.corner_pos_n.y == 1.0f || (def.corner_pos_n.y == 0.0f && window_move_from_title_bar)) ? clamp_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max = ImVec2(def.corner_pos_n.x == 0.0f ? clamp_rect.Max.x : +FLT_MAX, def.corner_pos_n.y == 0.0f ? clamp_rect.Max.y : +FLT_MAX);
            ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + ImLerp(def.inner_dir * grip_hover_outer_size, def.inner_dir * -grip_hover_inner_size, def.corner_pos_n);
            CalcResizePosSizeFromAnyCorner(window, corner_target, def.corner_pos_n, &pos_target, &size_target);
        }

        if (resize_grip_n == 0 || held || hovered)
            resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
    }

    int resize_border_mask = 0x00;
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        resize_border_mask |= ((window->ChildFlags & ImGuiChildFlags_ResizeX) ? 0x02 : 0) | ((window->ChildFlags & ImGuiChildFlags_ResizeY) ? 0x08 : 0);
    else
        resize_border_mask = g.IO.ConfigWindowsResizeFromEdges ? 0x0F : 0x00;
    for (int border_n = 0; border_n < 4; border_n++) {
        if ((resize_border_mask & (1 << border_n)) == 0)
            continue;
        const ImGuiResizeBorderDef& def = resize_border_def[border_n];
        const ImGuiAxis axis = (border_n == ImGuiDir_Left || border_n == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;

        bool hovered, held;
        ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_inner_size, WINDOWS_HOVER_PADDING);
        ImGuiID border_id = window->GetID(border_n + 4);
        ItemAdd(border_rect, border_id, NULL, ImGuiItemFlags_NoNav);
        ButtonBehavior(border_rect, border_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        if (hovered && g.HoveredIdTimer <= WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER)
            hovered = false;
        if (hovered || held)
            g.MouseCursor = (axis == ImGuiAxis_X) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;
        if (held && g.IO.MouseDoubleClicked[0]) {
            if (border_n == 1 || border_n == 3) {
                size_target[axis] = CalcWindowSizeAfterConstraint(window, size_auto_fit)[axis];
                ret_auto_fit_mask |= (1 << axis);
                hovered = held = false;
            }
            ClearActiveID();
        }
        else if (held) {
            const bool just_scrolled_manually_while_resizing = (g.WheelingWindow != NULL && g.WheelingWindowScrolledFrame == g.FrameCount && IsWindowChildOf(window, g.WheelingWindow, false));
            if (g.ActiveIdIsJustActivated || just_scrolled_manually_while_resizing) {
                g.WindowResizeBorderExpectedRect = border_rect;
                g.WindowResizeRelativeMode = false;
            } if ((window->Flags & ImGuiWindowFlags_ChildWindow) && memcmp(&g.WindowResizeBorderExpectedRect, &border_rect, sizeof(ImRect)) != 0)
                g.WindowResizeRelativeMode = true;

            const ImVec2 border_curr = (window->Pos + ImMin(def.segment_n1, def.segment_n2) * window->Size);
            const float border_target_rel_mode_for_axis = border_curr[axis] + g.IO.MouseDelta[axis];
            const float border_target_abs_mode_for_axis = g.IO.MousePos[axis] - g.ActiveIdClickOffset[axis] + WINDOWS_HOVER_PADDING;

            ImVec2 border_target = window->Pos;
            border_target[axis] = border_target_abs_mode_for_axis;

            bool ignore_resize = false;
            if (g.WindowResizeRelativeMode) {
                border_target[axis] = border_target_rel_mode_for_axis;
                if (g.IO.MouseDelta[axis] == 0.0f || (g.IO.MouseDelta[axis] > 0.0f) == (border_target_rel_mode_for_axis > border_target_abs_mode_for_axis))
                    ignore_resize = true;
            }

            ImVec2 clamp_min(border_n == ImGuiDir_Right ? clamp_rect.Min.x : -FLT_MAX, border_n == ImGuiDir_Down || (border_n == ImGuiDir_Up && window_move_from_title_bar) ? clamp_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max(border_n == ImGuiDir_Left ? clamp_rect.Max.x : +FLT_MAX, border_n == ImGuiDir_Up ? clamp_rect.Max.y : +FLT_MAX);
            border_target = ImClamp(border_target, clamp_min, clamp_max);
            if (flags & ImGuiWindowFlags_ChildWindow) {
                if ((flags & (ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar)) == 0 || (flags & ImGuiWindowFlags_NoScrollbar))
                    border_target.x = ImClamp(border_target.x, window->ParentWindow->InnerClipRect.Min.x, window->ParentWindow->InnerClipRect.Max.x);
                if (flags & ImGuiWindowFlags_NoScrollbar)
                    border_target.y = ImClamp(border_target.y, window->ParentWindow->InnerClipRect.Min.y, window->ParentWindow->InnerClipRect.Max.y);
            }
            if (!ignore_resize)
                CalcResizePosSizeFromAnyCorner(window, border_target, ImMin(def.segment_n1, def.segment_n2), &pos_target, &size_target);
        }

        if (hovered)
            *border_hovered = border_n;

        if (held)
            *border_held = border_n;
    }
    PopID();

    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;

    if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window) {
        ImVec2 nav_resize_dir;
        if (g.NavInputSource == ImGuiInputSource_Keyboard && g.IO.KeyShift)
            nav_resize_dir = GetKeyMagnitude2d(ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow);
        if (g.NavInputSource == ImGuiInputSource_Gamepad)
            nav_resize_dir = GetKeyMagnitude2d(ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadDpadRight, ImGuiKey_GamepadDpadUp, ImGuiKey_GamepadDpadDown);
        if (nav_resize_dir.x != 0.0f || nav_resize_dir.y != 0.0f) {
            const float NAV_RESIZE_SPEED = 600.0f;
            const float resize_step = NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y);
            g.NavWindowingAccumDeltaSize += nav_resize_dir * resize_step;
            g.NavWindowingAccumDeltaSize = ImMax(g.NavWindowingAccumDeltaSize, clamp_rect.Min - window->Pos - window->Size);
            g.NavWindowingToggleLayer = false;
            g.NavHighlightItemUnderNav = true;
            resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
            ImVec2 accum_floored = ImTrunc(g.NavWindowingAccumDeltaSize);
            if (accum_floored.x != 0.0f || accum_floored.y != 0.0f) {
                size_target = CalcWindowSizeAfterConstraint(window, window->SizeFull + accum_floored);
                g.NavWindowingAccumDeltaSize -= accum_floored;
            }
        }
    }

    const ImVec2 curr_pos = window->Pos;
    const ImVec2 curr_size = window->SizeFull;
    if (size_target.x != FLT_MAX && (window->Size.x != size_target.x || window->SizeFull.x != size_target.x))
        window->Size.x = window->SizeFull.x = size_target.x;
    if (size_target.y != FLT_MAX && (window->Size.y != size_target.y || window->SizeFull.y != size_target.y))
        window->Size.y = window->SizeFull.y = size_target.y;
    if (pos_target.x != FLT_MAX && window->Pos.x != ImTrunc(pos_target.x))
        window->Pos.x = ImTrunc(pos_target.x);
    if (pos_target.y != FLT_MAX && window->Pos.y != ImTrunc(pos_target.y))
        window->Pos.y = ImTrunc(pos_target.y);
    if (curr_pos.x != window->Pos.x || curr_pos.y != window->Pos.y || curr_size.x != window->SizeFull.x || curr_size.y != window->SizeFull.y)
        MarkIniSettingsDirty(window);

    if (*border_held != -1)
        g.WindowResizeBorderExpectedRect = GetResizeBorderRect(window, *border_held, grip_hover_inner_size, WINDOWS_HOVER_PADDING);

    return ret_auto_fit_mask;
}

static float CalcScrollEdgeSnap(float target, float snap_min, float snap_max, float snap_threshold, float center_ratio) {
    if (target <= snap_min + snap_threshold)
        return ImLerp(snap_min, target, center_ratio);
    if (target >= snap_max - snap_threshold)
        return ImLerp(target, snap_max, center_ratio);
    return target;
}

constexpr float SCROLL_CLAMP_THRESHOLD = 20.0f;

static ImVec2 CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window, bool clamp) {
    ImVec2 scroll = window->Scroll;
    if (window->ScrollTarget.x < FLT_MAX) {
        float decoration_total_width = window->ScrollbarSizes.x;
        float center_x_ratio = window->ScrollTargetCenterRatio.x;
        float scroll_target_x = window->ScrollTarget.x;
        if (window->ScrollTargetEdgeSnapDist.x > 0.0f) {
            float snap_x_min = 0.0f;
            float snap_x_max = window->ScrollMax.x + window->SizeFull.x - decoration_total_width;
            scroll_target_x = CalcScrollEdgeSnap(scroll_target_x, snap_x_min, snap_x_max, window->ScrollTargetEdgeSnapDist.x, center_x_ratio);
        }
        scroll.x = scroll_target_x - center_x_ratio * (window->SizeFull.x - decoration_total_width);
    } if (window->ScrollTarget.y < FLT_MAX) {
        float decoration_total_height = window->TitleBarHeight + window->MenuBarHeight + window->ScrollbarSizes.y;
        float center_y_ratio = window->ScrollTargetCenterRatio.y;
        float scroll_target_y = window->ScrollTarget.y;
        if (window->ScrollTargetEdgeSnapDist.y > 0.0f) {
            float snap_y_min = 0.0f;
            float snap_y_max = window->ScrollMax.y + window->SizeFull.y - decoration_total_height;
            scroll_target_y = CalcScrollEdgeSnap(scroll_target_y, snap_y_min, snap_y_max, window->ScrollTargetEdgeSnapDist.y, center_y_ratio);
        }
        scroll.y = scroll_target_y - center_y_ratio * (window->SizeFull.y - decoration_total_height);
    }
    scroll.x = ImFloor(ImMax(scroll.x, 0.0f));
    scroll.y = ImFloor(ImMax(scroll.y, 0.0f));
    if (!window->Collapsed && !window->SkipItems) {
        scroll.x = ImMin(scroll.x, window->ScrollMax.x);
        if (!clamp)
            scroll.y = ImMin(scroll.y - SCROLL_CLAMP_THRESHOLD, window->ScrollMax.y + SCROLL_CLAMP_THRESHOLD);
        else
            scroll.y = ImMin(scroll.y, window->ScrollMax.y);
    }
    return scroll;
}

static ImGuiWindow* CreateNewWindow(const char* name, ImGuiWindowFlags flags) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
    window->Flags = flags;
    g.WindowsById.SetVoidPtr(window->ID, window);

    ImGuiWindowSettings* settings = NULL;
    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
        if ((settings = ImGui::FindWindowSettingsByWindow(window)) != 0)
            window->SettingsOffset = g.SettingsWindows.offset_from_ptr(settings);

    InitOrLoadWindowSettings(window, settings);

    if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
        g.Windows.push_front(window);
    else
        g.Windows.push_back(window);

    return window;
}

static ImGuiCol GetWindowBgColorIdx(ImGuiWindow* window) {
    if (window->Flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        return ImGuiCol_PopupBg;
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        return ImGuiCol_ChildBg;
    return ImGuiCol_WindowBg;
}

void RenderWindowShadow(ImGuiWindow* window) {
    float shadow_size = 12.0f;
    ImU32 shadow_col = IM_COL32(0, 0, 0, 110);
    ImVec2 shadow_offset = ImVec2(0.0f, 2.0f);
    Draw->ShadowRect(window->DrawList, window->Pos, window->Pos + window->Size, shadow_col, shadow_size, shadow_offset, ImDrawFlags_ShadowCutOutShapeBackground, window->WindowRounding);
}

static void RenderWindowOuterSingleBorder(ImGuiWindow* window, int border_n, ImU32 border_col, float border_size) {
    const ImGuiResizeBorderDef& def = resize_border_def[border_n];
    const float rounding = window->WindowRounding;
    const ImRect border_r = GetResizeBorderRect(window, border_n, rounding, 0.0f);
    window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.segment_n1) + ImVec2(0.5f, 0.5f) + def.inner_dir * rounding, rounding, def.outer_angle - IM_PI * 0.25f, def.outer_angle);
    window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.segment_n2) + ImVec2(0.5f, 0.5f) + def.inner_dir * rounding, rounding, def.outer_angle, def.outer_angle + IM_PI * 0.25f);
    window->DrawList->PathStroke(border_col, ImDrawFlags_None, border_size);
}

static void RenderWindowOuterBorders(ImGuiWindow* window) {
    ImGuiContext& g = *GImGui;
    const float border_size = window->WindowBorderSize;
    const ImU32 border_col = GetColorU32(ImGuiCol_Border);
    if (border_size > 0.0f && (window->Flags & ImGuiWindowFlags_NoBackground) == 0)
        window->DrawList->AddRect(window->Pos, window->Pos + window->Size, border_col, window->WindowRounding, 0, window->WindowBorderSize);
    else if (border_size > 0.0f) {
        if (window->ChildFlags & ImGuiChildFlags_ResizeX)
            RenderWindowOuterSingleBorder(window, 1, border_col, border_size);
        if (window->ChildFlags & ImGuiChildFlags_ResizeY)
            RenderWindowOuterSingleBorder(window, 3, border_col, border_size);
    } if (window->ResizeBorderHovered != -1 || window->ResizeBorderHeld != -1) {
        const int border_n = (window->ResizeBorderHeld != -1) ? window->ResizeBorderHeld : window->ResizeBorderHovered;
        const ImU32 border_col_resizing = GetColorU32((window->ResizeBorderHeld != -1) ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered);
        RenderWindowOuterSingleBorder(window, border_n, border_col_resizing, ImMax(2.0f, window->WindowBorderSize));
    } if (g.Style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar)) {
        float y = window->Pos.y + window->TitleBarHeight - 1;
        window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), border_col, g.Style.FrameBorderSize);
    }
}

bool ScrollbarEx(const ImRect& bb_frame, ImGuiID id, ImGuiAxis axis, ImS64* p_scroll_v, ImS64 size_avail_v, ImS64 size_contents_v, ImDrawFlags flags) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    const float bb_frame_width = bb_frame.GetWidth();
    const float bb_frame_height = bb_frame.GetHeight();
    if (bb_frame_width <= 0.0f || bb_frame_height <= 0.0f)
        return false;

    float alpha = 1.0f;
    if ((axis == ImGuiAxis_Y) && bb_frame_height < g.FontSize + g.Style.FramePadding.y * 2.0f)
        alpha = ImSaturate((bb_frame_height - g.FontSize) / (g.Style.FramePadding.y * 2.0f));
    if (alpha <= 0.0f)
        return false;

    const ImGuiStyle& style = g.Style;
    const bool allow_interaction = (alpha >= 1.0f);

    ImRect bb = bb_frame;
    bb.Expand(ImVec2(-ImClamp(ImTrunc((bb_frame_width - 2.0f) * 0.5f), 0.0f, 3.0f), -ImClamp(ImTrunc((bb_frame_height - 2.0f) * 0.5f), 0.0f, 3.0f)));

    const float scrollbar_size_v = (axis == ImGuiAxis_X) ? bb.GetWidth() : bb.GetHeight();

    IM_ASSERT(ImMax(size_contents_v, size_avail_v) > 0.0f);
    const ImS64 win_size_v = ImMax(ImMax(size_contents_v, size_avail_v), (ImS64)1);
    const float grab_h_pixels = ImClamp(scrollbar_size_v * ((float)size_avail_v / (float)win_size_v), style.GrabMinSize, scrollbar_size_v);
    const float grab_h_norm = grab_h_pixels / scrollbar_size_v;

    bool held = false;
    bool hovered = false;
    ItemAdd(bb_frame, id, NULL, ImGuiItemFlags_NoNav);
    ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

    const ImS64 scroll_max = ImMax((ImS64)1, size_contents_v - size_avail_v);
    float scroll_ratio = ImSaturate((float)*p_scroll_v / (float)scroll_max);
    float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;
    if (held && allow_interaction && grab_h_norm < 1.0f) {
        const float scrollbar_pos_v = bb.Min[axis];
        const float mouse_pos_v = g.IO.MousePos[axis];

        const float clicked_v_norm = ImSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);

        bool seek_absolute = false;
        if (g.ActiveIdIsJustActivated) {
            seek_absolute = (clicked_v_norm < grab_v_norm || clicked_v_norm > grab_v_norm + grab_h_norm);
            if (seek_absolute)
                g.ScrollbarClickDeltaToGrabCenter = 0.0f;
            else
                g.ScrollbarClickDeltaToGrabCenter = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
        }

        const float scroll_v_norm = ImSaturate((clicked_v_norm - g.ScrollbarClickDeltaToGrabCenter - grab_h_norm * 0.5f) / (1.0f - grab_h_norm));
        *p_scroll_v = (ImS64)(scroll_v_norm * scroll_max);

        scroll_ratio = ImSaturate((float)*p_scroll_v / (float)scroll_max);
        grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;

        if (seek_absolute)
            g.ScrollbarClickDeltaToGrabCenter = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
    }

    const ImU32 bg_col = GetColorU32(ImGuiCol_ScrollbarBg);
    const ImU32 grab_col = GetColorU32(held ? ImGuiCol_ScrollbarGrabActive : hovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab, alpha);
    window->DrawList->AddRectFilled(bb_frame.Min, bb_frame.Max, bg_col, window->WindowRounding, flags);
    ImRect grab_rect;
    if (axis == ImGuiAxis_X)
        grab_rect = ImRect(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y, ImLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, bb.Max.y);
    else
        grab_rect = ImRect(bb.Min.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm), bb.Max.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels);
    window->DrawList->AddRectFilled(grab_rect.Min, grab_rect.Max, grab_col, style.ScrollbarRounding);

    return held;
}

void Scrollbar(ImGuiAxis axis) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiID id = GetWindowScrollbarID(window, axis);

    ImRect bb = GetWindowScrollbarRect(window, axis);
    ImDrawFlags rounding_corners = ImDrawFlags_RoundCornersNone;
    if (axis == ImGuiAxis_X) {
        rounding_corners |= ImDrawFlags_RoundCornersBottomLeft;
        if (!window->ScrollbarY)
            rounding_corners |= ImDrawFlags_RoundCornersBottomRight;
    }
    else {
        if ((window->Flags & ImGuiWindowFlags_NoTitleBar) && !(window->Flags & ImGuiWindowFlags_MenuBar))
            rounding_corners |= ImDrawFlags_RoundCornersTopRight;
        if (!window->ScrollbarX)
            rounding_corners |= ImDrawFlags_RoundCornersBottomRight;
    }
    float size_avail = window->InnerRect.Max[axis] - window->InnerRect.Min[axis];
    float size_contents = window->ContentSize[axis] + window->WindowPadding[axis] * 2.0f;
    ImS64 scroll = (ImS64)window->Scroll[axis];
    ::ScrollbarEx(bb, id, axis, &scroll, (ImS64)size_avail, (ImS64)size_contents, rounding_corners);
    window->Scroll[axis] = (float)scroll;
}

void RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, bool handle_borders_and_resize_grips, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size) {
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    IM_ASSERT(window->BeginCount == 0);
    window->SkipItems = false;

    const float window_rounding = window->WindowRounding;
    const float window_border_size = window->WindowBorderSize;
    if (window->Collapsed) {
        const float backup_border_size = style.FrameBorderSize;
        g.Style.FrameBorderSize = window->WindowBorderSize;
        ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && g.NavCursorVisible) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
        RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
        g.Style.FrameBorderSize = backup_border_size;
    }
    else {
        if (!(flags & ImGuiWindowFlags_NoBackground))
        {
            ImU32 bg_col = GetColorU32(GetWindowBgColorIdx(window));
            bool override_alpha = false;
            float alpha = 1.0f;
            if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasBgAlpha) {
                alpha = g.NextWindowData.BgAlphaVal;
                override_alpha = true;
            }
            if (override_alpha)
                bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
            window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? 0 : ImDrawFlags_RoundCornersBottom);
        }

        if ((!(flags & ImGuiWindowFlags_ChildWindow) || (flags & ImGuiWindowFlags_Popup)) && !(flags & ImGuiWindowFlags_NoBackground) && Var->window.shadow_size > 0.0f)
            RenderWindowShadow(window);

        if (!(flags & ImGuiWindowFlags_NoTitleBar)) {
            ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
            window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawFlags_RoundCornersTop);
        }

        if (flags & ImGuiWindowFlags_MenuBar) {
            ImRect menu_bar_rect = window->MenuBarRect();
            menu_bar_rect.ClipWith(window->Rect());
            window->DrawList->AddRectFilled(menu_bar_rect.Min + ImVec2(window_border_size, 0), menu_bar_rect.Max - ImVec2(window_border_size, 0), GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawFlags_RoundCornersTop);
            if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
                window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
        }

        if (window->ScrollbarX)
            ::Scrollbar(ImGuiAxis_X);
        if (window->ScrollbarY)
            ::Scrollbar(ImGuiAxis_Y);

        if (handle_borders_and_resize_grips && !(flags & ImGuiWindowFlags_NoResize)) {
            for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++) {
                const ImU32 col = resize_grip_col[resize_grip_n];
                if ((col & IM_COL32_A_MASK) == 0)
                    continue;
                const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
                const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.corner_pos_n);
                window->DrawList->PathLineTo(corner + grip.inner_dir * ((resize_grip_n & 1) ? ImVec2(window_border_size, resize_grip_draw_size) : ImVec2(resize_grip_draw_size, window_border_size)));
                window->DrawList->PathLineTo(corner + grip.inner_dir * ((resize_grip_n & 1) ? ImVec2(resize_grip_draw_size, window_border_size) : ImVec2(window_border_size, resize_grip_draw_size)));
                window->DrawList->PathArcToFast(ImVec2(corner.x + grip.inner_dir.x * (window_rounding + window_border_size), corner.y + grip.inner_dir.y * (window_rounding + window_border_size)), window_rounding, grip.angle_min12, grip.angle_max12);
                window->DrawList->PathFillConvex(col);
            }
        }

        if (handle_borders_and_resize_grips)
            RenderWindowOuterBorders(window);
    }
}

void RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open) {
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    const bool has_close_button = (p_open != NULL);
    const bool has_collapse_button = !(flags & ImGuiWindowFlags_NoCollapse) && (style.WindowMenuButtonPosition != ImGuiDir_None);

    const ImGuiItemFlags item_flags_backup = g.CurrentItemFlags;
    g.CurrentItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    float pad_l = style.FramePadding.x;
    float pad_r = style.FramePadding.x;
    float button_sz = g.FontSize;
    ImVec2 close_button_pos;
    ImVec2 collapse_button_pos;
    if (has_close_button) {
        close_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - button_sz, title_bar_rect.Min.y + style.FramePadding.y);
        pad_r += button_sz + style.ItemInnerSpacing.x;
    } if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Right) {
        collapse_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - button_sz, title_bar_rect.Min.y + style.FramePadding.y);
        pad_r += button_sz + style.ItemInnerSpacing.x;
    } if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Left) {
        collapse_button_pos = ImVec2(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y + style.FramePadding.y);
        pad_l += button_sz + style.ItemInnerSpacing.x;
    }

    if (has_collapse_button)
        if (CollapseButton(window->GetID("#COLLAPSE"), collapse_button_pos))
            window->WantCollapseToggle = true;

    if (has_close_button)
        if (CloseButton(window->GetID("#CLOSE"), close_button_pos))
            *p_open = false;

    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    g.CurrentItemFlags = item_flags_backup;

    const float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? button_sz * 0.80f : 0.0f;
    const ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);

    if (pad_l > style.FramePadding.x)
        pad_l += g.Style.ItemInnerSpacing.x;
    if (pad_r > style.FramePadding.x)
        pad_r += g.Style.ItemInnerSpacing.x;
    if (style.WindowTitleAlign.x > 0.0f && style.WindowTitleAlign.x < 1.0f) {
        float centerness = ImSaturate(1.0f - ImFabs(style.WindowTitleAlign.x - 0.5f) * 2.0f);
        float pad_extend = ImMin(ImMax(pad_l, pad_r), title_bar_rect.GetWidth() - pad_l - pad_r - text_size.x);
        pad_l = ImMax(pad_l, pad_extend * centerness);
        pad_r = ImMax(pad_r, pad_extend * centerness);
    }

    ImRect layout_r(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y, title_bar_rect.Max.x - pad_r, title_bar_rect.Max.y);
    ImRect clip_r(layout_r.Min.x, layout_r.Min.y, ImMin(layout_r.Max.x + g.Style.ItemInnerSpacing.x, title_bar_rect.Max.x), layout_r.Max.y);
    if (flags & ImGuiWindowFlags_UnsavedDocument) {
        ImVec2 marker_pos;
        marker_pos.x = ImClamp(layout_r.Min.x + (layout_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x + text_size.x, layout_r.Min.x, layout_r.Max.x);
        marker_pos.y = (layout_r.Min.y + layout_r.Max.y) * 0.5f;
        if (marker_pos.x > layout_r.Min.x)
        {
            RenderBullet(window->DrawList, marker_pos, GetColorU32(ImGuiCol_Text));
            clip_r.Max.x = ImMin(clip_r.Max.x, marker_pos.x - (int)(marker_size_x * 0.5f));
        }
    }

    RenderTextClipped(layout_r.Min, layout_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_r);
}

bool CGui::Begin(std::string_view name, bool* p_open, ImGuiWindowFlags flags) {
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(name.data() != NULL && name.data()[0] != '\0');
    IM_ASSERT(g.WithinFrameScope);
    IM_ASSERT(g.FrameCountEnded != g.FrameCount);

    ImGuiWindow* window = FindWindowByName(name.data());
    const bool window_just_created = (window == NULL);
    if (window_just_created)
        window = CreateNewWindow(name.data(), flags);

    if (g.DebugBreakInWindow == window->ID)
        IM_DEBUG_BREAK();

    if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    const int current_frame = g.FrameCount;
    const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);
    window->IsFallbackWindow = (g.CurrentWindowStack.Size == 0 && g.WithinFrameScopeWithImplicitWindow);

    bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);
    if (flags & ImGuiWindowFlags_Popup) {
        ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId);
        window_just_activated_by_user |= (window != popup_ref.Window);
    }
    window->Appearing = window_just_activated_by_user;
    if (window->Appearing)
        SetWindowConditionAllowFlag(window, ImGuiCond_Appearing, true);

    if (first_begin_of_the_frame) {
        UpdateWindowInFocusOrderList(window, window_just_created, flags);
        window->Flags = (ImGuiWindowFlags)flags;
        if (!(flags & ImGuiWindowFlags_ChildWindow))
            window->ChildFlags = 0;
        else
            window->ChildFlags = GetNextWindowChildFlags(g.NextWindowData);
        window->LastFrameActive = current_frame;
        window->LastTimeActive = (float)g.Time;
        window->BeginOrderWithinParent = 0;
        window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
    }
    else {
        flags = window->Flags;
    }

    ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back().Window;
    ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
    IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));

    if (window->IDStack.Size == 0)
        window->IDStack.push_back(window->ID);

    g.CurrentWindow = window;
    ImGuiWindowStackData window_stack_data;
    memset(&window_stack_data, 0, sizeof(window_stack_data));
    window_stack_data.Window = window;
    window_stack_data.ParentLastItemDataBackup = g.LastItemData;
    g.CurrentWindowStack.push_back(window_stack_data);

    StoreWindowRecoveryState(g.CurrentWindowStack.back(), &g);
    g.StackSizesInBeginForCurrentWindow = &g.CurrentWindowStack.back().StackSizesInBegin;

    if (flags & ImGuiWindowFlags_ChildMenu)
        g.BeginMenuDepth++;

    if (first_begin_of_the_frame) {
        UpdateWindowParentAndRootLinks(window, flags, parent_window);
        window->ParentWindowInBeginStack = parent_window_in_stack;
        window->ParentWindowForFocusRoute = (flags & ImGuiWindowFlags_ChildWindow) ? parent_window_in_stack : NULL;
    }

    PushFocusScope((window->ChildFlags & ImGuiChildFlags_NavFlattened) ? g.CurrentFocusScopeId : window->ID);
    window->NavRootFocusScopeId = g.CurrentFocusScopeId;

    if (flags & ImGuiWindowFlags_Popup) {
        ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        popup_ref.Window = window;
        popup_ref.ParentNavLayer = parent_window_in_stack->DC.NavLayerCurrent;
        g.BeginPopupStack.push_back(popup_ref);
        window->PopupId = popup_ref.PopupId;
    }

    bool window_pos_set_by_api = false;
    bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
    if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasPos) {
        window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
        if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f) {
            window->SetWindowPosVal = g.NextWindowData.PosVal;
            window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
            window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
        }
        else {
            SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
        }
    } if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasSize) {
        window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
        window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
        if ((window->ChildFlags & ImGuiChildFlags_ResizeX) && (window->SetWindowSizeAllowFlags & ImGuiCond_FirstUseEver) == 0)
            g.NextWindowData.SizeVal.x = window->SizeFull.x;
        if ((window->ChildFlags & ImGuiChildFlags_ResizeY) && (window->SetWindowSizeAllowFlags & ImGuiCond_FirstUseEver) == 0)
            g.NextWindowData.SizeVal.y = window->SizeFull.y;
        SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
    } if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasScroll) {
        if (g.NextWindowData.ScrollVal.x >= 0.0f) {
            window->ScrollTarget.x = g.NextWindowData.ScrollVal.x;
            window->ScrollTargetCenterRatio.x = 0.0f;
        } if (g.NextWindowData.ScrollVal.y >= 0.0f) {
            window->ScrollTarget.y = g.NextWindowData.ScrollVal.y;
            window->ScrollTargetCenterRatio.y = 0.0f;
        }
    }
    if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasContentSize)
        window->ContentSizeExplicit = g.NextWindowData.ContentSizeVal;
    else if (first_begin_of_the_frame)
        window->ContentSizeExplicit = ImVec2(0.0f, 0.0f);
    if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasCollapsed)
        SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
    if (GetNextWindowDataFlags(g.NextWindowData) & ImGuiNextWindowDataFlags_HasFocus)
        FocusWindow(window);
    if (window->Appearing)
        SetWindowConditionAllowFlag(window, ImGuiCond_Appearing, false);

    g.CurrentWindow = NULL;

    if (first_begin_of_the_frame) {
        const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip);
        const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesCannotSkipItems > 0);
        window->Active = true;
        window->HasCloseButton = (p_open != NULL);
        window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
        window->IDStack.resize(1);
        window->DrawList->_ResetForNewFrame();
        window->DC.CurrentTableIdx = -1;

        if (window->MemoryCompacted)
            GcAwakeTransientWindowBuffers(window);

        bool window_title_visible_elsewhere = false;
        if (g.NavWindowingListWindow != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)
            window_title_visible_elsewhere = true;
        if (window_title_visible_elsewhere && !window_just_created && strcmp(name.data(), window->Name) != 0) {
            size_t buf_len = (size_t)window->NameBufLen;
            window->Name = ImStrdupcpy(window->Name, &buf_len, name.data());
            window->NameBufLen = (int)buf_len;
        }

        CalcWindowContentSizes(window, &window->ContentSize, &window->ContentSizeIdeal);
        if (window->HiddenFramesCanSkipItems > 0)
            window->HiddenFramesCanSkipItems--;
        if (window->HiddenFramesCannotSkipItems > 0)
            window->HiddenFramesCannotSkipItems--;
        if (window->HiddenFramesForRenderOnly > 0)
            window->HiddenFramesForRenderOnly--;

        if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
            window->HiddenFramesCannotSkipItems = 1;

        if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0) {
            window->HiddenFramesCannotSkipItems = 1;
            if (flags & ImGuiWindowFlags_AlwaysAutoResize) {
                if (!window_size_x_set_by_api)
                    window->Size.x = window->SizeFull.x = 0.f;
                if (!window_size_y_set_by_api)
                    window->Size.y = window->SizeFull.y = 0.f;
                window->ContentSize = window->ContentSizeIdeal = ImVec2(0.f, 0.f);
            }
        }

        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)GetMainViewport();
        SetWindowViewport(window, viewport);
        SetCurrentWindow(window);

        if (flags & ImGuiWindowFlags_ChildWindow)
            window->WindowBorderSize = style.ChildBorderSize;
        else
            window->WindowBorderSize = ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
        window->WindowPadding = style.WindowPadding;
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !(window->ChildFlags & ImGuiChildFlags_AlwaysUseWindowPadding) && window->WindowBorderSize == 0.0f)
            window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);

        window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
        window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

        bool use_current_size_for_scrollbar_x = window_just_created;
        bool use_current_size_for_scrollbar_y = window_just_created;

        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse)) {
            ImRect title_bar_rect = window->TitleBarRect();
            if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseClickedCount[0] == 2)
                window->WantCollapseToggle = true;
            if (window->WantCollapseToggle) {
                window->Collapsed = !window->Collapsed;
                if (!window->Collapsed)
                    use_current_size_for_scrollbar_y = true;
                MarkIniSettingsDirty(window);
            }
        }
        else {
            window->Collapsed = false;
        }
        window->WantCollapseToggle = false;

        const ImVec2 scrollbar_sizes_from_last_frame = window->ScrollbarSizes;
        window->DecoOuterSizeX1 = 0.0f;
        window->DecoOuterSizeX2 = 0.0f;
        window->DecoOuterSizeY1 = window->TitleBarHeight + window->MenuBarHeight;
        window->DecoOuterSizeY2 = 0.0f;
        window->ScrollbarSizes = ImVec2(0.0f, 0.0f);

        const ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, window->ContentSizeIdeal);
        if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed) {
            if (!window_size_x_set_by_api) {
                window->SizeFull.x = size_auto_fit.x;
                use_current_size_for_scrollbar_x = true;
            } if (!window_size_y_set_by_api) {
                window->SizeFull.y = size_auto_fit.y;
                use_current_size_for_scrollbar_y = true;
            }
        }
        else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0) {
            if (!window_size_x_set_by_api && window->AutoFitFramesX > 0) {
                window->SizeFull.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
                use_current_size_for_scrollbar_x = true;
            } if (!window_size_y_set_by_api && window->AutoFitFramesY > 0) {
                window->SizeFull.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
                use_current_size_for_scrollbar_y = true;
            } if (!window->Collapsed)
                MarkIniSettingsDirty(window);
        }

        window->SizeFull = CalcWindowSizeAfterConstraint(window, window->SizeFull);
        window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

        if (window_just_activated_by_user) {
            window->AutoPosLastDirection = ImGuiDir_None;
            if ((flags & ImGuiWindowFlags_Popup) != 0 && !(flags & ImGuiWindowFlags_Modal) && !window_pos_set_by_api)
                window->Pos = g.BeginPopupStack.back().OpenPopupPos;
        }

        if (flags & ImGuiWindowFlags_ChildWindow) {
            IM_ASSERT(parent_window && parent_window->Active);
            window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
            parent_window->DC.ChildWindows.push_back(window);
            if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
                window->Pos = parent_window->DC.CursorPos;
        }

        const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesCannotSkipItems == 0);
        if (window_pos_with_pivot)
            SetWindowPos(window, window->SetWindowPosVal - window->Size * window->SetWindowPosPivot, 0);
        else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
            window->Pos = FindBestWindowPosForPopup(window);

        ImRect viewport_rect(viewport->GetMainRect());
        ImRect viewport_work_rect(viewport->GetWorkRect());
        ImVec2 visibility_padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
        ImRect visibility_rect(viewport_work_rect.Min + visibility_padding, viewport_work_rect.Max - visibility_padding);

        if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow))
            if (viewport_rect.GetWidth() > 0.0f && viewport_rect.GetHeight() > 0.0f)
                ClampWindowPos(window, visibility_rect);
        window->Pos = ImTrunc(window->Pos);

        window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

        bool want_focus = false;
        if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing)) {
            if (flags & ImGuiWindowFlags_Popup)
                want_focus = true;
            else if ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
                want_focus = true;
        }

#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (g.TestEngineHookItems) {
            IM_ASSERT(window->IDStack.Size == 1);
            window->IDStack.Size = 0;
            IMGUI_TEST_ENGINE_ITEM_ADD(window->ID, window->Rect(), NULL);
            IMGUI_TEST_ENGINE_ITEM_INFO(window->ID, window->Name, (g.HoveredWindow == window) ? ImGuiItemStatusFlags_HoveredRect : 0);
            window->IDStack.Size = 1;
        }
#endif
        int border_hovered = -1, border_held = -1;
        ImU32 resize_grip_col[4] = {};
        const int resize_grip_count = (window->Flags & ImGuiWindowFlags_ChildWindow) ? 0 : g.IO.ConfigWindowsResizeFromEdges ? 2 : 1;
        const float resize_grip_draw_size = ImTrunc(ImMax(g.FontSize * 1.10f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
        if (!window->Collapsed)
            if (int auto_fit_mask = UpdateWindowManualResize(window, size_auto_fit, &border_hovered, &border_held, resize_grip_count, &resize_grip_col[0], visibility_rect)) {
                if (auto_fit_mask & (1 << ImGuiAxis_X))
                    use_current_size_for_scrollbar_x = true;
                if (auto_fit_mask & (1 << ImGuiAxis_Y))
                    use_current_size_for_scrollbar_y = true;
            }
        window->ResizeBorderHovered = (signed char)border_hovered;
        window->ResizeBorderHeld = (signed char)border_held;

        if (!window->Collapsed) {
            ImVec2 avail_size_from_current_frame = ImVec2(window->SizeFull.x, window->SizeFull.y - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2));
            ImVec2 avail_size_from_last_frame = window->InnerRect.GetSize() + scrollbar_sizes_from_last_frame;
            ImVec2 needed_size_from_last_frame = window_just_created ? ImVec2(0, 0) : window->ContentSize + window->WindowPadding * 2.0f;
            float size_x_for_scrollbars = use_current_size_for_scrollbar_x ? avail_size_from_current_frame.x : avail_size_from_last_frame.x;
            float size_y_for_scrollbars = use_current_size_for_scrollbar_y ? avail_size_from_current_frame.y : avail_size_from_last_frame.y;
            window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
            window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((needed_size_from_last_frame.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
            if (window->ScrollbarX && !window->ScrollbarY)
                window->ScrollbarY = (needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar);
            window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);

            window->DecoOuterSizeX2 += window->ScrollbarSizes.x;
            window->DecoOuterSizeY2 += window->ScrollbarSizes.y;
        }

        const ImRect host_rect = ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip) ? parent_window->ClipRect : viewport_rect;
        const ImRect outer_rect = window->Rect();
        const ImRect title_bar_rect = window->TitleBarRect();
        window->OuterRectClipped = outer_rect;
        window->OuterRectClipped.ClipWith(host_rect);

        window->InnerRect.Min.x = window->Pos.x + window->DecoOuterSizeX1;
        window->InnerRect.Min.y = window->Pos.y + window->DecoOuterSizeY1;
        window->InnerRect.Max.x = window->Pos.x + window->Size.x - window->DecoOuterSizeX2;
        window->InnerRect.Max.y = window->Pos.y + window->Size.y - window->DecoOuterSizeY2;

        float top_border_size = (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
        window->InnerClipRect.Min.x = ImTrunc(0.5f + window->InnerRect.Min.x + ImMax(ImTrunc(window->WindowPadding.x * 0.5f), window->WindowBorderSize));
        window->InnerClipRect.Min.y = ImTrunc(0.5f + window->InnerRect.Min.y + top_border_size);
        window->InnerClipRect.Max.x = ImTrunc(0.5f + window->InnerRect.Max.x - ImMax(ImTrunc(window->WindowPadding.x * 0.5f), window->WindowBorderSize));
        window->InnerClipRect.Max.y = ImTrunc(0.5f + window->InnerRect.Max.y - window->WindowBorderSize);
        window->InnerClipRect.ClipWithFull(host_rect);

        window->ScrollMax.x = ImMax(0.0f, window->ContentSize.x + window->WindowPadding.x * 2.0f - window->InnerRect.GetWidth());
        window->ScrollMax.y = ImMax(0.0f, window->ContentSize.y + window->WindowPadding.y * 2.0f - window->InnerRect.GetHeight());

        window->DC.StateStorage = &window->StateStorage;

        window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window, true);
        window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
        window->DecoInnerSizeX1 = window->DecoInnerSizeY1 = 0.0f;

        IM_ASSERT(window->DrawList->CmdBuffer.Size == 1 && window->DrawList->CmdBuffer[0].ElemCount == 0);
        window->DrawList->PushTextureID(g.IO.Fonts->TexID);
        PushClipRect(host_rect.Min, host_rect.Max, false);

        bool render_decorations_in_parent = false;
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip) {
            ImGuiWindow* previous_child = parent_window->DC.ChildWindows.Size >= 2 ? parent_window->DC.ChildWindows[parent_window->DC.ChildWindows.Size - 2] : NULL;
            bool previous_child_overlapping = previous_child ? previous_child->Rect().Overlaps(window->Rect()) : false;
            bool parent_is_empty = (parent_window->DrawList->VtxBuffer.Size == 0);
            if (window->DrawList->CmdBuffer.back().ElemCount == 0 && !parent_is_empty && !previous_child_overlapping)
                render_decorations_in_parent = true;
        }
        if (render_decorations_in_parent)
            window->DrawList = parent_window->DrawList;

        const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
        const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
        const bool handle_borders_and_resize_grips = true;
        RenderWindowDecorations(window, title_bar_rect, title_bar_is_highlight, handle_borders_and_resize_grips, resize_grip_count, resize_grip_col, resize_grip_draw_size);

        if (render_decorations_in_parent)
            window->DrawList = &window->DrawListInst;

        const bool allow_scrollbar_x = !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar);
        const bool allow_scrollbar_y = !(flags & ImGuiWindowFlags_NoScrollbar);
        const float work_rect_size_x = (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : ImMax(allow_scrollbar_x ? window->ContentSize.x : 0.0f, window->Size.x - window->WindowPadding.x * 2.0f - (window->DecoOuterSizeX1 + window->DecoOuterSizeX2)));
        const float work_rect_size_y = (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : ImMax(allow_scrollbar_y ? window->ContentSize.y : 0.0f, window->Size.y - window->WindowPadding.y * 2.0f - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2)));
        window->WorkRect.Min.x = ImTrunc(window->InnerRect.Min.x - window->Scroll.x + ImMax(window->WindowPadding.x, window->WindowBorderSize));
        window->WorkRect.Min.y = ImTrunc(window->InnerRect.Min.y - window->Scroll.y + ImMax(window->WindowPadding.y, window->WindowBorderSize));
        window->WorkRect.Max.x = window->WorkRect.Min.x + work_rect_size_x;
        window->WorkRect.Max.y = window->WorkRect.Min.y + work_rect_size_y;
        window->ParentWorkRect = window->WorkRect;

        window->ContentRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x + window->DecoOuterSizeX1;
        window->ContentRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->DecoOuterSizeY1;
        window->ContentRegionRect.Max.x = window->ContentRegionRect.Min.x + (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : (window->Size.x - window->WindowPadding.x * 2.0f - (window->DecoOuterSizeX1 + window->DecoOuterSizeX2)));
        window->ContentRegionRect.Max.y = window->ContentRegionRect.Min.y + (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : (window->Size.y - window->WindowPadding.y * 2.0f - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2)));

        window->DC.Indent.x = window->DecoOuterSizeX1 + window->WindowPadding.x - window->Scroll.x;
        window->DC.GroupOffset.x = 0.0f;
        window->DC.ColumnsOffset.x = 0.0f;

        double start_pos_highp_x = (double)window->Pos.x + window->WindowPadding.x - (double)window->Scroll.x + window->DecoOuterSizeX1 + window->DC.ColumnsOffset.x;
        double start_pos_highp_y = (double)window->Pos.y + window->WindowPadding.y - (double)window->Scroll.y + window->DecoOuterSizeY1;
        window->DC.CursorStartPos = ImVec2((float)start_pos_highp_x, (float)start_pos_highp_y);
        window->DC.CursorStartPosLossyness = ImVec2((float)(start_pos_highp_x - window->DC.CursorStartPos.x), (float)(start_pos_highp_y - window->DC.CursorStartPos.y));
        window->DC.CursorPos = window->DC.CursorStartPos;
        window->DC.CursorPosPrevLine = window->DC.CursorPos;
        window->DC.CursorMaxPos = window->DC.CursorStartPos;
        window->DC.IdealMaxPos = window->DC.CursorStartPos;
        window->DC.CurrLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
        window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
        window->DC.IsSameLine = window->DC.IsSetPos = false;

        window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        window->DC.NavLayersActiveMask = window->DC.NavLayersActiveMaskNext;
        window->DC.NavLayersActiveMaskNext = 0x00;
        window->DC.NavIsScrollPushableX = true;
        window->DC.NavHideHighlightOneFrame = false;
        window->DC.NavWindowHasScrollY = (window->ScrollMax.y > 0.0f);

        window->DC.MenuBarAppending = false;
        window->DC.MenuColumns.Update(style.ItemSpacing.x, window_just_activated_by_user);
        window->DC.TreeDepth = 0;
        window->DC.ChildWindows.resize(0);
        window->DC.CurrentColumns = NULL;
        window->DC.LayoutType = ImGuiLayoutType_Vertical;
        window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;

        window->DC.TextWrapPos = -1.0f;
        window->DC.ItemWidthStack.resize(0);
        window->DC.TextWrapPosStack.resize(0);

        if (window->AutoFitFramesX > 0)
            window->AutoFitFramesX--;
        if (window->AutoFitFramesY > 0)
            window->AutoFitFramesY--;

        if (want_focus)
            FocusWindow(window, ImGuiFocusRequestFlags_UnlessBelowModal);
        if (want_focus && window == g.NavWindow)
            NavInitWindow(window, false);

        if (!(flags & ImGuiWindowFlags_NoTitleBar))
            RenderWindowTitleBarContents(window, ImRect(title_bar_rect.Min.x + window->WindowBorderSize, title_bar_rect.Min.y, title_bar_rect.Max.x - window->WindowBorderSize, title_bar_rect.Max.y), name.data(), p_open);

        window->HitTestHoleSize.x = window->HitTestHoleSize.y = 0;

#ifndef IMGUI_DISABLE_DEBUG_TOOLS
        if (g.DebugLocateId != 0 && (window->ID == g.DebugLocateId || window->MoveId == g.DebugLocateId))
            DebugLocateItemResolveWithLastItem();
#endif

#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
            IMGUI_TEST_ENGINE_ITEM_ADD(g.LastItemData.ID, g.LastItemData.Rect, &g.LastItemData);
#endif
    }
    else {
        SetCurrentWindow(window);
    }

    PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

    window->WriteAccessed = false;
    window->BeginCount++;
    g.NextWindowData.ClearFlags();

    if (first_begin_of_the_frame) {
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_ChildMenu)) {
            IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
            const bool nav_request = (window->ChildFlags & ImGuiChildFlags_NavFlattened) && (g.NavAnyRequest && g.NavWindow && g.NavWindow->RootWindowForNav == window->RootWindowForNav);
            if (!g.LogEnabled && !nav_request)
                if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y) {
                    if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
                        window->HiddenFramesCannotSkipItems = 1;
                    else
                        window->HiddenFramesCanSkipItems = 1;
                }

            if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCanSkipItems > 0))
                window->HiddenFramesCanSkipItems = 1;
            if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCannotSkipItems > 0))
                window->HiddenFramesCannotSkipItems = 1;
        }

        if (style.Alpha <= 0.0f)
            window->HiddenFramesCanSkipItems = 1;

        bool hidden_regular = (window->HiddenFramesCanSkipItems > 0) || (window->HiddenFramesCannotSkipItems > 0);
        window->Hidden = hidden_regular || (window->HiddenFramesForRenderOnly > 0);

        if (window->DisableInputsFrames > 0)
        {
            window->DisableInputsFrames--;
            window->Flags |= ImGuiWindowFlags_NoInputs;
        }
        bool skip_items = false;
        if (window->Collapsed || !window->Active || hidden_regular)
            if (window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesCannotSkipItems <= 0)
                skip_items = true;
        window->SkipItems = skip_items;
    }

#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (!window->IsFallbackWindow)
        if ((g.IO.ConfigDebugBeginReturnValueOnce && window_just_created) || (g.IO.ConfigDebugBeginReturnValueLoop && g.DebugBeginReturnValueCullDepth == g.CurrentWindowStack.Size)) {
            if (window->AutoFitFramesX > 0) { window->AutoFitFramesX++; }
            if (window->AutoFitFramesY > 0) { window->AutoFitFramesY++; }
            return false;
        }
#endif

    return !window->SkipItems;
}

void CGui::End() { ImGui::End(); } 