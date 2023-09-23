#include "Menu.hpp"

#include "Config.hpp"
#include "Features/GameRecord.hpp"

enum Tab {
	World,
	Misc,
	Recording,
	Configs
};

void Menu::Draw() {
	if (!g_shouldDraw)
		return;

	static Tab tab = World;

	static std::string selectedCampath = std::string();
	static std::string selectedCfg = std::string();

	ImGui::SetNextWindowSize({500, 370});
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), 0, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGui::Begin("p2fx", nullptr, ImGuiWindowFlags_NoDecoration);
	{
		auto draw = ImGui::GetWindowDrawList();

		auto pos = ImGui::GetWindowPos();
		auto size = ImGui::GetWindowSize();

		draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + 51), ImColor(24, 24, 24), 9.0f, ImDrawFlags_RoundCornersTop);
		draw->AddRectFilledMultiColorRounded(pos, ImVec2(pos.x + 55, pos.y + 51), ImColor(1.0f, 1.0f, 1.0f, 0.00f), ImColor(1.0f, 1.0f, 1.0f, 0.05f), ImColor(1.0f, 1.0f, 1.0f, 0.00f), ImColor(1.0f, 1.0f, 1.0f, 0.00f), ImColor(1.0f, 1.0f, 1.0f, 0.05f), 9.0f, ImDrawFlags_RoundCornersAll);

		draw->AddText(Fonts::logo, 17.0f, ImVec2(pos.x + 25, pos.y + 17), ImColor(192, 203, 229), "A");
		draw->AddText(Fonts::semibold, 17.0f, ImVec2(pos.x + 49, pos.y + 18), ImColor(192, 203, 229), "p2fx");

		ImGui::SetCursorPos({100, 19});
		ImGui::BeginGroup();
		{
			if (CImGui::Tab("World", tab == World)) tab = World;
			ImGui::SameLine();
			if (CImGui::Tab("Misc", tab == Misc)) tab = Misc;
			ImGui::SameLine();
			if (CImGui::Tab("Recording", tab == Recording)) tab = Recording;
			ImGui::SameLine();
			if (CImGui::Tab("Config", tab == Configs)) tab = Configs;
		}
		ImGui::EndGroup();

		switch (tab) {
		case World:
			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 25, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "Fog");

			ImGui::SetCursorPos({25, 85});
			ImGui::BeginChild("##container", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				CImGui::Checkbox("Override", "fog_override");
				CImGui::Checkbox("Enable", "fog_enable");
				CImGui::Colorpicker("Color", "fog_color");
				CImGui::Slider("Start", "fog_start", 0, 8000);
				CImGui::Slider("End", "fog_end", 0, 8000);
				CImGui::Sliderf("Max Density", "fog_maxdensity", 0.0f, 1.0f);
				CImGui::Sliderf("HDR Color Scale", "fog_hdrcolorscale", 0.0f, 1.0f);
				CImGui::Checkbox("Enable Water Fog", "fog_enable_water_fog");
				CImGui::Checkbox("Enable Skybox Fog", "fog_enableskybox");
				CImGui::Colorpicker("Color Skybox", "fog_colorskybox");
				CImGui::Slider("Start Skybox", "fog_startskybox", 0, 8000);
				CImGui::Slider("End Skybox", "fog_endskybox", 0, 8000);
				CImGui::Sliderf("Max Density Skybox", "fog_maxdensityskybox", 0.0f, 1.0f);
				CImGui::Sliderf("HDR Color Scale Skybox", "fog_hdrcolorscaleskybox", 0.0f, 1.0f);
			}
			ImGui::EndChild();

			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 285, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "Depth of Field");

			ImGui::SetCursorPos({285, 85});
			ImGui::BeginChild("##container1", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				CImGui::Checkbox("Override", "mat_dof_override");
				CImGui::Checkbox("Enabled", "mat_dof_enabled");
				CImGui::Slider("Quality", "mat_dof_quality", 0, 3);
				CImGui::Slider("Near Focus Depth", "mat_dof_near_focus_depth", 0, 8000);
				CImGui::Slider("Near Blur Depth", "mat_dof_near_blur_depth", 0, 8000);
				CImGui::Slider("Near Blur Radius", "mat_dof_near_blur_radius", 0, 10);
				CImGui::Slider("Far Focus Depth", "mat_dof_far_focus_depth", 0, 8000);
				CImGui::Slider("Far Blur Depth", "mat_dof_far_blur_depth", 0, 8000);
				CImGui::Slider("Far Blur Radius", "mat_dof_far_blur_radius", 0, 10);
				CImGui::Slider("Max Blur Radius", "mat_dof_max_blur_radius", 0, 10);
			}
			ImGui::EndChild();
			break;
		case Misc:
			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 25, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "Misc");

			ImGui::SetCursorPos({25, 85});
			ImGui::BeginChild("##container", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				CImGui::Sliderf("Bloom Scale", "mat_bloomscale", 0.0f, 10.0f);
				CImGui::Sliderf("Sun Brightness", "r_flashlightbrightness", 0.0f, 10.0f);
				CImGui::Sliderf("Force Tonemap Scale", "mat_force_tonemap_scale", 0.0f, 16.0f);
				ImGui::Text("Ambient Light");
				CImGui::Colorpickerf2("Ambient Light", "mat_ambient_light_r", "mat_ambient_light_g", "mat_ambient_light_b");

				CImGui::CheckboxInv("Disable Crosshair", "crosshair");
				CImGui::Checkbox("Disable CM Leaderboard", "p2fx_disable_challenge_stats_hud");
				CImGui::Checkbox("Disable CM Timer", "hidehud", 16);
				CImGui::Checkbox("Disable Coop Score Hud", "p2fx_disable_coop_score_hud");
			}
			ImGui::EndChild();

			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 285, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "View");

			ImGui::SetCursorPos({285, 85});
			ImGui::BeginChild("##container1", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				CImGui::Slider2("FOV", "cl_fov", "p2fx_force_fov", 45, 140);
				CImGui::Slider("Viewmodel FOV", "cl_viewmodelfov", 0, 90);
				CImGui::Sliderf("Viewmodel X", "viewmodel_offset_x", -25.0f, 25.0f);
				CImGui::Sliderf("Viewmodel Y", "viewmodel_offset_y", -25.0f, 25.0f);
				CImGui::Sliderf("Viewmodel Z", "viewmodel_offset_z", -25.0f, 25.0f);
				CImGui::Checkbox("Disable Weapon Sway", "p2fx_disable_weapon_sway");
			}
			ImGui::EndChild();
			break;
		case Recording:
			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 25, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "Render");

			ImGui::SetCursorPos({25, 85});
			ImGui::BeginChild("##container", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				CImGui::Checkbox("Autostart", "p2fx_render_autostart");
				CImGui::Checkbox("Autostop", "p2fx_render_autostop");
				CImGui::Combo2("Video Container Format", "p2fx_render_vformat", {"avi", "mp4", "mov", "mxf"});
				CImGui::Combo2("Video Codec", "p2fx_render_vcodec", {"huffyuv", "h264", "hevc", "vp8", "vp9", "dnxhd"});
				CImGui::Slider("Video Bitrate", "p2fx_render_vbitrate", 1, 100000, "%dkb/s");
				CImGui::Slider("Quality", "p2fx_render_quality", 0, 50);
				CImGui::Slider("FPS", "p2fx_render_fps", 1, 2000);
				CImGui::Slider("Blend (n frames)", "p2fx_render_blend", 1, 128);
				CImGui::Combo("Blend Mode", "p2fx_render_blend_mode", "Linear\0Gaussian\0");
				CImGui::Slider("Shutter Angle", "p2fx_render_shutter_angle", 30, 360);
				CImGui::Combo2("Audio Codec", "p2fx_render_acodec", {"aac", "ac3", "vorbis", "opus", "flac"});
				CImGui::Slider("Audio Bitrate", "p2fx_render_abitrate", 96, 320, "%dkb/s");
				CImGui::Slider("Sample Rate", "p2fx_render_sample_rate", 1000, 48000, "%dkHz");
				CImGui::Checkbox("Merge Renders When Finished", "p2fx_render_merge");
				CImGui::Checkbox("Skip Coop Videos", "p2fx_render_skip_coop_videos");
			}
			ImGui::EndChild();

			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 285, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "Game Record");

			ImGui::SetCursorPos({285, 85});
			ImGui::BeginChild("##container1", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				CImGui::Checkbox("Record Players", "p2gr_record_players");
				CImGui::Checkbox("Record Player Cameras", "p2gr_record_cameras");
				CImGui::Checkbox("Record Player Viewmodels", "p2gr_record_viewmodels");
				CImGui::Checkbox("Record Other Entities", "p2gr_record_others");
				CImGui::Slider("Frame Rate", "p2gr_framerate", 0, 240, "%dfps");
				static char grFileName[32];
				ImGui::InputText("Filename", grFileName, IM_ARRAYSIZE(grFileName));
				if (!gameRecord->GetRecording()) {
					if (ImGui::Button("Start Recording", ImVec2(208.0f, 19.0f))) {
						engine->ExecuteCommand(Utils::ssprintf("p2gr_start %s", grFileName).c_str());
					}
				} else {
					if (ImGui::Button("Stop Recording", ImVec2(208.0f, 19.0f))) {
						engine->ExecuteCommand("p2gr_end");
					}
				}
			}
			ImGui::EndChild();
			break;
		case Configs:
			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 25, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "Configs");

			ImGui::SetCursorPos({25, 85});
			ImGui::BeginChild("##container", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				if (ImGui::BeginListBox("##Configs")) {
					for (auto &cfg : Config::g_Cfgs) {
						const bool isSelected = (selectedCfg == cfg);

						if (ImGui::Selectable(cfg.c_str(), isSelected))
							selectedCfg = cfg;

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndListBox();
				}

				static char cfgName[32];
				ImGui::InputText("Config Name", cfgName, IM_ARRAYSIZE(cfgName));

				if (ImGui::Button("Create")) {
					if (strlen(cfgName) != 0)
						Config::g_Cfgs.push_back(cfgName);
						Config::Save(cfgName);
				}
				if (ImGui::Button("Save")) {
					if (!selectedCfg.empty())
						Config::Save(selectedCfg);
				}
				if (ImGui::Button("Load")) {
					if (!selectedCfg.empty())
						Config::Load(selectedCfg);
				}
				if (ImGui::Button("Delete")) {
					if (!selectedCfg.empty()) {
						Config::Delete(selectedCfg);
						selectedCfg = "";
					}
				}
				if (ImGui::Button("Refresh")) {
					Config::EnumerateCfgs();
				}
			}
			ImGui::EndChild();

			draw->AddText(Fonts::medium, 14.0f, ImVec2(pos.x + 285, pos.y + 60), ImColor(1.0f, 1.0f, 1.0f, 0.6f), "Camera Paths");

			ImGui::SetCursorPos({285, 85});
			ImGui::BeginChild("##container1", ImVec2(190, 275), false, ImGuiWindowFlags_NoScrollbar);
			{
				if (ImGui::BeginListBox("##Campaths")) {
					for (auto &cfg : Campath::g_Campaths) {
						const bool isSelected = (selectedCampath == cfg);

						if (ImGui::Selectable(cfg.c_str(), isSelected))
							selectedCampath = cfg;

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndListBox();
				}

				static char campathName[32];
				ImGui::InputText("Camera Path Name", campathName, IM_ARRAYSIZE(campathName));

				if (ImGui::Button("Create")) {
					if (strlen(campathName) != 0)
						Campath::g_Campaths.push_back(campathName);
						Config::Save(campathName);
				}
				if (ImGui::Button("Save")) {
					if (!selectedCampath.empty())
						Campath::Save(selectedCampath);
				}
				if (ImGui::Button("Load")) {
					if (!selectedCampath.empty())
						Campath::Load(selectedCampath);
				}
				if (ImGui::Button("Delete")) {
					if (!selectedCampath.empty()) {
						Campath::Delete(selectedCampath);
						selectedCampath = "";
					}
				}
				if (ImGui::Button("Refresh")) {
					Campath::EnumerateCfgs();
				}

				CImGui::Combo("Camera Interpolation", "p2fx_cam_path_interp", "Linear\0Cubic Spline\0");
				CImGui::Button("Remove All Camera Markers", "p2fx_cam_path_remkfs");
			}
			ImGui::EndChild();
			break;
		}
	}
	ImGui::End();

	ImGui::PopStyleVar();
}

void Menu::Init() {
	auto &io = ImGui::GetIO();

	io.LogFilename = NULL;
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	Config::Init();
	Campath::Init();
}