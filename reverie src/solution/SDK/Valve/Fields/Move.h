#pragma once

struct CSubtickMoves;

struct CMoveData {
	uint8_t has_zero_frametime : 1;
	uint8_t is_late_command : 1;
	int player_handle;
	Vector abs_viewangles;
	Vector viewangles;
	Vector last_movement_impulses;
	float forwardmove;
	float leftove;
	float upmove;
	Vector velocity;
	Vector angles;
	Vector input_rotated;
	CUtlVector<CSubtickMoves> subtick_moves;
	CUtlVector<CAttackSubtickMoves> attack_subtick_moves;
	bool has_subtick_inputs;
	float weapon_time_switch_subticked;
	CUtlVector<CGameTrace> touch_list;
	Vector collision_normal;
	Vector ground_normal;
	Vector abs_origin;
	int32_t tickcount;
	int32_t target_tick;
	float subtick_start_fraction;
	float subtick_end_fraction;

	Vector wish_velocity;
	Vector old_angles;
	Vector walk_wish_vel;
	Vector continous_acceleration;
	Vector frame_velocity_delta;
	float max_speed;
	float client_max_speed;
	float friction_decel;
	float pre_air_move_position_z;
	float pre_air_move_velocity_z;
	float pre_air_move_accelerate_z;
	bool in_air;
	bool gamecode_move_player;
	bool in_setup_move;

	void Reset();

	//bool AddTouchTrace(CGameTrace* trace, Math::Vec3& velocity);
	//void Init(CUserCommand* command, int32_t target_tick_input, bool force_zero_frametime);
};

struct CCSPlayer_MovementServices;
class CUserCommand;

struct CModernJump {
	FindField(int32_t, m_nLastActualJumpPressTick, xs("CCSPlayerModernJump"), xs("m_nLastActualJumpPressTick"));
	FindField(float, m_flLastActualJumpPressFrac, xs("CCSPlayerModernJump"), xs("m_flLastActualJumpPressFrac"));
	FindField(int32_t, m_nLastUsableJumpPressTick, xs("CCSPlayerModernJump"), xs("m_nLastUsableJumpPressTick"));
	FindField(float, m_flLastUsableJumpPressFrac, xs("CCSPlayerModernJump"), xs("m_flLastUsableJumpPressFrac"));
	FindField(float, m_flLastLandedVelocityX, xs("CCSPlayerModernJump"), xs("m_flLastLandedVelocityX"));
	FindField(float, m_flLastLandedVelocityY, xs("CCSPlayerModernJump"), xs("m_flLastLandedVelocityY"));
	FindField(float, m_flLastLandedVelocityZ, xs("CCSPlayerModernJump"), xs("m_flLastLandedVelocityZ"));
	FindField(float, m_flLastLandedFrac, xs("CCSPlayerModernJump"), xs("m_flLastLandedFrac"));
	FindField(int, m_nLastLandedTick, xs("CCSPlayerModernJump"), xs("m_nLastLandedTick"));

	bool CanJump(CMoveData* data);
	void Initialize(CCSPlayer_MovementServices* move, CMoveData* data);
};

