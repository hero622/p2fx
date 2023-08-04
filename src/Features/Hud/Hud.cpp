#include "Hud.hpp"

#include "Features/Demo/GhostEntity.hpp"
#include "Features/Session.hpp"
#include "Features/Timer/PauseTimer.hpp"
#include "Features/EntityList.hpp"
#include "Modules/Client.hpp"
#include "Modules/Engine.hpp"
#include "Modules/EngineDemoPlayer.hpp"
#include "Modules/Scheme.hpp"
#include "Modules/Server.hpp"
#include "Modules/Surface.hpp"
#include "Modules/VGui.hpp"
#include "Event.hpp"
#include "Variable.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <optional>

#ifdef _WIN32
#	define strcasecmp _stricmp
#endif

BaseHud::BaseHud(int type, bool drawSecondSplitScreen, int version)
	: type(type)
	, drawSecondSplitScreen(drawSecondSplitScreen)
	, version(version) {
}
bool BaseHud::ShouldDraw() {
	if (engine->demoplayer->IsPlaying() || engine->IsOrange()) {
		return this->type & HudType_InGame;
	}

	if (!engine->hoststate->m_activeGame) {
		return this->type & HudType_Menu;
	}

	if (pauseTimer->IsActive()) {
		return this->type & HudType_Paused;
	}

	if (session->isRunning) {
		return this->type & HudType_InGame;
	}

	return this->type & HudType_LoadingScreen;
}

std::vector<Hud *> &Hud::GetList() {
	static std::vector<Hud *> list;
	return list;
}

Hud::Hud(int type, bool drawSecondSplitScreen, int version)
	: BaseHud(type, drawSecondSplitScreen, version) {
	Hud::GetList().push_back(this);
}

CON_COMMAND_F(p2fx_pip_align, "p2fx_pip_align <top|center|bottom> <left|center|right> - aligns the remote view.\n", FCVAR_DONTRECORD) {
	if (args.ArgC() != 3) {
		return console->Print(p2fx_pip_align.ThisPtr()->m_pszHelpString);
	}
	int sw, sh;
	engine->GetScreenSize(nullptr, sw, sh);
	float scale = Variable("ss_pipscale").GetFloat();
	float w = sw * scale, h = sh * scale, x, y;
	
	if (!strcasecmp(args[1], "top")) {
		y = sh - h - 25;
	} else if (!strcasecmp(args[1], "center")) {
		y = (sh - h) / 2;
	} else if (!strcasecmp(args[1], "bottom")) {
		y = 25;
	} else {
		return console->Print(p2fx_pip_align.ThisPtr()->m_pszHelpString);
	}

	if (!strcasecmp(args[2], "left")) {
		x = sw - w - 25;
	} else if (!strcasecmp(args[2], "center")) {
		x = (sw - w) / 2;
	} else if (!strcasecmp(args[2], "right")) {
		x = 25;
	} else {
		return console->Print(p2fx_pip_align.ThisPtr()->m_pszHelpString);
	}

	Variable("ss_pip_right_offset").SetValue(x);
	Variable("ss_pip_bottom_offset").SetValue(y);
}
