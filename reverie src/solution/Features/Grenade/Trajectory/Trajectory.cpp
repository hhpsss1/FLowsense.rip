#include <shared.h>

namespace {
    constexpr uint64_t k_grenade_collision_mask = 0x1C3003ull;
    constexpr float k_pass_player_nudge = 3.5f;

    bool IsPlayerPawn(C_BaseEntity* entity) {
        if (!entity)
            return false;

        const auto identity = entity->m_pEntity();
        if (!identity)
            return false;

        const auto class_info = entity->GetClassBindingBase();
        if (!class_info || !class_info->name)
            return false;

        return FnvHash(class_info->name) == FnvHash("C_CSPlayerPawn");
    }

    std::pair<Vector, std::optional<CGameTrace>> trace_grenade_segment(Vector from, Vector to, Vector mins, Vector maxs, C_CSPlayerPawn* thrower_pawn, int sim_tick) {
        Vector cursor = from;

        for (int pass = 0; pass < 16; ++pass) {
            auto remainder = to - cursor;
            float rem_sqr = remainder.LengthSqr();
            if (rem_sqr < 1e-6f)
                return { to, std::nullopt };

            const bool skip_thrower = thrower_pawn && sim_tick < 18;
            auto filter = Physics->MakeMovementFilter(skip_thrower ? thrower_pawn : nullptr, k_grenade_collision_mask, 4);

            CRay ray{};
            ray.Init(mins, maxs);

            auto trace = Physics->MakeTrace();
            Vector start_pos = cursor;
            Vector end_pos = to;
            Physics->TraceRay(&ray, &start_pos, &end_pos, &filter, &trace);

            if (trace.fraction >= 1.0f)
                return { to, std::nullopt };

            Vector hit_pos = cursor + (to - cursor) * trace.fraction;

            if (IsPlayerPawn(trace.entity)) {
                float rem_len = std::sqrt(rem_sqr);
                Vector dir = remainder * (1.0f / rem_len);
                cursor = hit_pos + dir * k_pass_player_nudge;
                continue;
            }

            trace.end_pos = hit_pos;
            return { hit_pos, trace };
        }

        return { cursor, std::nullopt };
    }
}

CGrenadeTrajectory::EGrenadeType CGrenadeTrajectory::GetTypeFromName(const char* name) {
    if (!name)
        return EGrenadeType::unknown;

    std::string s(name);
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (s.find(xs("smoke")) != std::string::npos)
        return EGrenadeType::smoke;
    if (s.find(xs("molotov")) != std::string::npos)
        return EGrenadeType::molotov;
    if (s.find(xs("inc")) != std::string::npos)
        return EGrenadeType::incendiary;
    if (s.find(xs("flash")) != std::string::npos)
        return EGrenadeType::flash;
    if (s.find(xs("hegrenade")) != std::string::npos || s.find(xs("explosive")) != std::string::npos)
        return EGrenadeType::he;
    if (s.find(xs("decoy")) != std::string::npos)
        return EGrenadeType::decoy;

    return EGrenadeType::unknown;
}

void CGrenadeTrajectory::SetSimulationParams(EGrenadeType type) {
    this->current_type = type;

    switch (type) {
    case EGrenadeType::smoke:
        this->detonate_time = 0.0f;
        this->velocity_threshold = 0.1f;
        break;
    case EGrenadeType::decoy:
        this->detonate_time = 2.0f;
        this->velocity_threshold = 0.2f;
        break;
    case EGrenadeType::molotov:
    case EGrenadeType::incendiary:
        this->detonate_time = (this->molotov_detonate_time > 0.0f) ? this->molotov_detonate_time : 2.0f;
        this->velocity_threshold = 0.0f;
        break;
    case EGrenadeType::flash:
    case EGrenadeType::he:
    default:
        this->detonate_time = 1.5f;
        this->velocity_threshold = 0.1f;
        break;
    }
}

void CGrenadeTrajectory::UpdateWeaponProperties(C_BaseCSGrenade* weapon, CCSWeaponBaseVData* vdata) {
    if (!vdata)
        return;

    this->throw_velocity = std::clamp(vdata->m_flThrowVelocity(), 1.0f, 10000.0f);

    const char* weapon_name = vdata->m_szName();
    if (!weapon_name && weapon)
        weapon_name = weapon->GetName().c_str();

    this->SetSimulationParams(this->GetTypeFromName(weapon_name));
}

