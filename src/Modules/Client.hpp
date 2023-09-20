#pragma once
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include <cstdint>

enum class CMStatus {
	CHALLENGE,
	WRONG_WARP,
	NONE,
};

class Client : public Module {
public:
	Interface *g_ClientDLL = nullptr;
	Interface *g_pClientMode = nullptr;
	Interface *g_HUDChallengeStats = nullptr;
	Interface *g_HUDQuickInfo = nullptr;
	Interface *s_EntityList = nullptr;
	Interface *g_Input = nullptr;
	Interface *g_HudChat = nullptr;
	Interface *g_HudMultiplayerBasicInfo = nullptr;
	Interface *g_HudSaveStatus = nullptr;
	Interface *g_GameMovement = nullptr;
	Interface *g_ClientTools = nullptr;

public:
	DECL_M(GetAbsOrigin, Vector);
	DECL_M(GetAbsAngles, QAngle);
	DECL_M(GetLocalVelocity, Vector);
	DECL_M(GetViewOffset, Vector);
	DECL_M(GetPortalLocal, CPortalPlayerLocalData);
	DECL_M(GetPlayerState, CPlayerState);

	using _GetClientEntity = ClientEnt *(__rescall *)(void *thisptr, int entnum);
	using _GetAllClasses = ClientClass *(*)();
	using _ShouldDraw = bool(__rescall *)(void *thisptr);
	using _ChatPrintf = void (*)(void *thisptr, int iPlayerIndex, int iFilter, const char *fmt, ...);
	using _StartMessageMode = void(__rescall *)(void *thisptr, int type);
	using _IN_ActivateMouse = void (*)(void *thisptr);
	using _IN_DeactivateMouse = void (*)(void *thisptr);

	using _GetEntity = void *(__rescall *)(void *thisptr, HTOOLHANDLE handle);
	using _SetRecording = void *(__rescall *)(void *thisptr, HTOOLHANDLE handle, bool recording);
	using _ShouldRecord = bool(__rescall *)(void *thisptr, HTOOLHANDLE handle);
	using _GetClassname = const char *(__rescall *)(void *thisptr, HTOOLHANDLE handle);
	using _EnableRecordingMode = void(__rescall *)(void *thisptr, bool bEnable);
	using _IsPlayer = bool(__rescall *)(void *thisptr, void *currentEnt);
	using _IsViewModel = bool(__rescall *)(void *thisptr, void *currentEnt);
	using _IsWeapon = bool(__rescall *)(void *thisptr, void *currentEnt);

	_GetClientEntity GetClientEntity = nullptr;
	_GetAllClasses GetAllClasses = nullptr;
	_ShouldDraw ShouldDraw = nullptr;
	_ChatPrintf ChatPrintf = nullptr;
	_StartMessageMode StartMessageMode = nullptr;
	_IN_ActivateMouse IN_ActivateMouse = nullptr;
	_IN_DeactivateMouse IN_DeactivateMouse = nullptr;

	_GetEntity GetEntity = nullptr;
	_SetRecording SetRecording = nullptr;
	_ShouldRecord ShouldRecord = nullptr;
	_GetClassname GetClassname = nullptr;
	_EnableRecordingMode EnableRecordingMode = nullptr;
	_IsPlayer IsPlayer = nullptr;
	_IsViewModel IsViewModel = nullptr;
	_IsWeapon IsWeapon = nullptr;

	std::string lastLevelName;
	void **gamerules;

public:
	ClientEnt *GetPlayer(int index);
	void CalcButtonBits(int nSlot, int &bits, int in_button, int in_ignore, kbutton_t *button, bool reset);
	bool ShouldDrawCrosshair();
	void Chat(Color col, const char *str);
	void MultiColorChat(const std::vector<std::pair<Color, std::string>> &components);
	void SetMouseActivated(bool state);
	CMStatus GetChallengeStatus();
	int GetSplitScreenPlayerSlot(void *entity);
	void ClFrameStageNotify(int stage);
	void OpenChat();

public:
	// CHLClient::FrameStageNotify 
	DECL_DETOUR(FrameStageNotify, int curStage);

	// C_BaseAnimating::RecordBones
	DECL_DETOUR_T(void *, RecordBones, CStudioHdr *hdr, matrix3x4_t *pBoneState);

	// CBaseViewModel::CalcViewModelLag
	DECL_DETOUR_T(void, CalcViewModelLag, Vector &origin, QAngle &angles, QAngle &original_angles);

	// CGameMovement::ProcessMovement
	DECL_DETOUR(ProcessMovement, void *player, CMoveData *move);

	// CRendering3dView::DrawOpaqueRenderables
	DECL_DETOUR(DrawOpaqueRenderables, void *renderCtx, int renderPath, void *deferClippedOpaqueRenderablesOut);

	// CRendering3dView::DrawTranslucentRenderables
	DECL_DETOUR(DrawTranslucentRenderables, bool inSkybox, bool shadowDepth);

	// CHLClient::LevelInitPreEntity
	DECL_DETOUR(LevelInitPreEntity, const char *levelName);

	// ClientModeShared::CreateMove
	DECL_DETOUR(CreateMove, float flInputSampleTime, CUserCmd *cmd);

	// CHud::GetName
	DECL_DETOUR_T(const char *, GetName);

	// CHudMultiplayerBasicInfo::ShouldDraw
	DECL_DETOUR_T(bool, ShouldDraw_BasicInfo);

	// CHudSaveStatus::ShouldDraw
	DECL_DETOUR_T(bool, ShouldDraw_SaveStatus);

	// CHudChat::MsgFunc_SayText2
	DECL_DETOUR(MsgFunc_SayText2, bf_read &msg);

	// CHudChat::GetTextColorForClient
#ifdef _WIN32
	DECL_DETOUR_T(void *, GetTextColorForClient, Color *col_out, TextColor color, int client_idx);
#else
	DECL_DETOUR_T(Color, GetTextColorForClient, TextColor color, int client_idx);
#endif

	// CInput::_DecodeUserCmdFromBuffer
	DECL_DETOUR(DecodeUserCmdFromBuffer, int nSlot, int buf, signed int sequence_number);

	// ClientModeShared::OverrideView
	DECL_DETOUR_T(void, OverrideView, CViewSetup *m_View);

	bool Init() override;
	void Shutdown() override;
	const char *Name() override {
		return MODULE("client");
	}
};

extern Client *client;

extern Variable cl_showpos;
extern Variable cl_sidespeed;
extern Variable cl_backspeed;
extern Variable cl_forwardspeed;
extern Variable in_forceuser;
extern Variable cl_fov;
extern Variable prevent_crouch_jump;
extern Variable r_portalsopenall;
extern Variable r_drawviewmodel;
extern Variable crosshairVariable;
