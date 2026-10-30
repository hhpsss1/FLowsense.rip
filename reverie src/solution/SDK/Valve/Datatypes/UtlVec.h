#pragma once

#include <cstring>

template<class t>
struct CUtlVector {
    int size;
    t* elements;
    unsigned int max_capacity;
    int flags;

    using reference = std::conditional_t<std::is_void_v<t>, void, t>;
    using const_reference = std::conditional_t<std::is_void_v<t>, void, const t>;

    reference operator[](int i) { return this->elements[i]; }
    const_reference operator[](int i) const { return this->elements[i]; }

    int AddToTail() {
        if (this->size == static_cast<int>(this->max_capacity) && !(this->flags & 0x40000000)) {
            static HMODULE tier0 = reinterpret_cast<HMODULE>(Memory->Modules.Tier0.Handle());
            static auto calc_new_capacity = reinterpret_cast<int(__fastcall*)(int, int, int, int)>(GetProcAddress(tier0, xs("UtlVectorMemory_CalcNewAllocationCount")));
            static auto allocate = reinterpret_cast<void*(__fastcall*)(void*, bool, size_t, size_t)>(GetProcAddress(tier0, xs("UtlVectorMemory_Alloc")));
            static auto failed_allocation = reinterpret_cast<void(__fastcall*)(int, int)>(GetProcAddress(tier0, xs("UtlVectorMemory_FailedAllocation")));

            int capacity = static_cast<int>(this->max_capacity);
            if (capacity == 0x7FFFFFFF)
                failed_allocation(capacity, 1);

            int required_capacity = capacity + 1;
            int new_capacity = calc_new_capacity(capacity, this->flags & 0x3FFFFFFF, required_capacity, sizeof(t));
            if (new_capacity < required_capacity) {
                if (new_capacity || required_capacity > -1) {
                    do {
                        new_capacity = (new_capacity + required_capacity) / 2;
                    } while (new_capacity < required_capacity);
                }
                else {
                    new_capacity = -1;
                }
            }

            this->elements = static_cast<t*>(allocate(this->elements, !(this->flags & 0xC0000000), sizeof(t) * static_cast<size_t>(new_capacity), sizeof(t) * static_cast<size_t>(capacity)));
            if (this->flags & 0xC0000000)
                this->flags &= 0x3FFFFFFF;
            this->max_capacity = new_capacity;
        }

        return this->size++;
    }

    template<typename u = t>
    int AddToTail(const u& value) requires (std::is_same_v<u, t> && !std::is_void_v<u>) {
        int index = this->AddToTail();
        if constexpr (std::is_trivially_copyable_v<t>)
            std::memcpy(&this->elements[index], &value, sizeof(t));
        else
            this->elements[index] = value;
        return index;
    }
};

template <typename t>
struct CUtlVectorTemple {
    CUtlVectorTemple() {
        this->size = 0;
        this->elements = nullptr;
    }

    t& Element(int i) { return this->elements[i]; }
    const t& Element(int i) const { return this->elements[i]; }
    t& operator[](int i) { return this->elements[i]; }
    int Count() const { return this->size; }
    void Clear() { this->size = 0; };

    int size;
    t* elements;
};

template<class t>
struct CUtlVector2 {
    t Begin() const { return this->elements; }
    t End() const { return this->elements + this->size; }
    bool Empty() const { return this->size == 0; }

    bool Exists(t val) const {
        for (const auto& it : *this)
            if (it == val)
                return true;

        return false;
    }

    int size;
    char pad002[4];
    t* elements;
    char pad003[8];
};

template<class t>
struct CUtlUnknown {
    struct Node_t {
        char pad004[0x10];
        t* value;
        int key;
    };

    int count;
    CUtlUnknown::Node_t* nodes;
    char pad003[8];
};
