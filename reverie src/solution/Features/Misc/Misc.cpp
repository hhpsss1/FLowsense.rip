#include <shared.h>

void CMisc::BuyBot() {
	if (!this->buy_pending || !Config::Get<bool>(Vars::Misc::BuyBot) || !ctx.pawn || !ctx.pawn->Alive() ||
		!GameRules || !ctx.pawn->m_bInBuyZone())
		return;

	const bool ct = ctx.pawn->m_iTeamNum() == 3;
	if ((!ct && ctx.pawn->m_iTeamNum() != 2) || (ct ? GameRules->m_bCTCantBuy() : GameRules->m_bTCantBuy()))
		return;

	const char* primary_t[] = { "", "g3sg1", "ssg08", "awp", "ak47", "ak47" };
	const char* primary_ct[] = { "", "scar20", "ssg08", "awp", "m4a1", "m4a1_silencer" };
	const char* secondary[] = { "", "deagle", "revolver", "elite", "p250", ct ? "fiveseven" : "tec9" };
	const int primary = std::clamp(Config::Get<int>(Vars::Misc::BuyBotPrimary), 0, 5);
	const int pistol = std::clamp(Config::Get<int>(Vars::Misc::BuyBotSecondary), 0, 5);
	std::string command;
	if (primary)
		command += std::format("buy {};", ct ? primary_ct[primary] : primary_t[primary]);
	if (pistol)
		command += std::format("buy {};", secondary[pistol]);
	if (Config::Get<bool>(Vars::Misc::BuyBotArmor))
		command += "buy vesthelm;buy vest;";
	if (ct && Config::Get<bool>(Vars::Misc::BuyBotDefuser))
		command += "buy defuser;";

	const char* grenades[] = { "hegrenade", "smokegrenade", ct ? "incgrenade" : "molotov", "flashbang" };
	const auto& selected = Config::Get<std::vector<int>>(Vars::Misc::BuyBotGrenades);
	for (size_t i = 0; i < 4 && i < selected.size(); ++i) {
		if (selected[i])
			command += std::format("buy {};", grenades[i]);
	}

	this->buy_pending = false;
	if (!command.empty())
		EngineClient->ExecuteClientCmd(command.c_str());
}

static void DrawGradientRect(ImVec2 pos, ImVec2 size, ImU32 gradient_color, ImDrawList* draw_list = ImGui::GetBackgroundDrawList()) {
	if (!draw_list)
		return;

	ImVec2 p1 = pos;
	ImVec2 p2 = ImVec2(pos.x + size.x, pos.y + size.y);

	float fade_w = (std::min)(20.0f, size.x * 0.5f);

	draw_list->AddRectFilled(ImVec2(p1.x + fade_w, p1.y), ImVec2(p2.x - fade_w, p2.y), gradient_color);

	draw_list->AddRectFilledMultiColor(p1, ImVec2(p1.x + fade_w, p2.y), IM_COL32(0, 0, 0, 0), gradient_color, gradient_color, IM_COL32(0, 0, 0, 0));
	draw_list->AddRectFilledMultiColor(ImVec2(p2.x - fade_w, p1.y), p2, gradient_color, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), gradient_color);
}

static int GetCPUUsage() {
	static ULARGE_INTEGER last_idle_time = {}, last_kernel_time = {}, last_user_time = {};
	static auto last_update_time = std::chrono::steady_clock::now();
	static float cached_cpu_usage = 0.0f;

	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time);

	if (elapsed.count() < 1000)
		return static_cast<int>(cached_cpu_usage + 0.5f);

	last_update_time = now;

	FILETIME idle_time, kernel_time, user_time;
	if (!GetSystemTimes(&idle_time, &kernel_time, &user_time))
		return static_cast<int>(cached_cpu_usage + 0.5f);

	ULARGE_INTEGER idle, kernel, user;
	idle.LowPart = idle_time.dwLowDateTime;
	idle.HighPart = idle_time.dwHighDateTime;
	kernel.LowPart = kernel_time.dwLowDateTime;
	kernel.HighPart = kernel_time.dwHighDateTime;
	user.LowPart = user_time.dwLowDateTime;
	user.HighPart = user_time.dwHighDateTime;

	ULONGLONG sys_idle = idle.QuadPart - last_idle_time.QuadPart;
	ULONGLONG sys_kernel = kernel.QuadPart - last_kernel_time.QuadPart;
	ULONGLONG sys_user = user.QuadPart - last_user_time.QuadPart;
	ULONGLONG sys_total = sys_kernel + sys_user;

	last_idle_time = idle;
	last_kernel_time = kernel;
	last_user_time = user;

	if (sys_total == 0)
		return static_cast<int>(cached_cpu_usage + 0.5f);

	cached_cpu_usage = static_cast<float>(100.0 - ((sys_idle * 100.0) / sys_total));
	return static_cast<int>(cached_cpu_usage + 0.5f);
}

