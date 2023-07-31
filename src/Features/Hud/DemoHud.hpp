#pragma once
#include "Hud.hpp"
#include "Variable.hpp"

class DemoHud : public Hud {
private:
	int playbackTicks;

public:
	DemoHud();
	bool ShouldDraw() override;
	void Paint(int slot) override;
	bool GetCurrentSize(int &xSize, int &ySize) override;

	void ParseDemoData();
};

extern DemoHud demoHud;