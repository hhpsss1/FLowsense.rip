#include <shared.h>

void CWorld::OnUpdateFOVSensitivity(C_CSPlayerPawn* pawn) {
	if (!pawn || !ctx.controller || pawn != ctx.pawn || pawn->m_iTeamNum() < 2)
		return;

	cvar(zoom_sensitivity_ratio);
	const float ratio = zoom_sensitivity_ratio->GetFloat();

	if (!Config::Get<bool>(Vars::World::FieldOfViewEnabled)) {
		pawn->m_flFOVSensitivityAdjust() = ratio;
		return;
	}

	const bool is_scoped = pawn->m_bIsScoped();
	float target_fov = Config::Get<float>(Vars::World::FieldOfView);

	if (is_scoped && ctx.weapon) {
		const int zoom_level = ctx.weapon->m_zoomLevel();
		if (zoom_level > 0) {
			const int item_index = ctx.weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
			float scope_base_fov = -1.f;

			if (item_index == WEAPON_SSG08)
				scope_base_fov = zoom_level == 1 ? 40.f : 15.f;
			else if (item_index == WEAPON_AWP)
				scope_base_fov = zoom_level == 1 ? 40.f : 10.f;
			else if (item_index == WEAPON_AUG || item_index == WEAPON_SG553)
				scope_base_fov = 45.f;

			if (scope_base_fov > 0.f) {
				const float zoom_param = std::clamp(zoom_level == 1 ? Config::Get<float>(Vars::World::FieldOfViewFirstZoom) : Config::Get<float>(Vars::World::FieldOfViewSecondZoom), 0.f, 1.f);
				target_fov = scope_base_fov + (Config::Get<float>(Vars::World::FieldOfView) - scope_base_fov) * zoom_param;
			}
		}
	}

	const float desired = ratio * (target_fov / 90.0f);

	if (is_scoped == this->fov_sensitivity_cached_scoped && target_fov == this->fov_sensitivity_cached_fov && this->fov_sensitivity_cached_value >= 0.0f) {
		if (std::fabsf(pawn->m_flFOVSensitivityAdjust() - this->fov_sensitivity_cached_value) < 0.0001f)
			return;
	}

	this->fov_sensitivity_cached_scoped = is_scoped;
	this->fov_sensitivity_cached_fov = target_fov;
	this->fov_sensitivity_cached_value = desired;

	pawn->m_flFOVSensitivityAdjust() = desired;
}