static int GetPing() {
	if (EngineClient && EngineClient->InGame()) {
		if (ctx.controller)
			return static_cast<int>(ctx.controller->m_iPing());
		auto net_channel = EngineClient->GetNetChannel(0);
		if (net_channel)
			return static_cast<int>(net_channel->GetAvgLatency() * 1000.f);
	}
	return 0;
}

static int GetVelocity() {
	if (EngineClient && EngineClient->InGame() && ctx.pawn && ctx.pawn->Alive()) {
		Vector vel = ctx.pawn->m_vecAbsVelocity();
		return static_cast<int>(std::round(std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z)));
	}
	return 0;
}

static std::string GetPlayerName() {
	if (EngineClient && EngineClient->InGame() && ctx.controller) {
		const char* name = ctx.controller->m_sSanitizedPlayerName();
		if (name && name[0] != '\0')
			return std::string(name);
	}
	char win_user[256] = {};
	DWORD size = sizeof(win_user);
	if (GetUserNameA(win_user, &size) && win_user[0] != '\0')
		return std::string(win_user);
	return "user";
}

void CMisc::Watermark() {
	if (!Config::Get<bool>(Vars::Misc::Watermark))
		return;

	auto draw_list = ImGui::GetBackgroundDrawList();
	if (!draw_list)
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImFont* font = (Var && Var->font.inter[0]) ? Var->font.inter[0] : ImGui::GetFont();
	const float font_size = font ? font->LegacySize : 14.f;

	static float last_fps = 0.0f;
	static float time_accumulator = 0.0f;
	time_accumulator += io.DeltaTime;
	if (time_accumulator >= 1.0f) {
		last_fps = io.Framerate;
		time_accumulator = 0.0f;
	}

	const bool ingame = EngineClient && EngineClient->InGame();

	std::string text1 = "CustomFlow";
	std::string suffix = "";

	if (Config::Get<bool>(Vars::Misc::WatermarkFps))
		suffix += "  " + std::to_string(static_cast<int>(last_fps)) + " fps";

	suffix += "  " + std::to_string(GetCPUUsage()) + "% cpu";

	if (ingame) {
		if (Config::Get<bool>(Vars::Misc::WatermarkPing))
			suffix += "  " + std::to_string(GetPing()) + " ms";

		if (Config::Get<bool>(Vars::Misc::WatermarkSpeed))
			suffix += "  " + std::to_string(GetVelocity()) + " speed";
	}

	if (Config::Get<bool>(Vars::Misc::WatermarkTime)) {
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		char time_str[64];
		std::tm tm_buf;
		localtime_s(&tm_buf, &now_c);
		strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_buf);
		suffix += "  " + std::string(time_str);
	}

	std::string username = "";
	if (Config::Get<bool>(Vars::Misc::WatermarkUsername))
		username = GetPlayerName();

	ImVec2 text1_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text1.c_str());
	ImVec2 user_size = username.empty() ? ImVec2(0.f, 0.f) : font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, username.c_str());
	ImVec2 suffix_size = suffix.empty() ? ImVec2(0.f, 0.f) : font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, suffix.c_str());

	float total_width = text1_size.x + (username.empty() ? 0.f : user_size.x + 8.f) + suffix_size.x + 10.f;
	float total_height = (std::max)({ text1_size.y, user_size.y, suffix_size.y });

	float pos_x = io.DisplaySize.x - total_width - 15.f;
	float pos_y = 15.f;

	ImVec2 bg_pos = ImVec2(pos_x - 5.f, pos_y - 2.5f);
	ImVec2 bg_size = ImVec2(total_width + 10.f, total_height + 6.f);

	const int opacity = std::clamp(Config::Get<int>(Vars::Misc::WatermarkOpacity), 0, 255);
	DrawGradientRect(bg_pos, bg_size, IM_COL32(0, 0, 0, opacity), draw_list);

	float current_x = pos_x;

	Color_t accent = Config::Get<Color_t>(Vars::Misc::WatermarkAccentColor);
	ImU32 accent_col = IM_COL32(accent.R(), accent.G(), accent.B(), 255);

	draw_list->AddText(font, font_size, ImVec2(current_x, pos_y), IM_COL32(255, 255, 255, 255), text1.c_str());
	current_x += text1_size.x;

	if (!username.empty()) {
		current_x += 6.f;
		draw_list->AddText(font, font_size, ImVec2(current_x, pos_y), accent_col, username.c_str());
		current_x += user_size.x;
	}

	if (!suffix.empty()) {
		draw_list->AddText(font, font_size, ImVec2(current_x, pos_y), IM_COL32(255, 255, 255, 255), suffix.c_str());
	}
}

