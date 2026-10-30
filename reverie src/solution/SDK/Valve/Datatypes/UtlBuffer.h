#pragma once

struct CUtlBuffer {
    char pad[0x80];

    CUtlBuffer(int a1, int size, int a3) {
        using fn_utl_buffer_init = void(__stdcall*)(CUtlBuffer*, int, int, int);
        static fn_utl_buffer_init utl_buffer_init = Memory->Modules.Tier0.Find(xs("40 55 56 41 57 48 83 EC ? 33 ED")).As<fn_utl_buffer_init>();

        utl_buffer_init(this, a1, size, a3);
    }

    CUtlBuffer(void* a1, int size, int a3) {
        using fn_utl_buffer_init = void(__stdcall*)(CUtlBuffer*, void*, int, int);
        static fn_utl_buffer_init utl_buffer_init = Memory->Modules.Tier0.Find(xs("48 89 5C 24 ? 57 48 83 EC ? 33 FF 41 8B C0")).As<fn_utl_buffer_init>();

        utl_buffer_init(this, a1, size, a3);
    }

    void PutString(const char* string) {
        using fn_utl_buffer_put_string = void(__stdcall*)(CUtlBuffer*, const char*);
        static fn_utl_buffer_put_string utl_buffer_put_string = Memory->Modules.Tier0.Find(xs("48 89 5C 24 ? 57 48 83 EC ? 0F B6 41 ? 48 8B FA")).As<fn_utl_buffer_put_string>();

        utl_buffer_put_string(this, string);
    }

    void EnsureCapacity(int size) {
        using fn_utl_buffer_ensure_capacity = void(__stdcall*)(CUtlBuffer*, int);
        static fn_utl_buffer_ensure_capacity utl_buffer_ensure_capacity = Memory->Modules.Tier0.Find(xs("48 89 74 24 ? 57 48 83 EC ? 8B 41 ? 8D 72")).As<fn_utl_buffer_ensure_capacity>();

        utl_buffer_ensure_capacity(this, size);
    }
};