void CWorld::OnSetupView(CViewSetup* setup) {
	if (!ctx.pawn || !ctx.controller || !ctx.weapon || ctx.pawn->m_iTeamNum() < 2 || !GlobalVars || !GlobalVars->map_name_short || std::string(GlobalVars->map_name_short).contains("<empty>") || !EngineClient->InGame())
		return;

	if (Config::Get<int>(Vars::World::VisualRecoil) == 2)
		setup->vec_angles = CSGOInput->current_move_data.viewangles;

	const float default_fov = Config::Get<float>(Vars::World::FieldOfView);
	float target_fov = default_fov;

	const int zoom_level = ctx.weapon->m_zoomLevel();

	if (Config::Get<bool>(Vars::World::FieldOfViewEnabled)) {
		if (zoom_level > 0) {
			const int item_index = ctx.weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
			float scope_base_fov = -1.f;

			if (item_index == WEAPON_SSG08)
				scope_base_fov = zoom_level == 1 ? 40.f : 15.f;
			else if (item_index == WEAPON_AWP)
				scope_base_fov = zoom_level == 1 ? 40.f : 10.f;
			else if (item_index == WEAPON_AUG || item_index == WEAPON_SG553)
				scope_base_fov = 45.f;

			if (scope_base_fov > 0.f) {
				const float zoom_param = std::clamp(zoom_level == 1 ? Config::Get<float>(Vars::World::FieldOfViewFirstZoom) : Config::Get<float>(Vars::World::FieldOfViewSecondZoom), 0.f, 1.f);
				target_fov = scope_base_fov + (default_fov - scope_base_fov) * zoom_param;
			}
		}
	}

	if (Config::Get<bool>(Vars::World::AspectRatio) && Config::Get<float>(Vars::World::AspectRatioValue) > 0.0f) {
		const float custom_ar = Config::Get<float>(Vars::World::AspectRatioValue);

		const float tan_half_fov = tanf(target_fov * 0.00872664625f);
		const float scaled_tan = custom_ar * tan_half_fov * 0.75f;
		const float new_fov = atanf(scaled_tan) * 114.591559f;

		setup->world_fov = new_fov;
		setup->view_aspect_ratio() = custom_ar;
		setup->view_aspect_flag() |= 0x2;
	} else {
		setup->world_fov = target_fov;
		setup->view_aspect_flag() &= ~0x2;
	}

	if (!ctx.pawn->Alive())
		return;

	Vector eye_pos = ctx.pawn->GetEyePosition();
	if (Ragebot->duck_peek_active) {
		eye_pos.z = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin().z + Ragebot->duck_peek_height;
		setup->vec_origin.z = eye_pos.z;
	}

	float target_distance = Config::Get<bool>(Vars::World::Thirdperson) ? (float)Config::Get<int>(Vars::World::ThirdpersonDistance) : 0.0f;
	this->thirdperson_distance = Config::Get<bool>(Vars::World::ThirdpersonDisableAnimation) ? target_distance : std::lerp(this->thirdperson_distance, target_distance, INTERVAL_PER_TICK * 12.0f);

	if (Config::Get<bool>(Vars::World::Thirdperson) && this->thirdperson_distance >= 20.0f) {
		Vector forward;
		Math::Angle2Vectors(setup->vec_angles, &forward);

		Vector cam_pos = eye_pos - (forward * this->thirdperson_distance);
		const float scale = setup->view_aspect_ratio() * 0.5625f;
		Vector mins{ -9.f * scale, -9.f * scale, -9.f };
		Vector maxs{ 9.f * scale, 9.f * scale, 9.f };

		CRay ray{};
		ray.Init(mins, maxs);
		auto trace = Physics->MakeTrace();
		CTraceFilter filter = Physics->MakeFilter(ctx.pawn, 0x80, 3, 15);
		filter.shape_attr.interacts_exclude = 0xC0000;

		if (Physics->TraceRay(&ray, &eye_pos, &cam_pos, &filter, &trace) && trace.fraction < 1.f)
			cam_pos = eye_pos - (forward * (this->thirdperson_distance * trace.fraction));

		setup->vec_origin = cam_pos;
	}

	this->camera_origin = setup->vec_origin;
}

void CWorld::ScopeOverlay_t::OnPresent() {
	if (!ctx.pawn || !ctx.controller || !ctx.weapon || !ctx.pawn->Alive() || ctx.pawn->m_iTeamNum() < 2 || !ctx.pawn->m_bIsScoped())
		return;
	
	if (!EngineClient->InGame())
		return;

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	const auto display{ ImGui::GetIO().DisplaySize };

	cvar(cl_crosshair_sniper_width);

	if (Config::Get<int>(Vars::World::RemoveScope) == 1) {
		dl->AddLine(ImVec2(0, display.y / 2), ImVec2(display.x, display.y / 2), IM_COL32(0, 0, 0, 255), float(cl_crosshair_sniper_width->GetInt()));
		dl->AddLine(ImVec2(display.x / 2, 0), ImVec2(display.x / 2, display.y), IM_COL32(0, 0, 0, 255), float(cl_crosshair_sniper_width->GetInt()));
	} else if (Config::Get<int>(Vars::World::RemoveScope) == 2) {

	}
}

