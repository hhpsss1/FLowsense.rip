#include <shared.h>

void CParticles::Add(const Vector& start, const Vector& end, Color_t col, float time, float width) {
	if (!ctx.pawn || !ctx.controller || !ParticleManager || !GlobalVars)
		return;

	if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	CBeamInfo info{};

	const char* path = xs("particles/econ/tracer.vpcf");

	CBufferString name(path, xs("fcpv"));
	ResourceSystem->BlockingLoadResourceByName(&name, "");

	if (!ParticleManager->CreateParticleEffect(&info.effect_index, path))
		return;

	Vector shoot_pos = start;
	ParticleManager->SetControlPoint(info.effect_index, 0, &shoot_pos);

	Vector end_pos = end;
	ParticleManager->SetControlPoint(info.effect_index, 1, &end_pos);

	Vector clr = Vector(col.r, col.g, col.b);
	ParticleManager->SetControlPoint(info.effect_index, 3, &clr);

	Vector duration = Vector(time, 0.f, 0.f);
	ParticleManager->SetControlPoint(info.effect_index, 4, &duration);

	Vector size = Vector(width, 0.f, 0.f);
	ParticleManager->SetControlPoint(info.effect_index, 5, &size);

	float alpha = 1.f;
	ParticleManager->SetControlPoint(info.effect_index, 6, &alpha);

	ParticleSystem->CreateSnapshot(&info.snapshot_particle);
	ParticleManager->InitEffect(info.effect_index, 0, info.snapshot_particle.binding);

	info.snapshot_particle->Draw(5, &info.particle_data);

	this->stack_beams.push_back({ info.effect_index, GlobalVars->current_time, time });
}

void CParticles::Clear() {
	if (ParticleManager) {
		for (const auto& beam : this->stack_beams) {
			if (beam.effect_index == 0)
				continue;

			ParticleManager->DestroyParticle(beam.effect_index, true, true);
			ParticleManager->ReleaseParticleIndex(beam.effect_index);
		}
	}

	this->stack_beams.clear();
}

void CParticles::Soul(C_CSPlayerPawn* victim) {
	if (!Config::Get<bool>(Vars::Effects::Soul))
		return;

	if (!ctx.pawn || !ctx.controller || !ParticleManager || !GlobalVars)
		return;

	if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	const char* path = nullptr;

	if (Config::Get<bool>(Vars::Effects::SoulZ))
		path = xs("particles/econ/soul_z.vpcf");
	else
		path = xs("particles/econ/soul.vpcf");

	CBufferString name(path, xs("fcpv"));
	ResourceSystem->BlockingLoadResourceByName(&name, "");

	uint32_t effect_index = 0;
	if (!ParticleManager->CreateParticleEffect(&effect_index, path))
		return;

	Vector density = Vector(Config::Get<float>(Vars::Effects::SoulDensity), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 2, &density);

	Vector duration = Vector(Config::Get<float>(Vars::Effects::SoulDuration), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 3, &duration);

	auto clr = Config::Get<Color_t>(Vars::Effects::SoulColor);
	Vector color = Vector(clr.r, clr.g, clr.b);
	ParticleManager->SetControlPoint(effect_index, 4, &color);

	Vector intensity = Vector(Config::Get<float>(Vars::Effects::SoulIntensity), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 5, &intensity);

	Vector glow_intensity = Vector(Config::Get<float>(Vars::Effects::SoulGlowIntensity), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 6, &glow_intensity);

	float opacity = 1.f;
	ParticleManager->SetControlPoint(effect_index, 7, &opacity);

	float glow_opacity = 1.f;
	ParticleManager->SetControlPoint(effect_index, 8, &glow_opacity);

	Vector gravity = Vector(Config::Get<float>(Vars::Effects::SoulGravityX), Config::Get<float>(Vars::Effects::SoulGravityY), Config::Get<float>(Vars::Effects::SoulGravityZ));
	ParticleManager->SetControlPoint(effect_index, 9, &gravity);

	struct {
		intptr_t xy{ 0x7f7fffff7f7fffffll };
		int z{ 0x7f7fffff };
	} default_pos;

	ParticleManager->SetEntityBinding(effect_index, 0, (void*)victim, 1, nullptr, &default_pos, 1, 0ll);
	ParticleManager->SetEntityBinding(effect_index, 1, (void*)victim, 1, nullptr, &default_pos, 1, 0ll);
}

