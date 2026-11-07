#include <shared.h>

extern int GetActiveSubtab(int);

static int s_skins_view = 0;
static int s_skins_selected_weapon = 0;
static int s_skins_category = WEAPON_CAT_ALL;
static int s_skins_team_mode = 2;
static int s_skins_active_team = 0;
static char s_skins_search[64] = "";
static char s_collection_name_input[64] = "";
static int s_collections_filter = 0;

static bool s_team_data_init = false;
static int s_pending_key = -1;
static int s_pending_paint = 0;
static float s_pending_wear = 0.01f;
static int s_pending_seed = 1;
static int s_skins_prev_view = 0;
static uint16_t s_selected_weapon_def = 0;
static uint16_t s_selected_agent_def = 0;

static int s_agent_active_team = 0;
static int s_agent_team_mode = 2;
static char s_agent_search[64]{};
static int s_agent_category = 0;
static CSkins::Item_t s_default_agent{};

struct CollectionItemEntry_t {
	uint16_t def_index{ 0 };
	int paint_t{ 0 };
	float wear_t{ 0.01f };
	int seed_t{ 1 };
	int paint_ct{ 0 };
	float wear_ct{ 0.01f };
	int seed_ct{ 1 };
};

struct SkinCollection_t {
	std::string name{};
	std::string date_str{};
	bool is_favourite{ false };
	std::vector<CollectionItemEntry_t> items{};
	uint16_t agent_t{ 0 };
	uint16_t agent_ct{ 0 };
};

static std::vector<SkinCollection_t> s_saved_collections;

struct CCollectionsManager {
	std::vector<SkinCollection_t>& GetAll() {
		return s_saved_collections;
	}

	void Save(const std::string& name, const std::vector<CollectionItemEntry_t>& items, uint16_t agent_t, uint16_t agent_ct, bool fav = false) {
		for (auto& col : s_saved_collections) {
			if (col.name == name) {
				col.items = items;
				col.agent_t = agent_t;
				col.agent_ct = agent_ct;
				return;
			}
		}

		s_saved_collections.push_back({ name, xs("Today"), fav, items, agent_t, agent_ct });
	}

	void Remove(const std::string& name) {
		std::erase_if(s_saved_collections, [&](const SkinCollection_t& c) { return c.name == name; });
	}

	void ToggleFavourite(const std::string& name) {
		for (auto& col : s_saved_collections) {
			if (col.name == name) {
				col.is_favourite = !col.is_favourite;
				break;
			}
		}
	}

	void ShowInExplorer(const std::string& name) {}
};

static CCollectionsManager CollectionsManager;

template<typename T, typename = void>
struct HasDimensions : std::false_type {};

template<typename T>
struct HasDimensions<T, std::void_t<decltype(std::declval<T>().width), decltype(std::declval<T>().height)>> : std::true_type {};

template<typename T>
static float GetIconAspect(const T& icon) {
	if constexpr (HasDimensions<T>::value) {
		if (icon.width > 0 && icon.height > 0)
			return static_cast<float>(icon.width) / static_cast<float>(icon.height);
	}

	return 1.45f;
}

static ImU32 GetRarityU32(int rarity) {
	switch (rarity) {
	case 1: return IM_COL32(176, 195, 217, 255);
	case 2: return IM_COL32(94, 152, 217, 255);
	case 3: return IM_COL32(75, 105, 255, 255);
	case 4: return IM_COL32(136, 71, 255, 255);
	case 5: return IM_COL32(211, 44, 230, 255);
	case 6: return IM_COL32(235, 75, 75, 255);
	case 7: return IM_COL32(228, 174, 57, 255);
	default: return IM_COL32(70, 75, 88, 255);
	}
}

static void DrawFittedImage(ImDrawList* dl, ImTextureID tex, ImVec2 bb_min, ImVec2 bb_max, float aspect = 4.0f / 3.0f) {
	if (!tex)
		return;

	const float box_w = bb_max.x - bb_min.x;
	const float box_h = bb_max.y - bb_min.y;
	if (box_w <= 0.f || box_h <= 0.f)
		return;

	float draw_w = box_w;
	float draw_h = draw_w / aspect;

	if (draw_h > box_h) {
		draw_h = box_h;
		draw_w = draw_h * aspect;
	}

	const ImVec2 img_min(bb_min.x + (box_w - draw_w) * 0.5f, bb_min.y + (box_h - draw_h) * 0.5f);
	const ImVec2 img_max(img_min.x + draw_w, img_min.y + draw_h);

	dl->AddImage(tex, img_min, img_max);
}

static void DrawCustomSearch(const char* id, const char* hint, char* buf, size_t buf_sz, float w, float h, ImFont* font_text, ImFont* font_icon, ImU32 bg, ImU32 border, ImU32 text_col, ImU32 hint_col) {
	ImVec2 p = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(28.f, (h - font_text->LegacySize) * 0.5f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, bg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, bg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, bg);
	ImGui::PushStyleColor(ImGuiCol_Border, border);
	ImGui::PushStyleColor(ImGuiCol_Text, text_col);
	ImGui::PushStyleColor(ImGuiCol_TextDisabled, hint_col);

	ImGui::SetNextItemWidth(w);
	ImGui::PushFont(font_text);
	ImGui::InputTextWithHint(id, hint, buf, buf_sz);
	ImGui::PopFont();

	ImGui::PopStyleColor(6);
	ImGui::PopStyleVar(3);

	ImVec2 isz = font_icon->CalcTextSizeA(font_icon->LegacySize, FLT_MAX, -1.f, ICON_FA_MAGNIFYING_GLASS);
	dl->AddText(font_icon, font_icon->LegacySize, ImVec2(p.x + 8.f, p.y + (h - isz.y) * 0.5f), hint_col, ICON_FA_MAGNIFYING_GLASS);
}