void CWorld::Wetness_t::OnUpdateShaderParam(MM::Murmur2Hash_t hash, float* value) {
	if (!ctx.pawn || !ctx.controller || !GlobalVars)
		return;

	if (!Config::Get<bool>(Vars::World::Enable) || !Config::Get<bool>(Vars::World::ModelWetness))
		return;

	static constexpr auto hash_exposure = MM::HashA1Const<true>("RainExposureToSky", MM::Murmur2Seed);
	static constexpr auto hash_timer = MM::HashA1Const<true>("RainExposureLocalTimer", MM::Murmur2Seed);
	static constexpr auto hash_wetness = MM::HashA1Const<true>("RainExposureToSkyWetness", MM::Murmur2Seed);

	if (hash == hash_exposure) {
		_mm_storeu_ps(value, _mm_set_ps1(1.f));
	} else if (hash == hash_timer) {
		const float speed = Config::Get<float>(Vars::World::ModelWetnessSpeed);
		const float timer = GlobalVars->real_time * speed;
		_mm_storeu_ps(value, _mm_set_ps1(timer));
	} else if (hash == hash_wetness) {
		const float coverage = Config::Get<float>(Vars::World::ModelWetnessCoverage);
		_mm_storeu_ps(value, _mm_set_ps1(coverage));
	}
}

void CWorld::Bloom_t::OnUpdateShaderParam(MM::Murmur2Hash_t hash, float* value) {
	if (!ctx.pawn || !ctx.controller || !GlobalVars)
		return;

	if (!Config::Get<bool>(Vars::World::Enable) || !Config::Get<bool>(Vars::World::Bloom))
		return;

	const float intensity = Config::Get<float>(Vars::World::BloomIntensity);

	if (hash == 0x565EAF76) {
		_mm_storeu_ps(value, _mm_set_ps1(0.3f + intensity * 1.2f));
	} else if (hash == 0xBA98A9B0) {
		_mm_storeu_ps(value, _mm_set_ps1(1.5f - intensity * 1.2f));
	} else if (hash == 0x2AE72B37) {
		_mm_storeu_ps(value, _mm_set_ps1(0.5f + intensity * 1.5f));
	} else if (hash == 0xB692902E) {
		_mm_storeu_ps(value, _mm_set_ps1(0.2f + intensity * 0.6f));
	} else if (hash == 0x1313A424) {
		_mm_storeu_ps(value, _mm_set_ps1(0.1f + intensity * 0.4f));
	}
}

void CWorld::Gamma_t::OnUpdateShaderParam(MM::Murmur2Hash_t hash, float* value) {
	if (!ctx.pawn || !ctx.controller || !GlobalVars)
		return;

	if (!Config::Get<bool>(Vars::World::Enable) || !Config::Get<bool>(Vars::World::Gamma))
		return;

	if (hash == 0x24470A87) {
		const float gamma_val = Config::Get<float>(Vars::World::GammaValue);

		float inverted_gamma = 2.2f;
		if (gamma_val <= 2.2f) {
			inverted_gamma = 6.6f - (gamma_val - 0.22f) * (20.0f / 9.0f);
		} else {
			inverted_gamma = 2.2f - (gamma_val - 2.2f) * 0.45f;
		}

		_mm_storeu_ps(value, _mm_set_ps1(inverted_gamma));
	}
}

void CWorld::Dof_t::OnUpdateShaderParam(MM::Murmur2Hash_t hash, float* value) {
	if (!ctx.pawn || !ctx.controller || !GlobalVars)
		return;

	if (!Config::Get<bool>(Vars::World::Enable) || !Config::Get<bool>(Vars::World::DepthOfField))
		return;

	if (hash == 0x2ACAB07C) {
		const float near_val = Config::Get<float>(Vars::World::DepthOfFieldNearDistance);
		const float far_val = Config::Get<float>(Vars::World::DepthOfFieldFarDistance);

		const float near_crisp = near_val;
		const float near_blurry = std::max(0.f, near_crisp - 100.f);

		const float far_crisp = std::max(near_crisp + 50.f, far_val);
		const float far_blurry = far_crisp + 300.f;

		_mm_storeu_ps(value, _mm_set_ps(far_blurry, far_crisp, near_crisp, near_blurry));
	}
}