bool CGrenadeTrajectory::ShouldDetonate(Vector vel, int tick) const {
    switch (this->current_type) {
    case EGrenadeType::smoke: {
        float speed_2d = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        int check_ticks = static_cast<int>(0.2f / INTERVAL_PER_TICK);
        return tick > 0 && speed_2d < this->velocity_threshold && (tick % check_ticks) == 0;
    }
    case EGrenadeType::decoy: {
        float speed_2d = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        int check_ticks = static_cast<int>(0.2f / INTERVAL_PER_TICK);
        return tick > 0 && speed_2d < this->velocity_threshold && (tick % check_ticks) == 0;
    }
    case EGrenadeType::molotov:
    case EGrenadeType::incendiary: {
        return (static_cast<float>(tick) * INTERVAL_PER_TICK) >= this->detonate_time;
    }
    case EGrenadeType::flash:
    case EGrenadeType::he: {
        return static_cast<float>(tick - 8) * INTERVAL_PER_TICK > this->detonate_time;
    }
    default:
        return static_cast<float>(tick) * INTERVAL_PER_TICK > this->detonate_time;
    }
}

Vector CGrenadeTrajectory::ClipVelocity(Vector velocity, Vector normal, float overbounce) {
    float backoff = velocity.Dot(normal) * overbounce;

    Vector out = {
        velocity.x - normal.x * backoff,
        velocity.y - normal.y * backoff,
        velocity.z - normal.z * backoff
    };

    if (std::abs(out.x) < 0.1f)
        out.x = 0.0f;

    if (std::abs(out.y) < 0.1f)
        out.y = 0.0f;

    if (std::abs(out.z) < 0.1f)
        out.z = 0.0f;

    return out;
}

void CGrenadeTrajectory::OnCreateMove() {
    if (!ctx.pawn || !ctx.pawn->Alive())
        return;

    cvar(sv_gravity);
    cvar(weapon_molotov_maxdetonateslope);
    cvar(molotov_throw_detonate_time);

    if (sv_gravity)
        this->sv_gravity = sv_gravity->GetFloat();

    if (weapon_molotov_maxdetonateslope)
        this->molotov_max_slope_z = std::cosf(weapon_molotov_maxdetonateslope->GetFloat() * Math::degs);

    if (molotov_throw_detonate_time)
        this->molotov_detonate_time = molotov_throw_detonate_time->GetFloat();

    const auto weapon = reinterpret_cast<C_BaseCSGrenade*>(ctx.weapon);
    if (weapon && weapon->IsWeapon()) {
        const float throw_time = weapon->m_fThrowTime();
        if (throw_time > 0.0f)
            this->last_throw_time = GlobalVars ? GlobalVars->current_time : 0.f;
    }
}

void CGrenadeTrajectory::OnPresent() {
    if (!GlobalVars || !EngineClient->InGame())
        return;

    if (!ctx.pawn || !ctx.pawn->Alive() || !ctx.controller)
        return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl)
        return;

    if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
        return;

    this->UpdateInFlight(ctx.pawn);

    if (!Config::Get<bool>(Vars::Grenades::Trajectory))
        return;

    const float curtime = GlobalVars ? GlobalVars->current_time : 0.f;

    for (auto& nade : this->in_flight) {
        if (!nade.info.valid)
            continue;

        float alpha = 1.0f;
        if (nade.detonated) {
            const float elapsed = curtime - nade.detonate_time;
            alpha = std::clamp(1.0f - elapsed / k_fade_duration, 0.0f, 1.0f);
        }

        if (alpha > 0.0f)
            this->RenderTrajectory(dl, nade.info, alpha, true, ctx.pawn);
    }

    this->RenderPreview(dl, ctx.pawn);
}

