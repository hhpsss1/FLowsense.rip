#include <shared.h>

namespace {
    constexpr uint64_t k_los_mask = 0x1C3003ull;
    constexpr float k_badge_radius = 38.0f;
    constexpr float k_track_radius = 32.0f;
    constexpr float k_track_thickness = 5.2f;
    constexpr float k_pin_length = 9.2f;
    constexpr float k_pin_half_width = 6.9f;
    constexpr float k_half_pi = Math::_pi * 0.5f;

    void draw_shadow_text(ImDrawList* draw_list, ImFont* font, float size, ImVec2 pos, const char* text, ImU32 fill_color, float alpha) {
        const ImU32 shadow_color = IM_COL32(0, 0, 0, static_cast<int>(255.0f * alpha * 0.85f));
        draw_list->AddText(font, size, { pos.x + 1.0f, pos.y + 1.0f }, shadow_color, text);
        draw_list->AddText(font, size, pos, fill_color, text);
    }
}

float CGrenadeWarnings::GetMaxDistance(CGrenadeTrajectory::EGrenadeType type) const {
    switch (type) {
    case CGrenadeTrajectory::EGrenadeType::he:
        return 550.0f;
    case CGrenadeTrajectory::EGrenadeType::flash:
        return 500.0f;
    case CGrenadeTrajectory::EGrenadeType::molotov:
    case CGrenadeTrajectory::EGrenadeType::incendiary:
        return 450.0f;
    case CGrenadeTrajectory::EGrenadeType::smoke:
        return 500.0f;
    default:
        return 400.0f;
    }
}

bool CGrenadeWarnings::ProjectAnchor(Vector& world, C_CSPlayerPawn* local_pawn, ImVec2& anchor, float& arrow_angle, bool& is_oof) const {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 display_center{ display.x * 0.5f, display.y * 0.5f };

    Vector2D screen_pos = Draw->WorldToScreen(world);
    const bool on_screen = (screen_pos.x != -1.0f || screen_pos.y != -1.0f);

    const float margin = k_badge_radius + 20.0f;
    is_oof = !on_screen || screen_pos.x < margin || screen_pos.x > display.x - margin || screen_pos.y < (margin + 10.0f) || screen_pos.y > display.y - margin;

    if (!is_oof) {
        anchor = { screen_pos.x, screen_pos.y - (k_badge_radius + k_pin_length) };
        anchor.y = std::max(anchor.y, k_badge_radius + 8.0f);
        anchor.x = std::clamp(anchor.x, k_badge_radius + 8.0f, display.x - k_badge_radius - 8.0f);
        arrow_angle = k_half_pi;
        return true;
    }

    auto eye = local_pawn->m_vecViewOffset() + local_pawn->m_pGameSceneNode()->m_vecAbsOrigin();
    auto dir = world - eye;
    const float len = std::sqrt(dir.LengthSqr());
    if (len < 1.0f)
        return false;

    dir = dir * (1.0f / len);

    const float yaw = ctx.camera_angles.y * Math::degs;

    const Vector forward{ std::cos(yaw), std::sin(yaw), 0.0f };
    const Vector right{ std::sin(yaw), -std::cos(yaw), 0.0f };

    const float front = dir.x * forward.x + dir.y * forward.y;
    const float side = dir.x * right.x + dir.y * right.y;

    const float view_angle = std::atan2(side, front);

    const float dir_screen_x = std::sin(view_angle);
    const float dir_screen_y = -std::cos(view_angle);

    const float oof_radius = std::min(display_center.x, display_center.y) * 0.65f;
    anchor = { display_center.x + dir_screen_x * oof_radius, display_center.y + dir_screen_y * oof_radius };

    arrow_angle = std::atan2(dir_screen_y, dir_screen_x);
    return true;
}

bool CGrenadeWarnings::TraceClear(Vector& from, Vector& to, C_CSPlayerPawn* skip, C_BaseEntity* accept) const {
    if (!Physics)
        return false;

    CTraceFilter filter = Physics->MakeFilter(skip, k_los_mask, 4, 15);
    
    CGameTrace trace;
    CRay ray{};
    Physics->TraceRay(&ray, &from, &to, &filter, &trace);

    return trace.fraction >= 0.97f || trace.entity == accept;
}

