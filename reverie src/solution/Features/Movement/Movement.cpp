#include <shared.h>
#include "Movement.h"
#include "../Prediction/Prediction.h"
#include "../../SDK/Globals.h"
#include <limits>

void CMovement::BeforePrediction(CUserCommand* cmd) {
	bunny_hop_command = -1;
	jump_bug_command = -1;
	boost_takeoff = false;

	if (!ctx.pawn->Alive())
		return;

	const auto move_type = ctx.pawn->m_nActualMoveType();
	if (move_type == MOVETYPE_LADDER || move_type == MOVETYPE_NOCLIP)
		return;

	const bool jump_bug = Config::Get<bool>(Vars::Movement::JumpBug);
	if (!jump_bug)
		jump_bug_pending_tick = -1;

	if (jump_bug && jump_bug_pending_tick == ctx.controller->m_nTickBase())
		JumpBug(cmd, true);

	if (EnginePrediction->m_nGroundTick == ctx.controller->m_nTickBase() && EnginePrediction->m_flGroundFrac == 0.f) {
		if (EnginePrediction->m_flNextJumpFrac == 0.f)
			BunnyHop(cmd, true);

		if (jump_bug)
			BoostTakeoff(cmd);
	}

	AirStrafer(cmd);
	this->FastStop(cmd);

	// advanced movement exploits
	this->Edgebug(cmd);
	this->Texturebug(cmd);
	this->Pixelsurf(cmd);
	this->MovementPrediction(cmd);

	if (jump_bug && !Config::Get<bool>(Vars::Movement::Strafer))
		PrepareJumpSubticks(cmd);
}

void CMovement::FastStop(CUserCommand* cmd) {
	constexpr uint64_t move_mask = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP;
	if (!Config::Get<bool>(Vars::Movement::FastStop) || !ctx.weapon || !ctx.weapon_data || this->PeekAssistState.returning ||
		ctx.pawn->m_nActualMoveType() != MOVETYPE_WALK || !(ctx.pawn->m_fFlags() & FL_ONGROUND) ||
		(ctx.pawn->m_fFlags() & FL_FROZEN) || (cmd->button_state.held & move_mask))
		return;

	auto base = cmd->pb.mutable_base();
	if (base->forwardmove() != 0.f || base->leftmove() != 0.f)
		return;

	for (const auto& step : base->subtick_moves()) {
		if ((step.button() & move_mask) || step.analog_forward_delta() != 0.f || step.analog_left_delta() != 0.f)
			return;
	}

	Vector velocity = ctx.pawn->m_vecVelocity();
	velocity.z = 0.f;
	const float speed = velocity.Length2d();
	if (speed <= 1.f)
		return;

	cvar(sv_friction);
	cvar(sv_stopspeed);
	cvar(sv_accelerate);
	cvar(sv_maxspeed);

	auto movement = ctx.pawn->m_pMovementServices();
	const int mode = std::clamp(ctx.weapon->m_weaponMode(), 0, 1);
	float max_speed = std::min(ctx.weapon_data->m_flMaxSpeed()[mode], sv_maxspeed->GetFloat());

	if (movement->m_flMaxspeed() > 0.f)
		max_speed = std::min(max_speed, movement->m_flMaxspeed());

	if ((ctx.pawn->m_fFlags() & FL_DUCKING) || (cmd->button_state.held & IN_DUCK))
		max_speed *= 0.34f;

	const float stamina = std::clamp(1.f - movement->m_flStamina() * 0.01f, 0.f, 1.f);
	const float friction = movement->m_flSurfaceFriction();
	const float remaining = std::max(0.f, speed - std::max(speed, sv_stopspeed->GetFloat()) * sv_friction->GetFloat() * friction * INTERVAL_PER_TICK);
	const float acceleration = sv_accelerate->GetFloat() * max_speed * stamina * stamina * friction * INTERVAL_PER_TICK;
	const float strength = acceleration > 0.f ? std::clamp(remaining / acceleration, 0.f, 1.f) : 0.f;
	const float yaw = base->viewangles().y() * Math::degs;
	const Vector direction = velocity * (-strength / speed);

	base->set_forwardmove(direction.x * std::cos(yaw) + direction.y * std::sin(yaw));
	base->set_leftmove(-direction.x * std::sin(yaw) + direction.y * std::cos(yaw));
}

void CMovement::BoostTakeoff(CUserCommand* cmd) {
	auto movement = ctx.pawn->m_pMovementServices();

	const bool jumping = bunny_hop_command == cmd->command_number || ((cmd->button_state.held & IN_JUMP) && !(movement->m_nButtons().held & IN_JUMP) && EnginePrediction->m_flNextJumpFrac == 0.f);

	if (!jumping || (cmd->button_state.held & IN_DUCK) || (EnginePrediction->m_fFlags & FL_DUCKING))
		return;

	const bool stable_ground = std::abs(EnginePrediction->m_vecVelocity.z) < 1.f && EnginePrediction->m_vecVelocity.Length2d() <= 260.f;
	if (!stable_ground)
		return;

	const bool duck_ready = movement->m_flDuckSpeed() >= 5.5f && movement->m_flDuckAmount() == 0.f && !movement->m_bDucking() && !movement->m_bDucked() && !movement->m_bDuckOverride() && !movement->m_duckUntilOnGround();

	if (!duck_ready || !Protobuf->ReserveSubtickMoveSteps(cmd, 2 + Config::Get<bool>(Vars::Ragebot::Enable), 0.f))
		return;

	auto press = Protobuf->AddSubtickMoveStep(cmd, 0.f);
	auto release = press ? Protobuf->AddSubtickMoveStep(cmd, 2.f / 64.f) : nullptr;
	if (!press || !release)
		return;

	press->set_button(IN_DUCK);
	press->set_pressed(true);
	release->set_button(IN_DUCK);
	release->set_pressed(false);

	boost_takeoff = true;
}

void CMovement::PrepareJumpSubticks(CUserCommand* cmd) {
	if (!(cmd->button_state.held & IN_JUMP))
		return;

	if (EnginePrediction->m_vecVelocity.z >= 0.f && !boost_takeoff)
		return;

	const int ticks = std::clamp(32 - cmd->pb.base().subtick_moves_size() - 4, 0, 32) & ~1;
	const float delta = std::numeric_limits<float>::min();

	for (int i = 0; i < ticks; ++i) {
		auto subtick = Protobuf->AddSubtickMoveStep(cmd, GetSubtickFraction(i, ticks));
		if (!subtick)
			break;

		subtick->set_analog_forward_delta((i & 1) ? -delta : delta);
	}
}

