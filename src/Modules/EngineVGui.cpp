#include "EngineVGui.hpp"

#include "Features/Hud/Hud.hpp"
#include "Features/Session.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Modules/Surface.hpp"
#include "P2FX.hpp"

#include <algorithm>

REDECL(EngineVGui::Paint);
REDECL(EngineVGui::UpdateProgressBar);

void EngineVGui::Draw(Hud *const &hud) {
	if (hud->ShouldDraw()) {
		hud->Paint(0);
	}
}

// CEngineVGui::Paint
DETOUR(EngineVGui::Paint, PaintMode_t mode) {
	auto result = EngineVGui::Paint(thisptr, mode);

	surface->StartDrawing(surface->matsurface->ThisPtr());

	if ((mode & PAINT_UIPANELS)) {
		for (auto const &hud : enginevgui->huds) {
			enginevgui->Draw(hud);
		}
	}

	surface->FinishDrawing();

	return result;
}

DETOUR(EngineVGui::UpdateProgressBar, int progress) {
	if (enginevgui->lastProgressBar != progress) {
		enginevgui->lastProgressBar = progress;
		enginevgui->progressBarCount = 0;
	}
	enginevgui->progressBarCount++;
	return 0;
}

bool EngineVGui::IsUIVisible() {
	return this->IsGameUIVisible(this->ienginevgui->ThisPtr());
}

bool EngineVGui::Init() {
	this->ienginevgui = Interface::Create(this->Name(), "VEngineVGui001");

	if (this->ienginevgui) {
		this->IsGameUIVisible = this->ienginevgui->Original<_IsGameUIVisible>(Offsets::IsGameUIVisible);

		this->ienginevgui->Hook(EngineVGui::Paint_Hook, EngineVGui::Paint, Offsets::Paint);
		this->ienginevgui->Hook(EngineVGui::UpdateProgressBar_Hook, EngineVGui::UpdateProgressBar, Offsets::Paint + 12);

		for (auto &hud : Hud::GetList()) {
			if (hud->version == SourceGame_Unknown || p2fx.game->Is(hud->version)) {
				this->huds.push_back(hud);
			}
		}
	}

	return this->hasLoaded = this->ienginevgui;
}
void EngineVGui::Shutdown() {
	Interface::Delete(this->ienginevgui);
	this->huds.clear();
}

EngineVGui *enginevgui;
