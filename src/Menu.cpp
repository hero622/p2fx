#include "Menu.hpp"

#include "Modules/Engine.hpp"

#include "../lib/imgui/imgui.h"

void Menu::Draw() {
	if (!engine->demoplayer->IsPlaying() || !g_shouldDraw)
		return;

	ImGui::Begin("ImGui Window");
	ImGui::End();
}