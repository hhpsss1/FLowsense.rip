#include "Ragebot.h"
#include <shared.h>
#include "../../SDK/Valve/Interfaces/NetworkClient.h"
#include <climits>
#include <limits>

CLagRecordScope::CLagRecordScope(const LagRecord* record, int processing_tick) {
	if (!LagCompensation->ValidRecord(record, processing_tick))
		return;

	skeleton = record->player->m_pGameSceneNode()->GetSkeletonInstance();
	previous = skeleton->HitboxTransformOverride();
	transforms.count = static_cast<int>(record->hitbox_transforms.size());
	transforms.capacity = transforms.count;
	transforms.transforms = record->hitbox_transforms.data();
	skeleton->HitboxTransformOverride() = &transforms;
}

CLagRecordScope::~CLagRecordScope() {
	if (skeleton)
		skeleton->HitboxTransformOverride() = previous;

}

const std::vector<LagHitbox_t>* CLagCompensation::GetHitboxes(CSkeletonInstance* skeleton, int count) {
	auto model = skeleton->GetModel();
	auto context = skeleton->HitboxListContext();
	auto set = skeleton->m_nHitboxSet();
	for (auto& cache : hitbox_cache) {
		if (cache.model == model && cache.context == context && cache.set == set && cache.hitboxes.size() == count)
			return &cache.hitboxes;

	}

	std::vector<CHitboxDescriptor> descriptors;
	if (!skeleton->GetHitboxList(descriptors, count))
		return nullptr;

	HitboxCache_t cache;
	cache.model = model;
	cache.context = context;
	cache.set = set;
	cache.hitboxes.reserve(count);
	for (int i = 0; i < count; i++) {
		auto hitbox = descriptors[i].hitbox;
		std::string name = hitbox->name ? hitbox->name : "";
		if (hitbox->bone_name)
			name += hitbox->bone_name;

		std::transform(name.begin(), name.end(), name.begin(),
					   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		LagHitbox_t data;
		data.descriptor = hitbox;
		data.mins = hitbox->mins;
		data.maxs = hitbox->maxs;
		data.radius = hitbox->shape_radius;
		data.hitgroup = hitbox->hitgroup;
		data.index = i;
		data.shape_type = hitbox->shape_type;
		data.foot = name.find("foot") != std::string::npos || name.find("ankle") != std::string::npos ||
					name.find("toe") != std::string::npos;
		cache.hitboxes.push_back(data);
	}

	hitbox_cache.emplace_back(std::move(cache));
	return &hitbox_cache.back().hitboxes;
}

bool CLagCompensation::RecordDataIntoTrack(C_CSPlayerPawn* player, LagRecord* record, int tick, const LagRecord* previous) {
	auto node = player->m_pGameSceneNode();
	auto collision = player->m_pCollision();
	auto skeleton = node ? node->GetSkeletonInstance() : nullptr;
	if (!skeleton || !collision || skeleton->pawn() != player || skeleton->HitboxTransformOverride())
		return false;

	auto model = skeleton->GetModel();
	auto pose = player->m_HitboxTransformHistory().GetLatest();
	if (!model || !pose || pose->tick != tick || !pose->transforms || pose->count <= 0 || pose->count > pose->capacity)
		return false;

	auto hitboxes = GetHitboxes(skeleton, pose->count);
	if (!hitboxes)
		return false;

	auto context = skeleton->HitboxListContext();
	auto set = skeleton->m_nHitboxSet();
	std::span<const Matrix2x4_t> transforms(pose->transforms, static_cast<size_t>(pose->count));
	const LagRecord* same_geometry = nullptr;
	if (record->MatchesGeometry(model, context, set, *hitboxes, transforms))
		same_geometry = record;
	else if (previous && previous->MatchesGeometry(model, context, set, *hitboxes, transforms))
		same_geometry = previous;

	auto& transform = node->m_nodeToWorld();
	record->player = player;
	record->handle = player->GetHandle().ToInt();
	record->m_flSpawnTime = player->m_flLastSpawnTimeIndex();
	record->m_flSimulationTime = player->m_flSimulationTime();
	record->m_nSimulationTick = TIME_TO_TICKS(record->m_flSimulationTime);
	record->animation_tick = tick;
	record->source_tick = record->m_nSimulationTick;
	record->extrapolated_ticks = 0;
	record->move_type = player->m_nActualMoveType();
	record->gravity_scale = player->m_flActualGravityScale();
	record->trace_mask = player->MovementTraceMask() | ((player->m_fFlags() & 0x10) ? 0x20 : 0);
	record->m_vecOrigin = Vector(transform.position.x, transform.position.y, transform.position.z);
	record->m_vecVelocity = player->m_vecAbsVelocity();
	record->m_vecMins = collision->m_vecMins();
	record->m_vecMaxs = collision->m_vecMaxs();
	record->m_fFlags = player->m_fFlags();
	record->m_angEyeAngles = player->m_angEyeAngles();
	record->m_angAbsRotation = node->m_angAbsRotation();
	record->update_tick = server_tick;
	auto services = player->m_pWeaponServices();
	auto weapon = services ? services->m_hActiveWeapon().get<C_CSWeaponBase>() : nullptr;
	record->weapon_handle = weapon ? weapon->GetHandle().ToInt() : -1;
	record->shooting = weapon && weapon->m_fLastShotTime() >= player->m_flOldSimulationTime() &&
					   weapon->m_fLastShotTime() <= record->m_flSimulationTime;
	std::memcpy(&record->root_transform, &transform, sizeof(record->root_transform));

	record->model = model;
	record->hitbox_list_context = context;
	record->hitbox_set = set;
	record->hitboxes = *hitboxes;
	if (same_geometry == record)
		return true;

	record->hitbox_transforms.assign(pose->transforms, pose->transforms + pose->count);
	if (same_geometry) {
		record->geometry = same_geometry->geometry;
		record->revision = same_geometry->revision;
		return true;
	}
	this->BuildGeometry(*record);
	return true;
}

void CLagCompensation::BuildGeometry(LagRecord& record) {
	record.geometry.resize(record.hitboxes.size());
	static constexpr int groups[] = {-1, 0, 1, 2, 3, 3, 4, 4, 1};
	for (size_t i = 0; i < record.hitboxes.size(); ++i) {
		const auto& data = record.hitboxes[i];
		const auto& transform = record.hitbox_transforms[i];
		int group = data.hitgroup >= 0 && data.hitgroup < std::size(groups) ? groups[data.hitgroup] : -1;
		record.geometry[i].Prepare(data.mins, data.shape_type == 1 ? data.mins : data.maxs,
									data.shape_type == 0 ? 0.f : data.radius, transform.ToMatrix3x4(),
									std::fabs(transform.array[0][3]), group, data.geometry_valid);
	}
	record.revision = ++this->record_revision;
}

bool CLagCompensation::PredictMovement(LagRecord& record, int ticks) {
	cvar(sv_gravity);
	if (!Physics || !sv_gravity || !record.trace_mask || ticks < 1 || ticks > 8)
		return false;

	const float gravity = sv_gravity->GetFloat() * record.gravity_scale;
	if (!std::isfinite(gravity) || gravity < 0.f)
		return false;

	CRay ray{};
	ray.Init(record.m_vecMins, record.m_vecMaxs);
	auto filter = Physics->MakeMovementFilter(record.player, record.trace_mask, 11);
	if (!filter.vtable)
		return false;

	for (int tick = 0; tick < ticks; ++tick) {
		auto& velocity = record.m_vecVelocity;
		if (record.m_fFlags & FL_ONGROUND)
			velocity.z = 0.f;
		else
			velocity.z -= gravity * INTERVAL_PER_TICK * 0.5f;

		Vector original_velocity = velocity;
		Vector planes[4];
		int plane_count = 0;
		float remaining = INTERVAL_PER_TICK;

		for (int bump = 0; bump < 4 && !velocity.Zero(); ++bump) {
			Vector end = record.m_vecOrigin + velocity * remaining;
			auto trace = Physics->MakeTrace();
			trace.fraction = std::numeric_limits<float>::quiet_NaN();
			Physics->TraceRay(&ray, &record.m_vecOrigin, &end, &filter, &trace);
			if (!std::isfinite(trace.fraction) || trace.fraction < 0.f || trace.fraction > 1.f || trace.start_in_solid)
				return false;

			record.m_vecOrigin += (end - record.m_vecOrigin) * trace.fraction;
			if (trace.fraction == 1.f)
				break;

			if (!trace.hit_normal.Valid() || std::abs(trace.hit_normal.LengthSqr() - 1.f) > 0.01f)
				return false;

			remaining *= 1.f - trace.fraction;
			planes[plane_count++] = trace.hit_normal;
			Vector clipped = velocity;
			int plane = 0;
			for (; plane < plane_count; ++plane) {
				clipped = velocity - planes[plane] * std::min(velocity.Dot(planes[plane]), 0.f);
				int other = 0;
				for (; other < plane_count; ++other) {
					if (other != plane && clipped.Dot(planes[other]) < -0.01f)
						break;
				}
				if (other == plane_count)
					break;
			}

			if (plane == plane_count) {
				if (plane_count != 2)
					clipped = {};
				else {
					Vector crease = planes[0].Cross(planes[1]);
					const float length = crease.LengthSqr();
					clipped = length > 0.f ? crease * (crease.Dot(velocity) / length) : Vector{};
				}
			}

			velocity = clipped.Dot(original_velocity) > 0.f ? clipped : Vector{};
		}

		record.m_fFlags &= ~FL_ONGROUND;
		if (velocity.z <= 0.f) {
			Vector end = record.m_vecOrigin - Vector(0.f, 0.f, 2.f);
			auto trace = Physics->MakeTrace();
			trace.fraction = std::numeric_limits<float>::quiet_NaN();
			Physics->TraceRay(&ray, &record.m_vecOrigin, &end, &filter, &trace);
			if (!std::isfinite(trace.fraction) || trace.fraction < 0.f || trace.fraction > 1.f || trace.start_in_solid)
				return false;

			if (trace.fraction < 1.f && (!trace.hit_normal.Valid() || std::abs(trace.hit_normal.LengthSqr() - 1.f) > 0.01f))
				return false;

			if (trace.fraction < 1.f && trace.hit_normal.z >= 0.7f) {
				record.m_vecOrigin += (end - record.m_vecOrigin) * trace.fraction;
				record.m_fFlags |= FL_ONGROUND;
				velocity.z = 0.f;
			}
		}

		if (!(record.m_fFlags & FL_ONGROUND))
			velocity.z -= gravity * INTERVAL_PER_TICK * 0.5f;

		if (!record.m_vecOrigin.Valid() || !velocity.Valid())
			return false;
	}
	return true;
}

bool CLagCompensation::Extrapolate(LagRecord& record, const LagRecord& source, const LagRecord& previous, int ticks) {
	const int gap = source.m_nSimulationTick - previous.m_nSimulationTick;
	if (ticks < 1 || ticks > 8 || gap < 1 || gap > 8 || source.extrapolated_ticks || previous.extrapolated_ticks ||
		source.animation_tick != source.m_nSimulationTick || previous.animation_tick != previous.m_nSimulationTick)
		return false;

	if (source.handle != previous.handle || source.m_flSpawnTime != previous.m_flSpawnTime || source.model != previous.model ||
		source.hitbox_list_context != previous.hitbox_list_context || source.hitbox_set != previous.hitbox_set ||
		source.hitbox_transforms.size() != previous.hitbox_transforms.size() ||
		source.weapon_handle != previous.weapon_handle || source.move_type != MOVETYPE_WALK || previous.move_type != MOVETYPE_WALK ||
		(source.m_fFlags & (FL_FROZEN | FL_INWATER | FL_ONTRAIN | FL_CONVEYOR | FL_WATERJUMP)) ||
		source.m_vecMins != previous.m_vecMins || source.m_vecMaxs != previous.m_vecMaxs ||
		((source.m_fFlags ^ previous.m_fFlags) & (FL_ONGROUND | FL_DUCKING)) ||
		(source.m_vecOrigin - previous.m_vecOrigin).LengthSqr() > 4096.f ||
		source.hitboxes.empty() || source.hitboxes.size() != source.hitbox_transforms.size())
		return false;

	record = source;
	if (!record.m_vecOrigin.Valid() || !record.m_vecVelocity.Valid() || !record.m_vecMins.Valid() || !record.m_vecMaxs.Valid())
		return false;

	for (int axis = 0; axis < 3; ++axis) {
		if (record.m_vecMins[axis] >= record.m_vecMaxs[axis])
			return false;
	}

	if (!this->PredictMovement(record, ticks))
		return false;

	const Vector offset = record.m_vecOrigin - source.m_vecOrigin;
	for (int axis = 0; axis < 3; ++axis) {
		record.root_transform.array[0][axis] += offset[axis];
		for (auto& transform : record.hitbox_transforms)
			transform.array[0][axis] += offset[axis];
	}

	record.source_tick = source.m_nSimulationTick;
	record.source_revision = source.revision;
	record.extrapolated_ticks = ticks;
	record.m_nSimulationTick += ticks;
	record.m_flSimulationTime += TICKS_TO_TIME(ticks);
	record.shooting = false;
	this->BuildGeometry(record);
	return true;
}

void CLagCompensation::ExtrapolateRecords(int processing_tick) {
	this->extrapolated_records.clear();
	this->extrapolation_tick = Config::Get<bool>(Vars::Ragebot::Extrapolation) ? processing_tick : -1;
}

void CLagCompensation::TryExtrapolate(int handle, int processing_tick) {
	const auto found = this->lag_records.find(handle);

	if (found == this->lag_records.end() || found->second.size() < 2)
		return;

	const auto& records = found->second;
	const int limit = std::clamp(Config::Get<int>(Vars::Ragebot::ExtrapolationTicks), 1, 8);
	const auto& source = records.back();

	if ((source.m_fFlags & FL_ONGROUND) && source.m_vecVelocity.LengthSqr() == 0.f)
		return;

	const int ticks = processing_tick - source.m_nSimulationTick;
	auto player = CBaseHandle(handle).get<C_CSPlayerPawn>();

	if (ticks < 1 || ticks > limit || !ValidRecord(&source, processing_tick) || !player || player != source.player
		|| !player->Alive() || !ctx.pawn || !player->Enemy(ctx.pawn) || player->m_flLastSpawnTimeIndex() != source.m_flSpawnTime
			|| player->m_flSimulationTime() != source.m_flSimulationTime || !player->m_vecBaseVelocity().Zero())
				return;

	auto& record = this->extrapolated_records[handle];
	if (!this->Extrapolate(record, source, records[records.size() - 2], ticks))
		this->extrapolated_records.erase(handle);
}

LagRecord* CLagCompensation::GetExtrapolatedRecord(C_CSPlayerPawn* player, int processing_tick) {
	if (processing_tick < 0 || processing_tick != this->extrapolation_tick)
		return nullptr;

	const int handle = player->GetHandle().ToInt();
	if (this->extrapolation_tried.insert(handle).second)
		this->TryExtrapolate(handle, processing_tick);

	auto record = this->extrapolated_records.find(handle);
	if (record == this->extrapolated_records.end() || record->second.player != player ||
		record->second.m_nSimulationTick != processing_tick || !ValidRecord(&record->second, processing_tick))
		return nullptr;

	return &record->second;
}

void CLagCompensation::OnAnimationUpdate(C_CSPlayerPawn* player, int tick) {
	if (server_tick < 0 || !player->Alive())
		return;

	auto local = EntitySystem->GetLocalPawn().As<C_CSPlayerPawn*>();
	if (!local || !local->Alive() || player == local)
		return;

	float simulation_time = player->m_flSimulationTime();
	if (simulation_time < 0.f || simulation_time >= TICKS_TO_TIME(INT_MAX) || TIME_TO_TICKS(simulation_time) != tick)
		return;

	int handle = player->GetHandle().ToInt();
	auto& records = lag_records[handle];
	LagRecord record;

	if (!records.empty() && tick <= records.back().m_nSimulationTick) {
		record = std::move(records.back());
		records.pop_back();
		if (tick < record.m_nSimulationTick)
			records.clear();

	} else if (records.size() >= static_cast<size_t>(max_records)) {
		record = std::move(records.front());
		records.pop_front();
	} else if (!free_records.empty()) {
		record = std::move(free_records.back());
		free_records.pop_back();
	}

	if (!RecordDataIntoTrack(player, &record, tick, records.empty() ? nullptr : &records.back())) {
		lag_records.erase(handle);
		return;
	}

	if (!records.empty()) {
		auto& previous = records.back();
		if (previous.model != record.model || previous.hitbox_list_context != record.hitbox_list_context ||
			previous.hitbox_set != record.hitbox_set ||
			previous.hitbox_transforms.size() != record.hitbox_transforms.size() ||
			(record.m_vecOrigin - previous.m_vecOrigin).LengthSqr() > 4096.f)
			records.clear();

	}

	records.emplace_back(std::move(record));
}

void CLagCompensation::RemoveInvalidRecords() {
	int oldest_tick = server_tick - static_cast<int>(max_unlag / INTERVAL_PER_TICK);

	for (auto it = lag_records.begin(); it != lag_records.end();) {
		auto player = CBaseHandle(it->first).get<C_CSPlayerPawn>();
		auto& records = it->second;

		if (!player || player->GetHandle().ToInt() != it->first || records.empty() || player != records.back().player ||
			!player->Alive() || player->m_flLastSpawnTimeIndex() != records.back().m_flSpawnTime) {
			it = lag_records.erase(it);
			continue;
		}

		while (!records.empty() &&
			   (records.front().m_nSimulationTick < oldest_tick || records.size() > static_cast<size_t>(max_records))) {
			free_records.emplace_back(std::move(records.front()));
			records.pop_front();
		}

		if (records.empty())
			it = lag_records.erase(it);
		else
			++it;

	}
}

void CLagCompensation::OnNetUpdate() {
	cvar(sv_maxunlag);
	static auto network = Memory->Modules.Engine2.FindInterface<CNetworkClientService*>(xs("NetworkClientService_001"));

	auto local = EntitySystem ? EntitySystem->GetLocalPawn().As<C_CSPlayerPawn*>() : nullptr;
	auto client = network ? network->GetClient() : nullptr;
	if (!local || !local->Alive() || !client || !sv_maxunlag) {
		Reset();
		return;
	}

	float window = sv_maxunlag->GetFloat();

	int tick = client->GetServerTick();
	if (tick < server_tick)
		Reset();

	if (tick != server_tick)
		server_tick_time = std::chrono::steady_clock::now();

	server_tick = tick;
	max_unlag = std::clamp(window, 0.f, 1.f);
	max_records = static_cast<int>(max_unlag / INTERVAL_PER_TICK) + 1;
	RemoveInvalidRecords();
}

int CLagCompensation::GetProcessingTick() const {
	if (server_tick < 0 || !ctx.controller)
		return -1;

	auto channel = EngineClient->GetNetChannel(0);
	if (!channel)
		return -1;

	double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - server_tick_time).count();
	double latency = std::max(channel->GetAvgLatency(), 0.f);
	int arrival_tick = server_tick + static_cast<int>(std::ceil((elapsed + latency) / INTERVAL_PER_TICK));
	return std::max(arrival_tick, ctx.controller->m_nTickBase());
}

