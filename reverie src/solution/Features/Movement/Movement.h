#pragma once
#include "../../SDK/Globals.h"
#include "../../SDK/Valve/Fields/Move.h"

class CUserCommand;

class CMovement {
private:
	int bunny_hop_command = -1;
	int jump_bug_command = -1;
	int jump_bug_pending_tick = -1;
	bool boost_takeoff = false;

	Vector leg_movement_impulses{};
	Vector leg_movement_input{};
	C_CSPlayerPawn* leg_movement_pawn{ nullptr };
	bool leg_movement_predicted{ false };

	void BoostTakeoff(CUserCommand* cmd);
	void PrepareJumpSubticks(CUserCommand* cmd);
	float GetSubtickWhen(CUserCommand* cmd, float when, bool quantize = true);
	bool PressJump(CUserCommand* cmd, float when);

	__forceinline float GetSubtickFraction(int step, int ticks) {
		if (!boost_takeoff || step == 0 || step == ticks)
			return static_cast<float>(step) / static_cast<float>(ticks);

		if (step == 1)
			return 1.f / 64.f;

		return static_cast<float>(step - 1) / static_cast<float>(ticks - 1);
	}

public:
	bool m_bKeepDucked = false;
	Vector angle{};
	bool m_bShouldStuck = false;

	struct PeekAssist_t {
		bool stop_latch{ false };
		Vector origin{};
		float best_distance{ 0.f };
		float animation{ 0.f };
		int stall_ticks{ 0 };
		int shot_tick{ -1 };
		bool active{ false };
		bool returning{ false };
		bool has_moved{ false };
		bool was_firing{ false };
		bool pending_shot_return{ false };
	} PeekAssistState;

	void BeforePrediction(CUserCommand* cmd);
	void FastStop(CUserCommand* cmd);
	void BunnyHop(CUserCommand* cmd, bool before_prediction = false);
	void JumpBug(CUserCommand* cmd, bool before_prediction = false);
	void AirStrafer(CUserCommand* cmd);
	void AirStuck(CUserCommand* cmd);
	void FastLadder(CUserCommand* cmd);
	void PeekAssist(CUserCommand* cmd);
	void ResetPeekAssist();
	void OnPresent();
	void LegMovement(CUserCommand* cmd, bool prediction = false);
	void RestoreLegMovement(CUserCommand* cmd);
	void Rotation();

	// --- advanced movement exploits ---
	void Edgebug(CUserCommand* cmd);
	void Texturebug(CUserCommand* cmd);
	void Pixelsurf(CUserCommand* cmd);
	void MovementPrediction(CUserCommand* cmd);

	bool m_bEdgebugActive = false;
	bool m_bTexturebugActive = false;
	bool m_bPixelsurfActive = false;
	float m_flLastFallStart = 0.f;
	Vector m_vecFallStart{};
};

extern CMovement* Movement;