void CGrenadeTrajectory::SetupThrow(C_CSPlayerPawn* local_pawn, C_BaseCSGrenade* weapon, Vector& origin, Vector& velocity) {
    float strength = 1.0f;

    const bool pin_pulled = weapon->m_bPinPulled();
    const float throw_time = weapon->m_fThrowTime();

    if (pin_pulled || throw_time > 0.0f) {
        strength = std::clamp(weapon->m_flThrowStrength(), 0.0f, 1.0f);
        if (std::abs(strength - 0.5f) <= 0.1f)
            strength = 0.5f;
    }

    auto angles = ctx.camera_angles;
    angles.x -= (90.0f - std::abs(angles.x)) * 10.0f / 90.0f;

    auto eye_pos = local_pawn->m_vecViewOffset() + local_pawn->m_pGameSceneNode()->m_vecAbsOrigin();
    eye_pos.z += strength * 12.0f - 12.0f;

    const float pitch = angles.x * Math::degs;
    const float yaw = angles.y * Math::degs;

    Vector forward = {
        std::cos(pitch) * std::cos(yaw),
        std::cos(pitch) * std::sin(yaw),
        -std::sin(pitch)
    };

    Vector mins{ -k_hull_size, -k_hull_size, -k_hull_size };
    Vector maxs{ k_hull_size,  k_hull_size,  k_hull_size };
    Vector trace_dest = eye_pos + forward * k_forward_offset;

    if (Physics) {
        CRay ray{};
        ray.Init(mins, maxs);

        auto filter = Physics->MakeMovementFilter(local_pawn, k_grenade_collision_mask, 4);
        auto trace = Physics->MakeTrace();
        Vector start_pos = eye_pos;
        Vector end_pos = trace_dest;
        Physics->TraceRay(&ray, &start_pos, &end_pos, &filter, &trace);

        Vector hit_pos = (trace.fraction < 1.0f) ? (eye_pos + forward * (k_forward_offset * trace.fraction)) : trace_dest;
        origin = hit_pos - forward * k_pull_back;
    }
    else {
        origin = eye_pos;
    }

    const float throw_vel = std::clamp(this->throw_velocity * 0.9f, 15.0f, 750.0f);
    const float throw_speed = (strength * 0.7f + 0.3f) * throw_vel;
    velocity = forward * throw_speed + local_pawn->m_vecAbsVelocity() * k_velocity_inherit;
}

void CGrenadeTrajectory::Simulate(Vector start, Vector velocity, C_CSPlayerPawn* thrower_pawn, TrajectoryInfo_t& out) {
    out.points.clear();
    out.points.reserve(k_max_ticks / k_ticks_per_point);
    out.bounces.clear();
    out.bounce_indices.clear();
    out.valid = false;
    out.end_tick = -1;

    auto pos = start;
    auto vel = velocity;
    int bounce_count = 0;
    int tick_timer = 0;

    for (int tick = 0; tick < k_max_ticks; ++tick) {
        if (tick_timer == 0)
            out.points.push_back(pos);

        bool hit = false;
        bool impact_detonate = false;
        Vector impact_pos{};
        this->StepSimulation(pos, vel, thrower_pawn, tick, hit, impact_detonate, impact_pos);

        if (hit) {
            ++bounce_count;
            out.points.push_back(impact_pos);
            out.bounces.push_back(impact_pos);
            out.bounce_indices.push_back(out.points.size() - 1);
            tick_timer = 0;
        }

        bool velocity_stopped = std::abs(vel.x) < 20.0f && std::abs(vel.y) < 20.0f && vel.LengthSqr() < k_stop_speed_sq;

        if (impact_detonate || this->ShouldDetonate(vel, tick) || bounce_count > k_max_bounces || (bounce_count > 0 && velocity_stopped)) {
            out.end_tick = tick;
            out.end_pos = pos;
            out.duration = static_cast<float>(tick) * INTERVAL_PER_TICK;
            break;
        }

        if (!hit && ++tick_timer >= k_ticks_per_point)
            tick_timer = 0;
    }

    if (out.end_tick < 0) {
        out.end_tick = k_max_ticks;
        out.end_pos = pos;
        out.duration = static_cast<float>(k_max_ticks) * INTERVAL_PER_TICK;
    }

    if (!out.points.empty()) {
        if (out.points.back().DistTo(out.end_pos) > 1.0f)
            out.points.push_back(out.end_pos);
    }

    if (out.points.size() == 1)
        out.points.push_back(out.end_pos);

    out.valid = (out.end_tick >= 0) && (out.points.size() >= 2);
}

