#pragma once

#include <wtypes.h>
#include "../Utils/Math/Math.h"
#include "Valve/Datatypes/Taia.h"

#define INTERVAL_PER_TICK 0.015625f
#define TIME_TO_TICKS(t) ((int)(0.5f + (float)(t) / INTERVAL_PER_TICK))
#define TICKS_TO_TIME(tick) ((tick) * INTERVAL_PER_TICK)

struct CSchemaSystem;
struct CEntitySystem;
struct CPrediction;
struct CCvar;
struct CPhysics;

struct CUserCommand;
struct CUserCommandManager;
struct C_CSPlayerPawn;
struct C_CSWeaponBase;
struct CCSWeaponBaseVData;
struct CCSPlayerController;
struct CBaseUserCmdPB;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct CSwapChain;
struct CGlobalVariables;
struct CCSGOInput;
struct CMaterialSystem;
struct CSceneSystem;
struct CEngineToClient;
struct CGameEventManager;
struct CParticleManager;
struct CParticleSystem;
struct CResourceSystem;
struct CSource2Client;
struct C_CSGameRules;
struct CFileSystem;
struct CLocalize;
struct CEnginePanorama;

inline CEnginePanorama* EnginePanorama;
inline CFileSystem* FileSystem;
inline CLocalize* Localize;
inline CResourceSystem* ResourceSystem;
inline CEngineToClient* EngineClient;
inline CCSGOInput* CSGOInput;
inline CSchemaSystem* SchemaSystem;
inline CEntitySystem* EntitySystem;
inline CGlobalVariables* GlobalVars;
inline CPrediction* Prediction;
inline CCvar* Cvar;
inline CSwapChain* SwapChain;
inline ID3D11Device* Device;
inline ID3D11DeviceContext* DeviceCtx;
inline ID3D11RenderTargetView* Target;
inline void* Inputs;
inline CPhysics* Physics;
inline CSource2Client* Source2Client;
inline CMaterialSystem* MaterialSystem;
inline CSceneSystem* SceneSystem;
inline void* ClientMode;
inline CGameEventManager* GameEventManager;
inline CParticleManager* ParticleManager;
inline CParticleSystem* ParticleSystem;
inline C_CSGameRules* GameRules;

struct Valve_t {
	void Fill();
	void Clear();

	void CreateRenderTarget();
	void DestroyRenderTarget();
} inline Valve;

struct Ctx_t {
	bool Fill();
	void Clear();

	CUserCommand* cmd;
	CUserCommandManager* cmd_manager;
	CBaseUserCmdPB* base_cmd;

	CCSPlayerController* controller;
	C_CSPlayerPawn* pawn;
	C_CSWeaponBase* weapon;
	CCSWeaponBaseVData* weapon_data;

	Vector eye_position;
	Taia_t player_time;
	Vector camera_origin;
	Vector camera_angles;
	Matrix4x4_t viewmatrix;
} inline ctx;

struct Entry_t {
	static void Fill(HMODULE);
	void Clear(HMODULE);
} inline Entry;