void CGui::RenderSkinsContent() {
	const int s_skins_subtab = GetActiveSubtab(5);

	auto& weapons = Skins->weapon_items;
	auto& knives = Skins->knife_items;
	auto& gloves = Skins->gloves_items;
	auto& agents = Skins->agent_items;

	if (weapons.empty()) {
		ImGui::PushFont(Var->font.inter[0]);
		ImGui::TextColored(ImVec4(0.5f, 0.54f, 0.62f, 1.0f), xs("No weapons loaded"));
		ImGui::PopFont();
		return;
	}

	auto get_knife_by_def = [&](uint16_t def) -> CSkins::Item_t* {
		for (auto& k : knives) {
			if (k.def_index == def)
				return &k;
		}

		return knives.empty() ? nullptr : &knives[0];
	};

	auto get_glove_by_def = [&](uint16_t def) -> CSkins::Item_t* {
		for (auto& g : gloves) {
			if (g.def_index == def)
				return &g;
		}

		return gloves.empty() ? nullptr : &gloves[0];
	};

	auto get_agent_by_def = [&](uint16_t def) -> CSkins::Item_t* {
		if (def == 0)
			return &s_default_agent;

		for (auto& a : agents) {
			if (a.def_index == def)
				return &a;
		}

		return &s_default_agent;
	};

	auto get_item_by_def = [&](uint16_t def) -> CSkins::Item_t* {
		for (auto& w : weapons) {
			if (w.def_index == def)
				return &w;
		}

		for (auto& k : knives) {
			if (k.def_index == def)
				return &k;
		}

		for (auto& g : gloves) {
			if (g.def_index == def)
				return &g;
		}

		return &weapons[0];
	};

	auto get_agent_team = [](const CSkins::Item_t& item) -> int {
		if (item.def_index == 0)
			return -1;

		if (item.model_path.find(xs("ctm_")) != std::string::npos || item.icon_path.find(xs("ctm_")) != std::string::npos || item.icon_path.find(xs("ct_")) != std::string::npos)
			return 1;

		return 0;
	};

	if (!s_team_data_init) {
		s_default_agent.def_index = 0;
		s_default_agent.name = xs("Default Agent");
		s_default_agent.type = ITEM_TYPE_AGENT;
		s_default_agent.rarity = 0;

		Skins->glove_def[0] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::GloveT));
		if (Skins->glove_def[0] == 0)
			Skins->glove_def[0] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::Glove));

		Skins->glove_def[1] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::GloveCT));
		if (Skins->glove_def[1] == 0)
			Skins->glove_def[1] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::Glove));

		Skins->agent_def[0] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::AgentT));
		if (Skins->agent_def[0] == 0)
			Skins->agent_def[0] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::Agent));

		Skins->agent_def[1] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::AgentCT));
		if (Skins->agent_def[1] == 0)
			Skins->agent_def[1] = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::Agent));

		auto init_item_presets = [](const CSkins::Item_t& item) {
			Skins->skin_data[0][item.def_index] = { 0, 0.01f, 1 };
			Skins->skin_data[1][item.def_index] = { 0, 0.01f, 1 };
		};

		for (auto& w : weapons)
			init_item_presets(w);

		for (auto& k : knives)
			init_item_presets(k);

		for (auto& g : gloves)
			init_item_presets(g);

		s_team_data_init = true;
	}

	if (s_selected_weapon_def == 0)
		s_selected_weapon_def = weapons[0].def_index;

	CSkins::Item_t* s_skins_selected_item = get_item_by_def(s_selected_weapon_def);
	CSkins::Item_t* s_agent_selected_item = get_agent_by_def(s_selected_agent_def);

	auto apply_item_data = [&](const CSkins::Item_t& item, int skin_idx, float wear, int seed, int mode) {
		if (item.type == ITEM_TYPE_KNIFE) {
			auto purge_knives = [&](int team) {
				for (auto& k : knives) {
					if (k.def_index != item.def_index)
						Skins->skin_data[team].erase(k.def_index);
				}
			};

			if (mode == 2 || mode == 0) {
				Skins->knif_def[0] = item.def_index;
				purge_knives(0);
			}

			if (mode == 2 || mode == 1) {
				Skins->knif_def[1] = item.def_index;
				purge_knives(1);
			}
		} else if (item.type == ITEM_TYPE_GLOVE) {
			auto purge_gloves = [&](int team) {
				for (auto& g : gloves) {
					if (g.def_index != item.def_index)
						Skins->skin_data[team].erase(g.def_index);
				}
			};

			if (mode == 2 || mode == 0) {
				Skins->glove_def[0] = item.def_index;
				purge_gloves(0);
			}

			if (mode == 2 || mode == 1) {
				Skins->glove_def[1] = item.def_index;
				purge_gloves(1);
			}

			Config::Set<int>(Vars::Skins::GloveT, static_cast<int>(Skins->glove_def[0]));
			Config::Set<int>(Vars::Skins::GloveCT, static_cast<int>(Skins->glove_def[1]));
			Config::Set<int>(Vars::Skins::Glove, static_cast<int>(Skins->glove_def[s_skins_active_team]));
		}

		auto save_team = [&](int team) {
			Skins->skin_data[team][item.def_index] = { skin_idx, wear, seed };
		};

		if (mode == 2) {
			save_team(0);
			save_team(1);
		} else {
			save_team(mode);
		}
	};

	ImU32 c_text = Draw->GetClr(Colors->text);
	ImU32 c_text_dim = Draw->GetClr(Colors->text_secondary, 0.65f);
	ImU32 c_accent = Draw->GetClr(Colors->accent);
	ImU32 c_card_bg = Draw->GetClr(Colors->button.background, 0.45f);
	ImU32 c_card_bg_hov = Draw->GetClr(Colors->button.background, 0.70f);
	ImU32 c_border = IM_COL32(255, 255, 255, 15);
	ImU32 c_field_bg = Draw->GetClr(Colors->button.background, 0.50f);

	const ImU32 c_team_t = IM_COL32(235, 168, 52, 255);
	const ImU32 c_team_ct = IM_COL32(75, 145, 245, 255);
	const float card_rounding = 8.f;

	auto draw_team_outline = [&](ImDrawList* dl, ImVec2 min, ImVec2 max, bool is_t, bool is_ct, float rounding = 8.f, float thickness = 1.5f) {
		if (is_t && is_ct) {
			float mid_x = (min.x + max.x) * 0.5f;
			dl->PushClipRect(min, ImVec2(mid_x, max.y), true);
			dl->AddRect(min, max, c_team_t, rounding, 0, thickness);
			dl->PopClipRect();
			dl->PushClipRect(ImVec2(mid_x, min.y), max, true);
			dl->AddRect(min, max, c_team_ct, rounding, 0, thickness);
			dl->PopClipRect();
		} else if (is_t) {
			dl->AddRect(min, max, c_team_t, rounding, 0, thickness);
		} else if (is_ct) {
			dl->AddRect(min, max, c_team_ct, rounding, 0, thickness);
		}
	};

	const float root_w = ImGui::GetContentRegionAvail().x;
	const float root_h = ImGui::GetContentRegionAvail().y;

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.f);
	ImGui::BeginChild(xs("##skins_root"), ImVec2(root_w, root_h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	{
		const float avail_w = ImGui::GetContentRegionAvail().x;
		const float avail_h = ImGui::GetContentRegionAvail().y;

		if (s_skins_subtab == 0) {
			auto& sel_item = *s_skins_selected_item;

			int committed_paint = 0;
			float committed_wear = 0.01f;
			int committed_seed = 1;

			auto it = Skins->skin_data[s_skins_active_team].find(sel_item.def_index);
			if (it != Skins->skin_data[s_skins_active_team].end()) {
				committed_paint = it->second.paint;
				committed_wear = it->second.wear;
				committed_seed = it->second.seed;
			}

			const int cur_key = static_cast<int>(sel_item.def_index) * 100 + s_skins_active_team * 10 + s_skins_team_mode;
			if (s_pending_key != cur_key) {
				s_pending_key = cur_key;
				s_pending_paint = committed_paint;
				s_pending_wear = committed_wear;
				s_pending_seed = committed_seed;
			}

			const float left_col_w = ImClamp(avail_w * 0.34f, 260.f, 340.f);
			const float col_gap = 20.f;
			const float right_col_w = avail_w - left_col_w - col_gap;

			const ImVec2 outer_pos = ImGui::GetCursorScreenPos();

			ImGui::SetCursorScreenPos(outer_pos);
			ImGui::BeginChild(xs("##loadout_col"), ImVec2(left_col_w, avail_h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				ImDrawList* dl = ImGui::GetWindowDrawList();
				const ImVec2 col_pos = ImGui::GetWindowPos();

				ImGui::PushFont(Var->font.inter[1]);
				dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, col_pos, c_text, xs("Loadout"));
				ImGui::PopFont();

				const float header_h = 36.f;
				const float footer_gap = 14.f;
				const float footer_btn_h = 42.f;
				const float footer_h = footer_gap + footer_btn_h;
				const float scroll_h = ImMax(avail_h - header_h - footer_h, 150.f);

				ImGui::SetCursorScreenPos(ImVec2(col_pos.x, col_pos.y + header_h));
				ImGui::BeginChild(xs("##loadout_scroll"), ImVec2(left_col_w, scroll_h), false);
				{
					ImDrawList* sdl = ImGui::GetWindowDrawList();
					const float prev_h = 132.f;
					ImVec2 pp = ImGui::GetCursorScreenPos();

					std::string cur_skin_name = xs("Default");
					int paintkit = 0;

					if (s_pending_paint > 0 && static_cast<size_t>(s_pending_paint - 1) < sel_item.paint_kits.size()) {
						auto& pk = sel_item.paint_kits[s_pending_paint - 1];
						cur_skin_name = pk.name;
						paintkit = pk.id;
					}

					ImVec2 f_center(pp.x + left_col_w * 0.5f, pp.y + prev_h * 0.5f);
					const char* fallback_ico = (sel_item.type == ITEM_TYPE_KNIFE) ? ICON_FA_SHIELD_HALVED : ((sel_item.type == ITEM_TYPE_GLOVE) ? ICON_FA_HAND : ICON_FA_GUN);
					ImVec2 f_sz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 2.2f, FLT_MAX, -1.f, fallback_ico);
					sdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 2.2f, f_center - f_sz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), fallback_ico);

					ImVec2 name_pos(pp.x, pp.y + prev_h + 10.f);
					ImGui::PushFont(Var->font.inter[1]);
					sdl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, name_pos, c_text, sel_item.name.c_str());
					ImGui::PopFont();

					ImVec2 skin_pos(name_pos.x, name_pos.y + Var->font.inter[1]->LegacySize + 6.f);
					ImGui::PushFont(Var->font.inter[0]);
					sdl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, skin_pos, c_accent, cur_skin_name.c_str());
					ImGui::PopFont();

					ImGui::SetCursorScreenPos(ImVec2(skin_pos.x, skin_pos.y + Var->font.inter[0]->LegacySize + 16.f));

					{
						ImVec2 row_p = ImGui::GetCursorScreenPos();
						const float pill_w = 150.f;
						const float pill_h = 24.f;
						const float item_w = pill_w / 3.f;
						ImVec2 pill_pos(row_p.x + left_col_w - pill_w, row_p.y);

						ImGui::PushFont(Var->font.inter[0]);
						sdl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, ImVec2(row_p.x, row_p.y + (pill_h - Var->font.inter[0]->LegacySize) * 0.5f), c_text_dim, xs("Team"));
						ImGui::PopFont();

						sdl->AddRectFilled(pill_pos, ImVec2(pill_pos.x + pill_w, pill_pos.y + pill_h), c_field_bg, 6.f);

						const char* team_opts[3] = { xs("T"), xs("CT"), xs("Both") };
						for (int team = 0; team < 3; ++team) {
							ImVec2 opt_min(pill_pos.x + team * item_w, pill_pos.y);
							ImVec2 opt_max(opt_min.x + item_w, opt_min.y + pill_h);

							ImGui::SetCursorScreenPos(opt_min);
							ImGui::PushID(team + 400);
							if (ImGui::InvisibleButton(xs("##team_btn"), ImVec2(item_w, pill_h))) {
								s_skins_team_mode = team;
								if (team == 0)
									s_skins_active_team = 0;
								else if (team == 1)
									s_skins_active_team = 1;

								auto team_it = Skins->skin_data[s_skins_active_team].find(sel_item.def_index);
								if (team_it != Skins->skin_data[s_skins_active_team].end()) {
									s_pending_paint = team_it->second.paint;
									s_pending_wear = team_it->second.wear;
									s_pending_seed = team_it->second.seed;
								} else {
									s_pending_paint = 0;
									s_pending_wear = 0.01f;
									s_pending_seed = 1;
								}

								s_pending_key = static_cast<int>(sel_item.def_index) * 100 + s_skins_active_team * 10 + s_skins_team_mode;
							}

							if (s_skins_team_mode == team) {
								ImU32 act_col = (team == 0) ? c_team_t : ((team == 1) ? c_team_ct : Draw->GetClr(Colors->accent, 0.85f));
								sdl->AddRectFilled(opt_min, opt_max, act_col, 6.f);
							}

							ImGui::PushFont(Var->font.inter[0]);
							ImVec2 tsz = Var->font.inter[0]->CalcTextSizeA(Var->font.inter[0]->LegacySize, FLT_MAX, -1.f, team_opts[team]);
							sdl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, opt_min + (ImVec2(item_w, pill_h) - tsz) * 0.5f, (s_skins_team_mode == team) ? IM_COL32(15, 16, 20, 255) : c_text_dim, team_opts[team]);
							ImGui::PopFont();
							ImGui::PopID();
						}

						ImGui::SetCursorScreenPos(ImVec2(row_p.x, row_p.y + pill_h + 12.f));
					}

					ImGuiStyle* g_style = &ImGui::GetStyle();
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(g_style->ItemSpacing.x, 8.f));

					int wear_idx = 0;
					if (s_pending_wear < 0.07f)
						wear_idx = 0;
					else if (s_pending_wear < 0.15f)
						wear_idx = 1;
					else if (s_pending_wear < 0.38f)
						wear_idx = 2;
					else if (s_pending_wear < 0.45f)
						wear_idx = 3;
					else
						wear_idx = 4;

					int prev_wear_idx = wear_idx;
					this->Dropdown(xs("Wear"), &wear_idx, { xs("Factory New"), xs("Minimal Wear"), xs("Field-Tested"), xs("Well-Worn"), xs("Battle-Scarred") });
					if (wear_idx != prev_wear_idx) {
						switch (wear_idx) {
						case 0:
							s_pending_wear = 0.01f;
							break;
						case 1:
							s_pending_wear = 0.09f;
							break;
						case 2:
							s_pending_wear = 0.20f;
							break;
						case 3:
							s_pending_wear = 0.40f;
							break;
						case 4:
							s_pending_wear = 0.70f;
							break;
						}
					}

					this->SliderInt(xs("Pattern seed"), &s_pending_seed, 1, 1000);
					this->SliderFloat(xs("Float"), &s_pending_wear, 0.0001f, 1.0f, xs("%.4f"));
					ImGui::PopStyleVar();
				}
				ImGui::Dummy(ImVec2(left_col_w, scroll_h));
				ImGui::EndChild();

				ImVec2 btn_min(col_pos.x, col_pos.y + avail_h - footer_btn_h);
				ImVec2 btn_max(btn_min.x + left_col_w, btn_min.y + footer_btn_h);

				ImGui::SetCursorScreenPos(btn_min);
				ImGui::InvisibleButton(xs("##apply_btn"), ImVec2(left_col_w, footer_btn_h));
				bool b_hov = ImGui::IsItemHovered();
				if (ImGui::IsItemClicked()) {
					apply_item_data(sel_item, s_pending_paint, s_pending_wear, s_pending_seed, s_skins_team_mode);
					applied_paint_kits.clear();
					Skins->should_update = true;
				}

				dl->AddRectFilled(btn_min, btn_max, Draw->GetClr(Colors->accent, b_hov ? 1.f : 0.9f), 7.f);

				const char* b_label = xs("Apply to loadout");
				ImVec2 icon_sz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize, FLT_MAX, -1.f, ICON_FA_CHECK);
				ImVec2 txt_sz = Var->font.inter[1]->CalcTextSizeA(Var->font.inter[1]->LegacySize, FLT_MAX, -1.f, b_label);
				float full_tw = icon_sz.x + 8.f + txt_sz.x;

				ImVec2 icon_pos(btn_min.x + (left_col_w - full_tw) * 0.5f, btn_min.y + (footer_btn_h - icon_sz.y) * 0.5f);
				ImVec2 txt_pos(icon_pos.x + icon_sz.x + 8.f, btn_min.y + (footer_btn_h - txt_sz.y) * 0.5f);

				dl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize, icon_pos, IM_COL32(15, 16, 20, 255), ICON_FA_CHECK);
				dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, txt_pos, IM_COL32(15, 16, 20, 255), b_label);
			}
			ImGui::EndChild();

			ImGui::SetCursorScreenPos(ImVec2(outer_pos.x + left_col_w + col_gap, outer_pos.y));
			ImGui::BeginChild(xs("##inv_right_pane"), ImVec2(right_col_w, avail_h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				ImDrawList* dl = ImGui::GetWindowDrawList();
				const ImVec2 col_pos = ImGui::GetWindowPos();
				const float title_h = ImMax(Var->font.inter[1]->LegacySize, Var->font.inter[0]->LegacySize);

				const char* title_text = xs("Inventory");
				if (s_skins_view == 1)
					title_text = sel_item.name.c_str();
				else if (s_skins_view == 2)
					title_text = xs("Select Knife");
				else if (s_skins_view == 3)
					title_text = xs("Select Gloves");

				ImGui::PushFont(Var->font.inter[1]);
				dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, col_pos, c_text, title_text);
				ImGui::PopFont();

				ImGui::SetCursorScreenPos(ImVec2(col_pos.x, col_pos.y + title_h + 14.f));
				const float toolbar_h = 32.f;

				if (s_skins_view == 0) {
					const float dd_w = 130.f;
					const float s_w = right_col_w - dd_w - 8.f;
					DrawCustomSearch(xs("##search_inv"), xs("Search inventory"), s_skins_search, sizeof(s_skins_search), s_w, toolbar_h, Var->font.inter[0], Var->font.fa[1], c_field_bg, c_border, c_text, c_text_dim);

					ImGui::SameLine(0.f, 8.f);
					ImGui::SetNextItemWidth(dd_w);
					this->Dropdown(xs("##wpn_cat_dd"), &s_skins_category, { xs("All items"), xs("Rifles"), xs("Pistols"), xs("SMGs"), xs("Heavy"), xs("Snipers"), xs("Knives"), xs("Gloves") });
				} else {
					const float back_w = 78.f;
					const float s_w = right_col_w - back_w - 8.f;
					DrawCustomSearch(xs("##search_skins"), xs("Search..."), s_skins_search, sizeof(s_skins_search), s_w, toolbar_h, Var->font.inter[0], Var->font.fa[1], c_field_bg, c_border, c_text, c_text_dim);

					ImGui::SameLine(0.f, 8.f);
					ImVec2 b_min = ImGui::GetCursorScreenPos();
					ImVec2 b_max(b_min.x + back_w, b_min.y + toolbar_h);

					ImGui::InvisibleButton(xs("##back_to_prev"), ImVec2(back_w, toolbar_h));
					bool b_hov = ImGui::IsItemHovered();
					if (ImGui::IsItemClicked()) {
						if (s_skins_view == 1) {
							s_skins_view = s_skins_prev_view;
						} else {
							s_skins_view = 0;
							s_skins_prev_view = 0;
						}

						s_skins_search[0] = '\0';
					}

					dl->AddRectFilled(b_min, b_max, b_hov ? c_card_bg_hov : c_card_bg, 6.f);
					dl->AddRect(b_min, b_max, c_border, 6.f);

					ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize, FLT_MAX, -1.f, ICON_FA_ARROW_LEFT);
					ImVec2 tsz = Var->font.inter[1]->CalcTextSizeA(Var->font.inter[1]->LegacySize, FLT_MAX, -1.f, xs("Back"));
					float btw = isz.x + 6.f + tsz.x;

					ImVec2 ip(b_min.x + (back_w - btw) * 0.5f, b_min.y + (toolbar_h - isz.y) * 0.5f);
					ImVec2 tp(ip.x + isz.x + 6.f, b_min.y + (toolbar_h - tsz.y) * 0.5f);

					dl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize, ip, c_text, ICON_FA_ARROW_LEFT);
					dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, tp, c_text, xs("Back"));
				}

				const float header_bottom_y = ImGui::GetCursorScreenPos().y;
				const float footer_h = 40.f;
				const float footer_gap = 12.f;
				const float grid_gap = 14.f;
				const float footer_top_y = col_pos.y + avail_h - footer_h;
				const float grid_top_y = header_bottom_y + grid_gap;
				const float grid_h = ImMax(footer_top_y - footer_gap - grid_top_y, 100.f);
				const float card_gap = 10.f;
				const float card_w = (right_col_w - card_gap) * 0.5f;
				const float card_h = card_w * 0.78f;

				ImGui::SetCursorScreenPos(ImVec2(col_pos.x, grid_top_y));
				ImGui::BeginChild(xs("##inv_grid_scroll"), ImVec2(right_col_w, grid_h), false);
				{
					if (s_skins_view == 0) {
						struct InvCardEntry_t {
							int type;
							size_t weapon_index;
						};

						std::vector<InvCardEntry_t> display_cards;

						bool show_knife = (s_skins_category == 0 || s_skins_category == 6);
						if (show_knife && !knives.empty()) {
							auto* cur_k = get_knife_by_def(Skins->knif_def[s_skins_active_team]);
							std::string kname = cur_k ? cur_k->name : xs("Knife");
							int kp_idx = cur_k ? Skins->skin_data[s_skins_active_team][cur_k->def_index].paint : 0;
							std::string ksname = (cur_k && kp_idx > 0 && static_cast<size_t>(kp_idx - 1) < cur_k->paint_kits.size()) ? cur_k->paint_kits[kp_idx - 1].name : xs("Default");

							if (s_skins_search[0] == '\0') {
								display_cards.push_back({ 1, 0 });
							} else {
								std::string full_name = kname + xs(" ") + ksname + xs(" knife");
								auto it = std::search(full_name.begin(), full_name.end(), s_skins_search, s_skins_search + strlen(s_skins_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
								if (it != full_name.end())
									display_cards.push_back({ 1, 0 });
							}
						}

						bool show_glove = (s_skins_category == 0 || s_skins_category == 7);
						if (show_glove && !gloves.empty()) {
							auto* cur_g = get_glove_by_def(Skins->glove_def[s_skins_active_team]);
							std::string gname = cur_g ? cur_g->name : xs("Gloves");
							int gp_idx = cur_g ? Skins->skin_data[s_skins_active_team][cur_g->def_index].paint : 0;
							std::string gsname = (cur_g && gp_idx > 0 && static_cast<size_t>(gp_idx - 1) < cur_g->paint_kits.size()) ? cur_g->paint_kits[gp_idx - 1].name : xs("Default");

							if (s_skins_search[0] == '\0') {
								display_cards.push_back({ 2, 0 });
							} else {
								std::string full_name = gname + xs(" ") + gsname + xs(" gloves");
								auto it = std::search(full_name.begin(), full_name.end(), s_skins_search, s_skins_search + strlen(s_skins_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
								if (it != full_name.end())
									display_cards.push_back({ 2, 0 });
							}
						}

						if (s_skins_category >= 0 && s_skins_category <= 5) {
							for (size_t i = 0; i < weapons.size(); ++i) {
								auto& item = weapons[i];
								if (s_skins_category != 0 && GetWeaponCategory(item.def_index) != s_skins_category)
									continue;

								int p_idx = Skins->skin_data[s_skins_active_team][item.def_index].paint;
								std::string skin_name = xs("Default");
								if (p_idx > 0 && static_cast<size_t>(p_idx - 1) < item.paint_kits.size())
									skin_name = item.paint_kits[p_idx - 1].name;

								if (s_skins_search[0] != '\0') {
									std::string full_name = item.name + xs(" ") + skin_name;
									auto it = std::search(full_name.begin(), full_name.end(), s_skins_search, s_skins_search + strlen(s_skins_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
									if (it == full_name.end())
										continue;
								}

								display_cards.push_back({ 0, i });
							}
						}

						for (size_t di = 0; di < display_cards.size(); ++di) {
							auto& card = display_cards[di];
							ImDrawList* gdl = ImGui::GetWindowDrawList();
							ImVec2 cp = ImGui::GetCursorScreenPos();

							if (card.type == 1) {
								auto* cur_k = get_knife_by_def(Skins->knif_def[s_skins_active_team]);
								std::string title_s = cur_k ? cur_k->name : xs("Knife");
								std::string sub_s = xs("Default");
								int rarity = 6;
								CMisc::Panorama_t::IconData_t icon_data{};

								if (cur_k) {
									int p_idx = Skins->skin_data[s_skins_active_team][cur_k->def_index].paint;
									if (p_idx > 0 && static_cast<size_t>(p_idx - 1) < cur_k->paint_kits.size()) {
										auto& pk = cur_k->paint_kits[p_idx - 1];
										sub_s = pk.name;
										rarity = pk.rarity;
										icon_data = cur_k->GetImage(pk);
									} else {
										icon_data = cur_k->GetImage();
									}
								}

								ImGui::PushID(99001);
								if (ImGui::InvisibleButton(xs("##knife_slot_btn"), ImVec2(card_w, card_h))) {
									s_skins_prev_view = 0;
									s_skins_view = 2;
									s_skins_search[0] = '\0';
								}

								bool is_hov = ImGui::IsItemHovered();
								gdl->AddRectFilled(cp, ImVec2(cp.x + card_w, cp.y + card_h), is_hov ? c_card_bg_hov : c_card_bg, card_rounding);

								bool has_k_t = (Skins->knif_def[0] > 0);
								bool has_k_ct = (Skins->knif_def[1] > 0);
								if (has_k_t || has_k_ct)
									draw_team_outline(gdl, cp, ImVec2(cp.x + card_w, cp.y + card_h), has_k_t, has_k_ct, card_rounding);
								else
									gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), c_border, card_rounding);

								if (icon_data.texture_view) {
									DrawFittedImage(gdl, reinterpret_cast<ImTextureID>(icon_data.texture_view), ImVec2(cp.x + 10.f, cp.y + 8.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 44.f), GetIconAspect(icon_data));
								} else {
									ImVec2 ic_p(cp.x + card_w * 0.5f, cp.y + (card_h - 44.f) * 0.5f + 4.f);
									ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 1.6f, FLT_MAX, -1.f, ICON_FA_SHIELD_HALVED);
									gdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 1.6f, ic_p - isz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), ICON_FA_SHIELD_HALVED);
								}

								ImGui::PushFont(Var->font.inter[1]);
								Draw->TextClipped(gdl, Var->font.inter[1], ImVec2(cp.x + 10.f, cp.y + card_h - 44.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 27.f), c_text, title_s.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
								ImGui::PopFont();

								ImGui::PushFont(Var->font.inter[0]);
								Draw->TextClipped(gdl, Var->font.inter[0], ImVec2(cp.x + 10.f, cp.y + card_h - 27.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 13.f), c_accent, sub_s.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
								ImGui::PopFont();

								gdl->AddRectFilled(ImVec2(cp.x + 10.f, cp.y + card_h - 9.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 6.f), GetRarityU32(rarity), 2.f);
								ImGui::PopID();
							} else if (card.type == 2) {
								auto* cur_g = get_glove_by_def(Skins->glove_def[s_skins_active_team]);
								std::string title_s = cur_g ? cur_g->name : xs("Gloves");
								std::string sub_s = xs("Default");
								int rarity = 6;
								CMisc::Panorama_t::IconData_t icon_data{};

								if (cur_g) {
									int p_idx = Skins->skin_data[s_skins_active_team][cur_g->def_index].paint;
									if (p_idx > 0 && static_cast<size_t>(p_idx - 1) < cur_g->paint_kits.size()) {
										auto& pk = cur_g->paint_kits[p_idx - 1];
										sub_s = pk.name;
										rarity = pk.rarity;
										icon_data = cur_g->GetImage(pk);
									} else {
										icon_data = cur_g->GetImage();
									}
								}

								ImGui::PushID(99002);
								if (ImGui::InvisibleButton(xs("##glove_slot_btn"), ImVec2(card_w, card_h))) {
									s_skins_prev_view = 0;
									s_skins_view = 3;
									s_skins_search[0] = '\0';
								}

								bool is_hov = ImGui::IsItemHovered();
								gdl->AddRectFilled(cp, ImVec2(cp.x + card_w, cp.y + card_h), is_hov ? c_card_bg_hov : c_card_bg, card_rounding);

								bool has_g_t = (Skins->glove_def[0] > 0);
								bool has_g_ct = (Skins->glove_def[1] > 0);
								if (has_g_t || has_g_ct)
									draw_team_outline(gdl, cp, ImVec2(cp.x + card_w, cp.y + card_h), has_g_t, has_g_ct, card_rounding);
								else
									gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), c_border, card_rounding);

								if (icon_data.texture_view) {
									DrawFittedImage(gdl, reinterpret_cast<ImTextureID>(icon_data.texture_view), ImVec2(cp.x + 10.f, cp.y + 8.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 44.f), GetIconAspect(icon_data));
								} else {
									ImVec2 ic_p(cp.x + card_w * 0.5f, cp.y + (card_h - 44.f) * 0.5f + 4.f);
									ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 1.6f, FLT_MAX, -1.f, ICON_FA_HAND);
									gdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 1.6f, ic_p - isz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), ICON_FA_HAND);
								}

								ImGui::PushFont(Var->font.inter[1]);
								Draw->TextClipped(gdl, Var->font.inter[1], ImVec2(cp.x + 10.f, cp.y + card_h - 44.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 27.f), c_text, title_s.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
								ImGui::PopFont();

								ImGui::PushFont(Var->font.inter[0]);
								Draw->TextClipped(gdl, Var->font.inter[0], ImVec2(cp.x + 10.f, cp.y + card_h - 27.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 13.f), c_accent, sub_s.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
								ImGui::PopFont();

								gdl->AddRectFilled(ImVec2(cp.x + 10.f, cp.y + card_h - 9.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 6.f), GetRarityU32(rarity), 2.f);
								ImGui::PopID();
							} else {
								size_t i = card.weapon_index;
								auto& item = weapons[i];

								int p_idx = Skins->skin_data[s_skins_active_team][item.def_index].paint;
								std::string skin_name = xs("Default");
								int rarity = 0;
								CMisc::Panorama_t::IconData_t icon_data{};

								if (p_idx > 0 && static_cast<size_t>(p_idx - 1) < item.paint_kits.size()) {
									auto& pk = item.paint_kits[p_idx - 1];
									skin_name = pk.name;
									rarity = pk.rarity;
									icon_data = item.GetImage(pk);
								} else {
									icon_data = item.GetImage();
								}

								ImGui::PushID(static_cast<int>(i + 100));
								bool is_sel = (s_selected_weapon_def == item.def_index);

								if (ImGui::InvisibleButton(xs("##wpn_item"), ImVec2(card_w, card_h))) {
									s_selected_weapon_def = item.def_index;
									s_skins_prev_view = 0;
									s_skins_view = 1;
									s_skins_search[0] = '\0';
								}

								bool is_hov = ImGui::IsItemHovered();
								gdl->AddRectFilled(cp, ImVec2(cp.x + card_w, cp.y + card_h), is_hov ? c_card_bg_hov : c_card_bg, card_rounding);
								gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), c_border, card_rounding);

								bool has_wpn_t = (Skins->skin_data[0][item.def_index].paint > 0);
								bool has_wpn_ct = (Skins->skin_data[1][item.def_index].paint > 0);
								if (has_wpn_t || has_wpn_ct)
									draw_team_outline(gdl, cp, ImVec2(cp.x + card_w, cp.y + card_h), has_wpn_t, has_wpn_ct, card_rounding);

								if (icon_data.texture_view) {
									DrawFittedImage(gdl, reinterpret_cast<ImTextureID>(icon_data.texture_view), ImVec2(cp.x + 10.f, cp.y + 8.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 44.f), GetIconAspect(icon_data));
								} else {
									ImVec2 ic_p(cp.x + card_w * 0.5f, cp.y + (card_h - 44.f) * 0.5f + 4.f);
									ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 1.6f, FLT_MAX, -1.f, ICON_FA_GUN);
									gdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 1.6f, ic_p - isz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), ICON_FA_GUN);
								}

								ImGui::PushFont(Var->font.inter[1]);
								Draw->TextClipped(gdl, Var->font.inter[1], ImVec2(cp.x + 10.f, cp.y + card_h - 44.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 27.f), c_text, item.name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
								ImGui::PopFont();

								ImGui::PushFont(Var->font.inter[0]);
								Draw->TextClipped(gdl, Var->font.inter[0], ImVec2(cp.x + 10.f, cp.y + card_h - 27.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 13.f), c_text_dim, skin_name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
								ImGui::PopFont();

								gdl->AddRectFilled(ImVec2(cp.x + 10.f, cp.y + card_h - 9.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 6.f), GetRarityU32(rarity), 2.f);

								if (is_sel)
									gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), Draw->GetClr(Colors->accent), card_rounding, 0, 1.5f);

								ImGui::PopID();
							}

							if ((di % 2) == 0 && (di + 1 < display_cards.size()))
								ImGui::SameLine(0.f, card_gap);
						}
					} else if (s_skins_view == 1) {
						std::vector<int> filtered_kits;
						if (s_skins_search[0] == '\0')
							filtered_kits.push_back(0);

						for (size_t k = 0; k < sel_item.paint_kits.size(); ++k) {
							auto& pk = sel_item.paint_kits[k];
							if (s_skins_search[0] != '\0') {
								auto it = std::search(pk.name.begin(), pk.name.end(), s_skins_search, s_skins_search + strlen(s_skins_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
								if (it == pk.name.end())
									continue;
							}

							filtered_kits.push_back(static_cast<int>(k) + 1);
						}

						for (size_t ki = 0; ki < filtered_kits.size(); ++ki) {
							int pk_idx = filtered_kits[ki];
							bool is_sel = (s_pending_paint == pk_idx);

							bool is_eq_t = (Skins->skin_data[0][sel_item.def_index].paint == pk_idx);
							bool is_eq_ct = (Skins->skin_data[1][sel_item.def_index].paint == pk_idx);

							std::string skin_name = (pk_idx == 0) ? xs("Default") : sel_item.paint_kits[pk_idx - 1].name;
							int rarity = (pk_idx == 0) ? 0 : sel_item.paint_kits[pk_idx - 1].rarity;
							CMisc::Panorama_t::IconData_t icon_data = (pk_idx == 0) ? sel_item.GetImage() : sel_item.GetImage(sel_item.paint_kits[pk_idx - 1]);

							ImGui::PushID(pk_idx + 8000);
							ImVec2 cp = ImGui::GetCursorScreenPos();
							if (ImGui::InvisibleButton(xs("##skin_item"), ImVec2(card_w, card_h)))
								s_pending_paint = pk_idx;

							bool is_hov = ImGui::IsItemHovered();
							ImDrawList* gdl = ImGui::GetWindowDrawList();

							gdl->AddRectFilled(cp, ImVec2(cp.x + card_w, cp.y + card_h), is_hov ? c_card_bg_hov : c_card_bg, card_rounding);
							gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), c_border, card_rounding);

							if (is_eq_t || is_eq_ct)
								draw_team_outline(gdl, cp, ImVec2(cp.x + card_w, cp.y + card_h), is_eq_t, is_eq_ct, card_rounding);

							if (icon_data.texture_view) {
								DrawFittedImage(gdl, reinterpret_cast<ImTextureID>(icon_data.texture_view), ImVec2(cp.x + 10.f, cp.y + 8.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 44.f), GetIconAspect(icon_data));
							} else {
								ImVec2 ic_p(cp.x + card_w * 0.5f, cp.y + (card_h - 44.f) * 0.5f + 4.f);
								ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 1.6f, FLT_MAX, -1.f, ICON_FA_PAINTBRUSH);
								gdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 1.6f, ic_p - isz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), ICON_FA_PAINTBRUSH);
							}

							ImGui::PushFont(Var->font.inter[1]);
							Draw->TextClipped(gdl, Var->font.inter[1], ImVec2(cp.x + 10.f, cp.y + card_h - 44.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 27.f), c_text, sel_item.name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
							ImGui::PopFont();

							ImGui::PushFont(Var->font.inter[0]);
							Draw->TextClipped(gdl, Var->font.inter[0], ImVec2(cp.x + 10.f, cp.y + card_h - 27.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 13.f), (pk_idx == 0) ? c_text_dim : GetRarityU32(rarity), skin_name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
							ImGui::PopFont();

							gdl->AddRectFilled(ImVec2(cp.x + 10.f, cp.y + card_h - 9.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 6.f), GetRarityU32(rarity), 2.f);

							if (is_sel)
								gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), Draw->GetClr(Colors->accent), card_rounding, 0, 1.5f);

							if ((ki % 2) == 0 && (ki + 1 < filtered_kits.size()))
								ImGui::SameLine(0.f, card_gap);

							ImGui::PopID();
						}
					} else if (s_skins_view == 2) {
						std::vector<size_t> filtered_knives;
						for (size_t i = 0; i < knives.size(); ++i) {
							auto& item = knives[i];
							if (s_skins_search[0] != '\0') {
								auto it = std::search(item.name.begin(), item.name.end(), s_skins_search, s_skins_search + strlen(s_skins_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
								if (it == item.name.end())
									continue;
							}

							filtered_knives.push_back(i);
						}

						for (size_t di = 0; di < filtered_knives.size(); ++di) {
							size_t i = filtered_knives[di];
							auto& item = knives[i];
							bool is_eq_t = (Skins->knif_def[0] == item.def_index);
							bool is_eq_ct = (Skins->knif_def[1] == item.def_index);

							int p_idx = Skins->skin_data[s_skins_active_team][item.def_index].paint;
							std::string skin_name = xs("Default");
							CMisc::Panorama_t::IconData_t icon_data{};

							if (p_idx > 0 && static_cast<size_t>(p_idx - 1) < item.paint_kits.size()) {
								auto& pk = item.paint_kits[p_idx - 1];
								skin_name = pk.name;
								icon_data = item.GetImage(pk);
							} else {
								icon_data = item.GetImage();
							}

							ImGui::PushID(static_cast<int>(i + 3000));
							ImVec2 cp = ImGui::GetCursorScreenPos();
							if (ImGui::InvisibleButton(xs("##knife_pick_item"), ImVec2(card_w, card_h))) {
								s_selected_weapon_def = item.def_index;
								s_skins_prev_view = 2;
								s_skins_view = 1;
								s_skins_search[0] = '\0';

								auto purge_knives = [&](int team) {
									for (auto& k : knives) {
										if (k.def_index != item.def_index)
											Skins->skin_data[team].erase(k.def_index);
									}
								};

								if (s_skins_team_mode == 2 || s_skins_team_mode == 0) {
									Skins->knif_def[0] = item.def_index;
									purge_knives(0);
								}

								if (s_skins_team_mode == 2 || s_skins_team_mode == 1) {
									Skins->knif_def[1] = item.def_index;
									purge_knives(1);
								}

								s_pending_key = -1;
							}

							bool is_hov = ImGui::IsItemHovered();
							ImDrawList* gdl = ImGui::GetWindowDrawList();

							gdl->AddRectFilled(cp, ImVec2(cp.x + card_w, cp.y + card_h), is_hov ? c_card_bg_hov : c_card_bg, card_rounding);
							gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), c_border, card_rounding);

							if (is_eq_t || is_eq_ct)
								draw_team_outline(gdl, cp, ImVec2(cp.x + card_w, cp.y + card_h), is_eq_t, is_eq_ct, card_rounding);

							if (icon_data.texture_view) {
								DrawFittedImage(gdl, reinterpret_cast<ImTextureID>(icon_data.texture_view), ImVec2(cp.x + 10.f, cp.y + 8.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 44.f), GetIconAspect(icon_data));
							} else {
								ImVec2 ic_p(cp.x + card_w * 0.5f, cp.y + (card_h - 44.f) * 0.5f + 4.f);
								ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 1.6f, FLT_MAX, -1.f, ICON_FA_SHIELD_HALVED);
								gdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 1.6f, ic_p - isz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), ICON_FA_SHIELD_HALVED);
							}

							ImGui::PushFont(Var->font.inter[1]);
							Draw->TextClipped(gdl, Var->font.inter[1], ImVec2(cp.x + 10.f, cp.y + card_h - 44.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 27.f), c_text, item.name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
							ImGui::PopFont();

							ImGui::PushFont(Var->font.inter[0]);
							Draw->TextClipped(gdl, Var->font.inter[0], ImVec2(cp.x + 10.f, cp.y + card_h - 27.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 13.f), (is_eq_t || is_eq_ct) ? c_accent : c_text_dim, skin_name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
							ImGui::PopFont();

							gdl->AddRectFilled(ImVec2(cp.x + 10.f, cp.y + card_h - 9.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 6.f), GetRarityU32(6), 2.f);

							if ((di % 2) == 0 && (di + 1 < filtered_knives.size()))
								ImGui::SameLine(0.f, card_gap);

							ImGui::PopID();
						}
					} else if (s_skins_view == 3) {
						std::vector<size_t> filtered_gloves;
						for (size_t i = 0; i < gloves.size(); ++i) {
							auto& item = gloves[i];
							if (s_skins_search[0] != '\0') {
								auto it = std::search(item.name.begin(), item.name.end(), s_skins_search, s_skins_search + strlen(s_skins_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
								if (it == item.name.end())
									continue;
							}

							filtered_gloves.push_back(i);
						}

						for (size_t di = 0; di < filtered_gloves.size(); ++di) {
							size_t i = filtered_gloves[di];
							auto& item = gloves[i];
							bool is_eq_t = (Skins->glove_def[0] == item.def_index);
							bool is_eq_ct = (Skins->glove_def[1] == item.def_index);

							int p_idx = Skins->skin_data[s_skins_active_team][item.def_index].paint;
							std::string skin_name = xs("Default");
							CMisc::Panorama_t::IconData_t icon_data{};

							if (p_idx > 0 && static_cast<size_t>(p_idx - 1) < item.paint_kits.size()) {
								auto& pk = item.paint_kits[p_idx - 1];
								skin_name = pk.name;
								icon_data = item.GetImage(pk);
							} else {
								icon_data = item.GetImage();
							}

							ImGui::PushID(static_cast<int>(i + 5000));
							ImVec2 cp = ImGui::GetCursorScreenPos();
							if (ImGui::InvisibleButton(xs("##glove_pick_item"), ImVec2(card_w, card_h))) {
								s_selected_weapon_def = item.def_index;
								s_skins_prev_view = 3;
								s_skins_view = 1;
								s_skins_search[0] = '\0';

								auto purge_gloves = [&](int team) {
									for (auto& g : gloves) {
										if (g.def_index != item.def_index)
											Skins->skin_data[team].erase(g.def_index);
									}
								};

								if (s_skins_team_mode == 2 || s_skins_team_mode == 0) {
									Skins->glove_def[0] = item.def_index;
									purge_gloves(0);
								}

								if (s_skins_team_mode == 2 || s_skins_team_mode == 1) {
									Skins->glove_def[1] = item.def_index;
									purge_gloves(1);
								}

								s_pending_key = -1;
							}

							bool is_hov = ImGui::IsItemHovered();
							ImDrawList* gdl = ImGui::GetWindowDrawList();

							gdl->AddRectFilled(cp, ImVec2(cp.x + card_w, cp.y + card_h), is_hov ? c_card_bg_hov : c_card_bg, card_rounding);
							gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), c_border, card_rounding);

							if (is_eq_t || is_eq_ct)
								draw_team_outline(gdl, cp, ImVec2(cp.x + card_w, cp.y + card_h), is_eq_t, is_eq_ct, card_rounding);

							if (icon_data.texture_view) {
								DrawFittedImage(gdl, reinterpret_cast<ImTextureID>(icon_data.texture_view), ImVec2(cp.x + 10.f, cp.y + 8.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 44.f), GetIconAspect(icon_data));
							} else {
								ImVec2 ic_p(cp.x + card_w * 0.5f, cp.y + (card_h - 44.f) * 0.5f + 4.f);
								ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 1.6f, FLT_MAX, -1.f, ICON_FA_HAND);
								gdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 1.6f, ic_p - isz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), ICON_FA_HAND);
							}

							ImGui::PushFont(Var->font.inter[1]);
							Draw->TextClipped(gdl, Var->font.inter[1], ImVec2(cp.x + 10.f, cp.y + card_h - 44.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 27.f), c_text, item.name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
							ImGui::PopFont();

							ImGui::PushFont(Var->font.inter[0]);
							Draw->TextClipped(gdl, Var->font.inter[0], ImVec2(cp.x + 10.f, cp.y + card_h - 27.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 13.f), (is_eq_t || is_eq_ct) ? c_accent : c_text_dim, skin_name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
							ImGui::PopFont();

							gdl->AddRectFilled(ImVec2(cp.x + 10.f, cp.y + card_h - 9.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 6.f), GetRarityU32(6), 2.f);

							if ((di % 2) == 0 && (di + 1 < filtered_gloves.size()))
								ImGui::SameLine(0.f, card_gap);

							ImGui::PopID();
						}
					}
				}
				ImGui::Dummy(ImVec2(right_col_w, grid_h));
				ImGui::EndChild();

				const float save_btn_w = 130.f;
				const float in_name_w = right_col_w - save_btn_w - 10.f;
				const ImVec2 bot_bar_p(col_pos.x, footer_top_y);

				ImGui::SetCursorScreenPos(bot_bar_p);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, (footer_h - Var->font.inter[0]->LegacySize) * 0.5f));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, c_field_bg);
				ImGui::PushStyleColor(ImGuiCol_Border, c_border);
				ImGui::PushStyleColor(ImGuiCol_Text, c_text);
				ImGui::PushStyleColor(ImGuiCol_TextDisabled, c_text_dim);
				ImGui::SetNextItemWidth(in_name_w);
				ImGui::PushFont(Var->font.inter[0]);
				ImGui::InputTextWithHint(xs("##col_name_input"), xs("Collection name"), s_collection_name_input, sizeof(s_collection_name_input));
				ImGui::PopFont();
				ImGui::PopStyleColor(4);
				ImGui::PopStyleVar(3);

				const ImVec2 sb_min(bot_bar_p.x + in_name_w + 10.f, bot_bar_p.y);
				const ImVec2 sb_max(sb_min.x + save_btn_w, sb_min.y + footer_h);

				ImGui::SetCursorScreenPos(sb_min);
				ImGui::InvisibleButton(xs("##save_col_btn"), ImVec2(save_btn_w, footer_h));
				bool sb_hov = ImGui::IsItemHovered();

				if (ImGui::IsItemClicked() && s_collection_name_input[0] != '\0') {
					std::vector<CollectionItemEntry_t> non_default_items;

					for (auto& w : weapons) {
						int pt = Skins->skin_data[0][w.def_index].paint;
						int pct = Skins->skin_data[1][w.def_index].paint;
						if (pt > 0 || pct > 0) {
							CollectionItemEntry_t entry{};
							entry.def_index = w.def_index;
							entry.paint_t = pt;
							entry.wear_t = Skins->skin_data[0][w.def_index].wear;
							entry.seed_t = Skins->skin_data[0][w.def_index].seed;
							entry.paint_ct = pct;
							entry.wear_ct = Skins->skin_data[1][w.def_index].wear;
							entry.seed_ct = Skins->skin_data[1][w.def_index].seed;
							non_default_items.push_back(entry);
						}
					}

					auto add_single_slot = [&](uint16_t def_t, uint16_t def_ct) {
						if (def_t > 0 && def_t == def_ct) {
							CollectionItemEntry_t entry{};
							entry.def_index = def_t;
							entry.paint_t = Skins->skin_data[0][def_t].paint;
							entry.wear_t = Skins->skin_data[0][def_t].wear;
							entry.seed_t = Skins->skin_data[0][def_t].seed;
							entry.paint_ct = Skins->skin_data[1][def_t].paint;
							entry.wear_ct = Skins->skin_data[1][def_t].wear;
							entry.seed_ct = Skins->skin_data[1][def_t].seed;
							non_default_items.push_back(entry);
						} else {
							if (def_t > 0) {
								CollectionItemEntry_t entry{};
								entry.def_index = def_t;
								entry.paint_t = Skins->skin_data[0][def_t].paint;
								entry.wear_t = Skins->skin_data[0][def_t].wear;
								entry.seed_t = Skins->skin_data[0][def_t].seed;
								non_default_items.push_back(entry);
							}
							if (def_ct > 0 && def_ct != def_t) {
								CollectionItemEntry_t entry{};
								entry.def_index = def_ct;
								entry.paint_ct = Skins->skin_data[1][def_ct].paint;
								entry.wear_ct = Skins->skin_data[1][def_ct].wear;
								entry.seed_ct = Skins->skin_data[1][def_ct].seed;
								non_default_items.push_back(entry);
							}
						}
					};

					add_single_slot(Skins->knif_def[0], Skins->knif_def[1]);
					add_single_slot(Skins->glove_def[0], Skins->glove_def[1]);

					if (!non_default_items.empty() || Skins->agent_def[0] > 0 || Skins->agent_def[1] > 0) {
						CollectionsManager.Save(s_collection_name_input, non_default_items, Skins->agent_def[0], Skins->agent_def[1], false);
						s_collection_name_input[0] = '\0';
					}
				}

				dl->AddRectFilled(sb_min, sb_max, Draw->GetClr(Colors->accent, sb_hov ? 1.f : 0.9f), 6.f);

				ImVec2 b_isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize, FLT_MAX, -1.f, ICON_FA_BOOKMARK);
				ImVec2 b_tsz = Var->font.inter[1]->CalcTextSizeA(Var->font.inter[1]->LegacySize, FLT_MAX, -1.f, xs("Save"));
				float stw = b_isz.x + 6.f + b_tsz.x;

				ImVec2 sip(sb_min.x + (save_btn_w - stw) * 0.5f, sb_min.y + (footer_h - b_isz.y) * 0.5f);
				ImVec2 stp(sip.x + b_isz.x + 6.f, sb_min.y + (footer_h - b_tsz.y) * 0.5f);

				dl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize, sip, IM_COL32(15, 16, 20, 255), ICON_FA_BOOKMARK);
				dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, stp, IM_COL32(15, 16, 20, 255), xs("Save"));
			}
			ImGui::EndChild();
		} else if (s_skins_subtab == 1) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImVec2 top_pos = ImGui::GetCursorScreenPos();

			ImGui::PushFont(Var->font.inter[1]);
			dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, top_pos, c_text, xs("Saved collections"));
			ImGui::PopFont();

			const float filter_w = 140.f;
			const float filter_h = 24.f;
			const float seg_w = filter_w / 2.f;
			ImVec2 f_pos(top_pos.x + avail_w - filter_w, top_pos.y + 4.f);

			dl->AddRectFilled(f_pos, ImVec2(f_pos.x + filter_w, f_pos.y + filter_h), c_field_bg, 6.f);
			dl->AddRect(f_pos, ImVec2(f_pos.x + filter_w, f_pos.y + filter_h), c_border, 6.f);

			const char* f_titles[2] = { xs("All"), xs("Favourite") };
			for (int f = 0; f < 2; ++f) {
				ImVec2 f_min(f_pos.x + f * seg_w, f_pos.y);
				ImVec2 f_max(f_min.x + seg_w, f_min.y + filter_h);

				ImGui::SetCursorScreenPos(f_min);
				ImGui::PushID(f + 6000);

				if (ImGui::InvisibleButton(xs("##seg_btn"), ImVec2(seg_w, filter_h)))
					s_collections_filter = f;

				if (s_collections_filter == f)
					dl->AddRectFilled(f_min, f_max, Draw->GetClr(Colors->accent, 0.85f), 5.f);

				ImGui::PushFont(Var->font.inter[0]);
				ImVec2 tsz = Var->font.inter[0]->CalcTextSizeA(Var->font.inter[0]->LegacySize, FLT_MAX, -1.f, f_titles[f]);
				dl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, f_min + (ImVec2(seg_w, filter_h) - tsz) * 0.5f, (s_collections_filter == f) ? c_text : c_text_dim, f_titles[f]);
				ImGui::PopFont();

				ImGui::PopID();
			}

			const float header_h = 42.f;
			ImGui::SetCursorScreenPos(ImVec2(top_pos.x, top_pos.y + header_h));

			auto all_cols = CollectionsManager.GetAll();
			std::vector<SkinCollection_t> visible_cols;
			for (auto& col : all_cols) {
				if (s_collections_filter == 1 && !col.is_favourite)
					continue;

				visible_cols.push_back(col);
			}

			if (!visible_cols.empty()) {
				const float col_gap = 12.f;
				const float ccard_w = (avail_w - col_gap) * 0.5f;
				const float ccard_h = 220.f;

				ImGui::BeginChild(xs("##collections_grid_child"), ImVec2(avail_w, avail_h - header_h), false);
				{
					auto render_smooth_icon = [&](const char* id_str, const char* icon_text, ImVec2 center, ImU32 base_col, ImU32 hov_col, bool is_active = false) -> bool {
						const float sz = 22.f;
						ImVec2 bmin(center.x - sz * 0.5f, center.y - sz * 0.5f);
						ImVec2 bmax(center.x + sz * 0.5f, center.y + sz * 0.5f);

						ImGui::SetCursorScreenPos(bmin);
						ImGui::InvisibleButton(id_str, ImVec2(sz, sz));
						bool hovered = ImGui::IsItemHovered();
						bool clicked = ImGui::IsItemClicked();

						ImGuiStorage* storage = ImGui::GetStateStorage();
						ImGuiID anim_id = ImGui::GetID(id_str);
						float anim = storage->GetFloat(anim_id, 0.0f);
						anim = ImLerp(anim, (hovered || is_active) ? 1.0f : 0.0f, ImGui::GetIO().DeltaTime * 14.f);
						storage->SetFloat(anim_id, anim);

						ImVec4 c_start = ImGui::ColorConvertU32ToFloat4(base_col);
						c_start.w *= 0.45f;
						ImVec4 c_end = ImGui::ColorConvertU32ToFloat4(hov_col);
						c_end.w = 1.0f;

						ImVec4 c_interp = ImLerp(c_start, c_end, anim);
						ImU32 final_col = ImGui::ColorConvertFloat4ToU32(c_interp);

						ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize, FLT_MAX, -1.f, icon_text);
						dl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize, center - isz * 0.5f, final_col, icon_text);

						return clicked;
					};

					for (size_t vi = 0; vi < visible_cols.size(); ++vi) {
						auto& col = visible_cols[vi];
						ImGui::PushID(static_cast<int>(vi + 9100));
						ImVec2 cp = ImGui::GetCursorScreenPos();

						dl->AddRectFilled(cp, ImVec2(cp.x + ccard_w, cp.y + ccard_h), c_card_bg, card_rounding);
						dl->AddRect(cp, ImVec2(cp.x + ccard_w, cp.y + ccard_h), c_border, card_rounding, 0, 1.0f);

						ImGui::PushFont(Var->font.inter[1]);
						dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, ImVec2(cp.x + 14.f, cp.y + 12.f), c_text, col.name.c_str());
						ImGui::PopFont();

						char sub_str[64];
						snprintf(sub_str, sizeof(sub_str), xs("%zu items · %s"), col.items.size(), col.date_str.c_str());
						ImGui::PushFont(Var->font.inter[0]);
						dl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, ImVec2(cp.x + 14.f, cp.y + 28.f), c_text_dim, sub_str);
						ImGui::PopFont();

						float icon_y = cp.y + 22.f;
						float cur_rx = cp.x + ccard_w - 20.f;

						if (render_smooth_icon(xs("##del_btn"), ICON_FA_TRASH_CAN, ImVec2(cur_rx, icon_y), c_text_dim, IM_COL32(235, 75, 75, 255))) {
							CollectionsManager.Remove(col.name);
							ImGui::PopID();
							break;
						}

						cur_rx -= 24.f;

						ImU32 fav_base = col.is_favourite ? c_accent : c_text_dim;
						ImU32 fav_hov = col.is_favourite ? c_accent : c_text;
						if (render_smooth_icon(xs("##fav_btn"), ICON_FA_BOOKMARK, ImVec2(cur_rx, icon_y), fav_base, fav_hov, col.is_favourite)) {
							CollectionsManager.ToggleFavourite(col.name);
							col.is_favourite = !col.is_favourite;
						}

						cur_rx -= 24.f;

						if (render_smooth_icon(xs("##fld_show_btn"), ICON_FA_FOLDER_OPEN, ImVec2(cur_rx, icon_y), c_text_dim, c_text))
							CollectionsManager.ShowInExplorer(col.name);

						const float slot_gap = 6.f;
						const float slot_area_w = ccard_w - 28.f;
						const float slot_w = (slot_area_w - slot_gap * 2.f) / 3.f;
						const float slot_h = 50.f;
						float slot_start_y = cp.y + 54.f;

						for (int row = 0; row < 2; ++row) {
							for (int c = 0; c < 3; ++c) {
								int slot_idx = row * 3 + c;
								ImVec2 sp(cp.x + 14.f + c * (slot_w + slot_gap), slot_start_y + row * (slot_h + slot_gap));

								dl->AddRectFilled(sp, ImVec2(sp.x + slot_w, sp.y + slot_h), c_field_bg, 5.f);
								dl->AddRect(sp, ImVec2(sp.x + slot_w, sp.y + slot_h), c_border, 5.f);

								if (slot_idx < static_cast<int>(col.items.size())) {
									auto& ci = col.items[slot_idx];
									auto check_slot_icon = [&](std::vector<CSkins::Item_t>& container) -> bool {
										for (auto& w : container) {
											if (w.def_index == ci.def_index) {
												int preview_paint = (ci.paint_t > 0) ? ci.paint_t : ci.paint_ct;
												CMisc::Panorama_t::IconData_t s_icon = (preview_paint > 0 && static_cast<size_t>(preview_paint - 1) < w.paint_kits.size()) ? w.GetImage(w.paint_kits[preview_paint - 1]) : w.GetImage();
												if (s_icon.texture_view)
													DrawFittedImage(dl, reinterpret_cast<ImTextureID>(s_icon.texture_view), ImVec2(sp.x + 4.f, sp.y + 4.f), ImVec2(sp.x + slot_w - 4.f, sp.y + slot_h - 4.f), GetIconAspect(s_icon));

												return true;
											}
										}

										return false;
									};

									if (!check_slot_icon(weapons)) {
										if (!check_slot_icon(knives)) {
											if (!check_slot_icon(gloves))
												check_slot_icon(agents);
										}
									}
								}
							}
						}

						const float eq_btn_h = 30.f;
						ImVec2 eq_min(cp.x + 14.f, cp.y + ccard_h - 14.f - eq_btn_h);
						ImVec2 eq_max(cp.x + ccard_w - 14.f, eq_min.y + eq_btn_h);

						ImGui::SetCursorScreenPos(eq_min);
						if (ImGui::InvisibleButton(xs("##equip_col_btn"), ImVec2(ccard_w - 28.f, eq_btn_h))) {
							for (auto& ci : col.items) {
								bool is_k = false;
								for (auto& k : knives) {
									if (k.def_index == ci.def_index) {
										Skins->knif_def[0] = k.def_index;
										Skins->knif_def[1] = k.def_index;
										is_k = true;
										break;
									}
								}

								if (!is_k) {
									for (auto& g : gloves) {
										if (g.def_index == ci.def_index) {
											Skins->glove_def[0] = g.def_index;
											Skins->glove_def[1] = g.def_index;
											break;
										}
									}
								}

								Skins->skin_data[0][ci.def_index] = { ci.paint_t, ci.wear_t, ci.seed_t };
								Skins->skin_data[1][ci.def_index] = { ci.paint_ct, ci.wear_ct, ci.seed_ct };
							}

							Skins->agent_def[0] = col.agent_t;
							Skins->agent_def[1] = col.agent_ct;

							Config::Set<int>(Vars::Skins::AgentT, static_cast<int>(col.agent_t));
							Config::Set<int>(Vars::Skins::AgentCT, static_cast<int>(col.agent_ct));
							Config::Set<int>(Vars::Skins::Agent, static_cast<int>(col.agent_t));

							s_pending_key = -1;
							applied_paint_kits.clear();
							Skins->should_update = true;
						}

						bool eq_hov = ImGui::IsItemHovered();
						dl->AddRectFilled(eq_min, eq_max, eq_hov ? c_card_bg_hov : c_field_bg, 6.f);
						dl->AddRect(eq_min, eq_max, c_border, 6.f);

						const char* eq_label = xs("Equip collection");
						ImVec2 ar_sz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize, FLT_MAX, -1.f, ICON_FA_ARROW_RIGHT);
						ImVec2 eq_tsz = Var->font.inter[0]->CalcTextSizeA(Var->font.inter[0]->LegacySize, FLT_MAX, -1.f, eq_label);
						float etw = ar_sz.x + 6.f + eq_tsz.x;

						ImVec2 a_ip(eq_min.x + ((ccard_w - 28.f) - etw) * 0.5f, eq_min.y + (eq_btn_h - ar_sz.y) * 0.5f);
						ImVec2 a_tp(a_ip.x + ar_sz.x + 6.f, eq_min.y + (eq_btn_h - eq_tsz.y) * 0.5f);

						dl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize, a_ip, c_accent, ICON_FA_ARROW_RIGHT);
						ImGui::PushFont(Var->font.inter[0]);
						dl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, a_tp, c_text, eq_label);
						ImGui::PopFont();

						ImGui::SetCursorScreenPos(cp);
						ImGui::Dummy(ImVec2(ccard_w, ccard_h));

						if ((vi % 2) == 0 && (vi + 1 < visible_cols.size()))
							ImGui::SameLine(0.f, col_gap);
						else
							ImGui::Dummy(ImVec2(0.f, col_gap));

						ImGui::PopID();
					}
				}
				ImGui::EndChild();
			}
		} else if (s_skins_subtab == 2) {
			auto& sel_agent = *s_agent_selected_item;

			const float left_col_w = ImClamp(avail_w * 0.34f, 260.f, 340.f);
			const float col_gap = 20.f;
			const float right_col_w = avail_w - left_col_w - col_gap;

			const ImVec2 outer_pos = ImGui::GetCursorScreenPos();

			ImGui::SetCursorScreenPos(outer_pos);
			ImGui::BeginChild(xs("##agent_loadout_col"), ImVec2(left_col_w, avail_h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				ImDrawList* dl = ImGui::GetWindowDrawList();
				const ImVec2 col_pos = ImGui::GetWindowPos();

				ImGui::PushFont(Var->font.inter[1]);
				dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, col_pos, c_text, xs("Loadout"));
				ImGui::PopFont();

				const float header_h = 36.f;
				const float footer_btn_h = 42.f;
				const float scroll_h = ImMax(avail_h - header_h - footer_btn_h - 14.f, 150.f);

				ImGui::SetCursorScreenPos(ImVec2(col_pos.x, col_pos.y + header_h));
				ImGui::BeginChild(xs("##agent_preview_scroll"), ImVec2(left_col_w, scroll_h), false);
				{
					ImDrawList* sdl = ImGui::GetWindowDrawList();
					const float prev_h = 360.f;
					ImVec2 pp = ImGui::GetCursorScreenPos();

					ImVec2 f_center(pp.x + left_col_w * 0.5f, pp.y + prev_h * 0.5f);
					const char* fallback_ico = (sel_agent.def_index == 0) ? ICON_FA_USER_SLASH : ICON_FA_USER;
					ImVec2 f_sz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 2.5f, FLT_MAX, -1.f, fallback_ico);
					sdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 2.5f, f_center - f_sz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), fallback_ico);

					ImVec2 name_pos(pp.x, pp.y + prev_h + 10.f);
					ImGui::PushFont(Var->font.inter[1]);
					sdl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, name_pos, c_text, sel_agent.name.c_str());
					ImGui::PopFont();

					int agent_team = get_agent_team(sel_agent);
					const char* team_str = (agent_team == 1) ? xs("Counter-Terrorist") : ((agent_team == 0) ? xs("Terrorist") : xs("Default Model"));
					ImU32 team_color = (agent_team == 1) ? c_team_ct : ((agent_team == 0) ? c_team_t : c_text_dim);

					ImVec2 team_pos(name_pos.x, name_pos.y + Var->font.inter[1]->LegacySize + 6.f);
					ImGui::PushFont(Var->font.inter[0]);
					sdl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, team_pos, team_color, team_str);
					ImGui::PopFont();

					ImGui::SetCursorScreenPos(ImVec2(team_pos.x, team_pos.y + Var->font.inter[0]->LegacySize + 16.f));

					{
						ImVec2 row_p = ImGui::GetCursorScreenPos();
						const float pill_w = 150.f;
						const float pill_h = 24.f;
						const float item_w = pill_w / 3.f;
						ImVec2 pill_pos(row_p.x + left_col_w - pill_w, row_p.y);

						ImGui::PushFont(Var->font.inter[0]);
						sdl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, ImVec2(row_p.x, row_p.y + (pill_h - Var->font.inter[0]->LegacySize) * 0.5f), c_text_dim, xs("Team"));
						ImGui::PopFont();

						sdl->AddRectFilled(pill_pos, ImVec2(pill_pos.x + pill_w, pill_pos.y + pill_h), c_field_bg, 6.f);

						const char* team_opts[3] = { xs("T"), xs("CT"), xs("Both") };
						for (int team = 0; team < 3; ++team) {
							ImVec2 opt_min(pill_pos.x + team * item_w, pill_pos.y);
							ImVec2 opt_max(opt_min.x + item_w, opt_min.y + pill_h);

							ImGui::SetCursorScreenPos(opt_min);
							ImGui::PushID(team + 700);
							if (ImGui::InvisibleButton(xs("##agent_team_btn"), ImVec2(item_w, pill_h))) {
								s_agent_team_mode = team;
								if (team == 0)
									s_agent_active_team = 0;
								else if (team == 1)
									s_agent_active_team = 1;
							}

							if (s_agent_team_mode == team) {
								ImU32 act_col = (team == 0) ? c_team_t : ((team == 1) ? c_team_ct : Draw->GetClr(Colors->accent, 0.85f));
								sdl->AddRectFilled(opt_min, opt_max, act_col, 6.f);
							}

							ImGui::PushFont(Var->font.inter[0]);
							ImVec2 tsz = Var->font.inter[0]->CalcTextSizeA(Var->font.inter[0]->LegacySize, FLT_MAX, -1.f, team_opts[team]);
							sdl->AddText(Var->font.inter[0], Var->font.inter[0]->LegacySize, opt_min + (ImVec2(item_w, pill_h) - tsz) * 0.5f, (s_agent_team_mode == team) ? IM_COL32(15, 16, 20, 255) : c_text_dim, team_opts[team]);
							ImGui::PopFont();
							ImGui::PopID();
						}

						ImGui::SetCursorScreenPos(ImVec2(row_p.x, row_p.y + pill_h + 12.f));
					}
				}
				ImGui::Dummy(ImVec2(left_col_w, scroll_h));
				ImGui::EndChild();

				ImVec2 btn_min(col_pos.x, col_pos.y + avail_h - footer_btn_h);
				ImVec2 btn_max(btn_min.x + left_col_w, btn_min.y + footer_btn_h);

				ImGui::SetCursorScreenPos(btn_min);
				ImGui::InvisibleButton(xs("##agent_apply_btn"), ImVec2(left_col_w, footer_btn_h));
				bool b_hov = ImGui::IsItemHovered();
				if (ImGui::IsItemClicked()) {
					if (s_agent_team_mode == 2 || s_agent_team_mode == 0) {
						Skins->agent_def[0] = sel_agent.def_index;
						Config::Set<int>(Vars::Skins::AgentT, static_cast<int>(sel_agent.def_index));
					}

					if (s_agent_team_mode == 2 || s_agent_team_mode == 1) {
						Skins->agent_def[1] = sel_agent.def_index;
						Config::Set<int>(Vars::Skins::AgentCT, static_cast<int>(sel_agent.def_index));
					}

					Config::Set<int>(Vars::Skins::Agent, static_cast<int>(Skins->agent_def[s_agent_active_team]));
					Skins->should_update = true;
				}

				dl->AddRectFilled(btn_min, btn_max, Draw->GetClr(Colors->accent, b_hov ? 1.f : 0.9f), 7.f);

				const char* b_label = (sel_agent.def_index == 0) ? xs("Reset to default") : xs("Apply to loadout");
				const char* b_ico = (sel_agent.def_index == 0) ? ICON_FA_ROTATE_LEFT : ICON_FA_CHECK;
				ImVec2 icon_sz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize, FLT_MAX, -1.f, b_ico);
				ImVec2 txt_sz = Var->font.inter[1]->CalcTextSizeA(Var->font.inter[1]->LegacySize, FLT_MAX, -1.f, b_label);
				float full_tw = icon_sz.x + 8.f + txt_sz.x;

				ImVec2 icon_pos(btn_min.x + (left_col_w - full_tw) * 0.5f, btn_min.y + (footer_btn_h - icon_sz.y) * 0.5f);
				ImVec2 txt_pos(icon_pos.x + icon_sz.x + 8.f, btn_min.y + (footer_btn_h - txt_sz.y) * 0.5f);

				dl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize, icon_pos, IM_COL32(15, 16, 20, 255), b_ico);
				dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, txt_pos, IM_COL32(15, 16, 20, 255), b_label);
			}
			ImGui::EndChild();

			ImGui::SetCursorScreenPos(ImVec2(outer_pos.x + left_col_w + col_gap, outer_pos.y));
			ImGui::BeginChild(xs("##agent_right_pane"), ImVec2(right_col_w, avail_h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				ImDrawList* dl = ImGui::GetWindowDrawList();
				const ImVec2 col_pos = ImGui::GetWindowPos();
				const float title_h = ImMax(Var->font.inter[1]->LegacySize, Var->font.inter[0]->LegacySize);

				ImGui::PushFont(Var->font.inter[1]);
				dl->AddText(Var->font.inter[1], Var->font.inter[1]->LegacySize, col_pos, c_text, xs("Agents"));
				ImGui::PopFont();

				ImGui::SetCursorScreenPos(ImVec2(col_pos.x, col_pos.y + title_h + 14.f));
				const float toolbar_h = 32.f;
				const float dd_w = 140.f;
				const float s_w = right_col_w - dd_w - 8.f;
				DrawCustomSearch(xs("##search_agents"), xs("Search agents"), s_agent_search, sizeof(s_agent_search), s_w, toolbar_h, Var->font.inter[0], Var->font.fa[1], c_field_bg, c_border, c_text, c_text_dim);

				ImGui::SameLine(0.f, 8.f);
				ImGui::SetNextItemWidth(dd_w);
				this->Dropdown(xs("##agent_cat_dd"), &s_agent_category, { xs("All agents"), xs("Terrorists"), xs("Counter-Terrorists") });

				const float header_bottom_y = ImGui::GetCursorScreenPos().y;
				const float grid_gap = 14.f;
				const float grid_top_y = header_bottom_y + grid_gap;
				const float grid_h = ImMax(col_pos.y + avail_h - grid_top_y, 100.f);
				const float card_gap = 10.f;
				const float card_w = (right_col_w - card_gap) * 0.5f;
				const float card_h = card_w * 0.78f;

				ImGui::SetCursorScreenPos(ImVec2(col_pos.x, grid_top_y));
				ImGui::BeginChild(xs("##agent_grid_scroll"), ImVec2(right_col_w, grid_h), false);
				{
					std::vector<CSkins::Item_t*> filtered_agents;

					bool match_default = false;
					if (s_agent_category == 0) {
						if (s_agent_search[0] == '\0') {
							match_default = true;
						} else {
							std::string def_name = s_default_agent.name;
							auto it = std::search(def_name.begin(), def_name.end(), s_agent_search, s_agent_search + strlen(s_agent_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
							if (it != def_name.end())
								match_default = true;
						}
					}

					if (match_default)
						filtered_agents.push_back(&s_default_agent);

					for (size_t i = 0; i < agents.size(); ++i) {
						auto& item = agents[i];
						int a_team = get_agent_team(item);
						if (s_agent_category == 1 && a_team != 0)
							continue;

						if (s_agent_category == 2 && a_team != 1)
							continue;

						if (s_agent_search[0] != '\0') {
							auto it = std::search(item.name.begin(), item.name.end(), s_agent_search, s_agent_search + strlen(s_agent_search), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
							if (it == item.name.end())
								continue;
						}

						filtered_agents.push_back(&item);
					}

					for (size_t di = 0; di < filtered_agents.size(); ++di) {
						auto* item_ptr = filtered_agents[di];
						auto& item = *item_ptr;

						bool is_eq_t = (Skins->agent_def[0] == item.def_index);
						bool is_eq_ct = (Skins->agent_def[1] == item.def_index);
						bool is_sel = (s_selected_agent_def == item.def_index);

						CMisc::Panorama_t::IconData_t icon_data = (item.def_index == 0) ? CMisc::Panorama_t::IconData_t{} : item.GetImage();
						int a_team = get_agent_team(item);
						const char* sub_s = (a_team == 1) ? xs("Counter-Terrorist") : ((a_team == 0) ? xs("Terrorist") : xs("Default Model"));

						ImGui::PushID(static_cast<int>(di + 12000));
						ImVec2 cp = ImGui::GetCursorScreenPos();
						if (ImGui::InvisibleButton(xs("##agent_item_btn"), ImVec2(card_w, card_h)))
							s_selected_agent_def = item.def_index;

						bool is_hov = ImGui::IsItemHovered();
						ImDrawList* gdl = ImGui::GetWindowDrawList();

						gdl->AddRectFilled(cp, ImVec2(cp.x + card_w, cp.y + card_h), is_hov ? c_card_bg_hov : c_card_bg, card_rounding);
						gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), c_border, card_rounding);

						if (is_eq_t || is_eq_ct)
							draw_team_outline(gdl, cp, ImVec2(cp.x + card_w, cp.y + card_h), is_eq_t, is_eq_ct, card_rounding);

						if (icon_data.texture_view) {
							DrawFittedImage(gdl, reinterpret_cast<ImTextureID>(icon_data.texture_view), ImVec2(cp.x + 10.f, cp.y + 8.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 44.f), GetIconAspect(icon_data));
						} else {
							ImVec2 ic_p(cp.x + card_w * 0.5f, cp.y + (card_h - 44.f) * 0.5f + 4.f);
							const char* fallback_ico = (item.def_index == 0) ? ICON_FA_USER_SLASH : ICON_FA_USER;
							ImVec2 isz = Var->font.fa[1]->CalcTextSizeA(Var->font.fa[1]->LegacySize * 1.6f, FLT_MAX, -1.f, fallback_ico);
							gdl->AddText(Var->font.fa[1], Var->font.fa[1]->LegacySize * 1.6f, ic_p - isz * 0.5f, Draw->GetClr(Colors->accent, 0.45f), fallback_ico);
						}

						ImGui::PushFont(Var->font.inter[1]);
						Draw->TextClipped(gdl, Var->font.inter[1], ImVec2(cp.x + 10.f, cp.y + card_h - 44.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 27.f), c_text, item.name.c_str(), nullptr, nullptr, ImVec2(0.f, 0.5f));
						ImGui::PopFont();

						ImGui::PushFont(Var->font.inter[0]);
						Draw->TextClipped(gdl, Var->font.inter[0], ImVec2(cp.x + 10.f, cp.y + card_h - 27.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 13.f), (is_eq_t || is_eq_ct) ? c_accent : c_text_dim, sub_s, nullptr, nullptr, ImVec2(0.f, 0.5f));
						ImGui::PopFont();

						gdl->AddRectFilled(ImVec2(cp.x + 10.f, cp.y + card_h - 9.f), ImVec2(cp.x + card_w - 10.f, cp.y + card_h - 6.f), GetRarityU32(item.rarity), 2.f);

						if (is_sel)
							gdl->AddRect(cp, ImVec2(cp.x + card_w, cp.y + card_h), Draw->GetClr(Colors->accent), card_rounding, 0, 1.5f);

						if ((di % 2) == 0 && (di + 1 < filtered_agents.size()))
							ImGui::SameLine(0.f, card_gap);

						ImGui::PopID();
					}
				}
				ImGui::Dummy(ImVec2(right_col_w, grid_h));
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}
	}
	ImGui::Dummy(ImVec2(root_w, root_h));
	ImGui::EndChild();
	ImGui::PopStyleVar();
}