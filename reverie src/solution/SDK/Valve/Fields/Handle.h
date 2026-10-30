#pragma once

#define INVALID_EHANDLE_INDEX 0xFFFFFFFF
#define ENT_ENTRY_MASK 0x7FFF
#define NUSERIAL_NUSHIFT_BITS 15
#define ENT_MAX_NETWORKED_ENTRY 16384

struct CResourceBinding { void* data; };
struct CResourceNameInfo { const char* resource_name_symbol; };

template <class t>
struct CStrongHandle {
	operator t* () const {
		if (this->binding == nullptr)
			return nullptr;

		return static_cast<t*>(this->binding->data);
	}

	t* operator->() const {
		if (this->binding == nullptr)
			return nullptr;

		return static_cast<t*>(this->binding->data);
	}

	const CResourceBinding* binding;
	const CResourceNameInfo* name;
};

struct CBaseHandle {
	CBaseHandle() noexcept : index(INVALID_EHANDLE_INDEX) {}
	CBaseHandle(const int entry, const int serial) noexcept { this->index = entry | (serial << NUSERIAL_NUSHIFT_BITS); }
	CBaseHandle(const int raw_index) noexcept : index(raw_index) {}

	bool operator!=(const CBaseHandle& other) const noexcept { return this->index != other.index; }
	bool operator==(const CBaseHandle& other) const noexcept { return this->index == other.index; }
	bool operator<(const CBaseHandle& other) const noexcept { return this->index < other.index; }

	bool Valid() const noexcept { return this->index != INVALID_EHANDLE_INDEX; }
	int GetEntryIndex() const noexcept { return static_cast<int>(this->index & ENT_ENTRY_MASK); }
	int GetSerialNumber() const noexcept { return static_cast<int>(this->index >> NUSERIAL_NUSHIFT_BITS); }
	inline int ToInt() const { return static_cast<int>(this->index); }
	
	void* GetBaseEntity();

	template <typename t = void>
	t* get() { return reinterpret_cast<t*>(this->GetBaseEntity()); }

	int index;
};