#pragma once

#include "Categories.hpp"
#include "Command.hpp"
#include "Rules.hpp"
#include "Variable.hpp"

#include <string>

#define SPEEDRUN_TOAST_TAG "speedrun"

namespace SpeedrunTimer {
	std::string Format(float raw);
	std::string SimpleFormat(float raw);
	float UnFormat(const std::string &formatted_time);

	void Init();
	void SetIpt(float ipt);
	void Update();
	void AddPauseTick();
	void FinishLoad();

	int GetOffsetTicks();
	int GetSegmentTicks();
	int GetSplitTicks();
	int GetTotalTicks();

	void Start();
	void Pause();
	void Resume();
	void Stop(std::string segName);
	void Split(bool newSplit, std::string segName, bool requested = true);
	void Reset(bool requested = true);

	bool IsRunning();

	void OnLoad();
	void CategoryChanged();
};  // namespace SpeedrunTimer

extern Variable p2fx_speedrun_skip_cutscenes;
extern Variable p2fx_speedrun_smartsplit;
extern Variable p2fx_speedrun_time_pauses;
extern Variable p2fx_speedrun_stop_in_menu;
extern Variable p2fx_speedrun_start_on_load;
extern Variable p2fx_speedrun_offset;
extern Variable p2fx_speedrun_autostop;

extern Command p2fx_speedrun_start;
extern Command p2fx_speedrun_stop;
extern Command p2fx_speedrun_split;
extern Command p2fx_speedrun_pause;
extern Command p2fx_speedrun_resume;
extern Command p2fx_speedrun_reset;
extern Command p2fx_speedrun_result;
extern Command p2fx_speedrun_export;