void CMisc::CrosshairIndicators() {
	if (!Config::Get<bool>(Vars::Misc::CrosshairIndicators) || !EngineClient || !EngineClient->InGame())
		return;

	if (!ctx.pawn || !ctx.pawn->Alive())
		return;

	auto draw_list = ImGui::GetBackgroundDrawList();
	if (!draw_list)
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImFont* text_font = (Var && Var->font.indicator_text) ? Var->font.indicator_text :
	                   ((Var && Var->font.inter[1]) ? Var->font.inter[1] :
	                   ((Var && Var->font.inter[0]) ? Var->font.inter[0] : ImGui::GetFont()));

	Color_t accent = Config::Get<Color_t>(Vars::Misc::CrosshairIndicatorsColor);
	int opacity = std::clamp(Config::Get<int>(Vars::Misc::CrosshairIndicatorsOpacity), 0, 255);

	float cx = io.DisplaySize.x * 0.5f;
	float cy = io.DisplaySize.y * 0.5f;

	std::vector<std::pair<std::string, bool>> states;
	states.push_back({ xs("DT"), Config::Get<bool>(Vars::Ragebot::Enable) });
	states.push_back({ xs("HS"), Config::Get<bool>(Vars::Antiaim::Enable) });
	states.push_back({ xs("BAIM"), Config::Get<bool>(Vars::Ragebot::Global::ForceBodyAim) });
	states.push_back({ xs("HC"), Config::Get<bool>(Vars::Ragebot::Enable) });
	states.push_back({ xs("DMG"), Config::Get<bool>(Vars::Ragebot::Enable) });

	float x = cx + 20.f;
	float y = cy - 10.f;
	float spacing = 16.f;

	for (auto& s : states) {
		if (!s.second)
			continue;

		ImU32 col = IM_COL32(accent.R(), accent.G(), accent.B(), opacity);
		draw_list->AddText(text_font, text_font ? text_font->LegacySize : 14.f, ImVec2(x, y), col, s.first.c_str());
		y += spacing;
	}
}

