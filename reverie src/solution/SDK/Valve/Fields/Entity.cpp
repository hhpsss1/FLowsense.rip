#include <shared.h>

bool CEntityIdentity::Valid() {
	return this->Index() != INVALID_EHANDLE_INDEX;
}

int CEntityIdentity::GetEntryIndex() {
	return this->Valid() ? this->Index() & ENT_ENTRY_MASK : ENT_ENTRY_MASK;
}

int CEntityIdentity::GetSerialNumber() {
	return this->Index() >> NUSERIAL_NUSHIFT_BITS;
}

bool CEntityIdentity::IsSafeToModify() {
	return (this->m_flags() & (0x2 | 0x4 | 0x8 | 0x10 | 0x200 | 0x400)) == 0;
}

CBaseHandle CEntityInstance::GetHandle() {
	auto identity = this->m_pEntity();
	return identity ? CBaseHandle(identity->GetEntryIndex(), identity->GetSerialNumber() - (identity->m_flags() & 1)) : CBaseHandle();
}

const CHitboxTransformRecord* CHitboxTransformHistory::GetLatest() const {
	if (first < 0 || first >= 4 || count <= 0 || count > 4)
		return nullptr;

	return &records[(first + count - 1) % 4];
}

bool C_BaseEntity::IsWeapon() {
	return Memory->Vtable.Execute<bool>(this, 165);
}

CSchemaClassInfo* CEntityInstance::GetClassBindingBase() {
	auto vtable = *reinterpret_cast<uintptr_t**>(this);
	if (!vtable || !vtable[47])
		return nullptr;

	CSchemaClassInfo* binding = nullptr;
	reinterpret_cast<void (__fastcall*)(CEntityInstance*, CSchemaClassInfo**)>(vtable[47])(this, &binding);
	return binding;
}

int C_BaseEntity::GetBoneIndex(const char* name) {
	static auto fn = Memory->Modules.Client.Find(xs("40 53 48 83 EC ? 48 8B 89 ? ? ? ? 48 8B DA 48 8B 01 FF 50 ? 48 8B C8")).As<int (__fastcall*)(void*, const char*)>();
	return fn && name ? fn(this, name) : -1;
}

bool C_CSWeaponBase::CanShoot() {
	return ctx.controller && m_iClip1() > 0 && this->m_nNextPrimaryAttackTick() <= ctx.controller->m_nTickBase();
}

bool C_CSWeaponBase::IsFiring() {
	auto data = this->GetWeaponData();
	if (!data)
		return false;

	if (C_BaseCSGrenade* nade = (C_BaseCSGrenade*)this; nade != nullptr && data->m_WeaponType() == WEAPONTYPE_GRENADE)
		return !nade->m_bPinPulled() && nade->m_fThrowTime() > 0.f && nade->m_fThrowTime() < TICKS_TO_TIME(ctx.controller->m_nTickBase());
	else if (data->m_WeaponType() == WEAPONTYPE_KNIFE)
		return (ctx.cmd->button_state.held & IN_ATTACK || ctx.cmd->button_state.held & IN_ATTACK2) && this->CanShoot();
	else if (data->m_bIsRevolver())
		return (ctx.cmd->button_state.held & IN_ATTACK && GlobalVars->current_time >= TICKS_TO_TIME(this->m_nPostponeFireReadyTicks()));
	else
		return (ctx.cmd->button_state.held & IN_ATTACK) && this->CanShoot();
}

float C_CSWeaponBase::GetSpread() {
	static auto fn = Memory->Modules.Client.Find(xs("48 63 91 ? ? ? ? 48 8B 81 ? ? ? ? 85 D2 78 ? 48 83 FA ? 73 ? F3 0F 10 84 90 ? ? ? ? C3 F3 0F 10 80 ? ? ? ? C3 CC CC CC CC CC 40 53")).As<float (__fastcall*)(void*)>();
	return fn ? fn(this) : -1.f;
}

float C_CSWeaponBase::GetInaccuracy() {
	float movement = 0.f;
	float air = 0.f;
	static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 55 56 57 48 81 EC B0 00 00 00 44 0F 29 84 24")).As<float (__fastcall*)(void*, float*, float*)>();
	return fn ? fn(this, &movement, &air) : -1.f;
}

