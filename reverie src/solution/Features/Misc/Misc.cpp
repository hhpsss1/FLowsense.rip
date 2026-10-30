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

void CMisc::OnPresent() {
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
