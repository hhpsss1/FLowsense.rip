#pragma once

#include <cstdint>

enum : uint64_t {
    IN_ATTACK = 1 << 0,
    IN_JUMP = 1 << 1,
    IN_DUCK = 1 << 2,
    IN_FORWARD = 1 << 3,
    IN_BACK = 1 << 4,
    IN_USE = 1 << 5,
    IN_TURNLEFT = 1 << 7,
    IN_TURNRIGHT = 1 << 8,
    IN_MOVELEFT = 1 << 9,
    IN_MOVERIGHT = 1 << 10,
    IN_ATTACK2 = 1 << 11,
    IN_RELOAD = 1 << 13,
    IN_SPRINT = 1 << 16,
    IN_SHOWSCORES = 1ULL << 33,
    IN_ZOOM = 1ULL << 34,
    IN_LOOKATWEAPON = 1ULL << 35
};

enum : int {
    FL_ONGROUND = (1 << 0), // entity is at rest / on the ground
    FL_DUCKING = (1 << 1), // player is fully crouched/uncrouched
    FL_WATERJUMP = (1 << 2), // player is jumping out of water
    FL_ONTRAIN = (1 << 3), // player is controlling a train, so movement commands should be ignored on client during prediction
    FL_INRAIN = (1 << 4), // entity is standing in rain
    FL_FROZEN = (1 << 5), // player is frozen for 3rd-person camera
    FL_ATCONTROLS = (1 << 6), // player can't move, but keeps mKey inputs for controlling another entity
    FL_CLIENT = (1 << 7), // entity is a client (player)
    FL_FAKECLIENT = (1 << 8), // entity is a fake client, simulated server side; don't send network messages to them
    FL_INWATER = (1 << 9), // entity is in water
    FL_FLY = (1 << 10),
    FL_SWIM = (1 << 11),
    FL_CONVEYOR = (1 << 12),
    FL_NPC = (1 << 13),
    FL_GODMODE = (1 << 14),
    FL_NOTARGET = (1 << 15),
    FL_AIMTARGET = (1 << 16),
    FL_PARTIALGROUND = (1 << 17), // entity is standing on a place where not all corners are valid
    FL_STATICPROP = (1 << 18), // entity is a static property
    FL_GRAPHED = (1 << 19),
    FL_GRENADE = (1 << 20),
    FL_STEPMOVEMENT = (1 << 21),
    FL_DONTTOUCH = (1 << 22),
    FL_BASEVELOCITY = (1 << 23), // entity have applied base velocity this frame
    FL_WORLDBRUSH = (1 << 24), // entity is not moveable/removeable brush (part of the world, but represented as an entity for transparency or something)
    FL_OBJECT = (1 << 25),
    FL_KILLME = (1 << 26), // entity is marked for death and will be freed by the game
    FL_ONFIRE = (1 << 27),
    FL_DISSOLVING = (1 << 28),
    FL_TRANSRAGDOLL = (1 << 29), // entity is turning into client-side ragdoll
    FL_UNBLOCKABLE_BY_PLAYER = (1 << 30)
};

enum e_move_type : uint8_t {
    MOVETYPE_NONE = 0,
    MOVETYPE_OBSOLETE,
    MOVETYPE_WALK,
    MOVETYPE_FLY,
    MOVETYPE_FLYGRAVITY,
    MOVETYPE_VPHYSICS,
    MOVETYPE_PUSH,
    MOVETYPE_NOCLIP,
    MOVETYPE_OBSERVER,
    MOVETYPE_LADDER,
    MOVETYPE_CUSTOM,
    MOVETYPE_LAST,
    MOVETYPE_INVALID,
    MOVETYPE_MAX_BITS = 5
};

