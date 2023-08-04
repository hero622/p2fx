#pragma once
#include "Game.hpp"
#include "Utils/SDK.hpp"
#include "Variable.hpp"

#include <array>
#include <vector>

enum HudType {
	HudType_NotSpecified = 0,
	HudType_InGame = (1 << 0),
	HudType_Paused = (1 << 1),
	HudType_Menu = (1 << 2),
	HudType_LoadingScreen = (1 << 3)
};

class BaseHud {
public:
	int type;
	bool drawSecondSplitScreen;
	int version;

public:
	BaseHud(int type, bool drawSecondSplitScreen, int version);
	virtual bool ShouldDraw();
};

class Hud : public BaseHud {
public:
	static std::vector<Hud *> &GetList();

public:
	Hud(int type, bool drawSecondSplitScreen = false, int version = SourceGame_Unknown);

public:
	virtual bool GetCurrentSize(int &xSize, int &ySize) = 0;
	virtual void Paint(int slot) = 0;
};