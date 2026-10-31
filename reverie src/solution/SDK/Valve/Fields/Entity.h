#pragma once

#include "../Datatypes/Hitbox.h"
#include <cstddef>
#include <vector>

struct CEntityInstance;

struct CEntityIdentity {
	FindOffset(CEntityInstance*, Instance, 0x0);
	FindOffset(uint32_t, Index, 0x10);

	FindField(uint32_t, m_flags, xs("CEntityIdentity"), xs("m_flags"));
	FindField(CEntityIdentity*, m_pPrevByClass, xs("CEntityIdentity"), xs("m_pPrevByClass"));
	FindField(CEntityIdentity*, m_pNextByClass, xs("CEntityIdentity"), xs("m_pNextByClass"));
	FindField(CEntityIdentity*, m_pPrev, xs("CEntityIdentity"), xs("m_pPrev"));
	FindField(CEntityIdentity*, m_pNext, xs("CEntityIdentity"), xs("m_pNext"));
	FindOffset(uint32_t, m_worldGroupId, 0x38);

	bool Valid();
	int GetEntryIndex();
	int GetSerialNumber();
	bool IsSafeToModify();
};

struct CEntityInstance {
	FindField(CEntityIdentity*, m_pEntity, xs("CEntityInstance"), xs("m_pEntity"));

	CBaseHandle GetHandle();
	CSchemaClassInfo* GetClassBindingBase();
};

struct CCollisionProperty {
	FindField(Vector, m_vecMins, xs("CCollisionProperty"), xs("m_vecMins"));
	FindField(Vector, m_vecMaxs, xs("CCollisionProperty"), xs("m_vecMaxs"));
	FindField(uint8_t, m_usSolidFlags, xs("CCollisionProperty"), xs("m_usSolidFlags"));
	FindField(uint8_t, m_nSolidType, xs("CCollisionProperty"), xs("m_nSolidType"));
};

struct CHitbox {
	const char* name;
	const char* surface_property;
	const char* bone_name;
	Vector mins;
	Vector maxs;
	float shape_radius;
	uint32_t bone_hash;
	int32_t hitgroup;
	uint8_t shape_type;
	bool translation_only;
	uint32_t crc;
	uint8_t render_color[4];
	uint16_t bone_index;
	bool is_forced_transform;
	char pad004B[5];
	CTransform forced_transform;
};

static_assert(offsetof(CHitbox, bone_index) == 0x48);
static_assert(offsetof(CHitbox, is_forced_transform) == 0x4A);
static_assert(offsetof(CHitbox, forced_transform) == 0x50);
static_assert(sizeof(CHitbox) == 0x70);

struct CHitboxTransformRecord {
	int tick;
	char pad0004[12];
	int count;
	char pad0014[12];
	Matrix2x4_t* transforms;
	int capacity;
	uint32_t flags;
	Matrix2x4_t inline_transforms[32];
};

struct CHitboxTransformHistory {
	void* vtable;
	char pad0008[8];
	CHitboxTransformRecord records[4];
	int first;
	int count;

	const CHitboxTransformRecord* GetLatest() const;
};

static_assert(offsetof(CHitboxTransformRecord, count) == 0x10);
static_assert(offsetof(CHitboxTransformRecord, transforms) == 0x20);
static_assert(offsetof(CHitboxTransformRecord, capacity) == 0x28);
static_assert(offsetof(CHitboxTransformRecord, flags) == 0x2C);
static_assert(offsetof(CHitboxTransformRecord, inline_transforms) == 0x30);
static_assert(sizeof(CHitboxTransformRecord) == 0x430);
static_assert(offsetof(CHitboxTransformHistory, records) == 0x10);
static_assert(offsetof(CHitboxTransformHistory, first) == 0x10D0);
static_assert(offsetof(CHitboxTransformHistory, count) == 0x10D4);
static_assert(sizeof(CHitboxTransformHistory) == 0x10D8);

struct CHitboxSets {
	char pad0000[40];
	int32_t hitbox_count;
	char pad002C[4];
	CHitbox* hitbox;
	char pad0038[16];
};

struct CRenderMesh {
	char pad0000[360];
	CHitboxSets* hitbox_sets;
	int32_t hitbox_sets_count;
};

class CModel;

struct CPermodelExternalPart {
	CTransform transform;
	const char* name;
	int32_t parent;
	uint8_t pad002C[4];
	CStrongHandle<CModel> ref_model;
};

struct CPermodelData {
	const char* name;
	uint32_t flags;
	Vector hull_min;
	Vector hull_max;
	Vector view_min;
	Vector view_max;
	float mass;
	Vector eye_position;
	float max_eye_deflection;
	char* surface_property;
	CUtlVectorTemple<CPermodelExternalPart> external_parts;
	CUtlVectorTemple<CRenderMesh*> ref_meshes;
};

struct CModel {
	CHitbox* GetHitbox(const int32_t index);
	int GetHitboxCount();

	void* vtable;
	CPermodelData permodel_data;
};

struct CModelState {
	char pad[160];
	CStrongHandle<CModel> model_handle;

	FindOffset(__int64, Unk, 0xD8);
};

struct C_CSPlayerPawn;

struct CAnimGraph2FloatParam {
	uint64_t name;
	uintptr_t container;
	int16_t index;

	void Set(float value);
};

