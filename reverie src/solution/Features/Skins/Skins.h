#pragma once

struct CVpkHeader {
	uint32_t msignature;
	uint32_t version;
	uint32_t tree_size;
	uint32_t file_data_section_size;
	uint32_t archive_md5_section_size;
	uint32_t other_md5_section_size;
	uint32_t signature_section_size;
};

struct CVpkEntry {
	uint32_t crc;
	uint16_t preload_bytes;
	uint16_t archive_index;
	uint32_t entry_offset;
	uint32_t entry_length;
	uint16_t terminator;
};

#pragma pack(pop)

enum EItemType {
	ITEM_TYPE_NONE,
	ITEM_TYPE_WEAPON,
	ITEM_TYPE_KNIFE,
	ITEM_TYPE_GLOVE,
	ITEM_TYPE_AGENT,
};

enum EItemCategory {
	ITEM_CAT_ALL = 0,
	ITEM_CAT_RIFLES,
	ITEM_CAT_PISTOLS,
	ITEM_CAT_SMGS,
	ITEM_CAT_HEAVY,
	ITEM_CAT_SNIPERS,
	ITEM_CAT_KNIVES,
	ITEM_CAT_GLOVES
};

enum EWeaponCategory {
	WEAPON_CAT_ALL = 0,
	WEAPON_CAT_RIFLES,
	WEAPON_CAT_PISTOLS,
	WEAPON_CAT_SMGS,
	WEAPON_CAT_HEAVY,
	WEAPON_CAT_SNIPERS
};

static EWeaponCategory GetWeaponCategory(uint16_t defindex) {
	switch (defindex) {
	case 1: case 2: case 3: case 4: case 30: case 32: case 36: case 61: case 63: case 64:
		return WEAPON_CAT_PISTOLS;
	case 7: case 8: case 10: case 13: case 16: case 39: case 60:
		return WEAPON_CAT_RIFLES;
	case 9: case 11: case 38: case 40:
		return WEAPON_CAT_SNIPERS;
	case 17: case 19: case 23: case 24: case 26: case 33: case 34:
		return WEAPON_CAT_SMGS;
	case 14: case 25: case 27: case 28: case 29: case 35:
		return WEAPON_CAT_HEAVY;
	default:
		return WEAPON_CAT_RIFLES;
	}
}

inline bool IsKnifeDefindex(uint16_t defindex) {
	if (defindex == 42 || defindex == 59 || defindex == 80 || (defindex >= 500 && defindex <= 526))
		return true;

	return false;
}

inline bool IsSkinnableWeaponDefindex(uint16_t defindex) {
	switch (defindex) {
	case 1: case 2: case 3: case 4: case 7: case 8: case 9: case 10: case 11:
	case 13: case 14: case 16: case 17: case 19: case 23: case 24: case 25:
	case 26: case 27: case 28: case 29: case 30: case 32: case 33: case 34:
	case 35: case 36: case 38: case 39: case 40: case 60: case 61: case 63: case 64:
		return true;
	default:
		return false;
	}
}

struct CAppliedSkinCache {
	int paint_kit_id = -1;
	float wear = -1.f;
	int seed = -1;
};

inline std::unordered_map<int, CAppliedSkinCache> applied_paint_kits{};

struct CSkins {
	struct ItemPaintKit_t {
		int id;
		int rarity;
		std::string name;
		std::string icon_path;
		bool is_legacy_model;
	};

	struct Item_t {
		EItemType type;
		uint16_t def_index;
		std::string name;
		std::string icon_path;
		std::string model_path;
		int rarity;

		std::vector<ItemPaintKit_t> paint_kits;

		CMisc::Panorama_t::IconData_t GetImage(ItemPaintKit_t kit = {});
	};

	struct SkinSetting_t {
		int paint = 0;
		float wear = 0.01f;
		int seed = 1;
	};

	bool should_update = false;
	uint16_t knif_def[2] = { 0, 0 };
	uint16_t glove_def[2] = { 0, 0 };
	uint16_t agent_def[2] = { 0, 0 };
	std::unordered_map<uint16_t, SkinSetting_t> skin_data[2];

	std::vector<Item_t> weapon_items;
	std::vector<Item_t> knife_items;
	std::vector<Item_t> gloves_items;
	std::vector<Item_t> agent_items;
	std::vector<Item_t> all_items;
	std::unordered_set<std::string> econ_images;

	void Init();
	void Clear();
	bool ParseEconImages();
	void ProcessAgent();
	void ProcessWeapons();
	void ProcessKnife();
	void ProcessGloves();

	C_BaseEntity* GetHudWeapon(C_CSPlayerPawn* pawn, C_CSWeaponBase* weapon);

	int GetItemCategory(const Item_t& item);
	const Item_t* FindItem(uint16_t defindex);

	void OnFrameStage();
};

inline CSkins* Skins = new CSkins();