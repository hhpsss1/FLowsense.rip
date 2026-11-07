#include <shared.h>
#include "../../SDK/Valve/Interfaces/NetworkClient.h"
#include <cfloat>
#include "ScanOrdering.h"
#include "HitboxPacket.h"

static int64_t NowMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static bool isDataValid(const LagRecord* record) {
	if (!record) return false;

	const size_t count = record->hitboxes.size();

	if (!count) return false;

	if (record->geometry.size() != count) return false;

	if (record->hitbox_transforms.size() != count) return false;

	return true;
}

Vector CRagebot::CalculateAngle(const Vector& direction) {
	return {-std::atan2(direction.z, std::hypot(direction.x, direction.y)) * Math::rads, std::atan2(direction.y, direction.x) * Math::rads, 0.f};
}

struct RageWeaponGroupVars_t {
	std::uint32_t AutomaticFire;
	std::uint32_t AutomaticScope;
	std::uint32_t AutoStop;
	std::uint32_t PenetrateWalls;
	std::uint32_t SilentAim;
	std::uint32_t Zeusbot;
	std::uint32_t Knifebot;
	std::uint32_t Hitchance;
	std::uint32_t MinimumDamage;
	std::uint32_t PointScale;
	std::uint32_t MaximumFov;
	std::uint32_t BodyPointScale;
	std::uint32_t TargetSelection;
	std::uint32_t DelayShotEnable;
	std::uint32_t DelayShot;
	std::uint32_t RemoveRecoil;
	std::uint32_t StrictHitchance;
	std::uint32_t ForceBodyAim;
	std::uint32_t Hitboxes;
	std::uint32_t Multipoints;
	std::uint32_t MaximumDistance;
	std::uint32_t TicksPerAimpoint;
};

#define BIND_RAGE_WEAPON_GROUP(grp) { \
	grp::AutomaticFire, grp::AutomaticScope, grp::AutoStop, grp::PenetrateWalls, \
	grp::SilentAim, grp::Zeusbot, grp::Knifebot, grp::Hitchance, grp::MinimumDamage, \
	grp::PointScale, grp::MaximumFov, grp::BodyPointScale, grp::TargetSelection, \
	grp::DelayShotEnable, grp::DelayShot, \
	grp::RemoveRecoil, grp::StrictHitchance, grp::ForceBodyAim, grp::Hitboxes, grp::Multipoints, \
	grp::MaximumDistance, grp::TicksPerAimpoint \
} \

enum e_rage_group : int {
	group_global = 0,
	group_autosniper,
	group_scout,
	group_awp,
	group_deagle,
	group_revolver,
	group_pistol
};

weapon_settings_t CRagebot::GetWeaponSettings(int weapon_id) {
	static const RageWeaponGroupVars_t autosniper_grp = BIND_RAGE_WEAPON_GROUP(Vars::Ragebot::AutoSniper);
	static const RageWeaponGroupVars_t scout_grp = BIND_RAGE_WEAPON_GROUP(Vars::Ragebot::Scout);
	static const RageWeaponGroupVars_t awp_grp = BIND_RAGE_WEAPON_GROUP(Vars::Ragebot::Awp);
	static const RageWeaponGroupVars_t deagle_grp = BIND_RAGE_WEAPON_GROUP(Vars::Ragebot::Deagle);
	static const RageWeaponGroupVars_t revolver_grp = BIND_RAGE_WEAPON_GROUP(Vars::Ragebot::Revolver);
	static const RageWeaponGroupVars_t pistol_grp = BIND_RAGE_WEAPON_GROUP(Vars::Ragebot::Pistol);
	static const RageWeaponGroupVars_t global_grp = BIND_RAGE_WEAPON_GROUP(Vars::Ragebot::Global);

	const RageWeaponGroupVars_t* grp = &global_grp;
	int group_id = group_global;

	switch (weapon_id) {
	case WEAPON_G3SG1:
	case WEAPON_SCAR20:
		grp = &autosniper_grp;
		group_id = group_autosniper;
		break;
	case WEAPON_SSG08:
		grp = &scout_grp;
		group_id = group_scout;
		break;
	case WEAPON_AWP:
		grp = &awp_grp;
		group_id = group_awp;
		break;
	case WEAPON_DEAGLE:
		grp = &deagle_grp;
		group_id = group_deagle;
		break;
	case WEAPON_REVOLVER:
		grp = &revolver_grp;
		group_id = group_revolver;
		break;
	default:
		if (ctx.weapon_data->m_WeaponType() == WEAPONTYPE_PISTOL) {
			grp = &pistol_grp;
			group_id = group_pistol;
		} else {
			grp = &global_grp;
			group_id = group_global;
		}
		break;
	}

	weapon_settings_t result;
	const auto& hitboxes = Config::Get<std::vector<bool>>(grp->Hitboxes);
	const auto& multipoints = Config::Get<std::vector<bool>>(grp->Multipoints);
	for (int i = 0; i < 6; ++i) {
		result.hitboxes[i] = i < hitboxes.size() && hitboxes[i];
		result.multipoints[i] = i < multipoints.size() && multipoints[i];
	}

	result.force_shot = Config::Get<bool>(Vars::Ragebot::ForceShot);
	result.penetrate_walls = Config::Get<bool>(grp->PenetrateWalls);
	result.automatic_fire = Config::Get<bool>(grp->AutomaticFire);
	result.automatic_scope = Config::Get<bool>(grp->AutomaticScope);
	result.stop_mode = 0;
	const auto& stop_modes = Config::Get<std::vector<bool>>(grp->AutoStop);
	for (auto i{ 0uz }; i < std::min(stop_modes.size(), size_t(4)); ++i) {
		if (stop_modes[i])
			result.stop_mode |= 1 << i;
	}
	result.silent_aim = Config::Get<bool>(grp->SilentAim);
	// No backtrack: historical records are no longer used.
	result.remove_recoil = Config::Get<bool>(grp->RemoveRecoil);
	result.strict_hitchance = Config::Get<bool>(grp->StrictHitchance);
	result.zeusbot = Config::Get<bool>(Vars::Ragebot::Zeusbot);
	result.knifebot = Config::Get<bool>(Vars::Ragebot::Knifebot);
	result.force_body_aim = Config::Get<bool>(grp->ForceBodyAim);
	result.target_selection = std::clamp(Config::Get<int>(grp->TargetSelection), 0, 3);
	result.delay_shot = Config::Get<bool>(grp->DelayShotEnable) ? std::clamp(Config::Get<int>(grp->DelayShot), 0, 16) : 0;
	result.hitchance = std::clamp(Config::Get<float>(grp->Hitchance), 0.f, 100.f);
	result.point_scale = std::clamp(Config::Get<float>(grp->PointScale) * 0.01f, 0.f, 1.f);
	result.body_point_scale = std::clamp(Config::Get<float>(grp->BodyPointScale) * 0.01f, 0.f, 1.f);
	result.minimum_damage = Config::Get<int>(grp->MinimumDamage);
	result.maximum_fov = std::clamp(Config::Get<float>(grp->MaximumFov), 0.f, 180.f);
	result.maximum_distance = std::max(Config::Get<float>(grp->MaximumDistance), 0.f);
	result.stop_speed = std::clamp(Config::Get<float>(grp->StopSpeed), 1.f, 100.f);
	result.resolver = Config::Get<bool>(grp->Resolver);
	result.resolver_mode = Config::Get<int>(grp->ResolverMode);
	result.resolver_hitbox_priority = Config::Get<int>(grp->ResolverHitboxPriority);
	result.resolver_yaw_range = std::clamp(Config::Get<float>(grp->ResolverYawRange), 1.f, 180.f);
	settings_prefix = group_id;
	return result;
}
int CRagebot::HitboxGroup(const LagHitbox_t& hitbox) const {
	static constexpr int groups[] = {-1, 0, 1, 2, 3, 3, 4, 4, 1};
	if (hitbox.hitgroup < 0 || hitbox.hitgroup >= std::size(groups))
		return -1;

	return hitbox.foot && (hitbox.hitgroup == 6 || hitbox.hitgroup == 7) ? 5 : groups[hitbox.hitgroup];
}

float CRagebot::CalcMinDamage(C_CSPlayerPawn* player) const {
	float health = static_cast<float>(player->m_iHealth());
	return settings.minimum_damage >= 100.f ? health + settings.minimum_damage - 100.f : std::min(settings.minimum_damage, health + 1.f);
}

float CRagebot::PrepareTargetDamage(C_CSPlayerPawn* player) {
	target_damage = AutoWall->PrepareDamage(player);
	static constexpr int groups[] = {-1, 0, 1, 2, 3, 3, 4, 4, 1};
	float limit = 0.f;
	for (int hitgroup = 0; hitgroup < 9; ++hitgroup) {
		maximum_damage[hitgroup] = AutoWall->DamageUpperBound(target_damage, hitgroup);
		int group = groups[hitgroup];
		if (group < 0 || (!settings.hitboxes[group] && !(group == 4 && settings.hitboxes[5])) || (group == 0 && settings.force_body_aim))
			continue;
		limit = std::max(limit, maximum_damage[hitgroup] * scan_bullets);
	}
	return limit;
}

std::span<LagRecord*> CRagebot::SelectRecords(C_CSPlayerPawn* player, bool extrapolate) {
	selected_records.clear();
	auto found = LagCompensation->lag_records.find(player->GetHandle().ToInt());
	if (found == LagCompensation->lag_records.end())
		return selected_records;

	if (processing_tick < 0)
		return selected_records;

	if (extrapolate && Config::Get<bool>(Vars::Ragebot::Prediction)) {
		if (auto record = LagCompensation->GetExtrapolatedRecord(player, processing_tick)) {
			selected_records.push_back(record);
		}
	}

	auto& records = found->second;
	// Backtrack: allow records within configured tick window (6-14 ticks).
	for (size_t i = records.size(); i > 0; --i) {
		auto record = &records[i - 1];
		if (!LagCompensation->ValidRecord(record, processing_tick))
			continue;

		selected_records.push_back(record);
	}

	// Backtrack prediction: sort so the predicted best tick is scanned first.
	if (Config::Get<bool>(Vars::Ragebot::BacktrackPrediction) && !selected_records.empty()) {
		int best_offset = LagCompensation->GetBestBacktrackTick(player, processing_tick, Config::Get<int>(Vars::Ragebot::BacktrackPredictionTicks));
		int target_tick = processing_tick - best_offset;

		std::sort(selected_records.begin(), selected_records.end(),
			[target_tick](LagRecord* a, LagRecord* b) {
				int da = std::abs(a->m_nSimulationTick - target_tick);
				int db = std::abs(b->m_nSimulationTick - target_tick);
				return da < db;
			});
	}

	return selected_records;
}

void CRagebot::SelectPoints(LagRecord* record, std::vector<AimPoint_t>& points, const std::array<ScannedPoint_t, 2>& best, int phase) {
	cvar(mp_damage_headshot_only);
	bool head_only = mp_damage_headshot_only->GetBool();
	float minimum_damage = CalcMinDamage(record->player);
	int record_priority = record->shooting ? 2 : (record->m_angEyeAngles.x < 10.f ? 1 : 0);
	record_priority += GlobalVars->tick_count - record->update_tick < 12;

	// --- CS2 resolver integration ---
	UpdateResolver(record->player, record);
	float resolved_yaw = ResolveYaw(record->player, record);
	int resolver_priority = GetResolverHitboxPriority(record, resolved_yaw);

	for (auto i{ 0uz }; i < record->hitboxes.size(); ++i) {
		const auto& hitbox = record->hitboxes[i];
		int group = HitboxGroup(hitbox);
		if (group < 0 || !settings.hitboxes[group] || (group == 0 && settings.force_body_aim) || (head_only && group != 0))
			continue;

		// Resolver hitbox priority: deprioritize hitboxes that don't match resolved aim direction
		if (settings.resolver && resolver_priority >= 0) {
			int group_priority = GetResolverHitboxPriority(record, resolved_yaw);
			// If this group doesn't match the preferred priority, reduce point_scale slightly
			if (group_priority != group && group_priority == 0 && group != 0) {
				// Preferring head, this is body - reduce scale
				// We'll handle this by adjusting scale below
			}
		}

		bool body = hitbox.hitgroup == 2 || hitbox.hitgroup == 3;
		int center_phase = body ? 3 : 1;
		bool center = phase == center_phase;
		if (!center && phase != center_phase - 1)
			continue;

		float damage = maximum_damage[hitbox.hitgroup] * scan_bullets;
		if (damage < minimum_damage)
			continue;

		ScannedPoint_t limit;
		limit.priority = record_priority + phase;
		limit.damage = damage;
		const auto& previous = best[hitbox.hitgroup == 1];
		if (previous.record && !(limit < previous))
			continue;

		const auto& geometry = record->geometry[i];
		if (!geometry.valid)
			continue;

		if (center) {
			points.push_back({geometry.center, static_cast<int>(i), false, false, limit.priority});
			continue;
		}

		float scale = group == 0 ? settings.point_scale : settings.body_point_scale;
		// Resolver: reduce scale for non-priority hitboxes
		if (settings.resolver && resolver_priority >= 0 && group != resolver_priority) {
			scale *= 0.5f;
		}
		if (!settings.multipoints[group] || scale <= 0.f)
			continue;

		if (!multipoints_ready)
			PrepareMultipoints(record);

		const auto& multipoints = multipoint_points[i];
		for (size_t index = 1; index < multipoints.count; ++index) {
			auto point = multipoints.points[index];
			point.priority = limit.priority;
			points.push_back(point);
		}
	}
}