bool C_CSWeaponBase::UpdateAccuracyPenalty() {
	static auto fn = Memory->Modules.Client.Find(xs("40 57 41 56 48 83 EC 68 48 8B F9 E8 ? ? ? ? 4C 8B F0 48 85 C0")).As<void (__fastcall*)(C_CSWeaponBase*)>();
	if (!fn)
		return false;

	fn(this);
	return true;
}

std::string C_CSWeaponBase::GetName() {
	auto wd = this->GetWeaponData();
	if (!wd)
		return {};

	const char* name = wd->m_szName();

	std::string full_name = name ? std::string(name) : std::string();

	if (full_name.size() >= 7 && full_name.substr(0, 7) == xs("weapon_"))
		full_name.erase(0, 7);

	std::transform(full_name.begin(), full_name.end(), full_name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	std::replace(full_name.begin(), full_name.end(), '_', ' ');

	return full_name;
}

void CAnimGraph2FloatParam::Set(float value) {
	if (!container || index < 0 || !std::isfinite(value))
		return;

	auto parameters = *reinterpret_cast<uintptr_t**>(container + 0x10);
	if (parameters && parameters[index])
		*reinterpret_cast<float*>(parameters[index] + 0x18) = value;
}

uintptr_t CSkeletonInstance::GetModelHandle() const {
	return *reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x1E0);
}

uintptr_t CSkeletonInstance::GetProcessedModelHandle() const {
	return *reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x200);
}

CModel* CSkeletonInstance::GetModel() const {
	const auto handle = GetModelHandle();
	if (!handle || handle != GetProcessedModelHandle())
		return nullptr;

	return *reinterpret_cast<CModel* const*>(handle);
}

bool CSkeletonInstance::GetHitboxList(std::vector<CHitboxDescriptor>& hitboxes, int count) {
	static auto get_set = Memory->Modules.Client.Find(xs("48 8B 81 E0 01 00 00 0F B6 91 FC 03 00 00 48 85 C0 74 08")).As<const char* (__fastcall*)(CSkeletonInstance*)>();
	static auto hash_name = Memory->Modules.Client.Find(xs("48 89 5C 24 08 57 48 81 EC C0 00 00 00 8B FA 48 C7 C3 FF FF FF FF")).As<uint32_t (__fastcall*)(const char*, uint32_t)>();
	static auto get_list = Memory->Modules.Client.Find(xs("4C 8B DC 45 89 4B 20 4D 89 43 18 49 89 53 10 49 89 4B 08 55 49 8D 6B A1")).As<void (__fastcall*)(CModel*, CUtlVector<CHitboxDescriptor>*, uint64_t, uint32_t)>();
	static auto allocator = reinterpret_cast<void**>(GetProcAddress(reinterpret_cast<HMODULE>(Memory->Modules.Tier0.Handle()), xs("g_pMemAlloc")));

	auto model = GetModel();
	if (!model || count <= 0 || !get_set || !hash_name || !get_list || !allocator || !*allocator)
		return false;

	auto name = get_set(this);
	uint32_t hash = name && *name ? hash_name(name, 0x31415926) : 0;
	hitboxes.resize(count);
	CUtlVector<CHitboxDescriptor> list{ 0, hitboxes.data(), static_cast<unsigned int>(count), static_cast<int>(0x80000000u) };
	get_list(model, &list, HitboxListContext(), hash);

	bool complete = list.size == count && list.elements == hitboxes.data();
	if (list.elements != hitboxes.data())
		Memory->Vtable.Execute<void>(*allocator, 3, list.elements);
	if (!complete)
		hitboxes.clear();
	return complete;
}

uint32_t CSkeletonInstance::GetBoneUsageFlags(int index) const {
	if (index < 0 || index >= bone_count || bone_count > transform_count)
		return 0;

	const auto flags = *reinterpret_cast<const uint32_t* const*>(reinterpret_cast<uintptr_t>(this) + 0x208);
	return flags ? flags[index] & BONE_USAGE_MASK : 0;
}

