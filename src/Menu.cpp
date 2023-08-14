#include "Menu.hpp"

#include "Config.hpp"
#include "Modules/Surface.hpp"

void Menu::Draw() {
	if (!g_shouldDraw)
		return;

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));

	if (ImGui::Begin("P2FX", (bool *)NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
		static int tab = 0;

		if (ImGui::Button("FX", ImVec2(100.0f, 30.0f))) {
			tab = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("FOG", ImVec2(100.0f, 30.0f))) {
			tab = 1;
		}
		ImGui::SameLine();
		if (ImGui::Button("DOF", ImVec2(100.0f, 30.0f))) {
			tab = 2;
		}
		ImGui::SameLine();
		if (ImGui::Button("RENDER", ImVec2(100.0f, 30.0f))) {
			tab = 3;
		}
		ImGui::SameLine();
		if (ImGui::Button("CAM", ImVec2(100.0f, 30.0f))) {
			tab = 4;
		}
		ImGui::SameLine();
		if (ImGui::Button("CFG", ImVec2(100.0f, 30.0f))) {
			tab = 5;
		}

		ImGui::Separator();

		static std::string selectedCampath = std::string();
		static std::string selectedCfg = std::string();

		switch (tab) {
		case 0:
			CImGui::Sliderf("Bloom Scale", "mat_bloomscale", 0.0f, 10.0f);
			CImGui::Sliderf("Sun Brightness", "r_flashlightbrightness", 0.0f, 10.0f);
			CImGui::Sliderf("Force Tonemap Scale", "mat_force_tonemap_scale", 0.0f, 16.0f);
			CImGui::Colorpickerf2("Ambient Light", "mat_ambient_light_r", "mat_ambient_light_g", "mat_ambient_light_b");

			ImGui::Separator();

			CImGui::Slider2("FOV", "cl_fov", "p2fx_force_fov", 45, 140);
			CImGui::Slider("Viewmodel FOV", "cl_viewmodelfov", 0, 90);
			CImGui::Sliderf("Viewmodel X", "viewmodel_offset_x", -25.0f, 25.0f);
			CImGui::Sliderf("Viewmodel Y", "viewmodel_offset_y", -25.0f, 25.0f);
			CImGui::Sliderf("Viewmodel Z", "viewmodel_offset_z", -25.0f, 25.0f);
			CImGui::Checkbox("Disable Weapon Sway", "p2fx_disable_weapon_sway");

			ImGui::Separator();

			CImGui::CheckboxInv("Disable Crosshair", "crosshair");
			CImGui::Checkbox("Disable Challenge Mode Leaderboard", "p2fx_disable_challenge_stats_hud");
			CImGui::Checkbox("Disable Challenge Mode Timer", "hidehud", 16);
			CImGui::Checkbox("Disable Coop Score Hud", "p2fx_disable_coop_score_hud");
			break;
		case 1:
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
			break;
		case 2:
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
			break;
		case 3:
			CImGui::Checkbox("Autostart", "p2fx_render_autostart");
			CImGui::Checkbox("Autostop", "p2fx_render_autostop");
			CImGui::Combo2("Video Codec", "p2fx_render_vcodec", {"h264", "hevc", "vp8", "vp9", "dnxhd"});
			CImGui::Slider("Video Bitrate", "p2fx_render_vbitrate", 0, 100000, "%dkb/s");
			CImGui::Slider("Quality", "p2fx_render_quality", 0, 50);
			CImGui::Slider("FPS", "p2fx_render_fps", 0, 2000);
			CImGui::Slider("Blend (n frames)", "p2fx_render_blend", 1, 128);
			CImGui::Combo("Blend Mode", "p2fx_render_blend_mode", "Linear\0Gaussian\0");
			CImGui::Slider("Shutter Angle", "p2fx_render_shutter_angle", 30, 360);
			CImGui::Combo2("Audio Codec", "p2fx_render_acodec", {"aac", "ac3", "vorbis", "opus", "flac"});
			CImGui::Slider("Audio Bitrate", "p2fx_render_abitrate", 96, 320, "%dkb/s");
			CImGui::Slider("Sample Rate", "p2fx_render_sample_rate", 1000, 48000, "%dkHz");
			CImGui::Checkbox("Merge Renders When Finished", "p2fx_render_merge");
			CImGui::Checkbox("Skip Coop Videos", "p2fx_render_skip_coop_videos");
			break;
		case 4:
			CImGui::Combo("Camera Interpolation", "p2fx_cam_path_interp", "Linear\0Cubic Spline\0");
			CImGui::Button("Remove All Camera Markers", "p2fx_cam_path_remkfs");

			ImGui::Separator();

			ImGui::Columns(2, "Columns", false);

			ImGui::SetColumnOffset(1, 240.0f);
			ImGui::PushItemWidth(240.0f);

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
			ImGui::InputText("##Campath name", campathName, IM_ARRAYSIZE(campathName));

			ImGui::PopItemWidth();

			ImGui::NextColumn();

			if (ImGui::Button("Create", ImVec2(80.0f, 27.0f))) {
				if (strlen(campathName) != 0)
					Campath::g_Campaths.push_back(campathName);
			}
			if (ImGui::Button("Save", ImVec2(80.0f, 27.0f))) {
				if (!selectedCampath.empty())
					Campath::Save(selectedCampath);
			}
			if (ImGui::Button("Load", ImVec2(80.0f, 27.0f))) {
				if (!selectedCampath.empty())
					Campath::Load(selectedCampath);
			}
			if (ImGui::Button("Delete", ImVec2(80.0f, 27.0f))) {
				if (!selectedCampath.empty()) {
					Campath::Delete(selectedCampath);
					selectedCampath = "";
				}
			}
			if (ImGui::Button("Refresh", ImVec2(80.0f, 27.0f))) {
				Campath::EnumerateCfgs();
			}
			break;
		case 5:
			ImGui::Columns(2, "Columns", false);

			ImGui::SetColumnOffset(1, 240.0f);
			ImGui::PushItemWidth(240.0f);

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
			ImGui::InputText("##Config name", cfgName, IM_ARRAYSIZE(cfgName));

			ImGui::PopItemWidth();

			ImGui::NextColumn();

			if (ImGui::Button("Create", ImVec2(80.0f, 27.0f))) {
				if (strlen(cfgName) != 0)
					Config::g_Cfgs.push_back(cfgName);
			}
			if (ImGui::Button("Save", ImVec2(80.0f, 27.0f))) {
				if (!selectedCfg.empty())
					Config::Save(selectedCfg);
			}
			if (ImGui::Button("Load", ImVec2(80.0f, 27.0f))) {
				if (!selectedCfg.empty())
					Config::Load(selectedCfg);
			}
			if (ImGui::Button("Delete", ImVec2(80.0f, 27.0f))) {
				if (!selectedCfg.empty()) {
					Config::Delete(selectedCfg);
					selectedCfg = "";
				}
			}
			if (ImGui::Button("Refresh", ImVec2(80.0f, 27.0f))) {
				Config::EnumerateCfgs();
			}
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
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGuiStyle &style = ImGui::GetStyle();

	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameRounding = 0.0f;
	style.FrameBorderSize = 1.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
	style.TabBorderSize = 0.0f;

	ImGui::StyleColorsClassic(&style);

	Config::Init();
	Campath::Init();
}