size_t CRagebot::SelectBestPoint(ScannedTarget_t* target, std::vector<ScannedPoint_t>& points) {
	constexpr size_t invalid = static_cast<size_t>(-1);
	auto select = [&](bool head) {
		for (;;) {
			size_t selected = invalid;
			for (auto i{ 0uz }; i < points.size(); ++i) {
				const auto& point = points[i];
				if (!point.record || point.dont_shoot || (point.hitgroup == 1) != head || !(point.damage >= target->minimum_damage && point.damage > 0.f))
					continue;

				if (!(point.hitchance < 0.f || point.hitchance >= required_hitchance))
					continue;

				if (selected == invalid || point < points[selected])
					selected = i;
			}

			if (selected == invalid || points[selected].hitchance >= required_hitchance)
				return selected;

			auto& point = points[selected];
			ScannedTarget_t candidate;
			candidate.player = target->player;
			candidate.best_point = point;
			candidate.angle = CalculateAngle(point.point - eye_position);
			point.hitchance = CalcHitchance(&candidate);
		}
	};

	auto body = select(false);
	auto head = body == invalid || points[body].damage < target->player->m_iHealth() ? select(true) : invalid;
	if (settings.delay_shot > 0) {
		int& delay = delayed_ticks[target->player->GetHandle().ToInt()];
		if (body != invalid && head != invalid) {
			if (delay < settings.delay_shot)
				head = invalid;
			delay = std::min(delay + 1, settings.delay_shot);
		} else {
			delay = 0;
		}
	} else if (!delayed_ticks.empty()) {
		delayed_ticks.erase(target->player->GetHandle().ToInt());
	}

	size_t selected = body != invalid ? body : head;
	if (head != invalid) {
		float body_damage = body != invalid ? points[body].damage : 0.f;
		if (body_damage < target->player->m_iHealth() && points[head].damage > body_damage && points[head].record->shooting)
			selected = head;
	}

	if (selected != invalid) {
		target->best_point = points[selected];
		target->angle = CalculateAngle(target->best_point.point - eye_position);
		target->hitchance = target->best_point.hitchance;
	}

	return selected;
}

ScannedTarget_t CRagebot::ScanTarget(C_CSPlayerPawn* player) {
	ScannedTarget_t target;
	target.player = player;
	target.minimum_damage = CalcMinDamage(player);
	if (PrepareTargetDamage(player) < target.minimum_damage)
		return target;
	auto records = SelectRecords(player);
	if (records.empty())
		return target;

	auto& scanned = point_results;
	auto& debug = point_debug;
	scanned.clear();
	debug.clear();
	std::array<ScannedPoint_t, 2> best{};
	bool stop_ready = !settings.stop_mode || (stop_target.best_point.record && stop_target.best_point.damage >= stop_target.minimum_damage);
	bool needs_scope = settings.automatic_scope && ctx.weapon_data->m_nZoomLevels() > 0 && !ctx.pawn->m_bIsScoped();
	int maximum_priority = 0;
	for (auto record : records) {
		int priority = record->shooting ? 2 : (record->m_angEyeAngles.x < 10.f ? 1 : 0);
		priority += GlobalVars->tick_count - record->update_tick < 12;
		maximum_priority = std::max(maximum_priority, priority + 3);
	}

	auto filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);
	auto target_filter = Physics->MakeFilter(player, 0x1C300B, 3, 15);
	filter.shape_attr.flags |= 2;
	filter.shape_attr.interacts_exclude |= 0x4000000000;
	filter.shape_attr.entity_ids_to_ignore[1] = target_filter.shape_attr.entity_ids_to_ignore[0];

	for (auto record : records) {
		if (!isDataValid(record)) continue;

		int priority = record->shooting ? 2 : (record->m_angEyeAngles.x < 10.f ? 1 : 0);
		priority += GlobalVars->tick_count - record->update_tick < 12;
		bool lethal_body = best[0].record && best[0].damage >= player->m_iHealth() && best[0].priority >= maximum_priority;
		bool body_done = best[0].record && best[0].priority > priority + 3;
		bool head_done = lethal_body || (best[1].record && best[1].priority > priority + 1);
		if (body_done && head_done)
			continue;

		CLagRecordScope scope(record, processing_tick);
		if (!scope)
			continue;

		hitchance_record = nullptr;

		auto& points = scan_points;
		auto& results = scan_results;
		points.clear();
		points.reserve(record->hitboxes.size() + 13);
		results.clear();
		results.reserve(record->hitboxes.size() + 13);
		multipoints_ready = false;
		float trace_range = CalcTraceRange(*record);
		TraceRecord_t* trace_record = nullptr;
		if (collect_stop_traces) {
			if (trace_record_count == trace_records.size())
				trace_records.emplace_back();

			trace_record = &trace_records[trace_record_count++];
			trace_record_lookup.emplace_back(record, trace_record_count - 1);
			trace_record->record = record;
			trace_record->range = trace_range;
			trace_record->source = eye_position;
			trace_record->points.clear();
		}

		std::array<bool, 2> spread_limited{};
		auto scan = [&](size_t slot) {
			size_t index = scan_indices[slot];
			auto& point = points[index];
			point.scanned = true;
			Vector direction = point.point - eye_position;
			float distance = direction.Normalize();
			if (distance <= 0.f || distance * distance > range_squared || direction.Dot(view_direction) < fov_cosine)
				return;

			auto& result = results[index];
			CAutoWall::Result_t bullet;
			point.traced = true;
			bool hit = AutoWall->Run(eye_position, point.point, player, bullet, settings.penetrate_walls, trace_range, &filter, &target_damage);
			if (trace_record)
				trace_record->points[index] = {{point.point.x, point.point.y, point.point.z}, bullet, hit};

			if (!hit || bullet.hitgroup != record->hitboxes[point.hitbox].hitgroup)
				return;

			result.record = record;
			result.point = point.point;
			result.hitbox = point.hitbox;
			result.multipoint = point.multipoint;
			result.damage = bullet.damage * scan_bullets;
			result.hitgroup = bullet.hitgroup;
			result.penetrated = bullet.penetrated;
			result.dont_shoot = point.dont_shoot;
			result.priority = point.priority;
		};
		for (int phase = 3; phase >= 0; --phase) {
			if (phase <= 1 && best[0].record && best[0].damage >= player->m_iHealth() && best[0].priority >= maximum_priority)
				break;
			if (phase <= 1 && !show_points && settings.delay_shot <= 0 && records.size() == 1 && !record->shooting && best[0].record && !best[0].dont_shoot)
				break;

			size_t first = points.size();
			SelectPoints(record, points, best, phase);
			results.resize(points.size());
			if (trace_record)
				trace_record->points.resize(points.size());

			size_t probe = first;
			for (size_t i = first + 1; i < points.size(); ++i) {
				if (maximum_damage[record->hitboxes[points[i].hitbox].hitgroup] > maximum_damage[record->hitboxes[points[probe].hitbox].hitgroup])
					probe = i;
			}

			size_t last = first;
			size_t batch_size = 1;
			while (last < points.size()) {
				scan_indices.clear();
				while (last < points.size() && scan_indices.size() < batch_size) {
					size_t index = last == first ? probe : (last == probe ? first : last);
					++last;
					auto& point = points[index];
					int group = record->hitboxes[point.hitbox].hitgroup;
					ScannedPoint_t limit;
					limit.priority = point.priority;
					limit.damage = maximum_damage[group] * scan_bullets;
					const auto& previous = best[group == 1];
					bool scan_point = !previous.record || limit < previous;
					if (scan_point && stop_ready && required_hitchance > 0.f && (!needs_scope || target.has_damage)) {
						Vector direction = point.point - eye_position;
						float distance = direction.Normalize();
						if (distance <= 0.f || distance * distance > range_squared || direction.Dot(view_direction) < fov_cosine) {
							point.scanned = true;
							scan_point = false;
						} else {
							ScannedTarget_t candidate;
							candidate.player = player;
							candidate.best_point.record = record;
							candidate.best_point.point = point.point;
							candidate.best_point.hitbox = point.hitbox;
							candidate.angle = CalculateAngle(point.point - eye_position);
							auto& result = results[index];
							if (!CanReachHitchance(&candidate)) {
								result.hitchance = std::nextafter(required_hitchance, 0.f);
								scan_point = false;
							} else if (point.multipoint && spread_limited[group == 1]) {
								result.hitchance = CalcHitchance(&candidate);
								scan_point = result.hitchance >= required_hitchance;
							}
						}
					}

					if (scan_point)
						scan_indices.push_back(index);
				}

				for (size_t slot = 0; slot < scan_indices.size(); ++slot)
					scan(slot);

				RageScan::StableInsertionSort(std::span(scan_indices), [&](size_t left, size_t right) {
					return results[left] < results[right];
				});
				for (size_t index : scan_indices) {
					auto& result = results[index];
					auto& previous = best[result.hitgroup == 1];
					if (!result.record || result.damage < target.minimum_damage)
						continue;

					if (!result.dont_shoot)
						target.has_damage = true;

					stop_ready = true;
					if (previous.record && !(result < previous))
						continue;

					ScannedTarget_t candidate;
					candidate.player = player;
					candidate.best_point = result;
					candidate.angle = CalculateAngle(result.point - eye_position);
					if (result.hitchance < 0.f)
						result.hitchance = CalcHitchance(&candidate);

					if (result.hitchance >= required_hitchance)
						previous = result;
					else if (!result.multipoint)
						spread_limited[result.hitgroup == 1] = true;
				}

				batch_size = 8;
			}
		}

		if (trace_record) {
			size_t count = 0;
			for (auto i{ 0uz }; i < points.size(); ++i) {
				if (points[i].traced)
					trace_record->points[count++] = trace_record->points[i];
			}
			trace_record->points.resize(count);
			if (count > 1) {
				std::sort(trace_record->points.data(), trace_record->points.data() + count, [](const TracePoint_t& first, const TracePoint_t& second) {
					return first.position < second.position;
				});
			}
		}

		for (auto i{ 0uz }; i < results.size(); ++i) {
			auto& result = results[i];
			result.debug_index = debug.size();
			if (show_points)
				debug.push_back({points[i].point, result.damage, target.minimum_damage, result.hitchance, result.record != nullptr, false, result.hitchance >= 0.f && result.hitchance < required_hitchance, points[i].scanned, record->m_nSimulationTick});

			if (result.record) {
				scanned.push_back(result);
				bool stop = result.damage >= target.minimum_damage;
				bool previous_stop = stop_target.best_point.record && stop_target.best_point.damage >= stop_target.minimum_damage;
				if (settings.stop_mode && !record->extrapolated_ticks && (!stop_target.player || (stop && !previous_stop) || (stop == previous_stop && result.damage > stop_target.best_point.damage))) {
					stop_target.player = player;
					stop_target.minimum_damage = target.minimum_damage;
					stop_target.best_point = result;
				}
			}
		}
	}

	size_t selected = SelectBestPoint(&target, scanned);
	if (show_points) {
		float required = required_hitchance;
		for (auto i{ 0uz }; i < scanned.size(); ++i) {
			auto& point = debug[scanned[i].debug_index];
			point.hitchance = scanned[i].hitchance;
			point.hitchance_failed = point.hitchance >= 0.f && point.hitchance < required;
		}

		if (selected != static_cast<size_t>(-1))
			debug[scanned[selected].debug_index].selected = true;

		int tick = target.best_point.record ? target.best_point.record->m_nSimulationTick : (debug.empty() ? -1 : debug.front().tick);
		std::lock_guard lock(debug_mutex);
		for (const auto& point : debug) {
			if (point.tick == tick)
				debug_points.push_back(point);
		}
		debug_revision++;
	}

	return target;
}

ScannedTarget_t CRagebot::ScanDamageTarget(C_CSPlayerPawn* player) {
	ScannedTarget_t target;
	target.player = player;
	target.minimum_damage = CalcMinDamage(player);
	if (PrepareTargetDamage(player) < target.minimum_damage)
		return target;

	for (auto record : SelectRecords(player, false)) {
		CLagRecordScope scope(record, processing_tick);
		if (scope && ScanRecord(record, &target))
			break;
	}
	target.has_damage = target.best_point.record && target.best_point.damage >= target.minimum_damage;
	bool previous_stop = stop_target.best_point.record && stop_target.best_point.damage >= stop_target.minimum_damage;
	if (target.best_point.record && (!stop_target.player || (target.has_damage && !previous_stop) || (target.has_damage == previous_stop && target.best_point.damage > stop_target.best_point.damage)))
		stop_target = target;
	return target;
}