void CGrenadeTrajectory::StepSimulation(Vector& pos, Vector& vel, C_CSPlayerPawn* thrower_pawn, int sim_tick, bool& hit, bool& detonated, Vector& impact_pos) {
    detonated = false;
    hit = false;

    float gravity = this->sv_gravity * k_gravity_scale;
    float new_vel_z = vel.z - gravity * INTERVAL_PER_TICK;

    Vector move = {
        vel.x * INTERVAL_PER_TICK,
        vel.y * INTERVAL_PER_TICK,
        (vel.z + new_vel_z) * 0.5f * INTERVAL_PER_TICK
    };

    vel.z = new_vel_z;

    Vector mins{ -k_hull_size, -k_hull_size, -k_hull_size };
    Vector maxs{ k_hull_size,  k_hull_size,  k_hull_size };
    Vector move_dest = pos + move;

    auto [travel_end, world_hit_trace] = trace_grenade_segment(pos, move_dest, mins, maxs, thrower_pawn, sim_tick);
    pos = travel_end;

    if (world_hit_trace.has_value()) {
        hit = true;
        impact_pos = travel_end;
        this->ResolveCollision(*world_hit_trace, pos, vel, thrower_pawn, sim_tick, detonated);
    }
}

void CGrenadeTrajectory::ResolveCollision(CGameTrace& trace, Vector& pos, Vector& vel, C_CSPlayerPawn* thrower_pawn, int sim_tick, bool& detonated) {
    detonated = false;

    if (this->current_type == EGrenadeType::molotov || this->current_type == EGrenadeType::incendiary) {
        if (trace.hit_normal.z >= this->molotov_max_slope_z || vel.LengthSqr() < k_stop_speed_sq) {
            detonated = true;
            vel = { 0.0f, 0.0f, 0.0f };
            return;
        }
    }

    Vector normal = trace.hit_normal;
    if (normal.LengthSqr() < 0.001f)
        normal = { 0.0f, 0.0f, 1.0f };

    auto new_vel = ClipVelocity(vel, normal, 2.0f);
    new_vel.x *= k_elasticity;
    new_vel.y *= k_elasticity;
    new_vel.z *= k_elasticity;

    if (trace.entity) {
        if (!IsPlayerPawn(trace.entity)) {
            const int health = trace.entity->m_iHealth();
            if (health > 0 && health <= 100 && trace.fraction > 0.7f) {
                if (new_vel.LengthSqr() > k_steep_bounce_speed_sq) {
                    float dot = new_vel.Normalized().Dot(normal);
                    if (dot > 0.5f) {
                        float dampen = (1.0f - dot) + 0.5f;
                        new_vel = new_vel * dampen;
                    }
                }
            }
        }
    }

    if (normal.z > k_steep_bounce_normal_z) {
        if (new_vel.LengthSqr() > k_steep_bounce_speed_sq) {
            float dot = new_vel.Normalized().Dot(normal);
            if (dot > 0.5f) {
                float dampen = 1.5f - dot;
                new_vel = new_vel * dampen;
            }
        }
    }

    if (new_vel.LengthSqr() < k_stop_speed_sq) {
        vel = { 0.0f, 0.0f, 0.0f };
        return;
    }

    vel = new_vel;
    pos = pos + normal * 0.1f;

    float remaining = 1.0f - trace.fraction;
    if (remaining > 0.0f) {
        Vector mins{ -k_hull_size, -k_hull_size, -k_hull_size };
        Vector maxs{ k_hull_size,  k_hull_size,  k_hull_size };
        Vector slide_goal = pos + new_vel * (remaining * INTERVAL_PER_TICK);

        auto [slide_end, slide_hit] = trace_grenade_segment(pos, slide_goal, mins, maxs, thrower_pawn, sim_tick);
        pos = slide_end;
    }
}

