#pragma once

#include <cstddef>
#include <cstdint>

enum ECvarFlags : uint32_t {
	FCVAR_NONE = 0,
	FCVAR_UNREGISTERED = 1 << 0,
	FCVAR_DEVELOPMENTONLY = 1 << 1,
	FCVAR_GAMEDLL = 1 << 2,
	FCVAR_CLIENTDLL = 1 << 3,
	FCVAR_HIDDEN = 1 << 4,
	FCVAR_PROTECTED = 1 << 5,
	FCVAR_SPONLY = 1 << 6,
	FCVAR_ARCHIVE = 1 << 7,
	FCVAR_NOTIFY = 1 << 8,
	FCVAR_USERINFO = 1 << 9,
	FCVAR_PRINTABLEONLY = 1 << 10,
	FCVAR_UNLOGGED = 1 << 11,
	FCVAR_NEVER_AS_STRING = 1 << 12,
	FCVAR_REPLICATED = 1 << 13,
	FCVAR_CHEAT = 1 << 14,
	FCVAR_DEMO = 1 << 16,
	FCVAR_DONTRECORD = 1 << 17,
	FCVAR_RELOAD_MATERIALS = 1 << 20,
	FCVAR_RELOAD_TEXTURES = 1 << 21,
	FCVAR_NOT_CONNECTED = 1 << 22,
	FCVAR_MATERIAL_SYSTETHREAD = 1 << 23,
	FCVAR_ARCHIVE_XBOX = 1 << 24,
	FCVAR_ACCESSIBLE_FROTHREADS = 1 << 25,
	FCVAR_SERVER_CAN_EXECUTE = 1 << 28,
	FCVAR_SERVER_CANNOT_QUERY = 1 << 29,
	FCVAR_CLIENTCMD_CAN_EXECUTE = 1 << 30,
	FCVAR_MATERIAL_THREAD_MASK = FCVAR_RELOAD_MATERIALS | FCVAR_RELOAD_TEXTURES | FCVAR_MATERIAL_SYSTETHREAD
};

union CConVarValue {
	bool i1;
	int16_t i16;
	uint16_t u16;
	int32_t i32;
	uint32_t u32;
	int64_t i64;
	uint64_t u64;
	float fl;
	double db;
	const char* sz;
};

struct CConVar {
	const char* name;
	CConVar* next;
	uint8_t padding[0x10];
	const char* description;
	uint32_t type;
	uint32_t registered;
	uint32_t flags;
	uint8_t padding2[0x24];
	CConVarValue value;

	bool GetBool() const;
	int GetInt() const;
	float GetFloat() const;
	const char* GetString() const;

	bool SetBool(bool value);
	int SetInt(int value);
	float SetFloat(float value);
	const char* SetString(const char* value);
};

struct CConVarEntry {
	CConVar* convar;
	uint16_t prev;
	uint16_t next;
};

struct CCvar {
	uint8_t padding[0x50];
	CConVarEntry* entries;

	CConVar* FindByName(const char* name) const;
	CConVar* FindByHash(uint64_t hash) const;
};

static_assert(offsetof(CConVar, value) == 0x58);
static_assert(sizeof(CConVarEntry) == 0x10);
static_assert(offsetof(CCvar, entries) == 0x50);

#define cvar(name) static auto name = Cvar->FindByHash(FnvHash(#name))