bool CSkeletonInstance::IsHitboxBone(int index) const {
	return (GetBoneUsageFlags(index) & BONE_USED_BY_HITBOX) != 0;
}

bool CSkeletonInstance::IsBoneValid(int index) const {
	return bone_cache && bone_validity && index >= 0 && index < bone_count && bone_count <= transform_count &&
		(bone_validity[index >> 5] & (1u << (index & 31))) != 0;
}

bool CSkeletonInstance::CalculateWorldSpaceBones(uint32_t flags) {
	static auto fn = Memory->Modules.Client.Find(xs("48 89 4C 24 ? 55 53 56 57 41 54 41 55 41 56 41 57 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8D 6C 24 ? 48 8B 81")).As<void (__fastcall*)(CSkeletonInstance*, uint32_t)>();
	if (!fn)
		return false;

	fn(this, flags & BONE_USAGE_MASK);
	return true;
}

bool CSkeletonInstance::CalculateAnimationState(uint32_t flags) {
	static auto fn = Memory->Modules.Client.Find(xs("40 55 56 57 41 54 41 55 41 56 41 57 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8D 6C 24")).As<void (__fastcall*)(CSkeletonInstance*, uint32_t)>();
	if (!fn)
		return false;

	fn(this, flags & BONE_USAGE_MASK);
	return true;
}

CHitbox* CModel::GetHitbox(const int32_t index) {
	auto model_data = &this->permodel_data;

	if (this->permodel_data.ref_meshes.size <= 0)
		return nullptr;

	auto mesh = this->permodel_data.ref_meshes.Element(0);
	if (!mesh)
		return nullptr;

	auto hitboxsets = mesh->hitbox_sets;
	if (!hitboxsets)
		return nullptr;

	if (hitboxsets->hitbox_count <= 0 || index < 0 || index >= hitboxsets->hitbox_count)
		return nullptr;

	auto& hitbox = hitboxsets->hitbox;
	return &hitbox[index];
}

int CModel::GetHitboxCount() {
	if (this->permodel_data.ref_meshes.size <= 0)
		return -1;

	auto meshes = this->permodel_data.ref_meshes.Element(0);
	if (!meshes)
		return -1;

	auto hithoxsets = meshes->hitbox_sets;
	if (!hithoxsets)
		return -1;

	return hithoxsets[0].hitbox_count;
}

C_CSWeaponBase* C_CSPlayerPawn::GetActiveWeapon() {
	if (!this || !this->m_pWeaponServices())
		return {};

	auto entity = this->m_pWeaponServices()->m_hActiveWeapon().get<C_CSWeaponBase>();
	if (!entity)
		return nullptr;

	return entity;
}

bool C_CSPlayerPawn::Enemy(C_CSPlayerPawn* other) {
	if (!other || other == this)
		return false;

	cvar(game_type);
	cvar(game_mode);
	cvar(mp_teammates_are_enemies);

	if (game_type && game_mode && game_type->GetInt() == 6 && game_mode->GetInt() == 0)
		return this->m_nSurvivalTeam() != other->m_nSurvivalTeam();

	if (mp_teammates_are_enemies && mp_teammates_are_enemies->GetBool())
		return true;

	return this->m_iTeamNum() != other->m_iTeamNum();
}

