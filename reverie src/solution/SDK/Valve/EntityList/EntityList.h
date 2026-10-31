#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct CEntityInstance;
struct C_BaseEntity;
struct C_CSPlayerPawn;
struct CCSPlayerController;
struct CBaseHandle;
struct C_CSGameRulesProxy;
struct C_PlantedC4;

struct CachedPlayer_t {
	CCSPlayerController* controller = nullptr;
	C_CSPlayerPawn* pawn = nullptr;
	int index = -1;
	int handle = -1;

	bool Fill();
};

struct CachedEntity_t {
	C_BaseEntity* entity = nullptr;
	int handle = -1;
	uint64_t class_hash = 0;
};

struct CEntityList {
	std::array<CachedPlayer_t, 64> players = {};
	std::array<CachedEntity_t, 64> grenades = {};
	std::array<CachedEntity_t, 256> weapons = {};
	size_t player_count = 0;
	size_t grenade_count = 0;
	size_t weapon_count = 0;

	C_CSGameRulesProxy* game_rules_proxy = {};
	C_PlantedC4* planted_c4 = nullptr;

	CachedPlayer_t* Find(C_BaseEntity* entity);
	void OnAdd(CEntityInstance* instance, CBaseHandle handle);
	void OnRemove(CEntityInstance* instance, CBaseHandle handle);
	void ForceUpdate();
	void Clear();
};

extern CEntityList* EntityList;
