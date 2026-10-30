#pragma once

enum EStringConvertErrorPolicy {
    STRING_CONVERT_FLAG_SKIP = 1,
    STRING_CONVERT_FLAG_FAIL = 2,
    STRING_CONVERT_FLAG_ASSERT = 4,
    STRING_CONVERT_REPLACE = 0,
    STRING_CONVERT_SKIP = STRING_CONVERT_FLAG_SKIP,
    STRING_CONVERT_FAIL = STRING_CONVERT_FLAG_FAIL,
    STRING_CONVERT_ASSERT_REPLACE = STRING_CONVERT_FLAG_ASSERT + STRING_CONVERT_REPLACE,
    STRING_CONVERT_ASSERT_SKIP = STRING_CONVERT_FLAG_ASSERT + STRING_CONVERT_SKIP,
    STRING_CONVERT_ASSERT_FAIL = STRING_CONVERT_FLAG_ASSERT + STRING_CONVERT_FAIL,
};

struct CBufferString {
    enum EAllocationOption {
        UNK1 = -1,
        UNK2 = 0,
        UNK3 = (1 << 1),
        UNK4 = (1 << 8),
        UNK5 = (1 << 9)
    };

    enum EAllocationFlags {
        LENGTH_MASK = (1 << 30) - 1,
        FLAGS_MASK = ~LENGTH_MASK,
        OVERFLOWED_MARKER = (1 << 30),
        FREE_HEAP_MARKER = (1 << 31),
        STACK_ALLOCATED_MARKER = (1 << 30),
        ALLOW_HEAP_ALLOCATION = (1 << 31)
    };
public:
    CBufferString(bool allow_heap_allocation = true) :
        length(0),
        allocated_size((allow_heap_allocation* ALLOW_HEAP_ALLOCATION) | STACK_ALLOCATED_MARKER | sizeof(string)),
        string_ptr(nullptr) {
    }

    CBufferString(const char* string, bool allow_heap_allocation = true) :
        CBufferString(allow_heap_allocation) {
        this->Insert(0, string);
    }
public:
    CBufferString(const CBufferString& other) : CBufferString() { *this = other; }

    CBufferString(const char* buf, uint64_t ex) : CBufferString() { this->FixupResourceName(buf, ex); }
    CBufferString(const char* buf, int ex) : CBufferString() { this->FixupResourceName(buf, static_cast<uint64_t>(ex)); }

    void FixupResourceName(const char* resource, uint64_t extension) {
        using Fn = void(__fastcall*)(CBufferString*, const char*, uint64_t);
        static auto fixup_fn = Memory->Modules.Resourcesystem.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 8B 41")).As<Fn>();
        fixup_fn(this, resource, extension);
    }

    ~CBufferString() { this->Purge(); }

    void SetHeapAllocationState(bool state) {
        if (state)
            this->allocated_size |= ALLOW_HEAP_ALLOCATION;
        else
            this->allocated_size &= ~ALLOW_HEAP_ALLOCATION;
    }

    int AllocatedNum() const { return this->allocated_size & LENGTH_MASK; }
    int Length() const { return this->length & LENGTH_MASK; }

    bool CanHeapAllocate() const { return (this->allocated_size & ALLOW_HEAP_ALLOCATION) != 0; }
    bool IsStackAllocated() const { return (this->allocated_size & STACK_ALLOCATED_MARKER) != 0; }
    bool ShouldFreeMemory() const { return (this->length & FREE_HEAP_MARKER) != 0; }
    bool IsOverflowed() const { return (this->length & OVERFLOWED_MARKER) != 0; }

    bool IsInputStringUnsafe(const char* data) const { return ((void*)data >= this && (void*)data < &this[1]) || (!this->IsAllocationEmpty() && data >= this->Base() && data < (this->Base() + this->AllocatedNum())); }

    bool IsAllocationEmpty() const { return this->AllocatedNum() == 0; }
protected:
    char* Base() { return this->IsStackAllocated() ? this->string : (!this->IsAllocationEmpty() ? this->string_ptr : nullptr); }
    const char* Base() const { return const_cast<CBufferString*>(this)->Base(); }
public:
    void Clear() {
        if (!this->IsAllocationEmpty())
            this->Base()[0] = '\0';

        this->length &= ~LENGTH_MASK;
    }
public:
    const char* Insert(int index, const char* buf, int count = -1, bool ignore_alignment = false) {
        using Fn = const char* (__fastcall*)(void*, int, const char*, int, bool);
        static auto insert_fn = Memory->Modules.Tier0.Find(xs("40 53 55 56 57 48 83 EC ? 48 63 EA")).As<Fn>();
        return insert_fn(this, index, buf, count, ignore_alignment);
    }

    void Purge(int allocated_bytes_to_preserve = 0) {
        using Fn = void(__fastcall*)(void*, int);
        static auto purge_fn = Memory->Modules.Tier0.Find(xs("48 89 5C 24 ? 57 48 83 EC ? 8B 41 ? 8D 7A")).As<Fn>();
        purge_fn(this, allocated_bytes_to_preserve);
    }
private:
    int length;
    int allocated_size;

    union {
        char* string_ptr;
        char string[8];
    };
};