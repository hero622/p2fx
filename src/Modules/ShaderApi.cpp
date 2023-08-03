#include "ShaderApi.hpp"

#include "Hook.hpp"
#include "Input.hpp"
#include "Modules/Engine.hpp"

#include "../lib/imgui/imgui_impl_dx9.h"
#include "../lib/imgui/imgui_impl_win32.h"

#include "Menu.hpp"

void ShaderApi::InitImGui(IDirect3DDevice9 *d3dDevice) {
	static bool g_init = false;
	if (!g_init) {
		ImGui::CreateContext();
		ImGui_ImplDX9_Init(d3dDevice);
		ImGui_ImplWin32_Init(Input::g_hWnd);
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

	g_d3dDevice = reinterpret_cast<IDirect3DDevice9 *>(g_d3dDeviceAddr);

	Reset = Memory::VMT<long(__stdcall *)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *)>(g_d3dDevice, 16);
	Reset_Hook.SetFunc(Reset);

	Present = Memory::VMT<long(__stdcall *)(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *)>(g_d3dDevice, 17);
	Present_Hook.SetFunc(Present);

	return this->hasLoaded;
}

void ShaderApi::Shutdown() {

}

ShaderApi *shaderApi;