float CGrenadeWarnings::ComputeHeDamage(Vector& pos, C_CSPlayerPawn* target) const {
    if (!target || !target->Alive())
        return 0.0f;

    const auto node = target->m_pGameSceneNode();
    if (!node)
        return 0.0f;

    constexpr float base_damage = 99.0f;
    constexpr float damage_radius = 350.0f;
    constexpr float sigma = 116.6f;

    auto center = node->m_vecAbsOrigin() + Vector{ 0.0f, 0.0f, 36.0f };
    const float distance = center.DistTo(pos);

    if (distance > damage_radius)
        return 0.0f;

    Vector trace_start = pos + Vector{ 0.0f, 0.0f, 1.0f };
    if (!this->TraceClear(trace_start, center, nullptr, target))
        return 0.0f;

    const float falloff = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
    float damage = base_damage * falloff;

    const int armor = target->m_ArmorValue();
    if (armor > 0) {
        constexpr float armor_ratio = 1.2f * 0.5f;
        const float damage_to_health = damage * armor_ratio;
        const float damage_to_armor = damage - damage_to_health;

        if (damage_to_armor <= static_cast<float>(armor))
            damage = damage_to_health;
        else
            damage -= static_cast<float>(armor);
    }

    const int final_damage = static_cast<int>(damage);
    if (final_damage <= 0)
        return 0.0f;

    return std::min(static_cast<float>(final_damage), static_cast<float>(target->m_iHealth()));
}

float CGrenadeWarnings::ComputeBestEnemyDamage(Vector& pos, C_CSPlayerPawn* local_pawn) const {
    if (!local_pawn)
        return 0.0f;

    float best = 0.0f;

    for (auto& player : EntityList->players) {
        if (!player.Fill())
            continue;

        auto pawn = player.pawn;
        if (!pawn || pawn == local_pawn || !pawn->Alive() || !local_pawn->Enemy(pawn))
            continue;

        const float damage = this->ComputeHeDamage(pos, pawn);
        if (damage > best)
            best = damage;
    }

    return best;
}

void CGrenadeWarnings::DrawBadge(ImDrawList* draw_list, BadgeState_t& state) {
    const float scale = state.scale;
    const float alpha = scale;
    const ImVec2 anchor = state.anchor;

    const float radius = k_badge_radius * scale;
    const float track_radius = k_track_radius * scale;
    const float track_thickness = k_track_thickness * scale;

    const float dir_x = std::cos(state.arrow_angle);
    const float dir_y = std::sin(state.arrow_angle);
    const float perp_x = -dir_y;
    const float perp_y = dir_x;

    const float pin_len = k_pin_length * scale;
    const float pin_half_w = k_pin_half_width * scale;

    const ImVec2 tip{ anchor.x + dir_x * (radius + pin_len), anchor.y + dir_y * (radius + pin_len) };
    const ImVec2 base1{ anchor.x + dir_x * (radius - 1.0f * scale) + perp_x * pin_half_w, anchor.y + dir_y * (radius - 1.0f * scale) + perp_y * pin_half_w };
    const ImVec2 base2{ anchor.x + dir_x * (radius - 1.0f * scale) - perp_x * pin_half_w, anchor.y + dir_y * (radius - 1.0f * scale) - perp_y * pin_half_w };

    const ImVec2 shadow_center{ anchor.x, anchor.y + 4.0f * scale };
    for (int i = 0; i < 6; ++i) {
        const float t = static_cast<float>(i) / 5.0f;
        const float offset = t * 11.5f * scale;
        const float k = (1.0f - t) * (1.0f - t);
        const float layer_alpha = alpha * 55.0f * k;

        if (layer_alpha > 0.2f)
            draw_list->AddCircleFilled(shadow_center, radius + offset, IM_COL32(0, 0, 0, static_cast<int>(layer_alpha)), 48);
    }

    const ImU32 bg_col = IM_COL32(24, 24, 24, static_cast<int>(245.0f * alpha));
    const ImU32 border_col = IM_COL32(12, 12, 12, static_cast<int>(255.0f * alpha));

    draw_list->AddTriangleFilled(tip, base1, base2, bg_col);
    draw_list->AddLine(tip, base1, border_col, 1.5f * scale);
    draw_list->AddLine(tip, base2, border_col, 1.5f * scale);

    draw_list->AddCircleFilled(anchor, radius, bg_col, 48);
    draw_list->AddCircle(anchor, radius, border_col, 48, 1.5f * scale);

    const ImU32 track_col = IM_COL32(40, 40, 40, static_cast<int>(240.0f * alpha));
    draw_list->AddCircle(anchor, track_radius, track_col, 48, track_thickness);

    const float progress = std::clamp(state.current_progress, 0.0f, 1.0f);
    if (progress > 0.005f) {
        const float start_angle = -k_half_pi;
        const float sweep = progress * Math::_pi2;
        const float end_angle = start_angle + sweep;

        const auto color_cfg = Config::Get<Color_t>(Vars::Grenades::WarningsColor);
        const ImU32 prog_col = ImColor(color_cfg.r, color_cfg.g, color_cfg.b, static_cast<int>(color_cfg.a * alpha));

        draw_list->PathArcTo(anchor, track_radius, start_angle, end_angle, 48);
        draw_list->PathStroke(prog_col, ImDrawFlags_None, track_thickness);

        const ImVec2 start_p{ anchor.x + std::cos(start_angle) * track_radius, anchor.y + std::sin(start_angle) * track_radius };
        const ImVec2 end_p{ anchor.x + std::cos(end_angle) * track_radius, anchor.y + std::sin(end_angle) * track_radius };
        draw_list->AddCircleFilled(start_p, track_thickness * 0.5f, prog_col, 16);
        draw_list->AddCircleFilled(end_p, track_thickness * 0.5f, prog_col, 16);
    }

    const auto icon = Misc->Panorama.GetGrenadeIcon(state.icon_index);
    if (icon.texture_view) {
        const float icon_h = 20.0f * scale;
        const float ratio = icon.height > 0 ? static_cast<float>(icon.width) / static_cast<float>(icon.height) : 1.0f;
        const float icon_w = icon_h * ratio;

        const float icon_cy = anchor.y - 8.0f * scale;
        const ImVec2 icon_pos{ anchor.x - icon_w * 0.5f, icon_cy - icon_h * 0.5f };

        draw_list->AddImage(reinterpret_cast<ImTextureID>(icon.texture_view), icon_pos, { icon_pos.x + icon_w, icon_pos.y + icon_h }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, IM_COL32(255, 255, 255, static_cast<int>(255.0f * alpha)));
    }

    ImFont* font_sub = Var->font.inter[0] ? Var->font.inter[0] : ImGui::GetFont();
    const float sub_font_sz = font_sub->LegacySize;
    const ImU32 sub_col = IM_COL32(255, 255, 255, static_cast<int>(255.0f * alpha));

    if (state.has_sub) {
        const ImVec2 sub_sz = font_sub->CalcTextSizeA(sub_font_sz, FLT_MAX, -1.0f, state.sub_text);
        const ImVec2 sub_pos{ anchor.x - sub_sz.x * 0.5f, anchor.y + 4.5f * scale };
        draw_shadow_text(draw_list, font_sub, sub_font_sz, sub_pos, state.sub_text, sub_col, alpha);
    }
}

