#pragma once

#include <vector>
#include <string>

namespace Config {
	void Load(std::string filename);
	void Save(std::string filename);
	void Delete(std::string filename);

	void EnumerateCfgs();
	void Init();

	inline std::vector<std::string> g_Cfgs;
};  // namespace Config

namespace Campath {
	void Load(std::string filename);
	void Save(std::string filename);
	void Delete(std::string filename);

	void EnumerateCfgs();
	void Init();

	inline std::vector<std::string> g_Campaths;
};  // namespace Config