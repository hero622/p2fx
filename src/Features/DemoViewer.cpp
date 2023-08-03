#include "DemoViewer.hpp"

#include "Event.hpp"

#include "Modules/Engine.hpp"
// #include "Modules/InputSystem.hpp"

#include "Demo/DemoParser.hpp"
#include "Hud/DemoHud.hpp"
#include "Camera.hpp"

#include "Menu.hpp"

DemoViewer *demoViewer;

DemoViewer::DemoViewer() {
	this->hasLoaded = true;
}

void DemoViewer::Think() {
	if (!engine->demoplayer->IsPlaying())
		return;

	// replace GetAsyncKeyState for IsButtonDown later

	if (GetAsyncKeyState(VK_F4) & 1) {
		demoHud.g_shouldDraw = !demoHud.g_shouldDraw;
	}

	if (GetAsyncKeyState(0x46) & 1) {
		for (const auto &state : camera->states) {
			float dist = (camera->currentState.origin - state.second.origin).Length();
			if (dist < 50.0f) {
				engine->ExecuteCommand("p2fx_cam_path_remkf");
				return;
			}
		}

		engine->ExecuteCommand("p2fx_cam_path_setkf");
	}

	if (GetAsyncKeyState(VK_F2) & 1) {
		Menu::g_shouldDraw = !Menu::g_shouldDraw;
	}

	if (GetAsyncKeyState(VK_F3) & 1) {
		engine->ExecuteCommand("incrementvar p2fx_cam_control 0 3 1");
	}

	if (GetAsyncKeyState(VK_SPACE) & 1) {
		engine->ExecuteCommand("demo_togglepause");
	}

	auto host_timescale = Variable("host_timescale");
	float timescale = host_timescale.GetFloat();
	if (GetAsyncKeyState(VK_DOWN) & 1) {
		timescale -= 0.2f;
	}
	if (GetAsyncKeyState(VK_UP) & 1) {
		timescale += 0.2f;
	}
	host_timescale.SetValue(std::clamp(timescale, 0.2f, 2.0f));
	
	if (GetAsyncKeyState(VK_LEFT) & 1) {
		for (auto itr = camera->states.rbegin(); itr != camera->states.rend(); ++itr) {
			if (itr->first < engine->demoplayer->GetTick()) {
				gotoTick = itr->first - 2;
				return;
			}
		}

		engine->ExecuteCommand("demo_gototick 0");
		gotoTick = 0;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 1) {
		for (const auto &state : camera->states) {
			if (state.first > engine->demoplayer->GetTick()) {
				engine->ExecuteCommand(Utils::ssprintf("sv_alternateticks 0; demo_gototick %d; demo_resume; hwait 1 demo_pause; hwait 1 sv_alternateticks 1", state.first - 2).c_str());
				return;
			}
		}
	}
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

static bool g_waitingForSession = false;
ON_EVENT(SESSION_START) {
	g_waitingForSession = false;
}

void DemoViewer::HandleGotoTick() {
	if (!(gotoTick > 0)) return;

	// 0 = not done anything
	// 1 = queued skip
	// 2 = in skip
	static int state = 0;

	int tick = engine->demoplayer->GetTick();

	auto p2fx_demo_remove_broken = Variable("p2fx_demo_remove_broken");
	static int remove_broken_value = p2fx_demo_remove_broken.GetInt();

	if (state == 0) {
		if (tick > g_demoStart + 2) {
			g_waitingForSession = true;
			p2fx_demo_remove_broken.SetValue(0);
			engine->ExecuteCommand(Utils::ssprintf("demo_gototick %d; demo_pause", gotoTick).c_str());
			state = 1;
		}
	} else if (state == 1) {
		if (engine->hoststate->m_currentState == HS_RUN && !g_waitingForSession) {
			state = 2;
		}
	} else if (state == 2) {
		if (!engine->demoplayer->IsSkipping()) {
			if (tick % 2 == g_demoStart % 2) {
				engine->SendToCommandBuffer("sv_alternateticks 0; demo_resume", 0);
			}
			state = 3;
		}
	} else if (state == 3) {
		if (tick % 2 != g_demoStart % 2) {
			engine->SendToCommandBuffer("sv_alternateticks 1; demo_pause", 0);
			p2fx_demo_remove_broken.SetValue(remove_broken_value);
			state = 0;
			gotoTick = 0;
		}
	}
}

ON_EVENT(FRAME) {
	demoViewer->Think();

	demoViewer->HandleGotoTick();
}

ON_EVENT(DEMO_START) {
	demoViewer->ParseDemoData();
}