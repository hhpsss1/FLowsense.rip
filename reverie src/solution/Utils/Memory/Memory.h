#pragma once
#include <libloaderapi.h>

struct Address_t {
    Address_t() : pointer(0x0) {}

    template <typename t = uintptr_t>
    Address_t(t p) {
        this->pointer = reinterpret_cast<uintptr_t>(p);
    }

    Address_t Add(ptrdiff_t offset) {
        this->pointer += offset;
        return *this;
    }

    Address_t Sub(ptrdiff_t offset) {
        this->pointer -= offset;
        return *this;
    }

    Address_t Deref() {
        this->pointer = *reinterpret_cast<uintptr_t*>(this->pointer);
        return *this;
    }

    Address_t Rel(int offset, int size) {
        this->pointer += *reinterpret_cast<int32_t*>(this->pointer + offset);
        this->pointer += size;
        return *this;
    }

    Address_t Abs(int pre, int post) {
        this->pointer += pre;
        this->pointer += sizeof(int32_t) + *reinterpret_cast<int32_t*>(this->pointer);
        this->pointer += post;
        return *this;
    }

    template<typename t>
    t As() { return reinterpret_cast<t>(this->pointer); }

    uintptr_t Get() { return this->pointer; }
private:
    uintptr_t pointer;
};

struct Module_t {
    Module_t() = default;
    Module_t(const char* name) { this->pointer = GetModuleHandleA(name); }

    void* Handle() { return this->pointer; }

    static std::vector<int> ToBytes(const std::string_view& pattern);
    Address_t Find(std::string_view pattern);

    template <typename t>
    t FindInterface(const char* name) const {
        using fn = t(*)(const char*, int*);
        const auto func = reinterpret_cast<fn>(GetProcAddress(reinterpret_cast<HMODULE>(this->pointer), xs("CreateInterface")));
        if (!func)
            return nullptr;

        return func(name, nullptr);
    }
private:
    void* pointer;
};

struct Memory_t {
	struct Vtable_t {
        template<typename type>
        [[nodiscard]] type Get(void* class_ptr, unsigned int index) {
            return reinterpret_cast<type>((*static_cast<uintptr_t**>(class_ptr))[index]);
        }

        template<typename type = void, typename... args>
        [[nodiscard]] type Execute(void* class_ptr, unsigned int index, args... arg) {
            auto func = reinterpret_cast<type(__thiscall*)(void*, args...)>((*static_cast<uintptr_t**>(class_ptr))[index]);
            return func(class_ptr, arg...);
        }
	} Vtable;

    struct Modules_t {
        Module_t Client, Server, Engine2, Schema, Tier0, Input, Navsystem, Rendersystem, Localize, Filesystem, Meshsystem, Particles, Panorama, Scenesystem, Materialsystem, Resourcesystem, Animationsystem, Soundsystem, Worldrenderer;
        
        void Fill();
        void Clear();
    } Modules;
};

inline Memory_t* Memory = new Memory_t();