#include <shared.h>

POINT CAntiAim::GetScreenCenter() {
	HWND hwnd = GetActiveWindow();
	if (!hwnd)
		return { 0, 0 };

	RECT rect{};
	GetClientRect(hwnd, &rect);

	POINT center{ (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
	ClientToScreen(hwnd, &center);

	return center;
}

void CAntiAim::RestoreMouse() {
	if (Hooks->RelativeMouseMode)
		Hooks->RelativeMouseMode(Inputs, true);

	int count = ShowCursor(TRUE);
	while (count < 0)
		count = ShowCursor(TRUE);

	this->mouse_active = false;
}

void CAntiAim::DrawRadialGradient(ImDrawList* dl, const ImVec2& pos, float radius, ImU32 color_in, ImU32 color_out, int segments) {
	dl->PrimReserve(segments * 3, segments + 2);

	const ImDrawIdx center_idx = static_cast<ImDrawIdx>(dl->_VtxCurrentIdx);
	dl->_VtxWritePtr[0].pos = pos;
	dl->_VtxWritePtr[0].uv = ImGui::GetIO().Fonts->TexUvWhitePixel;
	dl->_VtxWritePtr[0].col = color_in;
	dl->_VtxWritePtr++;
	dl->_VtxCurrentIdx++;

	for (int i = 0; i <= segments; ++i) {
		const float rad = (static_cast<float>(i) * Math::_pi2) / static_cast<float>(segments);
		const ImVec2 p = { pos.x + std::cos(rad) * radius, pos.y + std::sin(rad) * radius };

		dl->_VtxWritePtr[0].pos = p;
		dl->_VtxWritePtr[0].uv = ImGui::GetIO().Fonts->TexUvWhitePixel;
		dl->_VtxWritePtr[0].col = color_out;
		dl->_VtxWritePtr++;
		dl->_VtxCurrentIdx++;
	}

	for (int i = 0; i < segments; ++i) {
		dl->_IdxWritePtr[0] = center_idx;
		dl->_IdxWritePtr[1] = static_cast<ImDrawIdx>(center_idx + 1 + i);
		dl->_IdxWritePtr[2] = static_cast<ImDrawIdx>(center_idx + 1 + i + 1);
		dl->_IdxWritePtr += 3;
	}
}

void CAntiAim::MouseOverride() {
	constexpr float max_radius = 200.0f;
	constexpr float dead_zone = 45.0f;

	const auto center = this->GetScreenCenter();

	if (!this->mouse_active) {
		if (Hooks->RelativeMouseMode)
			Hooks->RelativeMouseMode(Inputs, false);

		SetCursorPos(center.x, center.y);
		ShowCursor(FALSE);

		this->mouse_active = true;
		return;
	}

	POINT cursor{};
	if (!GetCursorPos(&cursor))
		return;

	const float raw_dx = static_cast<float>(cursor.x - center.x);
	const float raw_dy = static_cast<float>(cursor.y - center.y);

	if (raw_dx != 0.0f || raw_dy != 0.0f) {
		const float sens = Config::Get<float>(Vars::Antiaim::MouseOverrideSensitivity) / 100.0f;

		this->virtual_x += raw_dx * sens;
		this->virtual_y += raw_dy * sens;

		const float len = std::sqrt(this->virtual_x * this->virtual_x + this->virtual_y * this->virtual_y);
		if (len > max_radius) {
			this->virtual_x = (this->virtual_x / len) * max_radius;
			this->virtual_y = (this->virtual_y / len) * max_radius;
		}

		SetCursorPos(center.x, center.y);
	}

	const float dist = std::sqrt(this->virtual_x * this->virtual_x + this->virtual_y * this->virtual_y);
	if (dist < dead_zone) {
		this->has_custom_yaw = false;
		this->custom_yaw = 0.0f;
		return;
	}

	this->custom_yaw = std::atan2(this->virtual_x, this->virtual_y) * (Math::rads);
	this->has_custom_yaw = true;
}

void CAntiAim::OnCreateMove() {
	this->indicator_active = false;

	if (!ctx.pawn || !ctx.cmd || !ctx.base_cmd)
		return;

	const bool aa_enabled = Config::Get<bool>(Vars::Antiaim::Enable);
	const bool override_enabled = Config::Get<bool>(Vars::Antiaim::MouseOverride);

	static bool prev_override = false;

	this->angle = Vector(ctx.base_cmd->viewangles().x(), ctx.base_cmd->viewangles().y(), ctx.base_cmd->viewangles().z());

	if (override_enabled && !prev_override) {
		if (this->has_custom_yaw) {
			this->has_custom_yaw = false;
			this->custom_yaw = 0.0f;
			this->virtual_x = 0.0f;
			this->virtual_y = 0.0f;
		}
	} else if (!override_enabled && prev_override) {
		if (this->mouse_active)
			this->RestoreMouse();
	}

	prev_override = override_enabled;

	if (!aa_enabled) {
		if (this->mouse_active)
			this->RestoreMouse();
		return;
	}

	if (ctx.cmd->button_state.held & IN_USE)
		return;

	if (auto mt = ctx.pawn->m_nActualMoveType(); mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER) {
		if (this->mouse_active)
			this->RestoreMouse();
		return;
	}

	if (ctx.weapon_data && ctx.weapon_data->m_WeaponType() == WEAPONTYPE_GRENADE) {
		if (auto nade = reinterpret_cast<C_BaseCSGrenade*>(ctx.weapon); nade->m_bPinPulled() || nade->m_fThrowTime() > 0.0f)
			return;
	}

	if (override_enabled)
		this->MouseOverride();

	this->angle = Vector(ctx.base_cmd->viewangles().x(), ctx.base_cmd->viewangles().y(), ctx.base_cmd->viewangles().z());
	if (Config::Get<bool>(Vars::Antiaim::AtTargets))
		this->angle.y = this->GetTargetYaw();

	switch (Config::Get<int>(Vars::Antiaim::Pitch)) {
	case 0:
	default:
		break;
	case 1:
		this->angle.x = 89.0f;
		break;
	case 2:
		this->angle.x = -89.0f;
		break;
	}

	switch (Config::Get<int>(Vars::Antiaim::Yaw)) {
	case 0:
	default:
		break;
	case 1:
		this->angle.y += 180.0f;
		break;
	}

	const int spinv = Config::Get<int>(Vars::Antiaim::SpinbotValue);
	if (Config::Get<bool>(Vars::Antiaim::Spinbot) && spinv != 0) {
		static float y = 0.0f;

		float speed = static_cast<float>(spinv);
		float deg = (speed / 100.0f) * 30.0f;

		y += deg;

		while (y > 180.0f)  y -= 360.0f;
		while (y < -180.0f) y += 360.0f;

		this->angle.y = y;
	}

	this->angle.y += this->CorrectYaw();
	this->angle.y += Config::Get<int>(Vars::Antiaim::YawOffset);

	if (this->has_custom_yaw)
		this->angle.y += this->custom_yaw;

	while (this->angle.y > 180.0f)  this->angle.y -= 360.0f;
	while (this->angle.y < -180.0f) this->angle.y += 360.0f;

	this->indicator_tick_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	this->indicator_active = true;

	ctx.base_cmd->mutable_viewangles()->set_x(this->angle.x);
	ctx.base_cmd->mutable_viewangles()->set_y(this->angle.y);
}

void CAntiAim::OnPresent() {
	if (!ctx.pawn)
		return;

	this->DrawDirectionIndicator();

	float dt = ImGui::GetIO().DeltaTime;
	if (dt <= 0.0f || dt > 0.2f)
		dt = 1.0f / 60.0f;

	float target_x = 0.0f;
	float target_y = 0.0f;
	float target_alpha = 0.0f;

	if (this->mouse_active) {
		target_x = this->virtual_x;
		target_y = this->virtual_y;
		target_alpha = 1.0f;

		this->visual_x += (target_x - this->visual_x) * std::min(1.0f, dt * 30.0f);
		this->visual_y += (target_y - this->visual_y) * std::min(1.0f, dt * 30.0f);
		this->visual_alpha += (target_alpha - this->visual_alpha) * std::min(1.0f, dt * 20.0f);
	} else {
		if (this->has_custom_yaw) {
			target_x = this->virtual_x;
			target_y = this->virtual_y;
			target_alpha = 1.0f;

			this->visual_x += (target_x - this->visual_x) * std::min(1.0f, dt * 30.0f);
			this->visual_y += (target_y - this->visual_y) * std::min(1.0f, dt * 30.0f);
			this->visual_alpha += (target_alpha - this->visual_alpha) * std::min(1.0f, dt * 20.0f);
		} else {
			target_x = 0.0f;
			target_y = 0.0f;
			target_alpha = 0.0f;

			this->visual_x += (target_x - this->visual_x) * std::min(1.0f, dt * 18.0f);
			this->visual_y += (target_y - this->visual_y) * std::min(1.0f, dt * 18.0f);
			this->visual_alpha += (target_alpha - this->visual_alpha) * std::min(1.0f, dt * 14.0f);
		}
	}

	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const ImVec2 center = { display.x * 0.5f, display.y * 0.5f };
	const ImVec2 cursor_pos = { center.x + this->visual_x, center.y + this->visual_y };
	constexpr float trail_radius = 24.0f;

	{
		std::lock_guard<std::mutex> lock(this->trail_mutex);

		for (size_t i = 0; i < this->trail.size();) {
			this->trail[i].alpha -= dt * 10.0f;
			this->trail[i].radius -= dt * 70.0f;

			if (this->trail[i].alpha <= 0.0f || this->trail[i].radius <= 1.0f) {
				this->trail[i] = this->trail.back();
				this->trail.pop_back();
			} else {
				++i;
			}
		}

		if (this->visual_alpha > 0.05f) {
			if (!this->has_last_pos) {
				this->last_trail_pos = cursor_pos;
				this->has_last_pos = true;
				this->trail.push_back({ cursor_pos, this->visual_alpha, trail_radius });
			} else {
				const float dx = cursor_pos.x - this->last_trail_pos.x;
				const float dy = cursor_pos.y - this->last_trail_pos.y;
				const float dist = std::sqrt(dx * dx + dy * dy);

				if (dist > 2.0f) {
					const int steps = std::clamp(static_cast<int>(dist / 2.5f), 1, 10);
					for (int i = 1; i <= steps; ++i) {
						const float t = static_cast<float>(i) / static_cast<float>(steps);
						this->trail.push_back({ { this->last_trail_pos.x + dx * t, this->last_trail_pos.y + dy * t }, this->visual_alpha, trail_radius });
					}
					this->last_trail_pos = cursor_pos;
				}
			}
		} else {
			this->has_last_pos = false;
		}
	}

	if (this->visual_alpha <= 0.01f && this->trail.empty())
		return;

	constexpr float dead_zone = 55.0f;
	const float dist = std::sqrt(this->visual_x * this->visual_x + this->visual_y * this->visual_y);

	const auto color = Config::Get<Color_t>(Vars::Antiaim::MouseOverrideColor1);
	const auto color_deadzone = Config::Get<Color_t>(Vars::Antiaim::MouseOverrideColor2);
	const Color_t base_color = (dist < dead_zone) ? color_deadzone : color;

	auto dl = ImGui::GetBackgroundDrawList();

	std::vector<TrailPoint_t> render_trail;
	{
		std::lock_guard<std::mutex> lock(this->trail_mutex);
		render_trail = this->trail;
	}

	for (const auto& pt : render_trail) {
		if (pt.alpha <= 0.0f || pt.radius <= 1.0f)
			continue;

		const ImU32 ci = IM_COL32(base_color.r, base_color.g, base_color.b, static_cast<int>(base_color.a * pt.alpha * 0.5f));
		const ImU32 co = IM_COL32(base_color.r, base_color.g, base_color.b, 0);
		this->DrawRadialGradient(dl, pt.pos, pt.radius, ci, co, 8);
	}

	if (this->visual_alpha > 0.01f) {
		const ImU32 ci = IM_COL32(base_color.r, base_color.g, base_color.b, static_cast<int>(base_color.a * this->visual_alpha));
		const ImU32 co = IM_COL32(base_color.r, base_color.g, base_color.b, 0);
		this->DrawRadialGradient(dl, cursor_pos, trail_radius, ci, co, 32);
	}
}

void CAntiAim::DrawDirectionIndicator() {
	float dt = ImGui::GetIO().DeltaTime;
	if (dt <= 0.0f || dt > 0.2f) {
		dt = 1.0f / 60.0f;
	}

	const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	const bool fresh = now - this->indicator_tick_ms.load() < 250;
	const bool inGame = fresh && ctx.pawn && ctx.controller && ctx.pawn->Alive();

	const auto yawOffset = static_cast<float>(Config::Get<int>(Vars::Antiaim::YawOffset));

	const bool show = inGame && Config::Get<bool>(Vars::Antiaim::Enable) && Config::Get<bool>(Vars::Antiaim::DirectionIndicator) 
		&& this->indicator_active.load() && std::fabs(yawOffset) >= 1.0f;

	const bool both = std::fabs(yawOffset) > 150.0f;
	const bool left = show && (both || yawOffset < 0.0f);
	const bool right = show && (both || yawOffset > 0.0f);

	auto approach = [dt](float& value, float target, float speed) {
		value += (target - value) * std::min(1.0f, dt * speed);
	};

	approach(this->indicator_alpha, show ? 1.0f : 0.0f, 14.0f);
	approach(this->indicator_left, left ? 1.0f : 0.0f, 20.0f);
	approach(this->indicator_right, right ? 1.0f : 0.0f, 20.0f);

	if (this->indicator_alpha <= 0.01f)
		return;

	const auto color = Config::Get<Color_t>(Vars::Antiaim::DirectionIndicatorColor);
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const ImVec2 center = { display.x * 0.5f, display.y * 0.5f };
	constexpr float screenOffset = 96.0f, length = 24.0f, half_height = 8.0f;
	auto dl = ImGui::GetBackgroundDrawList();

	// direction: -1 = left arrow (points left), +1 = right arrow (points right)
	auto draw_arrow = [&](float direction, float lit) {
		const float base_x = center.x + direction * screenOffset;
		const ImVec2 tip = { base_x + direction * length, center.y };
		const ImVec2 top = { base_x, center.y - half_height };
		const ImVec2 bottom = { base_x, center.y + half_height };
		const float strength = (0.25f + 0.75f * lit) * this->indicator_alpha;

		dl->AddTriangleFilled(tip + ImVec2(1, 1), top + ImVec2(1, 1), bottom + ImVec2(1, 1), IM_COL32(0, 0, 0, static_cast<int>(strength)));
		dl->AddTriangleFilled(tip, top, bottom, IM_COL32(color.r, color.g, color.b, static_cast<int>(color.a * strength)));
		};

	draw_arrow(-1.0f, this->indicator_left);
	draw_arrow(1.0f, this->indicator_right);
}

float CAntiAim::GetTargetYaw() {
	if (!ctx.pawn || !ctx.base_cmd)
		return ctx.base_cmd ? ctx.base_cmd->viewangles().y() : 0.0f;

	float yaw = ctx.base_cmd->viewangles().y();
	float distance = FLT_MAX;
	const Vector origin = ctx.pawn->m_pGameSceneNode()->m_vecAbsOrigin();
	for (size_t i = 0; i < EntityList->player_count; ++i) {
		auto& entity = EntityList->players[i];
		if (!entity.Fill())
			continue;

		auto player = entity.pawn;
		if (!player->Alive() || player == ctx.pawn || !player->Enemy(ctx.pawn) || player->m_bGunGameImmunity())
			continue;

		auto node = player->m_pGameSceneNode();
		if (!node)
			continue;

		Vector delta = node->m_vecAbsOrigin() - origin;
		delta.z = 0.f;
		const float length = delta.LengthSqr();
		if (length <= 0.f || length >= distance)
			continue;

		distance = length;
		yaw = std::atan2(delta.y, delta.x) * Math::rads;
	}
	return yaw;
}

float CAntiAim::CorrectYaw() {
	if (!Config::Get<bool>(Vars::Antiaim::YawCorrection))
		return 0.f;

	constexpr uint64_t move_mask = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;

	const bool is_jumping = !(EnginePrediction->m_fFlags & FL_ONGROUND);
	const bool is_ducking = ctx.pawn->m_pMovementServices()->m_flDuckAmount() > 0.f || (EnginePrediction->m_fFlags & FL_DUCKING);
	const bool is_moving = (ctx.cmd->button_state.held & move_mask) != 0 && ctx.pawn->m_vecVelocity().Length2d() > 15.f;

	const auto wtype = ctx.weapon_data ? ctx.weapon_data->m_WeaponType() : 0;

	if (wtype == WEAPONTYPE_RIFLE || wtype == WEAPONTYPE_SNIPER_RIFLE || wtype == WEAPONTYPE_SHOTGUN) {
		if (is_jumping)
			return is_ducking ? 18.f : 26.f;
		else if (is_moving)
			return is_ducking ? 23.f : 29.f;
		else
			return is_ducking ? 15.f : 34.f;
	} else if (wtype == WEAPONTYPE_SUBMACHINEGUN) {
		if (is_jumping)
			return is_ducking ? 10.f : 15.f;
		else if (is_moving)
			return is_ducking ? 15.f : 15.f;
		else
			return is_ducking ? 15.f : 15.f;
	} else if (wtype == WEAPONTYPE_PISTOL) {
		if (is_jumping)
			return is_ducking ? 18.f : 10.f;
		else if (is_moving)
			return is_ducking ? 18.f : 10.f;
		else
			return is_ducking ? 12.f : 10.f;
	} else if (wtype == WEAPONTYPE_KNIFE || wtype == WEAPONTYPE_MELEE) {
		if (is_jumping)
			return is_ducking ? 15.f : 15.f;
		else if (is_moving)
			return is_ducking ? 15.f : 12.f;
		else
			return is_ducking ? 10.f : 12.f;
	} else if (wtype == WEAPONTYPE_GRENADE) {
		if (is_jumping)
			return is_ducking ? 10.f : 7.f;
		else if (is_moving)
			return is_ducking ? 10.f : 15.f;
		else
			return is_ducking ? 10.f : 7.f;
	}

	return 0.f;
}