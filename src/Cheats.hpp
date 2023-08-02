#pragma once
#include "Command.hpp"
#include "Variable.hpp"

class Cheats {
public:
	void Init();
	void Shutdown();

	static void PatchBhop(int slot, void *player, CUserCmd *cmd);
	static void AutoStrafe(int slot, void *player, CUserCmd *cmd);
};

extern Variable p2fx_autorecord;
extern Variable p2fx_autojump;
extern Variable p2fx_jumpboost;
extern Variable p2fx_aircontrol;
extern Variable p2fx_duckjump;
extern Variable p2fx_disable_challenge_stats_hud;
extern Variable p2fx_disable_steam_pause;
extern Variable p2fx_disable_no_focus_sleep;
extern Variable p2fx_disable_progress_bar_update;
extern Variable p2fx_prevent_mat_snapshot_recompute;
extern Variable p2fx_challenge_autostop;
extern Variable p2fx_show_entinp;
extern Variable p2fx_force_qc;
extern Variable p2fx_patch_bhop;
extern Variable p2fx_patch_cfg;
extern Variable p2fx_prevent_ehm;
extern Variable p2fx_disable_weapon_sway;

extern Variable sv_laser_cube_autoaim;
extern Variable ui_loadingscreen_transition_time;
extern Variable ui_loadingscreen_fadein_time;
extern Variable ui_loadingscreen_mintransition_time;
extern Variable ui_transition_effect;
extern Variable ui_transition_time;
extern Variable hide_gun_when_holding;
extern Variable cl_viewmodelfov;
extern Variable r_flashlightbrightness;

extern Command p2fx_togglewait;