void CMisc::Indicators() {
	if (!Config::Get<bool>(Vars::Misc::Indicators) || !EngineClient || !EngineClient->InGame())
		return;

	if (!ctx.pawn || !ctx.pawn->Alive())
		return;

	auto draw_list = ImGui::GetBackgroundDrawList();
	if (!draw_list)
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImFont* text_font = (Var && Var->font.indicator_text) ? Var->font.indicator_text : 
	                   ((Var && Var->font.inter[1]) ? Var->font.inter[1] : 
	                   ((Var && Var->font.inter[0]) ? Var->font.inter[0] : ImGui::GetFont()));
	ImFont* icon_font = (Var && Var->font.indicator_icon) ? Var->font.indicator_icon : 
	                   ((Var && Var->font.fa[1]) ? Var->font.fa[1] : 
	                   ((Var && Var->font.fa[0]) ? Var->font.fa[0] : text_font));

	const float text_font_size = text_font ? text_font->LegacySize : 18.f;
	const float icon_font_size = icon_font ? icon_font->LegacySize : 16.f;

	const auto& ind_items = Config::Get<std::vector<int>>(Vars::Misc::IndicatorItems);
	Color_t accent = Config::Get<Color_t>(Vars::Misc::WatermarkAccentColor);

	struct Indicator_t {
		std::string key;
		std::string text;
		std::string extra_text;
		const char* icon;
		bool active;
		ImU32 icon_color_override;
		ImU32 text_color_override;
		ImU32 extra_color;
	};

	std::vector<Indicator_t> indicators;

	// 0: Doubletap
	bool dt_item = ind_items.size() > 0 && ind_items[0];
	bool dt_active = dt_item && Config::Get<bool>(Vars::Ragebot::Enable);
	const char* dt_icon = reinterpret_cast<const char*>(ICON_FA_BATTERY_FULL);
	const char* dt_text = "2x";
	bool dt_broken = false;
	if (ctx.weapon && ctx.weapon->m_iClip1() <= 1) {
		dt_icon = reinterpret_cast<const char*>(ICON_FA_BATTERY_EMPTY);
		dt_text = "1x";
		dt_broken = true;
	}
	indicators.push_back({
		"DT",
		dt_text,
		"",
		dt_icon,
		dt_active,
		dt_broken ? IM_COL32(255, 60, 60, 255) : IM_COL32(accent.R(), accent.G(), accent.B(), 255),
		0,
		0
	});

	// 1: Hideshot
	bool hs_item = ind_items.size() > 1 && ind_items[1];
	bool hs_active = hs_item && Config::Get<bool>(Vars::Antiaim::Enable);
	indicators.push_back({
		"HS",
		"HS",
		"",
		reinterpret_cast<const char*>(ICON_FA_SHIELD),
		hs_active,
		0, 0, 0
	});

	// 2: Bodyaim
	bool baim_item = ind_items.size() > 2 && ind_items[2];
	bool baim_active = baim_item && Config::Get<bool>(Vars::Ragebot::Global::ForceBodyAim);
	indicators.push_back({
		"BAIM",
		"BAIM",
		"",
		reinterpret_cast<const char*>(ICON_FA_BULLSEYE),
		baim_active,
		0, 0, 0
	});

	// 3: Ping warning
	int ping_val = GetPing();
	bool ping_item = ind_items.size() > 3 && ind_items[3];
	bool ping_active = ping_item && ping_val >= 80;
	float time_val = static_cast<float>(ImGui::GetTime());
	float pulse = 0.3f + 0.7f * (0.5f + 0.5f * std::sin(time_val * 5.0f));
	ImU32 ping_col = IM_COL32(255, 255, 0, static_cast<int>(255 * pulse));
	indicators.push_back({
		"PING",
		"PING",
		"",
		reinterpret_cast<const char*>(ICON_FA_WIFI),
		ping_active,
		ping_col,
		ping_col,
		0
	});

	// 4: Hitchance
	bool hc_item = ind_items.size() > 4 && ind_items[4];
	bool hc_active = hc_item && Config::Get<bool>(Vars::Ragebot::Enable);
	float hc_val = Config::Get<float>(Vars::Ragebot::Global::Hitchance);
	std::string hc_str = hc_val > 0.f ? std::to_string(static_cast<int>(hc_val)) : "AUTO";
	indicators.push_back({
		"HC",
		hc_str,
		"",
		reinterpret_cast<const char*>(ICON_FA_CROSSHAIRS),
		hc_active,
		0, 0, 0
	});

	// 5: Damage
	bool dmg_item = ind_items.size() > 5 && ind_items[5];
	bool dmg_active = dmg_item && Config::Get<bool>(Vars::Ragebot::Enable);
	int dmg_val = Config::Get<int>(Vars::Ragebot::Global::MinimumDamage);
	std::string dmg_str = dmg_val > 0 ? std::to_string(dmg_val) : "LETHAL";
	indicators.push_back({
		"DMG",
		dmg_str,
		"",
		reinterpret_cast<const char*>(ICON_FA_HEART),
		dmg_active,
		0, 0, 0
	});

	// 6: Bomb
	bool bomb_item = ind_items.size() > 6 && ind_items[6];
	bool bomb_active = false;
	std::string bomb_str = "BOMB";
	std::string bomb_extra = "";
	if (bomb_item && EntityList && EntityList->planted_c4) {
		auto c4 = EntityList->planted_c4;
		if (c4->m_bBombTicking() && !c4->m_bBombDefused() && !c4->m_bHasExploded()) {
			bomb_active = true;
			float blow_time = c4->m_flC4Blow();
			float cur_time = GlobalVars ? GlobalVars->current_time : 0.f;
			float time_to_explode = (std::max)(0.f, blow_time - cur_time);
			std::string site = c4->m_nBombSite() == 0 ? "A" : "B";
			bomb_str = std::format("{} - {:.1f}s", site, time_to_explode);
			bomb_extra = "FATAL";
		}
	}
	indicators.push_back({
		"BOMB",
		bomb_str,
		bomb_extra,
		reinterpret_cast<const char*>(ICON_FA_BOMB),
		bomb_active,
		IM_COL32(255, 180, 0, 255),
		0,
		IM_COL32(255, 50, 50, 255)
	});

	// 7: Peek assist
	bool peek_item = ind_items.size() > 7 && ind_items[7];
	bool peek_active = peek_item && Config::Get<bool>(Vars::Movement::PeekAssist);
	indicators.push_back({
		"PEEK",
		"PEEK",
		"",
		reinterpret_cast<const char*>(ICON_FA_ARROWS_ROTATE),
		peek_active,
		0, 0, 0
	});

	static std::unordered_map<std::string, float> animated_y;
	static std::unordered_map<std::string, float> alpha_values;

	const float start_x = 15.0f;
	const float start_y = io.DisplaySize.y * 0.5f + 30.0f;
	const float spacing = 10.0f;
	const float speed = 0.1f;
	const float fade_speed = 0.08f;
	const int opacity = std::clamp(Config::Get<int>(Vars::Misc::IndicatorOpacity), 0, 255);

	float offset_y = 0.0f;

	for (const auto& ind : indicators) {
		float target_y = start_y + offset_y;

		if (animated_y.find(ind.key) == animated_y.end()) {
			animated_y[ind.key] = target_y;
			alpha_values[ind.key] = ind.active ? 1.0f : 0.0f;
		}

		animated_y[ind.key] = animated_y[ind.key] * (1.0f - speed) + target_y * speed;

		if (ind.active) {
			alpha_values[ind.key] += fade_speed;
			if (alpha_values[ind.key] > 1.0f)
				alpha_values[ind.key] = 1.0f;
		} else {
			alpha_values[ind.key] -= fade_speed;
			if (alpha_values[ind.key] <= 0.0f) {
				alpha_values.erase(ind.key);
				animated_y.erase(ind.key);
				continue;
			}
		}

		float alpha = alpha_values[ind.key];
		int gradient_alpha = static_cast<int>(opacity * alpha);

		ImVec2 icon_size = icon_font->CalcTextSizeA(icon_font_size, FLT_MAX, 0.0f, ind.icon);
		ImVec2 text_size = text_font->CalcTextSizeA(text_font_size, FLT_MAX, 0.0f, ind.text.c_str());
		ImVec2 extra_size = ind.extra_text.empty() ? ImVec2(0.f, 0.f) : text_font->CalcTextSizeA(text_font_size, FLT_MAX, 0.0f, ind.extra_text.c_str());

		float row_height = (std::max)(icon_size.y, text_size.y);
		float total_w = icon_size.x + 8.f + text_size.x + (ind.extra_text.empty() ? 0.f : extra_size.x + 6.f) + 12.f;

		ImVec2 pos = ImVec2(start_x, animated_y[ind.key]);
		ImVec2 bg_pos = ImVec2(pos.x - 5.f, pos.y - 3.f);
		ImVec2 bg_size = ImVec2(total_w + 10.f, row_height + 6.f);

		DrawGradientRect(bg_pos, bg_size, IM_COL32(0, 0, 0, gradient_alpha), draw_list);

		ImU32 icon_col = ind.icon_color_override != 0 ?
			(ind.icon_color_override & 0x00FFFFFF) | (static_cast<uint32_t>(static_cast<float>((ind.icon_color_override >> 24) & 0xFF) * alpha) << 24) :
			IM_COL32(accent.R(), accent.G(), accent.B(), static_cast<int>(255 * alpha));

		ImU32 text_col = ind.text_color_override != 0 ?
			(ind.text_color_override & 0x00FFFFFF) | (static_cast<uint32_t>(static_cast<float>((ind.text_color_override >> 24) & 0xFF) * alpha) << 24) :
			IM_COL32(255, 255, 255, static_cast<int>(255 * alpha));

		draw_list->AddText(icon_font, icon_font_size, ImVec2(pos.x + 3.f, pos.y + (row_height - icon_size.y) * 0.5f), icon_col, ind.icon);

		float text_x = pos.x + 3.f + icon_size.x + 8.f;
		draw_list->AddText(text_font, text_font_size, ImVec2(text_x, pos.y + (row_height - text_size.y) * 0.5f), text_col, ind.text.c_str());
		text_x += text_size.x;

		if (!ind.extra_text.empty()) {
			text_x += 6.f;
			ImU32 extra_col = (ind.extra_color & 0x00FFFFFF) | (static_cast<uint32_t>(static_cast<float>((ind.extra_color >> 24) & 0xFF) * alpha) << 24);
			draw_list->AddText(text_font, text_font_size, ImVec2(text_x, pos.y + (row_height - extra_size.y) * 0.5f), extra_col, ind.extra_text.c_str());
		}

		if (ind.active) {
			offset_y += row_height + spacing;
		}
	}
}

