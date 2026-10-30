#pragma once
#include <string>
#include <vector>

#include <imgui/imgui.h>
#include "../Data/FontsAwesome.h"

struct IDirect3DDevice9;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

struct CSubtab {
	std::string label;
	std::string icon;
};

struct CTab {
	std::string label;
	std::string icon;
	int id;
	std::vector<CSubtab> subtabs;
	int active_subtab{ 0 };
};

struct CCategory {
	std::string label;
	std::vector<CTab> tabs;
};

struct CVariables {
	struct {
		IDirect3DDevice9* device_dx9{ nullptr };
		ID3D11Device* device_dx11{ nullptr };
		ID3D11DeviceContext* device_context{ nullptr };
		IDXGISwapChain* swap_chain{ nullptr };
		bool active_mouse_input{ false };
	} winapi;

	struct {
		ImGuiWindowFlags flags{ ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground };
		ImVec2 size{ 800.f, 700.f };

		ImVec2 padding{ 0, 0 };
		ImVec2 spacing{ 0, 0 };
		float scroll_size{ 4.f };
		float shadow_size{ 0.f };
		float border_size{ 0.f };
		float rounding{ 16.f };
		ImU32 outline_col{ IM_COL32(58, 59, 59, 255) };
	} window;

	struct {
		float sidebar_width{ 160.f };
		float header_height{ 50.f };

		int active_tab_id{ 2 };
		int hotkeys_tab_id{ 6 };
		float content_alpha{ 0.f };

		std::vector<CCategory> categories{
			{
				("Aimbot"),
				{
					{ ("Ragebot"), ICON_FA_CROSSHAIRS, 0, {} },
					{ ("Legitbot"), ICON_FA_BULLSEYE, 1, {} }
				}
			},
			{
				("Visuals"),
				{
					{ ("Visuals"), ICON_FA_EYE, 2, { { ("Players"), ICON_FA_USERS }, { ("Weapons"), ICON_FA_GUN } } },
					{ ("World"), ICON_FA_GLOBE, 3, { { ("Ambience"), ICON_FA_EYE_LOW_VISION }, { ("Perspective"), ICON_FA_EYE } } }
				}
			},
			{
				("Miscellaneous"),
				{
					{ ("Misc"), ICON_FA_GEAR, 4, {} },
					{ ("Skins"), ICON_FA_SHIRT, 5, { { ("Inventory"), ICON_FA_BOX_ARCHIVE }, { ("Collections"), ICON_FA_BOOKMARK }, { ("Agents"), ICON_FA_USER } } },
					{ ("Hotkeys"), ICON_FA_KEYBOARD, 6, {} }
				}
			}
		};
	} gui;

	struct {
		ImFont* icons[3];
		ImFont* inter[6];
		ImFont* fa[3];
		ImFont* verdanab;
		ImFont* verdana;
		ImFont* pixel;
		ImFont* indicator_text;
		ImFont* indicator_icon;
	} font;
};

inline CVariables* Var = new CVariables();