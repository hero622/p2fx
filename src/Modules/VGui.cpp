#include "VGui.hpp"

#include "Features/Hud/Hud.hpp"
#include "Features/Session.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Modules/Surface.hpp"
#include "P2FX.hpp"

#include <algorithm>

REDECL(VGui::Paint);
REDECL(VGui::UpdateProgressBar);

void VGui::Draw(Hud *const &hud) {
	if (hud->ShouldDraw()) {
		hud->Paint(0);
	}
}

// CEngineVGui::Paint
DETOUR(VGui::Paint, PaintMode_t mode) {
	auto result = VGui::Paint(thisptr, mode);

	surface->StartDrawing(surface->matsurface->ThisPtr());

	if ((mode & PAINT_UIPANELS)) {
		for (auto const &hud : vgui->huds) {
			vgui->Draw(hud);
		}
	}

	surface->FinishDrawing();

	return result;
}

DETOUR(VGui::UpdateProgressBar, int progress) {
	if (vgui->lastProgressBar != progress) {
		vgui->lastProgressBar = progress;
		vgui->progressBarCount = 0;
	}
	vgui->progressBarCount++;
	if (p2fx_disable_progress_bar_update.GetInt() == 1 && vgui->progressBarCount > 1) {
		return 0;
	}
	if (p2fx_disable_progress_bar_update.GetInt() == 2) {
		return 0;
	}
	return VGui::UpdateProgressBar(thisptr, progress);
}

bool VGui::IsUIVisible() {
	return this->IsGameUIVisible(this->enginevgui->ThisPtr());
}

bool VGui::Init() {
	this->enginevgui = Interface::Create(this->Name(), "VEngineVGui001");

	if (this->enginevgui) {
		this->IsGameUIVisible = this->enginevgui->Original<_IsGameUIVisible>(Offsets::IsGameUIVisible);

		this->enginevgui->Hook(VGui::Paint_Hook, VGui::Paint, Offsets::Paint);
		this->enginevgui->Hook(VGui::UpdateProgressBar_Hook, VGui::UpdateProgressBar, Offsets::Paint + 12);

		for (auto &hud : Hud::GetList()) {
			if (hud->version == SourceGame_Unknown || p2fx.game->Is(hud->version)) {
				this->huds.push_back(hud);
			}
		}
	}

	return this->hasLoaded = this->enginevgui;
}
void VGui::Shutdown() {
	Interface::Delete(this->enginevgui);
	this->huds.clear();
}

VGui *vgui;
