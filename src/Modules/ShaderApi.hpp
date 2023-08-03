#pragma once
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include <d3d9.h>

class ShaderApi : public Module {
public:
	IDirect3DDevice9 *g_d3dDevice = nullptr;

public:
	bool g_isVulkan = false;

public:
	void InitImGui(IDirect3DDevice9 *d3dDevice);
	void Draw(IDirect3DDevice9 *d3dDevice);

public:
	bool Init() override;
	void Shutdown() override;
	const char *Name() override { return g_isVulkan ? MODULE("shaderapivk") : MODULE("shaderapidx9"); }
};

extern ShaderApi *shaderApi;