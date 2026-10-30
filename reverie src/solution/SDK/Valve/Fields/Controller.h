#pragma once

struct CCSPlayerController_InGameMoneyServices {
	FindField(int32_t, m_iAccount, xs("CCSPlayerController_InGameMoneyServices"), xs("m_iAccount"));
};

struct CCSPlayerController {
	FindField(int32_t, m_nTickBase, xs("CBasePlayerController"), xs("m_nTickBase"));
	FindField(bool, m_bPawnHasDefuser, xs("CCSPlayerController"), xs("m_bPawnHasDefuser"));
	FindField(uint64_t, m_nInButtonsWhichAreToggles, xs("CBasePlayerController"), xs("m_nInButtonsWhichAreToggles"));
	FindField(bool, m_bPawnHasHelmet, xs("CCSPlayerController"), xs("m_bPawnHasHelmet"));
	FindField(bool, m_bPawnIsAlive, xs("CCSPlayerController"), xs("m_bPawnIsAlive"));
	FindField(uint32_t, m_iPing, xs("CCSPlayerController"), xs("m_iPing"));
	FindField(const char*, m_sSanitizedPlayerName, xs("CCSPlayerController"), xs("m_sSanitizedPlayerName"));
	FindField(bool, m_bIsLocalPlayerController, xs("CBasePlayerController"), xs("m_bIsLocalPlayerController"));
	FindField(uint64_t, m_steamID, xs("CBasePlayerController"), xs("m_steamID"));
	FindField(CBaseHandle, m_hPawn, xs("CBasePlayerController"), xs("m_hPawn"));
	FindField(CBaseHandle, m_hObserverPawn, xs("CCSPlayerController"), xs("m_hObserverPawn"));
	FindField(CCSPlayerController_InGameMoneyServices*, m_pInGameMoneyServices, xs("CCSPlayerController"), xs("m_pInGameMoneyServices"));

	CUserCommandManager* GetCommandManager();
};