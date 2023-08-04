#include "Server.hpp"

#include "Client.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Features/Camera.hpp"
#include "Features/Demo/NetworkGhostPlayer.hpp"
#include "Features/EntityList.hpp"
#include "Features/FovChanger.hpp"
#include "Features/Hud/Crosshair.hpp"
#include "Features/NetMessage.hpp"
#include "Features/Session.hpp"
#include "Features/Speedrun/SpeedrunTimer.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include "Features/OverlayRender.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <cfloat>

#define RESET_COOP_PROGRESS_MESSAGE_TYPE "coop-reset"
#define CM_FLAGS_MESSAGE_TYPE "cm-flags"

Variable sv_cheats;
Variable sv_footsteps;
Variable sv_alternateticks;
Variable sv_bonus_challenge;
Variable sv_accelerate;
Variable sv_airaccelerate;
Variable sv_paintairacceleration;
Variable sv_friction;
Variable sv_maxspeed;
Variable sv_stopspeed;
Variable sv_maxvelocity;
Variable sv_gravity;

REDECL(Server::CheckJumpButton);
REDECL(Server::CheckJumpButtonBase);
REDECL(Server::PlayerMove);
REDECL(Server::FinishGravity);
REDECL(Server::AirMove);
REDECL(Server::AirMoveBase);
REDECL(Server::GameFrame);
REDECL(Server::ApplyGameSettings);
REDECL(Server::OnRemoveEntity);
REDECL(Server::PlayerRunCommand);
REDECL(Server::ViewPunch);
REDECL(Server::IsInPVS);
REDECL(Server::ProcessMovement);
REDECL(Server::GetPlayerViewOffset);
REDECL(Server::StartTouchChallengeNode);
REDECL(Server::say_callback);

SMDECL(Server::GetPortals, int, iNumPortalsPlaced);
SMDECL(Server::GetAbsOrigin, Vector, m_vecAbsOrigin);
SMDECL(Server::GetAbsAngles, QAngle, m_angAbsRotation);
SMDECL(Server::GetLocalVelocity, Vector, m_vecVelocity);
SMDECL(Server::GetFlags, int, m_fFlags);
SMDECL(Server::GetEFlags, int, m_iEFlags);
SMDECL(Server::GetMaxSpeed, float, m_flMaxspeed);
SMDECL(Server::GetGravity, float, m_flGravity);
SMDECL(Server::GetViewOffset, Vector, m_vecViewOffset);
SMDECL(Server::GetPortalLocal, CPortalPlayerLocalData, m_PortalLocal);
SMDECL(Server::GetEntityName, char *, m_iName);
SMDECL(Server::GetEntityClassName, char *, m_iClassname);
SMDECL(Server::GetPlayerState, CPlayerState, pl);

ServerEnt *Server::GetPlayer(int index) {
	return this->UTIL_PlayerByIndex(index);
}
bool Server::IsPlayer(void *entity) {
	return Memory::VMT<bool (*)(void *)>(entity, Offsets::IsPlayer)(entity);
}
bool Server::AllowsMovementChanges() {
	return sv_cheats.GetBool();
}
int Server::GetSplitScreenPlayerSlot(void *entity) {
	// Simplified version of CBasePlayer::GetSplitScreenPlayerSlot
	for (auto i = 0; i < Offsets::MAX_SPLITSCREEN_PLAYERS; ++i) {
		if (server->UTIL_PlayerByIndex(i + 1) == entity) {
			return i;
		}
	}

	return 0;
}
void Server::KillEntity(void *entity) {
	variant_t val = {0};
	val.fieldType = FIELD_VOID;
	void *player = this->GetPlayer(1);
	server->AcceptInput(entity, "Kill", player, player, val, 0);
}

float Server::GetCMTimer() {
	ServerEnt *sv_player = this->GetPlayer(1);
	if (!sv_player) {
		ClientEnt *cl_player = client->GetPlayer(1);
		if (!cl_player) return 0.0f;
		return cl_player->field<float>("fNumSecondsTaken");
	}
	return sv_player->field<float>("fNumSecondsTaken");
}

