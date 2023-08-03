#pragma once
#include "Utils.hpp"

#include <d3d9.h>
#include <array>

namespace Input {
	enum ButtonState {
		IDLE,
		PRESSED,
		HELD
	};

	struct Key {
		ButtonState state;

		bool IsPressed() {
			bool ret = state == PRESSED;
			if (ret)
				state = HELD;
			return ret;
		}

		bool IsHeld() {
			return state == HELD;
		}
	};

	inline WNDPROC g_oWndProc;
	inline HWND g_hWnd;

	void Init();
	void Shutdown();

	long __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	inline std::array<Key, 256> keys;
};