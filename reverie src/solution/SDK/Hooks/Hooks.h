#pragma once

#define Hook(original, target) darkhook::hook_function(original, target)
#define GetOriginal(target) darkhook::find_original(target)

struct CSceneAnimtableObject;
struct CMeshPrimitiveOutputBuffer;
class CMaterial2;

void* __fastcall GeneratePrimitives(void* desc, CSceneAnimtableObject* object, void* a3, CMeshPrimitiveOutputBuffer* render_buf);

struct LightSceneBackup_t {
	uintptr_t address;
	float r, g, b, a;
	bool valid;
};

inline LightSceneBackup_t light_backups[256] = { 0 };
inline int light_backup_count = 0;

struct SkyboxBackup_t {
	uintptr_t address;
	float r, g, b, a;
	CMaterial2* material;
	bool valid;
};

inline SkyboxBackup_t skybox_backups[256] = { 0 };
inline int skybox_backup_count = 0;

struct Hooks_t {
	bool Fill();
	void Clear();

	static long WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void* (*RelativeMouseMode)(void*, bool) = nullptr;
};

inline Hooks_t* Hooks = new Hooks_t();
