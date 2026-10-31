#include <shared.h>

CEntityList* EntityList = new CEntityList();

bool CachedPlayer_t::Fill() {
	pawn = nullptr;
	if (!controller || !EntitySystem)
		return false;

	auto current_controller = CBaseHandle(handle).get<CCSPlayerController>();
	if (!current_controller || current_controller != controller)
		return false;

	pawn = current_controller->m_hPawn().get<C_CSPlayerPawn>();
	return pawn != nullptr;
}

CachedPlayer_t* CEntityList::Find(C_BaseEntity* entity) {
	for (size_t i = 0; i < player_count; ++i) {
		if (players[i].Fill() && players[i].pawn == entity)
			return &players[i];
	}

	return nullptr;
}

void CEntityList::OnAdd(CEntityInstance* instance, CBaseHandle handle) {
	if (!instance || !handle.Valid())
		return;

	auto binding = instance->GetClassBindingBase();
	if (!binding || !binding->name)
		return;

	const int index = handle.GetEntryIndex();
	const int raw_handle = handle.ToInt();
	const auto hash = FnvHash(binding->name);
	auto entity = reinterpret_cast<C_BaseEntity*>(instance);

	if (hash == FnvHash("CCSPlayerController")) {
		for (size_t i = 0; i < player_count; ++i) {
			if (players[i].index != index)
				continue;

			players[i] = { reinterpret_cast<CCSPlayerController*>(instance), nullptr, index, raw_handle };
			return;
		}

		if (player_count < players.size())
			players[player_count++] = { reinterpret_cast<CCSPlayerController*>(instance), nullptr, index, raw_handle };
		return;
	}

	if (hash == FnvHash("C_HEGrenadeProjectile") || hash == FnvHash("C_FlashbangProjectile") || hash == FnvHash("C_SmokeGrenadeProjectile") || hash == FnvHash("C_DecoyProjectile") || hash == FnvHash("C_MolotovProjectile") || hash == FnvHash("C_Inferno")) {
		for (size_t i = 0; i < grenade_count; ++i) {
			if (grenades[i].handle == raw_handle)
				return;
		}

		if (grenade_count < grenades.size())
			grenades[grenade_count++] = { entity, raw_handle, hash };

		return;
	}

	if (hash == FnvHash("C_CSGameRulesProxy")) {
		game_rules_proxy = reinterpret_cast<C_CSGameRulesProxy*>(entity);
		return;
	}

	if (hash == FnvHash("C_PlantedC4")) {
		planted_c4 = reinterpret_cast<C_PlantedC4*>(entity);
		return;
	}

	if (!entity->IsWeapon())
		return;

	for (size_t i = 0; i < weapon_count; ++i) {
		if (weapons[i].handle == raw_handle)
			return;
	}

	if (weapon_count < weapons.size())
		weapons[weapon_count++] = { entity, raw_handle, hash };
}

void CEntityList::OnRemove(CEntityInstance* instance, CBaseHandle handle) {
	if (!handle.Valid())
		return;

	const int raw_handle = handle.ToInt();

	const auto binding = instance->GetClassBindingBase();
	if (!binding || !binding->name)
		return;

	const auto hash = FnvHash(binding->name);

	for (size_t i = 0; i < player_count; ++i) {
		if (players[i].handle != raw_handle)
			continue;

		players[i] = players[--player_count];
		players[player_count] = {};
		break;
	}

	for (size_t i = 0; i < grenade_count; ++i) {
		if (grenades[i].handle != raw_handle)
			continue;

		grenades[i] = grenades[--grenade_count];
		grenades[grenade_count] = {};
		break;
	}

	for (size_t i = 0; i < weapon_count; ++i) {
		if (weapons[i].handle != raw_handle)
			continue;

		weapons[i] = weapons[--weapon_count];
		weapons[weapon_count] = {};
		break;
	}

	if (hash == FnvHash("C_CSGameRulesProxy"))
		game_rules_proxy = nullptr;

	if (hash == FnvHash("C_PlantedC4"))
		planted_c4 = nullptr;
}

void CEntityList::ForceUpdate() {
	Clear();

	if (!EntitySystem)
		return;

	for (int i = 0; i <= 2048; ++i) {
		auto entity = EntitySystem->GetBaseEntity(i).As<C_BaseEntity*>();
		if (!entity || !entity->m_pEntity() || entity->m_pEntity()->Instance() != entity)
			continue;

		OnAdd(entity, entity->GetHandle());
	}
}

void CEntityList::Clear() {
	players.fill({});
	grenades.fill({});
	weapons.fill({});
	player_count = 0;
	grenade_count = 0;
	weapon_count = 0;
	planted_c4 = nullptr;
}