struct CCS2PawnGraphController {
	FindField(CAnimGraph2FloatParam, m_flCrouchAmount, xs("CCS2PawnGraphController"), xs("m_flCrouchAmount"));
};

struct CSkeletonInstance {
	// World calculation may add this dependency to a hitbox request.
	static constexpr uint32_t BONE_DEFORMATION_DEPENDENCY = 0x8;
	static constexpr uint32_t BONE_USED_BY_HITBOX = 0x100;
	static constexpr uint32_t BONE_USAGE_MASK = 0xFFFFF;

	FindOffset(C_CSPlayerPawn*, pawn, 0x30);
	FindOffset(uint8_t, WorldTransformDirty, 0x110);
	FindOffset(uintptr_t, HitboxListContext, 0x348);
	FindOffset(void*, HitboxTransformOverride, 0x440);
	FindOffset(uint32_t, WorldBoneMask, 0x458);

	FindField(CModelState, m_modelState, xs("CSkeletonInstance"), xs("m_modelState"));
	FindField(uint8_t, m_nHitboxSet, xs("CSkeletonInstance"), xs("m_nHitboxSet"));

	uintptr_t GetModelHandle() const;
	uintptr_t GetProcessedModelHandle() const;
	CModel* GetModel() const;
	bool GetHitboxList(std::vector<CHitboxDescriptor>& hitboxes, int count);
	uint32_t GetBoneUsageFlags(int index) const;
	bool IsHitboxBone(int index) const;
	bool IsBoneValid(int index) const;

	char pad0[0x1C0];
	Matrix2x4_t* bone_cache;
	uint32_t* bone_validity;
	int transform_count; // Bones followed by attachment transforms.
	int bone_count;
	int attachment_count;
	char pad2[0x4];

	// Return whether the engine entry point was called; check bone validity separately.
	bool CalculateWorldSpaceBones(uint32_t flags);
	bool CalculateAnimationState(uint32_t flags);
};

static_assert(offsetof(CSkeletonInstance, bone_cache) == 0x1C0);
static_assert(offsetof(CSkeletonInstance, bone_validity) == 0x1C8);
static_assert(offsetof(CSkeletonInstance, transform_count) == 0x1D0);
static_assert(offsetof(CSkeletonInstance, bone_count) == 0x1D4);
static_assert(offsetof(CSkeletonInstance, attachment_count) == 0x1D8);

struct CGameSceneNode {
	FindField(Vector, m_vecOrigin, xs("CGameSceneNode"), xs("m_vecOrigin"));
	FindField(Vector, m_angRotation, xs("CGameSceneNode"), xs("m_angRotation"));
	FindField(float, m_flScale, xs("CGameSceneNode"), xs("m_flScale"));
	FindField(Vector, m_vecAbsOrigin, xs("CGameSceneNode"), xs("m_vecAbsOrigin"));
	FindField(Vector, m_angAbsRotation, xs("CGameSceneNode"), xs("m_angAbsRotation"));
	FindField(float, m_flAbsScale, xs("CGameSceneNode"), xs("m_flAbsScale"));
	FindField(float, m_flZOffset, xs("CGameSceneNode"), xs("m_flZOffset"));
	FindField(Vector, m_vRenderOrigin, xs("CGameSceneNode"), xs("m_vRenderOrigin"));
	FindField(CGameSceneNode*, m_pParent, xs("CGameSceneNode"), xs("m_pParent"));
	FindField(CGameSceneNode*, m_pChild, xs("CGameSceneNode"), xs("m_pChild"));
	FindField(CEntityInstance*, m_pOwner, xs("CGameSceneNode"), xs("m_pOwner"));
	FindField(CGameSceneNode*, m_pNextSibling, xs("CGameSceneNode"), xs("m_pNextSibling"));
	FindField(CTransform, m_nodeToWorld, xs("CGameSceneNode"), xs("m_nodeToWorld"));

	CSkeletonInstance* GetSkeletonInstance() { return Memory->Vtable.Execute<CSkeletonInstance*>(this, 14); }

	void SetSkeleton(uint64_t mask);
	void SetMeshGroup(uint64_t mask);
	void SetMeshGroupMask(uint64_t mask);
};

