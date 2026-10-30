#pragma once

#include "../Datatypes/Taia.h"
#include <cstdint>

class CCSPlayerController;
class C_BaseEntity;
class CUserCommand;

struct PredictionContext_t
{
	int slot_state;
	int split_screen_slot;
	int is_local_spectate;
	int doa_state;
	void* shit;
	CCSPlayerController* controller;
	char _gap_18[16];
	__int16 dirty_list_count;
	__int16 dirty_list_cap;
	char _gap_2C[4];
	__int64 dirty_list_data;
	__int16 _gap_38;
	__int16 bone_setup_flag;
	int field_3C;
	int predictable_count;
	int _gap_44;
	C_BaseEntity* predictables;
	char _gap_50[24];
	float last_error_report_time;
	int error_ticks;
	int user_cmd_errors;
	int error_count;
	int last_executed_command_number;
	int first_predicted_command;
	int last_predicted_command_number;
	float predicted_view[3];
	int intermediate_base_command;
	int _gap_94;
	int intermediate_data_count;
	int _gap_9C;
	__int64 intermediate_data;
	int intermediate_data_max;
	int _gap_AC;
};

struct CPrediction
{
	__int64 vftable;
	__int64 app_system_cursor_08;
	__int64 app_system_cursor_10;
	__int64 field_18;
	int connection_count;
	int field_24;
	__int64 vftable_game_event_listener;
	int update_type;
	bool in_prediction;
	CUserCommand* current_command;
	int engine_tick;
	int last_server_tick;
	int last_predicted_tick;
	int field_4C;
	int prediction_contexts_size;
	int field_54;
	PredictionContext_t* prediction_contexts;
	int prediction_contexts_alloc_count;
	int prediction_contexts_grow_flags;
	__int64 saved_real_time[2];
	__int64 saved_timer[2];
	__int64 saved_timer_double[2];
	__int64 saved_current_time[2];
	__int64 saved_globals_unk[2];
	__int64 saved_tick_fraction[2];
	__int16 field_C8;
	bool prediction_enabled;
	bool field_CB;
	bool server_prediction_enabled;
	char _gap_CD[3];
	float prediction_time_offset;
	bool true_view_enabled;
	bool true_view_timing_clamped;
	char _gap_D6[6];
	Taia_t taia_true_spectate_target_time;
	Taia_t taia_prev_true_spectate_target_time;
	int desired_final_predicted_tick;
	bool fire_prediction_reset_event;
	char _gap_F1[27];
	int final_predicted_tick;
	void* prediction_data;
	int active_slot;
	int field_11C;
	int reinit_counter;
	bool needs_reinit;
	char _gap_125[3];
	uint8_t prediction_reset_state;

	void Update(int call_reason)
	{
		Memory->Vtable.Execute<void>(this, 11, call_reason);
	}
};