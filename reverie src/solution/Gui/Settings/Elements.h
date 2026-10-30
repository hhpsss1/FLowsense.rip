#pragma once

#include <imgui/imgui.h>

struct CElements {
	struct {
		ImVec2 padding{ 20, 18 };
		ImVec2 spacing{ 12, 12 };
	} content;

	struct {
		ImVec2 padding{ 12, 12 };
		ImVec2 spacing{ 0, 7 };
		float titlebar_height{ 50 };
		float name_padding{ 20 };
	} child;

	struct {
		ImVec2 size{ 115, 60 };
		float icon_padding{ 10 };
		float triangle_size{ 55 };
	} section;

	struct {
		ImVec2 size{ 90, 60 };
	} sub_section;

	struct {
		float height{ 22 };
		float width{ 28 };
		float radius{ 4 };
	} checkbox;

	struct {
		float height{ 36 };
		float clickable_height{ 16 };
		float line_padding{ 5 };
		float radius{ 4 };
	} slider;

	struct {
		float height{ 28 };
		float clickable_height{ 28 };
		float rounding{ 6 };
		float icon_padding{ 8 };
		float text_padding{ 8 };
		float selectable_height{ 28 };
	} dropdown;

	struct {
		float height{ 35 };
		float rounding{ 4 };
		float padding{ 15 };
	} text_field;

	struct {
		float height{ 35 };
		float rounding{ 4 };
	} button;

	struct {
		float height{ 16 };
		float width{ 28 };
		float rounding{ 4.f };
		ImVec2 padding{ 8, 8 };
		ImVec2 picker_size{ 180, 120 };
		ImVec2 bar_size{ picker_size.x, 8 };
	} color_edit;

	struct {
		float height{ 16 };
		float padding{ 10 };
		float rounding{ 4 };
	} keybind;

	struct {
		float rounding{ 4.f };
		float item_height{ 20.f };
		float item_padding{ 8.f };
		float item_spacing{ 4.f };
		ImVec2 padding{ 0.f, 4.f };
		float border_size{ 1.f };
		float scrollbar_width{ 6.f };
	} listbox;
};

inline CElements* Elements = new CElements();