float CMovement::GetSubtickWhen(CUserCommand* cmd, float when, bool quantize) {
	const float step_when = quantize ? std::nearbyint(when * 64.f) / 64.f : when;

	for (int i = 0; i < cmd->pb.base().subtick_moves_size(); ++i) {
		const float input_when = cmd->pb.base().subtick_moves(i).when();
		if (std::nearbyint(input_when * 64.f) / 64.f == step_when)
			when = std::max(when, input_when);
	}

	return when;
}

bool CMovement::PressJump(CUserCommand* cmd, float when) {
	auto release = Protobuf->AddSubtickMoveStep(cmd, when);
	if (!release)
		return false;

	release->set_button(IN_JUMP);
	release->set_pressed(false);

	auto press = Protobuf->AddSubtickMoveStep(cmd, when);
	if (!press)
		return false;

	press->set_button(IN_JUMP);
	press->set_pressed(true);

	bunny_hop_command = cmd->command_number;
	return true;
}

void CMovement::BunnyHop(CUserCommand* cmd, bool before_prediction) {
	if (!Config::Get<bool>(Vars::Movement::Bunnyhop))
		return;

	if (bunny_hop_command == cmd->command_number)
		return;

	if (!(cmd->button_state.held & IN_JUMP))
		return;

	const int tick = ctx.controller->m_nTickBase() - (before_prediction ? 0 : 1);
	if (jump_bug_pending_tick == tick + 1)
		return;

	int land_tick = EnginePrediction->m_nLastLandedTick;
	float when = EnginePrediction->m_flLastLandedFrac;

	if (EnginePrediction->m_nGroundTick == tick && EnginePrediction->m_flGroundFrac == 0.f) {
		land_tick = tick;
		when = 0.f;
	} else {
		if (EnginePrediction->m_bPredictUnducked && (cmd->button_state.held & IN_DUCK) && !(ctx.pawn->m_fFlags() & FL_DUCKING))
			return;

		if (land_tick < tick && EnginePrediction->m_nGroundTick == tick) {
			land_tick = tick;
			when = EnginePrediction->m_flGroundFrac;
		}
	}

	if (land_tick != tick)
		return;

	if (when < 0.f || when >= 1.f)
		return;

	if (EnginePrediction->m_flNextJumpFrac >= 1.f)
		return;

	if (!Protobuf->ReserveSubtickMoveSteps(cmd, 2 + Config::Get<bool>(Vars::Ragebot::Enable), when))
		return;

	when = GetSubtickWhen(cmd, std::max(when, EnginePrediction->m_flNextJumpFrac));

	PressJump(cmd, when);
}

void CMovement::JumpBug(CUserCommand* cmd, bool before_prediction) {
	if (!Config::Get<bool>(Vars::Movement::JumpBug))
		return;

	auto movement = ctx.pawn->m_pMovementServices();

	if (jump_bug_command == cmd->command_number)
		return;

	const int tick = ctx.controller->m_nTickBase() - (before_prediction ? 0 : 1);
	const bool pending = jump_bug_pending_tick == tick;
	if (jump_bug_pending_tick != tick)
		jump_bug_pending_tick = -1;

	if (!(cmd->button_state.held & IN_JUMP))
		return;

	const bool ducked = (EnginePrediction->m_fFlags & FL_DUCKING) != 0;
	float fraction = EnginePrediction->m_flLandingFrac;
	float when = EnginePrediction->m_flLandingEndFrac;

	if (before_prediction) {
		if (!pending || !ducked || EnginePrediction->m_nGroundTick != tick || EnginePrediction->m_flGroundFrac != 0.f)
			return;

		when = EnginePrediction->m_flNextJumpFrac;
		fraction = when;
		if (when >= 1.f)
			return;
	} else {
		if (EnginePrediction->m_vecVelocity.z >= 0.f)
			return;

		if (EnginePrediction->m_nGroundTick == tick && EnginePrediction->m_flGroundFrac == 0.f)
			return;

		if (EnginePrediction->m_nLandingTick != tick)
			return;

		if (EnginePrediction->m_flLandingHeight <= 0.f)
			when = (std::ceil(fraction * 64.f) - 1.f) / 64.f;

		if (pending && EnginePrediction->m_flLandingStartFrac == 0.f)
			when = 0.f;
	}

	if (movement->m_bDuckOverride() || EnginePrediction->m_bLandingDucked)
		return;

	if (!ducked && EnginePrediction->m_flDuckSpeed < 3.5f)
		return;

	if (when < 0.f || when > 1.f || when < EnginePrediction->m_flLandingStartFrac)
		return;

	if (when < EnginePrediction->m_flNextJumpFrac)
		return;

	if (!ducked && when == 0.f)
		return;

	const int buttons = (when == 1.f ? 0 : 3) + ((cmd->button_state.held & IN_DUCK) ? 0 : 1);
	if (!Protobuf->ReserveSubtickMoveSteps(cmd, buttons + Config::Get<bool>(Vars::Ragebot::Enable), fraction))
		return;

	if (!(cmd->button_state.held & IN_DUCK)) {
		auto subtick_move = Protobuf->AddSubtickMoveStep(cmd, 0.f);
		if (!subtick_move)
			return;

		subtick_move->set_button(IN_DUCK);
		subtick_move->set_pressed(true);
	}

	if (when == 1.f) {
		jump_bug_pending_tick = tick + 1;
		return;
	}

	when = GetSubtickWhen(cmd, when, false);

	auto release = Protobuf->AddSubtickMoveStep(cmd, when);
	if (!release)
		return;

	release->set_button(IN_DUCK);
	release->set_pressed(false);

	if (!PressJump(cmd, when))
		return;

	jump_bug_command = cmd->command_number;
	jump_bug_pending_tick = -1;
}

