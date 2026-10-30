#include <shared.h>
#include "PredictionFunctions.h"

bool prediction_functions_t::Init() {
	if (initialized)
		return movementPlayer != nullptr;

	initialized = true;

	GetInterpolation = Memory->Modules.Client.Find(xs("40 53 48 83 EC 20 48 8B D9 48 8B 0D ? ? ? ? 48 83 C1 08 48 8B 01 FF 90 B8 00 00 00 84 C0")).As<decltype(GetInterpolation)>();
	GetTime = Memory->Modules.Client.Find(xs("40 53 48 83 EC 30 48 8B 05 ? ? ? ? 48 8B D9 0F 29 74 24 20 48 8D 4C 24 40 F3 0F 10 70 50")).As<decltype(GetTime)>();
	FromGlobal = Memory->Modules.Client.Find(xs("40 53 48 83 EC 20 48 8B D9 45 85 C0 74 0B 89 11 48 8B C1 48 83 C4 20 5B C3")).As<decltype(FromGlobal)>();
	ToGlobal = Memory->Modules.Client.Find(xs("48 83 EC 28 85 D2 75 1B 48 8B 05 ? ? ? ? 48 85 C0 74 0F 44 8B C2 8B D1 48 8B C8")).As<decltype(ToGlobal)>();
	movementPlayer = Memory->Modules.Client.Find(xs("48 89 05 ? ? ? ? 48 8D 15 ? ? ? ? 83 FD 03 0F 84 ? ? ? ? 48 8B 05")).Rel(3, 7).As<C_CSPlayerPawn**>();

	return movementPlayer != nullptr;
}