// CGameMovement::CheckJumpButton
DETOUR_T(bool, Server::CheckJumpButton) {
	auto jumped = false;

	if (server->AllowsMovementChanges()) {
		auto mv = *reinterpret_cast<CHLMoveData **>((uintptr_t)thisptr + Offsets::mv);

		server->jumpedLastTime = false;
		server->savedVerticalVelocity = mv->m_vecVelocity[2];

		server->callFromCheckJumpButton = true;
		jumped = Server::CheckJumpButton(thisptr);
		server->callFromCheckJumpButton = false;

		if (jumped) {
			server->jumpedLastTime = true;
		}
	} else {
		jumped = Server::CheckJumpButton(thisptr);
	}

	return jumped;
}

// CGameMovement::PlayerMove
DETOUR(Server::PlayerMove) {
	return Server::PlayerMove(thisptr);
}

extern Hook g_playerRunCommandHook;
// CPortal_Player::PlayerRunCommand
DETOUR(Server::PlayerRunCommand, CUserCmd *cmd, void *moveHelper) {
	g_playerRunCommandHook.Disable();
	auto ret = Server::PlayerRunCommand(thisptr, cmd, moveHelper);
	g_playerRunCommandHook.Enable();

	return ret;
}
Hook g_playerRunCommandHook(&Server::PlayerRunCommand_Hook);

extern Hook g_ViewPunch_Hook;
DETOUR_T(void, Server::ViewPunch, const QAngle &offset) {
	g_ViewPunch_Hook.Disable();
	Server::ViewPunch(thisptr, offset);
	g_ViewPunch_Hook.Enable();
}
Hook g_ViewPunch_Hook(&Server::ViewPunch_Hook);

// CGameMovement::ProcessMovement
DETOUR(Server::ProcessMovement, void *player, CMoveData *move) {
	int slot = server->GetSplitScreenPlayerSlot(player);
	Event::Trigger<Event::PROCESS_MOVEMENT>({ slot, true });

	return Server::ProcessMovement(thisptr, player, move);
}

// CGameMovement::GetPlayerViewOffset
DETOUR_T(Vector*, Server::GetPlayerViewOffset, bool ducked) {
	return Server::GetPlayerViewOffset(thisptr, ducked);
}

extern Hook g_IsInPVS_Hook;
DETOUR_T(bool, Server::IsInPVS, void *info) {
	g_IsInPVS_Hook.Disable();
	auto res = Server::IsInPVS(thisptr, info);
	g_IsInPVS_Hook.Enable();

	return res;
}
Hook g_IsInPVS_Hook(&Server::IsInPVS_Hook);

ON_INIT {
	NetMessage::RegisterHandler(
		CM_FLAGS_MESSAGE_TYPE, +[](const void *data, size_t size) {
			if (size == 6 && engine->IsOrange()) {
				char slot = *(char *)data;
				float time = *(float *)((uintptr_t)data + 1);
				bool end = !!*(char *)((uintptr_t)data + 5);
				Event::Trigger<Event::CM_FLAGS>({slot, time, end});
			}
		});
}

static inline bool hasSlotCompleted(void *thisptr, int slot) {
#ifdef _WIN32
	return *(uint8_t *)((uintptr_t)thisptr + 0x4B0 + slot);
#else
	return *(uint8_t *)((uintptr_t)thisptr + 0x4C8 + slot);
#endif
}

static inline bool isFakeFlag(void *thisptr) {
	return hasSlotCompleted(thisptr, 2);
}

static void TriggerCMFlag(int slot, float time, bool end) {
	Event::Trigger<Event::CM_FLAGS>({slot, time, end});
	if (engine->IsCoop()) {
		char data[6];
		data[0] = (char)slot;
		*(float *)(data + 1) = time;
		data[5] = (char)end;
		NetMessage::SendMsg(CM_FLAGS_MESSAGE_TYPE, data, sizeof data);
	}
}

