#include "DemoViewer.hpp"

#include "Event.hpp"

#include "Modules/Engine.hpp"
#include "Modules/InputSystem.hpp"

#include "Hud/DemoHud.hpp"
#include "Camera.hpp"
#include "Renderer.hpp"

#include "Input.hpp"
#include "Menu.hpp"

DemoViewer *demoViewer;

DemoViewer::DemoViewer() {
	this->hasLoaded = true;
}

void DemoViewer::Think() {
	if (!engine->demoplayer->IsPlaying())
		return;

	if (Input::keys[VK_F4].IsPressed()) {
		demoHud.g_shouldDraw = !demoHud.g_shouldDraw;
	}

	if (Input::keys[0x46].IsPressed()) {
		if (camera->controlType == Drive) {
			for (const auto &state : camera->states) {
				float dist = (camera->currentState.origin - state.second.origin).Length();
				if (dist < 50.0f) {
					engine->ExecuteCommand("p2fx_cam_path_remkf", true);
					return;
				}
			}

			engine->ExecuteCommand("p2fx_cam_path_setkf", true);
		}
	}

	if (Input::keys[VK_F2].IsPressed()) {
		Menu::g_shouldDraw = !Menu::g_shouldDraw;

		if (Menu::g_shouldDraw)
			inputSystem->UnlockCursor();
		else
			inputSystem->LockCursor();
	}

	if (Input::keys[VK_F3].IsPressed()) {
		engine->ExecuteCommand("incrementvar p2fx_cam_control 0 3 1", true);
	}

	if (Input::keys[VK_SPACE].IsPressed()) {
		if (engine->demoplayer->GetTick() == g_demoPlaybackTicks - 5) {
			engine->ExecuteCommand("demo_gototick 0", true);
			return;
		}

		engine->ExecuteCommand("demo_togglepause", true);
	}

	auto host_timescale = Variable("host_timescale");
	float timescale = host_timescale.GetFloat();
	if (Input::keys[VK_DOWN].IsPressed()) {
		timescale -= 0.2f;
	}
	if (Input::keys[VK_UP].IsPressed()) {
		timescale += 0.2f;
	}
	host_timescale.SetValue(std::clamp(timescale, 0.2f, 2.0f));
	
	if (Input::keys[VK_LEFT].IsPressed()) {
		for (auto itr = camera->states.rbegin(); itr != camera->states.rend(); ++itr) {
			if (itr->first < engine->demoplayer->GetTick()) {
				gotoTick = std::max(itr->first - 2, 0);
				return;
			}
		}

		engine->ExecuteCommand("demo_gototick 0", true);
		gotoTick = -1;
	}
	if (Input::keys[VK_RIGHT].IsPressed()) {
		for (const auto &state : camera->states) {
			if (state.first > engine->demoplayer->GetTick()) {
				engine->ExecuteCommand(Utils::ssprintf("sv_alternateticks 0; demo_gototick %d; demo_resume", state.first - 2, true).c_str());
				Scheduler::InHostTicks(1, [=]() {
					engine->ExecuteCommand("demo_pause; sv_alternateticks 1", true);
				});
				return;
			}
		}
	}
}

void DemoViewer::ParseDemoData() {
	DemoParser parser;
	Demo demo;
	auto dir = std::string(engine->GetGameDirectory()) + std::string("/") + std::string(engine->demoplayer->DemoName);
	if (parser.Parse(dir, &demo, true, &std::map<int, DataGhost>(), &CustomDatas())) {
		parser.Adjust(&demo);

		g_demoPlaybackTicks = demo.playbackTicks;
		g_demoStart = demo.firstPositivePacketTick;
		g_demoSpeedrunTime = parser.speedrunTime;
		g_demoSpeedrunLength = 0;
		for (size_t i = 0; i < g_demoSpeedrunTime.nSplits; ++i) {
			auto split = g_demoSpeedrunTime.splits[i];
			for (size_t j = 0; j < split.nSegments; ++j) {
				g_demoSpeedrunLength += split.segments[j].ticks;
			}
		}
	}
}

static bool g_waitingForSession = false;
ON_EVENT(SESSION_START) {
	g_waitingForSession = false;
}

void DemoViewer::HandleGotoTick() {
	if (gotoTick == -1) return;

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
			gotoTick = -1;
		}
	}
}

Variable p2fx_demo_pause_at_end("p2fx_demo_pause_at_end", "1", "Stops the game from exiting from demos after they have finished playing.\n");
void DemoViewer::PauseAtEnd() {
	if (!p2fx_demo_pause_at_end.GetBool())
		return;

	if (g_demoPlaybackTicks == -1)
		return;

	if (!engine->demoplayer->IsPaused() && engine->demoplayer->GetTick() == g_demoPlaybackTicks - 5) {
		engine->ExecuteCommand("demo_pause", true);
		Event::Trigger<Event::DEMO_STOP>({});
	}
}

ON_EVENT(FRAME) {
	demoViewer->Think();

	demoViewer->HandleGotoTick();

	demoViewer->PauseAtEnd();
}

ON_EVENT(DEMO_START) {
	demoViewer->ParseDemoData();
}