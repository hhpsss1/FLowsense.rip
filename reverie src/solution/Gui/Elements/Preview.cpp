#include <shared.h>

struct ImVec2;

extern bool ColorPicker(const char* label, float col[4], ImGuiColorEditFlags flags, const char* display_name);

namespace {
	using namespace esp_layout;

	struct EspGroupVars_t {
		std::uint32_t Enable;
		std::uint32_t Name;
		std::uint32_t NameColor;
		std::uint32_t NameFont;
		std::uint32_t Box;
		std::uint32_t BoxGradient;
		std::uint32_t BoxGlow;
		std::uint32_t BoxColor;
		std::uint32_t BoxGradientColor;
		std::uint32_t HealthBar;
		std::uint32_t HealthBarGradient;
		std::uint32_t HealthBarGlow;
		std::uint32_t HealthBarColor;
		std::uint32_t HealthBarGradientColor;
		std::uint32_t HealthBarText;
		std::uint32_t HealthBarFont;
		std::uint32_t AmmoBar;
		std::uint32_t AmmoBarGradient;
		std::uint32_t AmmoBarGlow;
		std::uint32_t AmmoBarColor;
		std::uint32_t AmmoBarGradientColor;
		std::uint32_t AmmoBarText;
		std::uint32_t AmmoBarFont;
		std::uint32_t LayoutABarSide;
		std::uint32_t LayoutABarAlign;
		std::uint32_t LayoutABarOrder;
		std::uint32_t ArmorBar;
		std::uint32_t ArmorBarGradient;
		std::uint32_t ArmorBarGlow;
		std::uint32_t ArmorBarColor;
		std::uint32_t ArmorBarGradientColor;
		std::uint32_t ArmorBarText;
		std::uint32_t ArmorBarFont;
		std::uint32_t LayoutArBarSide;
		std::uint32_t LayoutArBarAlign;
		std::uint32_t LayoutArBarOrder;
		std::uint32_t WeaponName;
		std::uint32_t WeaponNameColor;
		std::uint32_t WeaponNameFont;
		std::uint32_t WeaponIcon;
		std::uint32_t WeaponIconColor;
		std::uint32_t GrenadeIcons;
		std::uint32_t GrenadeIconsColor;
		std::uint32_t Skeleton;
		std::uint32_t SkeletonColor;
		std::uint32_t Fading;
		std::uint32_t FadingDuration;
		std::uint32_t LayoutNameSide;
		std::uint32_t LayoutNameAlign;
		std::uint32_t LayoutNameOrder;
		std::uint32_t LayoutHBarSide;
		std::uint32_t LayoutHBarAlign;
		std::uint32_t LayoutHBarOrder;
		std::uint32_t LayoutWNameSide;
		std::uint32_t LayoutWNameAlign;
		std::uint32_t LayoutWNameOrder;
		std::uint32_t LayoutWIconSide;
		std::uint32_t LayoutWIconAlign;
		std::uint32_t LayoutWIconOrder;
		std::uint32_t LayoutGrenadeIconsSide;
		std::uint32_t LayoutGrenadeIconsAlign;
		std::uint32_t LayoutGrenadeIconsOrder;
		std::uint32_t FlagsScoped;
		std::uint32_t FlagsScopedColor;
		std::uint32_t LayoutFlagScopedSide;
		std::uint32_t LayoutFlagScopedAlign;
		std::uint32_t LayoutFlagScopedOrder;
		std::uint32_t LayoutFlagScopedFont;
		std::uint32_t FlagsBomb;
		std::uint32_t FlagsBombColor;
		std::uint32_t LayoutFlagBombSide;
		std::uint32_t LayoutFlagBombAlign;
		std::uint32_t LayoutFlagBombOrder;
		std::uint32_t LayoutFlagBombFont;
		std::uint32_t FlagsTaser;
		std::uint32_t FlagsTaserColor;
		std::uint32_t LayoutFlagTaserSide;
		std::uint32_t LayoutFlagTaserAlign;
		std::uint32_t LayoutFlagTaserOrder;
		std::uint32_t LayoutFlagTaserFont;
		std::uint32_t FlagsDefuser;
		std::uint32_t FlagsDefuserColor;
		std::uint32_t LayoutFlagDefuserSide;
		std::uint32_t LayoutFlagDefuserAlign;
		std::uint32_t LayoutFlagDefuserOrder;
		std::uint32_t LayoutFlagDefuserFont;
		std::uint32_t FlagsPin;
		std::uint32_t FlagsPinColor;
		std::uint32_t LayoutFlagPinSide;
		std::uint32_t LayoutFlagPinAlign;
		std::uint32_t LayoutFlagPinOrder;
		std::uint32_t LayoutFlagPinFont;
		std::uint32_t FlagsMoney;
		std::uint32_t FlagsMoneyColor;
		std::uint32_t LayoutFlagMoneySide;
		std::uint32_t LayoutFlagMoneyAlign;
		std::uint32_t LayoutFlagMoneyOrder;
		std::uint32_t LayoutFlagMoneyFont;
		std::uint32_t FlagsReload;
		std::uint32_t FlagsReloadColor;
		std::uint32_t LayoutFlagReloadSide;
		std::uint32_t LayoutFlagReloadAlign;
		std::uint32_t LayoutFlagReloadOrder;
		std::uint32_t LayoutFlagReloadFont;
		std::uint32_t FlagsPing;
		std::uint32_t FlagsPingColor;
		std::uint32_t LayoutFlagPingSide;
		std::uint32_t LayoutFlagPingAlign;
		std::uint32_t LayoutFlagPingOrder;
		std::uint32_t LayoutFlagPingFont;
		std::uint32_t FlagsArmor;
		std::uint32_t FlagsArmorColor;
		std::uint32_t LayoutFlagArmorSide;
		std::uint32_t LayoutFlagArmorAlign;
		std::uint32_t LayoutFlagArmorOrder;
		std::uint32_t LayoutFlagArmorFont;
		std::uint32_t FlagsSlow;
		std::uint32_t FlagsSlowColorFull;
		std::uint32_t FlagsSlowColorNormal;
		std::uint32_t LayoutFlagSlowSide;
		std::uint32_t LayoutFlagSlowAlign;
		std::uint32_t LayoutFlagSlowOrder;
		std::uint32_t LayoutFlagSlowFont;
	};

