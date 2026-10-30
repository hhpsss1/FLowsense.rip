#include <shared.h>
#include "Prediction.h"
#include "../Movement/Movement.h"
#include "../Ragebot/Ragebot.h"
#include "../../SDK/Globals.h"
#include <limits>

static Taia_t TimeFromSeconds(float time) {
	const float remainder = std::fmod(time, INTERVAL_PER_TICK);
	const float ticks = (time - remainder) * (1.f / INTERVAL_PER_TICK) + 0.5f;
	Taia_t result(static_cast<int>(ticks), remainder * (1.f / INTERVAL_PER_TICK));
	result.Normalize();
	return result;
}

static Taia_t NewestPlayerTime(const Taia_t& newest, float interval_seconds) {
	const Taia_t interval = TimeFromSeconds(interval_seconds);
	Taia_t time = newest + interval;
	if (time.fraction == 0.f) {
		--time.tick;
		time.fraction = 1.f;
	}

	time.fraction = std::nextafter(time.fraction, 0.f);
	if (!(time - interval < newest))
		time = time - Taia_t(0, std::numeric_limits<float>::epsilon() * 2.f);

	return time;
}

static bool PlayerTime(CCSPlayer_WeaponServices* services, float interval_seconds, const Taia_t& limit, Taia_t& time) {
	const auto& positions = services->m_ShootPositionHistory();
	if (positions.count < 2 || positions.count > 32 || positions.first < 0 || positions.first >= 32)
		return false;

	const Taia_t interval = TimeFromSeconds(interval_seconds);
	const Taia_t& oldest = positions.entries[positions.first].time;
	const int newest = (positions.first + positions.count - 1) % 32;
	Taia_t lower = oldest + interval;
	if (lower - interval < oldest)
		lower = lower + Taia_t(0, std::numeric_limits<float>::epsilon() * 2.f);

	lower = std::max(lower, limit - Taia_t(4, 0.f));
	const Taia_t upper = std::min(NewestPlayerTime(positions.entries[newest].time, interval_seconds), limit);
	if (lower > upper)
		return false;

	time = upper;
	return true;
}

bool C_Prediction::Update(CUserCommand* cmd) {
	if (Prediction->in_prediction || Prediction->true_view_enabled)
		return false;

	auto manager = ctx.controller->GetCommandManager();
	if (manager->command_number != cmd->command_number)
		return false;

	const auto old_command_number = manager->command_number;
	manager->command_number = cmd->command_number - 1;
	Prediction->Update(0);
	manager->command_number = old_command_number;

	return IsValidCommand(cmd);
}

bool C_Prediction::IsValidCommand(CUserCommand* cmd) {
	for (int i = 0; i < Prediction->prediction_contexts_size; ++i) {
		auto& context = Prediction->prediction_contexts[i];
		if (context.controller != ctx.controller)
			continue;

		if (context.predictable_count <= 0 || cmd->command_number <= 0)
			return false;

		if (cmd->command_number <= context.last_executed_command_number) {
			cvar(sv_late_commands_allowed);
			if (!Config::Get<bool>(Vars::Ragebot::Enable) || !Config::Get<bool>(Vars::Ragebot::DoublePrecision))
				return false;

			int late_limit = sv_late_commands_allowed ? sv_late_commands_allowed->GetInt() : 2;
			int late_commands = context.last_executed_command_number - cmd->command_number + 1;
			if (late_commands >= late_limit || !(ctx.pawn->m_fFlags() & FL_ONGROUND) || (cmd->button_state.held & IN_JUMP))
				return false;

			late_command = ctx.controller->m_nTickBase() == Prediction->last_server_tick;
			return late_command;
		}

		const int predicted_commands = cmd->command_number - context.last_executed_command_number - 1;
		return ctx.controller->m_nTickBase() == Prediction->last_server_tick + predicted_commands;
	}

	return false;
}

void C_Prediction::UpdateJumpTime() {
	m_flNextJumpFrac = 0.f;

	cvar(sv_jump_spam_penalty_time);
	if (!functions.FromGlobal || !sv_jump_spam_penalty_time)
		return;

	int tick = 0;
	functions.FromGlobal(&tick, ctx.controller->m_nTickBase(), ctx.pawn->m_pEntity()->m_worldGroupId());

	auto& jump = ctx.pawn->m_pMovementServices()->m_ModernJump();
	const float fraction = static_cast<float>(jump.m_nLastActualJumpPressTick() - tick) +
		jump.m_flLastActualJumpPressFrac() + sv_jump_spam_penalty_time->GetFloat() * 64.f;

	m_flNextJumpFrac = std::max(0.f, (std::floor(fraction * 64.f) + 1.f) / 64.f);
}

