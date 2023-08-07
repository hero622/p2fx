#pragma once
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#pragma push_macro("GetClassName")
#undef GetClassName

typedef unsigned int VPANEL;

class vgui_Panel {
public:

};

class vgui_Label : vgui_Panel {
public:
	/* void SetText(const char *tokenName) {
		using _SetText = void(__rescall *)(void *, const char *);
		Memory::VMT<_SetText>(this, 1)(this, tokenName);
	} */
};

class VGui : public Module {
public:
	Interface *ipanel = nullptr;

public:
	using _GetName = const char *(__rescall *)(void *thisptr, VPANEL vguiPanel);
	using _GetPanel = vgui_Panel *(__rescall *)(void *thisptr, VPANEL vguiPanel, const char *destinationModule);

	_GetName GetName = nullptr;
	_GetPanel GetPanel = nullptr;

public:
	// IPanel::PaintTraverse
	DECL_DETOUR(PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce);

	bool Init() override;
	void Shutdown() override;
	const char *Name() override { return MODULE("vgui2"); }
};

extern VGui *vgui;

#pragma pop_macro("GetClassName")