void CMovement::AirStrafer(CUserCommand* cmd) {
	if (!Config::Get<bool>(Vars::Movement::Strafer))
		return;

	if (!Cvar || !ctx.weapon_data || !ctx.pawn->Alive())
		return;

	const auto move_type = ctx.pawn->m_nActualMoveType();
	if (move_type == MOVETYPE_LADDER || move_type == MOVETYPE_NOCLIP)
		return;

	auto movement = ctx.pawn->m_pMovementServices();
	if (EnginePrediction->m_fFlags & FL_ONGROUND) {
		PeekAssistState.stop_latch = false;
		if (bunny_hop_command != cmd->command_number && !boost_takeoff)
			return;
	}

	if (ctx.cmd->button_state.held & IN_SPRINT) {
		this->m_bShouldStuck = true;
		return;
	}

	const bool peek_returning = false;

	if (!peek_returning) {
		PeekAssistState.stop_latch = false;
	} else {
		if (PeekAssistState.stop_latch) {
			this->m_bShouldStuck = true;
			return;
		}

		auto node = ctx.pawn->m_pGameSceneNode();
		if (!node)
			return;

		Vector current_pos = node->m_vecAbsOrigin();
		Vector to_target = PeekAssistState.origin - current_pos;
		to_target.z = 0.f;

		float dist_to_target = to_target.Length2d();

		bool should_latch = dist_to_target < 2.0f;

		if (!should_latch) {
			Vector vel2d = EnginePrediction->m_vecVelocity;
			vel2d.z = 0.f;

			Vector dir_to_target = to_target;
			dir_to_target.Normalize();

			float closing_speed = vel2d.Dot(dir_to_target);
			float predicted_travel = closing_speed * INTERVAL_PER_TICK;

			should_latch = (closing_speed > 0.f && predicted_travel >= dist_to_target) || (closing_speed < 0.f && dist_to_target < 15.0f);
		}

		if (should_latch) {
			PeekAssistState.stop_latch = true;
			this->m_bShouldStuck = true;
			return;
		}
	}

	cvar(sv_airaccelerate);
	cvar(sv_air_max_wishspeed);
	cvar(sv_quantize_movement_input);
	cvar(sv_maxspeed);
	cvar(sv_gravity);
	cvar(sv_staminarecoveryrate);
	cvar(sv_enablebunnyhopping);

	if (!sv_airaccelerate || !sv_air_max_wishspeed || !sv_quantize_movement_input || !sv_maxspeed || !sv_gravity || !sv_staminarecoveryrate || !sv_enablebunnyhopping)
		return;

	auto base = cmd->pb.mutable_base();
	const auto buttons = cmd->button_state.held;

	const int ticks = std::clamp(32 - base->subtick_moves_size() - Config::Get<bool>(Vars::Ragebot::Enable), 0, 32);
	if (!ticks)
		return;

	static uint64_t last_buttons = 0;
	static uint64_t last_pressed = 0;
	static bool side_switch = false;

	auto check_button = [&](uint64_t button, uint64_t opposite) {
		if (!(buttons & button))
			last_pressed &= ~button;
		else if (!(last_buttons & button) || !(last_pressed & opposite))
			last_pressed = (last_pressed & ~opposite) | button;
	};

	check_button(IN_MOVELEFT, IN_MOVERIGHT);
	check_button(IN_MOVERIGHT, IN_MOVELEFT);
	check_button(IN_FORWARD, IN_BACK);
	check_button(IN_BACK, IN_FORWARD);
	last_buttons = buttons;

	const auto previous_buttons = movement->m_nButtons().held;
	Vector2D last_move((previous_buttons & (IN_FORWARD | IN_BACK)) ? movement->m_flCmdForwardMove() : 0.f, (previous_buttons & (IN_MOVELEFT | IN_MOVERIGHT)) ? movement->m_flCmdLeftMove() : 0.f);

	float yaw = 0.f;

	if (peek_returning) {
		Vector current_pos = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();
		Vector to_target = PeekAssistState.origin - current_pos;
		to_target.z = 0.f;
		to_target.Normalize();

		yaw = std::remainderf(Math::Rad2Deg(std::atan2(to_target.y, to_target.x)), 360.f);
	} else {
		float offset = 0.f;
		if (last_pressed & IN_MOVELEFT)
			offset = 90.f;
		else if (last_pressed & IN_MOVERIGHT)
			offset = -90.f;

		if (last_pressed & IN_FORWARD)
			offset *= 0.5f;
		else if (last_pressed & IN_BACK)
			offset = -offset * 0.5f + 180.f;

		yaw = std::remainderf(this->angle.y + offset, 360.f);
	}

	const int weapon_mode = ctx.weapon->m_weaponMode();
	float move_speed = std::min(sv_maxspeed->GetFloat(), 260.f);

	if (movement->m_flMaxspeed() > 0.f)
		move_speed = std::min(move_speed, movement->m_flMaxspeed());

	move_speed = std::min(move_speed, ctx.weapon_data->m_flMaxSpeed()[weapon_mode >= 0 && weapon_mode < 2 ? weapon_mode : 0]);
	if ((ctx.pawn->m_fFlags() & FL_DUCKING) || (buttons & IN_DUCK))
		move_speed *= 0.34f;

	const float air_max_wishspeed = sv_air_max_wishspeed->GetFloat();
	const float air_accel = sv_airaccelerate->GetFloat();
	const float gravity = sv_gravity->GetFloat() * ctx.pawn->m_flActualGravityScale();
	const float stamina_recovery = sv_staminarecoveryrate->GetFloat();
	const bool quantize = sv_quantize_movement_input->GetBool();

	Vector velocity = EnginePrediction->m_vecVelocity;
	if ((EnginePrediction->m_fFlags & FL_ONGROUND) && !sv_enablebunnyhopping->GetBool()) {
		const float limit = movement->m_flMaxspeed() * 1.1f;
		const float speed = velocity.Length();
		if (limit > 0.f && speed > limit)
			velocity *= limit / speed;
	}

	float surface_friction = movement->m_flSurfaceFriction();
	float stamina = movement->m_flStamina();
	float last_yaw_delta = 0.f;

	int i = 0;
	for (; i < ticks; ++i) {
		const float when = GetSubtickFraction(i, ticks);
		const float next_when = GetSubtickFraction(i + 1, ticks);
		const float frame_time = (std::nearbyint(next_when * 64.f) - std::nearbyint(when * 64.f)) / 4096.f;
		const float stamina_scale = std::clamp(1.f - stamina / 100.f, 0.f, 1.f);
		const float wish_speed = move_speed * stamina_scale * stamina_scale;
		const float max_wishspeed = std::min(wish_speed, air_max_wishspeed);
		const float accel_term = air_accel * wish_speed * surface_friction * frame_time;
		const float speed = velocity.Length2d();
		const float velocity_angle = std::remainder(Math::Rad2Deg(std::atan2(velocity.y, velocity.x)), 360.f);
		const float velocity_delta = std::remainder(yaw - velocity_angle, 360.f);
		float wish_yaw = yaw;

		if (speed > 0.1f) {
			const float cos_angle = (max_wishspeed - accel_term) / speed;
			if (cos_angle < 1.f) {
				const float ideal_angle = Math::Rad2Deg(std::acos(std::max(cos_angle, 0.f)));
				if (std::abs(velocity_delta) < 0.1f)
					wish_yaw = velocity_angle + (side_switch ? ideal_angle : -ideal_angle);
				else
					wish_yaw = velocity_angle + (velocity_delta > 0.f ? ideal_angle : -ideal_angle);
			}
		}

		const float wish_radians = Math::Deg2Rad(wish_yaw);
		Vector wish_dir(std::cos(wish_radians), std::sin(wish_radians), 0.f);
		const float wish_length = wish_dir.Length2d();

		if (wish_length >= 0.001f) {
			wish_dir /= wish_length;
			const float add_speed = max_wishspeed - velocity.Dot(wish_dir);
			if (add_speed > 0.f)
				velocity += wish_dir * std::min(accel_term, add_speed);
		}

		const float move_yaw = quantize ? std::remainder(ctx.base_cmd->viewangles().y(), 360.f) : wish_yaw;
		const float yaw_delta = quantize ? std::remainder(wish_yaw - move_yaw, 360.f) : 0.f;
		const float rotation = Math::Deg2Rad(ctx.base_cmd->viewangles().y() - std::remainder(move_yaw, 360.f));
		float forward = std::clamp(std::cos(rotation), -1.f, 1.f);
		float left = std::clamp(-std::sin(rotation), -1.f, 1.f);

		if (std::fabsf(forward) < 0.001f)
			forward = 0.f;

		if (std::fabsf(left) < 0.001f)
			left = 0.f;

		auto subtick = Protobuf->AddSubtickMoveStep(cmd, when);
		if (!subtick)
			break;

		subtick->set_analog_forward_delta(forward - last_move.x);
		subtick->set_analog_left_delta(left - last_move.y);
		if (yaw_delta != last_yaw_delta)
			subtick->set_yaw_delta(yaw_delta - last_yaw_delta);

		last_move = { forward, left };
		last_yaw_delta = yaw_delta;
		velocity.z -= gravity * frame_time;
		surface_friction = velocity.z > 0.f && velocity.z <= 140.f ? 0.25f : 1.f;
		stamina = std::max(0.f, stamina - stamina_recovery * frame_time);
		side_switch = !side_switch;
	}

	if (!i)
		return;

	base->set_forwardmove(last_move.x);
	base->set_leftmove(last_move.y);

	cmd->button_state.SetButton(IN_FORWARD, last_move.x > 0.f);
	cmd->button_state.SetButton(IN_BACK, last_move.x < 0.f);
	cmd->button_state.SetButton(IN_MOVELEFT, last_move.y > 0.f);
	cmd->button_state.SetButton(IN_MOVERIGHT, last_move.y < 0.f);
}

