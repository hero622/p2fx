#include "Menu.hpp"

#include "Modules/Engine.hpp"
#include "Modules/InputSystem.hpp"

void Menu::Draw() {
	if (GetAsyncKeyState(VK_F2) & 1) {
		g_shouldDraw = !g_shouldDraw;

		if (g_shouldDraw)
			inputSystem->UnlockCursor();
		else
			inputSystem->LockCursor();
	}

	if (!g_shouldDraw)
		return;

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));

	if (ImGui::Begin("P2FX", &g_shouldDraw, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
		static int tab = 0;

		if (ImGui::Button("FOG", ImVec2(100.0f, 30.0f))) {
			tab = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("DOF", ImVec2(100.0f, 30.0f))) {
			tab = 1;
		}
		ImGui::SameLine();
		if (ImGui::Button("RAIN", ImVec2(100.0f, 30.0f))) {
			tab = 2;
		}
		ImGui::SameLine();
		if (ImGui::Button("CFG", ImVec2(100.0f, 30.0f))) {
			tab = 3;
		}

		switch (tab) {
		case 0:
			CImGui::Checkbox("Override", "fog_override");
			CImGui::Checkbox("Enable", "fog_enable");
			CImGui::Colorpicker3("Color", "fog_color");
			CImGui::Slider("Start", "fog_start", 0, 8000);
			CImGui::Slider("End", "fog_end", 0, 8000);
			CImGui::Sliderf("Max Density", "fog_maxdensity", 0.0f, 1.0f);
			CImGui::Sliderf("HDR Color Scale", "fog_hdrcolorscale", 0.0f, 1.0f);
			CImGui::Checkbox("Enable Water Fog", "fog_enable_water_fog");
			CImGui::Checkbox("Enable Skybox Fog", "fog_enableskybox");
			CImGui::Colorpicker3("Color Skybox", "fog_colorskybox");
			CImGui::Slider("Start Skybox", "fog_startskybox", 0, 8000);
			CImGui::Slider("End Skybox", "fog_endskybox", 0, 8000);
			CImGui::Sliderf("Max Density Skybox", "fog_maxdensityskybox", 0.0f, 1.0f);
			CImGui::Sliderf("HDR Color Scale Skybox", "fog_hdrcolorscaleskybox", 0.0f, 1.0f);
			break;
		case 1:
			CImGui::Checkbox("Override", "mat_dof_override");
			CImGui::Checkbox("Enabled", "mat_dof_enabled");
			CImGui::Slider("Quality", "mat_dof_quality", 0, 3);
			CImGui::Slider("Near Blur Radius", "mat_dof_near_blur_radius", 0, 10);
			CImGui::Slider("Near Focus Depth", "mat_dof_near_focus_depth", 0, 8000);
			CImGui::Slider("Far Blur Depth", "mat_dof_far_blur_depth", 0, 8000);
			CImGui::Slider("Far Blur Radius", "mat_dof_far_blur_radius", 0, 10);
			CImGui::Slider("Far Focus Depth", "mat_dof_far_focus_depth", 0, 8000);
			CImGui::Slider("Max Blur Radius", "mat_dof_max_blur_radius", 0, 10);
			break;
		case 3:
			CImGui::Combo("Camera Interpolation", "p2fx_cam_path_interp", "Linear\0Cubic Spline\0");
			break;
		}

		ImGui::End();
	}
}

void Menu::Init() {
	auto &io = ImGui::GetIO();

	io.LogFilename = NULL;
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
}