#include "InputSystem.hpp"

#include "Cheats.hpp"
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Offsets.hpp"
#include "P2FX.hpp"
#include "Utils.hpp"

REDECL(InputSystem::SleepUntilInput);
#ifdef _WIN32
REDECL(InputSystem::GetRawMouseAccumulators);
#endif

ButtonCode_t InputSystem::GetButton(const char *pString) {
	return this->StringToButtonCode(this->g_InputSystem->ThisPtr(), pString);
}

bool InputSystem::IsKeyDown(ButtonCode_t key) {
	if (!this->g_inputEnabled)
		return false;

	return this->IsButtonDown(this->g_InputSystem->ThisPtr(), key);
}

void InputSystem::GetCursorPos(int &x, int &y) {
	if (!this->g_inputEnabled)
		return;

	return this->GetCursorPosition(this->g_InputSystem->ThisPtr(), x, y);
}

void InputSystem::SetCursorPos(int x, int y) {
	if (!this->g_inputEnabled)
		return;

	return this->SetCursorPosition(this->g_InputSystem->ThisPtr(), x, y);
}

void InputSystem::UnlockCursor() {
	if (!this->g_Context)
		this->g_Context = this->PushInputContext(this->g_InputStackSystem->ThisPtr());

	this->EnableInputContext(this->g_InputStackSystem->ThisPtr(), this->g_Context, true);
	this->SetCursorVisible(this->g_InputStackSystem->ThisPtr(), this->g_Context, false);
	this->SetMouseCapture(this->g_InputStackSystem->ThisPtr(), this->g_Context, true);

	this->g_inputEnabled = false;
}

void InputSystem::LockCursor() {
	if (this->g_Context)
		this->EnableInputContext(this->g_InputStackSystem->ThisPtr(), this->g_Context, false);

	this->g_inputEnabled = true;
}

// CInputSystem::SleepUntilInput
DETOUR(InputSystem::SleepUntilInput, int nMaxSleepTimeMS) {
	if (p2fx_disable_no_focus_sleep.GetBool()) {
		nMaxSleepTimeMS = 0;
	}

	return InputSystem::SleepUntilInput(thisptr, nMaxSleepTimeMS);
}

#ifdef _WIN32
// CInputSystem::GetRawMouseAccumulators
DETOUR_T(void, InputSystem::GetRawMouseAccumulators, int &x, int &y) {
	InputSystem::GetRawMouseAccumulators(thisptr, x, y);
}
#endif

bool InputSystem::Init() {
	this->g_InputSystem = Interface::Create(this->Name(), "InputSystemVersion001");
	this->g_InputStackSystem = Interface::Create(this->Name(), "InputStackSystemVersion001");

	if (this->g_InputSystem) {
		this->StringToButtonCode = this->g_InputSystem->Original<_StringToButtonCode>(Offsets::StringToButtonCode);

		this->g_InputSystem->Hook(InputSystem::SleepUntilInput_Hook, InputSystem::SleepUntilInput, Offsets::SleepUntilInput);
#ifdef _WIN32
		this->g_InputSystem->Hook(InputSystem::GetRawMouseAccumulators_Hook, InputSystem::GetRawMouseAccumulators, Offsets::GetRawMouseAccumulators);
#endif
		this->IsButtonDown = this->g_InputSystem->Original<_IsButtonDown>(Offsets::IsButtonDown);
		this->GetCursorPosition = this->g_InputSystem->Original<_GetCursorPosition>(Offsets::GetCursorPosition);
		this->SetCursorPosition = this->g_InputSystem->Original<_SetCursorPosition>(Offsets::SetCursorPosition);
	}

	if (this->g_InputStackSystem) {
		this->PushInputContext = this->g_InputStackSystem->Original<_PushInputContext>(Offsets::PushInputContext);
		this->EnableInputContext = this->g_InputStackSystem->Original<_EnableInputContext>(Offsets::EnableInputContext);
		this->SetCursorVisible = this->g_InputStackSystem->Original<_SetCursorVisible>(Offsets::SetCursorVisible);
		this->SetMouseCapture = this->g_InputStackSystem->Original<_SetMouseCapture>(Offsets::SetMouseCapture);
	}

	auto unbind = Command("unbind");
	if (!!unbind) {
		auto cc_unbind_callback = (uintptr_t)unbind.ThisPtr()->m_pCommandCallback;
		this->KeySetBinding = Memory::Read<_KeySetBinding>(cc_unbind_callback + Offsets::Key_SetBinding);
	}

	return this->hasLoaded = this->g_InputSystem && this->g_InputStackSystem && !!unbind;
}
void InputSystem::Shutdown() {
	Interface::Delete(this->g_InputSystem);
	Interface::Delete(this->g_InputStackSystem);
}

InputSystem *inputSystem;