void CMovement::AirStuck(CUserCommand* cmd) {
	cvar(sv_airaccelerate);

	if (auto mt = ctx.pawn->m_nActualMoveType(); mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER)
		return;

	if ((EnginePrediction->m_fFlags & FL_ONGROUND) || !this->m_bShouldStuck) {
		this->m_bShouldStuck = false;
		return;
	}

	Vector velocity = EnginePrediction->m_vecAbsVelocity;
	velocity.z = 0.f;

	float speed = velocity.Length2d();
	if (speed < 1.0f) {
		cmd->pb.mutable_base()->set_forwardmove(0.f);
		cmd->pb.mutable_base()->set_leftmove(0.f);

		const auto move_mask = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;
		cmd->button_state.held &= ~move_mask;
		cmd->button_state.toggle &= ~move_mask;
		cmd->button_state.scroll &= ~move_mask;

		this->m_bShouldStuck = false;
		return;
	}

	Vector wish_dir = Vector(-velocity.x / speed, -velocity.y / speed, 0.f);

	float ideal = (std::max(ctx.weapon_data->m_flMaxSpeed()[0], 10.0f)) * (sv_airaccelerate->GetFloat() * ctx.pawn->m_pMovementServices()->m_flSurfaceFriction() * INTERVAL_PER_TICK);

	float ratio = 1.0f;
	if (speed < ideal && ideal > 0.0f)
		ratio = std::clamp(speed / ideal, 0.0f, 1.0f);

	float yaw = Math::Deg2Rad(cmd->pb.mutable_base()->viewangles().y());
	Vector forward = Vector(std::cosf(yaw), std::sinf(yaw), 0.f);
	Vector left = Vector(std::sinf(yaw), -std::cosf(yaw), 0.f);

	float fmove = wish_dir.Dot(forward) * ratio;
	float lmove = wish_dir.Dot(left) * ratio;

	cmd->pb.mutable_base()->set_forwardmove(fmove);
	cmd->pb.mutable_base()->set_leftmove(-lmove);

	const auto move_mask = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;
	cmd->button_state.held &= ~move_mask;
	cmd->button_state.toggle &= ~move_mask;
	cmd->button_state.scroll &= ~move_mask;

	if (fmove > 0.01f)
		cmd->button_state.held |= IN_FORWARD;
	else if (fmove < -0.01f)
		cmd->button_state.held |= IN_BACK;

	if (-lmove > 0.01f)
		cmd->button_state.held |= IN_MOVELEFT;
	else if (-lmove < -0.01f)
		cmd->button_state.held |= IN_MOVERIGHT;

	this->m_bShouldStuck = false;
}