bool CLagCompensation::ValidRecord(const LagRecord* record, int processing_tick) const {
	if (!record || server_tick < 0)
		return false;

	if (processing_tick < 0)
		processing_tick = GetProcessingTick();

	if (processing_tick < 0)
		return false;

	if (record->extrapolated_ticks) {
		const int limit = std::clamp(Config::Get<int>(Vars::Ragebot::ExtrapolationTicks), 1, 8);
		if (!Config::Get<bool>(Vars::Ragebot::Extrapolation) || record->extrapolated_ticks < 1 || record->extrapolated_ticks > limit ||
			record->source_tick + record->extrapolated_ticks != record->m_nSimulationTick || processing_tick - record->source_tick > limit ||
			(static_cast<double>(processing_tick) - record->source_tick) * INTERVAL_PER_TICK > max_unlag)
			return false;

		auto records = this->lag_records.find(record->handle);
		if (records == this->lag_records.end() || records->second.empty() ||
			records->second.back().m_nSimulationTick != record->source_tick || records->second.back().revision != record->source_revision ||
			records->second.back().player != record->player || records->second.back().m_flSpawnTime != record->m_flSpawnTime)
			return false;
	}

	double age = (static_cast<double>(processing_tick) - record->m_nSimulationTick) * INTERVAL_PER_TICK;
	return age >= 0.0 && age <= max_unlag;
}

void CLagCompensation::Reset() {
	lag_records.clear();
	extrapolated_records.clear();
	free_records.clear();
	hitbox_cache.clear();
	server_tick = -1;
	server_tick_time = {};
}

CLagCompensation* LagCompensation = new CLagCompensation;