void CParticles::Weather() {
	static bool reset = false;

	auto Create = [&]() {
		int weather_type = Config::Get<int>(Vars::Effects::WeatherType);

		if (weather_type == 0 || weather_type == 1) {
			const char* path = (weather_type == 0) ? xs("particles/econ/stars.vpcf") : xs("particles/econ/snow.vpcf");

			CBufferString name(path, xs("fcpv"));
			ResourceSystem->BlockingLoadResourceByName(&name, "");

			for (int i = 0; i < 4; i++) {
				uint32_t index{ 0 };
				if (ParticleManager->CreateParticleEffect(&index, path) != nullptr && index != 0)
					weather_index.push_back(index);
			}
		} else if (weather_type == 2) {
			const char* path_meteor = xs("particles/econ/meteor.vpcf");
			const char* path_ash = xs("particles/econ/ash.vpcf");

			CBufferString name_meteor(path_meteor, xs("fcpv"));
			ResourceSystem->BlockingLoadResourceByName(&name_meteor, "");

			CBufferString name_ash(path_ash, xs("fcpv"));
			ResourceSystem->BlockingLoadResourceByName(&name_ash, "");

			for (int i = 0; i < 4; i++) {
				uint32_t index_meteor{ 0 };
				if (ParticleManager->CreateParticleEffect(&index_meteor, path_meteor) != nullptr && index_meteor != 0)
					weather_index.push_back(index_meteor);

				uint32_t index_ash{ 0 };
				if (ParticleManager->CreateParticleEffect(&index_ash, path_ash) != nullptr && index_ash != 0)
					weather_index.push_back(index_ash);
			}
		}
	};

	auto Remove = [&]() {
		for (uint32_t& i : weather_index) {
			if (i != 0) {
				ParticleManager->DestroyParticle(i, true, true);
				ParticleManager->ReleaseParticleIndex(i);
			}
		}

		if (!weather_index.empty())
			weather_index.clear();
	};

	if (!Config::Get<bool>(Vars::Effects::Weather) || !ctx.pawn || !ctx.controller || !ParticleManager || !GameRules || !GlobalVars || (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))) {
		if (!weather_index.empty()) {
			Remove();
			reset = true;
		}

		return;
	}

	static bool last_world_particles = false;
	static int last_particles_type = -1;
	static float last_round_time = 0.f;

	bool particles_changed = (last_world_particles != Config::Get<bool>(Vars::Effects::Weather));
	bool type_changed = (last_particles_type != Config::Get<int>(Vars::Effects::WeatherType));

	float current_round_time = GameRules ? GameRules->m_fRoundStartTime() : GlobalVars->current_time;
	bool round_changed = (current_round_time != last_round_time);

	last_world_particles = Config::Get<bool>(Vars::Effects::Weather);
	last_particles_type = Config::Get<int>(Vars::Effects::WeatherType);
	last_round_time = current_round_time;

	if (reset || particles_changed || type_changed || round_changed) {
		if (!weather_index.empty())
			Remove();

		Create();
		reset = false;
	}

	if (!weather_index.empty()) {
		Vector origin = ctx.pawn && ctx.pawn->m_pGameSceneNode() && ctx.pawn->Alive() && GameRules && GameRules->m_fRoundStartTime() > 5.f ? ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin() : Vector();
		if (!origin.Valid() || origin.Zero())
			return;

		Vector forward, right;
		Math::Angle2Vectors(Movement->angle, &forward, &right);

		forward.NormalizeLength();
		right.NormalizeLength();

		float distance = 350.f;
		
		Vector positions[4]{
			origin + forward * distance,
			origin - forward * distance,
			origin + right * distance,
			origin - right * distance
		};

		int weather_type = Config::Get<int>(Vars::Effects::WeatherType);

		if (weather_type == 0) {
			Vector stars_intensity = Vector(Config::Get<float>(Vars::Effects::StarsIntensity), 0.f, 0.f);
			Vector stars_glow_intensity = Vector(Config::Get<float>(Vars::Effects::StarsGlowIntensity), 0.f, 0.f);
			Vector stars_density = Vector(Config::Get<float>(Vars::Effects::StarsDensity), 0.f, 0.f);
			Vector stars_color = Vector(Config::Get<Color_t>(Vars::Effects::StarsColor).r, Config::Get<Color_t>(Vars::Effects::StarsColor).g, Config::Get<Color_t>(Vars::Effects::StarsColor).b);
			Vector stars_gravity = Vector(Config::Get<float>(Vars::Effects::StarsGravityX), Config::Get<float>(Vars::Effects::StarsGravityY), Config::Get<float>(Vars::Effects::StarsGravityZ));

			for (int i = 0; i < 4; i++) {
				if (i < weather_index.size()) {
					uint32_t index = weather_index[i];
					if (index != 0) {
						ParticleManager->SetControlPoint(index, 0, &positions[i]);
						ParticleManager->SetControlPoint(index, 1, &stars_color);
						ParticleManager->SetControlPoint(index, 2, &stars_density);
						ParticleManager->SetControlPoint(index, 3, &stars_intensity);
						ParticleManager->SetControlPoint(index, 4, &stars_glow_intensity);
						ParticleManager->SetControlPoint(index, 5, &stars_gravity);
					}
				}
			}
		} else if (weather_type == 1) {
			Vector snow_density = Vector(Config::Get<float>(Vars::Effects::SnowDensity), 0.f, 0.f);
			Vector snow_glow_intensity = Vector(Config::Get<float>(Vars::Effects::SnowGlowIntensity), 0.f, 0.f);
			Vector snow_size = Vector(Config::Get<float>(Vars::Effects::SnowSize), 0.f, 0.f);
			Color_t clr = Config::Get<Color_t>(Vars::Effects::SnowColor);
			Vector snow_color = Vector(clr.Rf(), clr.Gf(), clr.Bf());
			Vector snow_gravity = Vector(Config::Get<float>(Vars::Effects::SnowGravityX), Config::Get<float>(Vars::Effects::SnowGravityY), Config::Get<float>(Vars::Effects::SnowGravityZ));
			Vector snow_wind = Vector(Config::Get<float>(Vars::Effects::SnowWind), 0.f, 0.f);
			float snow_opacity = Config::Get<float>(Vars::Effects::SnowOpacity);
			float snow_glow_opacity = Config::Get<float>(Vars::Effects::SnowGlowOpacity);

			for (int i = 0; i < 4; i++) {
				if (i < weather_index.size()) {
					uint32_t index = weather_index[i];
					if (index != 0) {
						ParticleManager->SetControlPoint(index, 0, &positions[i]);
						ParticleManager->SetControlPoint(index, 1, &snow_density);
						ParticleManager->SetControlPoint(index, 2, &snow_color);
						ParticleManager->SetControlPoint(index, 3, &snow_size);
						ParticleManager->SetControlPoint(index, 4, &snow_glow_intensity);
						ParticleManager->SetControlPoint(index, 5, &snow_gravity);
						ParticleManager->SetControlPoint(index, 6, &snow_wind);
						ParticleManager->SetControlPoint(index, 7, &snow_opacity);
						ParticleManager->SetControlPoint(index, 8, &snow_glow_opacity);
					}
				}
			}
		} else if (weather_type == 2) {
			Color_t clr = Config::Get<Color_t>(Vars::Effects::AshColor);
			Vector ash_color = Vector(clr.r, clr.g, clr.b);
			Vector ash_gravity = Vector(Config::Get<float>(Vars::Effects::AshGravityX), Config::Get<float>(Vars::Effects::AshGravityY), Config::Get<float>(Vars::Effects::AshGravityZ));
			Vector ash_duration = Vector(Config::Get<float>(Vars::Effects::AshDuration), 0.f, 0.f);
			Vector ash_density = Vector(Config::Get<float>(Vars::Effects::AshDensity), 0.f, 0.f);
			Vector ash_child_density = Vector(Config::Get<float>(Vars::Effects::AshChildDensity), 0.f, 0.f);

			for (int i = 0; i < 4; i++) {
				int meteor_idx = i * 2;
				int ash_idx = i * 2 + 1;

				auto ApplyAshCPs = [&](uint32_t idx) {
					if (idx == 0)
						return;

					ParticleManager->SetControlPoint(idx, 0, &positions[i]);
					ParticleManager->SetControlPoint(idx, 3, &ash_gravity);
					ParticleManager->SetControlPoint(idx, 4, &ash_color);
					ParticleManager->SetControlPoint(idx, 5, &ash_duration);
					ParticleManager->SetControlPoint(idx, 6, &ash_density);
					ParticleManager->SetControlPoint(idx, 15, &ash_child_density);
				};

				if (meteor_idx < weather_index.size())
					ApplyAshCPs(weather_index[meteor_idx]);

				if (ash_idx < weather_index.size())
					ApplyAshCPs(weather_index[ash_idx]);
			}
		}
	}
}