void CRagebot::ScanTargets(bool full_scan) {
	show_points = Config::Get<bool>(Vars::Ragebot::ShowAimpoints);
	if (show_points) {
		std::lock_guard lock(debug_mutex);
		debug_points.clear();
		debug_stamp_ms = NowMs();
		debug_revision++;
	}

	scanned_targets.clear();
	scanned_targets.reserve(target_count);
	bool sticky = settings.target_selection == 0 && ctx.cmd->command_number - last_target_shot < 256;
	if (sticky) {
		auto end = targets.begin() + target_count;
		auto previous = std::find_if(targets.begin(), end, [&](const Target_t& target) {
			return target.player->GetHandle().ToInt() == last_target_handle;
		});
		if (previous != end)
			std::rotate(targets.begin(), previous, previous + 1);
	}
	bool health_order = full_scan && settings.target_selection == 3 && !show_points && settings.delay_shot <= 0;
	if (health_order) {
		std::stable_sort(targets.begin(), targets.begin() + target_count, [](const Target_t& first, const Target_t& second) {
			return first.player->m_iHealth() < second.player->m_iHealth();
		});
	}

	for (auto i{ 0uz }; i < target_count; ++i) {
		auto target = full_scan ? ScanTarget(targets[i].player) : ScanDamageTarget(targets[i].player);
		bool keep = full_scan && sticky && target.player->GetHandle().ToInt() == last_target_handle && target.best_point.record;
		scanned_targets.push_back(target);
		if (keep || (!full_scan && target.has_damage) || (health_order && target.best_point.record))
			break;
	}
}

ScannedTarget_t* CRagebot::SelectTarget() {
	ScannedTarget_t* best = nullptr;
	Vector view = Vector(ctx.cmd->pb.base().viewangles().x(), ctx.cmd->pb.base().viewangles().y(), 0.f);
	auto fov = [&](const Vector& angle) {
		Vector delta = angle - view;
		return std::hypot(std::remainder(delta.x, 360.f), std::remainder(delta.y, 360.f));
	};
	float required = required_hitchance;

	for (auto& target : scanned_targets) {
		if (!target.best_point.record || target.best_point.dont_shoot || target.best_point.damage < target.minimum_damage || target.hitchance < required)
			continue;

		if (target.player->GetHandle().ToInt() == last_target_handle && ctx.cmd->command_number - last_target_shot < 256 && settings.target_selection == 0)
			return &target;

		bool better = !best;
		if (best) {
			switch (settings.target_selection) {
			case 1:
				better = (target.best_point.point - eye_position).LengthSqr() < (best->best_point.point - eye_position).LengthSqr();
				break;
			case 2:
				better = fov(target.angle) < fov(best->angle);
				break;
			case 3:
				better = target.player->m_iHealth() < best->player->m_iHealth();
				break;
			default:
				better = target.best_point.damage > best->best_point.damage;
				break;
			}
		}

		if (better)
			best = &target;
	}
	return best;
}

void CRagebot::Run(CCSGOInput* input) {
	trace_record_count = 0;
	trace_record_lookup.clear();
	collect_stop_traces = false;
	hitchance_record = nullptr;

	if (!input || !ctx.cmd || !ctx.controller || !ctx.pawn || !ctx.pawn->Alive() || !ctx.weapon || !ctx.weapon_data || !Config::Get<bool>(Vars::Ragebot::Enable)) {
		Reset();
		return;
	}

	const bool aimpoints_enabled = Config::Get<bool>(Vars::Ragebot::ShowAimpoints);
	if (!aimpoints_enabled) {
		std::lock_guard lock(debug_mutex);
		debug_points.clear();
		debug_revision++;
	}
	
	int weapon_id = ctx.weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
	settings = GetWeaponSettings(weapon_id);

	show_points = aimpoints_enabled && (ctx.cmd->command_number % Vars::Ragebot::TicksPerAimpoint) == 0;

	required_hitchance = settings.hitchance * 0.01f;
	bool grounded = (ctx.pawn->m_fFlags() & EnginePrediction->m_fFlags & FL_ONGROUND) != 0;
	if (!settings.strict_hitchance && weapon_id == WEAPON_SSG08 && !grounded)
		required_hitchance *= 0.8f;

	if (!settings.stop_mode)
		stop_command = -1;

	ResetAttack(input);

	if ((ctx.pawn->m_fFlags() & FL_FROZEN) || ctx.pawn->m_bIsDefusing())
		return;

#if defined(_DEBUG) || defined(DEBUG_RAGEBOT)
	if (ctx.cmd->command_number % 64 == 0)
		Logger->Log(xs("[Ragebot] cmd=%d clip=%d weapon_ready=%d spread_valid=%d hitchance=%.0f auto_fire=%d attack=%d"),
			ctx.cmd->command_number,
			ctx.weapon ? ctx.weapon->m_iClip1() : -1,
			WeaponReady() ? 1 : 0,
			EnginePrediction->WeaponAccuracyValid() ? 1 : 0,
			settings.hitchance,
			settings.automatic_fire ? 1 : 0,
			ctx.cmd->button_state.held & IN_ATTACK ? 1 : 0);
#endif

	this->AutoRevolver();

	eye_position = ctx.eye_position;
	if (!eye_position.Valid())
		return;

	range_squared = ctx.weapon_data->m_flRange() * ctx.weapon_data->m_flRange();
	fov_cosine = std::cos(settings.maximum_fov * Math::degs);
	processing_tick = LagCompensation->GetProcessingTick();
	LagCompensation->ExtrapolateRecords(processing_tick);
	Math::Angle2Vectors(input->current_move_data.viewangles, &view_direction);

	if (weapon_id == WEAPON_TASER) {
		if (settings.zeusbot)
			Zeusbot(input);

		return;
	}

	if (ctx.weapon_data->m_WeaponType() == WEAPONTYPE_KNIFE) {
		if (settings.knifebot)
			Knifebot(input);

		return;
	}

	if (ctx.weapon_data->m_WeaponType() < WEAPONTYPE_PISTOL || ctx.weapon_data->m_WeaponType() > WEAPONTYPE_MACHINEGUN ||
		ctx.weapon->m_iClip1() <= 0 || ctx.weapon->m_bInReload() || (!settings.automatic_fire && !(ctx.cmd->button_state.held & IN_ATTACK)))
		return;

	if (weapon_id == WEAPON_REVOLVER && settings.automatic_fire && !Config::Get<bool>(Vars::Ragebot::PrepareRevolver)) {
		ctx.cmd->button_state.SetButtonState(IN_ATTACK2, CInButtonState::in_button_up);
		ctx.cmd->pb.clear_attack2_start_history_index();
		CanFire(input);
	}

	bool needs_scope = settings.automatic_scope && ctx.weapon_data->m_nZoomLevels() > 0 && !ctx.pawn->m_bIsScoped();
	bool full_scan = show_points || settings.delay_shot > 0 || (WeaponReady() && !needs_scope);
	if (!full_scan && !settings.stop_mode && !needs_scope)
		return;

	scan_bullets = ctx.weapon_data->m_nNumBullets();
	if (scan_bullets < 1 || scan_bullets > 16 || (full_scan && !CalcSpreadValues()))
		return;

	CAutoWall::Batch_t batch;
	FindTargets();
	CollectTargetsBySlots();
	stop_target = {};
	Vector stop_velocity = ctx.pawn->m_vecVelocity();
	stop_velocity.z = 0.f;
	collect_stop_traces = full_scan && settings.CanPredictStop((ctx.pawn->m_fFlags() & FL_ONGROUND) != 0, stop_velocity.LengthSqr());

	ScanTargets(full_scan);

	if (settings.stop_mode) {
		if (this->CanAutoStop() && (!stop_target.best_point.record || stop_target.best_point.damage < stop_target.minimum_damage)) {
			std::sort(trace_record_lookup.begin(), trace_record_lookup.end(), [](const auto& first, const auto& second) {
				return std::less<const LagRecord*>{}(first.first, second.first);
			});
			stop_target = SelectStopTarget();
		}

		AutoStop(stop_target.player ? &stop_target : nullptr);
	}

	if (!ctx.weapon->CanShoot())
		return;

	if (ShouldScope()) {
		CanFire(input);
		return;
	}

	if (!full_scan)
		return;

	auto target = SelectTarget();
	if (!target)
		return;

	if (!CanFire(input))
		return;

	if (Fire(target, input)) {
		last_target_handle = target->player->GetHandle().ToInt();
		last_target_shot = ctx.cmd->command_number;
		Movement->PeekAssistState.pending_shot_return = true;

		LogShot(ctx.cmd->command_number, GlobalVars->tick_count, target->player,
			target->best_point.point, target->angle, target->best_point.damage, target->hitchance, true);
		MarkShotHit(ctx.cmd->command_number);
	}
}

void CRagebot::PrepareCommand() {
	if (!Config::Get<bool>(Vars::Ragebot::Enable) || !Config::Get<bool>(Vars::Ragebot::DuckPeek) || !ctx.pawn || !ctx.pawn->Alive() ||
		!ctx.weapon || !ctx.weapon_data || ctx.weapon_data->m_WeaponType() < WEAPONTYPE_PISTOL || ctx.weapon_data->m_WeaponType() > WEAPONTYPE_MACHINEGUN ||
		ctx.pawn->m_nActualMoveType() != MOVETYPE_WALK || !(ctx.pawn->m_fFlags() & FL_ONGROUND) || (ctx.pawn->m_fFlags() & FL_FROZEN) ||
		(ctx.cmd->button_state.held & (IN_JUMP | IN_USE))) {
		this->duck_peek_active = false;
		return;
	}

	if (!this->duck_peek_active)
		this->duck_peek_height = ctx.pawn->GetEyePosition().z - ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin().z;

	this->duck_peek_active = true;
	ctx.cmd->button_state.SetButton(IN_DUCK, !this->WeaponReady(1));
}

void CRagebot::OnPresent() {
	if (!Config::Get<bool>(Vars::Ragebot::ShowAimpoints))
		return;

	if (NowMs() - debug_stamp_ms.load() > 400)
		return;

	DrawShotTelemetry();

	static thread_local std::vector<DebugPoint_t> Points;
	static thread_local uint64_t seen_revision = ~0ull;
	if (seen_revision != debug_revision.load()) {
		std::lock_guard Lock(debug_mutex);
		Points.assign(debug_points.begin(), debug_points.end());
		seen_revision = debug_revision.load();
	}

	auto* draw_list = ImGui::GetBackgroundDrawList();
	constexpr size_t maxPoints = 600;
	size_t drawnPoints = 0;

	const DebugPoint_t* hovered = nullptr;
	float hover_distance = 36.f;
	for (const auto& point : Points) {
		if (drawnPoints >= maxPoints)
			break;

		auto screen = Draw->WorldToScreen(point.point);
		if (screen.x < 0.f || screen.y < 0.f)
			continue;
		
		drawnPoints++;

		ImU32 color = IM_COL32(255, 255, 255, 220);
		if (point.hitchance_failed)
			color = IM_COL32(180, 120, 255, 220);
		else if (!point.scanned)
			color = IM_COL32(135, 135, 145, 160);
		else if (!point.hit)
			color = IM_COL32(240, 85, 85, 220);
		else if (point.damage < point.minimum_damage)
			color = IM_COL32(255, 175, 70, 220);

		if (point.selected)
			color = IM_COL32(95, 230, 130, 255);

		ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(screen.x, screen.y), point.selected ? 3.f : 2.f, color);

		if (Draw->opened) {
			const auto& mouse = ImGui::GetIO().MousePos;
			float distance = (mouse.x - screen.x) * (mouse.x - screen.x) + (mouse.y - screen.y) * (mouse.y - screen.y);
			if (distance < hover_distance) {
				hovered = &point;
				hover_distance = distance;
			}
		}
	}

	if (hovered) {
		if (!hovered->scanned && hovered->hitchance_failed)
			ImGui::SetTooltip(xs("Hit chance requirement not met"));
		else if (!hovered->scanned)
			ImGui::SetTooltip(xs("Not scanned: a better point already qualifies"));
		else if (!hovered->hit)
			ImGui::SetTooltip(xs("No matching hit"));
		else if (hovered->hitchance >= 0.f)
			ImGui::SetTooltip(xs("Damage: %.2f / %.0f\nHit chance: %s%.0f%%"), hovered->damage, hovered->minimum_damage, hovered->hitchance_failed ? "<= " : ">= ", hovered->hitchance * 100.f);
		else
			ImGui::SetTooltip(xs("Damage: %.2f / %.0f"), hovered->damage, hovered->minimum_damage);
	}
}

