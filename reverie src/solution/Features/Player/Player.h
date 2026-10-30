#pragma once

namespace esp_layout {
	enum { side_top = 0, side_right = 1, side_bottom = 2, side_left = 3 };
	enum { align_start = 0, align_center = 1, align_end = 2 };

	struct Dims_t {
		float width = 0.f;
		float height = 0.f;
		bool full_span = false;
		float extra_reserve = 0.f;
	};

	struct Rect_t {
		ImVec2 min{};
		ImVec2 max{};
	};

	struct Item_t {
		int lid = 0;
		int side = side_top;
		int align = align_center;
		int order = 0;
		Dims_t sz{};
		Rect_t rect{};
	};

	inline bool IsHorizontalSide(int side) { return side == side_top || side == side_bottom; }

	inline void ResolveDims(int side, const Dims_t& sz, const ImVec2& box_min, const ImVec2& box_max, float& perp, float& along) {
		float box_w = box_max.x - box_min.x;
		float box_h = box_max.y - box_min.y;

		if (IsHorizontalSide(side)) {
			perp = sz.height;
			along = sz.full_span ? box_w : sz.width;
		} else {
			perp = sz.width;
			along = sz.full_span ? box_h : sz.height;
		}
	}

	inline Rect_t PlaceItem(int side, int align, float offset, const Dims_t& sz, const ImVec2& box_min, const ImVec2& box_max) {
		float box_w = box_max.x - box_min.x;
		float box_h = box_max.y - box_min.y;

		float perp, along;
		ResolveDims(side, sz, box_min, box_max, perp, along);

		Rect_t r{};

		if (IsHorizontalSide(side)) {
			float ax_min, ax_max;

			if (align == align_start) {
				ax_min = box_min.x;
				ax_max = ax_min + along;
			} else if (align == align_end) {
				ax_max = box_max.x;
				ax_min = ax_max - along;
			} else {
				float mid = box_min.x + box_w * 0.5f;
				ax_min = mid - along * 0.5f;
				ax_max = mid + along * 0.5f;
			}

			r.min.x = ax_min;
			r.max.x = ax_max;

			if (side == side_top) {
				r.max.y = box_min.y - offset;
				r.min.y = r.max.y - perp;
			} else {
				r.min.y = box_max.y + offset;
				r.max.y = r.min.y + perp;
			}
		} else {
			float ax_min, ax_max;

			if (align == align_start) {
				ax_min = box_min.y;
				ax_max = ax_min + along;
			} else if (align == align_end) {
				ax_max = box_max.y;
				ax_min = ax_max - along;
			} else {
				float mid = box_min.y + box_h * 0.5f;
				ax_min = mid - along * 0.5f;
				ax_max = mid + along * 0.5f;
			}

			r.min.y = ax_min;
			r.max.y = ax_max;

			if (side == side_left) {
				r.max.x = box_min.x - offset;
				r.min.x = r.max.x - perp;
			} else {
				r.min.x = box_max.x + offset;
				r.max.x = r.min.x + perp;
			}
		}

		return r;
	}

	inline void ComputeLayout(std::vector<Item_t>& items, const ImVec2& box_min, const ImVec2& box_max, float base_offset = 4.f, float gap = 2.f, float list_gap = 1.f) {
		for (int side : { side_top, side_bottom }) {
			std::vector<int> idxs;

			for (int i = 0; i < (int)items.size(); i++)
				if (items[i].side == side)
					idxs.push_back(i);

			std::stable_sort(idxs.begin(), idxs.end(), [&](int a, int b) { return items[a].order < items[b].order; });

			float offset = base_offset;

			for (int idx : idxs) {
				items[idx].align = align_center;
				items[idx].rect = PlaceItem(side, align_center, offset, items[idx].sz, box_min, box_max);

				float perp, along;
				ResolveDims(side, items[idx].sz, box_min, box_max, perp, along);
				offset += perp + items[idx].sz.extra_reserve + gap;
			}
		}

		for (int side : { side_left, side_right }) {
			std::vector<int> full_idxs, norm_idxs;

			for (int i = 0; i < (int)items.size(); i++) {
				if (items[i].side != side)
					continue;

				items[i].align = align_start;

				if (items[i].sz.full_span)
					full_idxs.push_back(i);
				else
					norm_idxs.push_back(i);
			}

			std::stable_sort(full_idxs.begin(), full_idxs.end(), [&](int a, int b) { return items[a].order < items[b].order; });
			std::stable_sort(norm_idxs.begin(), norm_idxs.end(), [&](int a, int b) { return items[a].order < items[b].order; });

			float perp_offset = base_offset;

			for (int idx : full_idxs) {
				items[idx].rect = PlaceItem(side, align_start, perp_offset, items[idx].sz, box_min, box_max);

				float perp, along;
				ResolveDims(side, items[idx].sz, box_min, box_max, perp, along);
				perp_offset += perp + items[idx].sz.extra_reserve + gap;
			}

			const float list_offset = perp_offset;
			float y_cursor = box_min.y;

			for (int idx : norm_idxs) {
				const Dims_t& sz = items[idx].sz;

				Rect_t r{};
				r.min.y = y_cursor;
				r.max.y = y_cursor + sz.height;

				if (side == side_left) {
					r.max.x = box_min.x - list_offset;
					r.min.x = r.max.x - sz.width;
				} else {
					r.min.x = box_max.x + list_offset;
					r.max.x = r.min.x + sz.width;
				}

				items[idx].rect = r;
				y_cursor += sz.height + sz.extra_reserve + list_gap;
			}
		}
	}