struct C_BaseEntity : CEntityInstance {
	FindField(int, m_iTeamNum, xs("C_BaseEntity"), xs("m_iTeamNum"));
	FindField(int, m_iHealth, xs("C_BaseEntity"), xs("m_iHealth"));
	FindField(int, m_fFlags, xs("C_BaseEntity"), xs("m_fFlags"));
	FindField(int, m_MoveType, xs("C_BaseEntity"), xs("m_MoveType"));
	FindField(int, m_iEFlags, xs("C_BaseEntity"), xs("m_iEFlags"));
	FindField(int, m_flFriction, xs("C_BaseEntity"), xs("m_flFriction"));
	FindField(CBaseHandle, m_hOwnerEntity, xs("C_BaseEntity"), xs("m_hOwnerEntity"));
	FindField(int, m_nActualMoveType, xs("C_BaseEntity"), xs("m_nActualMoveType"));
	FindField(int, m_nLastPredictableCommand, xs("C_BaseEntity"), xs("m_nLastPredictableCommand"));
	FindField(int, m_nFirstPredictableCommand, xs("C_BaseEntity"), xs("m_nFirstPredictableCommand"));
	FindField(Vector, m_vecVelocity, xs("C_BaseEntity"), xs("m_vecVelocity"));
	FindField(Vector, m_vecAbsVelocity, xs("C_BaseEntity"), xs("m_vecAbsVelocity"));
	FindField(uint32_t, m_nSubclassID, xs("C_BaseEntity"), xs("m_nSubclassID"));
	FindField(Vector, m_vecViewOffset, xs("C_BaseModelEntity"), xs("m_vecViewOffset"));
	FindField(Vector, m_vecBaseVelocity, xs("C_BaseModelEntity"), xs("m_vecBaseVelocity"));
	FindField(float, m_flSimulationTime, xs("C_BaseEntity"), xs("m_flSimulationTime"));
	FindField(float, m_flOldSimulationTime, xs("C_BaseEntity"), xs("m_flOldSimulationTime"));
	FindField(int, m_nSimulationTick, xs("C_BaseEntity"), xs("m_nSimulationTick"));
	FindField(float, m_flGravityScale, xs("C_BaseEntity"), xs("m_flGravityScale"));
	FindField(float, m_flActualGravityScale, xs("C_BaseEntity"), xs("m_flActualGravityScale"));
	FindField(int, m_nGroundBodyIndex, xs("C_BaseEntity"), xs("m_nGroundBodyIndex"));
	FindField(CBaseHandle, m_hGroundEntity, xs("C_BaseEntity"), xs("m_hGroundEntity"));
	FindField(CGameSceneNode*, m_pGameSceneNode, xs("C_BaseEntity"), xs("m_pGameSceneNode"));
	FindField(CCollisionProperty*, m_pCollision, xs("C_BaseEntity"), xs("m_pCollision"));
	FindField(Vector, m_vecAngVelocity, xs("C_BaseEntity"), xs("m_vecAngVelocity"));
	FindField(float, m_flWaterLevel, xs("C_BaseEntity"), xs("m_flWaterLevel"));
	FindField(uint8_t, m_lifeState, xs("C_BaseEntity"), xs("m_lifeState"));
	FindField(int, m_nNoInterpolationTick, xs("C_BaseEntity"), xs("m_nNoInterpolationTick"));
	FindField(bool, m_bSimulationTimeChanged, xs("C_BaseEntity"), xs("m_bSimulationTimeChanged"));
	FindField(float, m_flElasticity, xs("C_BaseEntity"), xs("m_flElasticity"));
	FindField(uint8_t, m_MoveCollide, xs("C_BaseEntity"), xs("m_MoveCollide"));

	int GetIndex() {
		static auto fn = Memory->Modules.Client.Find(xs("48 83 EC ? 4C 8B 0D ? ? ? ? 4C 8B DA")).As<int(__fastcall*)(void*, int*)>();

		int index = -1;
		fn(this, &index);
		return index;
	}

	bool IsWeapon();
	int GetBoneIndex(const char* name);

	float GetInterpolationAmount() {
		cvar(cq_enable);
		cvar(cq_dilation_percentage);

		if (!cq_enable->GetBool() || (this->m_fFlags() & 0x100) != 0)
			return 1.f / 64.f;
		
		return (cq_dilation_percentage->GetFloat() / 100.f + 1.f) / 64.f;
	}

	void SetModel(const char* model) {
		static auto fn = Memory->Modules.Client.Find(xs("40 53 48 83 EC ? 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24")).As<void(__fastcall*)(void*, const char*)>();
		fn(this, model);
	}

	void PostDataUpdate(int type) { Memory->Vtable.Execute<void>(this, 10, type); }
};

struct CAttributeList {
	FindField(uintptr_t, m_Attributes, xs("CAttributeList"), xs("m_Attributes"));
};

struct C_EconItemView {
	FindField(uint16_t, m_iItemDefinitionIndex, xs("C_EconItemView"), xs("m_iItemDefinitionIndex"));
	FindField(uint64_t, m_iItemID, xs("C_EconItemView"), xs("m_iItemID"));
	FindField(uint32_t, m_iItemIDLow, xs("C_EconItemView"), xs("m_iItemIDLow"));
	FindField(uint32_t, m_iItemIDHigh, xs("C_EconItemView"), xs("m_iItemIDHigh"));
	FindField(uint32_t, m_iAccountID, xs("C_EconItemView"), xs("m_iAccountID"));
	FindField(int, m_nFallbackPaintKit, xs("C_EconItemView"), xs("m_nFallbackPaintKit"));
	FindField(int, m_nFallbackSeed, xs("C_EconItemView"), xs("m_nFallbackSeed"));
	FindField(float, m_flFallbackWear, xs("C_EconItemView"), xs("m_flFallbackWear"));
	FindField(int, m_nFallbackStatTrak, xs("C_EconItemView"), xs("m_nFallbackStatTrak"));
	FindField(int, m_iEntityQuality, xs("C_EconItemView"), xs("m_iEntityQuality"));
	FindFieldArray(m_AttributeList, CAttributeList, xs("C_EconItemView"), xs("m_AttributeList"));
	FindField(const char*, m_szCustomName, xs("C_EconItemView"), xs("m_szCustomName"));
	FindField(const char*, m_szCustomNameOverride, xs("C_EconItemView"), xs("m_szCustomNameOverride"));
	FindField(bool, m_bInitialized, xs("C_EconItemView"), xs("m_bInitialized"));
	FindField(bool, m_bDisallowSOC, xs("C_EconItemView"), xs("m_bDisallowSOC"));
	FindField(bool, m_bRestoreCustomMaterialAfterPrecache, xs("C_EconItemView"), xs("m_bRestoreCustomMaterialAfterPrecache"));
	FindField(void*, m_NetworkedDynamicAttributes, xs("C_EconItemView"), xs("m_NetworkedDynamicAttributes"));
	FindOffset(uintptr_t, NameDescriptionPtr, 0x200);

