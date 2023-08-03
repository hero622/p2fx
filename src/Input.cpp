#include "Input.hpp"

#include "Modules/ShaderApi.hpp"

#include "../lib/imgui/imgui_impl_dx9.h"
#include "../lib/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

long __stdcall Input::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	for (int i = 0; i < 256; i++) {
		auto key = keys[i];

		if (key.state != PRESSED)
			continue;

		key.state = HELD;
	}

	switch (Msg) {
	case WM_KEYDOWN:
		keys[wParam].state = PRESSED;
		break;
	case WM_KEYUP:
		keys[wParam].state = IDLE;
		break;
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

	return CallWindowProcW(g_oWndProc, hWnd, Msg, wParam, lParam);
}

void Input::Init() {
	auto creationParams = D3DDEVICE_CREATION_PARAMETERS();
	shaderApi->g_d3dDevice->GetCreationParameters(&creationParams);
	g_hWnd = creationParams.hFocusWindow;

	g_oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongW(g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
}

void Input::Shutdown() {
	SetWindowLongW(g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_oWndProc));
}