#pragma once
#include "../../SDK/Globals.h"
#include "../../SDK/Valve/Datatypes/Taia.h"
#include "../../SDK/Valve/Fields/Entity.h"
#include "../../SDK/Valve/Fields/Move.h"
#include "PredictionFunctions.h"
#include <array>

class CUserCommand;
struct C_CSPlayerPawn;

class C_Prediction {
private:
	struct movement_step_t {
		uint64_t button = 0;
		bool pressed = false;
		float when = 0.f;
		float forward = 0.f;
		float left = 0.f;
		float pitch = 0.f;
		float yaw = 0.f;

		bool operator==(const movement_step_t&) const = default;
	};

	struct movement_input_t {
		std::array<uint64_t, 3> buttons = {};
		std::array<float, 6> movement = {};
		std::array<movement_step_t, 32> steps = {};
		int count = -1;

		bool operator==(const movement_input_t&) const = default;
	};

	prediction_functions_t functions;
	CMoveData moveData = {};
	movement_input_t movementInput;
	CUserCommand* predictionCommand = nullptr;
	C_CSPlayerPawn* oldMovementPlayer = nullptr;

	float flOldCurrentTime = 0.f;
	float flOldFrameTime = 0.f;
	float flOldInterpolationAmount = 0.f;
	int iOldTickCount = 0;
	int iOldCurrentThreadId = 0;
	bool bOldInPrediction = false;

	float weaponInaccuracy = 0.f;
	float weaponSpread = 0.f;
	bool weaponAccuracyValid = false;
	int weaponAccuracyCommand = -1;
	bool has_prediction_errors = false;
	bool bStarted = false;
	bool bCaptureLanding = false;
	bool late_command = false;

	bool Update(CUserCommand* cmd);
	bool IsValidCommand(CUserCommand* cmd);
	void UpdateJumpTime();
	void UpdateButtons(CUserCommand* cmd);
	void StartCommand(CUserCommand* cmd);
	void PerformPrediction(CUserCommand* cmd);
	void UpdateWeaponAccuracy();
	void FinishCommand();
	void RestoreData();

public:
	int m_nShootHistoryIndex = -1;
	Taia_t m_ShootTime = {};
	float m_flNextPrimaryAttack = 0.f;
	float m_fThrowTime = 0.f;
	int m_fFlags = 0;
	int m_nPostponeFireReadyTicks = 0;
	int m_nNextSecondaryAttackTick = 0;
	float m_flDuckSpeed = 0.f;
	Vector m_vecVelocity;
	Vector m_vecAbsVelocity;

	int m_nLastLandedTick = 0;
	float m_flLastLandedFrac = 0.f;
	float m_flNextJumpFrac = 0.f;
	bool m_bPredictUnducked = false;
	bool m_bLandingDucked = false;
	int m_nGroundTick = -1;
	float m_flGroundFrac = 0.f;
	int m_nLandingTick = -1;
	float m_flLandingFrac = 0.f;
	float m_flLandingStartFrac = 0.f;
	float m_flLandingEndFrac = 0.f;
	float m_flLandingHeight = 0.f;

	__forceinline float WeaponInaccuracy() { return weaponInaccuracy; };
	__forceinline float WeaponSpread() { return weaponSpread; };
	__forceinline bool WeaponAccuracyValid() { return weaponAccuracyValid; };
	__forceinline int WeaponAccuracyCommand() { return weaponAccuracyCommand; };
	__forceinline float frametime() { return flOldFrameTime; };
	__forceinline float curtime() { return flOldCurrentTime; };
	__forceinline int tickcount() { return iOldTickCount; };
	__forceinline bool HasPredictionErrors() { return has_prediction_errors; };
	bool LandedThisCommand() const { return bStarted && predictionCommand && m_nLandingTick >= 0 && m_nLandingTick == moveData.tickcount; }

	void BackupData();
	void CaptureLanding(CModernJump* jump, CMoveData* data);
	void CaptureGround(CCSPlayer_MovementServices* movement, CMoveData* data, CGameTrace* trace);

	bool Start(CUserCommand* cmd);
	void End();
};

extern C_Prediction* EnginePrediction;