struct CCSPlayer_MovementServices {
	FindField(int32_t, m_nImpulse, xs("CPlayer_MovementServices"), xs("m_nImpulse"));
	FindField(CInButtonState, m_nButtons, xs("CPlayer_MovementServices"), xs("m_nButtons"));
	FindField(uint64_t, m_nQueuedButtonDownMask, xs("CPlayer_MovementServices"), xs("m_nQueuedButtonDownMask"));
	FindField(uint64_t, m_nQueuedButtonChangeMask, xs("CPlayer_MovementServices"), xs("m_nQueuedButtonChangeMask"));
	FindField(uint64_t, m_nButtonDoublePressed, xs("CPlayer_MovementServices"), xs("m_nButtonDoublePressed"));
	FindField(uint32_t*, m_pButtonPressedCmdNumber, xs("CPlayer_MovementServices"), xs("m_pButtonPressedCmdNumber"));
	FindField(uint32_t, m_nLastCommandNumberProcessed, xs("CPlayer_MovementServices"), xs("m_nLastCommandNumberProcessed"));
	FindField(uint64_t, m_nToggleButtonDownMask, xs("CPlayer_MovementServices"), xs("m_nToggleButtonDownMask"));
	FindField(float, m_flCmdForwardMove, xs("CPlayer_MovementServices"), xs("m_flCmdForwardMove"));
	FindField(float, m_flCmdLeftMove, xs("CPlayer_MovementServices"), xs("m_flCmdLeftMove"));
	FindField(float, m_flCmdUpMove, xs("CPlayer_MovementServices"), xs("m_flCmdUpMove"));
	FindField(float, m_flMaxspeed, xs("CPlayer_MovementServices"), xs("m_flMaxspeed"));
	FindFieldArray(m_arrForceSubtickMoveWhen, float, xs("CPlayer_MovementServices"), xs("m_arrForceSubtickMoveWhen"));
	FindField(float, m_flForwardMove, xs("CPlayer_MovementServices"), xs("m_flForwardMove"));
	FindField(float, m_flLeftMove, xs("CPlayer_MovementServices"), xs("m_flLeftMove"));
	FindField(float, m_flUpMove, xs("CPlayer_MovementServices"), xs("m_flUpMove"));
	FindField(Vector, m_vecLastMovementImpulses, xs("CPlayer_MovementServices"), xs("m_vecLastMovementImpulses"));
	FindField(Vector, m_vecOldViewAngles, xs("CPlayer_MovementServices"), xs("m_vecOldViewAngles"));
	FindField(float, m_flStepSoundTime, xs("CPlayer_MovementServices_Humanoid"), xs("m_flStepSoundTime"));
	FindField(float, m_flFallVelocity, xs("CPlayer_MovementServices_Humanoid"), xs("m_flFallVelocity"));
	FindField(Vector, m_groundNormal, xs("CPlayer_MovementServices_Humanoid"), xs("m_groundNormal"));
	FindField(float, m_flSurfaceFriction, xs("CPlayer_MovementServices_Humanoid"), xs("m_flSurfaceFriction"));
	FindField(void*, m_surfaceProps, xs("CPlayer_MovementServices_Humanoid"), xs("m_surfaceProps"));
	FindField(int32_t, m_nStepside, xs("CPlayer_MovementServices_Humanoid"), xs("m_nStepside"));
	FindField(Vector, m_vecSmoothedVelocity, xs("CPlayer_MovementServices_Humanoid"), xs("m_vecSmoothedVelocity"));
	FindField(bool, m_bUsingGroundTopologyOffset, xs("CCSPlayer_MovementServices"), xs("m_bUsingGroundTopologyOffset"));
	FindField(float, m_flUsingGroundTopologyOffsetTransitionSmoothing, xs("CCSPlayer_MovementServices"), xs("m_flUsingGroundTopologyOffsetTransitionSmoothing"));
	FindField(Vector, m_vecLadderNormal, xs("CCSPlayer_MovementServices"), xs("m_vecLadderNormal"));
	FindField(int32_t, m_nLadderSurfacePropIndex, xs("CCSPlayer_MovementServices"), xs("m_nLadderSurfacePropIndex"));
	FindField(bool, m_bDucked, xs("CCSPlayer_MovementServices"), xs("m_bDucked"));
	FindField(float, m_flDuckAmount, xs("CCSPlayer_MovementServices"), xs("m_flDuckAmount"));
	FindField(float, m_flDuckSpeed, xs("CCSPlayer_MovementServices"), xs("m_flDuckSpeed"));
	FindField(bool, m_bDuckOverride, xs("CCSPlayer_MovementServices"), xs("m_bDuckOverride"));
	FindField(bool, m_bDesiresDuck, xs("CCSPlayer_MovementServices"), xs("m_bDesiresDuck"));
	FindField(bool, m_bDucking, xs("CCSPlayer_MovementServices"), xs("m_bDucking"));
	FindField(float, m_flDuckRootOffset, xs("CCSPlayer_MovementServices"), xs("m_flDuckRootOffset"));
	FindField(float, m_flDuckViewOffset, xs("CCSPlayer_MovementServices"), xs("m_flDuckViewOffset"));
	FindField(float, m_flLastDuckTime, xs("CCSPlayer_MovementServices"), xs("m_flLastDuckTime"));
	FindField(float, m_flBombPlantViewOffset, xs("CCSPlayer_MovementServices"), xs("m_flBombPlantViewOffset"));
	FindField(Vector2D, m_vecLastPositionAtFullCrouchSpeed, xs("CCSPlayer_MovementServices"), xs("m_vecLastPositionAtFullCrouchSpeed"));
	FindField(bool, m_duckUntilOnGround, xs("CCSPlayer_MovementServices"), xs("m_duckUntilOnGround"));
	FindField(bool, m_bHasWalkMovedSinceLastJump, xs("CCSPlayer_MovementServices"), xs("m_bHasWalkMovedSinceLastJump"));
	FindField(bool, m_bInStuckTest, xs("CCSPlayer_MovementServices"), xs("m_bInStuckTest"));
	FindField(int32_t, m_nTraceCount, xs("CCSPlayer_MovementServices"), xs("m_nTraceCount"));
	FindField(int32_t, m_StuckLast, xs("CCSPlayer_MovementServices"), xs("m_StuckLast"));
	FindField(bool, m_bSpeedCropped, xs("CCSPlayer_MovementServices"), xs("m_bSpeedCropped"));
	FindField(int32_t, m_nOldWaterLevel, xs("CCSPlayer_MovementServices"), xs("m_nOldWaterLevel"));
	FindField(float, m_flWaterEntryTime, xs("CCSPlayer_MovementServices"), xs("m_flWaterEntryTime"));
	FindField(Vector, m_vecForward, xs("CCSPlayer_MovementServices"), xs("m_vecForward"));
	FindField(Vector, m_vecLeft, xs("CCSPlayer_MovementServices"), xs("m_vecLeft"));
	FindField(Vector, m_vecUp, xs("CCSPlayer_MovementServices"), xs("m_vecUp"));
	FindField(int32_t, m_nGameCodeHasMovedPlayerAfterCommand, xs("CCSPlayer_MovementServices"), xs("m_nGameCodeHasMovedPlayerAfterCommand"));
	FindField(bool, m_bMadeFootstepNoise, xs("CCSPlayer_MovementServices"), xs("m_bMadeFootstepNoise"));
	FindField(int32_t, m_iFootsteps, xs("CCSPlayer_MovementServices"), xs("m_iFootsteps"));
	FindField(float, m_fStashGrenadeParameterWhen, xs("CCSPlayer_MovementServices"), xs("m_fStashGrenadeParameterWhen"));
	FindField(bool, m_bUseFrictionStashedSpeed, xs("CCSPlayer_MovementServices"), xs("m_bUseFrictionStashedSpeed"));
	FindField(float, m_flUseFrictionStashedSpeedUntilFrac, xs("CCSPlayer_MovementServices"), xs("m_flUseFrictionStashedSpeedUntilFrac"));
	FindField(float, m_flFrictionStashedSpeed, xs("CCSPlayer_MovementServices"), xs("m_flFrictionStashedSpeed"));
	FindField(float, m_flStamina, xs("CCSPlayer_MovementServices"), xs("m_flStamina"));
	FindField(float, m_flHeightAtJumpStart, xs("CCSPlayer_MovementServices"), xs("m_flHeightAtJumpStart"));
	FindField(float, m_flMaxJumpHeightThisJump, xs("CCSPlayer_MovementServices"), xs("m_flMaxJumpHeightThisJump"));
	FindField(float, m_flMaxJumpHeightLastJump, xs("CCSPlayer_MovementServices"), xs("m_flMaxJumpHeightLastJump"));
	FindField(float, m_flStaminaAtJumpStart, xs("CCSPlayer_MovementServices"), xs("m_flStaminaAtJumpStart"));
	FindField(float, m_flVelMulAtJumpStart, xs("CCSPlayer_MovementServices"), xs("m_flVelMulAtJumpStart"));
	FindField(float, m_flAccumulatedJumpError, xs("CCSPlayer_MovementServices"), xs("m_flAccumulatedJumpError"));
	FindField(__int64, m_LegacyJump, xs("CCSPlayer_MovementServices"), xs("m_LegacyJump"));
	FindField(CModernJump, m_ModernJump, xs("CCSPlayer_MovementServices"), xs("m_ModernJump"));
	FindField(int32_t, m_nLastJumpTick, xs("CCSPlayer_MovementServices"), xs("m_nLastJumpTick"));
	FindField(float, m_flLastJumpFrac, xs("CCSPlayer_MovementServices"), xs("m_flLastJumpFrac"));
	FindField(float, m_flLastJumpVelocityZ, xs("CCSPlayer_MovementServices"), xs("m_flLastJumpVelocityZ"));
	FindField(bool, m_bJumpApexPending, xs("CCSPlayer_MovementServices"), xs("m_bJumpApexPending"));
	FindField(float, m_flTicksSinceLastSurfingDetected, xs("CCSPlayer_MovementServices"), xs("m_flTicksSinceLastSurfingDetected"));
	FindField(Vector, m_vecWalkWishVel, xs("CCSPlayer_MovementServices"), xs("m_vecWalkWishVel"));
	FindField(bool, m_bHasEverProcessedCommand, xs("CCSPlayer_MovementServices"), xs("m_bHasEverProcessedCommand"));

	FindOffset(uint64_t, m_afButtonForced, 0x198);

	void SetPredictionCommand(CUserCommand* command);
	void ResetPredictionCommand();

	void CheckMovingGround(float frame_time);
	void SetupMove(CUserCommand* command, CMoveData* move_data);
	void FinishMove(CUserCommand* command, CMoveData* move_data);
	void DoMovement(CUserCommand* command, CMoveData* move_data);
	void ProcessImpacts(CUserCommand* command, CMoveData* move_data);
	void UpdateInputState(CUserCommand* command);
	void BackupInputState();
	void RestoreInputState();
	void FinishInputState(CUserCommand* command);
	void ForceButtonState(uint64_t button, bool pressed);
	void RemoveJumpButton();
};
