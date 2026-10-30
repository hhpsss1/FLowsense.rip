#include <shared.h>

CTraceFilter CPhysics::MakeFilter(C_CSPlayerPawn* pawn, uint64_t mask, uint8_t collision_group, uint8_t object_set_mask) {
	CTraceFilter filter{};
	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF 24")).As<CTraceFilter* (__fastcall*)(CTraceFilter*, C_CSPlayerPawn*, uint64_t, uint8_t, uint8_t)>();
	if (fn)
		fn(&filter, pawn, mask, collision_group, object_set_mask);

	return filter;
}

CTraceFilter CPhysics::MakeMovementFilter(C_CSPlayerPawn* pawn, uint64_t interacts_with, uint8_t collision_group) {
	CTraceFilter filter{};
	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF C7 41")).As<CTraceFilter*(__fastcall*)(CTraceFilter*, C_CSPlayerPawn*, uint64_t, uint8_t)>();
	if (fn)
		fn(&filter, pawn, interacts_with, collision_group);

	return filter;
}

CGameTrace CPhysics::MakeTrace() {
	CGameTrace trace{};
	static auto fn = Memory->Modules.Client.Find(xs("40 55 41 55 41 57 48 83 EC")).As<void (__fastcall*)(CGameTrace*)>();
	if (fn)
		fn(&trace);
	return trace;
}

CTraceData::CTraceData() {
	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 79 ? 33 F6 C7 47")).As<void (__fastcall*)(CTraceData*)>();
	fn(this);
}

CTraceData::~CTraceData() {
	static auto fn = Memory->Modules.Client.Find(xs("E8 ? ? ? ? 8B 8D ? ? ? ? 8B C1 48 8B 95 ? ? ? ? 25 FF FF FF 7F")).Rel(1, 5).As<void (__fastcall*)(CTraceData*)>();
	fn(this);
}

void CPhysics::SetupTrace(CTraceData* data, const Vector& start, const Vector& delta, CTraceFilter& filter, int penetration_count) {
	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? F2 0F 10 02 4D 8D 71 08 F2 0F 11 81 F8 1C 00 00")).As<void (__fastcall*)(CTraceData*, const Vector*, const Vector*, CTraceFilter*, int, bool)>();
	fn(data, &start, &delta, &filter, penetration_count, true);
}

bool CPhysics::ClipTraceToPlayer(CTraceData* data, C_CSPlayerPawn* player) {
	static auto query = Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 4D 8B CF F3 0F 11 74 24 ? 4D 8B C4 C7 44 24 ? ? ? ? ?")).Rel(3, 7).As<void**>();
	static auto clip = Memory->Modules.Client.Find(xs("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC 20 02 00 00")).As<bool (__fastcall*)(void*, CTraceData*, C_CSPlayerPawn*, const Vector*, const Vector*, int)>();
	static auto sort = Memory->Modules.Client.Find(xs("44 88 4C 24 ? 55 53 56")).As<void (__fastcall*)(CTraceSegment*, CTraceSegment*, int64_t, uint8_t)>();
	static auto rebuild = Memory->Modules.Client.Find(xs("40 55 56 41 54 41 55 41 56 48 8B EC 48 81 EC 80 00 00 00")).As<void (__fastcall*)(CTraceData*, const Vector*)>();

	if (!clip(*query, data, player, &data->start, &data->end, 0))
		return false;

	sort(data->trace_segments, data->trace_segments + data->filled_segment_count, data->filled_segment_count, 0);
	rebuild(data, &data->end);
	return true;
}

void CPhysics::TraceBullet(CTraceData* data, float damage, float penetration, float range_modifier, int team, int penetration_count) {
	static auto fn = Memory->Modules.Client.Find(xs("40 53 57 41 56 48 83 EC ? 8B 84 24")).As<void (__fastcall*)(CTraceData*, float, float, float, int, int, void*)>();
	if (fn)
		fn(data, damage, penetration, range_modifier, penetration_count, team, nullptr);
}

void CPhysics::GetTraceInfo(CTraceData* data, CGameTrace* trace, const CTraceInfo& info) {
	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B E9 0F 29 74 24")).As<void (__fastcall*)(CTraceData*, CGameTrace*, float, CTraceSegment*)>();
	fn(data, trace, info.previous_length_mod, &data->trace_segments[info.handle >> 16]);
}

bool CPhysics::TraceRay(CRay* ray, Vector* start, Vector* end, CTraceFilter* filter, CGameTrace* trace) {
	static auto fn = Memory->Modules.Client.Find(xs("48 89 54 24 ? 48 89 4C 24 ? 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? B8")).As<bool (__fastcall*)(CPhysics*, CRay*, Vector*, Vector*, CTraceFilter*, CGameTrace*)>();
	return fn && fn(this, ray, start, end, filter, trace);
}
