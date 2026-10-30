#pragma once

struct CParticleSnapshot {
	void Draw(int count, void* data) { Memory->Vtable.Execute<void>(this, 1, count, data); }
};

struct CParticleInformation {
	float time, width, alpha;
};

struct CParticleData {
	Vector* position;
	char pad01[0x74];
	float* times;
	char pad02[0x28];
	float* times2;
	char pad03[0x98];
};

struct CParticleName {
	char pad[0x8];
	char** particle_name;
};

struct CParticleCollection {
	char m_pad[0x18];
	CParticleName* name;
};

struct CParticleColor {
	float red, green, blue;
};

struct CBeamInfo {
	unsigned int effect_index = -1;
	Vector* positions = nullptr;
	float* times = nullptr;
	CStrongHandle<CParticleSnapshot> snapshot_particle;
	CParticleData particle_data;
};

struct CParticleSystem {
	void CreateSnapshot(CStrongHandle<CParticleSnapshot>* a1) {
		void* unk = 0;
		Memory->Vtable.Execute<void>(this, 41, a1, &unk);
	}

	void Draw(CStrongHandle<CParticleSnapshot>* a1, int count, void* data) { Memory->Vtable.Execute<void>(this, 43, a1, count, data); }
};

struct CParticleManager {
	int* CreateParticleEffect(unsigned int* index, const char* name) {
		using fn = int* (__fastcall*)(CParticleManager*, unsigned int*, const char*, int, __int64, __int64, __int64, int);
		static fn x = Memory->Modules.Client.Find(xs("E8 ? ? ? ? 8B 08 89 8B ? ? ? ? E8 ? ? ? ? 8B 93")).Rel(1, 5).As<fn>();
		return x(this, index, name, 2, 0ll, 0ll, 0ll, 0);
	}

	bool SetControlPoint(unsigned int effect_index, int flag, void* data) {
		using fn = bool(__fastcall*)(CParticleManager*, unsigned int, int, void*, float);
		static fn x = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? F3 0F 10 1D ? ? ? ? 41 8B F8 8B DA 4C 8D 05")).As<fn>();
		return x(this, effect_index, flag, data, 0.f);
	}

	bool SetEntityBinding(unsigned int effect_index, int flag, void* pawn, int idk, void* idk1, void* pos, int lol, long long ahaha) {
		using fn = bool(__fastcall*)(CParticleManager*, unsigned int, int, void*, int, void*, void*, int, long long);
		static fn x = Memory->Modules.Client.Find(xs("41 54 41 55 41 57 48 81 EC ? ? ? ? 4D 8B F9 45 8B E0")).As<fn>();
		return x(this, effect_index, flag, pawn, idk, idk1, pos, lol, ahaha);
	}

	void SetEffect(unsigned int index, int unk, void* clr, int unk2) {
		using fn_t = void(__fastcall*)(CParticleManager*, unsigned int, int, void*, int);
		static fn_t fn = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? F3 0F 10 1D ? ? ? ? 41 8B F8 8B DA 4C 8D 05")).As<fn_t>();
		fn(this, index, unk, clr, unk2);
	}

	bool InitEffect(int index, unsigned int unk, const CResourceBinding* binding) {
		using fn = bool(__fastcall*)(CParticleManager*, int, unsigned int, const CResourceBinding*);
		static fn x = Memory->Modules.Client.Find(xs("40 56 48 83 EC ? 41 8B F0")).As<fn>();
		return x(this, index, unk, binding);
	}

	void DestroyParticle(int idx, bool a1, bool a2) {
		using func_t = void(__fastcall*)(void*, int, char, char);
		static func_t fn = Memory->Modules.Client.Find(xs("83 FA ? 0F 84 ? ? ? ? 41 54")).As<func_t>();
		return fn(this, idx, a1, a2);
	}

	void ReleaseParticleIndex(int idx) { Memory->Vtable.Execute<void>(this, 3, idx); }
	void Unk2(CStrongHandle<CParticleSnapshot>* a1, int count, void* data) { Memory->Vtable.Execute<void>(this, 42, a1, count, data); }
};