void CRagebot::DoublePrecision() {
	cvar(sv_late_commands_allowed);

	if (!Config::Get<bool>(Vars::Ragebot::DoublePrecision))
		return;

	precision_enabled.store((!sv_late_commands_allowed || sv_late_commands_allowed->GetInt() > 1));
}

float CRagebot::AdjustCommandTime(void* client, int command, double time, float interval, float adjustment) const {
	if (!precision_enabled.load() || command <= 0 || !std::isfinite(time) || !std::isfinite(interval) || interval <= 0.f)
		return adjustment;

	struct CommandTiming_t {
		int command;
		char pad0[20];
		double time;
		float latency;
		char pad1[4];
		float margin;
		char pad2[12];
	};
	static_assert(sizeof(CommandTiming_t) == 56);

	auto data = static_cast<const std::uint8_t*>(client);
	auto history = reinterpret_cast<const CommandTiming_t*>(data + 2897320);
	int first_command = std::max(1, std::max(command - 256, *reinterpret_cast<const int*>(data + 2897280) + 1));
	std::array<float, 8> margins;
	int count = 0;
	for (int index = command - 1; index >= first_command && count < static_cast<int>(margins.size()); --index) {
		const auto& sample = history[index & 255];
		if (sample.command != index || !std::isfinite(sample.time) || sample.time <= 0. || !std::isfinite(sample.latency) || sample.latency < 0.f || !std::isfinite(sample.margin) || std::fabs(sample.margin) >= 1.f)
			continue;

		double age = time - sample.time;
		if (age < 0. || age > sample.latency + interval * margins.size())
			continue;

		margins[count++] = static_cast<float>((command - index) * interval - age + sample.margin);
	}

	if (count < 4)
		return adjustment;

	std::sort(margins.begin(), margins.begin() + count);
	float margin = (margins[(count - 1) / 2] + margins[count / 2]) * 0.5f;
	float error = margin + interval * 0.5f;
	return std::clamp(error / static_cast<float>(margins.size()), -interval * 0.05f, interval * 0.05f);
}

void CRagebot::Reset() {
	this->duck_peek_active = false;

	trace_record_count = 0;
	trace_record_lookup.clear();
	collect_stop_traces = false;
	hitchance_record = nullptr;
	processing_tick = -1;
	hitchance_cache.Clear();
	selected_records.clear();
	stop_command = -1;
	stop_weapon = -1;
	last_early_stop = -1;
	applied_stop_command = -1;
	predicted_stop_command = -1;
	stop_early_only = false;
	stop_input = {};
	stop_subtick_input.clear();
	revolver_weapon = -1;
	revolver_command = -1;
	next_revolver_cock = 0;
	last_revolver_spawn_time = 0.f;
	last_target_handle = -1;
	last_target_shot = -1;
	last_fire_command = -1;
	last_fire_position = {};
	last_fire_angle = {};
	target_count = 0;
	scanned_targets.clear();
	delayed_ticks.clear();

	std::lock_guard lock(debug_mutex);
	debug_points.clear();
}

float CRagebot::CalcTraceRange(const LagRecord& record) const {
	float range = ctx.weapon_data->m_flRange();
	if (record.geometry.empty())
		return range;

	float distance = 0.f;
	for (const auto& hitbox : record.geometry) {
		if (!hitbox.valid)
			return range;

		distance = std::max(distance, (hitbox.center - eye_position).Length() + std::sqrt(hitbox.bound_radius_squared) + 1.f);
	}
	return std::min(range, distance);
}

void CRagebot::PrepareScan(LagRecord* record) {
	cvar(mp_damage_headshot_only);
	scan_hitboxes.clear();
	scan_damage_limit = 0.f;
	scan_range = CalcTraceRange(*record);
	scan_hitboxes.reserve(record->hitboxes.size());
	float eye_magnitude = std::fabs(eye_position.x) + std::fabs(eye_position.y) + std::fabs(eye_position.z);
	for (auto i{ 0uz }; i < record->hitboxes.size(); ++i) {
		const auto& data = record->hitboxes[i];
		int group = HitboxGroup(data);
		if (group < 0 || !settings.hitboxes[group] || (group == 0 && settings.force_body_aim) || (mp_damage_headshot_only->GetBool() && group != 0))
			continue;

		auto& transform = record->hitbox_transforms[i];
		const auto& geometry = record->geometry[i];
		ScanHitbox_t hitbox;
		hitbox.index = static_cast<int>(i);
		hitbox.group = group;
		hitbox.first = geometry.first;
		hitbox.last = geometry.last;
		hitbox.center = geometry.center;
		hitbox.relative_center = hitbox.center - eye_position;
		hitbox.direction = hitbox.relative_center;
		float distance = hitbox.direction.Normalize();
		hitbox.radius = data.radius * std::fabs(transform.array[0][3]);
		hitbox.damage_limit = maximum_damage[data.hitgroup] * scan_bullets;
		scan_damage_limit = std::max(scan_damage_limit, hitbox.damage_limit);

		float rotation_length = 0.f;
		for (float value : transform.array[1])
			rotation_length += value * value;
		bool valid_box = data.shape_type != 0 || (data.mins.x <= data.maxs.x && data.mins.y <= data.maxs.y && data.mins.z <= data.maxs.z);

		float bound_radius = (hitbox.last - hitbox.first).Length() * 0.5f;
		if (data.shape_type == 1 || data.shape_type == 2)
			bound_radius += hitbox.radius;

		float center_magnitude = std::fabs(hitbox.center.x) + std::fabs(hitbox.center.y) + std::fabs(hitbox.center.z);
		float padding = 0.01f + 32.f * FLT_EPSILON * (center_magnitude + eye_magnitude + bound_radius);
		bound_radius = bound_radius * 1.001f + padding;
		hitbox.bound_radius_squared = bound_radius * bound_radius;
		if (!data.geometry_valid || !valid_box || data.shape_type > 2 || !hitbox.center.Valid() || std::fabs(rotation_length - 1.f) > 0.0001f || std::fabs(transform.array[0][3]) <= 0.000001f || hitbox.radius < 0.f)
			hitbox.bound_radius_squared = -1.f;
		else if (distance > bound_radius) {
			hitbox.cone_sine = bound_radius / distance;
			hitbox.cone_cosine = std::sqrt(std::max(0.f, 1.f - hitbox.cone_sine * hitbox.cone_sine));
		}

		scan_hitboxes.push_back(hitbox);
	}

	std::sort(scan_hitboxes.begin(), scan_hitboxes.end(), [](const ScanHitbox_t& first, const ScanHitbox_t& second) {
		return first.group != second.group ? first.group < second.group : first.index < second.index;
	});
}

float CRagebot::HitboxDamageLimit(const ScanHitbox_t& source) const {
	float damage = 0.f;
	for (const auto& hitbox : scan_hitboxes) {
		if (hitbox.damage_limit <= damage)
			continue;

		float overlap = source.cone_cosine * hitbox.cone_cosine - source.cone_sine * hitbox.cone_sine;
		if (source.cone_cosine < 0.f || hitbox.cone_cosine < 0.f || RageHitbox_t::Dot(source.direction, hitbox.direction) + 0.00001f >= overlap) {
			damage = hitbox.damage_limit;
			if (damage >= scan_damage_limit)
				return damage;
		}
	}
	return damage;
}

float CRagebot::PointDamageLimit(const Vector& direction, float range) const {
	float damage = 0.f;
	for (const auto& hitbox : scan_hitboxes) {
		if (hitbox.damage_limit <= damage)
			continue;

		float distance = std::clamp(RageHitbox_t::Dot(hitbox.relative_center, direction), 0.f, range);
		Vector closest = hitbox.relative_center - direction * distance;
		if (hitbox.bound_radius_squared < 0.f || closest.LengthSqr() <= hitbox.bound_radius_squared) {
			damage = hitbox.damage_limit;
			if (damage >= scan_damage_limit)
				return damage;
		}
	}
	return damage;
}

void CRagebot::AddMultipoint(AimPoints_t& points, const Vector& point, int hitbox) const {
	if (points.count == points.points.size())
		return;

	Vector direction = point - eye_position;
	float length_squared = direction.LengthSqr();
	if (!direction.Valid() || length_squared <= 0.f)
		return;

	for (auto i{ 0uz }; i < points.count; ++i) {
		Vector previous = points.points[i].point - eye_position;
		double cross_x = static_cast<double>(direction.y) * previous.z - static_cast<double>(direction.z) * previous.y;
		double cross_y = static_cast<double>(direction.z) * previous.x - static_cast<double>(direction.x) * previous.z;
		double cross_z = static_cast<double>(direction.x) * previous.y - static_cast<double>(direction.y) * previous.x;
		double cross_squared = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

		if ((point - points.points[i].point).LengthSqr() <= 0.00000001f || (RageHitbox_t::Dot(direction, previous) > 0.f && cross_squared <= static_cast<double>(length_squared) * previous.LengthSqr() * 1e-16))
			return;
	}
	points.points[points.count++] = {point, hitbox, true};
}

void CRagebot::PrepareMultipoints(LagRecord* record) {
	multipoints_ready = true;
	multipoint_points.resize(record->hitboxes.size());
	for (auto& points : multipoint_points)
		points.count = 0;

	Vector view = record->m_vecOrigin + (record->m_vecMins + record->m_vecMaxs) * 0.5f - eye_position;
	if (view.Normalize() <= 0.0001f)
		view = Vector(1.f, 0.f, 0.f);

	Vector right = view.Cross(Vector(0.f, 0.f, 1.f));
	if (right.Normalize() <= 0.0001f)
		right = view.Cross(Vector(1.f, 0.f, 0.f)).Normalized();

	Vector up = right.Cross(view).Normalized();
	std::array<std::array<AimPoint_t, 3>, 6> edges;
	for (auto& group : edges) {
		for (auto& point : group) {
			point.hitbox = -1;
		}
	}

	float scores[6][3];
	for (auto& group : scores)
		std::fill(std::begin(group), std::end(group), -FLT_MAX);

	cvar(mp_damage_headshot_only);
	for (auto i{ 0uz }; i < record->hitboxes.size(); ++i) {
		const auto& data = record->hitboxes[i];
		const auto& geometry = record->geometry[i];
		int group = HitboxGroup(data);
		if (!geometry.valid || group < 0 || !settings.hitboxes[group] || (group == 0 && settings.force_body_aim) || (mp_damage_headshot_only->GetBool() && group != 0))
			continue;

		auto& points = multipoint_points[i];
		points.points[points.count++] = {geometry.center, static_cast<int>(i)};
		if (!settings.multipoints[group])
			continue;

		ScanHitbox_t hitbox;
		hitbox.index = static_cast<int>(i);
		hitbox.group = group;
		hitbox.center = geometry.center;
		hitbox.direction = geometry.center - eye_position;
		hitbox.direction.Normalize();
		hitbox.matrix = record->hitbox_transforms[i].ToMatrix3x4();

		AimPoints_t candidates = points;
		GetMultipoints(record, hitbox, candidates);
		for (size_t index = 1; index < candidates.count; ++index) {
			const auto& point = candidates.points[index];
			Vector direction = point.point - eye_position;
			float depth = RageHitbox_t::Dot(direction, view);
			if (depth <= 0.0001f)
				continue;

			float side = RageHitbox_t::Dot(direction, right) / depth;
			float height = RageHitbox_t::Dot(direction, up) / depth;
			const float values[] = {side, -side, height};
			for (int edge = 0; edge < (group == 0 ? 3 : 2); ++edge) {
				if (values[edge] <= scores[group][edge])
					continue;

				scores[group][edge] = values[edge];
				edges[group][edge] = point;
			}
		}
	}

	for (int group = 0; group < 6; ++group) {
		for (int edge = 0; edge < (group == 0 ? 3 : 2); ++edge) {
			const auto& point = edges[group][edge];
			if (point.hitbox < 0)
				continue;

			AddMultipoint(multipoint_points[point.hitbox], point.point, point.hitbox);
		}
	}
}

void CRagebot::GetMultipoints(LagRecord* record, const ScanHitbox_t& hitbox, AimPoints_t& points) const {
	const auto& geometry = record->geometry[hitbox.index];
	const auto& data = record->hitboxes[hitbox.index];
	float scale = hitbox.group == 0 ? settings.point_scale : settings.body_point_scale;
	if (!geometry.valid || scale <= 0.f || data.shape_type > 2)
		return;

	scale = std::min(scale, 1.f) * 0.99f;

	Vector view = hitbox.direction;
	if (view.Normalize() <= 0.0001f)
		return;

	Vector right = view.Cross(Vector(0.f, 0.f, 1.f));
	if (right.Normalize() <= 0.0001f)
		right = view.Cross(Vector(1.f, 0.f, 0.f)).Normalized();

	Vector up = right.Cross(view).Normalized();
	const Vector directions[] = {right, right * -1.f, up};
	for (int i = 0; i < (hitbox.group == 0 ? 3 : 2); ++i) {
		Vector point = geometry.center + (geometry.Contour(eye_position, view, directions[i]) - geometry.center) * scale;
		AddMultipoint(points, point, hitbox.index);
	}
}