	void SetAttributeByName(const char* name, float value) {
		using fn = void(__fastcall*)(void*, const char*, float);
		static fn x = Memory->Modules.Client.Find(xs("E8 ? ? ? ? 66 41 0F 6E D4")).Rel(1, 5).As<fn>();
		return x(this, name, value);
	}
};

struct C_AttributeContainer {
	FindFieldPtr(C_EconItemView, m_Item, xs("C_AttributeContainer"), xs("m_Item"));
};

struct C_EconEntity : C_BaseEntity {
	FindField(int, m_nFallbackSeed, xs("C_EconEntity"), xs("m_nFallbackSeed"));
	FindField(int, m_nFallbackPaintKit, xs("C_EconEntity"), xs("m_nFallbackPaintKit"));
	FindField(int, m_nFallbackStatTrak, xs("C_EconEntity"), xs("m_nFallbackStatTrak"));
	FindField(float, m_flFallbackWear, xs("C_EconEntity"), xs("m_flFallbackWear"));
	FindField(uint32_t, m_OriginalOwnerXuidLow, xs("C_EconEntity"), xs("m_OriginalOwnerXuidLow"));
	FindField(uint32_t, m_OriginalOwnerXuidHigh, xs("C_EconEntity"), xs("m_OriginalOwnerXuidHigh"));
	FindFieldPtr(C_AttributeContainer, m_AttributeManager, xs("C_EconEntity"), xs("m_AttributeManager"));
};

struct CCSWeaponBaseVData {
	FindField(int, m_WeaponType, xs("CCSWeaponBaseVData"), xs("m_WeaponType"));
	FindField(bool, m_bIsRevolver, xs("CCSWeaponBaseVData"), xs("m_bIsRevolver"));
	FindField(const char*, m_szName, xs("CCSWeaponBaseVData"), xs("m_szName"));
	FindField(float, m_flRange, xs("CCSWeaponBaseVData"), xs("m_flRange"));
	FindField(float, m_flRangeModifier, xs("CCSWeaponBaseVData"), xs("m_flRangeModifier"));
	FindField(float, m_flHeadshotMultiplier, xs("CCSWeaponBaseVData"), xs("m_flHeadshotMultiplier"));
	FindField(float, m_flArmorRatio, xs("CCSWeaponBaseVData"), xs("m_flArmorRatio"));
	FindField(float, m_flPenetration, xs("CCSWeaponBaseVData"), xs("m_flPenetration"));
	FindField(float, m_flThrowVelocity, xs("CCSWeaponBaseVData"), xs("m_flThrowVelocity"));
	FindField(float, m_flInaccuracyJumpApex, xs("CCSWeaponBaseVData"), xs("m_flInaccuracyJumpApex"));
	FindField(float, m_flInaccuracyJumpInitial, xs("CCSWeaponBaseVData"), xs("m_flInaccuracyJumpInitial"));
	FindField(int, m_nDamage, xs("CCSWeaponBaseVData"), xs("m_nDamage"));
	FindField(int32_t, m_nNumBullets, xs("CCSWeaponBaseVData"), xs("m_nNumBullets"));
	FindField(int32_t, m_nZoomLevels, xs("CCSWeaponBaseVData"), xs("m_nZoomLevels"));
	FindField(int32_t, m_nSpreadSeed, xs("CCSWeaponBaseVData"), xs("m_nSpreadSeed"));
	FindField(int32_t, m_iMaxClip1, xs("CBasePlayerWeaponVData"), xs("m_iMaxClip1"));
	FindField(bool, m_bIsFullAuto, xs("CCSWeaponBaseVData"), xs("m_bIsFullAuto"));
	FindFieldArray(m_flCycleTime, float_t, xs("CCSWeaponBaseVData"), xs("m_flCycleTime"));
	FindFieldArray(m_flMaxSpeed, float_t, xs("CCSWeaponBaseVData"), xs("m_flMaxSpeed"));
	FindFieldArray(m_flInaccuracyStand, float_t, xs("CCSWeaponBaseVData"), xs("m_flInaccuracyStand"));
	FindFieldArray(m_flInaccuracyCrouch, float_t, xs("CCSWeaponBaseVData"), xs("m_flInaccuracyCrouch"));
};

