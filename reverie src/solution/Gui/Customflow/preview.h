// Customflow-Remake - made by kdt255 (https://github.com/kdt255/Customflow-Remake)
// Free software: MIT + Commons Clause, selling is not permitted. If you paid for this, you got scammed.
#pragma once
#include "customflow_config.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace customflow {

// Live ESP preview: box, skeleton, name, health, weapon and flag overlays of `v`, laid out around an invisible reference body.
void DrawEspPreview(ImDrawList* dl, const ImRect& area, const PlayerVisuals& v, double time);

} // namespace customflow