void CMovement::FastLadder(CUserCommand* cmd) {
	if (!Config::Get<bool>(Vars::Movement::FastLadder))
		return;

	if (auto mt = ctx.pawn->m_nActualMoveType(); mt != MOVETYPE_LADDER)
		return;

	auto btns = cmd->button_state.held;
	const float fwd = cmd->pb.mutable_base()->forwardmove();

	bool wanna_up = (btns & IN_FORWARD) && fwd > 0.01f;
	bool wanna_down = (btns & IN_BACK) && fwd < -0.01f;

	if (!wanna_up && !wanna_down)
		return;

	auto modified = this->angle;
	modified.x = 89.0f;

	btns &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);

	if (wanna_up) {
		modified.y -= 90.0f;
		cmd->pb.mutable_base()->set_forwardmove(-1.0f);
		cmd->pb.mutable_base()->set_leftmove(1.0f);
		btns |= (IN_MOVELEFT | IN_BACK);
	} else if (wanna_down) {
		modified.y += 90.0f;
		cmd->pb.mutable_base()->set_forwardmove(1.0f);
		cmd->pb.mutable_base()->set_leftmove(1.0f);
		btns |= (IN_MOVELEFT | IN_FORWARD);
	}

	Math::NormalizeYaw(modified.y);

	cmd->button_state.held = btns;

	cmd->pb.mutable_base()->mutable_viewangles()->set_x(modified.x);
	cmd->pb.mutable_base()->mutable_viewangles()->set_y(modified.y);
}

void CMovement::ResetPeekAssist() {
	PeekAssistState.active = false;
	PeekAssistState.returning = false;
	PeekAssistState.has_moved = false;
	PeekAssistState.was_firing = false;
	PeekAssistState.stall_ticks = 0;
	PeekAssistState.best_distance = 0.f;
	PeekAssistState.origin = {};
	PeekAssistState.pending_shot_return = false;
}

void CMovement::PeekAssist(CUserCommand* cmd) {
	if (!cmd || !ctx.pawn || !ctx.pawn->Alive() || !Config::Get<bool>(Vars::Movement::PeekAssist)) {
		this->ResetPeekAssist();
		return;
	}

	const auto mt = ctx.pawn->m_nActualMoveType();
	if (mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER) {
		this->ResetPeekAssist();
		return;
	}

	auto node = ctx.pawn->m_pGameSceneNode();
	if (!node) {
		this->ResetPeekAssist();
		return;
	}

	const Vector current_origin = node->m_vecAbsOrigin();

	if (!PeekAssistState.active) {
		PeekAssistState.origin = current_origin;
		PeekAssistState.active = true;
		PeekAssistState.returning = false;
		PeekAssistState.has_moved = false;
		PeekAssistState.pending_shot_return = false;
		PeekAssistState.stall_ticks = 0;
		PeekAssistState.best_distance = 0.f;
	}

	if (!PeekAssistState.returning) {
		const float max_dist = Config::Get<float>(Vars::Movement::PeekAssistMaxDistance);

		if (max_dist > 0.f) {
			Vector diff = current_origin - PeekAssistState.origin;
			diff.z = 0.f;

			const float cur_dist = diff.Length2d();

			if (cur_dist > max_dist) {
				diff.Normalize();

				PeekAssistState.origin = current_origin - diff * max_dist;
				PeekAssistState.origin.z = current_origin.z;
			}
		}
	}

	const bool is_firing = ctx.weapon && ctx.weapon->IsFiring();
	const bool shot_triggered = is_firing && !PeekAssistState.was_firing;
	PeekAssistState.was_firing = is_firing;

	constexpr uint64_t move_mask = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;

	const bool moving_now = (cmd->button_state.held & move_mask) != 0;

	if (moving_now)
		PeekAssistState.has_moved = true;

	auto types = Config::Get<std::vector<int>>(Vars::Movement::PeekAssistTypes);

	const bool on_key_release = types.at(peek_assist_types_on_key_release) != 0;

	const bool on_shot = types.at(peek_assist_types_on_shot) != 0;

	if (!PeekAssistState.returning) {
		bool start_return = false;

		if (PeekAssistState.pending_shot_return) {
			PeekAssistState.pending_shot_return = false;
			start_return = true;
		}

		if (on_key_release && PeekAssistState.has_moved && !moving_now)
			start_return = true;

		if (on_shot && shot_triggered)
			PeekAssistState.pending_shot_return = true;

		if (start_return) {
			PeekAssistState.returning = true;
			PeekAssistState.best_distance = (PeekAssistState.origin - current_origin).Length2d();
			PeekAssistState.stall_ticks = 0;
		}
	}

	if (!PeekAssistState.returning)
		return;

	Vector delta = PeekAssistState.origin - current_origin;
	delta.z = 0.f;

	const float dist = delta.Length2d();

	if (dist < PeekAssistState.best_distance - 0.5f) {
		PeekAssistState.best_distance = dist;
		PeekAssistState.stall_ticks = 0;
	} else {
		PeekAssistState.stall_ticks++;
	}

	const bool on_ground = (ctx.pawn->m_fFlags() & FL_ONGROUND) != 0;

	if ((on_ground && dist < 10.f) || PeekAssistState.stall_ticks > 64) {
		PeekAssistState.returning = false;
		PeekAssistState.has_moved = false;
		PeekAssistState.pending_shot_return = false;
		PeekAssistState.stall_ticks = 0;

		cmd->pb.mutable_base()->set_forwardmove(0.f);
		cmd->pb.mutable_base()->set_leftmove(0.f);

		cmd->button_state.held &= ~move_mask;
		cmd->button_state.toggle &= ~move_mask;
		cmd->button_state.scroll &= ~move_mask;
		return;
	}

	if (!on_ground)
		return;

	delta.Normalize();

	const auto& base = cmd->pb.base();
	Vector move_angle(base.viewangles().x(), base.viewangles().y(), 0.f);
	if (base.subtick_moves_size() && base.subtick_moves(base.subtick_moves_size() - 1).when() == 1.f) {
		for (const auto& step : base.subtick_moves()) {
			move_angle.x += step.pitch_delta();
			move_angle.y += step.yaw_delta();
		}
	}

	Vector forward, right;
	Math::Angle2Vectors(move_angle, &forward, &right);
	forward.z = 0.f;
	right.z = 0.f;
	forward.Normalize();
	right.Normalize();

	float fmove = delta.Dot(forward);
	float lmove = delta.Dot(right);

	const float slow_factor = std::clamp(dist / 32.f, 0.3f, 1.f);

	fmove *= slow_factor;
	lmove *= slow_factor;

	cmd->pb.mutable_base()->set_forwardmove(fmove);
	cmd->pb.mutable_base()->set_leftmove(-lmove);

	cmd->button_state.held &= ~move_mask;
	cmd->button_state.toggle &= ~move_mask;
	cmd->button_state.scroll &= ~move_mask;

	if (fmove > 0.01f)
		cmd->button_state.held |= IN_FORWARD;
	else if (fmove < -0.01f)
		cmd->button_state.held |= IN_BACK;

	if (-lmove > 0.01f)
		cmd->button_state.held |= IN_MOVELEFT;
	else if (-lmove < -0.01f)
		cmd->button_state.held |= IN_MOVERIGHT;
}