void CWorld::AddBoxOverlay(const Vector& pos, float size, const Color_t& color, float duration) {
	std::lock_guard<std::mutex> lock(this->impact_mutex);

	ImpactRequest_t request{};
	request.position = pos;
	request.size = size;
	request.color = color;
	request.spawn_time = GlobalVars && GlobalVars->map_name_short && !(std::string(GlobalVars->map_name_short).contains("<empty>")) ? GlobalVars->current_time : 0.0f;
	request.duration = duration;

	this->impact_requests.push_back(request);
}

struct Face_t {
	int idx[4];
	Vector normal;
};

static const Face_t k_faces[6] = {
	{ {0,1,2,3}, { 0,  0, -1} },
	{ {4,5,6,7}, { 0,  0,  1} },
	{ {0,1,5,4}, { 0, -1,  0} },
	{ {3,2,6,7}, { 0,  1,  0} },
	{ {0,3,7,4}, {-1,  0,  0} },
	{ {1,2,6,5}, { 1,  0,  0} },
};

static const int k_edges[12][2] = {
	{0,1},{1,2},{2,3},{3,0},
	{4,5},{5,6},{6,7},{7,4},
	{0,4},{1,5},{2,6},{3,7}
};

void CWorld::OnPresent() {
	if (!EngineClient->InGame())
		return;

	if (!ctx.controller || !ctx.pawn || !ctx.cmd || !ctx.pawn->Alive())
		return;

	if (!GlobalVars)
		return;

	if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	std::vector<ImpactRequest_t> ss;

	{
		std::lock_guard<std::mutex> lock(this->impact_mutex);
		if (this->impact_requests.empty())
			return;

		ss = this->impact_requests;
	}

	const float curtime = GlobalVars->current_time;

	std::vector<size_t> to_remove;

	for (size_t idx = 0; idx < ss.size(); ++idx) {
		auto& box = ss[idx];

        float age = curtime - static_cast<float>(box.spawn_time);
        float t = age / box.duration;

        if (t >= 1.0f) {
            to_remove.push_back(idx);
            continue;
        }

        const float appear_time = 0.08f;
        float appear_t = std::clamp(age / appear_time, 0.0f, 1.0f);
        float inv = 1.0f - appear_t;
        float scale = 1.0f - inv * inv * inv;

        const float fade_start = 0.75f;
        float fade = t < fade_start ? 1.0f : 1.0f - ((t - fade_start) / (1.0f - fade_start));

        float s = box.size * scale;
        if (s <= 0.5f)
            continue;

        Vector corners[8] = {
            { box.position.x - s, box.position.y - s, box.position.z - s },
            { box.position.x + s, box.position.y - s, box.position.z - s },
            { box.position.x + s, box.position.y + s, box.position.z - s },
            { box.position.x - s, box.position.y + s, box.position.z - s },
            { box.position.x - s, box.position.y - s, box.position.z + s },
            { box.position.x + s, box.position.y - s, box.position.z + s },
            { box.position.x + s, box.position.y + s, box.position.z + s },
            { box.position.x - s, box.position.y + s, box.position.z + s },
        };

        ImVec2 screen[8];
        bool visible[8];
        int visible_count = 0;

		for (int i = 0; i < 8; i++) {
			Vector2D screen_pos = Draw->WorldToScreen(corners[i]);

			visible[i] = !(screen_pos.x == -1.0f && screen_pos.y == -1.0f);

			if (visible[i]) {
				screen[i] = ImVec2(screen_pos.x, screen_pos.y);
				visible_count++;
			}
		}

        if (visible_count == 0)
            continue;

        auto draw_list = ImGui::GetBackgroundDrawList();

        struct SortedFace_t {
            const Face_t* face;
            float dist;
        };

		SortedFace_t sorted[6];

        for (int f = 0; f < 6; f++) {
            const Face_t& face = k_faces[f];
            Vector center = {
                (corners[face.idx[0]].x + corners[face.idx[1]].x + corners[face.idx[2]].x + corners[face.idx[3]].x) * 0.25f,
                (corners[face.idx[0]].y + corners[face.idx[1]].y + corners[face.idx[2]].y + corners[face.idx[3]].y) * 0.25f,
                (corners[face.idx[0]].z + corners[face.idx[1]].z + corners[face.idx[2]].z + corners[face.idx[3]].z) * 0.25f,
            };
            Vector to_cam = camera_origin - center;
            sorted[f] = { &face, to_cam.x * to_cam.x + to_cam.y * to_cam.y + to_cam.z * to_cam.z };
        }

        std::sort(std::begin(sorted), std::end(sorted), [](const SortedFace_t& a, const SortedFace_t& b) { return a.dist > b.dist; });

        for (auto& sf : sorted) {
            const Face_t& face = *sf.face;

            bool all_visible = true;
            for (int i = 0; i < 4; i++) {
                if (!visible[face.idx[i]]) {
                    all_visible = false;
                    break;
                }
            }

            if (!all_visible)
                continue;

            Vector center = {
                (corners[face.idx[0]].x + corners[face.idx[1]].x + corners[face.idx[2]].x + corners[face.idx[3]].x) * 0.25f,
                (corners[face.idx[0]].y + corners[face.idx[1]].y + corners[face.idx[2]].y + corners[face.idx[3]].y) * 0.25f,
                (corners[face.idx[0]].z + corners[face.idx[1]].z + corners[face.idx[2]].z + corners[face.idx[3]].z) * 0.25f,
            };

			Vector to_cam = camera_origin - center;
            float len = std::sqrt(to_cam.x * to_cam.x + to_cam.y * to_cam.y + to_cam.z * to_cam.z);
            if (len > 0.001f) {
                to_cam.x /= len;
                to_cam.y /= len;
                to_cam.z /= len;
            }

            float ndotv = face.normal.x * to_cam.x + face.normal.y * to_cam.y + face.normal.z * to_cam.z;
            float brightness = std::clamp(ndotv, 0.0f, 1.0f);
            float face_alpha = (0.10f + 0.35f * brightness) * fade;

            ImVec2 poly[4] = { screen[face.idx[0]], screen[face.idx[1]], screen[face.idx[2]], screen[face.idx[3]] };

            ImColor fill_color(box.color.r, box.color.g, box.color.b, static_cast<int>(box.color.a * face_alpha));
            draw_list->AddConvexPolyFilled(poly, 4, fill_color);
        }

        struct glow_layer_t {
            float thickness;
            float alpha_mult;
        };

        static const glow_layer_t glow_layers[] = {
            { 7.0f, 0.06f },
            { 4.5f, 0.14f },
            { 2.5f, 0.30f },
            { 1.2f, 1.00f },
        };

        for (auto& layer : glow_layers) {
            int a = static_cast<int>(box.color.a * layer.alpha_mult * fade);
            if (a <= 0)
                continue;

            ImColor line_color(box.color.r, box.color.g, box.color.b, a);

            for (auto& e : k_edges) {
                if (!visible[e[0]] || !visible[e[1]])
                    continue;

                draw_list->AddLine(screen[e[0]], screen[e[1]], line_color, layer.thickness);
            }
        }
    }

    if (!to_remove.empty()) {
        std::lock_guard<std::mutex> lock(this->impact_mutex);
        for (auto rit = to_remove.rbegin(); rit != to_remove.rend(); ++rit) {
            if (*rit < this->impact_requests.size())
				this->impact_requests.erase(this->impact_requests.begin() + *rit);
        }
    }
}

