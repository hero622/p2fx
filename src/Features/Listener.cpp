#include "Listener.hpp"

#include "Command.hpp"
#include "Features/Session.hpp"
#include "Modules/Engine.hpp"
#include "Utils/SDK.hpp"
#include "Variable.hpp"

#include <cstring>

static const char *EVENTS[] = {
	"player_spawn_blue",
	"player_spawn_orange",
};

Listener *listener;

Listener::Listener()
	: m_bRegisteredForEvents(false)
	, installedChangeCallback(false)
	, changeCount(0) {
	this->hasLoaded = true;
}
void Listener::Init() {
	if (engine->hasLoaded && engine->AddListener && !this->m_bRegisteredForEvents) {
		for (const auto &event : EVENTS) {
			this->m_bRegisteredForEvents = engine->AddListener(engine->s_GameEventManager->ThisPtr(), this, event, true);

			if (this->m_bRegisteredForEvents) {
				//console->DevMsg("P2FX: Added event listener for %s!\n", event);
			} else {
				console->DevWarning("P2FX: Failed to add event listener for %s!\n", event);
				break;
			}
		}
	}

	/*if (tier1->hasLoaded && tier1->InstallGlobalChangeCallback && !this->installedChangeCallback) {
        tier1->InstallGlobalChangeCallback(tier1->g_pCVar->ThisPtr(), (FnChangeCallback_t)Listener::OnCheatsChanged);
        installedChangeCallback = true;
    }*/
}
void Listener::Shutdown() {
	if (engine->hasLoaded && engine->RemoveListener && this->m_bRegisteredForEvents) {
		engine->RemoveListener(engine->s_GameEventManager->ThisPtr(), this);
		this->m_bRegisteredForEvents = false;
	}

	/*if (tier1->hasLoaded && tier1->RemoveGlobalChangeCallback && this->installedChangeCallback) {
        tier1->RemoveGlobalChangeCallback(tier1->g_pCVar->ThisPtr(), (FnChangeCallback_t)Listener::OnCheatsChanged);
        this->installedChangeCallback = false;
    }*/
}
Listener::~Listener() {
	this->Shutdown();
}
void Listener::FireGameEvent(IGameEvent *ev) {
	if (!ev) {
		return;
	}
}
int Listener::GetEventDebugID() {
	return 42;
}
void Listener::Reset() {
	this->changeCount = 0;
}
void Listener::OnCheatsChanged(IConVar *pVar, const char *pOldString, float flOldValue) {
	//if (!std::strcmp(pVar->GetName(), "ss_force_primary_fullscreen")) {
	//    if (!session->isRunning && engine->GetMaxClients() >= 2 && ++listener->changeCount == 3) {
	//        console->Print("Session started! (coop)\n");
	//        //session->Start();
	//    }
	//}
}