void CMisc::OnPresent() {
	this->Watermark();
	this->Indicators();

	if (!Config::Get<bool>(Vars::Misc::Hitmarker) || !EngineClient->InGame() || !ctx.pawn || !ctx.pawn->Alive()) {
		this->hitmarker_time = 0;
		return;
	}

	const uint64_t time = this->hitmarker_time.load();
	const float alpha = time ? std::clamp(1.f - static_cast<float>(GetTickCount64() - time) / 450.f, 0.f, 1.f) : 0.f;
	if (alpha <= 0.f)
		return;

	const auto color = Config::Get<Color_t>(Vars::Misc::HitmarkerColor);
	const ImU32 packed = IM_COL32(color.R(), color.G(), color.B(), static_cast<uint8_t>(color.A() * alpha));
	const ImVec2 center = ImGui::GetIO().DisplaySize * 0.5f;
	auto draw_list = ImGui::GetBackgroundDrawList();
	for (int x : { -1, 1 }) {
		for (int y : { -1, 1 })
			draw_list->AddLine(center + ImVec2(x * 4.f, y * 4.f), center + ImVec2(x * 9.f, y * 9.f), packed, 1.5f);
	}
}

CMisc::Panorama_t::IconData_t CMisc::Panorama_t::GetPanoramaTexture(const std::string& path) {
	const uint32_t hash_path = FnvHash(path.c_str());
	auto icon = this->icons.find(hash_path);

	if (icon == this->icons.end()) {
		static auto load_text_file = Memory->Modules.Client.Find(xs("40 55 57 41 56 48 83 EC ? 4C 8B F1")).As<uint8_t*(__fastcall*)(const char*, void*)>();

		std::string full_path = std::format("panorama/images/{}.vsvg_c", path);
		std::string svg_data;

		const auto file_data = load_text_file(full_path.c_str(), nullptr);
		if (!file_data)
			return {};

		uint8_t* current_pos = file_data;
		while (!std::string(reinterpret_cast<const char*>(current_pos)).starts_with(xs("<svg ")))
			++current_pos;

		svg_data = reinterpret_cast<const char*>(current_pos);
		if (svg_data.empty())
			return {};

		std::vector<uint8_t> texture_data(0xFFFFFF);
		PanoramaImageData_t image(texture_data);

		uint32_t width = 0, height = 0;
		if (!image.LoadSvg(svg_data.c_str(), svg_data.size(), &width, &height))
			return {};

		ID3D11Texture2D* texture = nullptr;
		ID3D11ShaderResourceView* texture_view = nullptr;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA sub_resource = {};
		sub_resource.pSysMem = texture_data.data();
		sub_resource.SysMemPitch = desc.Width * 4;
		sub_resource.SysMemSlicePitch = 0;

		Device->CreateTexture2D(&desc, &sub_resource, &texture);

		if (texture) {
			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = desc.MipLevels;
			srv_desc.Texture2D.MostDetailedMip = 0;

			Device->CreateShaderResourceView(texture, &srv_desc, &texture_view);
			texture->Release();
		}

		if (!texture_view)
			return {};

		icon = this->icons.insert_or_assign(hash_path, IconData_t(texture_view, width, height)).first;
		return icon->second;
	}

	return icon->second;
}

