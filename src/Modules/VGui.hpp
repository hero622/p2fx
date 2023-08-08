#pragma once
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

typedef unsigned int VPANEL;

class BaseModHybridButton {
public:
	void SetText(const char *tokenName) {
		using _SetText = void(__rescall *)(void *, const char *);
		Memory::VMT<_SetText>(this, 184)(this, tokenName);
	}
};

class GenericPanelList {
public:
	/* unsigned short AddPanelItem(void *panelItem, bool bNeedsInvalidateScheme) {
		using _AddPanelItem = unsigned short(__rescall *)(void *, void *, bool);
		return Memory::VMT<_AddPanelItem>(this, 183)(this, panelItem, bNeedsInvalidateScheme);
	} */

	void RemoveAllPanelItems() {
		using _RemoveAllPanelItems = void(__rescall *)(void *);
		Memory::VMT<_RemoveAllPanelItems>(this, 187)(this);
	}
};

class SaveGameItem {
public:
	SaveGameItem(void *pParent, const char *pPanelName) {

	}

public:
	wchar_t m_DateString[128];
};


class VGui : public Module {
public:
	Interface *ipanel = nullptr;

public:
	using _SetVisible = int(__rescall *)(void *thisptr, VPANEL vguiPanel, bool state);
	using _GetName = const char *(__rescall *)(void *thisptr, VPANEL vguiPanel);
	using _GetPanel = void *(__rescall *)(void *thisptr, VPANEL vguiPanel, const char *destinationModule);

	_SetVisible SetVisible = nullptr;
	_GetName GetName = nullptr;
	// cast necessary !!!
	_GetPanel GetPanel = nullptr;

public:
	// IPanel::PaintTraverse
	DECL_DETOUR(PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce);

	// SaveLoadGameDialog::PopulateSaveGameList
	DECL_DETOUR(PopulateSaveGameList);

	bool Init() override;
	void Shutdown() override;
	const char *Name() override { return MODULE("vgui2"); }
};

extern VGui *vgui;