struct C_CSWeaponBase : C_EconEntity {
	FindField(int32_t, m_zoomLevel, xs("C_CSWeaponBaseGun"), xs("m_zoomLevel"));
	FindField(int32_t, m_nNextPrimaryAttackTick, xs("C_BasePlayerWeapon"), xs("m_nNextPrimaryAttackTick"));
	FindField(float_t, m_flNextPrimaryAttackTickRatio, xs("C_BasePlayerWeapon"), xs("m_flNextPrimaryAttackTickRatio"));
	FindField(int32_t, m_nNextSecondaryAttackTick, xs("C_BasePlayerWeapon"), xs("m_nNextSecondaryAttackTick"));
	FindField(float_t, m_flNextSecondaryAttackTickRatio, xs("C_BasePlayerWeapon"), xs("m_flNextSecondaryAttackTickRatio"));
	FindField(float, m_flRecoilIndex, xs("C_CSWeaponBase"), xs("m_flRecoilIndex"));
	FindField(float, m_fAccuracyPenalty, xs("C_CSWeaponBase"), xs("m_fAccuracyPenalty"));
	FindField(float, m_flTurningInaccuracyDelta, xs("C_CSWeaponBase"), xs("m_flTurningInaccuracyDelta"));
	FindField(Vector, m_vecTurningInaccuracyEyeDirLast, xs("C_CSWeaponBase"), xs("m_vecTurningInaccuracyEyeDirLast"));
	FindField(float, m_flTurningInaccuracy, xs("C_CSWeaponBase"), xs("m_flTurningInaccuracy"));
	FindField(float, m_flLastAccuracyUpdateTime, xs("C_CSWeaponBase"), xs("m_flLastAccuracyUpdateTime"));
	FindField(int32_t, m_weaponMode, xs("C_CSWeaponBase"), xs("m_weaponMode"));
	FindField(int32_t, m_nPostponeFireReadyTicks, xs("C_CSWeaponBase"), xs("m_nPostponeFireReadyTicks"));
	FindField(float, m_flPostponeFireReadyFrac, xs("C_CSWeaponBase"), xs("m_flPostponeFireReadyFrac"));
	FindField(bool, m_bIsHauledBack, xs("C_CSWeaponBase"), xs("m_bIsHauledBack"));
	FindField(bool, m_bInReload, xs("C_CSWeaponBase"), xs("m_bInReload"));
	FindField(int32_t, m_iClip1, xs("C_BasePlayerWeapon"), xs("m_iClip1"));
	FindField(int32_t, m_pReserveAmmo, xs("C_BasePlayerWeapon"), xs("m_pReserveAmmo"));
	FindField(float, m_fLastShotTime, xs("C_CSWeaponBase"), xs("m_fLastShotTime"));
	FindField(float, m_flWatTickOffset, xs("C_CSWeaponBase"), xs("m_flWatTickOffset"));
	FindField(bool, m_bBurstMode, xs("C_CSWeaponBase"), xs("m_bBurstMode"));
	FindField(int32_t, m_iBurstShotsRemaining, xs("C_CSWeaponBaseGun"), xs("m_iBurstShotsRemaining"));
	FindField(int, m_iOriginalTeamNumber, xs("C_CSWeaponBase"), xs("m_iOriginalTeamNumber"));

	CCSWeaponBaseVData* GetWeaponData() { return *reinterpret_cast<CCSWeaponBaseVData**>(reinterpret_cast<uintptr_t>(this) + 904); }
	bool GetAttackTime(CUserCommand* command, Taia_t& time, int mode = 0);
	bool CanShoot();
	float GetSpread();
	float GetInaccuracy();
	bool UpdateAccuracyPenalty();
	std::string GetName();
	bool IsFiring();

	void UpdateCompositeMaterial() {
		static auto fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 44 0F B6 F2")).As<void(__fastcall*)(void*, bool)>();
		if (!fn)
			return;

		fn(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + 0x608), true);
	}

	void UpdatePaintKit() {
		static auto fn = Memory->Modules.Client.Find(xs("40 55 53 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 44 0F B6 FA")).As<void(__fastcall*)(void*, bool)>();
		if (!fn)
			return;

		fn(this, true);
	}
};

struct C_BaseCSGrenade : C_CSWeaponBase {
	FindField(bool, m_bIsHeldByPlayer, xs("C_BaseCSGrenade"), xs("m_bIsHeldByPlayer"));
	FindField(bool, m_bPinPulled, xs("C_BaseCSGrenade"), xs("m_bPinPulled"));
	FindField(bool, m_bJumpThrow, xs("C_BaseCSGrenade"), xs("m_bJumpThrow"));
	FindField(bool, m_bJustPulledPin, xs("C_BaseCSGrenade"), xs("m_bJustPulledPin"));
	FindField(float, m_fThrowTime, xs("C_BaseCSGrenade"), xs("m_fThrowTime"));
	FindField(float, m_flThrowStrength, xs("C_BaseCSGrenade"), xs("m_flThrowStrength"));
	FindField(float, m_flThrowStrengthApproach, xs("C_BaseCSGrenade"), xs("m_flThrowStrengthApproach"));
	FindField(CBaseHandle, m_hThrower, xs("C_BaseGrenade"), xs("m_hThrower"));
};

struct C_BaseCSGrenadeProjectile : C_BaseCSGrenade {
	FindField(float, m_nExplodeEffectTickBegin, xs("C_BaseCSGrenadeProjectile"), xs("m_nExplodeEffectTickBegin"));
	FindField(Vector, m_vInitialPosition, xs("C_BaseCSGrenadeProjectile"), xs("m_vInitialPosition"));
	FindField(Vector, m_vInitialVelocity, xs("C_BaseCSGrenadeProjectile"), xs("m_vInitialVelocity"));
	FindField(bool, m_bExplodeEffectBegan, xs("C_BaseCSGrenadeProjectile"), xs("m_bExplodeEffectBegan"));
	FindField(bool, m_bSmokeEffectSpawned, xs("C_SmokeGrenadeProjectile"), xs("m_bSmokeEffectSpawned"));
	FindField(int, m_nDecoyShotTick, xs("C_DecoyProjectile"), xs("m_nDecoyShotTick"));
};

