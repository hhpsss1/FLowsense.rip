#pragma once

#include "../../Utils/Math/Math.h"
#include "../../SDK/Valve/Datatypes/Hitbox.h"
#include <cstdint>
#include <cstring>
#include <chrono>
#include <deque>
#include <span>
#include <unordered_map>
#include <vector>
#include <unordered_set>

struct RageHitbox_t;

struct C_CSPlayerPawn;
struct CSkeletonInstance;
class CModel;

struct LagHitbox_t {
	const CHitbox* descriptor = nullptr;
	Vector mins;
	Vector maxs;
	float radius = 0.f;
	int hitgroup = 0;
	int index = -1;
	uint8_t shape_type = 0;
	bool foot = false;
	bool geometry_valid = true;
};

struct LagRecord {
	uint64_t revision = 0;
	C_CSPlayerPawn* player = nullptr;
	int handle = -1;
	float m_flSpawnTime = 0.f;
	float m_flSimulationTime = 0.f;
	int m_nSimulationTick = 0;
	int animation_tick = 0;
	int source_tick = 0;
	uint64_t source_revision = 0;
	int extrapolated_ticks = 0;
	int move_type = 0;
	int weapon_handle = -1;
	float gravity_scale = 1.f;
	uint64_t trace_mask = 0;
	Vector m_vecOrigin;
	Vector m_vecVelocity;
	Vector m_vecMins;
	Vector m_vecMaxs;
	int m_fFlags = 0;
	Vector m_angEyeAngles;
	Vector m_angAbsRotation;
	int update_tick = 0;
	bool shooting = false;
	Matrix2x4_t root_transform;

	CModel* model = nullptr;
	uintptr_t hitbox_list_context = 0;
	uint8_t hitbox_set = 0;
	std::span<const LagHitbox_t> hitboxes;
	std::vector<Matrix2x4_t> hitbox_transforms;
	std::vector<RageHitbox_t> geometry;

	bool MatchesGeometry(CModel* candidate_model, uintptr_t candidate_context, uint8_t candidate_set,
		std::span<const LagHitbox_t> candidate_hitboxes, std::span<const Matrix2x4_t> candidate_transforms) const {
		return revision != 0 && model == candidate_model && hitbox_list_context == candidate_context &&
			hitbox_set == candidate_set && hitboxes.data() == candidate_hitboxes.data() &&
			hitboxes.size() == candidate_hitboxes.size() && !candidate_transforms.empty() &&
			hitbox_transforms.size() == candidate_transforms.size() &&
			std::memcmp(hitbox_transforms.data(), candidate_transforms.data(), candidate_transforms.size_bytes()) == 0;
	}
};

class CLagRecordScope {
  private:
	CSkeletonInstance* skeleton = nullptr;
	void* previous = nullptr;
	CHitboxTransformList transforms;

  public:
	explicit CLagRecordScope(const LagRecord* record, int processing_tick = -1);
	~CLagRecordScope();
	CLagRecordScope(const CLagRecordScope&) = delete;
	CLagRecordScope& operator=(const CLagRecordScope&) = delete;

	explicit operator bool() const { return skeleton != nullptr; }
};

class CLagCompensation {
private:
	struct HitboxCache_t {
		CModel* model = nullptr;
		uintptr_t context = 0;
		uint8_t set = 0;
		std::vector<LagHitbox_t> hitboxes;
	};

	int extrapolation_tick = -1;

	int server_tick = -1;
	uint64_t record_revision = 0;
	std::chrono::steady_clock::time_point server_tick_time;
	int max_records = 65;
	float max_unlag = 1.f;
	std::vector<LagRecord> free_records;
	std::vector<HitboxCache_t> hitbox_cache;
	std::unordered_map<int, LagRecord> extrapolated_records;

	std::unordered_set<int> extrapolation_tried;

	const std::vector<LagHitbox_t>* GetHitboxes(CSkeletonInstance* skeleton, int count);
	void BuildGeometry(LagRecord& record);
	bool PredictMovement(LagRecord& record, int ticks);
	bool Extrapolate(LagRecord& record, const LagRecord& source, const LagRecord& previous, int ticks);
	void RemoveInvalidRecords();
	void TryExtrapolate(int handle, int processing_tick);

public:
	std::unordered_map<int, std::deque<LagRecord>> lag_records;

	bool RecordDataIntoTrack(C_CSPlayerPawn* player, LagRecord* record, int tick, const LagRecord* previous = nullptr);
	void OnAnimationUpdate(C_CSPlayerPawn* player, int tick);
	void OnNetUpdate();
	void ExtrapolateRecords(int processing_tick);
	LagRecord* GetExtrapolatedRecord(C_CSPlayerPawn* player, int processing_tick);
	int GetProcessingTick() const;
	bool ValidRecord(const LagRecord* record, int processing_tick = -1) const;
	int GetBestBacktrackTick(C_CSPlayerPawn* player, int processing_tick, int max_ticks) const;
	void Reset();
};

extern CLagCompensation* LagCompensation;
