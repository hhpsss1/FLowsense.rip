#include <shared.h>

void CSkins::OnFrameStage() {
	if (!ctx.pawn || !ctx.controller || !ctx.cmd || !ctx.weapon_data || !ctx.pawn->Alive() || !GlobalVars)
		return;

	if (GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
		return;

	if (!EngineClient->InGame())
		return;

	if (!ctx.pawn->m_pEntity() || !ctx.pawn->m_pEntity()->IsSafeToModify() || !ctx.pawn->m_pCollision())
		return;

	if (auto team = ctx.pawn->m_iTeamNum(); team != 2 && team != 3)
		return;

	auto* item_view = &ctx.pawn->m_EconGloves();
	if (!item_view || !item_view->m_NetworkedDynamicAttributes())
		return;

	const auto weapons = ctx.pawn->m_pWeaponServices()->m_hMyWeapons();
	if (weapons.size <= 0 || !weapons.elements)
		return;

	this->ProcessAgent();
	this->ProcessGloves();
	this->ProcessKnife();
	this->ProcessWeapons();

	this->should_update = false;
}

void CSkins::ProcessAgent() {
	static uintptr_t s_tracked_pawn = 0;
	static float s_last_spawn_time = -1.f;
	static std::string s_default_models[2]{ "", "" };
	static std::string s_applied_model = "";

	uintptr_t cur_pawn_ptr = reinterpret_cast<uintptr_t>(ctx.pawn);
	float spawn_time = ctx.pawn->m_flLastSpawnTimeIndex();
	int team_idx = (ctx.pawn->m_iTeamNum() == 2) ? 0 : 1;

	if (s_tracked_pawn != cur_pawn_ptr || s_last_spawn_time != spawn_time) {
		s_tracked_pawn = cur_pawn_ptr;
		s_last_spawn_time = spawn_time;
		s_applied_model = "";

		auto* scene = ctx.pawn->m_pGameSceneNode();
		if (scene) {
			auto* skeleton = scene->GetSkeletonInstance();
			if (skeleton) {
				auto* model = skeleton->GetModel();
				if (model && model->permodel_data.name && model->permodel_data.name[0] != '\0') {
					std::string_view name(model->permodel_data.name);
					if (name.find(xs("customplayer")) == std::string_view::npos)
						s_default_models[team_idx] = model->permodel_data.name;
				}
			}
		}
	}

	uint16_t selected_def = this->agent_def[team_idx];

	if (selected_def == 0) {
		const auto agent_var = Config::Get<int>(ctx.pawn->m_iTeamNum() == 2 ? Vars::Skins::AgentT : Vars::Skins::AgentCT);
		if (agent_var > 0)
			selected_def = static_cast<uint16_t>(agent_var);
		else if (const auto def_var = Config::Get<int>(Vars::Skins::Agent); def_var > 0)
			selected_def = static_cast<uint16_t>(def_var);
	}

	if (selected_def == 0) {
		if (!s_default_models[team_idx].empty() && s_applied_model != s_default_models[team_idx]) {
			auto mins = ctx.pawn->m_pCollision()->m_vecMins();
			auto maxs = ctx.pawn->m_pCollision()->m_vecMaxs();

			ctx.pawn->SetModel(s_default_models[team_idx].c_str());

			ctx.pawn->m_pCollision()->m_vecMins() = mins;
			ctx.pawn->m_pCollision()->m_vecMaxs() = maxs;

			s_applied_model = s_default_models[team_idx];
		}

		return;
	}

	const Item_t* selected_item = nullptr;
	for (const auto& item : this->agent_items) {
		if (item.def_index == selected_def) {
			selected_item = &item;
			break;
		}
	}

	if (!selected_item || selected_item->model_path.empty())
		return;

	if (s_applied_model != selected_item->model_path) {
		auto mins = ctx.pawn->m_pCollision()->m_vecMins();
		auto maxs = ctx.pawn->m_pCollision()->m_vecMaxs();

		ctx.pawn->SetModel(selected_item->model_path.c_str());

		ctx.pawn->m_pCollision()->m_vecMins() = mins;
		ctx.pawn->m_pCollision()->m_vecMaxs() = maxs;

		s_applied_model = selected_item->model_path;
	}
}

struct GloveState_t {
	uintptr_t tracked_pawn = 0;
	float last_spawn_time = -1.0f;
	int last_team = 0;
	bool overridden = false;
	uint16_t original_def = 0;
	uint64_t original_item_id = 0;
	uint32_t original_id_high = 0;
	uint32_t original_id_low = 0;
	uint32_t original_account_id = 0;
	bool original_restore_custom_material = false;
	bool original_init = false;
	bool original_disallow_soc = false;
	bool captured = false;
	uint16_t applied_def = 0;
	int applied_paint = 0;
	int applied_seed = 0;
	float applied_wear = 0.0f;
	int update_ticks = 0;
};

static GloveState_t GloveState;

static void SetPawnBodyGroup(C_CSPlayerPawn* pawn, int group, uint32_t value) {
	static auto set_bodygroup_fn = Memory->Modules.Client.Find(xs("85 D2 0F 88 ? ? ? ? 55 56 57")).As<void(__fastcall*)(C_BaseEntity*, int, uint32_t)>();
	if (set_bodygroup_fn && pawn)
		set_bodygroup_fn(pawn, group, value);
}

static void RefreshPawnGloves(C_CSPlayerPawn* pawn, C_EconItemView* item_view, bool is_custom) {
	static auto invalidate_fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D B9 ? ? ? ? 48 8B F1")).As<void(__fastcall*)(C_EconItemView*)>();
	if (item_view && invalidate_fn)
		invalidate_fn(item_view);

	pawn->m_bNeedToReApplyGloves() = true;

	SetPawnBodyGroup(pawn, 0, is_custom ? 1u : 0u);
}

void CSkins::ProcessGloves() {
	auto* item_view = &ctx.pawn->m_EconGloves();
	uintptr_t pawn_ptr = reinterpret_cast<uintptr_t>(ctx.pawn);
	float spawn_time = ctx.pawn->m_flLastSpawnTimeIndex();
	int team_idx = (ctx.pawn->m_iTeamNum() == 3) ? 1 : 0;

	if (GloveState.tracked_pawn != pawn_ptr || GloveState.last_spawn_time != spawn_time || GloveState.last_team != team_idx) {
		GloveState = {};
		GloveState.tracked_pawn = pawn_ptr;
		GloveState.last_spawn_time = spawn_time;
		GloveState.last_team = team_idx;
	}

	uint16_t target_glove_def = this->glove_def[team_idx];
	if (target_glove_def == 0) {
		target_glove_def = static_cast<uint16_t>(Config::Get<int>((team_idx == 0) ? Vars::Skins::GloveT : Vars::Skins::GloveCT));
		if (target_glove_def == 0)
			target_glove_def = static_cast<uint16_t>(Config::Get<int>(Vars::Skins::Glove));
	}

	const Item_t* target_glove_item = nullptr;
	if (target_glove_def > 0) {
		for (const auto& item : this->gloves_items) {
			if (item.def_index == target_glove_def) {
				target_glove_item = &item;
				break;
			}
		}
	}

	if (!target_glove_item) {
		if (GloveState.overridden && GloveState.captured) {
			item_view->m_iItemDefinitionIndex() = GloveState.original_def;
			item_view->m_iItemID() = GloveState.original_item_id;
			item_view->m_iItemIDHigh() = GloveState.original_id_high;
			item_view->m_iItemIDLow() = GloveState.original_id_low;
			item_view->m_iAccountID() = GloveState.original_account_id;
			item_view->m_bRestoreCustomMaterialAfterPrecache() = GloveState.original_restore_custom_material;
			item_view->m_bInitialized() = GloveState.original_init;
			item_view->m_bDisallowSOC() = GloveState.original_disallow_soc;

			RefreshPawnGloves(ctx.pawn, item_view, false);

			GloveState = {};
			GloveState.tracked_pawn = pawn_ptr;
			GloveState.last_spawn_time = spawn_time;
			GloveState.last_team = team_idx;
		}

		return;
	}

	if (!GloveState.captured) {
		GloveState.original_def = item_view->m_iItemDefinitionIndex();
		GloveState.original_item_id = item_view->m_iItemID();
		GloveState.original_id_high = item_view->m_iItemIDHigh();
		GloveState.original_id_low = item_view->m_iItemIDLow();
		GloveState.original_account_id = item_view->m_iAccountID();
		GloveState.original_restore_custom_material = item_view->m_bRestoreCustomMaterialAfterPrecache();
		GloveState.original_init = item_view->m_bInitialized();
		GloveState.original_disallow_soc = item_view->m_bDisallowSOC();
		GloveState.captured = true;
	}

	int target_paint_kit_id = 0;
	float target_wear = 0.01f;
	int target_seed = 1;

	int pk_idx = 0;
	auto it = this->skin_data[team_idx].find(target_glove_def);
	if (it != this->skin_data[team_idx].end()) {
		pk_idx = it->second.paint;
		target_wear = it->second.wear;
		target_seed = it->second.seed;
	}

	if (pk_idx > 0 && static_cast<size_t>(pk_idx - 1) < target_glove_item->paint_kits.size())
		target_paint_kit_id = target_glove_item->paint_kits[pk_idx - 1].id;

	const bool needs_apply = !GloveState.overridden || GloveState.applied_def != target_glove_def || GloveState.applied_paint != target_paint_kit_id || GloveState.applied_seed != target_seed || GloveState.applied_wear != target_wear || this->should_update;

	if (needs_apply) {
		const auto steam_id = ctx.controller->m_steamID();
		const auto account_id = static_cast<uint32_t>(steam_id & 0xffffffff);
		const uint64_t fake_id = 0xf000000000000000ull | (static_cast<uint64_t>(target_glove_def) << 16) | static_cast<uint64_t>(target_paint_kit_id & 0xFFFF);

		item_view->m_iItemDefinitionIndex() = target_glove_def;
		item_view->m_iItemID() = fake_id;
		item_view->m_iItemIDHigh() = static_cast<uint32_t>(fake_id >> 32);
		item_view->m_iItemIDLow() = static_cast<uint32_t>(fake_id & 0xffffffff);
		item_view->m_iAccountID() = account_id;
		item_view->m_bRestoreCustomMaterialAfterPrecache() = true;
		item_view->m_bDisallowSOC() = false;
		item_view->m_bInitialized() = true;

		item_view->SetAttributeByName(xs("set item texture prefab"), static_cast<float>(target_paint_kit_id));
		item_view->SetAttributeByName(xs("set item texture seed"), static_cast<float>(target_seed));
		item_view->SetAttributeByName(xs("set item texture wear"), target_wear);

		GloveState.overridden = true;
		GloveState.applied_def = target_glove_def;
		GloveState.applied_paint = target_paint_kit_id;
		GloveState.applied_seed = target_seed;
		GloveState.applied_wear = target_wear;
		GloveState.update_ticks = 4;
	}

	if (GloveState.overridden)
		SetPawnBodyGroup(ctx.pawn, 0, 1u);

	if (GloveState.update_ticks > 0) {
		item_view->m_bInitialized() = true;
		RefreshPawnGloves(ctx.pawn, item_view, true);
		GloveState.update_ticks--;
	}
}

static std::unordered_map<std::string, CMisc::Panorama_t::IconData_t> images;

static bool ReadVpkString(const uint8_t*& cursor, const uint8_t* end, std::string_view& value) {
	if (cursor >= end)
		return false;

	const auto* begin = cursor;

	while (cursor < end && *cursor)
		++cursor;

	if (cursor >= end)
		return false;

	value = std::string_view{
		reinterpret_cast<const char*>(begin),
		static_cast<size_t>(cursor - begin)
	};

	++cursor;

	return true;
}

bool CSkins::ParseEconImages() {
	const auto vpk_content = FileSystem->ReadRaw(xs("pak01_dir.vpk"));
	if (vpk_content.size() < sizeof(CVpkHeader))
		return false;

	const auto* raw = vpk_content.data();
	const auto* header = reinterpret_cast<const CVpkHeader*>(raw);

	if (header->msignature != 0x55AA1234)
		return false;

	if (header->version != 2)
		return false;

	constexpr std::string_view target_directory = "panorama/images/econ/default_generated";
	constexpr size_t header_size = sizeof(CVpkHeader);

	if (header->tree_size > vpk_content.size() - header_size)
		return false;

	const auto* ptr = raw + header_size;
	const auto* tree_end = ptr + header->tree_size;

	this->econ_images.clear();
	this->econ_images.reserve(4096);

	while (ptr < tree_end) {
		std::string_view extension;

		if (!ReadVpkString(ptr, tree_end, extension))
			return false;

		if (extension.empty())
			break;

		while (ptr < tree_end) {
			std::string_view directory;

			if (!ReadVpkString(ptr, tree_end, directory))
				return false;

			if (directory.empty())
				break;

			const bool wanted_directory = directory == target_directory;

			while (ptr < tree_end) {
				std::string_view filename;

				if (!ReadVpkString(ptr, tree_end, filename))
					return false;

				if (filename.empty())
					break;

				if (static_cast<size_t>(tree_end - ptr) < sizeof(CVpkEntry))
					return false;

				const auto* entry = reinterpret_cast<const CVpkEntry*>(ptr);

				ptr += sizeof(CVpkEntry);

				if (entry->terminator != 0xFFFF)
					return false;

				if (entry->preload_bytes > static_cast<size_t>(tree_end - ptr))
					return false;

				if (wanted_directory)
					this->econ_images.emplace(filename);

				ptr += entry->preload_bytes;
			}
		}
	}

	return true;
}

int CSkins::GetItemCategory(const Item_t& item) {
	if (item.type == ITEM_TYPE_KNIFE)
		return ITEM_CAT_KNIVES;

	if (item.type == ITEM_TYPE_GLOVE)
		return ITEM_CAT_GLOVES;

	return GetWeaponCategory(item.def_index);
}

const CSkins::Item_t* CSkins::FindItem(uint16_t defindex) {
	for (const auto& item : this->all_items) {
		if (item.def_index == defindex)
			return &item;
	}

	return nullptr;
}

void CSkins::Init() {
	const auto source2 = Source2Client;
	if (!source2)
		return;

	const auto econ_item_system = source2->GetEconItemSystem();
	if (!econ_item_system)
		return;

	this->Clear();

	//if (!this->ParseEconImages()) {
		//Log("Failed to parse econ images");
		//return;
	//}

	const auto econ_item_schema = econ_item_system->schema;
	const auto& econ_item_definition_map = econ_item_schema->GetItemDefinitionMap();
	const auto& econ_paint_kits_map = econ_item_schema->GetPaintKitsMap();

	static const std::string base_icon_path = xs("s2r://panorama/images/econ/default_generated/");
	static const std::string base_default_icon_path = xs("s2r://panorama/images/econ/weapons/base_weapons/");

	this->weapon_items.clear();
	this->knife_items.clear();
	this->gloves_items.clear();
	this->agent_items.clear();

	for (const auto& it : econ_item_definition_map) {
		auto* item_definition = it.value;
		if (!item_definition)
			continue;

		const char* base_name = item_definition->BaseName();
		if (!base_name || base_name[0] == '\0')
			continue;

		const bool is_weapon = item_definition->IsWeapon();
		const bool is_knife = item_definition->IsKnife();
		const bool is_gloves = item_definition->IsGloves();
		const bool is_agent = item_definition->IsAgent();

		if (!is_weapon && !is_knife && !is_gloves && !is_agent)
			continue;

		const char* name = Localize->FindSafe(base_name);
		std::string item_name = item_definition->ItemName();

		Item_t item{};

		item.def_index = item_definition->DefinitionIndex();
		item.rarity = item_definition->Rarity();
		item.name = (name && name[0] != '\0' && name[0] != '#') ? name : base_name;

		const char* mdl = item_definition->ModelName();
		if (mdl && mdl[0] != '\0')
			item.model_path = mdl;

		if (is_weapon)
			item.type = ITEM_TYPE_WEAPON;
		else if (is_knife)
			item.type = ITEM_TYPE_KNIFE;
		else if (is_gloves)
			item.type = ITEM_TYPE_GLOVE;
		else if (is_agent)
			item.type = ITEM_TYPE_AGENT;

		if (is_weapon)
			item.icon_path = base_default_icon_path + item_name + xs("_png.vtex");

		if (is_agent && strstr(base_name, xs("map_based")))
			continue;

		if (is_agent) {
			std::string aname = base_name + 19;
			std::string icon_path = xs("s2r://panorama/images/econ/characters/customplayer_") + aname + xs("_png.vtex");

			item.icon_path = icon_path;
			this->agent_items.emplace_back(item);
			continue;
		}

		for (const auto& kit_it : econ_paint_kits_map) {
			auto* paint_kit = kit_it.value;

			if (!paint_kit || paint_kit->id == 0 || paint_kit->id == 9001)
				continue;

			if (strstr(paint_kit->name, xs("doppler_phase2")) || strstr(paint_kit->name, xs("doppler_phase3")) || strstr(paint_kit->name, xs("doppler_phase4")))
				continue;

			std::string icon_name = item_name + xs("_") + paint_kit->name + xs("_light_png");

			if (this->econ_images.contains(icon_name)) {
				ItemPaintKit_t item_paint_kit{};

				item_paint_kit.id = paint_kit->id;
				item_paint_kit.rarity = paint_kit->rarity;
				item_paint_kit.is_legacy_model = paint_kit->IsLegacyModel();

				if (paint_kit->description_tag && paint_kit->description_tag[0] != '\0') {
					const char* loc_name = Localize->FindSafe(paint_kit->description_tag);

					if (loc_name && loc_name[0] != '\0' && loc_name[0] != '#')
						item_paint_kit.name = loc_name;
					else if (paint_kit->name && paint_kit->name[0] != '\0')
						item_paint_kit.name = paint_kit->name;
					else
						item_paint_kit.name = xs("Unknown");
				} else if (paint_kit->name && paint_kit->name[0] != '\0') {
					item_paint_kit.name = paint_kit->name;
				} else {
					item_paint_kit.name = xs("Default");
				}

				std::string icon_path = base_icon_path + icon_name + xs(".vtex");
				item_paint_kit.icon_path = icon_path;

				item.paint_kits.emplace_back(item_paint_kit);
			}
		}

		if (is_weapon) {
			this->weapon_items.emplace_back(item);
		} else if (is_knife) {
			if (item.def_index != 42 && item.def_index != 59) {
				if (!item.paint_kits.empty())
					item.icon_path = item.paint_kits.front().icon_path;
				else
					item.icon_path = base_default_icon_path + item_name + xs("_png.vtex");

				this->knife_items.emplace_back(item);
			}
		} else if (is_gloves) {
			if (!item.paint_kits.empty())
				item.icon_path = item.paint_kits.front().icon_path;
			else
				item.icon_path = base_default_icon_path + item_name + xs("_png.vtex");

			this->gloves_items.emplace_back(item);
		}
	}

	this->all_items.clear();
	this->all_items.insert(this->all_items.end(), this->weapon_items.begin(), this->weapon_items.end());
	this->all_items.insert(this->all_items.end(), this->knife_items.begin(), this->knife_items.end());
	this->all_items.insert(this->all_items.end(), this->gloves_items.begin(), this->gloves_items.end());
	this->all_items.insert(this->all_items.end(), this->agent_items.begin(), this->agent_items.end());
}

void CSkins::Clear() {
	for (auto& [path, data] : images) {
		if (data.texture_view)
			reinterpret_cast<IUnknown*>(data.texture_view)->Release();
	}

	images.clear();
}

CMisc::Panorama_t::IconData_t CSkins::Item_t::GetImage(ItemPaintKit_t kit) {
	std::string path = (kit.id == 0) ? this->icon_path : kit.icon_path;

	if (path.empty() && !this->paint_kits.empty())
		path = this->paint_kits.front().icon_path;

	if (path.empty())
		return {};

	const auto image = images.find(path);
	if (image != images.end())
		return image->second;

	CMisc::Panorama_t::IconData_t output{};
	if (Misc->Panorama.LoadVtex(path, &output)) {
		if (output.texture_view) {
			reinterpret_cast<IUnknown*>(output.texture_view)->AddRef();
			images[path] = output;
		}

		return output;
	}

	return {};
}

namespace detail {
	static inline uint32_t Murmurhash2Lower(const char* str, int len, uint32_t seed) {
		constexpr auto m = 0x5bd1e995;
		constexpr auto r = 24;

		auto h = seed ^ len;
		auto i = 0;

		while (len >= 4) {
			auto k =
				static_cast<uint32_t>((str[i] >= 'A' && str[i] <= 'Z') ? str[i] + 32 : str[i]) |
				(static_cast<uint32_t>((str[i + 1] >= 'A' && str[i + 1] <= 'Z') ? str[i + 1] + 32 : str[i + 1]) << 8) |
				(static_cast<uint32_t>((str[i + 2] >= 'A' && str[i + 2] <= 'Z') ? str[i + 2] + 32 : str[i + 2]) << 16) |
				(static_cast<uint32_t>((str[i + 3] >= 'A' && str[i + 3] <= 'Z') ? str[i + 3] + 32 : str[i + 3]) << 24);

			k *= m;
			k ^= k >> r;
			k *= m;

			h *= m;
			h ^= k;

			i += 4;
			len -= 4;
		}

		switch (len) {
		case 3: h ^= static_cast<uint32_t>((str[i + 2] >= 'A' && str[i + 2] <= 'Z') ? str[i + 2] + 32 : str[i + 2]) << 16; [[fallthrough]];
		case 2: h ^= static_cast<uint32_t>((str[i + 1] >= 'A' && str[i + 1] <= 'Z') ? str[i + 1] + 32 : str[i + 1]) << 8; [[fallthrough]];
		case 1: h ^= static_cast<uint32_t>((str[i] >= 'A' && str[i] <= 'Z') ? str[i] + 32 : str[i]); h *= m;
		}

		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;

		return h;
	}

	static inline uint32_t MakeSubclassToken(int16_t def_index) {
		const auto s = std::to_string(def_index);
		return Murmurhash2Lower(s.c_str(), static_cast<int>(s.length()), 0x31415926);
	}
}

struct KnifeState_t {
	uintptr_t tracked_pawn = 0;
	uint32_t tracked_weapon_handle = 0;
	float last_spawn_time = -1.0f;
	uint32_t last_active_handle = 0;
	bool overridden = false;
	uint16_t original_def = 0;
	uint64_t original_item_id = 0;
	uint32_t original_id_high = 0;
	uint32_t original_id_low = 0;
	uint32_t original_account_id = 0;
	bool original_init = false;
	int original_paint_kit = 0;
	int original_seed = 0;
	float original_wear = 0.0f;
	int original_stattrak = -1;
	bool captured = false;
	uint16_t applied_def = 0;
	int applied_paint = -1;
	float applied_wear = -1.f;
	int applied_seed = -1;
};

static KnifeState_t KnifeState;

static bool IsKnifeItem(uint16_t defindex) {
	if (IsKnifeDefindex(defindex))
		return true;

	for (const auto& item : Skins->knife_items) {
		if (item.def_index == defindex)
			return true;
	}

	return false;
}

static void ClearKnifeHud(C_EconItemView* item_view) {
	static auto invalidate_fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D B9 ? ? ? ? 48 8B F1")).As<void(__fastcall*)(C_EconItemView*)>();
	if (item_view && invalidate_fn)
		invalidate_fn(item_view);
}

static void UpdateKnifeModel(C_CSWeaponBase* weapon, C_EconItemView* item_view, C_CSPlayerPawn* pawn, uint16_t defindex, const char* custom_model, uint64_t mesh_mask) {
	const auto token = detail::MakeSubclassToken(static_cast<int16_t>(defindex));
	weapon->m_nSubclassID() = token;

	static auto get_viewmodel_fn = Memory->Modules.Client.Find(xs("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 83 BB 88 03 00 00 00")).As<void(__fastcall*)(C_CSWeaponBase*)>();
	if (get_viewmodel_fn)
		get_viewmodel_fn(weapon);

	const char* model_path = custom_model;
	if (!model_path || model_path[0] == '\0') {
		static auto get_model_path_fn = Memory->Modules.Client.Find(xs("48 89 5C 24 10 56 48 83 EC 20 48 8B 1D ? ? ? ?")).As<const char*(__fastcall*)(C_EconItemView*)>();
		if (get_model_path_fn)
			model_path = get_model_path_fn(item_view);
	}

	if (model_path && model_path[0] != '\0') {
		weapon->SetModel(model_path);

		auto hud_weapon = Skins->GetHudWeapon(pawn, weapon);
		if (hud_weapon) {
			hud_weapon->SetModel(model_path);
			if (auto scene_node = hud_weapon->m_pGameSceneNode())
				scene_node->SetMeshGroupMask(mesh_mask);
		}
	}
}

static void RebuildKnifePaint(C_CSWeaponBase* weapon, C_CSPlayerPawn* pawn, uint64_t mesh_mask) {
	if (auto scene_node = weapon->m_pGameSceneNode())
		scene_node->SetMeshGroupMask(mesh_mask);

	auto hud_weapon = Skins->GetHudWeapon(pawn, weapon);
	if (hud_weapon) {
		if (auto hud_node = hud_weapon->m_pGameSceneNode())
			hud_node->SetMeshGroupMask(mesh_mask);
	}

	weapon->UpdateCompositeMaterial();
	weapon->PostDataUpdate(1);
	weapon->UpdatePaintKit();
}

void CSkins::ProcessKnife() {
	if (!EngineClient->InGame() || !ctx.pawn->Alive())
		return;

	const auto weapons = ctx.pawn->m_pWeaponServices()->m_hMyWeapons();
	const auto pawn_ptr = reinterpret_cast<uintptr_t>(ctx.pawn);
	const float spawn_time = ctx.pawn->m_flLastSpawnTimeIndex();

	if (KnifeState.tracked_pawn != pawn_ptr || KnifeState.last_spawn_time != spawn_time) {
		KnifeState = {};
		KnifeState.tracked_pawn = pawn_ptr;
		KnifeState.last_spawn_time = spawn_time;
	}

	const int team_idx = (ctx.pawn->m_iTeamNum() == 3) ? 1 : 0;
	uint16_t target_knife_def = this->knif_def[team_idx];

	const Item_t* target_knife_item = nullptr;
	if (target_knife_def > 0) {
		for (const auto& item : this->knife_items) {
			if (item.def_index == target_knife_def) {
				target_knife_item = &item;
				break;
			}
		}
	}

	int target_paint_kit_id = 0;
	bool is_legacy = false;
	float target_wear = 0.01f;
	int target_seed = 1;

	if (target_knife_item) {
		int pk_idx = 0;
		auto it = this->skin_data[team_idx].find(target_knife_def);
		if (it != this->skin_data[team_idx].end()) {
			pk_idx = it->second.paint;
			target_wear = it->second.wear;
			target_seed = it->second.seed;
		}

		if (pk_idx > 0 && static_cast<size_t>(pk_idx - 1) < target_knife_item->paint_kits.size()) {
			auto& pk = target_knife_item->paint_kits[pk_idx - 1];
			target_paint_kit_id = pk.id;
			is_legacy = pk.is_legacy_model;
		}
	}

	const uint64_t knife_mesh_mask = (target_paint_kit_id > 0) ? (is_legacy ? 2 : 1) : 1;

	auto active_handle = ctx.pawn->m_pWeaponServices()->m_hActiveWeapon();
	const uint32_t active_int = active_handle.ToInt();
	const bool active_changed = (active_int != KnifeState.last_active_handle);
	KnifeState.last_active_handle = active_int;

	bool is_knife_active = false;
	if (active_handle.Valid()) {
		if (auto* cur_wep = active_handle.get<C_CSWeaponBase>()) {
			if (auto* cur_iv = cur_wep->m_AttributeManager()->m_Item())
				is_knife_active = IsKnifeItem(cur_iv->m_iItemDefinitionIndex());
		}
	}

	for (int i = 0; i < weapons.size; ++i) {
		auto handle = weapons.elements[i];
		if (!handle.Valid())
			continue;

		auto* weapon = handle.get<C_CSWeaponBase>();
		if (!weapon)
			continue;

		auto* weapon_identity = weapon->m_pEntity();
		if (!weapon_identity || !weapon_identity->IsSafeToModify())
			continue;

		const auto attribute_manager = weapon->m_AttributeManager();
		if (!attribute_manager)
			continue;

		const auto item_view = attribute_manager->m_Item();
		if (!item_view)
			continue;

		const auto current_def = item_view->m_iItemDefinitionIndex();
		if (!IsKnifeItem(current_def))
			continue;

		const bool is_this_active = is_knife_active && (handle.GetEntryIndex() == active_handle.GetEntryIndex());

		if (!target_knife_item) {
			if (KnifeState.overridden && KnifeState.captured) {
				item_view->m_iItemDefinitionIndex() = KnifeState.original_def;
				item_view->m_iItemID() = KnifeState.original_item_id;
				item_view->m_iItemIDHigh() = KnifeState.original_id_high;
				item_view->m_iItemIDLow() = KnifeState.original_id_low;
				item_view->m_iAccountID() = KnifeState.original_account_id;
				item_view->m_bInitialized() = KnifeState.original_init;

				weapon->m_nFallbackPaintKit() = KnifeState.original_paint_kit;
				weapon->m_nFallbackSeed() = KnifeState.original_seed;
				weapon->m_flFallbackWear() = KnifeState.original_wear;
				weapon->m_nFallbackStatTrak() = KnifeState.original_stattrak;

				item_view->SetAttributeByName(xs("set item texture prefab"), static_cast<float>(KnifeState.original_paint_kit));
				item_view->SetAttributeByName(xs("set item texture seed"), static_cast<float>(KnifeState.original_seed));
				item_view->SetAttributeByName(xs("set item texture wear"), KnifeState.original_wear);

				UpdateKnifeModel(weapon, item_view, ctx.pawn, KnifeState.original_def, nullptr, 1);
				RebuildKnifePaint(weapon, ctx.pawn, 1);
				ClearKnifeHud(item_view);

				KnifeState.overridden = false;
				KnifeState.applied_def = 0;
				KnifeState.applied_paint = -1;
			}

			break;
		}

		const uint32_t weapon_handle_uint = handle.ToInt();
		if (KnifeState.tracked_weapon_handle != weapon_handle_uint) {
			KnifeState = {};
			KnifeState.tracked_pawn = pawn_ptr;
			KnifeState.last_spawn_time = spawn_time;
			KnifeState.tracked_weapon_handle = weapon_handle_uint;
		}

		if (!KnifeState.overridden) {
			KnifeState.original_def = item_view->m_iItemDefinitionIndex();
			KnifeState.original_item_id = item_view->m_iItemID();
			KnifeState.original_id_high = item_view->m_iItemIDHigh();
			KnifeState.original_id_low = item_view->m_iItemIDLow();
			KnifeState.original_account_id = item_view->m_iAccountID();
			KnifeState.original_init = item_view->m_bInitialized();
			KnifeState.original_paint_kit = weapon->m_nFallbackPaintKit();
			KnifeState.original_seed = weapon->m_nFallbackSeed();
			KnifeState.original_wear = weapon->m_flFallbackWear();
			KnifeState.original_stattrak = weapon->m_nFallbackStatTrak();
			KnifeState.captured = true;
		}

		const auto target_token = detail::MakeSubclassToken(static_cast<int16_t>(target_knife_def));
		const bool needs_full_update = (!KnifeState.overridden || this->should_update || KnifeState.applied_def != target_knife_def || KnifeState.applied_paint != target_paint_kit_id || KnifeState.applied_wear != target_wear || KnifeState.applied_seed != target_seed || weapon->m_nSubclassID() != target_token || item_view->m_iItemDefinitionIndex() != target_knife_def);

		const char* custom_mdl = target_knife_item->model_path.empty() ? nullptr : target_knife_item->model_path.c_str();

		if (needs_full_update) {
			const auto steam_id = ctx.controller->m_steamID();
			const auto account_id = static_cast<uint32_t>(steam_id & 0xffffffff);

			item_view->m_iItemDefinitionIndex() = target_knife_def;
			item_view->m_iItemID() = (0xFULL << 60) | target_knife_def;
			item_view->m_iItemIDHigh() = 0xf0000000;
			item_view->m_iItemIDLow() = 0x10;
			item_view->m_iAccountID() = account_id;
			item_view->m_bDisallowSOC() = false;
			item_view->m_bRestoreCustomMaterialAfterPrecache() = true;
			item_view->m_bInitialized() = true;
			item_view->NameDescriptionPtr() = 0;
			item_view->m_iEntityQuality() = 3;

			weapon->m_nFallbackPaintKit() = target_paint_kit_id;
			weapon->m_nFallbackSeed() = (target_paint_kit_id > 0) ? target_seed : 0;
			weapon->m_flFallbackWear() = (target_paint_kit_id > 0) ? target_wear : 0.0f;
			weapon->m_nFallbackStatTrak() = -1;

			if (target_paint_kit_id > 0) {
				item_view->SetAttributeByName(xs("set item texture prefab"), static_cast<float>(target_paint_kit_id));
				item_view->SetAttributeByName(xs("set item texture seed"), static_cast<float>(target_seed));
				item_view->SetAttributeByName(xs("set item texture wear"), target_wear);
			} else {
				item_view->SetAttributeByName(xs("set item texture prefab"), 0.0f);
				item_view->SetAttributeByName(xs("set item texture seed"), 0.0f);
				item_view->SetAttributeByName(xs("set item texture wear"), 0.0f);
			}

			UpdateKnifeModel(weapon, item_view, ctx.pawn, target_knife_def, custom_mdl, knife_mesh_mask);
			RebuildKnifePaint(weapon, ctx.pawn, knife_mesh_mask);
			ClearKnifeHud(item_view);

			KnifeState.overridden = true;
			KnifeState.applied_def = target_knife_def;
			KnifeState.applied_paint = target_paint_kit_id;
			KnifeState.applied_wear = target_wear;
			KnifeState.applied_seed = target_seed;
		} else if (is_this_active && active_changed) {
			UpdateKnifeModel(weapon, item_view, ctx.pawn, target_knife_def, custom_mdl, knife_mesh_mask);
			RebuildKnifePaint(weapon, ctx.pawn, knife_mesh_mask);
			ClearKnifeHud(item_view);
		}

		break;
	}
}

C_BaseEntity* CSkins::GetHudWeapon(C_CSPlayerPawn* pawn, C_CSWeaponBase* weapon) {
	if (!pawn || !weapon)
		return nullptr;

	auto model_arms_handle = pawn->m_hHudModelArms();
	if (!model_arms_handle.Valid())
		return nullptr;

	auto model_arms = model_arms_handle.get<C_BaseEntity>();
	if (!model_arms)
		return nullptr;

	auto scene_node = model_arms->m_pGameSceneNode();
	if (!scene_node)
		return nullptr;

	const auto target_entry = weapon->GetHandle().GetEntryIndex();

	for (auto* child = scene_node->m_pChild(); child; child = child->m_pNextSibling()) {
		auto* owner = child->m_pOwner();
		if (!owner)
			continue;

		auto* entity = reinterpret_cast<C_BaseEntity*>(owner);
		auto owner_handle = entity->m_hOwnerEntity();
		if (!owner_handle.Valid())
			continue;

		if (owner_handle.GetEntryIndex() == target_entry)
			return entity;
	}

	return nullptr;
}

static void ClearWeaponHud(C_EconItemView* item_view) {
	static auto invalidate_fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D B9 ? ? ? ? 48 8B F1")).As<void(__fastcall*)(C_EconItemView*)>();
	if (item_view && invalidate_fn)
		invalidate_fn(item_view);
}

static void EngineUpdateWeaponData(C_CSWeaponBase* weapon) {
	if (!weapon)
		return;

	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 85 C0")).As<void(__fastcall*)(void*)>();
	if (fn)
		fn(weapon);
}

static void EngineRegenerateHudSkins() {
	static auto fn = Memory->Modules.Client.Find(xs("48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10")).As<void(__fastcall*)()>();
	if (fn)
		fn();
}

static bool IsWeaponSafeToModify(C_CSWeaponBase* weapon) {
	if (!weapon)
		return false;

	auto* identity = weapon->m_pEntity();
	if (!identity || !identity->IsSafeToModify())
		return false;

	const uintptr_t subclass_id_addr = reinterpret_cast<uintptr_t>(&weapon->m_nSubclassID());
	const uintptr_t subclass_vdata = *reinterpret_cast<uintptr_t*>(subclass_id_addr + 0x8);

	return subclass_vdata != 0;
}

static bool ApplySkin(C_CSPlayerPawn* pawn, CCSPlayerController* controller, C_CSWeaponBase* weapon, C_EconItemView* item_view, int defindex, int paint_kit_index, float wear, int seed, int stat_trak) {
	int target_paint_kit_id = 0;
	bool is_legacy = false;

	if (paint_kit_index > 0) {
		auto item_it = std::find_if(Skins->weapon_items.begin(), Skins->weapon_items.end(), [defindex](const CSkins::Item_t& item) { return item.def_index == defindex; });
		if (item_it != Skins->weapon_items.end()) {
			const auto& paint_kits = item_it->paint_kits;
			const size_t paint_kit_idx = static_cast<size_t>(paint_kit_index - 1);
			if (paint_kit_idx < paint_kits.size()) {
				target_paint_kit_id = paint_kits[paint_kit_idx].id;
				is_legacy = paint_kits[paint_kit_idx].is_legacy_model;
			}
		}
	}

	const bool is_custom = (target_paint_kit_id > 0);
	const float expected_wear = is_custom ? wear : 0.0f;
	const int expected_seed = is_custom ? seed : 0;

	const bool matches_target = (weapon->m_nFallbackPaintKit() == target_paint_kit_id) && (weapon->m_flFallbackWear() == expected_wear) && (weapon->m_nFallbackSeed() == expected_seed) && (item_view->m_iItemDefinitionIndex() == defindex);

	auto active_weapon_handle = pawn->m_pWeaponServices() ? pawn->m_pWeaponServices()->m_hActiveWeapon() : CBaseHandle{};
	const bool is_active = active_weapon_handle.Valid() && (active_weapon_handle.GetEntryIndex() == weapon->GetHandle().GetEntryIndex());

	uint64_t mesh_group = is_custom ? (is_legacy ? 2 : 1) : 1;
	if (defindex == 60 || defindex == 61)
		mesh_group |= 16;

	if (matches_target && !Skins->should_update) {
		if (is_active) {
			if (auto hud_weapon = Skins->GetHudWeapon(pawn, weapon)) {
				if (auto hud_scene_node = hud_weapon->m_pGameSceneNode())
					hud_scene_node->SetMeshGroupMask(mesh_group);
			}
		}

		return false;
	}

	if (!IsWeaponSafeToModify(weapon))
		return false;

	const auto steam_id = controller->m_steamID();
	const auto account_id = static_cast<uint32_t>(steam_id & 0xffffffff);
	const uint64_t item_id_val = (0xFULL << 60) | defindex;

	item_view->m_iItemDefinitionIndex() = defindex;
	item_view->m_iItemID() = item_id_val;
	item_view->m_iAccountID() = account_id;
	item_view->NameDescriptionPtr() = 0;
	item_view->m_bInitialized() = true;

	weapon->m_nFallbackPaintKit() = target_paint_kit_id;
	weapon->m_flFallbackWear() = expected_wear;
	weapon->m_nFallbackSeed() = expected_seed;
	weapon->m_nFallbackStatTrak() = is_custom ? stat_trak : -1;

	if (is_custom) {
		item_view->m_iItemIDHigh() = 0xf0000000;
		item_view->m_iItemIDLow() = 0x10;
		item_view->m_bDisallowSOC() = false;
		item_view->m_bRestoreCustomMaterialAfterPrecache() = true;

		item_view->SetAttributeByName(xs("set item texture prefab"), static_cast<float>(target_paint_kit_id));
		item_view->SetAttributeByName(xs("set item texture seed"), static_cast<float>(seed));
		item_view->SetAttributeByName(xs("set item texture wear"), wear);
	} else {
		item_view->m_iItemIDHigh() = static_cast<uint32_t>(item_id_val >> 32);
		item_view->m_iItemIDLow() = static_cast<uint32_t>(item_id_val & 0xFFFFFFFF);
		item_view->m_bDisallowSOC() = true;
		item_view->m_bRestoreCustomMaterialAfterPrecache() = false;

		item_view->SetAttributeByName(xs("set item texture prefab"), 0.0f);
		item_view->SetAttributeByName(xs("set item texture seed"), 0.0f);
		item_view->SetAttributeByName(xs("set item texture wear"), 0.0f);
	}

	if (auto scene_node = weapon->m_pGameSceneNode())
		scene_node->SetMeshGroupMask(mesh_group);

	if (is_active) {
		if (auto hud_weapon = Skins->GetHudWeapon(pawn, weapon)) {
			if (auto hud_scene_node = hud_weapon->m_pGameSceneNode())
				hud_scene_node->SetMeshGroupMask(mesh_group);
		}
	}

	weapon->UpdateCompositeMaterial();
	weapon->UpdatePaintKit();
	EngineUpdateWeaponData(weapon);
	ClearWeaponHud(item_view);

	return true;
}

void CSkins::ProcessWeapons() {
	const auto weapons = ctx.pawn->m_pWeaponServices()->m_hMyWeapons();
	const int team_idx = (ctx.pawn->m_iTeamNum() == 3) ? 1 : 0;
	bool updated_any = false;

	for (int i = 0; i < weapons.size; i++) {
		auto weapon_handle = weapons.elements[i];
		if (!weapon_handle.Valid())
			continue;

		auto* weapon = weapon_handle.get<C_CSWeaponBase>();
		if (!weapon)
			continue;

		auto* weapon_identity = weapon->m_pEntity();
		if (!weapon_identity || !weapon_identity->IsSafeToModify())
			continue;

		const auto attribute_manager = weapon->m_AttributeManager();
		if (!attribute_manager)
			continue;

		const auto item_view = attribute_manager->m_Item();
		if (!item_view)
			continue;

		const auto defindex = item_view->m_iItemDefinitionIndex();

		if (IsKnifeDefindex(defindex))
			continue;

		bool is_knife = false;
		for (const auto& k : this->knife_items) {
			if (k.def_index == defindex) {
				is_knife = true;
				break;
			}
		}

		if (is_knife)
			continue;

		if (!IsSkinnableWeaponDefindex(defindex))
			continue;

		int paint_kit_index = 0;
		float wear = 0.01f;
		int seed = 1;

		auto it = this->skin_data[team_idx].find(defindex);
		if (it != this->skin_data[team_idx].end()) {
			paint_kit_index = it->second.paint;
			wear = it->second.wear;
			seed = it->second.seed;
		}

		if (paint_kit_index <= 0)
			continue;

		if (ApplySkin(ctx.pawn, ctx.controller, weapon, item_view, defindex, paint_kit_index, wear, seed, -1))
			updated_any = true;
	}

	if (updated_any)
		EngineRegenerateHudSkins();
}