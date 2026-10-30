#pragma once

enum EKV3HashKeys : uint64_t {
    kv3_unknown_key_hash = 0x31415926,
    kv3_id_unknown_hash1 = 0x41B818518343427E,
    kv3_id_unknown_hash2 = 0xB5F447C23C0CDF8C
};

struct CMaterialKeyVar {
    uint64_t key;
    const char* name;

    CMaterialKeyVar(uint64_t key, const char* name) : key(key), name(name) {}

    CMaterialKeyVar(const char* n, bool should_find_key = false) :
        name(n) {
        key = should_find_key ? FindKey(n) : 0x0;
    }

    uint64_t FindKey(const char* n) {
        using fn_find_key_var = uint64_t(__fastcall*)(const char*, unsigned int, int);
        static fn_find_key_var find_key_var = Memory->Modules.Particles.Find(xs("48 89 5C 24 ? 57 48 81 EC ? ? ? ? 33 C0 8B DA")).As<fn_find_key_var>();

        return find_key_var(n, 0x12U, 0x31415926);
    }
};

struct CKv3ID {
    const char* name;
    uint64_t hash1;
    uint64_t hash2;
};

struct CKeyValues3 {
    uint64_t key;
    void* value;
    std::byte padding[0x8];
};

namespace KV3 {
    using load_kv3_fn = bool(__fastcall*)(CKeyValues3*, void*, const char*, const CKv3ID*, const char*, int);
    inline load_kv3_fn load_key_value = nullptr;

    inline bool InitLoader() {
        const void* tier0 = Memory->Modules.Tier0.Handle();
        if (tier0 == nullptr)
            return false;

        load_key_value = reinterpret_cast<load_kv3_fn>(GetProcAddress((HMODULE)tier0, xs("?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z")));
        return load_key_value != nullptr;
    }
}