extern Hook g_flagStartTouchHook;
DETOUR(Server::StartTouchChallengeNode, void *entity) {
	if (server->IsPlayer(entity) && !isFakeFlag(thisptr) && client->GetChallengeStatus() == CMStatus::CHALLENGE) {
		int slot = server->GetSplitScreenPlayerSlot(entity);
		if (!hasSlotCompleted(thisptr, slot)) {
			float time = server->GetCMTimer();
			bool end = !engine->IsCoop() || hasSlotCompleted(thisptr, slot == 1 ? 0 : 1);
			TriggerCMFlag(slot, time, end);
		}
	}

	g_flagStartTouchHook.Disable();
	auto ret = Server::StartTouchChallengeNode(thisptr, entity);
	g_flagStartTouchHook.Enable();

	return ret;
}
Hook g_flagStartTouchHook(&Server::StartTouchChallengeNode_Hook);

// CGameMovement::FinishGravity
DETOUR(Server::FinishGravity) {
	return Server::FinishGravity(thisptr);
}

// CGameMovement::AirMove
DETOUR_B(Server::AirMove) {
	return Server::AirMove(thisptr);
}

extern Hook g_AcceptInputHook;

// TODO: the windows signature is a bit dumb. fastcall is like thiscall
// but for normal functions and takes an arg in edx, so we use it
// because msvc won't let us use thiscall on a non-member function
#ifdef _WIN32
static void __fastcall AcceptInput_Hook(void *thisptr, void *unused, const char *inputName, void *activator, void *caller, variant_t parameter, int outputID)
#else
static void __cdecl AcceptInput_Hook(void *thisptr, const char *inputName, void *activator, void *caller, variant_t parameter, int outputID)
#endif
{
	const char *entName = server->GetEntityName(thisptr);
	const char *className = server->GetEntityClassName(thisptr);
	if (!entName) entName = "";
	if (!className) className = "";

	std::optional<int> activatorSlot;

	if (activator && server->IsPlayer(activator)) {
		activatorSlot = server->GetSplitScreenPlayerSlot(activator);
	}

	SpeedrunTimer::TestInputRules(entName, className, inputName, parameter.ToString(), activatorSlot);

	if (engine->demorecorder->isRecordingDemo) {
		size_t entNameLen = strlen(entName);
		size_t classNameLen = strlen(className);
		size_t inputNameLen = strlen(inputName);

		const char *paramStr = parameter.ToString();

		size_t len = entNameLen + classNameLen + inputNameLen + strlen(paramStr) + 5;
		if (activatorSlot) {
			len += 1;
		}
		char *data = (char *)malloc(len);
		char *data1 = data;
		if (!activatorSlot) {
			data[0] = 0x03;
		} else {
			data[0] = 0x04;
			data[1] = *activatorSlot;
			++data1;
		}
		strcpy(data1 + 1, entName);
		strcpy(data1 + 2 + entNameLen, className);
		strcpy(data1 + 3 + entNameLen + classNameLen, inputName);
		strcpy(data1 + 4 + entNameLen + classNameLen + inputNameLen, paramStr);
		engine->demorecorder->RecordData(data, len);
		free(data);
	}

	// HACKHACK
	// Deals with maps where you hit a normal transition trigger
	if (!strcmp(className, "portal_stats_controller") && !strcmp(inputName, "OnLevelEnd") && client->GetChallengeStatus() == CMStatus::CHALLENGE) {
		float time = server->GetCMTimer();
		if (engine->IsCoop()) {
			TriggerCMFlag(0, time, false);
			TriggerCMFlag(1, time, true);
		} else {
			TriggerCMFlag(0, time, true);
		}
	}

	g_AcceptInputHook.Disable();
	server->AcceptInput(thisptr, inputName, activator, caller, parameter, outputID);
	g_AcceptInputHook.Enable();
}

// This is kinda annoying - it's got to be in a separate function
// because we need a reference to an entity vtable to find the address
// of CBaseEntity::AcceptInput, but we generally can't do that until
// we've loaded into a level.
static bool IsAcceptInputTrampolineInitialized = false;
Hook g_AcceptInputHook(&AcceptInput_Hook);
static void InitAcceptInputTrampoline() {
	void *ent = server->m_EntPtrArray[0].m_pEntity;
	if (ent == nullptr) return;
	IsAcceptInputTrampolineInitialized = true;
	server->AcceptInput = Memory::VMT<Server::_AcceptInput>(ent, Offsets::AcceptInput);

	g_AcceptInputHook.SetFunc(server->AcceptInput);
}