enum EWeapon : uint16_t {
    WEAPON_DEAGLE = 1,
    WEAPON_DUAL_BERETTAS = 2,
    WEAPON_FIVE_SEVEN = 3,
    WEAPON_GLOCK = 4,
    WEAPON_AK47 = 7,
    WEAPON_AUG = 8,
    WEAPON_AWP = 9,
    WEAPON_FAMAS = 10,
    WEAPON_G3SG1 = 11,
    WEAPON_GALIL = 13,
    WEAPON_M249 = 14,
    WEAPON_M4A4 = 16,
    WEAPON_MAC10 = 17,
    WEAPON_P90 = 19,
    WEAPON_UMP = 24,
    WEAPON_MP5SD = 23,
    WEAPON_XM1024 = 25,
    WEAPON_BIZON = 26,
    WEAPON_MAG7 = 27,
    WEAPON_NEGEV = 28,
    WEAPON_SAWEDOFF = 29,
    WEAPON_TEC9 = 30,
    WEAPON_TASER = 31,
    WEAPON_HKP2000 = 32,
    WEAPON_MP7 = 33,
    WEAPON_MP9 = 34,
    WEAPON_NOVA = 35,
    WEAPON_P250 = 36,
    WEAPON_SCAR20 = 38,
    WEAPON_SG553 = 39,
    WEAPON_SSG08 = 40,
    WEAPON_KNIFE_CT = 42,
    WEAPON_FLASHBANG = 43,
    WEAPON_HEGRENADE = 44,
    WEAPON_SMOKE = 45,
    WEAPON_MOLOTOV = 46,
    WEAPON_DECOY = 47,
    WEAPON_INCENDIARY = 48,
    WEAPON_C4 = 49,
    WEAPON_HEALTHSHOT = 57,
    WEAPON_KNIFE_T = 59,
    WEAPON_M4A1_S = 60,
    WEAPON_USP_S = 61,
    WEAPON_CZ75 = 63,
    WEAPON_REVOLVER = 64,
    WEAPON_TACTICAL_AWARENESS_GRENADE = 68,
    WEAPON_BAYONET = 500,
    WEAPON_KNIFE_CSS = 503,
    WEAPON_KNIFE_FLIP = 505,
    WEAPON_KNIFE_GUT = 506,
    WEAPON_KNIFE_KARAMBIT = 507,
    WEAPON_KNIFE_M9_BAYONET = 508,
    WEAPON_KNIFE_TACTICAL = 509,
    WEAPON_KNIFE_FALCHION = 512,
    WEAPON_KNIFE_SURVIVAL_BOWIE = 514,
    WEAPON_KNIFE_BUTTERFLY = 515,
    WEAPON_KNIFE_PUSH = 516,
    WEAPON_KNIFE_CORD = 517,
    WEAPON_KNIFE_CANIS = 518,
    WEAPON_KNIFE_URSUS = 519,
    WEAPON_KNIFE_GYPSY_JACKKNIFE = 520,
    WEAPON_KNIFE_OUTDOOR = 521,
    WEAPON_KNIFE_STILETTO = 522,
    WEAPON_KNIFE_WIDOWMAKER = 523,
    WEAPON_KNIFE_SKELETON = 525,
    WEAPON_KNIFE_KUKRI = 526,
};

enum EWeaponType: uint32_t {
    WEAPONTYPE_KNIFE = 0x0,
    WEAPONTYPE_PISTOL = 0x1,
    WEAPONTYPE_SUBMACHINEGUN = 0x2,
    WEAPONTYPE_RIFLE = 0x3,
    WEAPONTYPE_SHOTGUN = 0x4,
    WEAPONTYPE_SNIPER_RIFLE = 0x5,
    WEAPONTYPE_MACHINEGUN = 0x6,
    WEAPONTYPE_C4 = 0x7,
    WEAPONTYPE_TASER = 0x8,
    WEAPONTYPE_GRENADE = 0x9,
    WEAPONTYPE_EQUIPMENT = 0xa,
    WEAPONTYPE_STACKABLE_ITEM = 0xb,
    WEAPONTYPE_FISTS = 0xc,
    WEAPONTYPE_BREACH_CHARGE = 0xd,
    WEAPONTYPE_BUMP_MINE = 0xe,
    WEAPONTYPE_TABLET = 0xf,
    WEAPONTYPE_MELEE = 0x10,
    WEAPONTYPE_SHIELD = 0x11,
    WEAPONTYPE_ZONE_REPULSOR = 0x12,
    WEAPONTYPE_UNKNOWN = 0x13,
};

enum : uint32_t {
    WEAPON_TYPE_KNIFE = 0x0,
    WEAPON_TYPE_PISTOL = 0x1,
    WEAPON_TYPE_SUBMACHINEGUN = 0x2,
    WEAPON_TYPE_RIFLE = 0x3,
    WEAPON_TYPE_SHOTGUN = 0x4,
    WEAPON_TYPE_SNIPER_RIFLE = 0x5,
    WEAPON_TYPE_MACHINEGUN = 0x6,
    WEAPON_TYPE_C4 = 0x7,
    WEAPON_TYPE_TASER = 0x8,
    WEAPON_TYPE_GRENADE = 0x9,
    WEAPON_TYPE_EQUIPMENT = 0xa,
    WEAPON_TYPE_STACKABLE_ITEM = 0xb,
    WEAPON_TYPE_FISTS = 0xc,
    WEAPON_TYPE_BREACH_CHARGE = 0xd,
    WEAPON_TYPE_BUMP_MINE = 0xe,
    WEAPON_TYPE_TABLET = 0xf,
    WEAPON_TYPE_MELEE = 0x10,
    WEAPON_TYPE_SHIELD = 0x11,
    WEAPON_TYPE_ZONE_REPULSOR = 0x12,
    WEAPON_TYPE_UNKNOWN = 0x13,
};