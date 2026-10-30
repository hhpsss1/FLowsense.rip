#include <shared.h>

void CCSGOInput::SetWeaponSelect(CUserCommand* cmd) {
	const uint32_t weapon_handle = static_cast<uint32_t>(this->current_move_data.target_handle);
	if (weapon_handle == 0xFFFFFFFF || weapon_handle == 0xFFFFFFFE || weapon_handle == 0)
		return;

	auto weapon = EntitySystem->GetBaseEntity(weapon_handle & 0x7FFF).As<C_BaseEntity*>();
	if (!weapon)
		return;

	const int entity_index = weapon->GetIndex();
	if (entity_index == -1)
		return;

	auto base_cmd = cmd->pb.mutable_base();
	if (base_cmd)
		base_cmd->set_weaponselect(entity_index);
}

bool CFrameData::ParseInputMessage(CSGOInputHistoryEntryPB* entry, bool interps_allowed, float first, float second, C_CSPlayerPawn* player) {
	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ?? 55 57 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 8B 01 48 8B F9")).As<void (__fastcall*)(CFrameData*, CSGOInputHistoryEntryPB*, bool, float, float, C_CSPlayerPawn*)>();
	if (!fn || !entry)
		return false;

	fn(this, entry, interps_allowed, first, second, player);
	return true;
}