void CRagebot::ReserveTargets(int count) {
	targets.fill({});
	target_count = 0;
	target_slot_count = 0;
	target_slots.fill({});
}

void CRagebot::InsertTarget(int slot, float score, int index) {
	if (slot < 0 || slot >= 3)
		return;

	auto& tslot = target_slots[slot];
	if (tslot.player != nullptr && target_slot_count >= 3)
		return;

	targets[target_count] = { nullptr, 0.f, 0.f, slot };
	auto& t = targets[target_count];
	target_count++;
	target_slot_count++;
}

void CRagebot::CollectTargetsBySlots() {
	target_slot_count = 0;
	target_slots.fill({});

	float slot_best[3] = { 0.f, 0.f, 0.f };
	int slot_idx[3] = { -1, -1, -1 };

	for (size_t i = 0; i < target_count; ++i) {
		auto& t = targets[i];
		if (!t.player || !t.player->Alive() || t.player == ctx.pawn || !t.player->Enemy(ctx.pawn) || t.player->m_bGunGameImmunity())
			continue;

		float score = 1.f / (t.distance_squared + 1.f) + t.fov * 0.001f;
		int best_slot = -1;
		for (int s = 0; s < 3; ++s) {
			if (score > slot_best[s]) {
				best_slot = s;
				break;
			}
		}
		if (best_slot < 0) continue;

		t.slot = best_slot;
		slot_best[best_slot] = score;
		slot_idx[best_slot] = static_cast<int>(i);
		target_slot_count++;
	}
}

void CRagebot::FindTargets() {
	target_count = 0;
	ReserveTargets(64);

	static constexpr float cull_margin = 48.f;
	const float cull_distance = settings.maximum_distance + cull_margin;
	const float cull_squared = cull_distance * cull_distance;

	for (auto i{ 0uz }; i < EntityList->player_count && target_count < targets.size(); i++) {
		auto& entity = EntityList->players[i];

		if (!entity.Fill())
			continue;

		auto player = entity.pawn;
		if (!player)
			continue;

		auto node = player->m_pGameSceneNode();
		if (!node || player == ctx.pawn || !player->Alive() || !player->Enemy(ctx.pawn) || player->m_bGunGameImmunity())
			continue;

		const Vector origin = node->m_vecAbsOrigin();
		if (settings.maximum_distance > 0.f) {
			Vector center = origin;
			center.z += 36.f;
			if ((center - eye_position).LengthSqr() > cull_squared)
				continue;
		}

		if (std::any_of(targets.begin(), targets.begin() + target_count, [&](const Target_t& target) { return target.player == player; }))
			continue;

		Vector view = Vector(ctx.cmd->pb.base().viewangles().x(), ctx.cmd->pb.base().viewangles().y(), 0.f);
		Vector delta = origin - eye_position;
		float dist = delta.Length();
		float fov = std::hypot(std::remainder(view.x - delta.Normalized().x * 100.f, 360.f), std::remainder(view.y - delta.Normalized().y * 100.f, 360.f));

		targets[target_count++] = { player, dist * dist, fov, -1 };
	}

	std::sort(targets.begin(), targets.begin() + target_count, [](const Target_t& first, const Target_t& second) {
		return first.distance_squared < second.distance_squared;
	});

	CollectTargetsBySlots();
}

bool CRagebot::ScanRecord(LagRecord* record, ScannedTarget_t* target) {
	if (!record || record->hitboxes.empty() || record->hitboxes.size() != record->hitbox_transforms.size())
		return false;

	if (target->best_point.record && target->best_point.damage >= target->minimum_damage)
		return true;

	PrepareScan(record);
	if (scan_damage_limit < target->minimum_damage)
		return false;

	auto& points = scan_points;
	auto& results = scan_results;
	for (int phase = 0; phase < 2; ++phase) {
		if (phase)
			PrepareMultipoints(record);
		points.clear();
		results.clear();
		scan_indices.clear();

		for (auto& hitbox : scan_hitboxes) {
			if (!record->geometry[hitbox.index].valid)
				continue;
			float hitbox_limit = HitboxDamageLimit(hitbox);
			if (hitbox_limit < target->minimum_damage || hitbox_limit <= target->best_point.damage)
				continue;

			AimPoints_t center;
			center.count = 1;
			center.points[0] = {record->geometry[hitbox.index].center, hitbox.index};
			const auto& generated = phase ? multipoint_points[hitbox.index] : center;

			for (size_t i = phase ? 1 : 0; i < generated.count; ++i) {
				const auto& point = generated.points[i];
				Vector direction = point.point - eye_position;
				float distance_squared = direction.LengthSqr();
				if (!direction.Valid() || distance_squared <= 0.f || distance_squared > range_squared)
					continue;

				direction /= std::sqrt(distance_squared);
				if (settings.maximum_fov < 180.f && direction.Dot(view_direction) < fov_cosine)
					continue;

				float damage_limit = PointDamageLimit(direction, ctx.weapon_data->m_flRange());
				if (damage_limit < target->minimum_damage || damage_limit <= target->best_point.damage)
					continue;

				points.push_back(point);
			}
		}
		results.resize(points.size());

		auto read_result = [&](size_t index, const CAutoWall::Result_t& bullet, bool hit) {
			const auto& point = points[index];
			if (!hit || bullet.hitgroup != record->hitboxes[point.hitbox].hitgroup)
				return;

			int hitbox = point.hitbox;
			if (record->hitboxes[hitbox].descriptor != bullet.hitbox_data) {
				auto found = std::find_if(record->hitboxes.begin(), record->hitboxes.end(), [&](const LagHitbox_t& data) {
					return data.descriptor == bullet.hitbox_data;
				});
				if (found == record->hitboxes.end())
					return;

				hitbox = static_cast<int>(found - record->hitboxes.begin());
			}

			int group = HitboxGroup(record->hitboxes[hitbox]);
			if (group < 0 || !settings.hitboxes[group])
				return;

			auto& result = results[index];
			result.record = record;
			result.point = point.point;
			result.hitbox = hitbox;
			result.multipoint = point.multipoint;
			result.damage = bullet.damage * scan_bullets;
			result.hitgroup = bullet.hitgroup;
			result.penetrated = bullet.penetrated;
		};

		const TraceRecord_t* cached = nullptr;
		auto cached_record = std::lower_bound(trace_record_lookup.begin(), trace_record_lookup.end(), record, [](const auto& entry, const LagRecord* value) {
			return std::less<const LagRecord*>{}(entry.first, value);
		});
		if (cached_record != trace_record_lookup.end() && cached_record->first == record && trace_records[cached_record->second].range == scan_range &&
			(trace_records[cached_record->second].source - eye_position).Zero())
			cached = &trace_records[cached_record->second];

		for (auto i{ 0uz }; i < points.size(); ++i) {
			if (cached && !cached->points.empty()) {
				std::array<float, 3> position = {points[i].point.x, points[i].point.y, points[i].point.z};
				const auto end = cached->points.data() + cached->points.size();
				auto found = std::lower_bound(cached->points.data(), end, position, [](const TracePoint_t& point, const std::array<float, 3>& value) {
					return point.position < value;
				});
				if (found != end && found->position == position) {
					read_result(i, found->result, found->hit);
					if (results[i].record && results[i].damage >= target->minimum_damage) {
						target->best_point = results[i];
						return true;
					}

					continue;
				}
			}

			scan_indices.push_back(i);
		}

		if (!scan_indices.empty()) {
			auto filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);
			auto target_filter = Physics->MakeFilter(target->player, 0x1C300B, 3, 15);
			filter.shape_attr.flags |= 2;
			filter.shape_attr.interacts_exclude |= 0x4000000000;
			filter.shape_attr.entity_ids_to_ignore[1] = target_filter.shape_attr.entity_ids_to_ignore[0];

			bool found_point = false;
			auto scan = [&](size_t slot) {
				if (found_point)
					return;

				size_t index = scan_indices[slot];
				CAutoWall::Result_t bullet;
				bool hit = AutoWall->Run(eye_position, points[index].point, target->player, bullet, settings.penetrate_walls, scan_range, &filter, &target_damage);
				read_result(index, bullet, hit);
				if (results[index].record && results[index].damage >= target->minimum_damage)
					found_point = true;
			};

			scan(0);
			for (size_t slot = 1; slot < scan_indices.size() && !found_point; ++slot)
				scan(slot);
		}

		for (const auto& result : results) {
			if (!result.record || result.damage <= target->best_point.damage)
				continue;

			target->best_point = result;
			if (result.damage >= target->minimum_damage)
				return true;
		}
	}
	return false;
}

ScannedTarget_t CRagebot::ScanStopTarget(C_CSPlayerPawn* player, bool history) {
	ScannedTarget_t target;
	target.player = player;
	target.minimum_damage = CalcMinDamage(player);
	if (PrepareTargetDamage(player) < target.minimum_damage)
		return target;

	bool newest = true;
	for (auto record : SelectRecords(player, false)) {
		if (newest) {
			newest = false;
			if (history)
				continue;
		} else if (!history) {
			break;
		}

		CLagRecordScope scope(record, processing_tick);
		if (!scope)
			continue;

		if (ScanRecord(record, &target))
			break;
	}
	return target;
}

ScannedTarget_t CRagebot::SelectStopTarget() {
	ScannedTarget_t best;
	for (bool history : {false}) {
		for (auto i{ 0uz }; i < target_count; ++i) {
			auto target = ScanStopTarget(targets[i].player, history);
			if (target.best_point.damage >= target.minimum_damage)
				return target;

			if (target.best_point.damage > best.best_point.damage)
				best = target;
		}
	}
	return best;
}

void CRagebot::RestoreAutoStop(CUserCommand* cmd) {
	if (!cmd || this->predicted_stop_command != cmd->command_number)
		return;

	auto base = cmd->pb.mutable_base();
	base->set_forwardmove(this->stop_input.x);
	base->set_leftmove(this->stop_input.y);
	for (auto i{ 0uz }; i < this->stop_subtick_input.size() && i < base->subtick_moves_size(); ++i) {
		auto step = base->mutable_subtick_moves(static_cast<int>(i));
		step->set_analog_forward_delta(this->stop_subtick_input[i].x);
		step->set_analog_left_delta(this->stop_subtick_input[i].y);
		step->set_yaw_delta(this->stop_subtick_input[i].z);
	}
	this->stop_subtick_input.clear();
	this->predicted_stop_command = -1;
	this->applied_stop_command = -1;
}

bool CRagebot::CanAutoStop(int tick_offset) const {
	if (!ctx.cmd || !ctx.pawn || !ctx.pawn->Alive() || !ctx.weapon || !ctx.weapon_data || !Config::Get<bool>(Vars::Ragebot::Enable) ||
		!this->settings.stop_mode || ctx.weapon_data->m_WeaponType() < WEAPONTYPE_PISTOL || ctx.weapon_data->m_WeaponType() > WEAPONTYPE_MACHINEGUN ||
		ctx.weapon->m_iClip1() <= 0 || ctx.weapon->m_bInReload() || (!this->settings.automatic_fire && !(ctx.cmd->button_state.held & IN_ATTACK)))
		return false;

	if (!(this->settings.stop_mode & STOP_BETWEEN_SHOTS) && !this->WeaponReady(tick_offset))
		return false;

	const bool grounded = (EnginePrediction->m_fFlags & FL_ONGROUND) != 0;
	const bool landed = !tick_offset && (this->settings.stop_mode & STOP_ON_LAND) && EnginePrediction->LandedThisCommand();
	if (!this->settings.CanStopMovement(grounded, landed) || ctx.pawn->m_nActualMoveType() != MOVETYPE_WALK ||
		(ctx.pawn->m_fFlags() & FL_FROZEN) || ctx.pawn->m_bIsDefusing())
		return false;

	if (!grounded || landed)
		return true;
	if (!ctx.pawn->m_hGroundEntity().Valid() || (ctx.cmd->button_state.held & IN_JUMP))
		return false;

	for (const auto& step : ctx.cmd->pb.base().subtick_moves()) {
		if ((step.button() & IN_JUMP) && step.pressed())
			return false;
	}
	return true;
}

