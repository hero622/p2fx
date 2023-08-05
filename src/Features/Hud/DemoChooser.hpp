#pragma once
#include "Hud.hpp"

#include "Variable.hpp"

class DemoChooser : public Hud {
public:
	DemoChooser();
	bool ShouldDraw() override;
	void Paint(int slot) override;
	bool GetCurrentSize(int &xSize, int &ySize) override;
};

extern DemoChooser demoChooser;