	inline Dims_t MeasureBarDims(ImFont* font, float cur_val, float max_val, int text_mode) {
		float reserve = 0.f;
		bool show_text = false;

		if (text_mode == 1 && cur_val < max_val && cur_val > 0.f)
			show_text = true;
		else if (text_mode == 2)
			show_text = true;

		if (font && show_text) {
			ImVec2 tsz = font->CalcTextSizeA(font->LegacySize, std::numeric_limits<float>::max(), 0.f, xs("100%"));
			reserve = tsz.y * 0.5f + 1.f;
		}

		return { 1.f, 1.f, true, reserve };
	}
}

namespace esp_flags {
    enum {
        flag_scoped = 0,
        flag_bomb,
        flag_taser,
        flag_defuser,
        flag_pin,
        flag_money,
        flag_reload,
        flag_ping,
        flag_armor,
        flag_slow,
        flag_count
    };

    enum EPlayerGroup {
        group_enemy = 0,
        group_friendly,
        group_local
    };

    inline int ResolveGroup(const std::string& prefix) {
        if (prefix.find("FRIENDLY") != std::string::npos || prefix.find("Friendly") != std::string::npos)
            return group_friendly;
        if (prefix.find("LOCAL") != std::string::npos || prefix.find("Local") != std::string::npos)
            return group_local;
        return group_enemy;
    }

    struct Input_t {
        bool scoped = false;
        bool bomb = false;
        bool taser = false;
        bool defuser = false;
        bool pin = false;
        bool reload = false;
        int money = 0;
        int ping = 0;
        int armor = 0;
        bool helmet = false;
        float velocity_modifier = 1.f;
    };

    struct State_t {
        float alpha[flag_count] = {};
    };

    struct RenderedFlag_t {
        int id;
        std::string text;
        ImU32 color;
    };

    inline bool FlagEnabledCfg(int group, int id) {
        #define GET_FLAG_ENABLE(ns) \
            switch (id) { \
            case flag_scoped: return Config::Get<bool>(ns::FlagsScoped); \
            case flag_bomb: return Config::Get<bool>(ns::FlagsBomb); \
            case flag_taser: return Config::Get<bool>(ns::FlagsTaser); \
            case flag_defuser: return Config::Get<bool>(ns::FlagsDefuser); \
            case flag_pin: return Config::Get<bool>(ns::FlagsPin); \
            case flag_money: return Config::Get<bool>(ns::FlagsMoney); \
            case flag_reload: return Config::Get<bool>(ns::FlagsReload); \
            case flag_ping: return Config::Get<bool>(ns::FlagsPing); \
            case flag_armor: return Config::Get<bool>(ns::FlagsArmor); \
            case flag_slow: return Config::Get<bool>(ns::FlagsSlow); \
            }

        switch (group) {
        case group_friendly: { GET_FLAG_ENABLE(Vars::Player::Friendly); break; }
        case group_local: { GET_FLAG_ENABLE(Vars::Player::Local); break; }
        default: { GET_FLAG_ENABLE(Vars::Player::Enemy); break; }
        }
        #undef GET_FLAG_ENABLE

        return false;
    }

    inline bool FlagEnabledCfg(const std::string& prefix, int id) {
        return FlagEnabledCfg(ResolveGroup(prefix), id);
    }

