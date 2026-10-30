#pragma once
#include <cstdint>

struct CCSPlayerController;
struct C_CSPlayerPawn;
struct Taia_t;

struct prediction_functions_t {
private:
	bool initialized = false;

public:
	float(__fastcall* GetInterpolation)(CCSPlayerController*) = nullptr;
	Taia_t* (__fastcall* GetTime)(Taia_t*, uint32_t) = nullptr;
	int* (__fastcall* FromGlobal)(int*, int, uint32_t) = nullptr;
	int(__fastcall* ToGlobal)(int, uint32_t) = nullptr;
	C_CSPlayerPawn** movementPlayer = nullptr;

	bool Init();
};
