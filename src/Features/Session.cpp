#include "Session.hpp"

#include "Event.hpp"
#include "Features/Hud/Hud.hpp"
#include "Features/Listener.hpp"
#include "Features/NetMessage.hpp"
#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Utils/SDK.hpp"

#include <chrono>
#include <thread>

Variable p2fx_loads_uncap("p2fx_loads_uncap", "0", 0, 1, "Temporarily set fps_max to 0 during loads\n");
Variable p2fx_loads_norender("p2fx_loads_norender", "0", 0, 1, "Temporarily set mat_norendering to 1 during loads\n");

Session *session;

Session::Session()
	: baseTick(0)
	, lastSession(0)
	, isRunning(true)
	, currentFrame(0)
	, lastFrame(0)
	, prevState(HS_RUN)
	, signonState(SIGNONSTATE_FULL) {
	this->hasLoaded = true;
}
int Session::GetTick() {
	auto result = engine->GetTick() - this->baseTick;
	return (result >= 0) ? result : 0;
}
void Session::Rebase(const int from) {
	this->baseTick = from;
}
void Session::Started(bool menu) {
	if (this->isRunning) {
		return;
	}

	if (menu) {
		console->Print("Session started! (menu)\n");
		this->Rebase(engine->GetTick());

		if (!engine->IsOrange()) {
			this->ResetLoads();
		}

		this->isRunning = true;
	} else {
		console->Print("Session Started!\n");
		this->Start();
	}
}
void Session::Start() {
	if (this->isRunning) {
		return;
	}

	auto tick = engine->GetTick();

	this->Rebase(tick);

	Event::Trigger<Event::SESSION_START>({ engine->isLevelTransition, engine->tickLoadStarted == -2 });
	engine->isLevelTransition = false;
	if (engine->tickLoadStarted == -2) engine->tickLoadStarted = -1;

	engine->hasRecorded = false;
	engine->hasPaused = false;
	engine->isPausing = false;
	engine->startedTransitionFadeout = false;
	engine->forcedPrimaryFullscreen = false;
	server->tickCount = 0;

	this->currentFrame = 0;
	this->isRunning = true;
}
void Session::Ended() {
	if (!this->isRunning) {
		return;
	}

	this->previousMap = engine->GetCurrentMapName();

	auto tick = this->GetTick();

	engine->isLevelTransition |= engine->IsOrange();
	if (engine->tickLoadStarted != -1) engine->tickLoadStarted = -2;
	Event::Trigger<Event::SESSION_END>({ engine->isLevelTransition, engine->tickLoadStarted == -2 });

	if (tick != 0) {
		console->Print("Session: %i (%.3f)\n", tick, engine->ToTime(tick));
		this->lastSession = tick;
	}

	this->lastFrame = this->currentFrame;
	this->currentFrame = 0;

	NetMessage::SessionEnded();

	if (listener) {
		listener->Reset();
	}

	this->loadStart = NOW();
	if (!engine->demoplayer->IsPlaying() && !engine->IsOrange()) {
		this->DoFastLoads();
	}

	this->isRunning = false;
}
void Session::Changed() {
	console->DevMsg("m_currentState = %i\n", engine->hoststate->m_currentState);

	if (engine->hoststate->m_currentState == HS_CHANGE_LEVEL_SP || engine->hoststate->m_currentState == HS_CHANGE_LEVEL_MP || engine->hoststate->m_currentState == HS_GAME_SHUTDOWN) {
		this->Ended();
	} else if (engine->hoststate->m_currentState == HS_RUN && !engine->hoststate->m_activeGame && engine->GetMaxClients() <= 1) {
		this->Started(true);
	}
}
void Session::Changed(int state) {
	console->DevMsg("state = %i\n", state);
	this->signonState = state;

	// Demo recorder starts syncing from this tick
	if (state == SIGNONSTATE_FULL) {
		this->Started();
		this->loadEnd = NOW();

		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(this->loadEnd - this->loadStart).count();
		console->DevMsg("Load took: %dms\n", time);
	} else if (state == SIGNONSTATE_PRESPAWN) {
		this->ResetLoads();
	} else {
		this->Ended();
	}
}

void Session::DoFastLoads() {
	if (p2fx_loads_uncap.GetBool()) {
		this->oldFpsMax = fps_max.GetInt();
		fps_max.SetValue(0);
	}

	if (p2fx_loads_norender.GetBool()) {
		mat_norendering.SetValue(1);
	}
}

void Session::ResetLoads() {
	if (p2fx_loads_uncap.GetBool() && fps_max.GetInt() == 0) {
		fps_max.SetValue(this->oldFpsMax);
	}

	if (p2fx_loads_norender.GetBool()) {
		mat_norendering.SetValue(0);
	}
}
