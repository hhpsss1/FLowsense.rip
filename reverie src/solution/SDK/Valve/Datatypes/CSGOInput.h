#pragma once

#include <cstddef>

struct C_CSPlayerPawn;

struct CSubtickMoves {
	float when;
	uint64_t button;

	union {
		bool pressed;
		struct {
			Vector2D move;
			Vector2D view;
		} analog;
	};

	//CSubtickMoves() = default;
	//CSubtickMoves(const CSubtickMoves& other) { std::memcpy(this, &other, sizeof(*this)); }

	//CSubtickMoves& operator=(const CSubtickMoves& other) {
	//	if (this != &other)
	//		std::memcpy(this, &other, sizeof(*this));

	//	return *this;
	//}
};

static_assert(sizeof(CSubtickMoves) == 0x20);

struct CAttackSubtickMoves {
	float when;
	uint64_t button;
	bool pressed;
};

static_assert(sizeof(CAttackSubtickMoves) == 0x18);

struct CFrameData {
	Taia_t render_data;
	Taia_t player_data;
	Vector viewangles;
	Vector shoot_position;
	int32_t player_attacked_index = -1;
	Vector target_head_position;
	Vector target_abs_position;
	Vector target_abs_angles;
	int32_t frame_number;
	char pad[12];

	bool ParseInputMessage(CSGOInputHistoryEntryPB* entry, bool interps_allowed, float first, float second, C_CSPlayerPawn* player);
};

static_assert(offsetof(CFrameData, player_data) == 0x08);
static_assert(offsetof(CFrameData, player_attacked_index) == 0x28);
static_assert(offsetof(CFrameData, target_head_position) == 0x2C);
static_assert(offsetof(CFrameData, target_abs_position) == 0x38);
static_assert(offsetof(CFrameData, target_abs_angles) == 0x44);
static_assert(offsetof(CFrameData, frame_number) == 0x50);
static_assert(sizeof(CFrameData) == 0x60);

struct CInputMoves {
	bool block_shot;
	bool in_thirdperson;
	char pad0[6];
	Vector thirdperson_angles;
	char pad1[20];
	uint64_t keyboard_pressed;
	uint64_t mousewheel_pressed;
	uint64_t unpressed;
	uint64_t keyboard_copy;
	float forwardmove;
	float leftmove;
	float upmove;
	int32_t moused_x;
	int32_t moused_y;
	int32_t subtick_count;
	CSubtickMoves subticks[32];
	Vector viewangles;
	int32_t target_handle;
	int32_t weapon_switch_tick;
	char pad2[140];
	int32_t predicted_subticks_count;
	char pad3[4];
	CSubtickMoves* predicted_subticks[32];
	char pad4[784];
	float subtick_fraction;
	Vector command_angles;
};

struct CExtendedMoveData {
	uint64_t keyboard_pressed;
	uint64_t mousewheel_pressed;
	uint64_t unpressed;
	float forwardmove;
	float leftmove;
	float upmove;
	int32_t moused_x;
	int32_t moused_y;
	int32_t subtick_count;
	CSubtickMoves subticks[32];
	Vector viewangles;
	float analog_fraction;
};

struct CButtonData {
	uint64_t button_flag;
	char* button_name;
	char pad0[8];
	float old_button_fraction;
	char pad1[4];
	float button_pressed_when;
	char pad2[4];
	int32_t button_when_start;
	int32_t button_pressed_tick;
	bool pressed;
	bool pressed_in_previous_tick;
	char pad3[1];
	bool unpressed;
	char pad4[20];
};

struct CButtonsData {
	CButtonData* buttons[8];
};

struct CCSGOInput {
	char pad0[16];
	CButtonData* button_attack;
	CButtonData* button_jump;
	CButtonData* button_duck;
	CButtonData* button_forward;
	CButtonData* button_back;
	CButtonData* button_use;
	char pad1[8];
	CButtonData* button_turnleft;
	CButtonData* button_turnright;
	CButtonData* button_moveleft;
	CButtonData* button_moveright;
	CButtonData* button_attack2;
	char pad2[8];
	CButtonData* reload_button;
	char pad3[16];
	CButtonData* sprint_button;
	char pad4[400];
	CInputMoves current_move_data;
	int32_t active_extended_move_datas;
	int32_t extended_move_data_size;
	CExtendedMoveData* extended_move_data;
	int32_t max_capacity_extended_move_data;
	int32_t extended_move_data_flags;
	int32_t buttons_data_size;
	int32_t current_buttons_size;
	CButtonData* predicted_buttons;
	int32_t max_buttons_data_capacity;
	int32_t buttons_data_flags;
	char pad5[61];
	bool predicting_buttons;
	bool primary_attack;
	bool secondary_attack;
	int32_t attack1_history;
	int32_t attack2_history;
	int32_t active_frames;
	int32_t frames_size;
	CFrameData* frame_data;
	int32_t max_frames_data;
	int32_t frames_data_flags;
	int32_t button_pressed_tick;
	int32_t input_tick;
	int32_t last_fire_tick;
	char pad6[612];

	void SetWeaponSelect(CUserCommand* cmd);
};