void CParticles::ShotSparks(const Vector& pos) {
	if (!Config::Get<bool>(Vars::Effects::ShotSparks))
		return;

	if (!ctx.pawn || !ctx.controller || !ParticleManager || !GlobalVars)
		return;

	if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	const char* path = xs("particles/econ/shot_sparks.vpcf");

	CBufferString name(path, xs("fcpv"));
	ResourceSystem->BlockingLoadResourceByName(&name, "");

	uint32_t effect_index;

	if (!ParticleManager->CreateParticleEffect(&effect_index, path))
		return;

	auto col = Config::Get<Color_t>(Vars::Effects::ShotSparksColor);

	Vector position = pos;
	ParticleManager->SetControlPoint(effect_index, 0, &position);

	Vector color = Vector(col.r, col.g, col.b);
	ParticleManager->SetControlPoint(effect_index, 1, &color);

	Vector density = Vector(Config::Get<float>(Vars::Effects::ShotSparksDensity), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 2, &density);

	Vector gravity = Vector(0.f, 0.f, Config::Get<float>(Vars::Effects::ShotSparksGravityZ));
	ParticleManager->SetControlPoint(effect_index, 3, &gravity);

	Vector intensity = Vector(Config::Get<float>(Vars::Effects::ShotSparksIntensity), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 4, &intensity);

	Vector glow_intensity = Vector(Config::Get<float>(Vars::Effects::ShotSparksGlowIntensity), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 5, &glow_intensity);

	Vector length = Vector(Config::Get<float>(Vars::Effects::ShotSparksLength), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 6, &length);

	Vector size = Vector(Config::Get<float>(Vars::Effects::ShotSparksSize), 0.f, 0.f);
	ParticleManager->SetControlPoint(effect_index, 7, &size);
}