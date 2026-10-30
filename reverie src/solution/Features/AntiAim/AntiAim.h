#pragma once

#include <atomic>
#include <chrono>

struct TrailPoint_t {
	ImVec2 pos{};
	float alpha{ 1.0f };
	float radius{ 0.0f };
};

struct CAntiAim {
	void OnCreateMove();
	void OnPresent();
private:
	void DrawDirectionIndicator();

	std::atomic<bool> indicator_active{ false };
	std::atomic<float> indicator_tick_ms{ 0.0f };
	float indicator_alpha{ 0.0f };
	float indicator_left{ 0.0f };
	float indicator_right{ 0.0f };

	float GetTargetYaw();
	float CorrectYaw();
	void MouseOverride();
	void RestoreMouse();
	POINT GetScreenCenter();
	void DrawRadialGradient(ImDrawList* dl, const ImVec2& pos, float radius, ImU32 color_in, ImU32 color_out, int segments);

	Vector angle{};
	float offset{};

	float custom_yaw{ 0.0f };
	bool has_custom_yaw{ false };
	bool mouse_active{ false };
	float virtual_x{ 0.0f };
	float virtual_y{ 0.0f };

	float visual_x{ 0.0f };
	float visual_y{ 0.0f };
	float visual_alpha{ 0.0f };

	std::vector<TrailPoint_t> trail{};
	std::mutex trail_mutex{};
	ImVec2 last_trail_pos{};
	bool has_last_pos{ false };
};

inline CAntiAim* AntiAim = new CAntiAim();