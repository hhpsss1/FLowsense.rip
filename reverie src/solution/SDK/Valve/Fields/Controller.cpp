#include <shared.h>

CUserCommandManager* CCSPlayerController::GetCommandManager() {
	static auto fn = Memory->Modules.Client.Find(xs("41 56 41 57 48 83 EC ? 48 8D 54 24")).As<CUserCommandManager*(__fastcall*)(CCSPlayerController*)>();
	return fn(this);
}