void CWorld::PenetrationCrosshair_t::OnPresent() {
	if (!Config::Get<bool>(Vars::World::PenetrationCrosshair) || !ctx.pawn || !ctx.controller || !ctx.weapon || !ctx.weapon_data || !ctx.pawn->Alive() || !GlobalVars || !Physics)
		return;

	if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	if (auto wt = ctx.weapon_data->m_WeaponType(); wt == WEAPONTYPE_KNIFE || wt == WEAPONTYPE_GRENADE || wt == WEAPONTYPE_C4 || wt == WEAPONTYPE_TASER)
		return;

	auto draw_list = ImGui::GetBackgroundDrawList();

	ImVec2 center = ImVec2((float)SwapChain->GetScreenWidth() * 0.5f, (float)SwapChain->GetScreenHeight() * 0.5f);

	ImVec2 top_left(center.x - 2.f, center.y - 2.f);
	ImVec2 bottom_right(center.x + 2.f, center.y + 2.f);

	Color_t col = Config::Get<Color_t>(Vars::World::PenetrationCrosshairColor2);

	Vector start = ctx.camera_origin;

	Vector dir{};
	Math::Angle2Vectors(ctx.camera_angles, &dir);

	if (dir.Normalize() <= 0.f)
		return;

	auto filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);
	filter.shape_attr.flags |= 2;
	filter.shape_attr.interacts_exclude |= 0x4000000000;

	CTraceData data;
	Physics->SetupTrace(&data, start, dir * ctx.weapon_data->m_flRange(), filter, 4);
	Physics->TraceBullet(&data, static_cast<float>(ctx.weapon_data->m_nDamage()), ctx.weapon_data->m_flPenetration(), ctx.weapon_data->m_flRangeModifier(), ctx.pawn->m_iTeamNum(), 4);

	if (data.trace_info.size <= 0)
		return;

	bool valid{ false };
	Vector hit_pos{};
	Vector hit_normal{};
	bool found_wall{ false };

	for (int i = 0; i < data.trace_info.size; i++) {
		const auto& info = data.trace_info[i];

		if (!info.IsPenetrationSegment()) {
			if (!found_wall) {
				CGameTrace trace;
				Physics->GetTraceInfo(&data, &trace, info);
				if (trace.entity) {
					hit_pos = trace.end_pos;
					hit_normal = trace.hit_normal;
					found_wall = true;
				}
			}
			continue;
		}

		for (int j = i + 1; j < data.trace_info.size; j++) {
			const auto& next = data.trace_info[j];

			if (next.IsPenetrationSegment())
				continue;

			if (next.damage > 0.f) {
				col = Config::Get<Color_t>(Vars::World::PenetrationCrosshairColor1);
				valid = true;
			}

			break;
		}

		if (valid)
			break;
	}

	if (Config::Get<int>(Vars::World::PenetrationCrosshairType) == 0) {
		draw_list->AddRectFilled(top_left, bottom_right, ImColor(col.r, col.g, col.b));
	} else {
		if (!found_wall)
			return;

		float nrm_len = std::sqrt(hit_normal.x * hit_normal.x + hit_normal.y * hit_normal.y + hit_normal.z * hit_normal.z);
		if (nrm_len > 0.0001f)
			hit_normal = Vector(hit_normal.x / nrm_len, hit_normal.y / nrm_len, hit_normal.z / nrm_len);

		const Vector decal_center = hit_pos + hit_normal * 0.5f;
		Vector up_hint = (std::abs(hit_normal.z) < 0.99f) ? Vector(0.f, 0.f, 1.f) : Vector(1.f, 0.f, 0.f);

		Vector tangent(hit_normal.y * up_hint.z - hit_normal.z * up_hint.y, hit_normal.z * up_hint.x - hit_normal.x * up_hint.z, hit_normal.x * up_hint.y - hit_normal.y * up_hint.x);

		float t_len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
		if (t_len > 0.0001f)
			tangent = Vector(tangent.x / t_len, tangent.y / t_len, tangent.z / t_len);

		Vector bitangent(hit_normal.y * tangent.z - hit_normal.z * tangent.y, hit_normal.z * tangent.x - hit_normal.x * tangent.z, hit_normal.x * tangent.y - hit_normal.y * tangent.x);

		float b_len = std::sqrt(bitangent.x * bitangent.x + bitangent.y * bitangent.y + bitangent.z * bitangent.z);
		if (b_len > 0.0001f)
			bitangent = Vector(bitangent.x / b_len, bitangent.y / b_len, bitangent.z / b_len);

		constexpr float decal_size = 4.5f;
		constexpr float glow_size = 2.6f;
		constexpr int arc_steps = 4;
		constexpr int ring_size = 4 * (arc_steps + 1);

		const Vector corners_world[4] = {
			decal_center - tangent * decal_size - bitangent * decal_size,
			decal_center + tangent * decal_size - bitangent * decal_size,
			decal_center + tangent * decal_size + bitangent * decal_size,
			decal_center - tangent * decal_size + bitangent * decal_size
		};

		auto world_to_screen = [](const Vector& world, ImVec2& out) -> bool {
			Vector2D screen = Draw->WorldToScreen(world);
			if (screen.x == -1.f && screen.y == -1.f)
				return false;

			out = ImVec2(screen.x, screen.y);
			return true;
		};

		ImVec2 corners_screen[4];
		for (int i = 0; i < 4; i++) {
			if (!world_to_screen(corners_world[i], corners_screen[i]))
				return;
		}

		ImVec2 ring_inner[ring_size];
		ImVec2 ring_mid[ring_size];
		ImVec2 ring_outer[ring_size];

		int vtx = 0;
		for (int i = 0; i < 4; i++) {
			const int corner = (i + 1) % 4;
			const float start_angle = -1.5707963f + 1.5707963f * static_cast<float>(i);

			for (int j = 0; j <= arc_steps; j++) {
				const float ang = start_angle + 1.5707963f * (static_cast<float>(j) / static_cast<float>(arc_steps));
				const Vector d = tangent * std::cos(ang) + bitangent * std::sin(ang);

				ring_inner[vtx] = corners_screen[corner];
				if (!world_to_screen(corners_world[corner] + d * (glow_size * 0.35f), ring_mid[vtx]) || !world_to_screen(corners_world[corner] + d * glow_size, ring_outer[vtx]))
					return;

				vtx++;
			}
		}

		Color_t final_color = col;

		const ImU32 border_color = ImColor(final_color.r, final_color.g, final_color.b, final_color.a);
		const ImU32 fill_color = ImColor(final_color.r, final_color.g, final_color.b, static_cast<int>(final_color.a * 0.35f));
		const ImU32 glow_in = ImColor(final_color.r, final_color.g, final_color.b, static_cast<int>(final_color.a * 0.32f));
		const ImU32 glow_mid = ImColor(final_color.r, final_color.g, final_color.b, static_cast<int>(final_color.a * 0.07f));
		const ImU32 glow_out = ImColor(final_color.r, final_color.g, final_color.b, 0);

		const ImVec2 uv = ImGui::GetIO().Fonts->TexUvWhitePixel;

		for (int i = 0; i < ring_size; i++) {
			const int next = (i + 1) % ring_size;

			draw_list->PrimReserve(6, 4);
			ImDrawIdx idx = static_cast<ImDrawIdx>(draw_list->_VtxCurrentIdx);
			draw_list->PrimWriteIdx(idx);
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 1));
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 2));
			draw_list->PrimWriteIdx(idx);
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 2));
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 3));
			draw_list->PrimWriteVtx(ring_inner[i], uv, glow_in);
			draw_list->PrimWriteVtx(ring_inner[next], uv, glow_in);
			draw_list->PrimWriteVtx(ring_mid[next], uv, glow_mid);
			draw_list->PrimWriteVtx(ring_mid[i], uv, glow_mid);

			draw_list->PrimReserve(6, 4);
			idx = static_cast<ImDrawIdx>(draw_list->_VtxCurrentIdx);
			draw_list->PrimWriteIdx(idx);
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 1));
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 2));
			draw_list->PrimWriteIdx(idx);
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 2));
			draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(idx + 3));
			draw_list->PrimWriteVtx(ring_mid[i], uv, glow_mid);
			draw_list->PrimWriteVtx(ring_mid[next], uv, glow_mid);
			draw_list->PrimWriteVtx(ring_outer[next], uv, glow_out);
			draw_list->PrimWriteVtx(ring_outer[i], uv, glow_out);
		}

		draw_list->AddConvexPolyFilled(corners_screen, 4, fill_color);
		draw_list->AddPolyline(corners_screen, 4, border_color, ImDrawFlags_Closed, 1.5f);
	}
}