static std::unordered_map<std::string, int> img_retries;
static std::unordered_map<std::string, void*> img_cache;

bool CMisc::Panorama_t::LoadVtex(const std::string& path, IconData_t* output) {
	if (path.empty() || img_retries[path] > 120)
		return false;

	auto& proxy = img_cache[path];

	if (!proxy) {
		auto ui_engine = EnginePanorama->GetUIEngine();
		if (!ui_engine)
			return false;

		auto resource_manager = ui_engine->GetResourceManager();
		if (!resource_manager)
			return false;

		proxy = resource_manager->LoadImageInternal(path.c_str(), 4);

		if (!proxy) {
			std::cout << "failed for: " << " | " << path << std::endl;

			img_retries[path]++;

			return false;
		}

		img_retries[path] = 0;
	}

	auto* image = reinterpret_cast<CImageProxySource*>(proxy);
	if (!image)
		return false;

	auto native_texture = image->GetNativeTexture();
	if (!native_texture)
		return false;

	auto size = image->GetImageSize();

	*output = IconData_t(native_texture, size.x, size.y);

	return true;
}

CMisc::Panorama_t::IconData_t CMisc::Panorama_t::GetWeaponIcon(uint16_t weapon_index) {
	std::string weapon_path;
	switch (weapon_index) {
	case WEAPON_DEAGLE: weapon_path = xs("icons/equipment/deagle"); break;
	case WEAPON_DUAL_BERETTAS: weapon_path = xs("icons/equipment/elite"); break;
	case WEAPON_FIVE_SEVEN: weapon_path = xs("icons/equipment/fiveseven"); break;
	case WEAPON_GLOCK: weapon_path = xs("icons/equipment/glock"); break;
	case WEAPON_AK47: weapon_path = xs("icons/equipment/ak47"); break;
	case WEAPON_AUG: weapon_path = xs("icons/equipment/aug"); break;
	case WEAPON_AWP: weapon_path = xs("icons/equipment/awp"); break;
	case WEAPON_FAMAS: weapon_path = xs("icons/equipment/famas"); break;
	case WEAPON_G3SG1: weapon_path = xs("icons/equipment/g3sg1"); break;
	case WEAPON_GALIL: weapon_path = xs("icons/equipment/galilar"); break;
	case WEAPON_M249: weapon_path = xs("icons/equipment/m249"); break;
	case WEAPON_M4A4: weapon_path = xs("icons/equipment/m4a1"); break;
	case WEAPON_MAC10: weapon_path = xs("icons/equipment/mac10"); break;
	case WEAPON_P90: weapon_path = xs("icons/equipment/p90"); break;
	case WEAPON_UMP: weapon_path = xs("icons/equipment/ump45"); break;
	case WEAPON_MP5SD: weapon_path = xs("icons/equipment/mp5sd"); break;
	case WEAPON_XM1024: weapon_path = xs("icons/equipment/xm1014"); break;
	case WEAPON_BIZON: weapon_path = xs("icons/equipment/bizon"); break;
	case WEAPON_MAG7: weapon_path = xs("icons/equipment/mag7"); break;
	case WEAPON_NEGEV: weapon_path = xs("icons/equipment/negev"); break;
	case WEAPON_SAWEDOFF: weapon_path = xs("icons/equipment/sawedoff"); break;
	case WEAPON_TEC9: weapon_path = xs("icons/equipment/tec9"); break;
	case WEAPON_TASER: weapon_path = xs("icons/equipment/taser"); break;
	case WEAPON_HKP2000: weapon_path = xs("icons/equipment/p2000"); break;
	case WEAPON_MP7: weapon_path = xs("icons/equipment/mp7"); break;
	case WEAPON_MP9: weapon_path = xs("icons/equipment/mp9"); break;
	case WEAPON_NOVA: weapon_path = xs("icons/equipment/nova"); break;
	case WEAPON_P250: weapon_path = xs("icons/equipment/p250"); break;
	case WEAPON_SCAR20: weapon_path = xs("icons/equipment/scar20"); break;
	case WEAPON_SG553: weapon_path = xs("icons/equipment/sg556"); break;
	case WEAPON_SSG08: weapon_path = xs("icons/equipment/ssg08"); break;
	case WEAPON_M4A1_S: weapon_path = xs("icons/equipment/m4a1_silencer"); break;
	case WEAPON_USP_S: weapon_path = xs("icons/equipment/usp_silencer"); break;
	case WEAPON_CZ75: weapon_path = xs("icons/equipment/cz75a"); break;
	case WEAPON_REVOLVER: weapon_path = xs("icons/equipment/revolver"); break;
	case WEAPON_HEGRENADE: weapon_path = xs("icons/equipment/hegrenade"); break;
	case WEAPON_MOLOTOV: weapon_path = xs("icons/equipment/molotov"); break;
	case WEAPON_INCENDIARY: weapon_path = xs("icons/equipment/incgrenade"); break;
	case WEAPON_SMOKE: weapon_path = xs("icons/equipment/smokegrenade"); break;
	case WEAPON_FLASHBANG: weapon_path = xs("icons/equipment/flashbang"); break;
	case WEAPON_DECOY: weapon_path = xs("icons/equipment/decoy"); break;
	default: return {};
	}

	return this->GetPanoramaTexture(weapon_path);
}

