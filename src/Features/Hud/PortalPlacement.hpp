#pragma once

#include "Command.hpp"
#include "Hud.hpp"
#include "Variable.hpp"

class PortalPlacementHud : public Hud {
public:
    PortalPlacementHud();
    bool ShouldDraw() override;
	void Paint(int slot) override;
	bool GetCurrentSize(int &xSize, int &ySize) override;
};

extern PortalPlacementHud portalplacementHud;

extern Variable p2fx_pp_hud;
extern Variable p2fx_pp_hud_show_blue;
extern Variable p2fx_pp_hud_show_orange;
extern Variable p2fx_pp_hud_x;
extern Variable p2fx_pp_hud_y;
extern Variable p2fx_pp_hud_opacity;
extern Variable p2fx_pp_hud_font;
