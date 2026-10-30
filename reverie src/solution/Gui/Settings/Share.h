#pragma once

#include <imgui/imgui.h>

extern int g_recent_count;
bool ColorPicker(const char* label, float col[4], ImGuiColorEditFlags flags, const char* display_name);
float CalcPickerTotalHeight(bool alpha_bar, const char* display_name);
void RenderColorRectWithAlphaCheckboard(ImDrawList* dl, ImVec2 pmin, ImVec2 pmax, ImU32 col, float gs, ImVec2 goff, float rounding, ImDrawFlags flags);