void CMovement::OnPresent() {
	const bool should_show = Config::Get<bool>(Vars::Movement::PeekAssist) && ctx.pawn && ctx.pawn->Alive() && this->PeekAssistState.active && !this->PeekAssistState.origin.Zero();

	this->PeekAssistState.animation = std::clamp(this->PeekAssistState.animation + (should_show ? 1.f : -1.f) * 6.f * ImGui::GetIO().DeltaTime, 0.f, 1.f);
	if (this->PeekAssistState.animation <= 0.f || !ctx.pawn || !ctx.pawn->Alive())
		return;

	static Vector origin{};
	if (should_show)
		origin = this->PeekAssistState.origin;

	const Vector2D center = Draw->WorldToScreen(origin);
	if (center.x == -1.f && center.y == -1.f)
		return;

	const float animation = this->PeekAssistState.animation * this->PeekAssistState.animation * (3.f - 2.f * this->PeekAssistState.animation);
	const float radius = 20.f * animation;
	constexpr int segments = 48;
	ImVec2 points[segments];

	for (int i = 0; i < segments; ++i) {
		const float angle = Math::_pi2 * static_cast<float>(i) / static_cast<float>(segments);
		const Vector point{ origin.x + std::cos(angle) * radius, origin.y + std::sin(angle) * radius, origin.z };
		const Vector2D screen = Draw->WorldToScreen(point);
		if (screen.x == -1.f && screen.y == -1.f)
			return;

		points[i] = { screen.x, screen.y };
	}

	const auto color = Config::Get<Color_t>(Vars::Movement::PeekAssistColor);
	const ImU32 inner = IM_COL32(color.R(), color.G(), color.B(), static_cast<uint8_t>(color.A() * animation));
	const ImU32 outer = IM_COL32(color.R(), color.G(), color.B(), 0);
	auto draw_list = ImGui::GetBackgroundDrawList();
	for (int i = 0; i < segments; ++i)
		draw_list->AddTriangleFilledMulticolor({ center.x, center.y }, points[i], points[(i + 1) % segments], inner, outer, outer);
}

void CMovement::LegMovement(CUserCommand* cmd, bool prediction) {
	if (!cmd || !ctx.pawn || !ctx.pawn->Alive()) {
		this->leg_movement_pawn = nullptr;
		this->leg_movement_predicted = false;
		return;
	}

	const bool enabled = Config::Get<bool>(Vars::Movement::LegMovement);
	const bool restore = this->leg_movement_pawn == ctx.pawn;

	if (!prediction)
		this->leg_movement_pawn = nullptr;

	if (!enabled && !restore)
		return;

	auto movement = ctx.pawn->m_pMovementServices();
	auto base = cmd->pb.mutable_base();

	if (!movement)
		return;

	if (prediction) {
		this->leg_movement_predicted = false;
		this->leg_movement_impulses = movement->m_vecLastMovementImpulses();

		if (base->subtick_moves_size())
			return;
	}

	for (const auto& step : base->subtick_moves()) {
		const bool analog = std::abs(step.analog_forward_delta()) > std::numeric_limits<float>::min() ||
			std::abs(step.analog_left_delta()) > std::numeric_limits<float>::min();

		if ((step.button() & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)) || (!step.button() && analog))
			return;
	}

	Vector input{ base->forwardmove(), base->leftmove(), 0.0f };
	const auto buttons = movement->m_nButtons().held;
	const float previous_forward = (buttons & (IN_FORWARD | IN_BACK)) ? movement->m_flCmdForwardMove() : 0.0f;
	const float previous_left = (buttons & (IN_MOVELEFT | IN_MOVERIGHT)) ? movement->m_flCmdLeftMove() : 0.0f;
	const Vector delta{ input.x - previous_forward, input.y - previous_left, 0.0f };

	cvar(sv_subtick_movement_view_angles);

	constexpr uint64_t blocked_buttons = IN_JUMP | IN_DUCK | IN_ATTACK | IN_ATTACK2;
	const auto flags = ctx.pawn->m_fFlags();
	const bool walking = ctx.pawn->m_nActualMoveType() == MOVETYPE_WALK && (flags & (FL_ONGROUND | FL_DUCKING)) == FL_ONGROUND;
	bool sliding = enabled && walking && movement->m_flDuckAmount() == 0.0f && !(cmd->button_state.held & blocked_buttons) &&
		!base->subtick_moves_size() && !input.Zero() && sv_subtick_movement_view_angles && sv_subtick_movement_view_angles->GetBool();

	if (!sliding && !restore)
		return;

	Vector checked = this->leg_movement_impulses + delta;
	Vector endpoint = checked - input * 2.0f;

	if (!checked.Valid() || std::abs(checked.x) > 2.0f || std::abs(checked.y) > 2.0f)
		return;

	sliding &= endpoint.Valid() && std::abs(endpoint.x) <= 2.0f && std::abs(endpoint.y) <= 2.0f;

	auto start = Protobuf->AddSubtickMoveStep(cmd, 0.0f);
	if (!start)
		return;

	start->set_analog_forward_delta(delta.x);
	start->set_analog_left_delta(delta.y);

	if (prediction) {
		this->leg_movement_input = input;
		this->leg_movement_predicted = true;
	}

	if (!sliding)
		return;

	auto reverse = Protobuf->AddSubtickMoveStep(cmd, 63.0f / 64.0f);
	auto finish = reverse ? Protobuf->AddSubtickMoveStep(cmd, 1.0f) : nullptr;

	if (!reverse || !finish)
		return;

	reverse->set_analog_forward_delta(-input.x * 2.0f);
	reverse->set_analog_left_delta(-input.y * 2.0f);
	finish->set_yaw_delta(180.0f);

	if (input.x != 0.0f)
		base->set_forwardmove(-input.x);
	if (input.y != 0.0f)
		base->set_leftmove(-input.y);

	if (!prediction)
		this->leg_movement_pawn = ctx.pawn;
}

