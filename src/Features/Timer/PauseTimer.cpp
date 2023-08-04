#include "PauseTimer.hpp"

#include "Event.hpp"
#include "Features/Hud/Hud.hpp"
#include "Features/Session.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

PauseTimer *pauseTimer;

PauseTimer::PauseTimer()
	: isActive(false)
	, ticks(0) {
	this->hasLoaded = true;
}
void PauseTimer::Start() {
	this->ticks = 0;
	this->isActive = true;
}
void PauseTimer::Increment() {
	++this->ticks;
}
void PauseTimer::Stop() {
	this->isActive = false;
	if (engine->demorecorder->isRecordingDemo && session->GetTick() != 0) {  // HACKHACK: pause timer currently gets triggered on every load, before session start. we gotta make that not happen
		char data[5];
		data[0] = 0x08;
		*(int *)(data + 1) = this->ticks;
		engine->demorecorder->RecordData(data, sizeof data);
	}
}
bool PauseTimer::IsActive() {
	return this->isActive;
}
int PauseTimer::GetTotal() {
	return this->ticks;
}

ON_EVENT(PRE_TICK) {
	if (engine->IsOrange() || engine->demoplayer->IsPlaying()) {
		return;
	}

	if (!server->IsRestoring()) {
		if (!event.simulating && !pauseTimer->IsActive()) {
			pauseTimer->Start();
		} else if (event.simulating && pauseTimer->IsActive()) {
			pauseTimer->Stop();
			console->DevMsg("Paused for %d non-simulated ticks.\n", pauseTimer->GetTotal());
		}
	}

	if (pauseTimer->IsActive() && session->isRunning) {
		pauseTimer->Increment();
	}
}
