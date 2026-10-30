#pragma once

#include <cstdint>

struct CGlobalVariables {
	float real_time;
	int32_t frame_count;
	float absolute_frame_time;
	float absolute_frame_start_time_stddev;
	int32_t max_clients;
	bool suppress_simulation;
	char m_pad1[19];
	void* start_simulation;
	float current_time;
	float frame_time;
	float absolute_subtick_fraction;
	float absolute_subtick_fraction_stddev;
	char m_pad2[4];
	int32_t tick_count;
	int32_t client_tick;
	float ingame_time;
	float interpolation_amount;
	float previous_subtick_fraction;
	int32_t current_thread_id;
	float tick_fraction;
	void* net_channel;
	char m_pad0068[280];
	char* map_name;
	char* map_name_short;
};