bool C_CSWeaponBase::GetAttackTime(CUserCommand* command, Taia_t& time, int mode) {
	static auto get_time = Memory->Modules.Client.Find(xs("40 53 48 83 EC 30 48 8B 05 ? ? ? ? 48 8B D9 0F 29 74 24 20 48 8D 4C 24 40 F3 0F 10 70 50")).As<Taia_t* (__fastcall*)(Taia_t*, uint32_t)>();
	static auto from_global = Memory->Modules.Client.Find(xs("40 53 48 83 EC 20 48 8B D9 45 85 C0 74 0B 89 11 48 8B C1 48 83 C4 20 5B C3")).As<int* (__fastcall*)(int*, int, uint32_t)>();
	static auto to_global = Memory->Modules.Client.Find(xs("48 83 EC 28 85 D2 75 1B 48 8B 05 ? ? ? ? 48 85 C0 74 0F 44 8B C2 8B D1 48 8B C8")).As<int (__fastcall*)(int, uint32_t)>();

	auto pawn = this->m_hOwnerEntity().get<C_CSPlayerPawn>();
	if (!pawn || !pawn->m_pWeaponServices())
		return false;

	const auto group = this->m_pEntity()->m_worldGroupId();
	const int index = mode == 0 ? command->pb.attack1_start_history_index() : mode == 1 ? command->pb.attack2_start_history_index() : -1;

	Taia_t now;
	get_time(&now, group);
	++now.tick;

	if (index >= 0 && index < command->pb.input_history_size()) {
		const auto& input = command->pb.input_history(index);

		from_global(&time.tick, input.player_tick_count(), group);
		time.fraction = input.player_tick_fraction();
		time.Normalize();

		const Taia_t oldest(now.tick - 4, now.fraction);
		if (time < oldest)
			time = oldest;

		if (time > now)
			time = now;

		time.tick = to_global(time.tick, group);
		return true;
	}

	const int tick = mode == 1 ? this->m_nNextSecondaryAttackTick() : this->m_nNextPrimaryAttackTick();
	const float fraction = mode == 1 ? this->m_flNextSecondaryAttackTickRatio() : this->m_flNextPrimaryAttackTickRatio();
	const float offset = this->m_flWatTickOffset();
	auto attack = Taia_t(tick, fraction).Normalized();
	auto data = this->GetWeaponData();

	if (mode == 0 && data && data->m_bIsRevolver() && m_bIsHauledBack()) {
		const auto ready = Taia_t(m_nPostponeFireReadyTicks(), m_flPostponeFireReadyFrac()).Normalized();
		if (ready <= Taia_t(now.tick - 1, now.fraction))
			attack = ready;
	}

	++attack.tick;
	time = attack.Shifted(offset);
	const auto next = time;
	time.tick = to_global(time.tick, group);

	const auto& history = pawn->m_pWeaponServices()->m_InputHistory();
	if (history.count < 0 || history.count > 32 || history.first < 0 || history.first >= 32)
		return false;

	for (int i = history.count - 1; i >= 0; --i) {
		const auto input = Taia_t(history.entries[(history.first + i) % 32].player_time).Normalized();
		if (input > time)
			continue;

		const auto age = time - input;
		if (age.tick < 4 || (age.tick == 4 && age.fraction == 0.f))
			return true;

		break;
	}

	const Taia_t limit(next.tick + 1, next.fraction);
	time = now > limit ? now : next;
	time.tick = to_global(time.tick, group);
	return false;
}

bool CShootPositionHistory::GetPosition(const Taia_t& time, Vector& position) const {
	if (count < 2 || count > 32 || first < 0 || first >= 32 || !std::isfinite(time.fraction))
		return false;

	for (int i = 1; i < count; ++i) {
		const auto& before = entries[(first + i - 1) % 32];
		const auto& after = entries[(first + i) % 32];

		if (before.time > time)
			return false;

		if (after.time < time)
			continue;

		const auto interval = after.time - before.time;
		const float duration = static_cast<float>(interval.tick) + interval.fraction;
		if (duration <= 0.f)
			return false;

		const auto elapsed = time - before.time;
		const float fraction = (static_cast<float>(elapsed.tick) + elapsed.fraction) / duration;
		position = before.position * (1.f - fraction) + after.position * fraction;
		return true;
	}

	return false;
}

void CCSPlayer_WeaponServices::InterpolateShootPosition(Taia_t time, Vector& position) {
	using fn = void(__fastcall*)(CCSPlayer_WeaponServices*, Vector*, Taia_t*);
	static auto fnInterpolateShootPosition = Memory->Modules.Client.Find(xs("40 55 56 41 56 48 81 EC ? ? ? ? 48 8B 05")).As<fn>();

	fnInterpolateShootPosition(this, &position, &time);
}

void CGameSceneNode::SetMeshGroupMask(uint64_t mask) {
	using fn = void(__fastcall*)(void*, unsigned int);
	static fn x = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71")).As<fn>();
	if (x)
		x(this, mask);
}