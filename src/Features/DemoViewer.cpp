#include "DemoViewer.hpp"

#include "Event.hpp"

#include "Modules/Engine.hpp"
// #include "Modules/InputSystem.hpp"

#include "Demo/DemoParser.hpp"
#include "Camera.hpp"

DemoViewer *demoViewer;

DemoViewer::DemoViewer() {
	this->hasLoaded = true;
}

void DemoViewer::Think() {
	if (!engine->demoplayer->IsPlaying())
		return;

	// replace GetAsyncKeyState for IsButtonDown later

	if (GetAsyncKeyState(VK_F3) & 1) {
		engine->ExecuteCommand("incrementvar sar_cam_control 0 3 1");
	}

	if (GetAsyncKeyState(VK_LEFT) & 1) {
		for (auto itr = camera->states.rbegin(); itr != camera->states.rend(); ++itr) {
			if (itr->first < engine->demoplayer->GetTick()) {
				engine->ExecuteCommand("demo_gototick 0");
				gotoTick = itr->first;
				return;
			}
		}

		engine->ExecuteCommand("demo_gototick 0");
		gotoTick = 0;
	}

	if (GetAsyncKeyState(VK_DOWN) & 1) {
		engine->ExecuteCommand("incrementvar host_timescale 0.1 2.0 -0.1");
	}

	if (GetAsyncKeyState(VK_SPACE) & 1) {
		engine->ExecuteCommand("demo_togglepause");
	}

	if (GetAsyncKeyState(VK_UP) & 1) {
		engine->ExecuteCommand("incrementvar host_timescale 0.1 2.0 0.1");
	}

	if (GetAsyncKeyState(VK_RIGHT) & 1) {
		for (const auto &state : camera->states) {
			if (state.first > engine->demoplayer->GetTick()) {
				engine->ExecuteCommand(Utils::ssprintf("demo_gototick %d", state.first).c_str());
				return;
			}
		}

		engine->ExecuteCommand(Utils::ssprintf("demo_gototick %d", g_demoPlaybackTicks - 1).c_str());
	}
}

void DemoViewer::OnDemoStart() {
	ParseDemoData();

	if (!(gotoTick > 0))
		return;

	engine->SendToCommandBuffer(Utils::ssprintf("demo_gototick %d", gotoTick).c_str(), 0);
}

void DemoViewer::ParseDemoData() {
	DemoParser parser;
	Demo demo;
	auto dir = std::string(engine->GetGameDirectory()) + std::string("/") + std::string(engine->demoplayer->DemoName);
	if (parser.Parse(dir, &demo)) {
		parser.Adjust(&demo);

		g_demoPlaybackTicks = demo.playbackTicks;
		g_demoStart = demo.firstPositivePacketTick;
	}
}

ON_EVENT(FRAME) {
	demoViewer->Think();
}

ON_EVENT(DEMO_START) {
	demoViewer->OnDemoStart();
}