void CRagebot::PrepareAutoStop() {
	if (!ctx.cmd || !ctx.pawn || !ctx.pawn->Alive() || !Config::Get<bool>(Vars::Ragebot::Enable) ||
		!ctx.weapon || !ctx.weapon_data || !CSGOInput || ctx.weapon->m_iClip1() <= 0 || ctx.weapon->m_bInReload())
		return;

	const int weapon_id = ctx.weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
	this->settings = this->GetWeaponSettings(weapon_id);
	if (!this->CanAutoStop(1)) {
		this->stop_command = -1;
		this->last_early_stop = -1;
		return;
	}

	this->eye_position = ctx.pawn->GetEyePosition();
	if (!this->eye_position.Valid())
		return;

	this->scan_bullets = ctx.weapon_data->m_nNumBullets();
	if (this->scan_bullets < 1 || this->scan_bullets > 16)
		return;

	this->range_squared = ctx.weapon_data->m_flRange() * ctx.weapon_data->m_flRange();
	this->fov_cosine = std::cos(this->settings.maximum_fov * Math::degs);
	this->processing_tick = LagCompensation->GetProcessingTick();
	Math::Angle2Vectors(CSGOInput->current_move_data.viewangles, &this->view_direction);

	this->trace_record_count = 0;
	this->trace_record_lookup.clear();
	this->collect_stop_traces = false;
	this->hitchance_record = nullptr;
	this->FindTargets();

	CAutoWall::Batch_t batch;
	for (auto i{ 0uz }; i < this->target_count; ++i) {
		auto target = this->ScanStopTarget(this->targets[i].player);
		if (this->AutoStop(&target, true))
			return;
	}
	this->AutoStop(nullptr, true);
}

bool CRagebot::AutoStop(ScannedTarget_t* target, bool prediction) {
	this->stop_command = -1;
	this->stop_early_only = false;
	if (!this->CanAutoStop(prediction ? 1 : 0)) {
		this->last_early_stop = -1;
		return false;
	}

	const int command = ctx.cmd->command_number;
	const int weapon = ctx.weapon->GetHandle().ToInt();
	if (!(this->settings.stop_mode & STOP_EARLY) || !(EnginePrediction->m_fFlags & FL_ONGROUND) ||
		this->stop_weapon != weapon || command < this->last_early_stop)
		this->last_early_stop = -1;
	this->stop_weapon = weapon;

	bool stop = target && target->best_point.record && target->best_point.damage >= target->minimum_damage;
	bool stop_early = false;
	Vector velocity = ctx.pawn->m_vecVelocity();
	velocity.z = 0.f;
	if (prediction && !stop && target && this->settings.CanPredictStop((ctx.pawn->m_fFlags() & FL_ONGROUND) != 0, velocity.LengthSqr())) {
		const Vector source = this->eye_position;
		this->eye_position += velocity * TICKS_TO_TIME(4);
		auto predicted = this->ScanStopTarget(target->player);
		if (!predicted.best_point.record || predicted.best_point.damage < predicted.minimum_damage)
			predicted = this->ScanStopTarget(target->player, true);
		this->eye_position = source;
		stop_early = predicted.best_point.record && predicted.best_point.damage >= predicted.minimum_damage;
	}

	if (stop_early)
		this->last_early_stop = command;

	this->stop_early_only = !stop && this->last_early_stop >= 0 && command - this->last_early_stop < 8;
	stop |= this->stop_early_only;
	if (stop)
		this->stop_command = command;

	return prediction ? stop : this->ApplyAutoStop();
}

bool CRagebot::ApplyAutoStop(int tick_offset) {
	if (!this->CanAutoStop(tick_offset)) {
		stop_command = -1;
		last_early_stop = -1;
		return false;
	}

	int command = ctx.cmd->command_number;
	if (stop_command < 0 || command < stop_command || command - stop_command > (tick_offset ? 1 : 0) || ctx.weapon->GetHandle().ToInt() != stop_weapon) {
		stop_command = -1;
		last_early_stop = -1;
		return false;
	}

	auto base = ctx.cmd->pb.mutable_base();

	if (!tick_offset && applied_stop_command == command)
		return true;

	cvar(sv_friction);
	cvar(sv_stopspeed);
	cvar(sv_accelerate);
	cvar(sv_maxspeed);
	cvar(sv_airaccelerate);
	const bool landed = !tick_offset && (settings.stop_mode & STOP_ON_LAND) && EnginePrediction->LandedThisCommand();
	const bool grounded = (EnginePrediction->m_fFlags & FL_ONGROUND) || landed;

	auto movement = ctx.pawn->m_pMovementServices();
	int mode = std::clamp(ctx.weapon->m_weaponMode(), 0, 1);
	float max_speed = std::min(ctx.weapon_data->m_flMaxSpeed()[mode], sv_maxspeed->GetFloat());

	if (movement->m_flMaxspeed() > 0.f)
		max_speed = std::min(max_speed, movement->m_flMaxspeed());

	float target_speed = max_speed * (settings.stop_speed / 100.f);
	if ((EnginePrediction->m_fFlags & FL_DUCKING) || (ctx.cmd->button_state.held & IN_DUCK))
		max_speed *= 0.34f;

	float stamina = std::clamp(1.f - movement->m_flStamina() * 0.01f, 0.f, 1.f);
	max_speed *= stamina * stamina;

	Vector velocity = landed ? ctx.pawn->m_vecVelocity() : EnginePrediction->m_vecVelocity;
	velocity.z = 0.f;

	float speed = velocity.Length2d();
	float friction = movement->m_flSurfaceFriction();
	float remaining = grounded ? std::max(0.f, speed - std::max(speed, sv_stopspeed->GetFloat()) * sv_friction->GetFloat() * friction * INTERVAL_PER_TICK) : speed;
	float acceleration = (grounded ? sv_accelerate : sv_airaccelerate)->GetFloat() * max_speed * friction * INTERVAL_PER_TICK;
	float strength = acceleration > 0.f ? std::clamp((remaining - target_speed) / acceleration, 0.f, 1.f) : 0.f;

	float yaw = base->viewangles().y() * Math::degs;
	Vector forward(std::cos(yaw), std::sin(yaw), 0.f);
	Vector left(-forward.y, forward.x, 0.f);
	if (this->stop_early_only && base->forwardmove() * velocity.Dot(forward) + base->leftmove() * velocity.Dot(left) < 0.f) {
		this->stop_command = -1;
		this->last_early_stop = -1;
		return false;
	}
	if (this->stop_early_only && strength == 0.f)
		return false;

	Vector direction = speed > 0.f ? velocity * (-strength / speed) : Vector();

	float forward_move = direction.Dot(forward);
	float left_move = direction.Dot(left);
	constexpr uint64_t mask = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_SPRINT;

	bool neutral = strength == 0.f && base->forwardmove() == 0.f && base->leftmove() == 0.f && !((ctx.cmd->button_state.held | ctx.cmd->button_state.toggle | ctx.cmd->button_state.scroll) & mask);
	for (const auto& step : base->subtick_moves()) {
		if (step.analog_forward_delta() != 0.f || step.analog_left_delta() != 0.f || (step.button() & mask))
			neutral = false;
	}

	if (neutral) {
		applied_stop_command = command;
		return true;
	}

	if (tick_offset) {
		stop_input = Vector(base->forwardmove(), base->leftmove(), 0.f);
		predicted_stop_command = command;
		stop_subtick_input.clear();
		if (!(EnginePrediction->m_fFlags & FL_ONGROUND)) {
			for (const auto& step : base->subtick_moves())
				stop_subtick_input.emplace_back(step.analog_forward_delta(), step.analog_left_delta(), step.yaw_delta());
		}
	}

	if (!(EnginePrediction->m_fFlags & FL_ONGROUND)) {
		for (auto& step : *base->mutable_subtick_moves()) {
			step.set_analog_forward_delta(0.f);
			step.set_analog_left_delta(0.f);
			step.set_yaw_delta(0.f);
		}
	}

	base->set_forwardmove(forward_move);
	base->set_leftmove(left_move);

	applied_stop_command = command;
	return true;
}

bool CRagebot::CalcSpreadValues() {
	if (!EnginePrediction->WeaponAccuracyValid() || EnginePrediction->WeaponAccuracyCommand() != ctx.cmd->command_number)
		return false;

	static auto random_seed = reinterpret_cast<CUniformRandomStreamState::Constructor_t>(GetProcAddress(static_cast<HMODULE>(Memory->Modules.Tier0.Handle()), xs("??0?$CUniformRandomStreamImpl@VCThreadNullMutex@@@@QEAA@H@Z")));
	static auto random_float = reinterpret_cast<CUniformRandomStreamState::RandomFloat_t>(GetProcAddress(static_cast<HMODULE>(Memory->Modules.Tier0.Handle()), xs("?RandomFloat@?$CUniformRandomStreamImpl@VCThreadNullMutex@@@@QEAAMMM@Z")));
	return random_seed && random_float && spread_cache.Update(EnginePrediction->WeaponInaccuracy(), EnginePrediction->WeaponSpread(), ctx.weapon_data->m_bIsRevolver(), random_seed, random_float);
}

void CRagebot::PrepareHitchance(const LagRecord& record) {
	if (hitchance_record == &record && std::memcmp(&hitchance_eye, &eye_position, sizeof(eye_position)) == 0)
		return;

	hitchance_rays.resize(record.geometry.size());
	double maximum_spread = std::sqrt(spread_cache.SortedRadiusSquared.back());
	for (auto i{ 0uz }; i < record.geometry.size(); ++i) {
		const auto& hitbox = record.geometry[i];
		if (hitbox.valid)
			hitbox.PrepareRay(eye_position, hitchance_rays[i], maximum_spread);
	}
	hitchance_record = &record;
	hitchance_eye = eye_position;
}

CHitchanceCache::Key CRagebot::HitchanceKey(const ScannedTarget_t* target, int desired) const {
	return CHitchanceCache::Key::Make(target->best_point.record->revision, spread_cache.Revision(),
		{eye_position.x, eye_position.y, eye_position.z}, {target->angle.x, target->angle.y, target->angle.z}, required_hitchance, desired, ctx.weapon_data->m_flRange());
}

bool CRagebot::CanReachHitchance(const ScannedTarget_t* target) {
	const auto& record = *target->best_point.record;
	const auto& selected = record.hitboxes[target->best_point.hitbox];
	if (required_hitchance <= 0.f)
		return true;

	int desired = HitboxGroup(selected);
	if (desired == 5)
		desired = 4;
	auto key = HitchanceKey(target, desired);
	float cached = 0.f;
	if (record.revision && hitchance_cache.Find(key, cached)) {
		return cached >= required_hitchance;
	}
	PrepareHitchance(record);
	Vector forward, right, up;
	Math::Angle2Vectors(target->angle, &forward, &right, &up);
	bool reachable = RageHitbox_t::CanReachHitchance(record.geometry, hitchance_rays, spread_cache.SortedRadiusSquared, forward, right, up, desired, required_hitchance);
	if (!reachable) {
		if (record.revision)
			hitchance_cache.Store(key, std::nextafter(required_hitchance, 0.f));
	}
	return reachable;
}

float CRagebot::CalcHitchance(const ScannedTarget_t* target) {
	if (required_hitchance <= 0.f)
		return 1.f;

	const auto& record = *target->best_point.record;
	const auto& selected = record.hitboxes[target->best_point.hitbox];
	int desired = HitboxGroup(selected);
	if (desired == 5)
		desired = 4;
	auto key = HitchanceKey(target, desired);
	float cached = 0.f;
	if (record.revision && hitchance_cache.Find(key, cached)) {
		return cached;
	}
	float result = CalcHitchanceUncached(target, desired);
	if (record.revision)
		hitchance_cache.Store(key, result);
	return result;
}

float CRagebot::CalcHitchanceUncached(const ScannedTarget_t* target, int desired) {
	const auto& record = *target->best_point.record;
	PrepareHitchance(record);
	Vector forward, right, up;
	Math::Angle2Vectors(target->angle, &forward, &right, &up);
	if (!RageHitbox_t::CanReachHitchance(record.geometry, hitchance_rays, spread_cache.SortedRadiusSquared, forward, right, up, desired, required_hitchance)) {
		return std::nextafter(required_hitchance, 0.f);
	}

	const auto& hitboxes = record.geometry;
	const auto& offsets = spread_cache.offsets;
	const Vector& eye = eye_position;
	float required = required_hitchance;
	float maximum_spread = spread_cache.maximum_radius;
	maximum_spread = maximum_spread * 1.001f + 0.00001f;
	float cone_cosine = 1.f / std::sqrt(1.f + maximum_spread * maximum_spread);
	float cone_sine = maximum_spread * cone_cosine;
	Vector axis = forward * (1.f / forward.Length());
	desired_hitboxes.clear();
	other_hitboxes.clear();
	desired_hitboxes.reserve(hitboxes.size());
	other_hitboxes.reserve(hitboxes.size());
	for (size_t index = 0; index < hitboxes.size(); ++index) {
		const auto& hitbox = hitboxes[index];
		if (!hitbox.valid)
			continue;

		const Vector& relative = hitchance_rays[index].relative_center;
		float depth = RageHitbox_t::Dot(relative, axis);
		float lateral = (relative - axis * depth).Length();
		float distance = depth * cone_cosine + lateral * cone_sine > 0.f ? std::max(0.f, lateral * cone_cosine - depth * cone_sine) : relative.Length();
		if (distance > hitchance_rays[index].bound_radius)
			continue;

		(hitbox.group == desired ? desired_hitboxes : other_hitboxes).push_back(index);
	}

	if (desired_hitboxes.empty())
		return 0.f;

	return RageHitboxPacket::Hitchance(hitboxes, hitchance_rays, desired_hitboxes, other_hitboxes, offsets, eye, forward, right, up, required, ctx.weapon_data->m_flRange());
}

