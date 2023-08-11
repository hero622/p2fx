#include "Config.hpp"

#include "Modules/Engine.hpp"

#include "Variable.hpp"

#include <fstream>
#include <filesystem>

#define CFG_DIR "portal2/cfg/p2fx/cfg"
#define CAMPATH_DIR "portal2/cfg/p2fx/campaths"

const char *getPath(std::string filename, bool cfg = true) {
	return (std::string(cfg ? CFG_DIR : CAMPATH_DIR) + std::string("/") + filename + std::string(".cfg")).c_str();
}

// Config

std::vector<std::string> cvars = {
	"mat_bloomscale",
	"r_flashlightbrightness",
	"mat_force_tonemap_scale",
	"mat_ambient_light_r",
	"mat_ambient_light_g",
	"mat_ambient_light_b",

	"cl_viewmodelfov",
	"viewmodel_offset_x",
	"viewmodel_offset_y",
	"viewmodel_offset_z",
	"p2fx_disable_weapon_sway",

	"crosshair",
	"p2fx_disable_challenge_stats_hud",
	"hidehud",
	"p2fx_disable_coop_score_hud",

	"p2fx_cam_path_interp",

	"fog_override",
	"fog_enable",
	"fog_color",
	"fog_start",
	"fog_end",
	"fog_maxdensity",
	"fog_hdrcolorscale",
	"fog_enable_water_fog",
	"fog_enableskybox",
	"fog_colorskybox",
	"fog_startskybox",
	"fog_endskybox",
	"fog_maxdensityskybox",
	"fog_hdrcolorscaleskybox",

	"mat_dof_override",
	"mat_dof_enabled",
	"mat_dof_quality",
	"mat_dof_near_focus_depth",
	"mat_dof_near_blur_depth",
	"mat_dof_near_blur_radius",
	"mat_dof_far_focus_depth",
	"mat_dof_far_blur_depth",
	"mat_dof_far_blur_radius",
	"mat_dof_max_blur_radius",

	"p2fx_render_autostart",
	"p2fx_render_autostop",
	"p2fx_render_vcodec",
	"p2fx_render_vbitrate",
	"p2fx_render_quality",
	"p2fx_render_fps",
	"p2fx_render_blend",
	"p2fx_render_blend_mode",
	"p2fx_render_shutter_angle",
	"p2fx_render_acodec",
	"p2fx_render_abitrate",
	"p2fx_render_sample_rate",
	"p2fx_render_merge",
	"p2fx_render_skip_coop_videos"};

void Config::Load(std::string filename) {
	engine->ExecuteCommand(Utils::ssprintf("exec \"%s\"", (std::string("p2fx/cfg/") + filename).c_str()).c_str());
}

void Config::Save(std::string filename) {
	FILE *fp = fopen(getPath(filename), "w");
	if (fp) {
		// stupid exception
		fprintf(fp, "p2fx_force_fov %s\n", Variable("cl_fov").GetString());

		for (auto &cvar : cvars) {
			fprintf(fp, "%s %s\n", cvar.c_str(), Variable(cvar.c_str()).GetString());
		}
		fclose(fp);
	}
}

void Config::Delete(std::string filename) {
	remove(getPath(filename));
	g_Cfgs.erase(std::remove(g_Cfgs.begin(), g_Cfgs.end(), filename), g_Cfgs.end());
}

void Config::EnumerateCfgs() {
	std::vector<std::string> paths;

	for (const auto &entry : std::filesystem::directory_iterator(CFG_DIR)) {
		auto path = entry.path();

		paths.push_back(path.stem().string());
	}

	Config::g_Cfgs = paths;
}

void Config::Init() {
	if (!std::filesystem::is_directory(CFG_DIR)) {
		std::filesystem::create_directories(CFG_DIR);
		return;
	}

	Config::EnumerateCfgs();
}

// Campath

void Campath::Load(std::string filename) {
	engine->ExecuteCommand(Utils::ssprintf("exec \"%s\"", (std::string("p2fx/campaths/") + filename).c_str()).c_str());
}

void Campath::Save(std::string filename) {
	engine->ExecuteCommand(Utils::ssprintf("p2fx_cam_path_export %s", getPath(filename, false)).c_str());
}

void Campath::Delete(std::string filename) {
	remove(getPath(filename, false));
	g_Campaths.erase(std::remove(g_Campaths.begin(), g_Campaths.end(), filename), g_Campaths.end());
}

void Campath::EnumerateCfgs() {
	std::vector<std::string> paths;

	for (const auto &entry : std::filesystem::directory_iterator(CAMPATH_DIR)) {
		auto path = entry.path();

		paths.push_back(path.stem().string());
	}

	Campath::g_Campaths = paths;
}

void Campath::Init() {
	if (!std::filesystem::is_directory(CAMPATH_DIR)) {
		std::filesystem::create_directories(CAMPATH_DIR);
		return;
	}

	Campath::EnumerateCfgs();
}