#pragma once
#include "Utils.hpp"

#include <d3d9.h>

namespace Window {
	inline WNDPROC g_oWndProc;
	inline HWND g_hWnd;

	void Init();
	void Shutdown();

	long __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};