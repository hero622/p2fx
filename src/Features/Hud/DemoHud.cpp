#include "DemoHud.hpp"

#include "Event.hpp"

#include "Modules/Scheme.hpp"
#include "Modules/Surface.hpp"
#include "Modules/Engine.hpp"

#include "Features/DemoViewer.hpp"
#include "Features/Camera.hpp"

#include "Variable.hpp"

#define DRAW_MULTI_COLOR_TEXT(font, x, y, clr1, clr2, txt1, txt2) \
	surface->DrawTxt(font, x, y, clr1, txt1);                        \
	surface->DrawTxt(font, x + surface->GetFontLength(font, txt1), y, clr2, txt2);

#define DRAW_CENTERED_TEXT(font, x, y, clr, ...) \
	surface->DrawTxt(font, x - surface->GetFontLength(font, __VA_ARGS__) / 2, y, clr, __VA_ARGS__);

DemoHud demoHud;

DemoHud::DemoHud()
	: Hud(HudType_InGame | HudType_Paused | HudType_Menu, false) {
}

bool DemoHud::ShouldDraw() {
	return engine->demoplayer->IsPlaying() && Hud::ShouldDraw();
}

void DemoHud::Paint(int slot) {
	int playbackTicks = demoViewer->g_demoPlaybackTicks;

	if (!(playbackTicks > 0))
		return;

	int screenX, screenY;

	engine->GetScreenSize(nullptr, screenX, screenY);

	const int w = 600;
	const int h = 120;

	int x = screenX / 2 - w / 2;
	int y = screenY - h;

	auto font = scheme->GetFontByID(62);
	auto iconFont = scheme->GetFontByID(168);
	
	const Color white = {255, 255, 255};
	const Color accent = {45, 140, 235};

	DRAW_MULTI_COLOR_TEXT(font, x, y - 30, accent, white, "F4 ", "Toggle Controls");

	int icmtxLength = surface->GetFontLength(font, "F Insert Camera Marker");
	int icmtxPos = w / 2 - icmtxLength / 2;
	DRAW_MULTI_COLOR_TEXT(font, x + icmtxPos, y - 30, accent, white, "F ", "Insert Camera Marker");

	int tghtxLength = surface->GetFontLength(font, "F2 Toggle Hud");
	int tghtxPos = w - tghtxLength;
	DRAW_MULTI_COLOR_TEXT(font, x + tghtxPos, y - 30, accent, white, "F2 ", "Toggle Hud");

	surface->DrawRect({0, 0, 0, 200}, x, y, x + w, y + h);

	y += 22;

	surface->DrawRect({70, 70, 70}, x + 16, y, x + w - 16, y + 64);
	surface->DrawRect({28, 28, 28}, x + 17, y + 1, x + w - 17, y + 63);

	x += 16;

	int pbtLength = surface->GetFontLength(font, "%d", playbackTicks);

	surface->DrawRect({70, 70, 70}, x + 8, y + 8, x + w - pbtLength - 48, y + 26);
	surface->DrawRect({20, 20, 20}, x + 9, y + 9, x + w - pbtLength - 49, y + 25);
	surface->DrawRect({0, 0, 0}, x + 9, y + 16, x + w - pbtLength - 49, y + 25);

	int timelineSize = x + w - pbtLength - 49 - (x + 9);

	int tick = engine->demoplayer->GetTick();

	int demoStart = demoViewer->g_demoStart;
	float demoStartFrac = (float)demoStart / (float)playbackTicks;
	float demoStartPos = demoStartFrac * timelineSize;

	surface->DrawRect({100, 0, 0}, x + 9, y + 9, x + demoStartPos, y + 25);

	surface->DrawTxt(font, x + w - pbtLength - 40, y + 3, white, "%d", playbackTicks);

	float playbackFrac = (float)tick / (float)playbackTicks;
	int playbackMarker = playbackFrac * timelineSize;

	surface->DrawColoredLine(x + 9 + playbackMarker - 1, y + 6, x + 9 + playbackMarker + 1, y + 6, white);
	surface->DrawColoredLine(x + 9 + playbackMarker - 2, y + 5, x + 9 + playbackMarker + 2, y + 5, white);
	surface->DrawColoredLine(x + 9 + playbackMarker - 3, y + 4, x + 9 + playbackMarker + 3, y + 4, white);
	surface->DrawColoredLine(x + 9 + playbackMarker - 4, y + 3, x + 9 + playbackMarker + 4, y + 3, white);
	surface->DrawColoredLine(x + 9 + playbackMarker - 5, y + 2, x + 9 + playbackMarker + 5, y + 2, white);
	surface->DrawColoredLine(x + 9 + playbackMarker - 6, y + 1, x + 9 + playbackMarker + 6, y + 1, white);
	surface->DrawColoredLine(x + 9 + playbackMarker - 7, y, x + 9 + playbackMarker + 7, y, white);

	DRAW_CENTERED_TEXT(font, x + 9 + playbackMarker, y - 24, white, "%d", tick);

	for (auto const &state : camera->states) {
		int keyframeTick = state.first;
		float keyframeFrac = (float)keyframeTick / (float)playbackTicks;
		int keyframeMarker = keyframeFrac * timelineSize;

		surface->DrawRect(white, x + 9 + keyframeMarker, y + 9, x + 10 + keyframeMarker, y + 25);
	}

	y += 30;

	surface->DrawRect({20, 20, 20}, x + 8, y, x + 168, y + 30);
	surface->DrawRect({36, 36, 36}, x + 10, y + 2, x + 166, y + 28);

	auto controlType = camera->controlType;
	const char *controlStr = "";
	switch (controlType) {
	case Default: controlStr = "DEFAULT"; break;
	case Drive: controlStr = "DRIVE"; break;
	case Cinematic: controlStr = "CINEMATIC"; break;
	case Follow: controlStr = "FOLLOW"; break;
	}
	DRAW_CENTERED_TEXT(font, x + 88, y + 2, white, "%s", controlStr);

	surface->DrawRect({20, 20, 20}, x + 172, y, x + 438, y + 30);
	surface->DrawRect({36, 36, 36}, x + 174, y + 2, x + 436, y + 28);

	surface->DrawTxt(iconFont, x + 202, y - 19, white, "Y");

	float host_timescale = Variable("host_timescale").GetFloat();
	surface->DrawTxt(font, x + 243, y + 2, white, "%.1fx", host_timescale);

	if (engine->demoplayer->IsPaused())
		surface->DrawTxt(iconFont, x + 307, y - 19, white, "R");
	else
		surface->DrawTxt(iconFont, x + 306, y - 19, white, "P");

	surface->DrawTxt(iconFont, x + 348, y - 19, white, "S");
	surface->DrawTxt(iconFont, x + 395, y - 19, white, "X");

	surface->DrawRect({20, 20, 20}, x + 442, y, x + 560, y + 30);
	surface->DrawRect({36, 36, 36}, x + 444, y + 2, x + 558, y + 28);
	DRAW_CENTERED_TEXT(font, x + 501, y + 2, white, "RECORD");

	y += 40;

	DRAW_CENTERED_TEXT(font, x + 88, y, accent, "F3");
	DRAW_CENTERED_TEXT(font, x + 207, y - 2, accent, "<");
	DRAW_CENTERED_TEXT(font, x + 260, y - 2, accent, "v");
	DRAW_CENTERED_TEXT(font, x + 312, y, accent, "SPACE");
	DRAW_CENTERED_TEXT(font, x + 356, y + 3, accent, "^");
	DRAW_CENTERED_TEXT(font, x + 402, y - 2, accent, ">");
	DRAW_CENTERED_TEXT(font, x + 501, y, accent, "R");
}

bool DemoHud::GetCurrentSize(int& xSize, int& ySize) {
	return false;
}