static constexpr std::array<std::string_view, 8> skybox_paths = {
	("materials/skybox/cs_italy_s2_skybox_sunset_2_exr_e56cedf6.vtex"),
	("materials/skybox/cs_italy_s2_skybox_sunset_2_fog_exr_a335887c.vtex"),
	("materials/skybox/sky_overcast_01_exr_da4019b1.vtex"),
	("materials/skybox/tests/src/lightingtest_sky_night_exr_2c5e8c62.vtex"),
	("materials/skybox/sky_hr_aztec_02_exr_f84f8de9.vtex"),
	("materials/skybox/jungle_cube_pfm_bc16d813.vtex"),
	("materials/skybox/sky_rain_night_01_exr_8d775aee.vtex"),
	("materials/skybox/sky_csgo_cloudy01_cube_pfm_f9a0b177.vtex")
};

inline const char* SkyboxTemplate = R"(
        <!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
        {
            Shader = "sky.vfx"
            g_flBrightnessExposureBias = 0.000000
            g_flRenderOnlyExposureBias = 0.000000
            SkyTexture = resource:"%s"
            g_tSkyTexture = resource:"%s"
        }
)";

inline CStrongHandle<CMaterial2> CreateMaterial(const char* mat_name, const char vmat[]) {
	alignas(16) std::byte storage[0x100 + sizeof(CKeyValues3)]{};
	auto* key_values = reinterpret_cast<CKeyValues3*>(storage + 0x100);
	const CKv3ID kv3_id{ mat_name, 0x469806E97412167CULL, 0xE73790B53EE6F2AFULL };

	KV3::InitLoader();
	KV3::load_key_value(key_values, nullptr, vmat, &kv3_id, nullptr, 0);

	CStrongHandle<CMaterial2> custom_material{};

	MaterialSystem->CreateMaterial(&custom_material, mat_name, key_values);

	return custom_material;
}

