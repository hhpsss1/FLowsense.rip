#pragma once

struct CEventHash {
	CEventHash(const char* string) {
		this->name = string;
		this->hash = MM::HashA1Runtime(string, static_cast<int>(std::strlen(string)), 0x31415926);
	}

	uint32_t hash;
	const char* name;
};

struct CGameEvent {
	virtual ~CGameEvent() {};

	const char* GetName() { return Memory->Vtable.Execute<const char*>(this, 1); }

	bool GetBool(const char* event_name, bool def = false) {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<bool>(this, 6, hash, def);
	}

	int GetInt(const char* event_name, int def = 0) {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<int>(this, 7, hash, def);
	}

	uint64_t GetUint64(const char* event_name, uint64_t def = 0) {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<uint64_t>(this, 8, hash, def);
	}

	float GetFloat(const char* event_name, float def = 0.0f) {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<float>(this, 9, hash, def);
	}

	const char* GetString(const char* event_name, const char* def = "") {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<const char*>(this, 10, hash, def);
	}

	const void* GetPointer(const char* event_name) {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<const void*>(this, 11, hash);
	}

	int GetPlayerIndex(const char* event_name) {
		const CEventHash hash(event_name);
		int index{};
		Memory->Vtable.Execute<void>(this, 14, hash, &index);
		return index;
	}

	CCSPlayerController* GetPlayerController(const char* event_name) {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<CCSPlayerController*>(this, 16, hash);
	}

	C_CSPlayerPawn* GetPlayerPawn(const char* event_name) {
		const CEventHash hash(event_name);
		return Memory->Vtable.Execute<C_CSPlayerPawn*>(this, 17, hash);
	}

	void SetBool(const char* event_name, bool value) {
		const CEventHash hash(event_name);
		Memory->Vtable.Execute<void>(this, 20, hash, value);
	}

	void SetInt(const char* event_name, int value) {
		const CEventHash hash(event_name);
		Memory->Vtable.Execute<void>(this, 21, hash, value);
	}

	void SetUint64(const char* event_name, uint64_t value) {
		const CEventHash hash(event_name);
		Memory->Vtable.Execute<void>(this, 22, hash, value);
	}

	void SetFloat(const char* event_name, float value) {
		const CEventHash hash(event_name);
		Memory->Vtable.Execute<void>(this, 23, hash, value);
	}

	void SetString(const char* event_name, const char* value) {
		const CEventHash hash(event_name);
		Memory->Vtable.Execute<void>(this, 24, hash, value);
	}

	void SetPtr(const char* event_name, const void* value) {
		const CEventHash hash(event_name);
		Memory->Vtable.Execute<void>(this, 25, hash, value);
	}
private:
	char pad[0x60];
};

struct CGameEventListener2 {
	virtual ~CGameEventListener2() {}
	virtual void FireGameEvent(CGameEvent* event) = 0;
	virtual int GetEventDebugID() { return this->debug_id; }
private:
	int debug_id;
};

struct CGameEventListener : CGameEventListener2 {
	void Init(const std::deque<const char*>& events);
	void Reset();
	virtual void FireGameEvent(CGameEvent* event) override;
	virtual int GetEventDebugID() override { return 42; }
};

inline CGameEventListener* Listener = new CGameEventListener();

struct CGameEventManager {
	virtual ~CGameEventManager() = 0;

	int LoadEventsProfile(const char* file_name) { return Memory->Vtable.Execute<int>(this, 1, file_name); }
	void Reset() { Memory->Vtable.Execute<void>(this, 2); }
	bool AddListener(CGameEventListener2* listener, const char* name, bool server_side = false) { return Memory->Vtable.Execute<bool>(this, 3, listener, name, server_side); }
	bool RemoveListener(CGameEventListener2* listener) { return Memory->Vtable.Execute<bool>(this, 5, listener); }
};