struct C_Inferno : C_BaseEntity {
	FindFieldArray(m_bFireIsBurning, bool, xs("C_Inferno"), xs("m_bFireIsBurning"));
	FindFieldArray(m_firePositions, Vector, xs("C_Inferno"), xs("m_firePositions"));
	FindFieldArray(m_fireParentPositions, Vector, xs("C_Inferno"), xs("m_fireParentPositions"));
	FindField(int32_t, m_fireCount, xs("C_Inferno"), xs("m_fireCount"));
	FindField(int32_t, m_nFireEffectTickBegin, xs("C_Inferno"), xs("m_nFireEffectTickBegin"));
	FindField(float, m_nFireLifetime, xs("C_Inferno"), xs("m_nFireLifetime"));
	FindField(Vector, m_minBounds, xs("C_Inferno"), xs("m_minBounds"));
	FindField(Vector, m_maxBounds, xs("C_Inferno"), xs("m_maxBounds"));
};

struct C_SmokeProjectile : C_BaseCSGrenadeProjectile {
	FindField(int32_t, m_nSmokeEffectTickBegin, xs("C_SmokeGrenadeProjectile"), xs("m_nSmokeEffectTickBegin"));
	FindField(bool, m_bDidSmokeEffect, xs("C_SmokeGrenadeProjectile"), xs("m_bDidSmokeEffect"));
	FindField(int32_t, m_nRandomSeed, xs("C_SmokeGrenadeProjectile"), xs("m_nRandomSeed"));
	FindField(Vector, m_vSmokeColor, xs("C_SmokeGrenadeProjectile"), xs("m_vSmokeColor"));
	FindField(Vector, m_vSmokeDetonationPos, xs("C_SmokeGrenadeProjectile"), xs("m_vSmokeDetonationPos"));
	FindField(int32_t, m_nVoxelFrameDataSize, xs("C_SmokeGrenadeProjectile"), xs("m_nVoxelFrameDataSize"));
	FindField(int32_t, m_nVoxelUpdate, xs("C_SmokeGrenadeProjectile"), xs("m_nVoxelUpdate"));
	FindField(bool, m_bSmokeVolumeDataReceived, xs("C_SmokeGrenadeProjectile"), xs("m_bSmokeVolumeDataReceived"));
};

struct CShootPosition {
	Taia_t time;
	Vector position;
};

struct CShootPositionHistory {
	int total;
	int padding;
	uintptr_t vtable;
	CShootPosition entries[32];
	int first;
	int count;

	bool GetPosition(const Taia_t& time, Vector& position) const;
};

struct CWeaponInput {
	uint8_t padding[0x60];
	Taia_t render_time;
	Taia_t player_time;
	uint8_t padding2[8];
};

struct CWeaponInputHistory {
	uintptr_t vtable;
	CWeaponInput entries[32];
	int first;
	int count;
};

static_assert(sizeof(CShootPosition) == 0x14);
static_assert(sizeof(CShootPositionHistory) == 0x298);
static_assert(offsetof(CShootPositionHistory, entries) == 0x10);
static_assert(offsetof(CShootPositionHistory, first) == 0x290);
static_assert(sizeof(CWeaponInput) == 0x78);
static_assert(sizeof(CWeaponInputHistory) == 0xF10);
static_assert(offsetof(CWeaponInputHistory, first) == 0xF08);

struct CCSPlayer_WeaponServices {
	FindField(CBaseHandle, m_hActiveWeapon, xs("CPlayer_WeaponServices"), xs("m_hActiveWeapon"));
	FindField(CUtlVector<CBaseHandle>, m_hMyWeapons, xs("CPlayer_WeaponServices"), xs("m_hMyWeapons"));
	FindField(float, m_flNextAttack, xs("CCSPlayer_WeaponServices"), xs("m_flNextAttack"));
	FindOffset(CShootPositionHistory, m_ShootPositionHistory, 0xD8);
	FindOffset(CWeaponInputHistory, m_InputHistory, 0x378);

	Vector GetEyePosition() {
		if (!this)
			return {};

		Vector eye_pos{};
		Memory->Vtable.Execute<void>(this, 32, &eye_pos);
		return eye_pos;
	}

	void InterpolateShootPosition(Taia_t time, Vector& position);
};

struct CCSPlayer_WaterServices {
	FindField(float, m_flWaterJumpTime, xs("CCSPlayer_WaterServices"), xs("m_flWaterJumpTime"));
	FindField(Vector, m_vecWaterJumpVel, xs("CCSPlayer_WaterServices"), xs("m_vecWaterJumpVel"));
	FindField(float, m_flSwimSoundTime, xs("CCSPlayer_WaterServices"), xs("m_flSwimSoundTime"));
};

struct CCSPlayer_CameraServices {
	FindField(Vector, m_vecCsViewPunchAngle, xs("CPlayer_CameraServices"), xs("m_vecCsViewPunchAngle"));
	FindField(int32_t, m_nCsViewPunchAngleTick, xs("CPlayer_CameraServices"), xs("m_nCsViewPunchAngleTick"));
	FindField(float, m_flCsViewPunchAngleTickRatio, xs("CPlayer_CameraServices"), xs("m_flCsViewPunchAngleTickRatio"));
	FindField(Vector, m_vClientScopeInaccuracy, xs("CCSPlayer_CameraServices"), xs("m_vClientScopeInaccuracy"));
	FindField(CBaseHandle, m_hActivePostProcessingVolume, xs("CPlayer_CameraServices"), xs("m_hActivePostProcessingVolume"));
	FindField(uint32_t, m_iFOV, xs("CCSPlayerBase_CameraServices"), xs("m_iFOV"));
};