void CGrenadeWarnings::OnPresent() {
    if (!ctx.pawn || !GlobalVars || !Physics) {
        this->states.clear();
        this->inferno_spawn_times.clear();
        return;
    }

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    if (!draw_list)
        return;

    if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
        return;

    GrenadeTrajectory->UpdateInFlight(ctx.pawn);

    const float cur_time = GlobalVars ? GlobalVars->current_time : 0.f;
    const bool enabled = Config::Get<bool>(Vars::Grenades::Warnings) && ctx.pawn->Alive();
    auto local_eye = ctx.pawn->GetEyePosition();

    for (auto& pair : this->states)
        pair.second.touched = false;

    if (enabled) {
        cvar(inferno_flame_lifetime);
        const float flame_lifetime = inferno_flame_lifetime ? inferno_flame_lifetime->GetFloat() : 7.0f;

        std::vector<Vector> active_inferno_positions{};

        std::unordered_set<void*> seen_infernos{};

        for (int i = 65; i < 2048; ++i) {
            auto entity = EntitySystem->GetBaseEntity(i).As<C_BaseEntity*>();
            if (!entity || !entity->m_pEntity() || (entity->m_pEntity()->m_flags() & 0x4) != 0)
                continue;

            const auto class_info = entity->GetClassBindingBase();
            if (!class_info || !class_info->name)
                continue;

            const auto class_hash = FnvHash(class_info->name);

            if (class_hash == FnvHash("C_Inferno")) {
                auto inf = reinterpret_cast<C_Inferno*>(entity);
                void* key = reinterpret_cast<void*>(inf);
                seen_infernos.insert(key);

                if (!this->inferno_spawn_times.contains(key))
                    this->inferno_spawn_times[key] = cur_time;

                if (inf->m_fireCount() <= 0)
                    continue;

                Vector closest{};
                float closest_dist = FLT_MAX;

                for (int f = 0; f < inf->m_fireCount(); ++f) {
                    if (!inf->m_bFireIsBurning()[f])
                        continue;

                    auto& fp = inf->m_firePositions()[f];
                    const float d = local_eye.DistTo(fp);
                    if (d < closest_dist) {
                        closest_dist = d;
                        closest = fp;
                    }
                }

                if (closest_dist > 450.0f)
                    continue;

                active_inferno_positions.push_back(closest);

                if (!this->TraceClear(local_eye, closest, ctx.pawn, nullptr))
                    continue;

                const float spawn_time = this->inferno_spawn_times[key];
                const float elapsed = cur_time - spawn_time;
                if (elapsed >= flame_lifetime)
                    continue;

                const float remaining = flame_lifetime - elapsed;
                const float progress = std::clamp(remaining / flame_lifetime, 0.0f, 1.0f);

                ImVec2 anchor{};
                float arrow_angle = k_half_pi;
                bool is_oof = false;
                if (!this->ProjectAnchor(closest, ctx.pawn, anchor, arrow_angle, is_oof))
                    continue;

                auto& state = this->states[key];
                state.touched = true;
                state.wants = true;
                state.target_anchor = anchor;
                state.target_arrow_angle = arrow_angle;
                state.is_oof = is_oof;
                state.icon_index = -1;
                state.target_progress = progress;
                state.has_sub = true;

                std::snprintf(state.sub_text, sizeof(state.sub_text), "%dm", static_cast<int>(std::round(closest_dist * 0.0254f)));
            } else if (class_hash == FnvHash("C_SmokeGrenadeProjectile")) {
                auto smk = reinterpret_cast<C_SmokeProjectile*>(entity);
                if (!smk->m_bSmokeEffectSpawned())
                    continue;

                const auto node = smk->m_pGameSceneNode();
                if (!node)
                    continue;

                auto smoke_pos = node->m_vecAbsOrigin();
                const float distance = local_eye.DistTo(smoke_pos);
                if (distance > 500.0f)
                    continue;

                if (!this->TraceClear(local_eye, smoke_pos, ctx.pawn, nullptr))
                    continue;

                constexpr float max_smoke_time = 20.5f;
                const float elapsed = (GlobalVars->tick_count - smk->m_nSmokeEffectTickBegin()) * INTERVAL_PER_TICK;
                if (elapsed >= max_smoke_time)
                    continue;

                const float remaining = max_smoke_time - elapsed;
                const float progress = std::clamp(remaining / max_smoke_time, 0.0f, 1.0f);

                ImVec2 anchor{};
                float arrow_angle = k_half_pi;
                bool is_oof = false;
                if (!this->ProjectAnchor(smoke_pos, ctx.pawn, anchor, arrow_angle, is_oof))
                    continue;

                if (is_oof)
                    continue;

                void* key = reinterpret_cast<void*>(smk);
                auto& state = this->states[key];
                state.touched = true;
                state.wants = true;
                state.target_anchor = anchor;
                state.target_arrow_angle = arrow_angle;
                state.is_oof = is_oof;
                state.icon_index = 4;
                state.target_progress = progress;
                state.has_sub = true;

                std::snprintf(state.sub_text, sizeof(state.sub_text), "%dm", static_cast<int>(std::round(distance * 0.0254f)));
            }
        }

        std::erase_if(this->inferno_spawn_times, [&seen_infernos](const auto& pair) {
            return !seen_infernos.contains(pair.first);
        });

        for (const auto& gren : GrenadeTrajectory->GetInFlight()) {
            if (gren.type == CGrenadeTrajectory::EGrenadeType::unknown || gren.type == CGrenadeTrajectory::EGrenadeType::decoy)
                continue;

            if (gren.detonated)
                continue;

            const float duration = std::max(gren.info.duration, 0.1f);
            const float elapsed = cur_time - gren.throw_time;
            if (elapsed >= duration)
                continue;

            Vector landing_pos = gren.info.valid ? gren.info.end_pos : Vector{};
            if (!gren.info.valid && gren.entity && gren.entity->m_pGameSceneNode())
                landing_pos = gren.entity->m_pGameSceneNode()->m_vecAbsOrigin();

            if (landing_pos.LengthSqr() < 1.0f)
                continue;

            if (gren.type == CGrenadeTrajectory::EGrenadeType::molotov || gren.type == CGrenadeTrajectory::EGrenadeType::incendiary) {
                bool overlaps_inferno = false;
                for (auto& inf_pos : active_inferno_positions) {
                    if (landing_pos.DistTo(inf_pos) < 220.0f) {
                        overlaps_inferno = true;
                        break;
                    }
                }

                if (overlaps_inferno)
                    continue;
            }

            const float distance = local_eye.DistTo(landing_pos);

            float local_dmg = 0.0f;
            float enemy_dmg = 0.0f;

            if (gren.type == CGrenadeTrajectory::EGrenadeType::he) {
                local_dmg = this->ComputeHeDamage(landing_pos, ctx.pawn);
                enemy_dmg = this->ComputeBestEnemyDamage(landing_pos, ctx.pawn);

                if (local_dmg < 1.0f && enemy_dmg < 1.0f)
                    continue;

                if (enemy_dmg < 1.0f && distance > this->GetMaxDistance(gren.type))
                    continue;
            } else {
                if (distance > this->GetMaxDistance(gren.type))
                    continue;

                if (!this->TraceClear(local_eye, landing_pos, ctx.pawn, nullptr))
                    continue;
            }

            ImVec2 anchor{};
            float arrow_angle = k_half_pi;
            bool is_oof = false;
            if (!this->ProjectAnchor(landing_pos, ctx.pawn, anchor, arrow_angle, is_oof))
                continue;

            const float progress = std::clamp(elapsed / duration, 0.0f, 1.0f);
            const int dist_m = static_cast<int>(std::round(distance * 0.0254f));

            void* key = reinterpret_cast<void*>(gren.entity);
            auto& state = this->states[key];
            state.touched = true;
            state.wants = true;
            state.target_anchor = anchor;
            state.target_arrow_angle = arrow_angle;
            state.is_oof = is_oof;
            state.target_progress = progress;
            state.has_sub = true;

            if (gren.type == CGrenadeTrajectory::EGrenadeType::he) {
                state.icon_index = 0;
                const float display_dmg = local_dmg >= 1.0f ? local_dmg : enemy_dmg;
                std::snprintf(state.sub_text, sizeof(state.sub_text), "-%dhp", static_cast<int>(display_dmg));
            } else if (gren.type == CGrenadeTrajectory::EGrenadeType::molotov || gren.type == CGrenadeTrajectory::EGrenadeType::incendiary) {
                state.icon_index = -1;
                std::snprintf(state.sub_text, sizeof(state.sub_text), "%dm", dist_m);
            } else if (gren.type == CGrenadeTrajectory::EGrenadeType::smoke) {
                state.icon_index = 4;
                std::snprintf(state.sub_text, sizeof(state.sub_text), "%dm", dist_m);
            } else if (gren.type == CGrenadeTrajectory::EGrenadeType::flash) {
                state.icon_index = 1;
                std::snprintf(state.sub_text, sizeof(state.sub_text), "%dm", dist_m);
            }
        }
    }

    const float dt = ImGui::GetIO().DeltaTime;

    for (auto it = this->states.begin(); it != this->states.end();) {
        auto& state = it->second;

        if (!state.touched)
            state.wants = false;

        const float target_scale = state.wants ? 1.0f : 0.0f;
        const float anim_speed = state.wants ? 14.0f : 18.0f;
        state.scale += (target_scale - state.scale) * std::clamp(dt * anim_speed, 0.0f, 1.0f);
        state.current_progress += (state.target_progress - state.current_progress) * std::clamp(dt * 15.0f, 0.0f, 1.0f);

        if (state.scale < 0.01f && !state.wants) {
            it = this->states.erase(it);
            continue;
        }

        if (state.anchor.x == 0.0f && state.anchor.y == 0.0f) {
            state.anchor = state.target_anchor;
            state.arrow_angle = state.target_arrow_angle;
        } else {
            state.anchor.x += (state.target_anchor.x - state.anchor.x) * std::clamp(dt * 20.0f, 0.0f, 1.0f);
            state.anchor.y += (state.target_anchor.y - state.anchor.y) * std::clamp(dt * 20.0f, 0.0f, 1.0f);

            float diff = state.target_arrow_angle - state.arrow_angle;

            while (diff < -Math::_pi)
                diff += Math::_pi2;

            while (diff > Math::_pi)
                diff -= Math::_pi2;

            state.arrow_angle += diff * std::clamp(dt * 16.0f, 0.0f, 1.0f);
        }

        this->DrawBadge(draw_list, state);
        ++it;
    }
}

void CGrenadeWarnings::Clear() {
    this->states.clear();
    this->inferno_spawn_times.clear();
}