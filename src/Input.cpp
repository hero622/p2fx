#include "Input.hpp"

#include "Modules/Engine.hpp"
#include "Modules/VGui.hpp"
#include "Modules/ShaderApi.hpp"

#include "Menu.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

long __stdcall Input::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	for (int i = 0; i < 256; i++) {
		auto key = keys[i];

		if (key.state != PRESSED)
			continue;

		key.state = HELD;
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

	switch (Msg) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		keys[wParam].state = PRESSED;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		keys[wParam].state = IDLE;
		break;
	}

	if (Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN) {
		if (Menu::g_shouldDraw)
			return 0;

		if (engine->demoplayer->IsPlaying() && !vgui->IsMenuOpened()) {
			if (wParam == VK_F4 || wParam == 0x46 || wParam == VK_F2 || wParam == VK_F3 || wParam == VK_SPACE || wParam == VK_DOWN || wParam == VK_UP || wParam == VK_LEFT || wParam == VK_RIGHT || wParam == 0x52 || wParam == VK_NEXT || wParam == VK_PRIOR)
				return 0;
		}
	}

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