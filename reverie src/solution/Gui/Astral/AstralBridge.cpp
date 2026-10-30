// AstralBridge.cpp - lifecycle glue between astral::Menu and Reverie's CDraw/Hooks.
// kindmops / hwidoholy

#include "AstralBridge.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// Reverie globals needed for open-state toggle.
// Draw->opened is the Reverie "menu visible" flag, controlled by Insert.
// We instead let Astral own visibility; keep Draw->opened = false always so
// CGui::Render() is never called (it's replaced entirely by AstralBridge::Frame).
#include "../../SDK/Globals.h"

namespace AstralBridge {

static astral::Menu g_menu;

astral::Menu& Get() { return g_menu; }

void Init(float system_dpi)
{
    // Astral loads fonts into the shared ImGui atlas and applies the style.
    // Call after ImGui context exists and backends are up.
    g_menu.Init(system_dpi);

    // Start with menu open so the user sees it immediately.
    g_menu.SetVisible(true);
}

void PreFrame()
{
    // Recycles retired icon textures, applies pending DPI/accent changes.
    // Must run before ImGui::NewFrame().
    g_menu.PreFrame();
}

void Frame()
{
    // Drives the full menu window + overlays for this frame.
    // Must run after ImGui::NewFrame() and before ImGui::Render().
    g_menu.Frame();
}

void Shutdown()
{
    g_menu.Shutdown();
}

void ToggleVisible()
{
    g_menu.SetVisible(!g_menu.Visible());
}

void SetVisible(bool v)
{
    g_menu.SetVisible(v);
}

} // namespace AstralBridge
