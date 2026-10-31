#pragma once
// CustomflowBridge - connects customflow::Menu lifecycle to the Reverie CDraw/Hooks pipeline.
// kindmops / hwidoholy

#include "menu.h"

namespace CustomflowBridge {

// Returns the singleton menu instance.
customflow::Menu& Get();

// Call after ImGui context + backends are initialized (inside CDraw::Capture).
void Init(float system_dpi);

// Call once per frame BEFORE ImGui::NewFrame() (inside CDraw::BeginFrame, before NewFrame).
void PreFrame();

// Call once per frame AFTER ImGui::NewFrame() and BEFORE ImGui::EndFrame/Render
// (replaces Gui->Render() in the present hook).
void Frame();

// Call after backend shutdown and before ImGui::DestroyContext() (inside CDraw::Clear).
void Shutdown();

// Toggle visibility (bind to Insert or whatever key Reverie uses).
void ToggleVisible();

// Set visibility state directly.
void SetVisible(bool v);

} // namespace CustomflowBridge