void CWorld::SkyboxChanger_t::OnFrameStage() {
	auto Reset = [this]() {
		if (has_custom_skybox || last_selected > 0) {
			has_custom_skybox = false;
			need_restore = true;
			last_selected = 0;
			custom_material_handle = {};
			custom_material = nullptr;
		}
	};

	if (!ctx.pawn || !ctx.controller || !GlobalVars || !ctx.cmd) {
		Reset();
		return;
	}

	if (GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>")) {
		Reset();
		return;
	}

	const bool enabled = Config::Get<bool>(Vars::World::SkyboxChanger);
	const int current = Config::Get<int>(Vars::World::SkyboxChangerType);

	if (!enabled || current == 0) {
		Reset();
		return;
	}

	if (last_selected == current && !need_update)
		return;

	last_selected = current;
	need_update = false;
	need_restore = false;

	std::string slc = std::string(skybox_paths[current - 1]);

	CBufferString buftex(slc.c_str(), xs("xetv"));
	ResourceSystem->BlockingLoadResourceByName(&buftex, "");

	char fmt[2048];
	sprintf_s(fmt, SkyboxTemplate, slc.c_str(), slc.c_str());

	static int mat_counter = 0;
	char mat_name[64];
	sprintf_s(mat_name, xs("ddc_skybox_%d"), mat_counter++);

	custom_material_handle = CreateMaterial(mat_name, fmt);
	custom_material = custom_material_handle;

	has_custom_skybox = (custom_material_handle.binding != nullptr && custom_material != nullptr);
}

void CWorld::SkyboxChanger_t::OnLevelShutdown() {
	default_material = nullptr;
	hook_default_material = nullptr;
	custom_material = nullptr;
	custom_material_handle = {};
	has_custom_skybox = false;
	need_restore = false;
	last_selected = -1;
}