bool CRagebot::WeaponReady(int tick_offset, bool check_revolver, Taia_t* result) const {
	static auto get_time = Memory->Modules.Client.Find(xs("40 53 48 83 EC 30 48 8B 05 ? ? ? ? 48 8B D9 0F 29 74 24 20 48 8D 4C 24 40 F3 0F 10 70 50")).As<Taia_t*(__fastcall*)(Taia_t*, uint32_t)>();
	if (!ctx.pawn || !ctx.weapon || !ctx.weapon_data || !ctx.pawn->Alive() || ctx.pawn->m_bIsDefusing() || ctx.pawn->m_iPlayerState() != 0)
		return false;

	if (ctx.pawn->m_bWaitForNoAttack())
		return false;

	auto services = ctx.pawn->m_pWeaponServices();
	if (!services || ctx.weapon->m_iClip1() <= 0)
		return false;

	if (ctx.weapon->m_bInReload())
		return false;

	if (ctx.pawn->m_iShotsFired() > 0 && (!ctx.weapon_data->m_bIsFullAuto() || ctx.weapon->m_bBurstMode()))
		return false;

	Taia_t time;
	auto group = ctx.weapon->m_pEntity()->m_worldGroupId();
	if (tick_offset) {
		static auto from_global = Memory->Modules.Client.Find(xs("40 53 48 83 EC 20 48 8B D9 45 85 C0 74 0B 89 11 48 8B C1 48 83 C4 20 5B C3")).As<int*(__fastcall*)(int*, int, uint32_t)>();
		from_global(&time.tick, ctx.controller->m_nTickBase() + tick_offset, group);
		time.fraction = 0.f;
	} else {
		get_time(&time, group);
	}
	auto next = Taia_t(ctx.weapon->m_nNextPrimaryAttackTick(), ctx.weapon->m_flNextPrimaryAttackTickRatio());

	if (time < next)
		return false;

	if (services->m_flNextAttack() > TICKS_TO_TIME(time.tick) + time.fraction * INTERVAL_PER_TICK)
		return false;

	if (result)
		*result = time;

	if (!check_revolver || !ctx.weapon_data->m_bIsRevolver())
		return true;

	auto ready = Taia_t(ctx.weapon->m_nPostponeFireReadyTicks(), ctx.weapon->m_flPostponeFireReadyFrac());
	return ctx.weapon->m_bIsHauledBack() && time >= ready;
}

void CRagebot::ResetAttack(CCSGOInput* input) {
	if (!ctx.cmd || !ctx.weapon || revolver_command != ctx.cmd->command_number - 1)
		return;

	for (const auto button : { IN_ATTACK, IN_ATTACK2 }) {
		auto state = button == IN_ATTACK ? input->button_attack : input->button_attack2;
		if (state && state->pressed)
			continue;

		ctx.cmd->button_state.SetButtonState(button, CInButtonState::in_button_up);
		if (button == IN_ATTACK)
			ctx.cmd->pb.clear_attack1_start_history_index();
		else
			ctx.cmd->pb.clear_attack2_start_history_index();
	}
}

void CRagebot::AutoRevolver() {
	if (!Config::Get<bool>(Vars::Ragebot::PrepareRevolver) || !ctx.pawn || !ctx.controller || !ctx.weapon || !ctx.cmd || !ctx.base_cmd ||
		ctx.weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex() != WEAPON_REVOLVER ||
		(ctx.cmd->button_state.held & (IN_ATTACK | IN_ATTACK2)) || ctx.weapon->m_iClip1() <= 0 || ctx.weapon->m_bInReload())
		return;

	const int tickbase = ctx.controller->m_nTickBase();
	auto services = ctx.pawn->m_pWeaponServices();
	if (!services || services->m_flNextAttack() > TICKS_TO_TIME(tickbase))
		return;

	const int weapon = ctx.weapon->GetHandle().ToInt();
	const float spawn_time = ctx.pawn->m_flLastSpawnTimeIndex();
	if (this->revolver_weapon != weapon || this->last_revolver_spawn_time != spawn_time) {
		this->revolver_weapon = weapon;
		this->last_revolver_spawn_time = spawn_time;
		this->next_revolver_cock = tickbase;
	}

	const int history_index = ctx.cmd->pb.input_history_size() - 1;
	if (history_index < 0)
		return;

	uint64_t button = IN_ATTACK;
	if (EnginePrediction->m_nPostponeFireReadyTicks > tickbase) {
		if (tickbase < this->next_revolver_cock)
			return;
	} else if (EnginePrediction->m_nNextSecondaryAttackTick - 3 > tickbase) {
		button = IN_ATTACK2;
	} else {
		this->next_revolver_cock = tickbase + 1;
		return;
	}

	ctx.cmd->button_state.SetButtonState(button, CInButtonState::in_button_down);
	if (button == IN_ATTACK)
		ctx.cmd->pb.set_attack1_start_history_index(history_index);
	else
		ctx.cmd->pb.set_attack2_start_history_index(history_index);

	auto state = ctx.base_cmd->mutable_buttons_pb();
	state->set_buttonstate1(ctx.cmd->button_state.held);
	state->set_buttonstate2(ctx.cmd->button_state.toggle);
	state->set_buttonstate3(ctx.cmd->button_state.scroll);
	this->revolver_command = ctx.cmd->command_number;
}

bool CRagebot::ShouldScope() const {
	if (!settings.automatic_scope || ctx.weapon_data->m_nZoomLevels() <= 0 || ctx.pawn->m_bIsScoped())
		return false;

	return std::any_of(scanned_targets.begin(), scanned_targets.end(), [](const ScannedTarget_t& target) {
		return target.has_damage;
	});
}

bool CRagebot::CanFire(CCSGOInput* input) {
	if (!settings.automatic_fire && !(ctx.cmd->button_state.held & IN_ATTACK))
		return false;

	Taia_t time;
	if (!WeaponReady(0, false, &time))
		return false;

	int history_index = ctx.cmd->pb.input_history_size() - 1;
	if (history_index < 0)
		return false;

	if (settings.automatic_scope && ctx.weapon_data->m_nZoomLevels() > 0 && !ctx.pawn->m_bIsScoped()) {
		auto next = Taia_t(ctx.weapon->m_nNextSecondaryAttackTick(), ctx.weapon->m_flNextSecondaryAttackTickRatio());

		if (time >= next) {
			ctx.cmd->button_state.SetButtonState(IN_ATTACK, CInButtonState::in_button_up);
			ctx.cmd->button_state.SetButton(IN_ATTACK2, true);
			ctx.cmd->pb.clear_attack1_start_history_index();
			ctx.cmd->pb.set_attack2_start_history_index(history_index);
		}

		return false;
	}

	auto ready = Taia_t(ctx.weapon->m_nPostponeFireReadyTicks(), ctx.weapon->m_flPostponeFireReadyFrac());
	if (!ctx.weapon_data->m_bIsRevolver() || (ctx.weapon->m_bIsHauledBack() && time >= ready))
		return true;

	if (Config::Get<bool>(Vars::Ragebot::PrepareRevolver))
		return false;

	if (!(input->button_attack && input->button_attack->pressed)) {
		revolver_weapon = ctx.weapon->GetHandle().ToInt();
		revolver_command = ctx.cmd->command_number;
	}

	ctx.cmd->button_state.SetButton(IN_ATTACK, true);
	ctx.cmd->pb.set_attack1_start_history_index(history_index);
	return false;
}

bool CRagebot::SetFireAngles(const Vector& angle, const Vector& recoil) {
	auto base = ctx.base_cmd;
	auto viewangles = base->mutable_viewangles();
	const float pitch = std::remainder(180.f - angle.x - recoil.x * 2.f, 360.f);
	const float yaw = std::remainder(angle.y + 180.f, 360.f);
	const float pitch_delta = viewangles->x() - pitch;
	const float yaw_delta = viewangles->y() - yaw;

	if (pitch_delta == 0.f && yaw_delta == 0.f)
		return true;

	if (!base->subtick_moves_size() || base->subtick_moves(base->subtick_moves_size() - 1).when() < 1.f) {
		float move_pitch = 0.f;
		float move_yaw = 0.f;
		for (const auto& step : base->subtick_moves()) {
			move_pitch += step.pitch_delta();
			move_yaw += step.yaw_delta();
		}

		auto step = Protobuf->AddSubtickMoveStep(ctx.cmd, 1.f);
		if (!step)
			return false;

		step->set_pitch_delta(-move_pitch);
		step->set_yaw_delta(-move_yaw);
	}

	auto step = base->mutable_subtick_moves(0);
	step->set_pitch_delta(step->pitch_delta() + pitch_delta);
	step->set_yaw_delta(step->yaw_delta() + yaw_delta);

	viewangles->set_x(pitch);
	viewangles->set_y(yaw);
	viewangles->set_z(0.f);
	return true;
}

bool CRagebot::Fire(ScannedTarget_t* target, CCSGOInput* input, int mode) {
	if (!target || !target->best_point.record || ctx.cmd->pb.input_history_size() == 0)
		return false;

	if (!LagCompensation->ValidRecord(target->best_point.record))
		return false;

	int render_tick = target->best_point.record->m_nSimulationTick;

	Vector recoil;
	if (ctx.weapon_data->m_WeaponType() != WEAPONTYPE_KNIFE)
		ctx.pawn->m_pAimPunchServices()->ComputeAimPunch(recoil, ctx.player_time, true);

	Vector angle = settings.remove_recoil ? target->angle - recoil : target->angle;
	angle.x = std::remainder(angle.x, 360.f);
	angle.y = std::remainder(angle.y, 360.f);
	angle.z = 0.f;

	auto* history = ctx.cmd->pb.mutable_input_history(0);

	auto viewangles = history->mutable_view_angles();
	viewangles->set_x(angle.x);
	viewangles->set_y(angle.y);
	viewangles->set_z(angle.z);

	history->set_render_tick_count(render_tick + 1);
	history->set_player_tick_count(ctx.player_time.tick);

	history->set_render_tick_fraction(0.f);
	history->set_player_tick_fraction(ctx.player_time.fraction);

	if (!this->SetFireAngles(angle, recoil))
		return false;

	cvar(weapon_accuracy_nospread);
	bool secondary = mode == 1 || (ctx.weapon_data->m_bIsRevolver() && weapon_accuracy_nospread->GetBool());

	if (ctx.weapon_data->m_bIsRevolver()) {
		this->revolver_weapon = ctx.weapon->GetHandle().ToInt();
		this->revolver_command = ctx.cmd->command_number;
		ctx.cmd->button_state.SetButtonState(secondary ? IN_ATTACK : IN_ATTACK2, CInButtonState::in_button_up);
		if (secondary)
			ctx.cmd->pb.clear_attack1_start_history_index();
		else
			ctx.cmd->pb.clear_attack2_start_history_index();
	}

	ctx.cmd->button_state.SetButton(secondary ? IN_ATTACK2 : IN_ATTACK, true);

	if (secondary)
		ctx.cmd->pb.set_attack2_start_history_index(0);
	else
		ctx.cmd->pb.set_attack1_start_history_index(0);

	if (!settings.silent_aim)
		input->current_move_data.viewangles = angle;

	last_fire_command = ctx.cmd->command_number;
	last_fire_position = eye_position;
	last_fire_angle = angle;

	return true;
}

void CRagebot::Zeusbot(CCSGOInput* input) {
	if (!WeaponReady())
		return;

	float inaccuracy = std::tan(EnginePrediction->WeaponInaccuracy());
	for (auto i{ 0uz }; i < EntityList->player_count; ++i) {
		auto& entity = EntityList->players[i];
		if (!entity.Fill())
			continue;

		auto player = entity.pawn;
		if (!player || player == ctx.pawn || !player->Alive() || !player->Enemy(ctx.pawn) || player->m_bGunGameImmunity())
			continue;

		for (auto record : SelectRecords(player)) {
			Vector center = record->m_vecOrigin + (record->m_vecMins + record->m_vecMaxs) * 0.5f;
			if ((center - eye_position).LengthSqr() > 155.f * 155.f)
				continue;

			CLagRecordScope scope(record);
			if (!scope)
				continue;

			for (size_t hitbox = 0; hitbox < record->hitboxes.size(); ++hitbox) {
				int group = record->hitboxes[hitbox].hitgroup;
				if (group != 2 && group != 3 && group != 4 && group != 5 && group != 8)
					continue;

				const auto& data = record->hitboxes[hitbox];
				Vector point = Math::TransformPoint((data.mins + data.maxs) * 0.5f, record->hitbox_transforms[hitbox].ToMatrix3x4());
				float distance = (point - eye_position).Length();
				float chance = inaccuracy > 0.f && distance > 0.f ? std::min(12.f / (distance * inaccuracy), 1.f) : 1.f;
				if (chance < 0.7f)
					continue;

				CAutoWall::Result_t bullet;
				bool hit = AutoWall->Run(eye_position, point, player, bullet, false);
				if (!hit)
					continue;

				ScannedTarget_t target;
				target.player = player;
				target.best_point.record = record;
				target.best_point.point = point;
				target.best_point.hitbox = static_cast<int>(hitbox);
				target.best_point.damage = bullet.damage;
				target.angle = CalculateAngle(point - eye_position);
				if (CanFire(input) && Fire(&target, input))
					return;
			}
		}
	}
}