void CGrenadeTrajectory::UpdateInFlight(C_CSPlayerPawn* local_pawn) {
    if (!GlobalVars || !EntitySystem || !ctx.pawn->Alive())
        return;

    const float cur_time = GlobalVars ? GlobalVars->current_time : 0.f;
    std::unordered_set<C_BaseEntity*> alive_projectiles{};

    for (int i = 65; i < 2048; ++i) {
        auto entity = EntitySystem->GetBaseEntity(i).As<C_BaseEntity*>();
        if (!entity || !entity->m_pEntity() || (entity->m_pEntity()->m_flags() & 0x4) != 0)
            continue;

        const auto class_info = entity->GetClassBindingBase();
        if (!class_info || !class_info->name)
            continue;

        auto proj_type = this->GetTypeFromName(class_info->name);
        if (proj_type == EGrenadeType::unknown)
            continue;

        const auto grenade_proj = reinterpret_cast<C_BaseCSGrenadeProjectile*>(entity);

        uint32_t thrower_handle = 0;
        auto h_thrower = grenade_proj->m_hThrower();
        if (h_thrower.Valid())
            thrower_handle = h_thrower.GetEntryIndex();

        if (!thrower_handle && entity->m_hOwnerEntity().Valid())
            thrower_handle = entity->m_hOwnerEntity().GetEntryIndex();

        alive_projectiles.insert(entity);

        FlightGrenade_t* existing = nullptr;
        for (auto& gren : this->in_flight) {
            if (gren.entity == entity) {
                existing = &gren;
                break;
            }
        }

        if (existing) {
            existing->last_seen = cur_time;
            if (!existing->corrected) {
                auto initial_pos = grenade_proj->m_vInitialPosition();
                auto initial_vel = grenade_proj->m_vInitialVelocity();

                if (initial_vel.LengthSqr() < 1.0f)
                    initial_vel = entity->m_vecAbsVelocity();

                if (initial_pos.LengthSqr() < 1.0f && entity->m_pGameSceneNode())
                    initial_pos = entity->m_pGameSceneNode()->m_vecAbsOrigin();

                if (initial_vel.LengthSqr() >= 1.0f) {
                    auto saved_type = this->current_type;
                    this->SetSimulationParams(existing->type);

                    auto thrower = EntitySystem->GetBaseEntity(existing->thrower_handle).As<C_CSPlayerPawn*>();
                    this->Simulate(initial_pos, initial_vel, thrower ? thrower : local_pawn, existing->info);

                    this->SetSimulationParams(saved_type);
                    existing->corrected = true;
                }
            }
            continue;
        }

        auto initial_pos = grenade_proj->m_vInitialPosition();
        auto initial_vel = grenade_proj->m_vInitialVelocity();

        if (initial_vel.LengthSqr() < 1.0f)
            initial_vel = entity->m_vecAbsVelocity();

        if (initial_pos.LengthSqr() < 1.0f && entity->m_pGameSceneNode())
            initial_pos = entity->m_pGameSceneNode()->m_vecAbsOrigin();

        if (initial_vel.LengthSqr() < 1.0f)
            continue;

        auto saved_type = this->current_type;
        this->SetSimulationParams(proj_type);

        FlightGrenade_t gren{};
        gren.entity = entity;
        gren.type = proj_type;
        gren.thrower_handle = thrower_handle;
        gren.throw_time = cur_time;
        gren.last_seen = cur_time;
        gren.corrected = true;

        auto thrower = EntitySystem->GetBaseEntity(thrower_handle).As<C_CSPlayerPawn*>();
        this->Simulate(initial_pos, initial_vel, thrower ? thrower : local_pawn, gren.info);

        this->SetSimulationParams(saved_type);
        this->in_flight.push_back(std::move(gren));
    }

    for (auto& gren : this->in_flight) {
        if (gren.detonated)
            continue;

        const float elapsed = cur_time - gren.throw_time;
        if (gren.info.valid && elapsed >= gren.info.duration) {
            gren.detonated = true;
            gren.detonate_time = cur_time;
            continue;
        }

        if (!alive_projectiles.contains(gren.entity) && cur_time - gren.last_seen >= k_missing_grace) {
            gren.detonated = true;
            gren.detonate_time = cur_time;
        }
    }

    std::erase_if(this->in_flight, [cur_time, &alive_projectiles](FlightGrenade_t& g) {
        if (!g.detonated)
            return false;
        return (cur_time - g.detonate_time) > k_fade_duration && !alive_projectiles.contains(g.entity);
        });
}

