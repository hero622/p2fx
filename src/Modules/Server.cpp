#include "Server.hpp"

#include "Client.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Features/Camera.hpp"
#include "Features/EntityList.hpp"
#include "Features/FovChanger.hpp"
#include "Features/Hud/Crosshair.hpp"
#include "Features/Session.hpp"
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
REDECL(Server::GameFrame);
REDECL(Server::OnRemoveEntity);
REDECL(Server::ProcessMovement);
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

// CGameMovement::ProcessMovement
DETOUR(Server::ProcessMovement, void *player, CMoveData *move) {
	int slot = server->GetSplitScreenPlayerSlot(player);
	Event::Trigger<Event::PROCESS_MOVEMENT>({ slot, true });

	return Server::ProcessMovement(thisptr, player, move);
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

// CServerGameDLL::GameFrame
DETOUR(Server::GameFrame, bool simulating)
{
	if (!IsAcceptInputTrampolineInitialized) InitAcceptInputTrampoline();
	if (!g_IsCMFlagHookInitialized) InitCMFlagHook();

	int tick = session->GetTick();

	server->isSimulating = simulating;

	Event::Trigger<Event::PRE_TICK>({simulating, tick});

	auto result = Server::GameFrame(thisptr, simulating);

	Event::Trigger<Event::POST_TICK>({simulating, tick});

	++server->tickCount;

	return result;
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
		this->g_GameMovement->Hook(Server::ProcessMovement_Hook, Server::ProcessMovement, Offsets::ProcessMovement);
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
	if (args.ArgC() != 2 || Utils::StartsWith(args[1], "!SAR:")) {
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
