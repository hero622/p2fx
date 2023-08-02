#pragma once
#include "Command.hpp"
#include "Hud.hpp"
#include "Variable.hpp"

#include <climits>


class LPHud : public Hud {
public:
	LPHud();
	bool ShouldDraw() override;
	void Paint(int slot) override;
	bool GetCurrentSize(int &xSize, int &ySize) override;
	void Set(int count);
};

extern LPHud lpHud;

extern Variable p2fx_lphud;
extern Variable p2fx_lphud_x;
extern Variable p2fx_lphud_y;
extern Variable p2fx_lphud_font;

extern Command p2fx_lphud_set;