    inline bool FlagCondition(int id, const Input_t& in) {
        switch (id) {
        case flag_scoped: return in.scoped;
        case flag_bomb: return in.bomb;
        case flag_taser: return in.taser;
        case flag_defuser: return in.defuser;
        case flag_pin: return in.pin;
        case flag_money: return true;
        case flag_reload: return in.reload;
        case flag_ping: return true;
        case flag_armor: return true;
        case flag_slow: return in.velocity_modifier < 0.999f;
        }
        return false;
    }

    inline Color_t FlagColor(int group, int id, const Input_t& in) {
        #define GET_FLAG_COLOR(ns) \
            switch (id) { \
            case flag_scoped: return Config::Get<Color_t>(ns::FlagsScopedColor); \
            case flag_bomb: return Config::Get<Color_t>(ns::FlagsBombColor); \
            case flag_taser: return Config::Get<Color_t>(ns::FlagsTaserColor); \
            case flag_defuser: return Config::Get<Color_t>(ns::FlagsDefuserColor); \
            case flag_pin: return Config::Get<Color_t>(ns::FlagsPinColor); \
            case flag_money: return Config::Get<Color_t>(ns::FlagsMoneyColor); \
            case flag_reload: return Config::Get<Color_t>(ns::FlagsReloadColor); \
            case flag_ping: return Config::Get<Color_t>(ns::FlagsPingColor); \
            case flag_armor: return Config::Get<Color_t>(ns::FlagsArmorColor); \
            case flag_slow: { \
                Color_t c0 = Config::Get<Color_t>(ns::FlagsSlowColorNormal); \
                Color_t c1 = Config::Get<Color_t>(ns::FlagsSlowColorFull); \
                float t = std::clamp(1.f - in.velocity_modifier, 0.f, 1.f); \
                return Color_t( \
                    (uint8_t)(c0.Rf() * 255.f + (c1.Rf() - c0.Rf()) * 255.f * t), \
                    (uint8_t)(c0.Gf() * 255.f + (c1.Gf() - c0.Gf()) * 255.f * t), \
                    (uint8_t)(c0.Bf() * 255.f + (c1.Bf() - c0.Bf()) * 255.f * t), \
                    (uint8_t)(c0.Af() * 255.f + (c1.Af() - c0.Af()) * 255.f * t) \
                ); \
            } \
            }

        switch (group) {
        case group_friendly: { GET_FLAG_COLOR(Vars::Player::Friendly); break; }
        case group_local: { GET_FLAG_COLOR(Vars::Player::Local); break; }
        default: { GET_FLAG_COLOR(Vars::Player::Enemy); break; }
        }
        #undef GET_FLAG_COLOR

        return Color_t::White();
    }

    inline Color_t FlagColor(const std::string& prefix, int id, const Input_t& in) {
        return FlagColor(ResolveGroup(prefix), id, in);
    }

    inline std::string FlagText(int id, const Input_t& in) {
        switch (id) {
        case flag_scoped: return xs("SCOPED");
        case flag_bomb: return xs("BOMB");
        case flag_taser: return xs("TASER");
        case flag_defuser: return xs("DEFUSER");
        case flag_pin: return xs("PIN");
        case flag_money: {
            char buf[32];
            sprintf_s(buf, xs("$%d"), in.money);
            return buf;
        }
        case flag_reload: return xs("RELOAD");
        case flag_ping: {
            char buf[32];
            sprintf_s(buf, xs("%dms"), in.ping);
            return buf;
        }
        case flag_armor: {
            if (in.armor <= 0)
                return xs("NA");
            return in.helmet ? xs("HK") : xs("K");
        }
        case flag_slow: return xs("SLOW");
        }
        return "";
    }

