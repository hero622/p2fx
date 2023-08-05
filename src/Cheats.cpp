#include "Cheats.hpp"

#include "Event.hpp"
#include "Features/Cvars.hpp"
#include "Features/Hud/Hud.hpp"
#include "Features/Listener.hpp"
#include "Features/OverlayRender.hpp"
#include "Game.hpp"
#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Offsets.hpp"

#include <cstring>

Variable p2fx_disable_challenge_stats_hud("p2fx_disable_challenge_stats_hud", "0", "Disables opening the challenge mode stats HUD.\n");
Variable p2fx_disable_steam_pause("p2fx_disable_steam_pause", "0", "Prevents pauses from steam overlay.\n");
Variable p2fx_disable_no_focus_sleep("p2fx_disable_no_focus_sleep", "0", "Does not yield the CPU when game is not focused.\n");
Variable p2fx_disable_progress_bar_update("p2fx_disable_progress_bar_update", "0", 0, 2, "Disables excessive usage of progress bar.\n");
Variable p2fx_prevent_mat_snapshot_recompute("p2fx_prevent_mat_snapshot_recompute", "0", "Shortens loading times by preventing state snapshot recomputation.\n");
Variable p2fx_disable_weapon_sway("p2fx_disable_weapon_sway", "0", 0, 1, "Disables the viewmodel lagging behind.\n");

Variable ui_loadingscreen_transition_time;
Variable ui_loadingscreen_fadein_time;
Variable ui_loadingscreen_mintransition_time;
Variable ui_transition_effect;
Variable ui_transition_time;
Variable hide_gun_when_holding;
Variable cl_viewmodelfov;
Variable r_flashlightbrightness;
Variable mat_hdr_level;

void Cheats::Init() {
	ui_loadingscreen_transition_time = Variable("ui_loadingscreen_transition_time");
	ui_loadingscreen_fadein_time = Variable("ui_loadingscreen_fadein_time");
	ui_loadingscreen_mintransition_time = Variable("ui_loadingscreen_mintransition_time");
	ui_transition_effect = Variable("ui_transition_effect");
	ui_transition_time = Variable("ui_transition_time");
	hide_gun_when_holding = Variable("hide_gun_when_holding");
	cl_viewmodelfov = Variable("cl_viewmodelfov");
	r_flashlightbrightness = Variable("r_flashlightbrightness");
	mat_hdr_level = Variable("mat_hdr_level");

	engine->ExecuteCommand("ui_loadingscreen_transition_time 0.0");
	engine->ExecuteCommand("ui_loadingscreen_fadein_time 0.0");
	engine->ExecuteCommand("ui_loadingscreen_mintransition_time 0.0");
	engine->ExecuteCommand("p2fx_disable_progress_bar_update 2");
	engine->ExecuteCommand("p2fx_prevent_mat_snapshot_recompute 1");
	engine->ExecuteCommand("p2fx_loads_uncap 1");
	engine->ExecuteCommand("p2fx_loads_norender 1");

	p2fx_disable_challenge_stats_hud.UniqueFor(SourceGame_Portal2);

	p2fx_disable_weapon_sway.UniqueFor(SourceGame_Portal2);

	cvars->Unlock();

	Variable::RegisterAll();
	Command::RegisterAll();
}
void Cheats::Shutdown() {
	cvars->Lock();

	Variable::UnregisterAll();
	Command::UnregisterAll();
}

ON_EVENT(FRAME) {
	sv_cheats.SetValue(1);
}