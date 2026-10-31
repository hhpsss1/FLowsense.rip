#include "Autowall.h"
#include <shared.h>
#include "TraceResult.h"

static_assert(offsetof(CTraceSegment, body) == 0x10);
static_assert(offsetof(CTraceSegment, entity_handle) == 0x2C);
static_assert(offsetof(CTraceSegment, type) == 0x33);

struct TraceFilterCache_t {
	CTraceFilter filter;
	C_CSPlayerPawn* local = nullptr;
	C_CSPlayerPawn* target = nullptr;
	int depth = 0;
	bool valid = false;
};

static thread_local TraceFilterCache_t trace_filter_cache;

CAutoWall::Batch_t::Batch_t() {
	if (trace_filter_cache.depth++ == 0)
		trace_filter_cache.valid = false;

}

CAutoWall::Batch_t::~Batch_t() {
	if (--trace_filter_cache.depth == 0)
		trace_filter_cache.valid = false;

}

float CAutoWall::DamageUpperBound(const DamageScale_t& scale, int hitgroup) {
	return scale.Scale(scale.base_damage, hitgroup) + 0.001f;
}

bool CAutoWall::Trace(CTraceData& data, const Vector& start, const Vector& end, C_CSPlayerPawn* target,
					  int penetrations, float maximum_range, const CTraceFilter* prepared_filter) {
	Vector direction = end - start;
	if (direction.Normalize() <= 0.f)
		return false;

	CTraceFilter filter;
	auto& cache = trace_filter_cache;
	if (prepared_filter) {
		filter = *prepared_filter;
	} else if (cache.depth && cache.valid && cache.local == ctx.pawn && cache.target == target) {
		filter = cache.filter;
	} else {
		filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);
		filter.shape_attr.flags |= 2;
		filter.shape_attr.interacts_exclude |= 0x4000000000;
		if (target) {
			auto target_filter = Physics->MakeFilter(target, 0x1C300B, 3, 15);
			filter.shape_attr.entity_ids_to_ignore[1] = target_filter.shape_attr.entity_ids_to_ignore[0];
		}

		if (cache.depth) {
			cache.filter = filter;
			cache.local = ctx.pawn;
			cache.target = target;
			cache.valid = true;
		}

	}

	float range = ctx.weapon_data->m_flRange();
	if (maximum_range > 0.f)
		range = std::min(range, maximum_range);

	Physics->SetupTrace(&data, start, direction * range, filter, penetrations);
	if (target && !Physics->ClipTraceToPlayer(&data, target))
		return false;

	Physics->TraceBullet(&data, static_cast<float>(ctx.weapon_data->m_nDamage()), ctx.weapon_data->m_flPenetration(),
						 ctx.weapon_data->m_flRangeModifier(), ctx.pawn->m_iTeamNum(), penetrations);
	return data.trace_info.size > 0;
}

CAutoWall::DamageScale_t CAutoWall::PrepareDamage(C_CSPlayerPawn* target) {
	cvar(mp_damage_headshot_only);
	cvar(mp_damage_scale_ct_head);
	cvar(mp_damage_scale_t_head);
	cvar(mp_damage_scale_ct_body);
	cvar(mp_damage_scale_t_body);
	cvar(mp_teammates_are_enemies);
	cvar(mp_friendlyfire);
	cvar(ff_damage_reduction_bullets);

	DamageScale_t scale;
	scale.base_damage = static_cast<float>(ctx.weapon_data->m_nDamage());
	scale.head_only = mp_damage_headshot_only->GetBool();
	scale.immune = target->m_bGunGameImmunity();
	bool ct = target->m_iTeamNum() == 3;
	scale.head = (ct ? mp_damage_scale_ct_head : mp_damage_scale_t_head)->GetFloat();
	scale.body = (ct ? mp_damage_scale_ct_body : mp_damage_scale_t_body)->GetFloat();
	scale.headshot_multiplier = ctx.weapon_data->m_flHeadshotMultiplier();
	scale.armor_ratio = ctx.weapon_data->m_flArmorRatio() * 0.5f;
	scale.armor = target->m_ArmorValue();
	auto items = target->m_pItemServices();
	scale.helmet = items && items->m_bHasHelmet();
	if (target->m_iTeamNum() == ctx.pawn->m_iTeamNum() && !mp_teammates_are_enemies->GetBool()) {
		scale.friendly = mp_friendlyfire->GetBool() ? ff_damage_reduction_bullets->GetFloat() : 0.f;
		scale.friendly_clamp = scale.friendly > 0.f && scale.friendly < 0.02f;
	}
	return scale;
}

bool CAutoWall::Run(const Vector& start, const Vector& end, C_CSPlayerPawn* target, Result_t& result, bool penetrate,
					float maximum_range, const CTraceFilter* prepared_filter, const DamageScale_t* prepared_damage) {
	result = {};
	if (!target || (prepared_damage ? prepared_damage->immune : target->m_bGunGameImmunity()))
		return false;

	CTraceData data;
	if (!Trace(data, start, end, target, penetrate ? 4 : 0, maximum_range, prepared_filter))
		return false;

	const auto identity = target->m_pEntity();
	const auto impact = RageTrace::ReadTargetImpact(data, identity ? identity->Index() : UINT32_MAX, [&](const CTraceInfo& info) -> const CHitbox* {
		CGameTrace trace;
		Physics->GetTraceInfo(&data, &trace, info);
		return trace.entity == target ? trace.hitbox : nullptr;
	});
	if (!impact.hitbox)
		return false;

	result.hitbox_data = impact.hitbox;
	result.hitgroup = impact.hitbox->hitgroup;
	result.damage = prepared_damage ? prepared_damage->Scale(impact.damage, result.hitgroup) : PrepareDamage(target).Scale(impact.damage, result.hitgroup);
	result.penetrated = impact.penetrated;
	return result.damage > 0.f;
}

bool CAutoWall::CanHit(const Vector& start, const Vector& direction, float& damage) {
	damage = 0.f;
	CTraceData data;
	if (!Trace(data, start, start + direction, nullptr, 4))
		return false;

	for (auto i{ 0uz }; i < data.trace_info.size; i++) {
		const auto& info = data.trace_info[i];
		if (info.damage <= 0.f)
			return false;

		if (!info.IsPenetrationSegment())
			continue;

		damage = info.damage;
		return true;
	}

	return false;
}

CAutoWall* AutoWall = new CAutoWall();