    inline int FlagSide(int group, int id) {
        #define GET_FLAG_SIDE(ns) \
            switch (id) { \
            case flag_scoped: return Config::Get<int>(ns::LayoutFlagScopedSide); \
            case flag_bomb: return Config::Get<int>(ns::LayoutFlagBombSide); \
            case flag_taser: return Config::Get<int>(ns::LayoutFlagTaserSide); \
            case flag_defuser: return Config::Get<int>(ns::LayoutFlagDefuserSide); \
            case flag_pin: return Config::Get<int>(ns::LayoutFlagPinSide); \
            case flag_money: return Config::Get<int>(ns::LayoutFlagMoneySide); \
            case flag_reload: return Config::Get<int>(ns::LayoutFlagReloadSide); \
            case flag_ping: return Config::Get<int>(ns::LayoutFlagPingSide); \
            case flag_armor: return Config::Get<int>(ns::LayoutFlagArmorSide); \
            case flag_slow: return Config::Get<int>(ns::LayoutFlagSlowSide); \
            }

        switch (group) {
        case group_friendly: { GET_FLAG_SIDE(Vars::Player::Friendly); break; }
        case group_local: { GET_FLAG_SIDE(Vars::Player::Local); break; }
        default: { GET_FLAG_SIDE(Vars::Player::Enemy); break; }
        }
        #undef GET_FLAG_SIDE

        return 3;
    }

    inline int FlagSide(const std::string& prefix, int id) {
        return FlagSide(ResolveGroup(prefix), id);
    }

    inline int FlagAlign(int group, int id) {
        #define GET_FLAG_ALIGN(ns) \
            switch (id) { \
            case flag_scoped: return Config::Get<int>(ns::LayoutFlagScopedAlign); \
            case flag_bomb: return Config::Get<int>(ns::LayoutFlagBombAlign); \
            case flag_taser: return Config::Get<int>(ns::LayoutFlagTaserAlign); \
            case flag_defuser: return Config::Get<int>(ns::LayoutFlagDefuserAlign); \
            case flag_pin: return Config::Get<int>(ns::LayoutFlagPinAlign); \
            case flag_money: return Config::Get<int>(ns::LayoutFlagMoneyAlign); \
            case flag_reload: return Config::Get<int>(ns::LayoutFlagReloadAlign); \
            case flag_ping: return Config::Get<int>(ns::LayoutFlagPingAlign); \
            case flag_armor: return Config::Get<int>(ns::LayoutFlagArmorAlign); \
            case flag_slow: return Config::Get<int>(ns::LayoutFlagSlowAlign); \
            }

        switch (group) {
        case group_friendly: { GET_FLAG_ALIGN(Vars::Player::Friendly); break; }
        case group_local: { GET_FLAG_ALIGN(Vars::Player::Local); break; }
        default: { GET_FLAG_ALIGN(Vars::Player::Enemy); break; }
        }
        #undef GET_FLAG_ALIGN

        return 0;
    }

    inline int FlagAlign(const std::string& prefix, int id) {
        return FlagAlign(ResolveGroup(prefix), id);
    }

    inline int FlagOrder(int group, int id) {
        #define GET_FLAG_ORDER(ns) \
            switch (id) { \
            case flag_scoped: return Config::Get<int>(ns::LayoutFlagScopedOrder); \
            case flag_bomb: return Config::Get<int>(ns::LayoutFlagBombOrder); \
            case flag_taser: return Config::Get<int>(ns::LayoutFlagTaserOrder); \
            case flag_defuser: return Config::Get<int>(ns::LayoutFlagDefuserOrder); \
            case flag_pin: return Config::Get<int>(ns::LayoutFlagPinOrder); \
            case flag_money: return Config::Get<int>(ns::LayoutFlagMoneyOrder); \
            case flag_reload: return Config::Get<int>(ns::LayoutFlagReloadOrder); \
            case flag_ping: return Config::Get<int>(ns::LayoutFlagPingOrder); \
            case flag_armor: return Config::Get<int>(ns::LayoutFlagArmorOrder); \
            case flag_slow: return Config::Get<int>(ns::LayoutFlagSlowOrder); \
            }

        switch (group) {
        case group_friendly: { GET_FLAG_ORDER(Vars::Player::Friendly); break; }
        case group_local: { GET_FLAG_ORDER(Vars::Player::Local); break; }
        default: { GET_FLAG_ORDER(Vars::Player::Enemy); break; }
        }
        #undef GET_FLAG_ORDER

        return 0;
    }

    inline int FlagOrder(const std::string& prefix, int id) {
        return FlagOrder(ResolveGroup(prefix), id);
    }

