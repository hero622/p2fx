#include "DemoChooser.hpp"

#include "Modules/Scheme.hpp"
#include "Modules/Surface.hpp"
#include "Modules/Engine.hpp"
#include "Modules/InputSystem.hpp"

#include "Input.hpp"
#include "Menu.hpp"

DemoChooser demoChooser;

DemoChooser::DemoChooser()
	: Hud(HudType_Menu, false) {
}

bool DemoChooser::ShouldDraw() {
	return surface->g_isInMainMenu && Hud::ShouldDraw();
}

bool PointInBounds(int x0, int y0, int x1, int y1, int x, int y) {
	if (x > x0 && x < x1 && y > y0 && y < y1)
		return true;

	return false;
}

void DemoChooser::Paint(int slot) {
	// this is so dumb tbh

	auto font = scheme->GetFontByID(91);

	Color clr = {120, 120, 120};
	Color focusClr = {255, 255, 255};
	Color cursorClr = {255, 255, 255, 15};

	int w, h;
	engine->GetScreenSize(nullptr, w, h);

	int mx, my;
	inputSystem->GetCursorPos(mx, my);

	int y = h - 496 * h / 1080;

	int x0 = 198 * w / 1920;
	int y0 = y - 4 * h / 1080;
	int x1 = 828 * w / 1920;
	int y1 = y + 52 * h / 1080;

	static bool hover = false;

	hover = PointInBounds(x0, y0, x1, y1, mx, my);

	if (Input::keys[VK_LBUTTON].IsPressed() && hover) {
		Menu::g_fileDialog.Open();
		surface->SetAllMenuPanelState(false);
	} else if (!Menu::g_fileDialog.IsOpened()) {
		surface->SetAllMenuPanelState(true);
	}

	if (surface->g_hide)
		return;

	if (hover)
		surface->DrawRect(cursorClr, x0, y0, x1, y1);
	surface->DrawTxt(font, 220 * w / 1920, y, hover ? focusClr : clr, "PLAY DEMO");
}

bool DemoChooser::GetCurrentSize(int &xSize, int &ySize) {
	return false;
}