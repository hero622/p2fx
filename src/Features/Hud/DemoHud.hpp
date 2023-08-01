#pragma once
#include "Hud.hpp"
#include "Variable.hpp"

class DemoHud : public Hud {
public:
	bool g_shouldDraw = true;

	DemoHud();
	bool ShouldDraw() override;
	void Paint(int slot) override;
	bool GetCurrentSize(int &xSize, int &ySize) override;
};

extern DemoHud demoHud;