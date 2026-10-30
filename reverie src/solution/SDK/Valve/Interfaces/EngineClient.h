#pragma once

struct CServerLocalFrameData {
	Vector viewangles;
	Vector shoot_position;
	int32_t target_index;
	Vector target_head_position;
	Vector target_abs_origin;
	Vector target_angles;
};

struct CServerFrameData {
	int32_t process_tick;
	int32_t render_tick;
	float render_tick_fraction;
	int32_t player_tick;
	float player_tick_fraction;
	float simulation_time;
	CServerLocalFrameData* local_frame_data;
	char pad[32];
};

struct CNetChannel {
	float GetLatency() { return Memory->Vtable.Execute<float>(this, 10); }
	float GetAvgLatency() { return GetLatency(); }
};

struct CEngineToClient {
	bool InGame() { return Memory->Vtable.Execute<bool>(this, 39); }
	void ExecuteClientCmd(const char* cmd) { Memory->Vtable.Execute<void>(this, 51, 0, cmd, 0x7FFEF001); }
	CNetChannel* GetNetChannel(int slot) { return Memory->Vtable.Execute<CNetChannel*>(this, 41, slot); }
	
	CServerFrameData GetServerFrameData() { 
		CServerFrameData data{};
		Memory->Vtable.Execute<void>(this, 173, &data);
		return data;
	}
};
