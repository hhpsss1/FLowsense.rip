#pragma once

struct CEntitySystem {
	Address_t GetBaseEntity(int i) {
		static auto fn = Memory->Modules.Client.Find(xs("4C 8D 49 ? 81 FA")).As<void*(__fastcall*)(CEntitySystem*, int)>();
		return fn(this, i);
	}

    Address_t GetLocalController() {
        static auto fn = Memory->Modules.Client.Find(xs("E8 ? ? ? ? 48 85 C0 74 ? 8B CF")).Rel(1, 5).As<void*(__fastcall*)(int)>();
        return fn(0);
    }

    Address_t GetLocalPawn() {
        static auto fn = Memory->Modules.Client.Find(xs("E8 ? ? ? ? 48 85 C0 74 ? B8 ? ? ? ? 48 C7 44 24")).Rel(1, 5).As<void*(__fastcall*)(int)>();
        return fn(0);
    }
};