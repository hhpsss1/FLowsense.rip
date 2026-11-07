#pragma once

#include <shared.h>

// Updated CS2 patterns
namespace Patterns {
    // GET_PLAYER_FROM_CONTROLLER: gets C_CSPlayerPawn from CCSPlayerController
    inline Address_t GetPlayerFromController;

    // SEND_MOVE: sends move command to server
    inline Address_t SendMove;

    // WORLD_TO_SCREEN: world position to screen coordinates
    inline Address_t WorldToScreen;

    // GET_VIEW_ANGLES: gets player view angles array
    inline Address_t GetViewAngles;

    // WEAPON_UPDATE_ACCURACY: weapon accuracy update function
    inline Address_t WeaponUpdateAccuracy;

    inline void Initialize() {
        GetPlayerFromController = Memory->Modules.Client.Find(xs("8B 91 D4 05 00 00 83 FA FF 74 3F"));
        SendMove = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 8D 55"));
        WorldToScreen = Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 1D ? ? ? ? 48 8B FA 48 8B F1"));
        GetViewAngles = Memory->Modules.Client.Find(xs("48 63 C2 48 69 D0 ? ? ? ? 48 8D 81"));
        WeaponUpdateAccuracy = Memory->Modules.Client.Find(xs("40 57 41 56 48 83 EC ?? 48 8B F9 E8 ? ? ? ? 4C 8B F0 48 85 C0"));
    }
}
