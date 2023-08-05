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

DECL_AUTO_COMMAND_COMPLETION(p2fx_fast_load_preset, ({"none", "sla", "normal", "full"}))
CON_COMMAND_F_COMPLETION(p2fx_fast_load_preset, "p2fx_fast_load_preset <preset> - sets all loading fixes to preset values\n", FCVAR_DONTRECORD, AUTOCOMPLETION_FUNCTION(p2fx_fast_load_preset)) {
	if (args.ArgC() != 2) {
		console->Print(p2fx_fast_load_preset.ThisPtr()->m_pszHelpString);
		return;
	}

	const char *preset = args.Arg(1);

#define CMD(x) engine->ExecuteCommand(x)
	if (!strcmp(preset, "none")) {
		if (!Game::IsSpeedrunMod()) {
			CMD("ui_loadingscreen_transition_time 1.0");
			CMD("ui_loadingscreen_fadein_time 1.0");
			CMD("ui_loadingscreen_mintransition_time 0.5");
		}
		CMD("p2fx_disable_progress_bar_update 0");
		CMD("p2fx_prevent_mat_snapshot_recompute 0");
		CMD("p2fx_loads_uncap 0");
		CMD("p2fx_loads_norender 0");
	} else if (!strcmp(preset, "sla")) {
		if (!Game::IsSpeedrunMod()) {
			CMD("ui_loadingscreen_transition_time 0.0");
			CMD("ui_loadingscreen_fadein_time 0.0");
			CMD("ui_loadingscreen_mintransition_time 0.0");
		}
		CMD("p2fx_disable_progress_bar_update 1");
		CMD("p2fx_prevent_mat_snapshot_recompute 1");
		CMD("p2fx_loads_uncap 0");
		CMD("p2fx_loads_norender 0");
	} else if (!strcmp(preset, "normal")) {
		if (!Game::IsSpeedrunMod()) {
			CMD("ui_loadingscreen_transition_time 0.0");
			CMD("ui_loadingscreen_fadein_time 0.0");
			CMD("ui_loadingscreen_mintransition_time 0.0");
		}
		CMD("p2fx_disable_progress_bar_update 1");
		CMD("p2fx_prevent_mat_snapshot_recompute 1");
		CMD("p2fx_loads_uncap 1");
		CMD("p2fx_loads_norender 0");
	} else if (!strcmp(preset, "full")) {
		if (!Game::IsSpeedrunMod()) {
			CMD("ui_loadingscreen_transition_time 0.0");
			CMD("ui_loadingscreen_fadein_time 0.0");
			CMD("ui_loadingscreen_mintransition_time 0.0");
		}
		CMD("p2fx_disable_progress_bar_update 2");
		CMD("p2fx_prevent_mat_snapshot_recompute 1");
		CMD("p2fx_loads_uncap 1");
		CMD("p2fx_loads_norender 1");
	} else {
		console->Print("Unknown preset %s!\n", preset);
		console->Print(p2fx_fast_load_preset.ThisPtr()->m_pszHelpString);
	}
#undef CMD
}

void Cheats::Init() {
	ui_loadingscreen_transition_time = Variable("ui_loadingscreen_transition_time");
	ui_loadingscreen_fadein_time = Variable("ui_loadingscreen_fadein_time");
	ui_loadingscreen_mintransition_time = Variable("ui_loadingscreen_mintransition_time");
	ui_transition_effect = Variable("ui_transition_effect");
	ui_transition_time = Variable("ui_transition_time");
	hide_gun_when_holding = Variable("hide_gun_when_holding");
	cl_viewmodelfov = Variable("cl_viewmodelfov");
	r_flashlightbrightness = Variable("r_flashlightbrightness");

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