void CGrenadeTrajectory::ComputeDamageAtEndpoint(TrajectoryInfo_t& traj, std::vector<DamageInfo_t>& damages, C_CSPlayerPawn* local_pawn, int* out_local_damage) {
    damages.clear();
    if (out_local_damage)
        *out_local_damage = 0;

    if (!traj.valid || traj.end_tick < 0 || this->current_type != EGrenadeType::he)
        return;

    constexpr float base_damage = 99.0f;
    constexpr float damage_radius = 350.0f;
    constexpr float sigma = 116.6f;

    if (!Physics || !local_pawn)
        return;

    Vector trace_start = traj.end_pos + Vector{ 0.0f, 0.0f, 2.0f };

    auto get_damage_for_pawn = [&](C_CSPlayerPawn* pawn) -> int {
        if (!pawn || !pawn->Alive())
            return 0;

        const auto node = pawn->m_pGameSceneNode();
        if (!node)
            return 0;

        auto player_origin = node->m_vecAbsOrigin();
        auto center = player_origin + Vector{ 0.0f, 0.0f, 36.0f };
        float distance = center.DistTo(traj.end_pos);

        if (distance > damage_radius)
            return 0;

        CRay ray{};
        ray.Init(trace_start);

        auto filter = Physics->MakeMovementFilter(nullptr, k_grenade_collision_mask, 4);
        auto trace = Physics->MakeTrace();
        Vector start_pos = trace_start;
        Vector end_pos = center;
        Physics->TraceRay(&ray, &start_pos, &end_pos, &filter, &trace);

        if (trace.fraction < 0.97f && trace.entity != pawn)
            return 0;

        float falloff = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
        float adjusted_damage = base_damage * falloff;
        int armor = pawn->m_ArmorValue();

        if (armor > 0) {
            constexpr float armor_ratio = 1.2f * 0.5f;
            float damage_to_health = adjusted_damage * armor_ratio;
            float damage_to_armor = (adjusted_damage - damage_to_health);

            if (damage_to_armor <= static_cast<float>(armor))
                adjusted_damage = damage_to_health;
            else
                adjusted_damage -= static_cast<float>(armor);
        }

        int raw_damage = static_cast<int>(adjusted_damage);
        return std::clamp(raw_damage, 0, pawn->m_iHealth());
        };

    if (out_local_damage)
        *out_local_damage = get_damage_for_pawn(local_pawn);

    for (auto& player : EntityList->players) {
        if (!player.Fill())
            continue;

        auto pawn = player.pawn;
        if (!pawn || pawn == local_pawn || !pawn->Alive() || !local_pawn->Enemy(pawn))
            continue;

        int final_damage = get_damage_for_pawn(pawn);
        if (final_damage <= 0)
            continue;

        const auto node = pawn->m_pGameSceneNode();
        Vector center = node ? (node->m_vecAbsOrigin() + Vector{ 0.0f, 0.0f, 36.0f }) : Vector{};
        damages.push_back({ final_damage, final_damage >= pawn->m_iHealth(), center });
    }
}

void CGrenadeTrajectory::RenderDamagePrediction(ImDrawList* draw_list, TrajectoryInfo_t& traj, C_CSPlayerPawn* local_pawn) {
    if (!Config::Get<bool>(Vars::Grenades::PredictDamage))
        return;

    if (!traj.valid || traj.end_tick < 0 || this->current_type != EGrenadeType::he)
        return;

    if (!local_pawn || !local_pawn->Alive())
        return;

    std::vector<DamageInfo_t> enemy_damages{};
    int local_damage = 0;
    this->ComputeDamageAtEndpoint(traj, enemy_damages, local_pawn, &local_damage);

    int max_enemy_damage = 0;
    for (const auto& d : enemy_damages) {
        if (d.damage > max_enemy_damage)
            max_enemy_damage = d.damage;
    }

    if (local_damage <= 0 && max_enemy_damage <= 0)
        return;

    const int local_health = local_pawn->m_iHealth();
    const bool local_lethal = (local_damage >= local_health && local_damage > 0);

    int display_damage = 0;
    Color_t target_color{};

    const auto local_color = Config::Get<Color_t>(Vars::Grenades::PredictDamageColor1);
    const auto enemy_color = Config::Get<Color_t>(Vars::Grenades::PredictDamageColor2);

    if (local_lethal || (local_damage >= max_enemy_damage && local_damage > 0)) {
        display_damage = local_damage;
        target_color = local_color;
    }
    else if (max_enemy_damage > local_damage && max_enemy_damage > 0) {
        display_damage = max_enemy_damage;
        target_color = enemy_color;
    }

    if (display_damage <= 0)
        return;

    ImFont* font = nullptr;
    const int font_idx = Config::Get<int>(Vars::Grenades::PredictDamageFont);
    switch (font_idx) {
    case 0:
        font = Var->font.pixel;
        break;
    case 1:
        font = Var->font.verdanab;
        break;
    case 2:
        font = Var->font.verdana;
        break;
    default:
        font = Var->font.pixel;
        break;
    }

    if (!font)
        font = ImGui::GetFont();

    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    float center_x = display_size.x * 0.5f;
    float center_y = display_size.y * 0.5f + 35.0f;

    std::string text = std::to_string(display_damage) + " HP";

    ImVec2 text_size = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, text.c_str());

    ImVec2 text_pos = ImVec2(std::floor(center_x - text_size.x * 0.5f), std::floor(center_y));

    ImU32 text_col_u32 = IM_COL32(target_color.r, target_color.g, target_color.b, target_color.a);
    ImU32 shadow_col_u32 = IM_COL32(0, 0, 0, static_cast<int>(target_color.a * 0.85f));

    draw_list->AddText(font, font->LegacySize, ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), shadow_col_u32, text.c_str());
    draw_list->AddText(font, font->LegacySize, text_pos, text_col_u32, text.c_str());
}