CMisc::Panorama_t::IconData_t CMisc::Panorama_t::GetWeaponIconByName(const std::string& weapon_name) {
	if (weapon_name.empty())
		return {};

	std::string clean_name = weapon_name;
	if (clean_name.length() >= 7 && clean_name.substr(0, 7) == xs("weapon_"))
		clean_name.erase(0, 7);

	std::vector<std::string> paths_to_try;
	paths_to_try.push_back(xs("icons/equipment/") + clean_name);

	std::string name_without_numbers = clean_name;
	name_without_numbers.erase(std::remove_if(name_without_numbers.begin(), name_without_numbers.end(), ::isdigit), name_without_numbers.end());
	if (!name_without_numbers.empty() && name_without_numbers != clean_name)
		paths_to_try.push_back(xs("icons/equipment/") + name_without_numbers);

	if (clean_name.find(xs("_")) != std::string::npos) {
		std::string name_without_underscore = clean_name;
		name_without_underscore.erase(std::remove(name_without_underscore.begin(), name_without_underscore.end(), '_'), name_without_underscore.end());
		if (!name_without_underscore.empty() && name_without_underscore != clean_name)
			paths_to_try.push_back(xs("icons/equipment/") + name_without_underscore);
	}

	for (const auto& path : paths_to_try) {
		auto icon_data = this->GetPanoramaTexture(path);
		if (icon_data.texture_view && icon_data.width > 0 && icon_data.height > 0)
			return icon_data;
	}

	return {};
}