static bool g_IsCMFlagHookInitialized = false;
static void InitCMFlagHook() {
	for (int i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {
		void *ent = server->m_EntPtrArray[i].m_pEntity;
		if (!ent) continue;

		auto classname = server->GetEntityClassName(ent);
		if (!classname || strcmp(classname, "challenge_mode_end_node")) continue;

		Server::StartTouchChallengeNode = Memory::VMT<Server::_StartTouchChallengeNode>(ent, Offsets::StartTouch);
		g_flagStartTouchHook.SetFunc(Server::StartTouchChallengeNode);
		g_IsCMFlagHookInitialized = true;

		break;
	}
}

static bool g_IsPlayerRunCommandHookInitialized = false;
static void InitPlayerRunCommandHook() {
	void *player = server->GetPlayer(1);
	if (!player) return;
	Server::PlayerRunCommand = Memory::VMT<Server::_PlayerRunCommand>(player, Offsets::PlayerRunCommand);
	g_playerRunCommandHook.SetFunc(Server::PlayerRunCommand);
	g_IsPlayerRunCommandHookInitialized = true;
}

// CServerGameDLL::GameFrame
DETOUR(Server::GameFrame, bool simulating)
{
	if (!IsAcceptInputTrampolineInitialized) InitAcceptInputTrampoline();
	if (!g_IsCMFlagHookInitialized) InitCMFlagHook();
	if (!g_IsPlayerRunCommandHookInitialized) InitPlayerRunCommandHook();

	if (p2fx_tick_debug.GetInt() >= 3 || (p2fx_tick_debug.GetInt() >= 2 && simulating)) {
		int host, server, client;
		engine->GetTicks(host, server, client);
		console->Print("CServerGameDLL::GameFrame %s (host=%d server=%d client=%d)\n", simulating ? "simulating" : "non-simulating", host, server, client);
	}

	int tick = session->GetTick();

	server->isSimulating = simulating;

	Event::Trigger<Event::PRE_TICK>({simulating, tick});

	auto result = Server::GameFrame(thisptr, simulating);

	Event::Trigger<Event::POST_TICK>({simulating, tick});

	++server->tickCount;

	return result;
}

// CServerGameDLL::ApplyGameSettings
DETOUR(Server::ApplyGameSettings, KeyValues *pKV) {
	return Server::ApplyGameSettings(thisptr, pKV);
}

DETOUR_T(void, Server::OnRemoveEntity, IHandleEntity *ent, CBaseHandle handle) {
	auto info = entityList->GetEntityInfoByIndex(handle.GetEntryIndex());
	bool hasSerialChanged = false;
	
	for (int i = g_ent_slot_serial.size() - 1; i >= 0; i--) {
		if (handle.GetEntryIndex() == g_ent_slot_serial[i].slot && !g_ent_slot_serial[i].done) {
			hasSerialChanged = true;
			info->m_SerialNumber = g_ent_slot_serial[i].serial - 1;
			console->Print("Serial number of slot %d has been set to %d.\n", g_ent_slot_serial[i].slot, g_ent_slot_serial[i].serial);
			g_ent_slot_serial[i].done = true;
		}
	}

	return Server::OnRemoveEntity(thisptr, ent, handle);
}

static int (*GlobalEntity_GetIndex)(const char *);
static void (*GlobalEntity_SetFlags)(int, int);

static void resetCoopProgress() {
	GlobalEntity_SetFlags(GlobalEntity_GetIndex("glados_spoken_flags0"), 0);
	GlobalEntity_SetFlags(GlobalEntity_GetIndex("glados_spoken_flags1"), 0);
	GlobalEntity_SetFlags(GlobalEntity_GetIndex("glados_spoken_flags2"), 0);
	GlobalEntity_SetFlags(GlobalEntity_GetIndex("glados_spoken_flags3"), 0);
	GlobalEntity_SetFlags(GlobalEntity_GetIndex("have_seen_dlc_tubes_reveal"), 0);
	engine->ExecuteCommand("mp_mark_all_maps_incomplete", true);
	engine->ExecuteCommand("mp_lock_all_taunts", true);
}

static int g_sendResetDoneAt = -1;

ON_EVENT(SESSION_START) {
	g_sendResetDoneAt = -1;
}

ON_EVENT(POST_TICK) {
	if (g_sendResetDoneAt != -1 && session->GetTick() >= g_sendResetDoneAt) {
		g_sendResetDoneAt = -1;
		NetMessage::SendMsg(RESET_COOP_PROGRESS_MESSAGE_TYPE, (void *)"done", 4);
	}
}

static void netResetCoopProgress(const void *data, size_t size) {
	if (size == 4 && !memcmp(data, "done", 4)) {
		Event::Trigger<Event::COOP_RESET_DONE>({});
	} else {
		resetCoopProgress();
		Event::Trigger<Event::COOP_RESET_REMOTE>({});
		g_sendResetDoneAt = session->GetTick() + 10; // send done message in 10 ticks, to be safe
	}
}

float hostTimeWrap() {
	return engine->GetHostTime();
}

static char g_orig_check_stuck_code[6];
static void *g_check_stuck_code;

bool Server::Init() {
	this->g_GameMovement = Interface::Create(this->Name(), "GameMovement001");
	this->g_ServerGameDLL = Interface::Create(this->Name(), "ServerGameDLL005");

	if (this->g_GameMovement) {
		this->g_GameMovement->Hook(Server::CheckJumpButton_Hook, Server::CheckJumpButton, Offsets::CheckJumpButton);
		this->g_GameMovement->Hook(Server::PlayerMove_Hook, Server::PlayerMove, Offsets::PlayerMove);

		this->g_GameMovement->Hook(Server::ProcessMovement_Hook, Server::ProcessMovement, Offsets::ProcessMovement);
		this->g_GameMovement->Hook(Server::GetPlayerViewOffset_Hook, Server::GetPlayerViewOffset, Offsets::GetPlayerViewOffset);
		this->g_GameMovement->Hook(Server::FinishGravity_Hook, Server::FinishGravity, Offsets::FinishGravity);
		this->g_GameMovement->Hook(Server::AirMove_Hook, Server::AirMove, Offsets::AirMove);

		auto ctor = this->g_GameMovement->Original(0);
		auto baseCtor = Memory::Read(ctor + Offsets::AirMove_Offset1);
		uintptr_t baseOffset;
		baseOffset = Memory::Deref<uintptr_t>(baseCtor + Offsets::AirMove_Offset2);
		Memory::Deref<_AirMove>(baseOffset + Offsets::AirMove * sizeof(uintptr_t *), &Server::AirMoveBase);

		Memory::Deref<_CheckJumpButton>(baseOffset + Offsets::CheckJumpButton * sizeof(uintptr_t *), &Server::CheckJumpButtonBase);

		uintptr_t airMove = (uintptr_t)AirMove;
#ifdef _WIN32
		if (p2fx.game->Is(SourceGame_Portal2)) {
			this->aircontrol_fling_speed_addr = *(float **)(airMove + 791);
		} else {
			this->aircontrol_fling_speed_addr = *(float **)(airMove + 662);
		}
#else
		if (p2fx.game->Is(SourceGame_EIPRelPIC)) {
			this->aircontrol_fling_speed_addr = *(float **)(airMove + 641);
		} else {
			this->aircontrol_fling_speed_addr = *(float **)(airMove + 524);
		}
#endif
		Memory::UnProtect(this->aircontrol_fling_speed_addr, 4);
	}

	if (auto g_ServerTools = Interface::Create(this->Name(), "VSERVERTOOLS001")) {
		auto GetIServerEntity = g_ServerTools->Original(Offsets::GetIServerEntity);
		Memory::Deref(GetIServerEntity + Offsets::m_EntPtrArray, &this->m_EntPtrArray);
#ifndef _WIN32
		if (p2fx.game->Is(SourceGame_EIPRelPIC)) {
			this->m_EntPtrArray = (CEntInfo *)((uintptr_t)this->m_EntPtrArray + 4);
		}
#endif

		this->gEntList = Interface::Create((void *)((uintptr_t)this->m_EntPtrArray - 4));
		this->gEntList->Hook(Server::OnRemoveEntity_Hook, Server::OnRemoveEntity, Offsets::OnRemoveEntity);

		this->CreateEntityByName = g_ServerTools->Original<_CreateEntityByName>(Offsets::CreateEntityByName);
		this->DispatchSpawn = g_ServerTools->Original<_DispatchSpawn>(Offsets::DispatchSpawn);
		this->SetKeyValueChar = g_ServerTools->Original<_SetKeyValueChar>(Offsets::SetKeyValueChar);
		this->SetKeyValueFloat = g_ServerTools->Original<_SetKeyValueFloat>(Offsets::SetKeyValueFloat);
		this->SetKeyValueVector = g_ServerTools->Original<_SetKeyValueVector>(Offsets::SetKeyValueVector);

		Interface::Delete(g_ServerTools);
	}

	if (this->g_ServerGameDLL) {
		auto Think = this->g_ServerGameDLL->Original(Offsets::Think);
		Memory::Read<_UTIL_PlayerByIndex>(Think + Offsets::UTIL_PlayerByIndex, &this->UTIL_PlayerByIndex);
		Memory::DerefDeref<CGlobalVars *>((uintptr_t)this->UTIL_PlayerByIndex + Offsets::gpGlobals, &this->gpGlobals);

		this->GetAllServerClasses = this->g_ServerGameDLL->Original<_GetAllServerClasses>(Offsets::GetAllServerClasses);
		this->IsRestoring = this->g_ServerGameDLL->Original<_IsRestoring>(Offsets::IsRestoring);

		this->g_ServerGameDLL->Hook(Server::GameFrame_Hook, Server::GameFrame, Offsets::GameFrame);
		this->g_ServerGameDLL->Hook(Server::ApplyGameSettings_Hook, Server::ApplyGameSettings, Offsets::ApplyGameSettings);
	}

#ifdef _WIN32
	GlobalEntity_GetIndex = (int (*)(const char *))Memory::Scan(server->Name(), "55 8B EC 51 8B 45 08 50 8D 4D FC 51 B9 ? ? ? ? E8 ? ? ? ? 66 8B 55 FC B8 FF FF 00 00", 0);
	GlobalEntity_SetFlags = (void (*)(int, int))Memory::Scan(server->Name(), "55 8B EC 80 3D ? ? ? ? 00 75 1F 8B 45 08 85 C0 78 18 3B 05 ? ? ? ? 7D 10 8B 4D 0C 8B 15 ? ? ? ? 8D 04 40 89 4C 82 08", 0);
#else
	if (p2fx.game->Is(SourceGame_EIPRelPIC)) {
		GlobalEntity_GetIndex = (int (*)(const char *))Memory::Scan(server->Name(), "53 83 EC 18 8D 44 24 0E 83 EC 04 FF 74 24 24 68 ? ? ? ? 50 E8 ? ? ? ? 0F B7 4C 24 1A 83 C4 0C 66 83 F9 FF 74 35 8B 15 ? ? ? ? 89 D0", 0);
		GlobalEntity_SetFlags = (void (*)(int, int))Memory::Scan(server->Name(), "80 3D ? ? ? ? 01 8B 44 24 04 74 1F 85 C0 78 1B 3B 05 ? ? ? ? 7D 13 8B 15 ? ? ? ? 8D 04 40", 0);
	} else {
		GlobalEntity_GetIndex = (int (*)(const char *))Memory::Scan(server->Name(), "55 89 E5 53 8D 45 F6 83 EC 24 8B 55 08 C7 44 24 04 ? ? ? ? 89 04 24 89 54 24 08", 0);
		GlobalEntity_SetFlags = (void (*)(int, int))Memory::Scan(server->Name(), "80 3D ? ? ? ? 00 55 89 E5 8B 45 08 75 1E 85 C0 78 1A 3B 05 ? ? ? ? 7D 12 8B 15", 0);
	}
#endif

	// Remove the limit on how quickly you can use 'say', and also hook it
	Command::Hook("say", Server::say_callback_hook, Server::say_callback);
#ifdef _WIN32
	uintptr_t insn_addr = (uintptr_t)say_callback + 52;
#else
	uintptr_t insn_addr = (uintptr_t)say_callback + (p2fx.game->Is(SourceGame_EIPRelPIC) ? 55 : 88);
#endif
	// This is the location of an ADDSD instruction which adds 0.66
	// to the current time. If we instead *subtract* 0.66, we'll
	// always be able to chat again! We can just do this by changing
	// the third byte from 0x58 to 0x5C, hence making the full
	// opcode start with F2 0F 5C.
	Memory::UnProtect((void *)(insn_addr + 2), 1);
	*(char *)(insn_addr + 2) = 0x5C;

	// find the TraceFirePortal function
#ifdef _WIN32
	TraceFirePortal = (_TraceFirePortal)Memory::Scan(server->Name(), "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC 38 07 00 00 56 57 8B F1", 0);
	FindPortal = (_FindPortal)Memory::Scan(server->Name(), "55 8B EC 0F B6 45 08 8D 0C 80 03 C9 53 8B 9C 09 ? ? ? ? 03 C9 56 57 85 DB 74 3C 8B B9 ? ? ? ? 33 C0 33 F6 EB 08", 0);
	Memory::UnProtect((void *)((uintptr_t)TraceFirePortal + 391), 1); // see setPortalsThruPortals
#else
	if (p2fx.game->Is(SourceGame_EIPRelPIC)) {
		TraceFirePortal = (_TraceFirePortal)Memory::Scan(server->Name(), "55 89 E5 57 56 8D B5 F4 F8 FF FF 53 81 EC 30 07 00 00 8B 45 14 6A 00 8B 5D 0C FF 75 08 56 89 85 D0 F8 FF FF", 0);
		FindPortal = (_FindPortal)Memory::Scan(server->Name(), "55 57 56 53 83 EC 2C 8B 44 24 40 8B 74 24 48 8B 7C 24 44 89 44 24 14 0F B6 C0 8D 04 80 89 74 24 0C C1 E0 02", 0);
		Memory::UnProtect((void *)((uintptr_t)TraceFirePortal + 388), 1); // see setPortalsThruPortals
	} else {
		TraceFirePortal = (_TraceFirePortal)Memory::Scan(server->Name(), "55 89 E5 57 56 8D BD F4 F8 FF FF 53 81 EC 3C 07 00 00 8B 45 14 C7 44 24 08 00 00 00 00 89 3C 24 8B 5D 0C", 0);
		FindPortal = (_FindPortal)Memory::Scan(server->Name(), "55 89 E5 57 56 53 83 EC 2C 8B 45 08 8B 7D 0C 8B 4D 10 89 45 D8 0F B6 C0 8D 04 80 89 7D E0 C1 E0 02 89 4D DC", 0);
		Memory::UnProtect((void *)((uintptr_t)TraceFirePortal + 462), 1); // see setPortalsThruPortals
	}
#endif

#ifdef _WIN32
	ViewPunch = (decltype (ViewPunch))Memory::Scan(server->Name(), "55 8B EC A1 ? ? ? ? 83 EC 0C 83 78 30 00 56 8B F1 0F 85 ? ? ? ? 8B 16 8B 82 00 05 00 00 FF D0 84 C0 0F 85 ? ? ? ? 8B 45 08 F3 0F 10 1D ? ? ? ? F3 0F 10 00");
#else
	if (p2fx.game->Is(SourceGame_EIPRelPIC)) {
		ViewPunch = (decltype (ViewPunch))Memory::Scan(server->Name(), "55 57 56 53 83 EC 1C A1 ? ? ? ? 8B 5C 24 30 8B 74 24 34 8B 40 30 85 C0 75 38 8B 03 8B 80 04 05 00 00 3D ? ? ? ? 75 36 8B 83 B8 0B 00 00 8B 0D ? ? ? ?");
	} else {
		ViewPunch = (decltype (ViewPunch))Memory::Scan(server->Name(), "55 89 E5 53 83 EC 24 A1 ? ? ? ? 8B 5D 08 8B 40 30 85 C0 74 0A 83 C4 24 5B 5D C3 8D 74 26 00 8B 03 89 1C 24 FF 90 04 05 00 00 84 C0 75 E7 8B 45 0C");
	}
#endif
	g_ViewPunch_Hook.SetFunc(ViewPunch);

	{
		// a call to Plat_FloatTime in CGameMovement::CheckStuck
#ifdef _WIN32
		uintptr_t code = Memory::Scan(this->Name(), "FF ? ? ? ? ? D9 5D F8 8B 56 04 8B 42 1C 8B ? ? ? ? ? 3B C3 75 04 33 C9 EB 08 8B C8 2B 4A 58 C1 F9 04 F3 0F 10 84 CE 70", 0);
#else
		uintptr_t code;
		if (p2fx.game->Is(SourceGame_EIPRelPIC)) {
			code = Memory::Scan(this->Name(), "E8 ? ? ? ? 8B 43 04 66 0F EF C0 DD 5C 24 08 F2 0F 5A 44 24 08 8B 40 24 85 C0 0F 84 CC 01 00 00 8B 15 ? ? ? ? 2B 42 58", 0);
		} else {
			code = Memory::Scan(this->Name(), "E8 ? ? ? ? 8B 43 04 DD 9D ? ? ? ? F2 0F 10 B5 ? ? ? ? 8B 50 24 66 0F 14 F6 66 0F 5A CE 85 D2", 0);
		}
#endif
		Memory::UnProtect((void *)code, sizeof g_orig_check_stuck_code);
		memcpy(g_orig_check_stuck_code, (void *)code, sizeof g_orig_check_stuck_code);

		*(uint8_t *)code = 0xE8;
		*(uint32_t *)(code + 1) = (uint32_t)&hostTimeWrap - (code + 5);
#ifdef _WIN32
		*(uint8_t *)(code + 5) = 0x90; // nop
#endif

		g_check_stuck_code = (void *)code;
	}

	if (p2fx.game->Is(SourceGame_Portal2)) {
#ifdef _WIN32
		Server::IsInPVS = (Server::_IsInPVS)Memory::Scan(this->Name(), "55 8B EC 51 53 8B 5D 08 56 57 33 FF 89 4D FC 66 39 79 1A 75 57 3B BB 10 20 00 00 0F 8D C0 00 00 00 8D B3 14 20 00 00");
#else
		// this only really matters for coop so this pattern is just for the base game
		Server::IsInPVS = (Server::_IsInPVS)Memory::Scan(this->Name(), "55 57 56 53 31 DB 83 EC 0C 8B 74 24 20 8B 7C 24 24 66 83 7E 1A 00 8B 87 10 20 00 00 89 C2 0F 85 BC 00 00 00 85 C0 7F 75 8D B4 26");
#endif
		g_IsInPVS_Hook.SetFunc(IsInPVS);
	}

	NetMessage::RegisterHandler(RESET_COOP_PROGRESS_MESSAGE_TYPE, &netResetCoopProgress);

	sv_cheats = Variable("sv_cheats");
	sv_footsteps = Variable("sv_footsteps");
	sv_alternateticks = Variable("sv_alternateticks");
	sv_bonus_challenge = Variable("sv_bonus_challenge");
	sv_accelerate = Variable("sv_accelerate");
	sv_airaccelerate = Variable("sv_airaccelerate");
	sv_paintairacceleration = Variable("sv_paintairacceleration");
	sv_friction = Variable("sv_friction");
	sv_maxspeed = Variable("sv_maxspeed");
	sv_stopspeed = Variable("sv_stopspeed");
	sv_maxvelocity = Variable("sv_maxvelocity");
	sv_gravity = Variable("sv_gravity");

	return this->hasLoaded = this->g_GameMovement && this->g_ServerGameDLL;
}
DETOUR_COMMAND(Server::say) {
	if (args.ArgC() != 2 || Utils::StartsWith(args[1], "!SAR:") || !networkManager.HandleGhostSay(args[1])) {
		Server::say_callback(args);
	}
}
void Server::Shutdown() {
	Command::Unhook("say", Server::say_callback);
	if (g_check_stuck_code) memcpy(g_check_stuck_code, g_orig_check_stuck_code, sizeof g_orig_check_stuck_code);
	Interface::Delete(this->gEntList);
	Interface::Delete(this->g_GameMovement);
	Interface::Delete(this->g_ServerGameDLL);
}

Server *server;