    inline int FlagFont(int group, int id) {
        #define GET_FLAG_FONT(ns) \
            switch (id) { \
            case flag_scoped: return Config::Get<int>(ns::LayoutFlagScopedFont); \
            case flag_bomb: return Config::Get<int>(ns::LayoutFlagBombFont); \
            case flag_taser: return Config::Get<int>(ns::LayoutFlagTaserFont); \
            case flag_defuser: return Config::Get<int>(ns::LayoutFlagDefuserFont); \
            case flag_pin: return Config::Get<int>(ns::LayoutFlagPinFont); \
            case flag_money: return Config::Get<int>(ns::LayoutFlagMoneyFont); \
            case flag_reload: return Config::Get<int>(ns::LayoutFlagReloadFont); \
            case flag_ping: return Config::Get<int>(ns::LayoutFlagPingFont); \
            case flag_armor: return Config::Get<int>(ns::LayoutFlagArmorFont); \
            case flag_slow: return Config::Get<int>(ns::LayoutFlagSlowFont); \
            }

        switch (group) {
        case group_friendly: { GET_FLAG_FONT(Vars::Player::Friendly); break; }
        case group_local: { GET_FLAG_FONT(Vars::Player::Local); break; }
        default: { GET_FLAG_FONT(Vars::Player::Enemy); break; }
        }
        #undef GET_FLAG_FONT

        return 0;
    }

    inline int FlagFont(const std::string& prefix, int id) {
        return FlagFont(ResolveGroup(prefix), id);
    }

    inline void UpdateState(int group, State_t& st, const Input_t& in, float dt) {
        const float speed = 10.f;

        for (int i = 0; i < flag_count; i++) {
            bool target = FlagEnabledCfg(group, i) && FlagCondition(i, in);
            float goal = target ? 1.f : 0.f;
            float k = std::clamp(dt * speed, 0.f, 1.f);

            st.alpha[i] += (goal - st.alpha[i]) * k;

            if (fabsf(st.alpha[i] - goal) < 0.001f)
                st.alpha[i] = goal;
        }
    }

    inline void UpdateState(const std::string& prefix, State_t& st, const Input_t& in, float dt) {
        UpdateState(ResolveGroup(prefix), st, in, dt);
    }

    inline std::vector<RenderedFlag_t> BuildRenderList(int group, const State_t& st, const Input_t& in) {
        std::vector<RenderedFlag_t> out;

        for (int i = 0; i < flag_count; i++) {
            if (st.alpha[i] <= 0.003f)
                continue;

            Color_t c = FlagColor(group, i, in);

            RenderedFlag_t rf;
            rf.id = i;
            rf.text = FlagText(i, in);
            rf.color = IM_COL32((int)(c.Rf() * 255.f), (int)(c.Gf() * 255.f), (int)(c.Bf() * 255.f), (int)(c.Af() * 255.f * st.alpha[i]));

            out.push_back(rf);
        }

        return out;
    }

    inline std::vector<RenderedFlag_t> BuildRenderList(const std::string& prefix, const State_t& st, const Input_t& in) {
        return BuildRenderList(ResolveGroup(prefix), st, in);
    }

    inline esp_layout::Dims_t MeasureFlagDims(ImFont* font, const std::string& text) {
        if (!font || text.empty())
            return { 0.f, 0.f, false };

        ImVec2 sz = font->CalcTextSizeA(font->LegacySize, std::numeric_limits<float>::max(), 0.f, text.c_str());
        return { sz.x, sz.y, false };
    }
}

struct Visuals_t {
	struct Player_t {
		void OnPresent();
	private:
		struct CachedPlayerData_t {
			std::string name;
			std::string weapon_name;
			uint16_t def_index;
			float health{ 100.f };
			float ammo{ 0.f };
			float max_ammo{ 0.f };
			float armor{ 0.f };
			float death_time{ 0.f };
			bool is_dead{ false };
			ImVec2 box_min{};
			ImVec2 box_max{};
			Vector origin{};
			Vector mins{};
			Vector maxs{};
		};

		bool GetBoundingBox(const Vector& origin, const Vector& mins, const Vector& maxs, ImVec2& out_min, ImVec2& out_max);
		void Render(const std::string& prefix, CCSPlayerController* controller, C_CSPlayerPawn* pawn, ImDrawList* draw_list);
		void RenderCached(const std::string& prefix, uintptr_t cache_id, CachedPlayerData_t& cache, float alpha, ImDrawList* draw_list);
        void DrawSkeleton(const std::string& prefix, C_CSPlayerPawn* pawn, ImDrawList* dl);

		std::unordered_map<uintptr_t, CachedPlayerData_t> cached_players;
		std::unordered_map<uintptr_t, CBaseHandle> last_non_grenade_weapon;
	} Player;
};

inline Visuals_t* Visuals = new Visuals_t();