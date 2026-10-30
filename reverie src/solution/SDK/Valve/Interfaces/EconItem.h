#pragma once

struct CPaintKit {
	int id;
	const char* name;
	const char* description;
	const char* description_tag;
	char pad[0x24];
	int rarity;

	bool IsLegacyModel() { return *reinterpret_cast<bool*>(uintptr_t(this) + 0xAE); }
};

struct CEconItemDefinition {
	FindOffset(uint16_t, DefinitionIndex, 0x10);
	FindOffset(uint8_t, Rarity, 0x42);
	FindOffset(const char*, BaseName, 0x70);
	FindOffset(const char*, TypeName, 0x80);
	FindOffset(const char*, ModelName, 0x148);
	FindOffset(int32_t, MaxStickersCount, 0x168);
	FindOffset(const char*, IconName, 0x230);
	FindOffset(const char*, ItemName, 0x260);

	bool IsWeapon() { return this->MaxStickersCount() >= 4; }
	bool IsKnife() { return FnvHash("#CSGO_Type_Knife") == FnvHash(this->TypeName()); }
	bool IsGloves() { return FnvHash("#Type_Hands") == FnvHash(this->TypeName()); }
	bool IsAgent() { return FnvHash("#Type_CustomPlayer") == FnvHash(this->TypeName()); }
};

struct CEconItemSchema {
	CUtlMap<int, CEconItemDefinition*>& GetItemDefinitionMap() { return *reinterpret_cast<CUtlMap<int, CEconItemDefinition*>*>(reinterpret_cast<uintptr_t>(this) + 0x128); };
	CUtlMap<int, CPaintKit*>& GetPaintKitsMap() { return *reinterpret_cast<CUtlMap<int, CPaintKit*>*>(reinterpret_cast<uintptr_t>(this) + 0x2F0); };
};

struct CEconItemSystem {
	void* pointer;
	CEconItemSchema* schema;
};