void C_Prediction::BackupData() {
	auto movement = ctx.pawn->m_pMovementServices();

	UpdateJumpTime();
	
	m_nGroundTick = ctx.pawn->m_hGroundEntity().Valid() ? ctx.controller->m_nTickBase() : -1;
	m_flGroundFrac = 0.f;
	m_nLandingTick = -1;
	m_flLandingFrac = 0.f;
	m_flLandingStartFrac = 0.f;
	m_flLandingEndFrac = 0.f;
	m_flLandingHeight = 0.f;
	m_bLandingDucked = false;
	m_bPredictUnducked = false;
	
	m_fFlags = ctx.pawn->m_fFlags();
	m_flDuckSpeed = movement->m_flDuckSpeed();
	m_vecVelocity = ctx.pawn->m_vecVelocity();
	m_vecAbsVelocity = ctx.pawn->m_vecAbsVelocity();
	m_nPostponeFireReadyTicks = ctx.weapon ? ctx.weapon->m_nPostponeFireReadyTicks() : 0;
	m_nNextSecondaryAttackTick = ctx.weapon ? ctx.weapon->m_nNextSecondaryAttackTick() : 0;

	flOldCurrentTime = GlobalVars->current_time;
	flOldFrameTime = GlobalVars->frame_time;
	flOldInterpolationAmount = GlobalVars->interpolation_amount;
	iOldTickCount = GlobalVars->tick_count;
	iOldCurrentThreadId = GlobalVars->current_thread_id;
	bOldInPrediction = Prediction->in_prediction;
}

void C_Prediction::RestoreData() {
	GlobalVars->current_time = flOldCurrentTime;
	GlobalVars->frame_time = flOldFrameTime;
	GlobalVars->interpolation_amount = flOldInterpolationAmount;
	GlobalVars->tick_count = iOldTickCount;
	GlobalVars->current_thread_id = iOldCurrentThreadId;

	Prediction->in_prediction = bOldInPrediction;
}

void C_Prediction::UpdateButtons(CUserCommand* cmd) {
	auto buttons = cmd->pb.mutable_base()->mutable_buttons_pb();
	buttons->set_buttonstate1(cmd->button_state.held);
	buttons->set_buttonstate2(cmd->button_state.toggle);
	buttons->set_buttonstate3(cmd->button_state.scroll);
}

void C_Prediction::StartCommand(CUserCommand* cmd) {
	UpdateButtons(cmd);
	ctx.pawn->m_pMovementServices()->SetPredictionCommand(cmd);
}

void C_Prediction::PerformPrediction(CUserCommand* cmd) {
	const int tick = late_command ? ctx.controller->m_nTickBase() : ++ctx.controller->m_nTickBase();

	GlobalVars->tick_count = tick;
	GlobalVars->current_time = TICKS_TO_TIME(tick);
	GlobalVars->frame_time = late_command ? 0.f : INTERVAL_PER_TICK;
	GlobalVars->interpolation_amount = 0.f;
	GlobalVars->current_thread_id = GetCurrentThreadId();

	Prediction->in_prediction = true;

	auto movement = ctx.pawn->m_pMovementServices();
	movement->BackupInputState();

	m_bPredictUnducked = Config::Get<bool>(Vars::Movement::JumpBug) && (cmd->button_state.held & IN_JUMP) && m_vecVelocity.z < 0.f && ((m_fFlags & FL_DUCKING) || (cmd->button_state.held & IN_DUCK));
	if (m_bPredictUnducked) {
		auto& buttons = movement->m_nButtons();
		buttons.held &= ~IN_DUCK;
		buttons.toggle &= ~IN_DUCK;
		buttons.scroll &= ~IN_DUCK;
	}

	oldMovementPlayer = *functions.movementPlayer;
	*functions.movementPlayer = ctx.pawn;

	movement->CheckMovingGround(GlobalVars->frame_time);

	moveData.Reset();
	moveData.has_zero_frametime = late_command;
	moveData.is_late_command = late_command;
	moveData.target_tick = tick;
	moveData.tickcount = tick - (late_command ? 0 : 1);
	moveData.subtick_start_fraction = 0.f;
	moveData.subtick_end_fraction = 1.f;

	movement->SetupMove(cmd, &moveData);
	movement->m_afButtonForced() = 0;

	bCaptureLanding = true;
	movement->DoMovement(cmd, &moveData);
	bCaptureLanding = false;

	movement->FinishMove(cmd, &moveData);
	movement->ProcessImpacts(cmd, &moveData);
	movement->RestoreInputState();
}

void C_Prediction::UpdateWeaponAccuracy() {
	if (!ctx.weapon || !ctx.weapon_data)
		return;

	float penalty = ctx.weapon->m_fAccuracyPenalty();
	float recoil = ctx.weapon->m_flRecoilIndex();
	float turning = ctx.weapon->m_flTurningInaccuracy();
	float turning_delta = ctx.weapon->m_flTurningInaccuracyDelta();
	Vector turning_direction = ctx.weapon->m_vecTurningInaccuracyEyeDirLast();
	float update_time = ctx.weapon->m_flLastAccuracyUpdateTime();

	if (ctx.weapon->UpdateAccuracyPenalty()) {
		weaponInaccuracy = ctx.weapon->GetInaccuracy();
		weaponSpread = ctx.weapon->GetSpread();
		weaponAccuracyValid = std::isfinite(weaponInaccuracy) && std::isfinite(weaponSpread) && weaponInaccuracy >= 0.f && weaponSpread >= 0.f;
		weaponAccuracyCommand = predictionCommand->command_number;
	}

	ctx.weapon->m_fAccuracyPenalty() = penalty;
	ctx.weapon->m_flRecoilIndex() = recoil;
	ctx.weapon->m_flTurningInaccuracy() = turning;
	ctx.weapon->m_flTurningInaccuracyDelta() = turning_delta;
	ctx.weapon->m_vecTurningInaccuracyEyeDirLast() = turning_direction;
	ctx.weapon->m_flLastAccuracyUpdateTime() = update_time;
}