void CMovement::RestoreLegMovement(CUserCommand* cmd) {
	if (!this->leg_movement_predicted)
		return;

	auto base = cmd->pb.mutable_base();
	base->clear_subtick_moves();

	if (this->leg_movement_input.x != 0.0f)
		base->set_forwardmove(this->leg_movement_input.x);
	if (this->leg_movement_input.y != 0.0f)
		base->set_leftmove(this->leg_movement_input.y);

	this->leg_movement_predicted = false;
}

void CMovement::Rotation() {
	if (auto mt = ctx.pawn->m_nActualMoveType(); mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER)
		return;

	Vector wish_angle = Vector(ctx.base_cmd->viewangles().x(), ctx.base_cmd->viewangles().y(), ctx.base_cmd->viewangles().z());
	float sign = wish_angle.x > 89.0f ? -1.0f : 1.0f;
	wish_angle.x = isfinite(wish_angle.x) ? remainder(wish_angle.x, 360.f) : 0.f;
	wish_angle.y = isfinite(wish_angle.y) ? remainder(wish_angle.y, 360.f) : 0.f;
	wish_angle.z = 0.0f;

	Vector forward, right, up;
	Math::Angle2Vectors(wish_angle, &forward, &right, &up);

	forward.z = 0.0f;
	right.z = 0.0f;
	up.x = 0.0f;
	up.y = 0.0f;

	forward.Normalize();
	right.Normalize();
	up.Normalize();

	Vector old_forward, old_right, old_up;
	Math::Angle2Vectors(this->angle, &old_forward, &old_right, &old_up);

	old_forward.z = 0.0f;
	old_right.z = 0.0f;
	old_up.x = 0.0f;
	old_up.y = 0.0f;

	old_forward.Normalize();
	old_right.Normalize();
	old_up.Normalize();

	const Vector scaled_forward = forward * ctx.base_cmd->forwardmove();
	const Vector scaled_right = right * ctx.base_cmd->leftmove();
	const Vector scaled_up = up * ctx.base_cmd->upmove();

	float fixed_forward = sign * (old_forward.Dot(scaled_right) + old_forward.Dot(scaled_forward));
	float fixed_side = old_right.Dot(scaled_right) + old_right.Dot(scaled_forward);
	float fixed_up = old_up.Dot(scaled_right) + old_up.Dot(scaled_forward) + old_up.Dot(scaled_up);

	ctx.base_cmd->set_forwardmove(std::clamp(fixed_forward, -1.0f, 1.0f));
	ctx.base_cmd->set_leftmove(std::clamp(fixed_side, -1.0f, 1.0f));
	ctx.base_cmd->set_upmove(std::clamp(fixed_up, -1.0f, 1.0f));

	if (!(ctx.pawn->m_fFlags() & FL_ONGROUND))
		return;

	if (ctx.base_cmd->forwardmove() > 0.0f)
		ctx.cmd->button_state.held |= IN_FORWARD;
	else if (ctx.base_cmd->forwardmove() < 0.0f)
		ctx.cmd->button_state.held |= IN_BACK;

	if (ctx.base_cmd->leftmove() > 0.0f)
		ctx.cmd->button_state.held |= IN_MOVELEFT;
	else if (ctx.base_cmd->leftmove() < 0.0f)
		ctx.cmd->button_state.held |= IN_MOVERIGHT;
}


// ============================================================================
//  Edgebug � cancels fall damage when landing on surface edges
//  Adapted from IDA dump sub_1801F97B0 (�������.txt)
// ============================================================================
void CMovement::Edgebug(CUserCommand* cmd) {
	if (!Config::Get<bool>(Vars::Movement::Edgebug) || !ctx.pawn || !ctx.pawn->Alive() || !cmd)
		return;

	auto mt = ctx.pawn->m_nActualMoveType();
	if (mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER)
		return;

	if (ctx.pawn->m_fFlags() & FL_ONGROUND)
		return;

	Vector velocity = ctx.pawn->m_vecVelocity();
	if (velocity.z > -150.f)
		return;

	// track fall start
	if (m_flLastFallStart == 0.f) {
		m_flLastFallStart = EnginePrediction->curtime();
		m_vecFallStart = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	}

	// trace down to find ground distance
	auto filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);
	auto trace = Physics->MakeTrace();

	CRay ray{};
	Vector start = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	Vector end = start + Vector(0.f, 0.f, -64.f);
	ray.Init(start, end);

	Physics->TraceRay(&ray, &start, &end, &filter, &trace);

	if (trace.fraction >= 1.f)
		return; // no ground nearby

	float ground_dist = trace.fraction * 64.f;
	if (ground_dist > 80.f)
		return; // too far from ground

	// check if surface normal is steep (edge)
	Vector normal = trace.hit_normal;
	if (std::abs(normal.z) > 0.7f)
		return; // flat ground, not an edge

	// edgebug: force jump at the right moment to cancel fall
	if (!m_bEdgebugActive && ground_dist < 16.f) {
		m_bEdgebugActive = true;

		// apply jump via subtick
		if (Protobuf->ReserveSubtickMoveSteps(cmd, 2 + Config::Get<bool>(Vars::Ragebot::Enable), 0.f)) {
			auto press = Protobuf->AddSubtickMoveStep(cmd, 0.f);
			if (press) {
				press->set_button(IN_JUMP);
				press->set_pressed(true);
			}
			auto release = press ? Protobuf->AddSubtickMoveStep(cmd, 1.f / 64.f) : nullptr;
			if (release) {
				release->set_button(IN_JUMP);
				release->set_pressed(false);
			}
		}
	}

	if (m_bEdgebugActive && (ctx.pawn->m_fFlags() & FL_ONGROUND)) {
		m_bEdgebugActive = false;
		m_flLastFallStart = 0.f;
		m_vecFallStart = {};
	}
}