	#define BIND_ESP_GROUP(grp) { \
		grp::Enable, grp::Name, grp::NameColor, grp::NameFont, \
		grp::Box, grp::BoxGradient, grp::BoxGlow, grp::BoxColor, grp::BoxGradientColor, \
		grp::HealthBar, grp::HealthBarGradient, grp::HealthBarGlow, grp::HealthBarColor, grp::HealthBarGradientColor, grp::HealthBarText, grp::HealthBarFont, \
		grp::AmmoBar, grp::AmmoBarGradient, grp::AmmoBarGlow, grp::AmmoBarColor, grp::AmmoBarGradientColor, grp::AmmoBarText, grp::AmmoBarFont, \
		grp::LayoutABarSide, grp::LayoutABarAlign, grp::LayoutABarOrder, \
		grp::ArmorBar, grp::ArmorBarGradient, grp::ArmorBarGlow, grp::ArmorBarColor, grp::ArmorBarGradientColor, grp::ArmorBarText, grp::ArmorBarFont, \
		grp::LayoutArBarSide, grp::LayoutArBarAlign, grp::LayoutArBarOrder, \
		grp::WeaponName, grp::WeaponNameColor, grp::WeaponNameFont, \
		grp::WeaponIcon, grp::WeaponIconColor, \
		grp::GrenadeIcons, grp::GrenadeIconsColor, \
		grp::Skeleton, grp::SkeletonColor, \
		grp::Fading, grp::FadingDuration, \
		grp::LayoutNameSide, grp::LayoutNameAlign, grp::LayoutNameOrder, \
		grp::LayoutHBarSide, grp::LayoutHBarAlign, grp::LayoutHBarOrder, \
		grp::LayoutWNameSide, grp::LayoutWNameAlign, grp::LayoutWNameOrder, \
		grp::LayoutWIconSide, grp::LayoutWIconAlign, grp::LayoutWIconOrder, \
		grp::LayoutGrenadeIconsSide, grp::LayoutGrenadeIconsAlign, grp::LayoutGrenadeIconsOrder, \
		grp::FlagsScoped, grp::FlagsScopedColor, grp::LayoutFlagScopedSide, grp::LayoutFlagScopedAlign, grp::LayoutFlagScopedOrder, grp::LayoutFlagScopedFont, \
		grp::FlagsBomb, grp::FlagsBombColor, grp::LayoutFlagBombSide, grp::LayoutFlagBombAlign, grp::LayoutFlagBombOrder, grp::LayoutFlagBombFont, \
		grp::FlagsTaser, grp::FlagsTaserColor, grp::LayoutFlagTaserSide, grp::LayoutFlagTaserAlign, grp::LayoutFlagTaserOrder, grp::LayoutFlagTaserFont, \
		grp::FlagsDefuser, grp::FlagsDefuserColor, grp::LayoutFlagDefuserSide, grp::LayoutFlagDefuserAlign, grp::LayoutFlagDefuserOrder, grp::LayoutFlagDefuserFont, \
		grp::FlagsPin, grp::FlagsPinColor, grp::LayoutFlagPinSide, grp::LayoutFlagPinAlign, grp::LayoutFlagPinOrder, grp::LayoutFlagPinFont, \
		grp::FlagsMoney, grp::FlagsMoneyColor, grp::LayoutFlagMoneySide, grp::LayoutFlagMoneyAlign, grp::LayoutFlagMoneyOrder, grp::LayoutFlagMoneyFont, \
		grp::FlagsReload, grp::FlagsReloadColor, grp::LayoutFlagReloadSide, grp::LayoutFlagReloadAlign, grp::LayoutFlagReloadOrder, grp::LayoutFlagReloadFont, \
		grp::FlagsPing, grp::FlagsPingColor, grp::LayoutFlagPingSide, grp::LayoutFlagPingAlign, grp::LayoutFlagPingOrder, grp::LayoutFlagPingFont, \
		grp::FlagsArmor, grp::FlagsArmorColor, grp::LayoutFlagArmorSide, grp::LayoutFlagArmorAlign, grp::LayoutFlagArmorOrder, grp::LayoutFlagArmorFont, \
		grp::FlagsSlow, grp::FlagsSlowColorFull, grp::FlagsSlowColorNormal, grp::LayoutFlagSlowSide, grp::LayoutFlagSlowAlign, grp::LayoutFlagSlowOrder, grp::LayoutFlagSlowFont \
	}

	const EspGroupVars_t& GetEspGroup(const std::string& prefix) {
		static const EspGroupVars_t enemy_grp = BIND_ESP_GROUP(Vars::Player::Enemy);
		static const EspGroupVars_t friendly_grp = BIND_ESP_GROUP(Vars::Player::Friendly);
		static const EspGroupVars_t local_grp = BIND_ESP_GROUP(Vars::Player::Local);

		if (prefix.find(xs("Friendly")) != std::string::npos || prefix.find(xs("friendly")) != std::string::npos || prefix.find(xs("Team")) != std::string::npos || prefix.find(xs("team")) != std::string::npos)
			return friendly_grp;

		if (prefix.find(xs("Local")) != std::string::npos || prefix.find(xs("local")) != std::string::npos)
			return local_grp;

		return enemy_grp;
	}

	struct PvGlowAnim_t {
		float v = 0.f;
	};

	struct PvChipAnim_t {
		float fill = 0.f;
		float hover = 0.f;
	};

	struct PvManageDrawer_t {
		float ease = 0.f;
		bool open = false;
	};

	struct PvSettingsPopup_t {
		bool open = false;
		bool hov = false;
		float a = 0.f;
		int owner = -1;
		ImVec2 anchor = ImVec2(0.f, 0.f);
		ImVec2 last_size = ImVec2(230.f, 160.f);
	};

	struct PvSwatchPopup_t {
		bool open = false;
		bool hov = false;
		float a = 0.f;
	};

	struct PvDragState_t {
		bool active = false;
		int dragging_id = -1;
		ImVec2 drag_offset{};
		int pending_side = side_top;
		int pending_align = align_center;
		int pending_order = 0;
	};

	static PvDragState_t pv_drag;

	int* PvGetSideVar(const EspGroupVars_t& grp, int id) {
		switch (id) {
		case 0: return &Config::Get<int>(grp.LayoutNameSide);
		case 1: return &Config::Get<int>(grp.LayoutABarSide);
		case 2: return &Config::Get<int>(grp.LayoutHBarSide);
		case 3: return &Config::Get<int>(grp.LayoutWNameSide);
		case 4: return &Config::Get<int>(grp.LayoutWIconSide);
		case 5: return &Config::Get<int>(grp.LayoutArBarSide);
		case 6: return &Config::Get<int>(grp.LayoutGrenadeIconsSide);
		}
		if (id >= 7 && id < 7 + esp_flags::flag_count) {
			int fid = id - 7;
			switch (fid) {
			case esp_flags::flag_scoped: return &Config::Get<int>(grp.LayoutFlagScopedSide);
			case esp_flags::flag_bomb: return &Config::Get<int>(grp.LayoutFlagBombSide);
			case esp_flags::flag_taser: return &Config::Get<int>(grp.LayoutFlagTaserSide);
			case esp_flags::flag_defuser: return &Config::Get<int>(grp.LayoutFlagDefuserSide);
			case esp_flags::flag_pin: return &Config::Get<int>(grp.LayoutFlagPinSide);
			case esp_flags::flag_money: return &Config::Get<int>(grp.LayoutFlagMoneySide);
			case esp_flags::flag_reload: return &Config::Get<int>(grp.LayoutFlagReloadSide);
			case esp_flags::flag_ping: return &Config::Get<int>(grp.LayoutFlagPingSide);
			case esp_flags::flag_armor: return &Config::Get<int>(grp.LayoutFlagArmorSide);
			case esp_flags::flag_slow: return &Config::Get<int>(grp.LayoutFlagSlowSide);
			}
		}
		return nullptr;
	}

	int* PvGetAlignVar(const EspGroupVars_t& grp, int id) {
		switch (id) {
		case 0: return &Config::Get<int>(grp.LayoutNameAlign);
		case 1: return &Config::Get<int>(grp.LayoutABarAlign);
		case 2: return &Config::Get<int>(grp.LayoutHBarAlign);
		case 3: return &Config::Get<int>(grp.LayoutWNameAlign);
		case 4: return &Config::Get<int>(grp.LayoutWIconAlign);
		case 5: return &Config::Get<int>(grp.LayoutArBarAlign);
		case 6: return &Config::Get<int>(grp.LayoutGrenadeIconsAlign);
		}
		if (id >= 7 && id < 7 + esp_flags::flag_count) {
			int fid = id - 7;
			switch (fid) {
			case esp_flags::flag_scoped: return &Config::Get<int>(grp.LayoutFlagScopedAlign);
			case esp_flags::flag_bomb: return &Config::Get<int>(grp.LayoutFlagBombAlign);
			case esp_flags::flag_taser: return &Config::Get<int>(grp.LayoutFlagTaserAlign);
			case esp_flags::flag_defuser: return &Config::Get<int>(grp.LayoutFlagDefuserAlign);
			case esp_flags::flag_pin: return &Config::Get<int>(grp.LayoutFlagPinAlign);
			case esp_flags::flag_money: return &Config::Get<int>(grp.LayoutFlagMoneyAlign);
			case esp_flags::flag_reload: return &Config::Get<int>(grp.LayoutFlagReloadAlign);
			case esp_flags::flag_ping: return &Config::Get<int>(grp.LayoutFlagPingAlign);
			case esp_flags::flag_armor: return &Config::Get<int>(grp.LayoutFlagArmorAlign);
			case esp_flags::flag_slow: return &Config::Get<int>(grp.LayoutFlagSlowAlign);
			}
		}
		return nullptr;
	}

	int* PvGetOrderVar(const EspGroupVars_t& grp, int id) {
		switch (id) {
		case 0: return &Config::Get<int>(grp.LayoutNameOrder);
		case 1: return &Config::Get<int>(grp.LayoutABarOrder);
		case 2: return &Config::Get<int>(grp.LayoutHBarOrder);
		case 3: return &Config::Get<int>(grp.LayoutWNameOrder);
		case 4: return &Config::Get<int>(grp.LayoutWIconOrder);
		case 5: return &Config::Get<int>(grp.LayoutArBarOrder);
		case 6: return &Config::Get<int>(grp.LayoutGrenadeIconsOrder);
		}
		if (id >= 7 && id < 7 + esp_flags::flag_count) {
			int fid = id - 7;
			switch (fid) {
			case esp_flags::flag_scoped: return &Config::Get<int>(grp.LayoutFlagScopedOrder);
			case esp_flags::flag_bomb: return &Config::Get<int>(grp.LayoutFlagBombOrder);
			case esp_flags::flag_taser: return &Config::Get<int>(grp.LayoutFlagTaserOrder);
			case esp_flags::flag_defuser: return &Config::Get<int>(grp.LayoutFlagDefuserOrder);
			case esp_flags::flag_pin: return &Config::Get<int>(grp.LayoutFlagPinOrder);
			case esp_flags::flag_money: return &Config::Get<int>(grp.LayoutFlagMoneyOrder);
			case esp_flags::flag_reload: return &Config::Get<int>(grp.LayoutFlagReloadOrder);
			case esp_flags::flag_ping: return &Config::Get<int>(grp.LayoutFlagPingOrder);
			case esp_flags::flag_armor: return &Config::Get<int>(grp.LayoutFlagArmorOrder);
			case esp_flags::flag_slow: return &Config::Get<int>(grp.LayoutFlagSlowOrder);
			}
		}
		return nullptr;
	}

	bool PvIsFlagEnabled(const EspGroupVars_t& grp, int fid) {
		switch (fid) {
		case esp_flags::flag_scoped: return Config::Get<bool>(grp.FlagsScoped);
		case esp_flags::flag_bomb: return Config::Get<bool>(grp.FlagsBomb);
		case esp_flags::flag_taser: return Config::Get<bool>(grp.FlagsTaser);
		case esp_flags::flag_defuser: return Config::Get<bool>(grp.FlagsDefuser);
		case esp_flags::flag_pin: return Config::Get<bool>(grp.FlagsPin);
		case esp_flags::flag_money: return Config::Get<bool>(grp.FlagsMoney);
		case esp_flags::flag_reload: return Config::Get<bool>(grp.FlagsReload);
		case esp_flags::flag_ping: return Config::Get<bool>(grp.FlagsPing);
		case esp_flags::flag_armor: return Config::Get<bool>(grp.FlagsArmor);
		case esp_flags::flag_slow: return Config::Get<bool>(grp.FlagsSlow);
		}
		return false;
	}

	Color_t PvGetFlagColor(const EspGroupVars_t& grp, int fid, const esp_flags::Input_t& in) {
		switch (fid) {
		case esp_flags::flag_scoped: return Config::Get<Color_t>(grp.FlagsScopedColor);
		case esp_flags::flag_bomb: return Config::Get<Color_t>(grp.FlagsBombColor);
		case esp_flags::flag_taser: return Config::Get<Color_t>(grp.FlagsTaserColor);
		case esp_flags::flag_defuser: return Config::Get<Color_t>(grp.FlagsDefuserColor);
		case esp_flags::flag_pin: return Config::Get<Color_t>(grp.FlagsPinColor);
		case esp_flags::flag_money: return Config::Get<Color_t>(grp.FlagsMoneyColor);
		case esp_flags::flag_reload: return Config::Get<Color_t>(grp.FlagsReloadColor);
		case esp_flags::flag_ping: return Config::Get<Color_t>(grp.FlagsPingColor);
		case esp_flags::flag_armor: return Config::Get<Color_t>(grp.FlagsArmorColor);
		case esp_flags::flag_slow: {
			if (in.velocity_modifier < 0.99f)
				return Config::Get<Color_t>(grp.FlagsSlowColorFull);
			return Config::Get<Color_t>(grp.FlagsSlowColorNormal);
		}
		}
		return Color_t::White();
	}

	int PvGetFlagFont(const EspGroupVars_t& grp, int fid) {
		switch (fid) {
		case esp_flags::flag_scoped: return Config::Get<int>(grp.LayoutFlagScopedFont);
		case esp_flags::flag_bomb: return Config::Get<int>(grp.LayoutFlagBombFont);
		case esp_flags::flag_taser: return Config::Get<int>(grp.LayoutFlagTaserFont);
		case esp_flags::flag_defuser: return Config::Get<int>(grp.LayoutFlagDefuserFont);
		case esp_flags::flag_pin: return Config::Get<int>(grp.LayoutFlagPinFont);
		case esp_flags::flag_money: return Config::Get<int>(grp.LayoutFlagMoneyFont);
		case esp_flags::flag_reload: return Config::Get<int>(grp.LayoutFlagReloadFont);
		case esp_flags::flag_ping: return Config::Get<int>(grp.LayoutFlagPingFont);
		case esp_flags::flag_armor: return Config::Get<int>(grp.LayoutFlagArmorFont);
		case esp_flags::flag_slow: return Config::Get<int>(grp.LayoutFlagSlowFont);
		}
		return 0;
	}

	const char* PvLidToUid(int lid) {
		switch (lid) {
		case 0: return xs("##pv_name");
		case 1: return xs("##pv_ab");
		case 2: return xs("##pv_hb");
		case 3: return xs("##pv_wname");
		case 4: return xs("##pv_wicon");
		case 5: return xs("##pv_arb");
		case 6: return xs("##pv_gicons");
		case 18: return xs("##pv_box");
		}
		if (lid >= 7 && lid < 7 + esp_flags::flag_count) {
			switch (lid - 7) {
			case esp_flags::flag_scoped: return xs("##pv_flag_scoped");
			case esp_flags::flag_bomb: return xs("##pv_flag_bomb");
			case esp_flags::flag_taser: return xs("##pv_flag_taser");
			case esp_flags::flag_defuser: return xs("##pv_flag_defuser");
			case esp_flags::flag_pin: return xs("##pv_flag_pin");
			case esp_flags::flag_money: return xs("##pv_flag_money");
			case esp_flags::flag_reload: return xs("##pv_flag_reload");
			case esp_flags::flag_ping: return xs("##pv_flag_ping");
			case esp_flags::flag_armor: return xs("##pv_flag_armor");
			case esp_flags::flag_slow: return xs("##pv_flag_slow");
			}
		}
		return xs("##pv_unknown");
	}

	void PvMoveItemTo(const EspGroupVars_t& grp, int id, int new_side, int new_order) {
		int* sv = PvGetSideVar(grp, id);
		int* av = PvGetAlignVar(grp, id);
		int* ov = PvGetOrderVar(grp, id);
		if (!sv || !av || !ov)
			return;

		const int old_side = *sv;
		static const int layout_ids[] = { 0, 1, 2, 3, 4, 5, 6 };

		auto collect_sorted_ids = [&](int side, int exclude_id) {
			std::vector<int> v;

			auto check_add = [&](int item_id) {
				if (item_id == exclude_id)
					return;

				int* osv = PvGetSideVar(grp, item_id);
				int* oov = PvGetOrderVar(grp, item_id);
				if (osv && oov && *osv == side)
					v.push_back(item_id);
			};

			for (int item_id : layout_ids)
				check_add(item_id);

			for (int f = 0; f < esp_flags::flag_count; f++)
				check_add(7 + f);

			std::sort(v.begin(), v.end(), [&](int a, int b) {
				return *PvGetOrderVar(grp, a) < *PvGetOrderVar(grp, b);
			});

			return v;
		};

		auto reassign_compact = [&](std::vector<int>& ids) {
			for (int i = 0; i < (int)ids.size(); i++) {
				int* oov = PvGetOrderVar(grp, ids[i]);
				if (oov)
					*oov = i;
			}
		};

		const bool same_side = (old_side == new_side);

		if (same_side) {
			std::vector<int> members = collect_sorted_ids(old_side, id);
			new_order = std::clamp(new_order, 0, (int)members.size());
			members.insert(members.begin() + new_order, id);
			reassign_compact(members);
		} else {
			std::vector<int> old_members = collect_sorted_ids(old_side, id);
			reassign_compact(old_members);

			std::vector<int> new_members = collect_sorted_ids(new_side, id);
			new_order = std::clamp(new_order, 0, (int)new_members.size());
			new_members.insert(new_members.begin() + new_order, id);
			reassign_compact(new_members);
		}

		*sv = new_side;
		*av = (new_side == side_top || new_side == side_bottom) ? align_center : align_start;
	}

	static PvManageDrawer_t pv_drawer;
	static PvSettingsPopup_t pv_settings;
	static bool pv_settings_just_toggled = false;
	static bool pv_settings_just_opened = false;
	static bool pv_swatch_popup_active = false;

	ImVec2 PvRoundPos(const ImVec2& p) { return ImVec2((float)((int)(p.x + 0.5f)), (float)((int)(p.y + 0.5f))); }

	ImFont* PvPickFont(int idx) {
		if (idx == 0 && Var->font.verdana)
			return Var->font.verdana;

		if (idx == 1 && Var->font.verdanab)
			return Var->font.verdanab;

		if (idx == 2 && Var->font.pixel)
			return Var->font.pixel;

		return Var->font.verdana ? Var->font.verdana : ImGui::GetFont();
	}

	void PvAddTextOutlined(ImDrawList* draw_list, ImFont* font, float font_size, const ImVec2& pos, ImU32 text_col, const char* text) {
		ImVec2 p = PvRoundPos(pos);
		int text_a = (text_col >> IM_COL32_A_SHIFT) & 0xFF;

		draw_list->AddText(font, font_size, ImVec2(p.x + 1.f, p.y + 1.f), IM_COL32(0, 0, 0, (int)(90 * (text_a / 255.f))), text);
		draw_list->AddText(font, font_size, ImVec2(p.x + 2.f, p.y + 2.f), IM_COL32(0, 0, 0, (int)(35 * (text_a / 255.f))), text);
		draw_list->AddText(font, font_size, p, text_col, text);
	}

	float PvGlowCurve(float t) {
		t = std::clamp(t, 0.f, 1.f);
		float f = 1.f - t;
		return f * f;
	}

	void PvAddBoxGlow(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, ImU32 col, float size = 8.f) {
		if (!draw_list || (col & IM_COL32_A_MASK) == 0 || max.x <= min.x || max.y <= min.y)
			return;

		int r = (col >> IM_COL32_R_SHIFT) & 0xFF;
		int g = (col >> IM_COL32_G_SHIFT) & 0xFF;
		int b = (col >> IM_COL32_B_SHIFT) & 0xFF;
		int base_a = (col >> IM_COL32_A_SHIFT) & 0xFF;

		auto make = [&](float t) -> ImU32 {
			int a = (int)(base_a * 0.35f * PvGlowCurve(t));
			return IM_COL32(r, g, b, a);
		};

		const int steps = 10;
		const float step = size / (float)steps;

		for (int i = 0; i < steps; i++) {
			float o0 = step * i;
			float o1 = step * (i + 1);
			float t0 = (float)i / (float)steps;
			float t1 = (float)(i + 1) / (float)steps;
			ImU32 c0 = make(t0);
			ImU32 c1 = make(t1);

			draw_list->AddRectFilledMultiColor(ImVec2(min.x - o0, min.y - o1), ImVec2(max.x + o0, min.y - o0), c1, c1, c0, c0);
			draw_list->AddRectFilledMultiColor(ImVec2(min.x - o0, max.y + o0), ImVec2(max.x + o0, max.y + o1), c0, c0, c1, c1);
			draw_list->AddRectFilledMultiColor(ImVec2(min.x - o1, min.y - o0), ImVec2(min.x - o0, max.y + o0), c1, c0, c0, c1);
			draw_list->AddRectFilledMultiColor(ImVec2(max.x + o0, min.y - o0), ImVec2(max.x + o1, max.y + o0), c0, c1, c1, c0);
		}
	}

	ImU32 PvLerpCol(ImU32 a, ImU32 b, float t) {
		int ar = (a >> IM_COL32_R_SHIFT) & 0xFF;
		int ag = (a >> IM_COL32_G_SHIFT) & 0xFF;
		int ab = (a >> IM_COL32_B_SHIFT) & 0xFF;
		int aa = (a >> IM_COL32_A_SHIFT) & 0xFF;

		int br = (b >> IM_COL32_R_SHIFT) & 0xFF;
		int bg = (b >> IM_COL32_G_SHIFT) & 0xFF;
		int bb = (b >> IM_COL32_B_SHIFT) & 0xFF;
		int ba = (b >> IM_COL32_A_SHIFT) & 0xFF;

		int r = (int)(ar + (br - ar) * t);
		int g = (int)(ag + (bg - ag) * t);
		int bl = (int)(ab + (bb - ab) * t);
		int al = (int)(aa + (ba - aa) * t);

		return IM_COL32(r, g, bl, al);
	}

	void PvAddGradientBorder(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, ImU32 col_a, ImU32 col_b, float thickness = 1.f) {
		float w = max.x - min.x;
		float h = max.y - min.y;
		float perim = 2.f * (w + h);

		if (perim <= 0.001f)
			return;

		ImVec2 pts[5] = { min, ImVec2(max.x, min.y), max, ImVec2(min.x, max.y), min };
		float lengths[4] = { w, h, w, h };
		float acc = 0.f;
		const int total_segs = 48;

		for (int side = 0; side < 4; side++) {
			ImVec2 p0 = pts[side];
			ImVec2 p1 = pts[side + 1];
			int side_segs = std::max(1, (int)(total_segs * (lengths[side] / perim)));

			for (int i = 0; i < side_segs; i++) {
				float t0 = (float)i / side_segs;
				float t1 = (float)(i + 1) / side_segs;
				ImVec2 a = ImLerp(p0, p1, t0);
				ImVec2 b = ImLerp(p0, p1, t1);
				float gt = (acc + lengths[side] * t1) / perim;

				draw_list->AddLine(a, b, PvLerpCol(col_a, col_b, gt), thickness);
			}

			acc += lengths[side];
		}
	}

	float PvGetGlowT(CGui* self, const char* id, bool active) {
		ImGuiID gid = ImGui::GetID(id);
		PvGlowAnim_t* ga = self->AnimContainer(&ga, gid);
		ga->v = ImClamp(ga->v + self->FixedSpeed(14.f) * (active ? 1.f : -1.f), 0.f, 1.f);
		return ga->v;
	}

	void PvDrawElemHoverGlow(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, float t, float alpha) {
		if (t <= 0.001f || max.x <= min.x || max.y <= min.y)
			return;

		float pulse = 0.85f + 0.15f * sinf((float)ImGui::GetTime() * 3.2f);
		float k = t * alpha * pulse;

		ImVec4 ac = Colors->accent.Value;
		ImU32 glow_col = IM_COL32((int)(ac.x * 255.f), (int)(ac.y * 255.f), (int)(ac.z * 255.f), (int)(110.f * k));

		PvAddBoxGlow(draw_list, min, max, glow_col, 12.f);
		Draw->Rect(draw_list, min, max, Draw->GetClr(Colors->accent, k * 0.85f), 2.f, 0, 1.5f);
	}

	struct PvEspElem_t {
		int id;
		const char* label;
		bool* enabled;
		std::function<void()> popup_fn;
	};

	struct PvChipGroup_t {
		const char* title;
		std::vector<PvEspElem_t> elems;
	};

	void PvRenderSettingsColor(const char* label, Color_t* col, bool alpha_support) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		const float row_h = 22.f;
		const float sw_size = 14.f;
		const float avail = ImGui::GetContentRegionAvail().x;
		const ImVec2 pos = window->DC.CursorPos;

		ImGui::Dummy(ImVec2(avail, row_h));

		ImFont* font = Var->font.inter[5] ? Var->font.inter[5] : ImGui::GetFont();
		Draw->TextClipped(window->DrawList, font, ImVec2(pos.x, pos.y), ImVec2(pos.x + avail - sw_size - 8.f, pos.y + row_h), Draw->GetClr(Colors->text, 0.7f), label, nullptr, nullptr, ImVec2(0.f, 0.5f));

		ImVec2 sw_min(pos.x + avail - sw_size, pos.y + (row_h - sw_size) * 0.5f);
		ImVec2 sw_max(sw_min.x + sw_size, sw_min.y + sw_size);

		ImGuiID id = window->GetID(label);
		PvSwatchPopup_t* st = Gui->AnimContainer(&st, id);

		ImVec4 cv(col->Rf(), col->Gf(), col->Bf(), alpha_support ? col->Af() : 1.f);
		window->DrawList->AddRectFilled(sw_min, sw_max, ImGui::ColorConvertFloat4ToU32(cv), 3.f);

		bool hovered = ImGui::IsMouseHoveringRect(sw_min, sw_max);
		window->DrawList->AddRect(sw_min, sw_max, hovered ? IM_COL32(255, 255, 255, 80) : IM_COL32(255, 255, 255, 30), 3.f);

		bool swatch_just_toggled = false;
		bool swatch_just_opened = false;

		if (hovered && ImGui::IsMouseClicked(0)) {
			st->open = !st->open;
			swatch_just_toggled = true;
			swatch_just_opened = st->open;
		}

		if (st->open && !swatch_just_toggled && ImGui::IsMouseClicked(0) && !st->hov)
			st->open = false;

		st->a = ImClamp(st->a + Gui->FixedSpeed(12.f) * (st->open ? 1.f : -1.f), 0.f, 1.f);

		if (st->a > 0.01f)
			pv_swatch_popup_active = true;

		if (st->a < 0.01f)
			return;

		Gui->PushStyleVar(ImGuiStyleVar_Alpha, st->a);
		Gui->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		Gui->PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
		Gui->PushStyleColor(ImGuiCol_WindowBg, Draw->GetClr(Colors->child.background, 0.97f));
		Gui->PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));

		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 popup_pos(sw_min.x - 176.f, sw_max.y + 6.f);
		popup_pos.x = ImClamp(popup_pos.x, 8.f, vp->Size.x - 190.f);
		popup_pos.y = ImClamp(popup_pos.y, 8.f, vp->Size.y - 260.f);

		ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);

		char popup_name[64];
		sprintf_s(popup_name, xs("##settings_swatch_%u"), id);

		if (ImGui::Begin(popup_name, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (swatch_just_opened)
				ImGui::SetWindowFocus();

			st->hov = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

			float tmp[4] = { col->Rf(), col->Gf(), col->Bf(), alpha_support ? col->Af() : 1.f };
			ImGuiColorEditFlags fl = ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview;

			if (alpha_support)
				fl |= ImGuiColorEditFlags_AlphaBar;
			else
				fl |= ImGuiColorEditFlags_NoAlpha;

			if (ColorPicker(xs("##sp"), tmp, fl, nullptr))
				*col = Color_t((uint8_t)(tmp[0] * 255.f), (uint8_t)(tmp[1] * 255.f), (uint8_t)(tmp[2] * 255.f), alpha_support ? (uint8_t)(tmp[3] * 255.f) : (uint8_t)(col->Af() * 255.f));
		}
		ImGui::End();

		Gui->PopStyleVar(3);
		Gui->PopStyleColor(2);
	}

	void PvRenderSettingsPopup(CGui* self, std::vector<PvEspElem_t>& elems) {
		pv_swatch_popup_active = false;

		bool just_toggled = pv_settings_just_toggled;
		bool just_opened = pv_settings_just_opened;
		pv_settings_just_toggled = false;
		pv_settings_just_opened = false;

		pv_settings.a = ImClamp(pv_settings.a + self->FixedSpeed(14.f) * (pv_settings.open ? 1.f : -1.f), 0.f, 1.f);

		if (pv_settings.a < 0.01f)
			return;

		const float ease = pv_settings.a * pv_settings.a * (3.f - 2.f * pv_settings.a);
		const float slide = (1.f - ease) * 10.f;

		self->PushStyleVar(ImGuiStyleVar_Alpha, pv_settings.a);
		self->PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		self->PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
		self->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.f, 12.f));
		self->PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 8.f));
		self->PushStyleColor(ImGuiCol_WindowBg, Draw->GetClr(Colors->child.background, 0.98f));
		self->PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));

		ImGuiViewport* vp = ImGui::GetMainViewport();
		const float popup_w = 230.f;

		ImVec2 pos = pv_settings.anchor;
		pos.x = ImClamp(pos.x, vp->Pos.x + 8.f, vp->Pos.x + vp->Size.x - popup_w - 8.f);
		pos.y = ImClamp(pos.y, vp->Pos.y + 8.f, vp->Pos.y + vp->Size.y - pv_settings.last_size.y - 8.f);

		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + slide), ImGuiCond_Always);
		ImGui::SetNextWindowSizeConstraints(ImVec2(popup_w, 0.f), ImVec2(popup_w, FLT_MAX));

		if (ImGui::Begin(xs("##espv_settings"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (just_opened)
				ImGui::SetWindowFocus();

			pv_settings.hov = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows);
			pv_settings.last_size = ImGui::GetWindowSize();

			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImVec2 win_pos = ImGui::GetWindowPos();
			ImVec2 win_size = ImGui::GetWindowSize();

			for (auto& e : elems) {
				if (e.id != pv_settings.owner)
					continue;

				ImGui::PushID(e.id);

				ImFont* hfont = Var->font.inter[1] ? Var->font.inter[1] : ImGui::GetFont();
				ImGui::PushFont(hfont);
				ImGui::TextUnformatted(e.label);
				ImGui::PopFont();

				const char* close_label = xs("x");
				ImFont* cfont = Var->font.inter[2] ? Var->font.inter[2] : ImGui::GetFont();
				ImVec2 csz = cfont->CalcTextSizeA(cfont->LegacySize, FLT_MAX, 0.f, close_label);
				ImVec2 cmin(win_pos.x + win_size.x - 14.f - csz.x - 6.f, win_pos.y + 10.f);

				bool chov = ImGui::IsMouseHoveringRect(cmin, ImVec2(cmin.x + csz.x + 6.f, cmin.y + csz.y + 4.f));
				if (chov && ImGui::IsMouseClicked(0))
					pv_settings.open = false;

				ImVec4 ccol(1.f, 1.f, 1.f, chov ? 0.9f : 0.4f);
				dl->AddText(cfont, cfont->LegacySize, ImVec2(cmin.x + 3.f, cmin.y), Draw->GetClr(ImColor(ccol)), close_label);

				ImVec2 sep_pos = ImGui::GetCursorScreenPos();
				dl->AddLine(ImVec2(sep_pos.x - 14.f, sep_pos.y), ImVec2(sep_pos.x + win_size.x, sep_pos.y), IM_COL32(255, 255, 255, 14), 1.f);
				ImGui::Dummy(ImVec2(0.f, 2.f));

				ImGui::PushFont(Var->font.inter[2]);
				e.popup_fn();
				ImGui::PopFont();

				ImGui::PopID();
				break;
			}
		}
		ImGui::End();

		self->PopStyleVar(5);
		self->PopStyleColor(2);

		if (pv_settings.open && !just_toggled && ImGui::IsMouseClicked(0) && !pv_settings.hov && !pv_swatch_popup_active)
			pv_settings.open = false;

		if (pv_settings.open && !just_toggled && ImGui::IsMouseClicked(1) && !pv_settings.hov && !pv_swatch_popup_active)
			pv_settings.open = false;
	}

	void PvOpenSettings(int id, const ImVec2& anchor) {
		if (pv_settings.open && pv_settings.owner == id) {
			pv_settings.open = false;
			pv_settings_just_toggled = true;
			return;
		}

		pv_settings.open = true;
		pv_settings.owner = id;
		pv_settings.anchor = anchor;
		pv_settings_just_toggled = true;
		pv_settings_just_opened = true;
	}

	struct PvChipLayout_t {
		ImVec2 min;
		ImVec2 max;
		int index;
	};

	float PvLayoutChips(ImFont* font, std::vector<PvEspElem_t>& elems, float max_width, float chip_h, float gap_x, float gap_y, std::vector<PvChipLayout_t>& out) {
		float cx = 0.f;
		float cy = 0.f;

		for (int i = 0; i < (int)elems.size(); i++) {
			ImVec2 tsz = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, elems[i].label);
			float chip_w = tsz.x + 20.f;

			if (cx + chip_w > max_width && cx > 0.f) {
				cx = 0.f;
				cy += chip_h + gap_y;
			}

			PvChipLayout_t cl;
			cl.min = ImVec2(cx, cy);
			cl.max = ImVec2(cx + chip_w, cy + chip_h);
			cl.index = i;
			out.push_back(cl);

			cx += chip_w + gap_x;
		}

		return cy + chip_h;
	}

	void PvDrawChip(CGui* self, ImDrawList* dl, const ImVec2& min, const ImVec2& max, PvEspElem_t& elem, float alpha) {
		ImGuiID cid = ImGui::GetID(elem.label);
		PvChipAnim_t* ca = self->AnimContainer(&ca, cid);

		ImGui::SetCursorScreenPos(min);
		ImGui::PushID(elem.id + 1000);
		ImGui::InvisibleButton(xs("##chip"), ImVec2(max.x - min.x, max.y - min.y), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		ImGui::SetNextItemAllowOverlap();
		bool hov = ImGui::IsItemHovered();

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			*elem.enabled = !(*elem.enabled);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && elem.popup_fn)
			PvOpenSettings(elem.id, ImGui::GetMousePos());

		ImGui::PopID();

		ca->fill = ImClamp(ca->fill + self->FixedSpeed(14.f) * (*elem.enabled ? 1.f : -1.f), 0.f, 1.f);
		ca->hover = ImClamp(ca->hover + self->FixedSpeed(16.f) * (hov ? 1.f : -1.f), 0.f, 1.f);

		const float rounding = (max.y - min.y) * 0.5f;
		ImFont* font = Var->font.inter[2] ? Var->font.inter[2] : ImGui::GetFont();

		ImVec4 ac = Colors->accent.Value;
		float fill_a = ca->fill * 0.9f * alpha;
		float bg_a = (0.09f + ca->hover * 0.08f) * alpha * (1.f - ca->fill * 0.6f);

		if (fill_a > 0.01f) {
			ImVec4 fc = ac;
			fc.w = fill_a;
			Draw->RectFilled(dl, min, max, Draw->GetClr(ImColor(fc)), rounding);
		}

		if (bg_a > 0.01f)
			Draw->RectFilled(dl, min, max, IM_COL32(255, 255, 255, (int)(bg_a * 255.f)), rounding);

		if (ca->hover > 0.01f && ca->fill < 0.5f) {
			ImVec4 bc = ac;
			bc.w = ca->hover * 0.35f * alpha;
			dl->AddRect(min, max, Draw->GetClr(ImColor(bc)), rounding, 0, 1.f);
		}

		float text_a = alpha * (ca->fill > 0.3f ? 1.f : 0.55f + ca->hover * 0.25f + ca->fill * 0.7f);
		ImVec4 tc = ca->fill > 0.3f ? ImVec4(0.05f, 0.05f, 0.05f, text_a) : ImVec4(1.f, 1.f, 1.f, text_a);

		ImVec2 tsz = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, elem.label);
		ImVec2 tp(min.x + (max.x - min.x) * 0.5f - tsz.x * 0.5f, min.y + (max.y - min.y) * 0.5f - tsz.y * 0.5f);
		dl->AddText(font, font->LegacySize, tp, Draw->GetClr(ImColor(tc)), elem.label);
	}

	float PvComputeDrawerHeight(ImFont* text_font, ImFont* hdr_font, float area_w, std::vector<PvChipGroup_t>& groups, float ease) {
		const float collapsed_h = 30.f;
		const float pad = 10.f;

		ImVec2 tsz = hdr_font->CalcTextSizeA(hdr_font->LegacySize, FLT_MAX, 0.f, xs("manage elements"));

		float content_h = pad + tsz.y + 6.f + 8.f;

		for (int i = 0; i < (int)groups.size(); i++) {
			ImVec2 gsz = text_font->CalcTextSizeA(text_font->LegacySize, FLT_MAX, 0.f, groups[i].title);
			content_h += gsz.y + 4.f;

			std::vector<PvChipLayout_t> layout;
			content_h += PvLayoutChips(text_font, groups[i].elems, area_w - pad * 2.f, 21.f, 5.f, 5.f, layout);

			if (i + 1 < (int)groups.size())
				content_h += 9.f;
		}

		content_h += pad;

		return collapsed_h + (content_h - collapsed_h) * ease;
	}

	void PvDrawBottomDrawer(CGui* self, ImDrawList* dl, const ImVec2& area_min, const ImVec2& area_max, float alpha, std::vector<PvChipGroup_t>& groups) {
		const float header_h = 30.f;
		ImVec2 toggle_min = area_min;
		ImVec2 toggle_max(area_max.x, area_min.y + header_h);

		ImFont* icon_font = Var->font.fa[1];
		ImFont* text_font = Var->font.inter[2] ? Var->font.inter[2] : ImGui::GetFont();
		ImFont* hdr_font = Var->font.inter[1] ? Var->font.inter[1] : ImGui::GetFont();

		const float ease = pv_drawer.ease * pv_drawer.ease * (3.f - 2.f * pv_drawer.ease);
		const float pad = 10.f;

		bool close_clicked = false;

		if (ease > 0.1f) {
			const char* close_label = xs("x");
			ImVec2 csz = text_font->CalcTextSizeA(text_font->LegacySize, FLT_MAX, 0.f, close_label);
			ImVec2 cmin(area_max.x - pad - csz.x - 6.f, area_min.y + pad - 2.f);
			ImVec2 cmax(cmin.x + csz.x + 12.f, cmin.y + csz.y + 12.f);

			if (ImGui::IsMouseHoveringRect(cmin, cmax) && ImGui::IsMouseClicked(0)) {
				close_clicked = true;
				pv_drawer.open = false;
			}
		}

		ImGui::SetCursorScreenPos(toggle_min);
		ImGui::PushID(xs("##drawer_header_toggle"));
		ImGui::InvisibleButton(xs("##btn"), ImVec2(toggle_max.x - toggle_min.x, header_h));
		bool toggle_zone_hov = ImGui::IsItemHovered();

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !close_clicked)
			pv_drawer.open = !pv_drawer.open;

		ImGui::PopID();

		pv_drawer.ease = ImClamp(pv_drawer.ease + self->FixedSpeed(12.f) * (pv_drawer.open ? 1.f : -1.f), 0.f, 1.f);

		const float rounding = 8.f;

		Draw->RectFilled(dl, area_min, area_max, Draw->GetClr(Colors->dropdown.background, alpha), rounding);

		const float btn_alpha = alpha * (1.f - ease);

		if (btn_alpha > 0.01f) {
			const char* title = xs("Manage elements");
			ImVec2 tsz = text_font->CalcTextSizeA(text_font->LegacySize, FLT_MAX, 0.f, title);
			float icon_w = 0.f;

			if (icon_font)
				icon_w = icon_font->CalcTextSizeA(icon_font->LegacySize * 0.7f, FLT_MAX, -1.f, ICON_FA_GEAR).x + 6.f;

			float content_w = tsz.x + icon_w;
			float cy = toggle_min.y + header_h * 0.5f;
			float cx = toggle_min.x + (toggle_max.x - toggle_min.x) * 0.5f - content_w * 0.5f;

			if (icon_font) {
				const float ifs = icon_font->LegacySize * 0.7f;
				ImVec2 isz = icon_font->CalcTextSizeA(ifs, FLT_MAX, -1.f, ICON_FA_GEAR);
				ImVec4 ic = Colors->accent.Value;
				ic.w = btn_alpha * (0.75f + (toggle_zone_hov ? 0.25f : 0.f));
				dl->AddText(icon_font, ifs, ImVec2(cx, cy - isz.y * 0.5f), Draw->GetClr(ImColor(ic)), ICON_FA_GEAR);
				cx += isz.x + 6.f;
			}

			ImVec4 tc(1.f, 1.f, 1.f, btn_alpha * (0.7f + (toggle_zone_hov ? 0.3f : 0.f)));
			dl->AddText(text_font, text_font->LegacySize, ImVec2(cx, cy - tsz.y * 0.5f), Draw->GetClr(ImColor(tc)), title);
		}

		if (ease > 0.01f) {
			float header_alpha = alpha * ease;

			const char* title = xs("manage elements");
			ImVec2 tsz = hdr_font->CalcTextSizeA(hdr_font->LegacySize, FLT_MAX, 0.f, title);
			ImVec2 tpos(area_min.x + pad, area_min.y + pad);
			dl->AddText(hdr_font, hdr_font->LegacySize, tpos, Draw->GetClr(ImColor(ImVec4(1.f, 1.f, 1.f, header_alpha))), title);

			const char* close_label = xs("x");
			ImVec2 csz = text_font->CalcTextSizeA(text_font->LegacySize, FLT_MAX, 0.f, close_label);
			ImVec2 cmin(area_max.x - pad - csz.x - 6.f, area_min.y + pad - 2.f);

			bool chov = ImGui::IsMouseHoveringRect(cmin, ImVec2(cmin.x + csz.x + 12.f, cmin.y + csz.y + 12.f));

			ImVec4 ccol(1.f, 1.f, 1.f, header_alpha * (chov ? 0.9f : 0.5f));
			dl->AddText(text_font, text_font->LegacySize, ImVec2(cmin.x + 3.f, cmin.y), Draw->GetClr(ImColor(ccol)), close_label);

			dl->AddLine(ImVec2(area_min.x + pad, area_min.y + pad + tsz.y + 6.f), ImVec2(area_max.x - pad, area_min.y + pad + tsz.y + 6.f), IM_COL32(255, 255, 255, (int)(10.f * header_alpha)), 1.f);

			float cy = area_min.y + pad + tsz.y + 6.f + 8.f;
			const float chips_w = area_max.x - area_min.x - pad * 2.f;

			for (int gi = 0; gi < (int)groups.size(); gi++) {
				ImVec4 gtc(1.f, 1.f, 1.f, header_alpha * 0.45f);
				dl->AddText(text_font, text_font->LegacySize, ImVec2(area_min.x + pad, cy), Draw->GetClr(ImColor(gtc)), groups[gi].title);

				ImVec2 gsz = text_font->CalcTextSizeA(text_font->LegacySize, FLT_MAX, 0.f, groups[gi].title);
				cy += gsz.y + 4.f;

				std::vector<PvChipLayout_t> layout;
				PvLayoutChips(text_font, groups[gi].elems, chips_w, 21.f, 5.f, 5.f, layout);

				float chips_h = 21.f;

				for (auto& cl : layout) {
					ImVec2 cmin2(area_min.x + pad + cl.min.x, cy + cl.min.y);
					ImVec2 cmax2(area_min.x + pad + cl.max.x, cy + cl.max.y);
					PvDrawChip(self, dl, cmin2, cmax2, groups[gi].elems[cl.index], header_alpha);
					chips_h = std::max(chips_h, cl.max.y);
				}

				cy += chips_h + (gi + 1 < (int)groups.size() ? 9.f : 0.f);
			}
		}
	}

	void PvDrawBarPreview(ImDrawList* dl, const Rect_t& r, int side, float cur_val, float max_val, float alpha, bool glow_on, bool grad_on, Color_t bar_col, Color_t grad_col, ImFont* font, int text_mode, bool is_ammo = false) {
		if (max_val <= 0.f)
			return;

		float pct = std::clamp(cur_val / max_val, 0.f, 1.f);
		bool vertical = (side == side_left || side == side_right);

		ImVec2 bmin = r.min;
		ImVec2 bmax = r.max;

		ImU32 bg = IM_COL32(0, 0, 0, (int)(110 * alpha));
		ImU32 outline = IM_COL32(0, 0, 0, (int)(160 * alpha));

		dl->AddRectFilled(ImVec2(bmin.x - 1.f, bmin.y - 1.f), ImVec2(bmax.x + 1.f, bmax.y + 1.f), outline);
		dl->AddRectFilled(bmin, bmax, bg);

		ImVec2 fmin, fmax;

		if (vertical) {
			float bh = bmax.y - bmin.y;
			float fh = bh * pct;
			fmin = ImVec2(bmin.x, bmax.y - fh);
			fmax = ImVec2(bmax.x, bmax.y);
		} else {
			float bw = bmax.x - bmin.x;
			float fw = bw * pct;
			fmin = ImVec2(bmin.x, bmin.y);
			fmax = ImVec2(bmin.x + fw, bmax.y);
		}

		ImU32 col = IM_COL32((int)(bar_col.Rf() * 255.f), (int)(bar_col.Gf() * 255.f), (int)(bar_col.Bf() * 255.f), (int)(bar_col.Af() * 255.f * alpha));
		bool has_fill = vertical ? (fmax.y > fmin.y) : (fmax.x > fmin.x);

		if (glow_on && has_fill)
			PvAddBoxGlow(dl, fmin, fmax, col, 6.f);

		if (has_fill) {
			if (grad_on) {
				const int segs = 40;
				for (int i = 0; i < segs; i++) {
					float t = (float)i / segs;

					ImU32 sc = IM_COL32(
						(int)((bar_col.Rf() + (grad_col.Rf() - bar_col.Rf()) * t) * 255.f),
						(int)((bar_col.Gf() + (grad_col.Gf() - bar_col.Gf()) * t) * 255.f),
						(int)((bar_col.Bf() + (grad_col.Bf() - bar_col.Bf()) * t) * 255.f),
						(int)((bar_col.Af() + (grad_col.Af() - bar_col.Af()) * t) * 255.f * alpha));

					if (vertical) {
						float sh = (fmax.y - fmin.y) / segs;
						ImVec2 smin(fmin.x, fmax.y - sh * (i + 1));
						ImVec2 smax(fmax.x, fmax.y - sh * i);
						dl->AddRectFilled(smin, smax, sc);
					} else {
						float sw = (fmax.x - fmin.x) / segs;
						ImVec2 smin(fmin.x + sw * i, fmin.y);
						ImVec2 smax(fmin.x + sw * (i + 1), fmax.y);
						dl->AddRectFilled(smin, smax, sc);
					}
				}
			} else {
				dl->AddRectFilled(fmin, fmax, col);
			}
		}

		bool should_draw_text = false;
		if (is_ammo) {
			if (text_mode == 1)
				should_draw_text = true;
		} else {
			if (text_mode == 1 && cur_val < max_val && cur_val > 0.f)
				should_draw_text = true;
			else if (text_mode == 2)
				should_draw_text = true;
		}

		if (font && should_draw_text) {
			std::string val_str;

			if (is_ammo)
				val_str = std::to_string((int)std::round(pct * 100.f));
			else
				val_str = std::to_string((int)std::round(cur_val));

			if (!val_str.empty()) {
				ImVec2 tsz = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, val_str.c_str());
				ImVec2 tp;

				if (side == side_left) {
					float maxb = std::max(bmin.y, bmax.y - tsz.y);
					tp = ImVec2(bmin.x - tsz.x * 0.5f, std::clamp(fmin.y - tsz.y * 0.5f, bmin.y, maxb));
				} else if (side == side_right) {
					float maxb = std::max(bmin.x, bmax.x - tsz.x);
					tp = ImVec2(bmax.x - tsz.x * 0.5f, std::clamp(fmin.y - tsz.y * 0.5f, bmin.y, maxb));
				} else if (side == side_top) {
					float maxb = std::max(bmin.x, bmax.x - tsz.x);
					tp = ImVec2(std::clamp(fmax.x - tsz.x * 0.5f, bmin.x, maxb), bmin.y - tsz.y * 0.5f);
				} else {
					float maxb = std::max(bmin.x, bmax.x - tsz.x);
					tp = ImVec2(std::clamp(fmax.x - tsz.x * 0.5f, bmin.x, maxb), bmin.y - tsz.y * 0.5f);
				}

				PvAddTextOutlined(dl, font, font->LegacySize, tp, IM_COL32(255, 255, 255, (int)(255 * alpha)), val_str.c_str());
			}
		}
	}

	struct PvHoverCand_t {
		ImVec2 min;
		ImVec2 max;
		float area;
		int lid;
		const char* id;
		Rect_t rect;
		int side;
		int align;
		int order;
	};

	std::vector<Item_t> PvBuildLayoutWithGhost(std::vector<Item_t> items, int drag_lid, const Dims_t& drag_sz, int new_side, int new_align, const ImVec2& box_min, const ImVec2& box_max, const ImVec2& mouse, int& out_order) {
		std::vector<Item_t> list;

		for (auto& it : items)
			if (it.lid != drag_lid)
				list.push_back(it);

		ComputeLayout(list, box_min, box_max);

		struct SideItem_t {
			int index;
			float metric;
		};

		std::vector<SideItem_t> group;

		for (int i = 0; i < (int)list.size(); i++) {
			if (list[i].side != new_side)
				continue;

			ImVec2 center((list[i].rect.min.x + list[i].rect.max.x) * 0.5f, (list[i].rect.min.y + list[i].rect.max.y) * 0.5f);

			float metric = 0.f;
			if (new_side == side_top)
				metric = box_min.y - center.y;
			else if (new_side == side_bottom)
				metric = center.y - box_max.y;
			else
				metric = center.y - box_min.y;

			group.push_back({ i, metric });
		}

		std::sort(group.begin(), group.end(), [](const SideItem_t& a, const SideItem_t& b) { return a.metric < b.metric; });

		float mouse_metric = 0.f;
		if (new_side == side_top)
			mouse_metric = box_min.y - mouse.y;
		else if (new_side == side_bottom)
			mouse_metric = mouse.y - box_max.y;
		else
			mouse_metric = mouse.y - box_min.y;

		int insert_index = 0;
		for (size_t i = 0; i < group.size(); i++) {
			if (mouse_metric > group[i].metric)
				insert_index = (int)i + 1;
		}

		for (int seq = 0; seq < (int)group.size(); seq++)
			list[group[seq].index].order = (seq < insert_index) ? seq : seq + 1;

		Item_t ghost{};
		ghost.lid = drag_lid;
		ghost.side = new_side;
		ghost.align = (new_side == side_top || new_side == side_bottom) ? align_center : align_start;
		ghost.order = insert_index;
		ghost.sz = drag_sz;
		list.push_back(ghost);

		ComputeLayout(list, box_min, box_max);

		out_order = insert_index;
		return list;
	}

	struct PvEspGrenadePreview_t {
		int icon_index;
		bool is_active;
		CMisc::Panorama_t::IconData_t icon_data;
	};

	inline int ComputeSide(const ImVec2& box_min, const ImVec2& box_max, const ImVec2& p) {
		float mid_x = (box_min.x + box_max.x) * 0.5f;
		float mid_y = (box_min.y + box_max.y) * 0.5f;
		float hw = std::max((box_max.x - box_min.x) * 0.5f, 1.f);
		float hh = std::max((box_max.y - box_min.y) * 0.5f, 1.f);
		float rx = (p.x - mid_x) / hw;
		float ry = (p.y - mid_y) / hh;

		if (std::abs(rx) > std::abs(ry))
			return rx > 0.f ? side_right : side_left;

		return ry > 0.f ? side_bottom : side_top;
	}

	void PvDrawEspScene(CGui* self, ImDrawList* dl, const ImVec2& scene_min, const ImVec2& scene_max, float alpha, const EspGroupVars_t& grp) {
		const ImVec2 ss(scene_max.x - scene_min.x, scene_max.y - scene_min.y);
		const ImVec2 sc(scene_min.x + ss.x * 0.5f, scene_min.y + ss.y * 0.47f);

		const bool box_on = Config::Get<bool>(grp.Box);
		const bool box_glow_on = Config::Get<bool>(grp.BoxGlow);
		const bool box_grad_on = Config::Get<bool>(grp.BoxGradient);
		const bool name_on = Config::Get<bool>(grp.Name);
		const int name_fidx = Config::Get<int>(grp.NameFont);
		const bool hb_on = Config::Get<bool>(grp.HealthBar);
		const bool hb_glow_on = Config::Get<bool>(grp.HealthBarGlow);
		const bool hb_grad_on = Config::Get<bool>(grp.HealthBarGradient);
		const bool ammo_on = Config::Get<bool>(grp.AmmoBar);
		const bool ammo_glow_on = Config::Get<bool>(grp.AmmoBarGlow);
		const bool ammo_grad_on = Config::Get<bool>(grp.AmmoBarGradient);
		const bool armor_on = Config::Get<bool>(grp.ArmorBar);
		const bool armor_glow_on = Config::Get<bool>(grp.ArmorBarGlow);
		const bool armor_grad_on = Config::Get<bool>(grp.ArmorBarGradient);
		const bool wname_on = Config::Get<bool>(grp.WeaponName);
		const int wname_fidx = Config::Get<int>(grp.WeaponNameFont);
		const bool wicon_on = Config::Get<bool>(grp.WeaponIcon);
		const bool gicons_on = Config::Get<bool>(grp.GrenadeIcons);

		ImFont* name_font = PvPickFont(name_fidx);
		ImFont* wname_font = PvPickFont(wname_fidx);
		ImFont* hb_font = PvPickFont(Config::Get<int>(grp.HealthBarFont));
		ImFont* ab_font = PvPickFont(Config::Get<int>(grp.AmmoBarFont));
		ImFont* arb_font = PvPickFont(Config::Get<int>(grp.ArmorBarFont));

		const char* demo_name = xs("CustomFlow");
		const std::string demo_weapon = xs("ssg08");
		const float demo_health = 22.f + (sinf((float)ImGui::GetTime() * 0.5f) * 0.5f + 0.5f) * 78.f;
		const float demo_ammo = 7.f;
		const float demo_max_ammo = 10.f;
		const float demo_armor = 100.f;

		float box_h = std::clamp(ss.y * 0.81f, 260.f, 480.f);
		float box_w = box_h * 0.48f;

		if (box_w > ss.x * 0.58f) {
			box_w = ss.x * 0.58f;
			box_h = box_w / 0.48f;
		}

		ImVec2 box_min(sc.x - box_w * 0.5f, sc.y - box_h * 0.5f);
		ImVec2 box_max(sc.x + box_w * 0.5f, sc.y + box_h * 0.5f);

		std::vector<PvEspGrenadePreview_t> demo_grenades = {
			{ 1, false, Misc->Panorama.GetGrenadeIcon(1) },
			{ 0, true,  Misc->Panorama.GetGrenadeIcon(0) },
			{ 4, false, Misc->Panorama.GetGrenadeIcon(4) },
			{ 2, false, Misc->Panorama.GetGrenadeIcon(2) }
		};

		auto measure_demo_gicons = [](const std::vector<PvEspGrenadePreview_t>& arr, float h = 10.f, float gap = 3.f) -> Dims_t {
			if (arr.empty())
				return { 0.f, 0.f, false };
			float total_w = 0.f;
			for (size_t i = 0; i < arr.size(); i++) {
				float w = h * 1.2f;
				if (arr[i].icon_data.width > 0 && arr[i].icon_data.height > 0)
					w = h * ((float)arr[i].icon_data.width / (float)arr[i].icon_data.height);
				total_w += w;
				if (i > 0)
					total_w += gap;
			}
			return { total_w, h, false };
		};

		std::vector<Item_t> items;

		if (name_on) {
			ImVec2 tsz = name_font->CalcTextSizeA(name_font->LegacySize, FLT_MAX, 0.f, demo_name);
			items.push_back({ 0, Config::Get<int>(grp.LayoutNameSide), Config::Get<int>(grp.LayoutNameAlign), Config::Get<int>(grp.LayoutNameOrder), { tsz.x, tsz.y, false }, {} });
		}

		if (hb_on)
			items.push_back({ 2, Config::Get<int>(grp.LayoutHBarSide), Config::Get<int>(grp.LayoutHBarAlign), Config::Get<int>(grp.LayoutHBarOrder), MeasureBarDims(hb_font, demo_health, 100.f, Config::Get<int>(grp.HealthBarText)), {} });

		if (ammo_on)
			items.push_back({ 1, Config::Get<int>(grp.LayoutABarSide), Config::Get<int>(grp.LayoutABarAlign), Config::Get<int>(grp.LayoutABarOrder), MeasureBarDims(ab_font, demo_ammo, demo_max_ammo, Config::Get<int>(grp.AmmoBarText)), {} });

		if (armor_on)
			items.push_back({ 5, Config::Get<int>(grp.LayoutArBarSide), Config::Get<int>(grp.LayoutArBarAlign), Config::Get<int>(grp.LayoutArBarOrder), MeasureBarDims(arb_font, demo_armor, 100.f, Config::Get<int>(grp.ArmorBarText)), {} });

		if (wname_on) {
			ImVec2 tsz = wname_font->CalcTextSizeA(wname_font->LegacySize, FLT_MAX, 0.f, demo_weapon.c_str());
			items.push_back({ 3, Config::Get<int>(grp.LayoutWNameSide), Config::Get<int>(grp.LayoutWNameAlign), Config::Get<int>(grp.LayoutWNameOrder), { tsz.x, tsz.y, false }, {} });
		}

		if (wicon_on) {
			CMisc::Panorama_t::IconData_t icon = Misc->Panorama.GetWeaponIconByName(xs("weapon_ssg08"));
			const float ih = 10.f;
			float iw = ih * 2.4f;

			if (icon.texture_view && icon.width > 0 && icon.height > 0)
				iw = ih * ((float)icon.width / (float)icon.height);

			items.push_back({ 4, Config::Get<int>(grp.LayoutWIconSide), Config::Get<int>(grp.LayoutWIconAlign), Config::Get<int>(grp.LayoutWIconOrder), { iw, ih, false }, {} });
		}

		if (gicons_on)
			items.push_back({ 6, Config::Get<int>(grp.LayoutGrenadeIconsSide), Config::Get<int>(grp.LayoutGrenadeIconsAlign), Config::Get<int>(grp.LayoutGrenadeIconsOrder), measure_demo_gicons(demo_grenades), {} });

		esp_flags::Input_t demo_flags_in{};
		demo_flags_in.scoped = true;
		demo_flags_in.bomb = true;
		demo_flags_in.taser = true;
		demo_flags_in.defuser = true;
		demo_flags_in.pin = true;
		demo_flags_in.reload = true;
		demo_flags_in.money = 4750;
		demo_flags_in.ping = 54;
		demo_flags_in.armor = 100;
		demo_flags_in.helmet = true;
		demo_flags_in.velocity_modifier = 0.4f + 0.6f * (0.5f + 0.5f * sinf((float)ImGui::GetTime() * 0.6f));

		std::vector<esp_flags::RenderedFlag_t> demo_flags_list;
		for (int i = 0; i < esp_flags::flag_count; i++) {
			if (!PvIsFlagEnabled(grp, i))
				continue;

			Color_t c = PvGetFlagColor(grp, i, demo_flags_in);
			esp_flags::RenderedFlag_t rf;
			rf.id = i;
			rf.text = esp_flags::FlagText(i, demo_flags_in);
			rf.color = IM_COL32((int)(c.Rf() * 255.f), (int)(c.Gf() * 255.f), (int)(c.Bf() * 255.f), (int)(c.Af() * 255.f));
			demo_flags_list.push_back(rf);
		}

		for (auto& rf : demo_flags_list) {
			ImFont* rf_font = PvPickFont(PvGetFlagFont(grp, rf.id));
			int* sv = PvGetSideVar(grp, 7 + rf.id);
			int* av = PvGetAlignVar(grp, 7 + rf.id);
			int* ov = PvGetOrderVar(grp, 7 + rf.id);
			items.push_back({ 7 + rf.id, sv ? *sv : side_right, av ? *av : align_start, ov ? *ov : 0, esp_flags::MeasureFlagDims(rf_font, rf.text), {} });
		}

		ComputeLayout(items, box_min, box_max);

		auto render_item = [&](int lid, const Rect_t& rect, int side, float a) {
			if (lid == 2) {
				Color_t hc = Config::Get<Color_t>(grp.HealthBarColor);
				Color_t hgc = Config::Get<Color_t>(grp.HealthBarGradientColor);
				PvDrawBarPreview(dl, rect, side, demo_health, 100.f, a, hb_glow_on, hb_grad_on, hc, hgc, hb_font, Config::Get<int>(grp.HealthBarText), false);
			} else if (lid == 1) {
				Color_t ac = Config::Get<Color_t>(grp.AmmoBarColor);
				Color_t agc = Config::Get<Color_t>(grp.AmmoBarGradientColor);
				PvDrawBarPreview(dl, rect, side, demo_ammo, demo_max_ammo, a, ammo_glow_on, ammo_grad_on, ac, agc, ab_font, Config::Get<int>(grp.AmmoBarText), true);
			} else if (lid == 5) {
				Color_t arc = Config::Get<Color_t>(grp.ArmorBarColor);
				Color_t argc = Config::Get<Color_t>(grp.ArmorBarGradientColor);
				PvDrawBarPreview(dl, rect, side, demo_armor, 100.f, a, armor_glow_on, armor_grad_on, arc, argc, arb_font, Config::Get<int>(grp.ArmorBarText), false);
			} else if (lid == 0) {
				Color_t nc = Config::Get<Color_t>(grp.NameColor);
				ImVec2 draw_pos = rect.min;
				ImVec2 tsz = name_font->CalcTextSizeA(name_font->LegacySize, FLT_MAX, 0.f, demo_name);
				draw_pos.y += ((rect.max.y - rect.min.y) - tsz.y) * 0.5f;

				PvAddTextOutlined(dl, name_font, name_font->LegacySize, draw_pos, IM_COL32((int)(nc.Rf() * 255.f), (int)(nc.Gf() * 255.f), (int)(nc.Bf() * 255.f), (int)(nc.Af() * 255.f * a)), demo_name);
			} else if (lid == 3) {
				Color_t wc = Config::Get<Color_t>(grp.WeaponNameColor);
				PvAddTextOutlined(dl, wname_font, wname_font->LegacySize, rect.min, IM_COL32((int)(wc.Rf() * 255.f), (int)(wc.Gf() * 255.f), (int)(wc.Bf() * 255.f), (int)(wc.Af() * 255.f * a)), demo_weapon.c_str());
			} else if (lid == 4) {
				CMisc::Panorama_t::IconData_t icon = Misc->Panorama.GetWeaponIconByName(xs("weapon_ssg08"));
				if (icon.texture_view && icon.width > 0 && icon.height > 0) {
					Color_t ic = Config::Get<Color_t>(grp.WeaponIconColor);
					ImU32 icol = IM_COL32((int)(ic.Rf() * 255.f), (int)(ic.Gf() * 255.f), (int)(ic.Bf() * 255.f), (int)(ic.Af() * 255.f * a));
					dl->AddImage((ImTextureID)icon.texture_view, rect.min, rect.max, ImVec2(0, 0), ImVec2(1, 1), icol);
				}
			} else if (lid == 6) {
				Color_t gc = Config::Get<Color_t>(grp.GrenadeIconsColor);
				float cur_x = rect.min.x;
				const float icon_h = rect.max.y - rect.min.y;
				const float gap = 3.f;

				for (const auto& g_info : demo_grenades) {
					float w = icon_h * 1.2f;
					if (g_info.icon_data.width > 0 && g_info.icon_data.height > 0)
						w = icon_h * ((float)g_info.icon_data.width / (float)g_info.icon_data.height);

					float alpha_mult = g_info.is_active ? 1.0f : 0.5f;
					int icon_alpha = static_cast<int>(gc.Af() * 255.f * a * alpha_mult);

					ImU32 icol = IM_COL32((int)(gc.Rf() * 255.f), (int)(gc.Gf() * 255.f), (int)(gc.Bf() * 255.f), icon_alpha);
					ImVec2 imin(cur_x, rect.min.y);
					ImVec2 imax(cur_x + w, rect.max.y);

					if (g_info.icon_data.texture_view)
						dl->AddImage((ImTextureID)g_info.icon_data.texture_view, imin, imax, ImVec2(0, 0), ImVec2(1, 1), icol);

					cur_x += w + gap;
				}
			} else if (lid >= 7) {
				int fid = lid - 7;

				for (auto& rf : demo_flags_list) {
					if (rf.id != fid)
						continue;

					ImFont* rf_font = PvPickFont(PvGetFlagFont(grp, fid));
					int ca = (rf.color >> IM_COL32_A_SHIFT) & 0xFF;
					ImU32 col = IM_COL32((rf.color >> IM_COL32_R_SHIFT) & 0xFF, (rf.color >> IM_COL32_G_SHIFT) & 0xFF, (rf.color >> IM_COL32_B_SHIFT) & 0xFF, (int)(ca * a));

					PvAddTextOutlined(dl, rf_font, rf_font->LegacySize, rect.min, col, rf.text.c_str());
					break;
				}
			}
		};

		if (box_on) {
			Color_t col = Config::Get<Color_t>(grp.BoxColor);
			Color_t grad_col = Config::Get<Color_t>(grp.BoxGradientColor);
			ImU32 col32 = IM_COL32((int)(col.Rf() * 255.f), (int)(col.Gf() * 255.f), (int)(col.Bf() * 255.f), (int)(col.Af() * 255.f * alpha));
			ImU32 grad32 = IM_COL32((int)(grad_col.Rf() * 255.f), (int)(grad_col.Gf() * 255.f), (int)(grad_col.Bf() * 255.f), (int)(grad_col.Af() * 255.f * alpha));

			if (box_glow_on)
				PvAddBoxGlow(dl, box_min, box_max, box_grad_on ? grad32 : col32, 10.f);

			Draw->Rect(dl, ImVec2(box_min.x - 1.f, box_min.y - 1.f), ImVec2(box_max.x + 1.f, box_max.y + 1.f), IM_COL32(0, 0, 0, (int)(140 * alpha)), 0.f, 0, 1.f);

			if (box_grad_on)
				PvAddGradientBorder(dl, box_min, box_max, col32, grad32, 1.f);
			else
				Draw->Rect(dl, box_min, box_max, col32, 0.f, 0, 1.f);
		}

		std::vector<Item_t> display_items = items;
		int ghost_lid = -1;

		if (pv_drag.active && pv_drag.dragging_id >= 0) {
			Item_t* dragging = nullptr;

			for (auto& it : items) {
				if (it.lid == pv_drag.dragging_id) {
					dragging = &it;
					break;
				}
			}

			if (dragging) {
				ImVec2 mp = ImGui::GetMousePos();
				int new_side = ComputeSide(box_min, box_max, mp);
				int new_align = (new_side == side_top || new_side == side_bottom) ? align_center : align_start;
				int insert_order = 0;

				display_items = PvBuildLayoutWithGhost(items, dragging->lid, dragging->sz, new_side, new_align, box_min, box_max, mp, insert_order);
				ghost_lid = dragging->lid;

				pv_drag.pending_side = new_side;
				pv_drag.pending_align = new_align;
				pv_drag.pending_order = insert_order;
			}
		}

		for (auto& it : display_items) {
			float a = alpha;

			if (ghost_lid != -1)
				a = (it.lid == ghost_lid) ? alpha * 0.95f : alpha * 0.5f;

			if (it.lid == ghost_lid) {
				ImVec4 ac = Colors->accent.Value;
				ImU32 ghost_bg = IM_COL32((int)(ac.x * 255.f), (int)(ac.y * 255.f), (int)(ac.z * 255.f), (int)(70.f * alpha));
				dl->AddRectFilled(it.rect.min, it.rect.max, ghost_bg, 2.f);
				dl->AddRect(it.rect.min, it.rect.max, Draw->GetClr(Colors->accent, alpha * 0.9f), 2.f, 0, 1.5f);
			}

			render_item(it.lid, it.rect, it.side, a);
		}

		std::vector<PvHoverCand_t> cands;
		ImVec2 mp = ImGui::GetMousePos();

		for (auto& it : display_items) {
			const float pad = 3.f;
			ImVec2 zmin(it.rect.min.x - pad, it.rect.min.y - pad);
			ImVec2 zmax(it.rect.max.x + pad, it.rect.max.y + pad);

			if (mp.x >= zmin.x && mp.x <= zmax.x && mp.y >= zmin.y && mp.y <= zmax.y) {
				const char* uid = PvLidToUid(it.lid);
				float area = (zmax.x - zmin.x) * (zmax.y - zmin.y);
				cands.push_back({ zmin, zmax, area, it.lid, uid, it.rect, it.side, it.align, it.order });
			}
		}

		PvHoverCand_t* winner = nullptr;
		for (auto& c : cands) {
			if (!winner) {
				winner = &c;
			} else {
				if (c.area < winner->area)
					winner = &c;
			}
		}

		if (winner && !pv_drawer.open && !pv_settings.open && !pv_swatch_popup_active) {
			if (!pv_drag.active && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				pv_drag.active = true;
				pv_drag.dragging_id = winner->lid;
				ImVec2 center((winner->rect.min.x + winner->rect.max.x) * 0.5f, (winner->rect.min.y + winner->rect.max.y) * 0.5f);
				pv_drag.drag_offset = ImVec2(mp.x - center.x, mp.y - center.y);
				pv_drag.pending_side = winner->side;
				pv_drag.pending_align = winner->align;
				pv_drag.pending_order = winner->order;
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				PvOpenSettings(winner->lid, mp);
			}
		}

		if (pv_drag.active && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			PvMoveItemTo(grp, pv_drag.dragging_id, pv_drag.pending_side, pv_drag.pending_order);

			pv_drag.active = false;
			pv_drag.dragging_id = -1;
		}

		static const char* all_ids[] = {
			xs("##pv_name"), xs("##pv_ab"), xs("##pv_hb"), xs("##pv_wname"), xs("##pv_wicon"), xs("##pv_arb"), xs("##pv_gicons"), xs("##pv_box"),
			xs("##pv_flag_scoped"), xs("##pv_flag_bomb"), xs("##pv_flag_taser"), xs("##pv_flag_defuser"), xs("##pv_flag_pin"),
			xs("##pv_flag_money"), xs("##pv_flag_reload"), xs("##pv_flag_ping"), xs("##pv_flag_armor"), xs("##pv_flag_slow")
		};

		for (const char* id : all_ids) {
			bool active = winner && strcmp(winner->id, id) == 0 && !pv_drag.active;
			float t = PvGetGlowT(self, id, active);

			if (t > 0.001f && winner) {
				PvDrawElemHoverGlow(dl, winner->min, winner->max, t, alpha);
			}
		}
	}
}

