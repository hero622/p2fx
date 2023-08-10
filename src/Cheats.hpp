#pragma once
#include "Command.hpp"
#include "Variable.hpp"

class Cheats {
public:
	void Init();
	void Shutdown();
};

extern Variable p2fx_disable_challenge_stats_hud;
extern Variable p2fx_disable_no_focus_sleep;
extern Variable p2fx_disable_weapon_sway;

extern Variable ui_loadingscreen_transition_time;
extern Variable ui_loadingscreen_fadein_time;
extern Variable ui_loadingscreen_mintransition_time;
extern Variable ui_transition_effect;
extern Variable ui_transition_time;
extern Variable hide_gun_when_holding;
extern Variable cl_viewmodelfov;
extern Variable r_flashlightbrightness;
extern Variable mat_hdr_manual_tonemap_rate;
extern Variable mat_motion_blur_enabled;