CMisc::Panorama_t::IconData_t CMisc::Panorama_t::GetGrenadeIcon(int index) {
	switch (index) {
	case -1:
		return this->GetPanoramaTexture(xs("icons/equipment/inferno"));
	case 0:
		return this->GetPanoramaTexture(xs("icons/equipment/hegrenade"));
	case 1:
		return this->GetPanoramaTexture(xs("icons/equipment/flashbang"));
	case 2:
		return this->GetPanoramaTexture(xs("icons/equipment/molotov"));
	case 3:
		return this->GetPanoramaTexture(xs("icons/equipment/decoy"));
	case 4:
		return this->GetPanoramaTexture(xs("icons/equipment/smokegrenade"));
	}
	return {};
}

void CMisc::QuickSwitch_t::Reset() {
	this->last_handle = 0;
	this->armed = false;
	this->pending = false;
	this->pending_ticks = 0;
}

void CMisc::QuickSwitch_t::OnCreateMove() {
	if (!this->enabled || !ctx.cmd || !ctx.base_cmd || !ctx.pawn || !ctx.pawn->Alive() || !Config::Get<bool>(Vars::Misc::QuickSwitch)) {
		this->Reset();
		return;
	}

	auto ws = ctx.pawn->m_pWeaponServices();
	if (!ws) {
		this->Reset();
		return;
	}

	auto active = ws->m_hActiveWeapon();
	const uint32_t active_handle = *reinterpret_cast<uint32_t*>(&active);

	const bool holding = ctx.weapon && ctx.weapon_data && ctx.weapon_data->m_WeaponType() == WEAPONTYPE_GRENADE;

	bool charging = false;
	if (holding) {
		auto grenade = static_cast<C_BaseCSGrenade*>(ctx.weapon);
		charging = grenade->m_bPinPulled() || grenade->m_fThrowTime() > 0.f;
	}

	bool last_owned = false;
	C_CSWeaponBase* primary = nullptr;
	C_CSWeaponBase* secondary = nullptr;
	C_CSWeaponBase* knife = nullptr;

	if (this->armed || this->pending) {
		auto& weapons = ws->m_hMyWeapons();
		for (int i = 0; i < weapons.size; i++) {
			auto handle_value = weapons[i];
			const uint32_t handle = *reinterpret_cast<uint32_t*>(&handle_value);
			if (!handle || handle >= 0xFFFFFFFE)
				continue;

			if (handle == this->last_handle)
				last_owned = true;

			auto weapon = EntitySystem->GetBaseEntity(handle & 0x7FFF).As<C_CSWeaponBase*>();
			if (!weapon)
				continue;

			auto data = weapon->GetWeaponData();
			if (!data)
				continue;

			switch (data->m_WeaponType()) {
			case WEAPONTYPE_SUBMACHINEGUN:
			case WEAPONTYPE_RIFLE:
			case WEAPONTYPE_SHOTGUN:
			case WEAPONTYPE_SNIPER_RIFLE:
			case WEAPONTYPE_MACHINEGUN:
				if (!primary)
					primary = weapon;
				break;
			case WEAPONTYPE_PISTOL:
				if (!secondary)
					secondary = weapon;
				break;
			case WEAPONTYPE_KNIFE:
				if (!knife)
					knife = weapon;
				break;
			default:
				break;
			}
		}
	}

	if (!this->pending) {
		if (charging) {
			this->armed = true;
			this->last_handle = active_handle;
			return;
		}

		if (!this->armed)
			return;

		const bool thrown = (holding && active_handle == this->last_handle) || !last_owned;

		this->armed = false;
		if (!thrown)
			return;

		this->pending = true;
		this->pending_ticks = 0;
	}

	if (charging) {
		this->pending = false;
		this->armed = true;
		this->last_handle = active_handle;
		return;
	}

	if (ctx.weapon && !holding) {
		this->pending = false;
		return;
	}

	if (++this->pending_ticks > 64) {
		this->pending = false;
		return;
	}

	auto target = primary ? primary : (secondary ? secondary : knife);
	if (!target)
		return;

	const int index = target->GetIndex();
	if (index == -1)
		return;

	ctx.base_cmd->set_weaponselect(index);
}