void C_Prediction::FinishCommand() {
	auto movement = ctx.pawn->m_pMovementServices();
	movement->FinishInputState(predictionCommand);

	*functions.movementPlayer = oldMovementPlayer;
	movement->ResetPredictionCommand();
}

bool C_Prediction::Start(CUserCommand* cmd) {
	if (bStarted)
		return false;

	weaponAccuracyValid = false;
	weaponAccuracyCommand = -1;
	late_command = false;

	m_nShootHistoryIndex = -1;

	if (!Update(cmd))
		return false;

	auto weapon = ctx.pawn->m_pWeaponServices();
	if (!weapon || !functions.Init())
		return false;

	BackupData();
	predictionCommand = cmd;

	Movement->BeforePrediction(cmd);
	Ragebot->PrepareAutoStop();
	Ragebot->ApplyAutoStop(1);
	Movement->LegMovement(cmd, true);
	bStarted = true;
	StartCommand(cmd);

	const auto history = weapon->m_ShootPositionHistory();
	PerformPrediction(cmd);
	Movement->RestoreLegMovement(cmd);
	Ragebot->RestoreAutoStop(cmd);

	UpdateWeaponAccuracy();

	auto& jump = ctx.pawn->m_pMovementServices()->m_ModernJump();
	m_nLastLandedTick = jump.m_nLastLandedTick();
	m_flLastLandedFrac = jump.m_flLastLandedFrac();

	ctx.player_time = Taia_t(ctx.controller->m_nTickBase() + 1, 0.f);
	PlayerTime(weapon, ctx.pawn->GetInterpolationAmount(), ctx.player_time, ctx.player_time);

	Taia_t shoot_position_time = ctx.player_time;
	shoot_position_time = shoot_position_time.GetFractionalTime(ctx.pawn->GetInterpolationAmount() * 64.f);
	ctx.pawn->m_pWeaponServices()->InterpolateShootPosition(shoot_position_time, ctx.eye_position);

	return true;
}

void C_Prediction::End() {
	if (!bStarted)
		return;

	UpdateButtons(predictionCommand);

	const int mode = (predictionCommand->button_state.held & IN_ATTACK2) && !(predictionCommand->button_state.held & IN_ATTACK) ? 1 : 0;

	FinishCommand();
	RestoreData();

	predictionCommand = nullptr;
	bStarted = false;
}

void C_Prediction::CaptureLanding(CModernJump* jump, CMoveData* data) {
	if (!bCaptureLanding || m_nLandingTick != -1)
		return;

	if (jump != &ctx.pawn->m_pMovementServices()->m_ModernJump())
		return;

	float fraction = data->subtick_end_fraction;
	const float height = data->abs_origin.z - data->pre_air_move_position_z;
	const float velocity = data->pre_air_move_velocity_z;
	const float acceleration = data->pre_air_move_accelerate_z;
	const float discriminant = velocity * velocity + (acceleration + acceleration) * height;

	if (height < 0.f && velocity < 0.f && acceleration < 0.f && discriminant >= 0.f) {
		const float time = std::clamp((-velocity - std::sqrt(discriminant)) / acceleration, 0.f, GlobalVars->frame_time);
		fraction = data->subtick_start_fraction + time / INTERVAL_PER_TICK;
	}

	if (fraction < data->subtick_start_fraction || fraction > data->subtick_end_fraction)
		return;

	m_nLandingTick = data->tickcount;
	m_flLandingFrac = fraction;
	m_flLandingStartFrac = data->subtick_start_fraction;
	m_flLandingEndFrac = data->subtick_end_fraction;
	m_bLandingDucked = (ctx.pawn->m_fFlags() & FL_DUCKING) != 0;
}

void C_Prediction::CaptureGround(CCSPlayer_MovementServices* movement, CMoveData* data, CGameTrace* trace) {
	if (!bCaptureLanding || movement != ctx.pawn->m_pMovementServices())
		return;

	const bool grounded = trace && trace->entity;
	if (m_nLandingTick == -1)
		m_flLandingHeight = grounded ? data->abs_origin.z - trace->end_pos.z : 0.f;

	if (!grounded || m_nGroundTick == data->tickcount)
		return;

	m_nGroundTick = data->tickcount;
	m_flGroundFrac = data->subtick_end_fraction;
}

C_Prediction* EnginePrediction = new C_Prediction;