struct CCSPlayer_AimPunchServices {
	FindField(int32_t, m_predictableBaseTick, xs("CCSPlayer_AimPunchServices"), xs("m_predictableBaseTick"));
	FindField(float, m_predictableBaseTickInterpAmount, xs("CCSPlayer_AimPunchServices"), xs("m_predictableBaseTickInterpAmount"));
	FindField(Vector, m_predictableBaseAngle, xs("CCSPlayer_AimPunchServices"), xs("m_predictableBaseAngle"));
	FindField(Vector, m_predictableBaseAngleVel, xs("CCSPlayer_AimPunchServices"), xs("m_predictableBaseAngleVel"));
	FindField(int32_t, m_unpredictableBaseTick, xs("CCSPlayer_AimPunchServices"), xs("m_unpredictableBaseTick"));
	FindField(Vector, m_unpredictableBaseAngle, xs("CCSPlayer_AimPunchServices"), xs("m_unpredictableBaseAngle"));

	void ComputeAimPunch(Vector& outAngle, Taia_t time, bool reduce_punch) {
		using recoil_fn = void(__fastcall*)(CCSPlayer_AimPunchServices*, Vector*, const Taia_t*, bool);
		static auto get_recoil = Memory->Modules.Client.Find(xs("4C 8B DC 49 89 5B 08 49 89 6B 10 49 89 73 18 57 48 83 EC 70 48 8B D9 44 88 4C 24 38 4D 89 43 B8")).As<recoil_fn>();
		get_recoil(this, &outAngle, &time, reduce_punch);
	}
};

struct CCSPlayer_ItemServices {
	FindField(bool, m_bHasHelmet, xs("CCSPlayer_ItemServices"), xs("m_bHasHelmet"));
	FindField(bool, m_bHasDefuser, xs("CCSPlayer_ItemServices"), xs("m_bHasDefuser"));
};

struct CCSPlayer_MovementServices;

struct C_CSPlayerPawn : C_BaseEntity {
	FindField(int32_t, m_ArmorValue, xs("C_CSPlayerPawn"), xs("m_ArmorValue"));
	FindField(bool, m_bIsWalking, xs("C_CSPlayerPawn"), xs("m_bIsWalking"));
	FindField(float, m_flFlashDuration, xs("C_CSPlayerPawnBase"), xs("m_flFlashDuration"));
	FindField(CCSPlayer_MovementServices*, m_pMovementServices, xs("C_BasePlayerPawn"), xs("m_pMovementServices"));
	FindField(CCSPlayer_ItemServices*, m_pItemServices, xs("C_BasePlayerPawn"), xs("m_pItemServices"));
	FindField(CCSPlayer_WeaponServices*, m_pWeaponServices, xs("C_BasePlayerPawn"), xs("m_pWeaponServices"));
	FindField(CCSPlayer_WaterServices*, m_pWaterServices, xs("C_BasePlayerPawn"), xs("m_pWaterServices"));
	FindField(CCSPlayer_CameraServices*, m_pCameraServices, xs("C_BasePlayerPawn"), xs("m_pCameraServices"));
	FindField(CCSPlayer_AimPunchServices*, m_pAimPunchServices, xs("C_CSPlayerPawn"), xs("m_pAimPunchServices"));
	FindField(int, m_nLastExecutedCommandNumber, xs("C_BasePlayerPawn"), xs("m_nLastExecutedCommandNumber"));
	FindField(int, m_nLastExecutedCommandTick, xs("C_BasePlayerPawn"), xs("m_nLastExecutedCommandTick"));
	FindField(Vector, m_angEyeAngles, xs("C_CSPlayerPawn"), xs("m_angEyeAngles"));
	FindField(bool, m_bGunGameImmunity, xs("C_CSPlayerPawn"), xs("m_bGunGameImmunity"));
	FindField(int, m_nSurvivalTeam, xs("C_CSPlayerPawnBase"), xs("m_nSurvivalTeam"));
	FindField(bool, m_bIsScoped, xs("C_CSPlayerPawn"), xs("m_bIsScoped"));
	FindField(int, m_iShotsFired, xs("C_CSPlayerPawn"), xs("m_iShotsFired"));
	FindField(bool, m_bWaitForNoAttack, xs("C_CSPlayerPawn"), xs("m_bWaitForNoAttack"));
	FindField(bool, m_bResumeZoom, xs("C_CSPlayerPawn"), xs("m_bResumeZoom"));
	FindField(float, m_flVelocityModifier, xs("C_CSPlayerPawn"), xs("m_flVelocityModifier"));
	FindField(Vector, m_vOldOrigin, xs("C_BasePlayerPawn"), xs("m_vOldOrigin"));
	FindField(float, m_lastLandTime, xs("C_CSPlayerPawn"), xs("m_lastLandTime"));
	FindField(bool, m_bIsDefusing, xs("C_CSPlayerPawn"), xs("m_bIsDefusing"));
	FindField(int, m_iPlayerState, xs("C_CSPlayerPawnBase"), xs("m_iPlayerState"));
	FindField(float, m_flLastSpawnTimeIndex, xs("C_CSPlayerPawnBase"), xs("m_flLastSpawnTimeIndex"));
	FindField(bool, m_bNeedToReApplyGloves, xs("C_CSPlayerPawn"), xs("m_bNeedToReApplyGloves"));
	FindField(Vector, AnglePrevious, xs("C_BasePlayerPawn"), xs("v_anglePrevious"));
	FindField(Vector, m_vecPredictionError, xs("C_BasePlayerPawn"), xs("m_vecPredictionError"));
	FindField(float, m_flPredictionErrorTime, xs("C_BasePlayerPawn"), xs("m_flPredictionErrorTime"));
	FindField(Vector, VAngle, xs("C_BasePlayerPawn"), xs("v_angle"));
	FindField(C_EconItemView, m_EconGloves, xs("C_CSPlayerPawn"), xs("m_EconGloves"));
	FindField(bool, m_bIsBuyMenuOpen, xs("C_CSPlayerPawn"), xs("m_bIsBuyMenuOpen"));
	FindField(bool, m_bInBuyZone, xs("C_CSPlayerPawn"), xs("m_bInBuyZone"));
	FindField(CBaseHandle, m_hHudModelArms, xs("C_CSPlayerPawn"), xs("m_hHudModelArms"));
	FindField(CBaseHandle, m_hController, xs("C_BasePlayerPawn"), xs("m_hController"));
	FindField(float, m_grenadeParameterStashTime, xs("C_CSPlayerPawn"), xs("m_grenadeParameterStashTime"));
	FindField(bool, m_bGrenadeParametersStashed, xs("C_CSPlayerPawn"), xs("m_bGrenadeParametersStashed"));
	FindField(Vector, m_angStashedShootAngles, xs("C_CSPlayerPawn"), xs("m_angStashedShootAngles"));
	FindField(Vector, m_vecStashedGrenadeThrowPosition, xs("C_CSPlayerPawn"), xs("m_vecStashedGrenadeThrowPosition"));
	FindField(Vector, m_vecStashedGrenadeThrowPawnCenter, xs("C_CSPlayerPawn"), xs("m_vecStashedGrenadeThrowPawnCenter"));
	FindField(Vector, m_vecStashedVelocity, xs("C_CSPlayerPawn"), xs("m_vecStashedVelocity"));
	FindField(float, m_flFOVSensitivityAdjust, xs("C_BasePlayerPawn"), xs("m_flFOVSensitivityAdjust"));
	FindOffset(uint64_t, GetTraceMask, 0xD48);
	FindOffset(uint64_t, MovementTraceMask, 0xD50);
	FindOffset(CHitboxTransformHistory, m_HitboxTransformHistory, 0x1FD0);

