#include <shared.h>

void CEvents::OnBulletImpact(CGameEvent* event) {
	if (!ctx.pawn || !ctx.controller || !GlobalVars || !ctx.cmd)
		return;

	if (GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	auto controller = event->GetPlayerController("userid");
	if (!controller)
		return;

	Vector world_pos = { event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z") };

	if (controller->m_bIsLocalPlayerController()) {
		if (Config::Get<bool>(Vars::Effects::ImpactsServer))
			World->AddBoxOverlay(world_pos, Config::Get<float>(Vars::Effects::ImpactsServerSize), Config::Get<Color_t>(Vars::Effects::ImpactsServerColor), Config::Get<float>(Vars::Effects::ImpactsServerDuration));

		Particles->ShotSparks(world_pos);

		if (Config::Get<bool>(Vars::Effects::BeamsLocal)) {
			this->FlushLocalImpacts(ctx.cmd->command_number);

			if (pending_local_impact.valid && pending_local_impact.group_key == ctx.cmd->command_number) {
				pending_local_impact.end_pos = world_pos;
			} else {
				pending_local_impact.valid = true;
				pending_local_impact.group_key = ctx.cmd->command_number;
				pending_local_impact.start_pos = ctx.pawn->GetEyePosition();
				pending_local_impact.end_pos = world_pos;
				pending_local_impact.type = ETracerType::Local;
			}

			return;
		}
	}

	auto shooter_pawn = (C_CSPlayerPawn*)controller->m_hPawn().get();
	if (!shooter_pawn || !shooter_pawn->Alive() || shooter_pawn == ctx.pawn)
		return;

	bool is_enemy = shooter_pawn->Enemy(ctx.pawn);

	if (is_enemy && !Config::Get<bool>(Vars::Effects::BeamsEnemy))
		return;

	if (!is_enemy && !Config::Get<bool>(Vars::Effects::BeamsFriendly))
		return;

	int current_tick = GlobalVars->tick_count;
	this->FlushRemoteImpacts(current_tick);

	uintptr_t shooter_key = reinterpret_cast<uintptr_t>(shooter_pawn);
	Vector shoot_pos = shooter_pawn->GetEyePosition();

	auto& pending = pending_remote_impacts[shooter_key];

	if (pending.valid && pending.group_key == current_tick) {
		pending.end_pos = world_pos;
	} else {
		pending.valid = true;
		pending.group_key = current_tick;
		pending.start_pos = shoot_pos;
		pending.end_pos = world_pos;
		pending.type = is_enemy ? ETracerType::Enemy : ETracerType::Friendly;
	}
}

void CEvents::OnPlayerHurt(CGameEvent* event) {
	if (!ctx.pawn || !ctx.controller || !GlobalVars)
		return;

	if (GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	auto controller = event->GetPlayerController(xs("userid"));
	auto attacker = event->GetPlayerController(xs("attacker"));
	if (!controller || !attacker)
		return;

	auto damage = event->GetInt(xs("dmg_health"), 0);
	if (damage <= 0)
		return;

	int hitgroup = event->GetInt(xs("hitgroup"));
	int health = event->GetInt(xs("health"));

	if (!attacker->m_bIsLocalPlayerController() || controller->m_bIsLocalPlayerController())
		return;

	auto victim_pawn = reinterpret_cast<C_CSPlayerPawn*>(controller->m_hPawn().get());
	if (!victim_pawn || !victim_pawn->Enemy(ctx.pawn))
		return;

	if (Config::Get<bool>(Vars::Misc::Hitmarker))
		Misc->hitmarker_time = GetTickCount64();

	if (Config::Get<bool>(Vars::Misc::Hitsound)) {
		int sound_index = 0;

		if (hitgroup != 1 && health > 0)
			sound_index = Config::Get<int>(Vars::Misc::HitsoundBodyHit);
		else if (hitgroup != 1 && health <= 0)
			sound_index = Config::Get<int>(Vars::Misc::HitsoundBodyKill);
		else if (hitgroup == 1 && health > 0)
			sound_index = Config::Get<int>(Vars::Misc::HitsoundHeadHit);
		else if (hitgroup == 1 && health <= 0)
			sound_index = Config::Get<int>(Vars::Misc::HitsoundHeadKill);

		const char* sounds[] = {
			"sounds/physics/reverie.vsnd",
			"sounds/ui/coin_pickup_01.vsnd",
			"sounds/ui/panorama/mainmenu_press_shop_01.vsnd",
			"sounds/ui/killcard_1.vsnd",
			"sounds/ui/armsrace_level_up.vsnd",
			"sounds/ui/panorama/claim_gift_01.vsnd",
			"sounds/ui/xp_star_full_03.vsnd",
			"sounds/ui/panorama/itemtile_click_02.vsnd",
			"sounds/physics/rust_headshot.vsnd",
			"sounds/physics/sparkle.vsnd",
			"sounds/physics/primordial.vsnd",
			"sounds/physics/cod.vsnd",
			"sounds/weapons/fx/tink/bullet_casing_07.vsnd",
			"sounds/physics/1_pig.vsnd"
		};

		this->PlayVsnd(sounds[sound_index]);
	}
}

void CEvents::OnWeaponFire(CGameEvent* event) {
	if (!ctx.pawn || !ctx.controller || !ctx.cmd || !ctx.weapon_data || !ctx.pawn->Alive() || !GlobalVars)
		return;

	if (GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	auto controller = event->GetPlayerController("userid");
	if (!controller || !controller->m_bIsLocalPlayerController())
		return;

	Vector start;
	Vector direction;

	if (Ragebot->LastFireCommand() == ctx.cmd->command_number) {
		start = Ragebot->LastFirePosition();
		Math::Angle2Vectors(const_cast<Vector&>(Ragebot->LastFireAngle()), &direction);
	} else {
		start = ctx.pawn->GetEyePosition();
		Math::Angle2Vectors(Movement->angle, &direction);
	}

	this->FireClientTracer(start, direction);
}

void CEvents::OnPlayerDeath(CGameEvent* event) {
	if (!ctx.pawn || !ctx.controller || !ctx.cmd || !ctx.weapon_data || !ctx.pawn->Alive() || !GlobalVars)
		return;

	if (GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	auto controller = event->GetPlayerController("userid");
	auto attacker = event->GetPlayerController("attacker");
	if (!controller || !attacker)
		return;

	int dmg = event->GetInt(xs("dmg_health"));
	if (dmg <= 0)
		return;

	if (!controller->m_bIsLocalPlayerController() && attacker->m_bIsLocalPlayerController()) {
		auto victim_pawn = reinterpret_cast<C_CSPlayerPawn*>(controller->m_hPawn().get());
		if (ctx.pawn->m_iTeamNum() == victim_pawn->m_iTeamNum())
			return;

		Particles->Soul(victim_pawn);
	}
}

void CEvents::OnFireGameEvent(CGameEvent* event) {
	if (!event)
		return;

	switch (FnvHash(event->GetName())) {
	case FnvHash("round_start"):
		Misc->buy_pending = true;
		Misc->hitmarker_time = 0;
		break;
	case FnvHash("player_hurt"):
		this->OnPlayerHurt(event);
		break;
	case FnvHash("bullet_impact"):
		this->OnBulletImpact(event);
		break;
	case FnvHash("weapon_fire"):
		this->OnWeaponFire(event);
		break;
	case FnvHash("player_death"):
		this->OnPlayerDeath(event);
		break;
	}
}

void CEvents::FlushPendingImpacts() {
	if (!ctx.cmd || !GlobalVars)
		return;

	this->FlushLocalImpacts(ctx.cmd->command_number);
	this->FlushRemoteImpacts(GlobalVars->tick_count);
}

void CEvents::FlushLocalImpacts(int current_cmd_number) {
	if (!pending_local_impact.valid)
		return;

	if (pending_local_impact.group_key != current_cmd_number) {
		this->SpawnTracer(pending_local_impact);
		pending_local_impact.valid = false;
	}
}

void CEvents::FlushRemoteImpacts(int current_tick) {
	for (auto& [key, pending] : pending_remote_impacts) {
		if (pending.valid && pending.group_key != current_tick) {
			this->SpawnTracer(pending);
			pending.valid = false;
		}
	}
}

void CEvents::SpawnTracer(PendingImpact& impact) {
	switch (impact.type) {
	case ETracerType::Local:
		Particles->Add(impact.start_pos, impact.end_pos, Config::Get<Color_t>(Vars::Effects::BeamsLocalColor), static_cast<float>(Config::Get<int>(Vars::Effects::BeamsLocalDuration)), Config::Get<float>(Vars::Effects::BeamsLocalWidth) / 10.f);
		break;
	case ETracerType::Enemy:
		Particles->Add(impact.start_pos, impact.end_pos, Config::Get<Color_t>(Vars::Effects::BeamsEnemyColor), static_cast<float>(Config::Get<int>(Vars::Effects::BeamsEnemyDuration)), Config::Get<float>(Vars::Effects::BeamsEnemyWidth) / 10.f);
		break;
	case ETracerType::Friendly:
		Particles->Add(impact.start_pos, impact.end_pos, Config::Get<Color_t>(Vars::Effects::BeamsFriendlyColor), static_cast<float>(Config::Get<int>(Vars::Effects::BeamsFriendlyDuration)), Config::Get<float>(Vars::Effects::BeamsFriendlyWidth) / 10.f);
		break;
	}
}

void CEvents::FireClientTracer(Vector& start, Vector& direction) {
	if (!Config::Get<bool>(Vars::Effects::ImpactsClient))
		return;

	Vector dir = direction;
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

	for (int i = 0; i < data.trace_info.size; i++) {
		const auto& info = data.trace_info[i];

		if (info.damage <= 0.f)
			break;

		if (info.IsPenetrationSegment())
			continue;

		CGameTrace trace;
		Physics->GetTraceInfo(&data, &trace, info);
		Vector segment_end = trace.end_pos;

		World->AddBoxOverlay(segment_end, Config::Get<float>(Vars::Effects::ImpactsClientSize), Config::Get<Color_t>(Vars::Effects::ImpactsClientColor), Config::Get<float>(Vars::Effects::ImpactsClientDuration));
	}
}

void CEvents::PlayVsnd(const char* vsnd) {
	static auto fn = Memory->Modules.Soundsystem.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? 33 F6 C7 45 ? ? ? ? ? 4C 8B C1")).As<void(__fastcall*)(const char*)>();
	if (fn && vsnd)
		fn(vsnd);
}

void CEvents::OnCreateMove() {
	cvar(snd_toolvolume);
	if (snd_toolvolume)
		snd_toolvolume->value.fl = Config::Get<float>(Vars::Misc::HitsoundVolume);
}