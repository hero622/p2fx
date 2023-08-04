#include "Tier1.hpp"

#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "P2FX.hpp"
#include "Utils.hpp"
#include "Event.hpp"

bool Tier1::Init() {
	this->g_pCVar = Interface::Create(this->Name(), "VEngineCvar007", false);
	if (this->g_pCVar) {
		this->RegisterConCommand = this->g_pCVar->Original<_RegisterConCommand>(Offsets::RegisterConCommand);
		this->UnregisterConCommand = this->g_pCVar->Original<_UnregisterConCommand>(Offsets::UnregisterConCommand);
		this->FindCommandBase = this->g_pCVar->Original<_FindCommandBase>(Offsets::FindCommandBase);

		this->m_pConCommandList = *(ConCommandBase **)((uintptr_t)this->g_pCVar->ThisPtr() + Offsets::m_pConCommandList);

		auto listdemo = reinterpret_cast<ConCommand *>(this->FindCommandBase(this->g_pCVar->ThisPtr(), "listdemo"));
		if (listdemo) {
			this->ConCommand_VTable = *(void **)listdemo;
		}

		auto sv_lan = reinterpret_cast<ConVar *>(this->FindCommandBase(this->g_pCVar->ThisPtr(), "sv_lan"));
		if (sv_lan) {
			this->ConVar_VTable = *(void **)sv_lan;
			this->ConVar_VTable2 = sv_lan->ConVar_VTable;

			auto vtable =
#ifdef _WIN32
				&this->ConVar_VTable2;
#else
				&this->ConVar_VTable;
#endif

			this->Dtor = Memory::VMT<_Dtor>(vtable, Offsets::Dtor);
			this->Create = Memory::VMT<_Create>(vtable, Offsets::Create);
		}

		this->InstallGlobalChangeCallback = this->g_pCVar->Original<_InstallGlobalChangeCallback>(Offsets::InstallGlobalChangeCallback);
		this->RemoveGlobalChangeCallback = this->g_pCVar->Original<_RemoveGlobalChangeCallback>(Offsets::RemoveGlobalChangeCallback);

		auto tier1 = Memory::GetModuleHandleByName(this->Name());
		if (tier1) {
			this->KeyValuesSystem = Memory::GetSymbolAddress<_KeyValuesSystem>(tier1, "KeyValuesSystem");
		}
	}

	return this->hasLoaded = this->g_pCVar && this->ConCommand_VTable && this->ConVar_VTable && this->ConVar_VTable2;
}
void Tier1::Shutdown() {
	Interface::Delete(this->g_pCVar);
}

Tier1 *tier1;