void CGui::RenderEspPreview(float alpha, const ImVec2& size_arg, const std::string& prefix) {
	if (alpha <= 0.001f)
		return;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const ImVec2 avail = ImGui::GetContentRegionAvail();

	if (avail.x <= 1.f || avail.y <= 1.f)
		return;

	const ImVec2 content_min = pos;
	const ImVec2 content_max = pos + avail;

	const EspGroupVars_t& grp = GetEspGroup(prefix);

	std::vector<PvEspElem_t> all_elems = {
		{ 0, xs("Name"), &Config::Get<bool>(grp.Name), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.NameColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.NameFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 18, xs("Box"), &Config::Get<bool>(grp.Box), [this, &grp]() {
			this->Checkbox(xs("Glow"), &Config::Get<bool>(grp.BoxGlow));
			this->Checkbox(xs("Gradient"), &Config::Get<bool>(grp.BoxGradient));
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.BoxColor), true);
			if (Config::Get<bool>(grp.BoxGradient))
				PvRenderSettingsColor(xs("Gradient color"), &Config::Get<Color_t>(grp.BoxGradientColor), true);
		} },
		{ 2, xs("Health bar"), &Config::Get<bool>(grp.HealthBar), [this, &grp]() {
			this->Checkbox(xs("Glow"), &Config::Get<bool>(grp.HealthBarGlow));
			this->Checkbox(xs("Gradient"), &Config::Get<bool>(grp.HealthBarGradient));
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.HealthBarColor), true);
			if (Config::Get<bool>(grp.HealthBarGradient))
				PvRenderSettingsColor(xs("Gradient color"), &Config::Get<Color_t>(grp.HealthBarGradientColor), true);
			this->Dropdown(xs("Text mode"), &Config::Get<int>(grp.HealthBarText), { xs("Off"), xs("If < 100"), xs("Always") });
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.HealthBarFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 1, xs("Ammo bar"), &Config::Get<bool>(grp.AmmoBar), [this, &grp]() {
			this->Checkbox(xs("Glow"), &Config::Get<bool>(grp.AmmoBarGlow));
			this->Checkbox(xs("Gradient"), &Config::Get<bool>(grp.AmmoBarGradient));
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.AmmoBarColor), true);
			if (Config::Get<bool>(grp.AmmoBarGradient))
				PvRenderSettingsColor(xs("Gradient color"), &Config::Get<Color_t>(grp.AmmoBarGradientColor), true);
			this->Dropdown(xs("Text mode"), &Config::Get<int>(grp.AmmoBarText), { xs("Off"), xs("Percentage") });
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.AmmoBarFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 5, xs("Armor bar"), &Config::Get<bool>(grp.ArmorBar), [this, &grp]() {
			this->Checkbox(xs("Glow"), &Config::Get<bool>(grp.ArmorBarGlow));
			this->Checkbox(xs("Gradient"), &Config::Get<bool>(grp.ArmorBarGradient));
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.ArmorBarColor), true);
			if (Config::Get<bool>(grp.ArmorBarGradient))
				PvRenderSettingsColor(xs("Gradient color"), &Config::Get<Color_t>(grp.ArmorBarGradientColor), true);
			this->Dropdown(xs("Text mode"), &Config::Get<int>(grp.ArmorBarText), { xs("Off"), xs("If < 100"), xs("Always") });
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.ArmorBarFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 3, xs("Weapon name"), &Config::Get<bool>(grp.WeaponName), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.WeaponNameColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.WeaponNameFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 4, xs("Weapon icon"), &Config::Get<bool>(grp.WeaponIcon), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.WeaponIconColor), true);
		} },
		{ 6, xs("Grenade icons"), &Config::Get<bool>(grp.GrenadeIcons), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.GrenadeIconsColor), true);
		} },
		{ 7, xs("Scoped"), &Config::Get<bool>(grp.FlagsScoped), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsScopedColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagScopedFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 8, xs("Bomb"), &Config::Get<bool>(grp.FlagsBomb), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsBombColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagBombFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 9, xs("Taser"), &Config::Get<bool>(grp.FlagsTaser), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsTaserColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagTaserFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 10, xs("Defuser"), &Config::Get<bool>(grp.FlagsDefuser), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsDefuserColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagDefuserFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 11, xs("Pin"), &Config::Get<bool>(grp.FlagsPin), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsPinColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagPinFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 12, xs("Money"), &Config::Get<bool>(grp.FlagsMoney), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsMoneyColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagMoneyFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 13, xs("Reload"), &Config::Get<bool>(grp.FlagsReload), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsReloadColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagReloadFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 14, xs("Ping"), &Config::Get<bool>(grp.FlagsPing), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsPingColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagPingFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 15, xs("Armor"), &Config::Get<bool>(grp.FlagsArmor), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.FlagsArmorColor), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagArmorFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 16, xs("Slow"), &Config::Get<bool>(grp.FlagsSlow), [this, &grp]() {
			PvRenderSettingsColor(xs("Color in"), &Config::Get<Color_t>(grp.FlagsSlowColorFull), true);
			PvRenderSettingsColor(xs("Color out"), &Config::Get<Color_t>(grp.FlagsSlowColorNormal), true);
			this->Dropdown(xs("Font"), &Config::Get<int>(grp.LayoutFlagSlowFont), { xs("Default"), xs("Bold"), xs("Small") });
		} },
		{ 17, xs("Skeleton"), &Config::Get<bool>(grp.Skeleton), [this, &grp]() {
			PvRenderSettingsColor(xs("Color"), &Config::Get<Color_t>(grp.SkeletonColor), true);
		} },
	};

	auto pick = [&](std::initializer_list<int> ids) {
		std::vector<PvEspElem_t> v;
		for (int id : ids)
			for (auto& e : all_elems)
				if (e.id == id)
					v.push_back(e);
		return v;
	};

	std::vector<PvChipGroup_t> groups = {
		{ xs("Elements"), pick({ 0, 18, 2, 1, 5, 3, 4, 6, 17 }) },
		{ xs("Flags"), pick({ 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }) },
	};

	ImFont* text_font = Var->font.inter[2] ? Var->font.inter[2] : ImGui::GetFont();
	ImFont* hdr_font = Var->font.inter[1] ? Var->font.inter[1] : ImGui::GetFont();

	const float collapsed_h = 30.f;
	const ImVec2 scene_min = content_min;
	const ImVec2 scene_max(content_max.x, content_max.y - collapsed_h - 6.f);

	ImGui::PushClipRect(content_min, content_max, true);
	PvDrawEspScene(this, dl, scene_min, scene_max, alpha, grp);
	ImGui::PopClipRect();

	float drawer_h = PvComputeDrawerHeight(text_font, hdr_font, avail.x - 12.f, groups, pv_drawer.ease);
	drawer_h = std::min(drawer_h, avail.y - 10.f);

	const ImVec2 drawer_min(content_min.x + 6.f, content_max.y - drawer_h - 4.f);
	const ImVec2 drawer_max(content_max.x - 6.f, content_max.y - 4.f);

	PvDrawBottomDrawer(this, dl, drawer_min, drawer_max, alpha, groups);
	PvRenderSettingsPopup(this, all_elems);

	ImGui::Dummy(avail);
}