void CGrenadeTrajectory::RenderPreview(ImDrawList* draw_list, C_CSPlayerPawn* local_pawn) {
    if (!Config::Get<bool>(Vars::Grenades::Trajectory))
        return;

	if (!EngineClient->InGame() || !local_pawn->Alive())
		return;

    const auto weapon = reinterpret_cast<C_BaseCSGrenade*>(local_pawn->GetActiveWeapon());
    if (!weapon || !weapon->IsWeapon())
        return;

    const auto vdata = weapon->GetWeaponData();
    if (!vdata || vdata->m_WeaponType() != static_cast<int>(WEAPONTYPE_GRENADE))
        return;

    const bool pin_pulled = weapon->m_bPinPulled();
    const float throw_time = weapon->m_fThrowTime();

    if (throw_time > 0.0f)
        return;

    if (!pin_pulled) {
        const float time_since_throw = (GlobalVars ? GlobalVars->current_time : 0.f) - this->last_throw_time;
        if (time_since_throw < k_throw_cooldown)
            return;
    }

    if (Config::Get<bool>(Vars::Grenades::TrajectoryPinPulled) && !pin_pulled)
        return;

    this->UpdateWeaponProperties(weapon, vdata);

    Vector origin{}, velocity{};
    this->SetupThrow(local_pawn, weapon, origin, velocity);

    TrajectoryInfo_t frame_traj{};
    this->Simulate(origin, velocity, local_pawn, frame_traj);

    if (frame_traj.valid) {
        this->RenderTrajectory(draw_list, frame_traj, 1.0f, false, local_pawn);
        this->RenderDamagePrediction(draw_list, frame_traj, local_pawn);
    }
}

