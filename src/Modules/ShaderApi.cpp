#include "ShaderApi.hpp"

#include "Hook.hpp"
#include "Input.hpp"
#include "Modules/Engine.hpp"

#include "../lib/imgui/imgui_impl_dx9.h"
#include "../lib/imgui/imgui_impl_win32.h"

#include "Fonts.hpp"
#include "Menu.hpp"

void ShaderApi::InitImGui(IDirect3DDevice9 *d3dDevice) {
	static bool g_init = false;
	if (!g_init) {
		ImGui::CreateContext();
		
		ImGui_ImplDX9_Init(d3dDevice);
		ImGui_ImplWin32_Init(Input::g_hWnd);

		ImGuiIO &io = ImGui::GetIO();
		ImGui::StyleColorsDark();

		ImFontConfig fontcfg;
		fontcfg.PixelSnapH = false;
		fontcfg.OversampleH = 5;
		fontcfg.OversampleV = 5;
		fontcfg.RasterizerMultiply = 1.2f;

		static const ImWchar ranges[] =
		{
			0x0020,
			0x00FF,  // Basic Latin + Latin Supplement
			0x0400,
			0x052F,  // Cyrillic + Cyrillic Supplement
			0x2DE0,
			0x2DFF,  // Cyrillic Extended-A
			0xA640,
			0xA69F,  // Cyrillic Extended-B
			0xE000,
			0xE226,  // icons
			0,
		};

		fontcfg.GlyphRanges = ranges;

		Fonts::medium = io.Fonts->AddFontFromMemoryTTF(InterMedium, sizeof(InterMedium), 15.0f, &fontcfg, ranges);
		Fonts::semibold = io.Fonts->AddFontFromMemoryTTF(InterSemiBold, sizeof(InterSemiBold), 17.0f, &fontcfg, ranges);

		Fonts::logo = io.Fonts->AddFontFromMemoryTTF(catrine_logo, sizeof(catrine_logo), 17.0f, &fontcfg, ranges);

		Menu::Init();
		
		g_init = true;
	}
}

void ShaderApi::Draw(IDirect3DDevice9 *d3dDevice) {
	// color correction fix
	// backup original state
	DWORD colorwrite, srgbwrite;
	d3dDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &colorwrite);
	d3dDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgbwrite);

	// fix drawing without calling engine functons/cl_showpos
	d3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
	// removes the source engine color correction
	d3dDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::GetIO().MouseDrawCursor = Menu::g_shouldDraw;

	Menu::Draw();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	// revert original state
	d3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorwrite);
	d3dDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, srgbwrite);
}

long(__stdcall *Reset)(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
long __stdcall Reset_Detour(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
static Hook Reset_Hook(&Reset_Detour);
long __stdcall Reset_Detour(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters) {
	ImGui_ImplDX9_InvalidateDeviceObjects();

	Reset_Hook.Disable();
	auto ret = Reset(pDevice, pPresentationParameters);
	Reset_Hook.Enable();

	ImGui_ImplDX9_CreateDeviceObjects();

	return ret;
}

long(__stdcall *Present)(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
long __stdcall Present_Detour(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
static Hook Present_Hook(&Present_Detour);
long __stdcall Present_Detour(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) {
	shaderApi->InitImGui(pDevice);
	shaderApi->Draw(pDevice);
	
	Present_Hook.Disable();
	auto ret = Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	Present_Hook.Enable();
	return ret;
}

bool ShaderApi::Init() {
	this->g_isVulkan = GetModuleHandleA(MODULE("dxvk_d3d9"));

	uintptr_t d3dDevicePtr = Memory::Scan(this->Name(), "89 1D ? ? ? ? E8 ? ? ? ? 8B 55", 2);
	void *g_d3dDeviceAddr = Memory::DerefDeref<void *>(d3dDevicePtr);

	this->g_d3dDevice = reinterpret_cast<IDirect3DDevice9 *>(g_d3dDeviceAddr);
	if (this->g_d3dDevice) {
		Reset = Memory::VMT<long(__stdcall *)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *)>(this->g_d3dDevice, Offsets::Reset);
		Reset_Hook.SetFunc(Reset);

		Present = Memory::VMT<long(__stdcall *)(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *)>(this->g_d3dDevice, Offsets::Present);
		Present_Hook.SetFunc(Present);
	}

	return this->hasLoaded && this->g_d3dDevice;
}

void ShaderApi::Shutdown() {
}

ShaderApi *shaderApi;