	bool Alive() { return this->m_iHealth() > 0; }
	bool Enemy(C_CSPlayerPawn* other);
	Vector GetEyePosition() {
		Vector eye_pos{};
		Memory->Vtable.Execute<void>(this, 173, &eye_pos);
		return eye_pos;
	}
	Vector GetShootPosition() { return this->m_pGameSceneNode()->m_vecOrigin() + this->m_vecViewOffset(); }

	C_CSWeaponBase* GetActiveWeapon();
};

struct C_CSGameRules {
	FindField(float, m_fRoundStartTime, xs("C_CSGameRules"), xs("m_fRoundStartTime"));
	FindField(bool, m_bTeamIntroPeriod, xs("C_CSGameRules"), xs("m_bTeamIntroPeriod"));
	FindField(bool, m_bIsValveDS, xs("C_CSGameRules"), xs("m_bIsValveDS"));
	FindField(bool, m_bFreezePeriod, xs("C_CSGameRules"), xs("m_bFreezePeriod"));
	FindField(bool, m_bWarmupPeriod, xs("C_CSGameRules"), xs("m_bWarmupPeriod"));
	FindField(bool, m_bTCantBuy, xs("C_CSGameRules"), xs("m_bTCantBuy"));
	FindField(bool, m_bCTCantBuy, xs("C_CSGameRules"), xs("m_bCTCantBuy"));
};

struct C_CSGameRulesProxy {
	C_CSGameRules* GetGameRules() { return *reinterpret_cast<C_CSGameRules**>(reinterpret_cast<uintptr_t>(this) + 0x600); }
};

struct C_PlantedC4 : public C_BaseEntity {
	FindField(bool, m_bBombTicking, xs("C_PlantedC4"), xs("m_bBombTicking"));
	FindField(int, m_nBombSite, xs("C_PlantedC4"), xs("m_nBombSite"));
	FindField(float, m_flC4Blow, xs("C_PlantedC4"), xs("m_flC4Blow"));
	FindField(bool, m_bCannotBeDefused, xs("C_PlantedC4"), xs("m_bCannotBeDefused"));
	FindField(bool, m_bHasExploded, xs("C_PlantedC4"), xs("m_bHasExploded"));
	FindField(float, m_flTimerLength, xs("C_PlantedC4"), xs("m_flTimerLength"));
	FindField(bool, m_bBeingDefused, xs("C_PlantedC4"), xs("m_bBeingDefused"));
	FindField(float, m_flDefuseLength, xs("C_PlantedC4"), xs("m_flDefuseLength"));
	FindField(float, m_flDefuseCountDown, xs("C_PlantedC4"), xs("m_flDefuseCountDown"));
	FindField(bool, m_bBombDefused, xs("C_PlantedC4"), xs("m_bBombDefused"));
};