// ============================================================================
//  Texturebug � sticks to texture seams to reduce fall speed
//  Adapted from IDA dump sub_180302D80 (���������� ���.txt)
// ============================================================================
void CMovement::Texturebug(CUserCommand* cmd) {
	if (!Config::Get<bool>(Vars::Movement::Texturebug) || !ctx.pawn || !ctx.pawn->Alive() || !cmd)
		return;

	auto mt = ctx.pawn->m_nActualMoveType();
	if (mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER)
		return;

	Vector velocity = ctx.pawn->m_vecVelocity();
	if (velocity.z > -100.f)
		return;

	// trace 4 rays in a cross pattern around the player to find texture seams
	Vector origin = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();

	auto filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);

	struct TraceResult { Vector normal; float fraction; bool hit; };
	TraceResult results[4] = {};

	Vector dirs[4] = {
		Vector(1.f, 0.f, 0.f),
		Vector(-1.f, 0.f, 0.f),
		Vector(0.f, 1.f, 0.f),
		Vector(0.f, -1.f, 0.f),
	};

	for (int i = 0; i < 4; ++i) {
		CRay ray{};
		Vector s = origin + dirs[i] * 16.f;
		Vector e = s + Vector(0.f, 0.f, -32.f);
		ray.Init(s, e);

		CGameTrace tr{};
		Physics->TraceRay(&ray, &s, &e, &filter, &tr);

		results[i].hit = tr.fraction < 1.f;
		results[i].fraction = tr.fraction;
		results[i].normal = tr.hit_normal;
	}

	// check if we're on a texture seam (different surface normals on adjacent traces)
	bool on_seam = false;
	for (int i = 0; i < 4 && !on_seam; ++i) {
		if (!results[i].hit) continue;
		for (int j = i + 1; j < 4; ++j) {
			if (!results[j].hit) continue;
			Vector diff = results[i].normal - results[j].normal;
			if (diff.LengthSqr() > 0.5f) {
				on_seam = true;
				break;
			}
		}
	}

	if (!on_seam) {
		m_bTexturebugActive = false;
		return;
	}

	if (!m_bTexturebugActive)
		m_bTexturebugActive = true;

	// apply upward counter-force to reduce fall speed when on seam
	if (velocity.z < -10.f) {
		auto base = cmd->pb.mutable_base();
		base->set_forwardmove(0.f);
		base->set_leftmove(0.f);

		Vector move_dir = -velocity;
		move_dir.z = 0.f;
		move_dir.Normalize();

		float strength = std::clamp(std::abs(velocity.z) / 400.f, 0.1f, 0.6f);
		float yaw = Math::Rad2Deg(std::atan2(move_dir.y, move_dir.x));
		float fmove = std::cos(Math::Deg2Rad(yaw)) * strength * 450.f;
		float lmove = std::sin(Math::Deg2Rad(yaw)) * strength * 450.f;

		cmd->pb.mutable_base()->set_forwardmove(fmove);
		cmd->pb.mutable_base()->set_leftmove(lmove);
	}
}

// ============================================================================
//  Pixelsurf � precise edge surfing using multi-ray traces
//  Adapted from IDA dump sub_1801E79D0 (�������� �����.txt)
// ============================================================================
void CMovement::Pixelsurf(CUserCommand* cmd) {
	if (!Config::Get<bool>(Vars::Movement::Pixelsurf) || !ctx.pawn || !ctx.pawn->Alive() || !cmd)
		return;

	auto mt = ctx.pawn->m_nActualMoveType();
	if (mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER)
		return;

	Vector velocity = ctx.pawn->m_vecVelocity();
	if (velocity.z > -80.f)
		return;

	Vector origin = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	auto filter = Physics->MakeFilter(ctx.pawn, 0x1C300B, 3, 15);

	// grid of traces below player to find exact edge
	const int grid = 4;
	const float spacing = 8.f;
	bool found_edge = false;
	Vector edge_normal{};

	for (int x = -1; x <= 1 && !found_edge; ++x) {
		for (int y = -1; y <= 1 && !found_edge; ++y) {
			CRay ray{};
			Vector s = origin + Vector(x * spacing, y * spacing, 4.f);
			Vector e = s + Vector(0.f, 0.f, -48.f);
			ray.Init(s, e);

			CGameTrace tr{};
			Physics->TraceRay(&ray, &s, &e, &filter, &tr);

			if (tr.fraction < 1.f && tr.fraction > 0.1f) {
				if (std::abs(tr.hit_normal.z) < 0.6f) {
					found_edge = true;
					edge_normal = tr.hit_normal;
				}
			}
		}
	}

	if (!found_edge) {
		m_bPixelsurfActive = false;
		return;
	}

	if (!m_bPixelsurfActive)
		m_bPixelsurfActive = true;

	// apply force to stay on the pixel edge
	Vector surf_dir = Vector(-edge_normal.y, edge_normal.x, 0.f);
	surf_dir.Normalize();

	float strength = std::clamp(std::abs(velocity.z) / 500.f, 0.1f, 0.8f);

	auto base = cmd->pb.mutable_base();
	base->set_forwardmove(surf_dir.x * strength * 500.f);
	base->set_leftmove(surf_dir.y * strength * 500.f);

	constexpr uint64_t move_mask = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;
	cmd->button_state.held &= ~move_mask;
	cmd->button_state.toggle &= ~move_mask;
	cmd->button_state.scroll &= ~move_mask;

	if (surf_dir.x > 0.01f) cmd->button_state.held |= IN_FORWARD;
	else if (surf_dir.x < -0.01f) cmd->button_state.held |= IN_BACK;
	if (surf_dir.y > 0.01f) cmd->button_state.held |= IN_MOVELEFT;
	else if (surf_dir.y < -0.01f) cmd->button_state.held |= IN_MOVERIGHT;
}

// ============================================================================
//  MovementPrediction � predicts player position N ticks ahead
//  Adapted from IDA dump sub_1801EDE70 (������� �����.txt)
// ============================================================================
void CMovement::MovementPrediction(CUserCommand* cmd) {
	if (!Config::Get<bool>(Vars::Movement::Prediction) || !ctx.pawn || !ctx.pawn->Alive() || !cmd)
		return;

	Vector predicted = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	Vector vel = ctx.pawn->m_vecVelocity();

	cvar(sv_gravity);
	float gravity = sv_gravity ? sv_gravity->GetFloat() * ctx.pawn->m_flActualGravityScale() : 800.f;

	int predict_ticks = Config::Get<int>(Vars::Ragebot::FutureTicks);
	if (predict_ticks <= 0)
		predict_ticks = 6;

	for (int i = 0; i < predict_ticks; ++i) {
		predicted = predicted + vel * INTERVAL_PER_TICK;
		if (!(ctx.pawn->m_fFlags() & FL_ONGROUND)) {
			vel.z -= gravity * INTERVAL_PER_TICK * 0.5f;
		}
	}

	(void)predicted;
}

CMovement* Movement = new CMovement();