void CGrenadeTrajectory::RenderTrajectory(ImDrawList* draw_list, TrajectoryInfo_t& traj, float alpha, bool is_thrown, C_CSPlayerPawn* local_pawn) {
    if (!traj.valid || traj.points.size() < 2)
        return;

    std::vector<DamageInfo_t> damages{};
    this->ComputeDamageAtEndpoint(traj, damages, local_pawn, nullptr);

    const bool has_damage = !damages.empty();
    const auto line_col = has_damage ? Config::Get<Color_t>(Vars::Grenades::TrajectoryHitColor) : Config::Get<Color_t>(Vars::Grenades::TrajectoryColor);
    const auto bounce_col = Config::Get<Color_t>(Vars::Grenades::TrajectoryColor);
    const auto end_col = Config::Get<Color_t>(Vars::Grenades::TrajectoryHitColor);

    const float scale = std::clamp(Config::Get<float>(Vars::Grenades::TrajectoryScale), 1.0f, 10.0f);
    const float glow_intensity = std::clamp(Config::Get<float>(Vars::Grenades::TrajectoryGlowIntensity), 0.0f, 1.0f);
    const float line_thickness = scale;
    const float circle_scale = 1.0f + (scale - 1.0f) * 0.15f;

    const size_t count = traj.points.size();

    std::vector<ImVec2> screen_points(count);
    std::vector<float> point_alphas(count);
    std::vector<bool> valid_points(count);

    for (size_t i = 0; i < count; ++i) {
        Vector2D screen = Draw->WorldToScreen(traj.points[i]);

        const bool is_valid = (screen.x != -1.0f || screen.y != -1.0f);
        valid_points[i] = is_valid;
        screen_points[i] = ImVec2(screen.x, screen.y);

        float t = static_cast<float>(i) / static_cast<float>(count - 1);
        point_alphas[i] = alpha * (1.0f - t * 0.5f);
    }

    if (glow_intensity > 0.0f) {
        constexpr int glow_layers = 6;
        constexpr float glow_max_spread = 6.0f;

        for (int layer = glow_layers; layer >= 1; --layer) {
            float layer_t = static_cast<float>(layer) / static_cast<float>(glow_layers);
            float extra = glow_max_spread * glow_intensity * layer_t;
            float layer_thickness = line_thickness + extra * 2.0f;
            float layer_alpha_mult = glow_intensity * 0.18f * std::pow(1.0f - layer_t, 2.4f);

            for (size_t i = 1; i < count; ++i) {
                if (!valid_points[i - 1] || !valid_points[i])
                    continue;

                float seg_alpha = point_alphas[i] * layer_alpha_mult;
                if (seg_alpha <= 0.001f)
                    continue;

                ImU32 glow_color = ImColor(line_col.r, line_col.g, line_col.b, static_cast<int>(seg_alpha * 255.0f));
                draw_list->AddLine(screen_points[i - 1], screen_points[i], glow_color, layer_thickness);
            }
        }
    }

    constexpr int gradient_buckets = 20;

    size_t run_start = 0;
    while (run_start < count) {
        if (!valid_points[run_start]) {
            ++run_start;
            continue;
        }

        size_t run_end = run_start;
        while (run_end + 1 < count && valid_points[run_end + 1])
            ++run_end;

        size_t run_len = run_end - run_start + 1;

        if (run_len < 2) {
            run_start = run_end + 1;
            continue;
        }

        size_t bucket_size = std::max<size_t>(1, static_cast<size_t>(std::ceil(static_cast<float>(run_len - 1) / gradient_buckets)));

        for (size_t seg_start = run_start; seg_start < run_end; seg_start += bucket_size) {
            size_t seg_end = std::min(seg_start + bucket_size, run_end);
            size_t seg_len = seg_end - seg_start + 1;

            if (seg_len < 2)
                continue;

            size_t mid_idx = (seg_start + seg_end) / 2;
            float seg_alpha = point_alphas[mid_idx];

            ImU32 col = ImColor(line_col.r, line_col.g, line_col.b, static_cast<int>(line_col.a * seg_alpha));

            draw_list->AddPolyline(&screen_points[seg_start], static_cast<int>(seg_len), col, ImDrawFlags_None, line_thickness);
        }

        run_start = run_end + 1;
    }

    for (auto idx : traj.bounce_indices) {
        if (idx >= count || !valid_points[idx])
            continue;

        ImU32 outline_color = IM_COL32(0, 0, 0, static_cast<int>(220.0f * alpha));
        ImU32 fill_color = ImColor(bounce_col.r, bounce_col.g, bounce_col.b, static_cast<int>(bounce_col.a * alpha));

        draw_list->AddCircleFilled(screen_points[idx], 4.0f * circle_scale, outline_color, 16);
        draw_list->AddCircleFilled(screen_points[idx], 2.5f * circle_scale, fill_color, 16);
    }

    if (traj.end_tick >= 0) {
        Vector2D end_screen = Draw->WorldToScreen(traj.end_pos);
        if (end_screen.x != -1.0f || end_screen.y != -1.0f) {
            ImVec2 screen_pos = ImVec2(end_screen.x, end_screen.y);
            ImU32 outline_color = IM_COL32(0, 0, 0, static_cast<int>(220.0f * alpha));
            ImU32 fill_color = ImColor(end_col.r, end_col.g, end_col.b, static_cast<int>(end_col.a * alpha));

            draw_list->AddCircleFilled(screen_pos, 4.0f * circle_scale, outline_color, 16);
            draw_list->AddCircleFilled(screen_pos, 2.5f * circle_scale, fill_color, 16);
        }
    }
}

void CGrenadeTrajectory::Clear() {
    this->in_flight.clear();
}