#include <shared.h>
#include <cstddef>
#include <cstring>

static_assert(offsetof(CMoveData, player_handle) == 4);
static_assert(offsetof(CMoveData, tickcount) == 212);
static_assert(offsetof(CMoveData, target_tick) == 216);
static_assert(offsetof(CMoveData, pre_air_move_position_z) == 300);
static_assert(offsetof(CMoveData, pre_air_move_velocity_z) == 304);
static_assert(offsetof(CMoveData, pre_air_move_accelerate_z) == 308);

void CMoveData::Reset() {
	has_zero_frametime = 0;
	is_late_command = false;
	player_handle = 0;
	abs_viewangles = {};
	viewangles = {};
	last_movement_impulses = {};
	forwardmove = 0.f;
	leftove = 0.f;
	upmove = 0.f;
	velocity = {};
	angles = {};
	input_rotated = {};

	subtick_moves.size = 0;
	attack_subtick_moves.size = 0;
	touch_list.size = 0;
	if (!touch_list.elements)
		touch_list.flags = 0x80;

	has_subtick_inputs = false;
	weapon_time_switch_subticked = 0.f;
	collision_normal = {};
	ground_normal = {};
	abs_origin = {};
	tickcount = 0;
	target_tick = 0;
	subtick_start_fraction = 0.f;
	subtick_end_fraction = 0.f;

	wish_velocity = {};
	old_angles = {};
	walk_wish_vel = {};
	continous_acceleration = {};
	frame_velocity_delta = {};
	max_speed = 0.f;
	client_max_speed = 0.f;
	friction_decel = 0.f;
	pre_air_move_position_z = 0.f;
	pre_air_move_velocity_z = 0.f;
	pre_air_move_accelerate_z = 0.f;
	in_air = false;
	gamecode_move_player = false;
	in_setup_move = false;
}

void CCSPlayer_MovementServices::CheckMovingGround(float frame_time) {
	Memory->Vtable.Execute<void>(this, 50, frame_time);
}

void CCSPlayer_MovementServices::SetupMove(CUserCommand* command, CMoveData* move_data) {
	Memory->Vtable.Execute<void>(this, 39, command, move_data);
}

void CCSPlayer_MovementServices::FinishMove(CUserCommand* command, CMoveData* move_data) {
	Memory->Vtable.Execute<void>(this, 43, command, move_data);
}

void CCSPlayer_MovementServices::DoMovement(CUserCommand* command, CMoveData* move_data) {
	using fn = void(__fastcall*)(CCSPlayer_MovementServices*, CUserCommand*, CMoveData*);
	static auto x = Memory->Modules.Client.Find(xs("48 8B C4 53 56 48 81 EC ? ? ? ? 0F 29 78")).As<fn>();
	x(this, command, move_data);
}

void CCSPlayer_MovementServices::ProcessImpacts(CUserCommand* command, CMoveData* move_data) {
	using fn = void(__fastcall*)(CCSPlayer_MovementServices*, CUserCommand*, CMoveData*);
	static auto x = Memory->Modules.Client.Find(xs("48 8B C4 53 56 41 55")).As<fn>();
	x(this, command, move_data);
}

void CCSPlayer_MovementServices::UpdateInputState(CUserCommand* command) {
	Memory->Vtable.Execute<void>(this, 30, command);
}

void CCSPlayer_MovementServices::BackupInputState() {
	auto input_state = reinterpret_cast<uint8_t*>(this);
	memcpy(input_state + 0x1E0, input_state + 0x58, 0x30);
	memcpy(input_state + 0x210, input_state + 0x190, 0x8);
	memcpy(input_state + 0x218, input_state + 0x1A0, 0xC);
}

void CCSPlayer_MovementServices::RestoreInputState() {
	auto input_state = reinterpret_cast<uint8_t*>(this);
	memcpy(input_state + 0x238, input_state + 0x198, 0x8);
	memcpy(input_state + 0x58, input_state + 0x1E0, 0x30);
	memcpy(input_state + 0x190, input_state + 0x210, 0x8);
	memcpy(input_state + 0x1A0, input_state + 0x218, 0xC);
}

void CCSPlayer_MovementServices::FinishInputState(CUserCommand* command) {
	UpdateInputState(command);

	auto input_state = reinterpret_cast<uint8_t*>(this);
	for (uint64_t button = 1; button; button <<= 1) {
		if (button & *reinterpret_cast<uint64_t*>(input_state + 0x238))
			ForceButtonState(button, false);
	}

	RemoveJumpButton();
}

void CCSPlayer_MovementServices::RemoveJumpButton() {
	Memory->Vtable.Execute<void>(this, 31);
}

void CCSPlayer_MovementServices::ForceButtonState(uint64_t button, bool pressed) {
	using fn = void(__fastcall*)(CCSPlayer_MovementServices*, uint64_t, bool);
	static auto x = Memory->Modules.Client.Find(xs("40 55 53 56 57 41 54 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 83 79 38 00 48 8B FA 41 0F B6 F0")).As<fn>();
	x(this, button, pressed);
}

void CCSPlayer_MovementServices::SetPredictionCommand(CUserCommand* command) {
	Memory->Vtable.Execute<void>(this, 47, command);
}

void CCSPlayer_MovementServices::ResetPredictionCommand() {
	Memory->Vtable.Execute<void>(this, 48);
}