void CRagebot::Knifebot(CCSGOInput* input) {
	if (!settings.automatic_fire && !(ctx.cmd->button_state.held & (IN_ATTACK | IN_ATTACK2)))
		return;

	for (auto i{ 0uz }; i < EntityList->player_count; ++i) {
		auto& entity = EntityList->players[i];
		if (!entity.Fill())
			continue;

		auto player = entity.pawn;
		if (!player || player == ctx.pawn || !player->Alive() || !player->Enemy(ctx.pawn) || player->m_bGunGameImmunity())
			continue;

		for (auto record : SelectRecords(player)) {
			Vector point = record->m_vecOrigin;
			point.z = std::clamp(eye_position.z, point.z, point.z + record->m_vecMaxs.z) - 0.01f;

			Vector delta = point - eye_position;
			if (delta.LengthSqr() > 6400.f)
				continue;

			Vector facing;
			Vector facing_angle(0.f, record->m_angEyeAngles.y, 0.f);

			Math::Angle2Vectors(facing_angle, &facing);
			Vector relative = record->m_vecOrigin - ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();
			relative.z = 0.f;

			bool backstab = relative.Normalized().Dot(facing) > 0.475f;
			float now = TICKS_TO_TIME(ctx.controller->m_nTickBase());
			float next = TICKS_TO_TIME(ctx.weapon->m_nNextPrimaryAttackTick()) + ctx.weapon->m_flNextPrimaryAttackTickRatio() * INTERVAL_PER_TICK;
			bool first = next + 0.4f < now;
			int light = backstab ? 90 : first ? 40 : 25;
			int heavy = backstab ? 180 : 65;
			bool stab = heavy >= player->m_iHealth() && light < player->m_iHealth();
			float range = stab ? 32.f : 48.f;
			int tick = stab ? ctx.weapon->m_nNextSecondaryAttackTick() : ctx.weapon->m_nNextPrimaryAttackTick();
			float fraction = stab ? ctx.weapon->m_flNextSecondaryAttackTickRatio() : ctx.weapon->m_flNextPrimaryAttackTickRatio();

			if (Taia_t(ctx.controller->m_nTickBase(), 0.f) < Taia_t(tick, fraction))
				continue;

			CLagRecordScope scope(record);
			if (!scope)
				continue;

			Vector end = eye_position + delta.Normalized() * range;
			Vector mins(-16.f, -16.f, -18.f), maxs(16.f, 16.f, 18.f);
			CRay ray{};
			ray.Init(mins, maxs);
			auto filter = Physics->MakeFilter(ctx.pawn, 0x1C3003, 3, 15);
			auto trace = Physics->MakeTrace();
			Physics->TraceRay(&ray, &eye_position, &end, &filter, &trace);
			if (trace.entity != player || trace.fraction >= 1.f) {
				CTraceData clipped;
				Vector clip_delta = delta.Normalized() * (range + 18.f);
				auto clip_filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);
				auto target_filter = Physics->MakeFilter(player, 0x1C300B, 3, 15);
				clip_filter.shape_attr.entity_ids_to_ignore[1] = target_filter.shape_attr.entity_ids_to_ignore[0];
				Physics->SetupTrace(&clipped, eye_position, clip_delta, clip_filter, 0);

				if (Physics->ClipTraceToPlayer(&clipped, player)) {
					for (int segment = 0; segment < clipped.filled_segment_count; ++segment) {
						const auto& hit = clipped.trace_segments[segment];
						if (hit.entity_handle != static_cast<uint32_t>(player->GetHandle().ToInt()) || hit.fraction >= trace.fraction)
							continue;

						trace.entity = player;
						trace.fraction = hit.fraction;
						trace.end_pos = eye_position + clip_delta * hit.fraction;
					}
				}
			}

			if (trace.entity != player || trace.fraction >= 1.f || (trace.end_pos - eye_position).LengthSqr() > range * range)
				continue;

			ScannedTarget_t target;
			target.player = player;
			target.best_point.record = record;
			target.best_point.point = point;
			target.angle = CalculateAngle(delta);
			if (Fire(&target, input, stab ? 1 : 0))
				return;
		}
	}
}

bool CRagebot::RefineShot(ScannedTarget_t* target, CCSGOInput* input) {
	if (!target || !target->best_point.record || !Config::Get<bool>(Vars::Ragebot::DoubleTap))
		return false;

	auto record = target->best_point.point;
	auto angle = target->angle;

	// уточнение по расчёту урона и хитчанса на смещённых точках
	std::array<Vector, 4> offsets = {{
		Vector(0.f, 0.f, 0.f),
		Vector(2.f, 0.f, 0.f),
		Vector(-2.f, 0.f, 0.f),
		Vector(0.f, 2.f, 0.f),
	}};

	float best_dmg = target->best_point.damage;
	Vector best_pt = record;
	Vector best_ang = angle;

	for (auto& off : offsets) {
		Vector pt = record + off;
		Vector dir = pt - eye_position;
		float dist = dir.Normalize();
		if (dist <= 0.f || dist > ctx.weapon_data->m_flRange())
			continue;

		CAutoWall::Result_t bullet;
		auto filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);
		bool hit = AutoWall->Run(eye_position, pt, target->player, bullet, settings.penetrate_walls, ctx.weapon_data->m_flRange(), &filter, &target_damage);
		if (!hit || bullet.damage < best_dmg)
			continue;

		best_dmg = bullet.damage;
		best_pt = pt;
		best_ang = CalculateAngle(pt - eye_position);
	}

	target->best_point.point = best_pt;
	target->best_point.damage = best_dmg;
	target->angle = best_ang;
	return true;
}

void CRagebot::PrepareRefine(ScannedTarget_t* target) {
	refine_enabled = Config::Get<bool>(Vars::Ragebot::DoubleTap);
	refine_step = Config::Get<float>(Vars::Ragebot::ReactionTime);
	if (refine_enabled && target) {
		refine_point = target->best_point.point;
		refine_tick = ctx.cmd ? ctx.cmd->command_number : 0;
	}
}

void CRagebot::LogShot(int command, int tick, C_CSPlayerPawn* target, const Vector& point, const Vector& angle, float damage, float hitchance, bool hit) {
	ShotRecord_t rec;
	rec.command = command;
	rec.tick = tick;
	rec.target_handle = target ? target->GetHandle().ToInt() : -1;
	rec.point = point;
	rec.angle = angle;
	rec.damage = damage;
	rec.hitchance = hitchance;
	rec.hit = hit;
	rec.logged = false;

	shot_history.push_back(rec);

	if (hit) {
		shots_hit++;
	} else {
		shots_missed++;
	}
	shots_fired++;

	// ограничиваем историю
	if (shot_history.size() > 256)
		shot_history.erase(shot_history.begin());
}

void CRagebot::MarkShotHit(int command) {
	for (auto it = shot_history.rbegin(); it != shot_history.rend(); ++it) {
		if (it->command == command && !it->hit) {
			it->hit = true;
			shots_hit++;
			shots_missed--;
			break;
		}
	}
}

void CRagebot::MarkShotMiss(int command) {
	for (auto it = shot_history.rbegin(); it != shot_history.rend(); ++it) {
		if (it->command == command && !it->hit) {
			it->hit = false;
			shots_hit--;
			shots_missed++;
			break;
		}
	}
}

void CRagebot::DrawShotTelemetry() {
	if (!Config::Get<bool>(Vars::Ragebot::ShowAimpoints))
		return;

	ImFont* font = Var->font.inter[0];
	if (!font)
		return;

	auto draw = [&](const char* text, ImVec2 pos, ImU32 col) {
		ImGui::GetForegroundDrawList()->AddText(font, font->LegacySize, pos, col, text);
	};

	int y = 10;
	draw(xs("Shots:"), ImVec2(10.f, (float)y), IM_COL32(255, 255, 255, 220));
	y += 14;
	draw(xs("Fired: "), ImVec2(20.f, (float)y), IM_COL32(200, 200, 200, 220));
	draw(std::to_string(shots_fired).c_str(), ImVec2(60.f, (float)y), IM_COL32(255, 255, 255, 255));
	y += 14;
	draw(xs("Hit:  "), ImVec2(20.f, (float)y), IM_COL32(100, 255, 100, 220));
	draw(std::to_string(shots_hit).c_str(), ImVec2(60.f, (float)y), IM_COL32(100, 255, 100, 255));
	y += 14;
	draw(xs("Miss: "), ImVec2(20.f, (float)y), IM_COL32(255, 80, 80, 220));
	draw(std::to_string(shots_missed).c_str(), ImVec2(60.f, (float)y), IM_COL32(255, 80, 80, 255));

	float hc = shots_fired > 0 ? (float)shots_hit / (float)shots_fired * 100.f : 0.f;
	y += 14;
	draw(xs("HC:   "), ImVec2(20.f, (float)y), IM_COL32(200, 200, 200, 220));
	char hc_buf[32];
	sprintf_s(hc_buf, "%.1f%%", hc);
	draw(hc_buf, ImVec2(60.f, (float)y), IM_COL32(255, 255, 100, 255));
}

CRagebot* Ragebot = new CRagebot;

float CRagebot::ResolveYaw(C_CSPlayerPawn* player, const LagRecord* record) {
	if (!settings.resolver || !player || !record)
		return record->m_angEyeAngles.y;

	int handle = player->GetHandle().ToInt();
	auto& state = resolver_states[handle];
	state.handle = handle;
	state.update_tick = ctx.cmd ? ctx.cmd->command_number : 0;

	bool moving = record->m_vecVelocity.Length2d() > 5.f;
	bool airborn = !(record->m_fFlags & FL_ONGROUND);
	state.moving = moving;
	state.airborn = airborn;

	float eye_yaw = record->m_angEyeAngles.y;
	state.last_eye_yaw = eye_yaw;

	// Mode 0: Normal - use eye yaw when moving, last resolved when standing still
	if (settings.resolver_mode == 0) {
		if (moving || airborn) {
			state.resolved_yaw = eye_yaw;
			state.last_yaw = eye_yaw;
		} else {
			state.resolved_yaw = state.last_yaw;
		}
	}
	// Mode 1: Aggressive - always use last known yaw, ignore eye angles
	else if (settings.resolver_mode == 1) {
		state.resolved_yaw = state.last_yaw;
	}
	// Mode 2: Delta - adjust based on miss history
	else if (settings.resolver_mode == 2) {
		if (state.shots_missed > 0) {
			state.resolved_yaw = Math::NormalizeYaw(state.last_yaw + state.miss_yaw_delta);
		} else {
			state.resolved_yaw = eye_yaw;
		}
	}

	return state.resolved_yaw;
}

int CRagebot::GetResolverHitboxPriority(const LagRecord* record, float resolved_yaw) {
	if (!record)
		return 0;

	Vector eye_pos = eye_position;
	Vector head_pos = record->m_vecOrigin + Vector(0.f, 0.f, 64.f);
	Vector to_head = head_pos - eye_pos;
	float head_yaw = Math::Rad2Deg(std::atan2(to_head.y, to_head.x));

	float yaw_delta = std::abs(Math::NormalizeYaw(resolved_yaw - head_yaw));

	// Priority 0: Head first (default)
	if (settings.resolver_hitbox_priority == 0) {
		if (yaw_delta < settings.resolver_yaw_range * 0.5f)
			return 0; // head
		if (yaw_delta < settings.resolver_yaw_range)
			return 1; // chest
		return 2; // body
	}
	// Priority 1: Chest first
	else if (settings.resolver_hitbox_priority == 1) {
		if (yaw_delta < settings.resolver_yaw_range * 0.5f)
			return 1; // chest
		if (yaw_delta < settings.resolver_yaw_range)
			return 0; // head
		return 2; // body
	}
	// Priority 2: Closest to crosshair (body first)
	else {
		return 2; // body
	}
}

void CRagebot::UpdateResolver(C_CSPlayerPawn* player, const LagRecord* record) {
	if (!settings.resolver || !player || !record)
		return;

	int handle = player->GetHandle().ToInt();
	auto& state = resolver_states[handle];

	state.last_yaw = record->m_angEyeAngles.y;
}


