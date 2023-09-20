#pragma once
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

typedef unsigned int VPANEL;

class Label {
public:
	void SetText(const char *tokenName) {
		Memory::VMT<void(__rescall *)(void *, const char *)>(this, 184)(this, tokenName);
	}

	void GetText(const wchar_t *textOut, int bufLenInBytes) {
		Memory::VMT<void(__rescall *)(void *, const wchar_t *, int)>(this, 186)(this, textOut, bufLenInBytes);
	}
};

class GenericPanelList {
public:
	void RemoveAllPanelItems() {
		Memory::VMT<void(__rescall *)(void *)>(this, 187)(this);
	}
};

struct ExtraInfo_t {
	ExtraInfo_t() {
		m_nImageId = -1;
	}

	int m_nImageId;
	CUtlString m_TitleString;
	CUtlString m_SubtitleString;
	CUtlString m_MapName;
	CUtlString m_VideoName;
	CUtlString m_URLName;
	CUtlString m_Command;
};

class VGui : public Module {
public:
	Interface *ipanel = nullptr;

public:
	using _SetVisible = int(__rescall *)(void *thisptr, VPANEL vguiPanel, bool state);
	using _IsVisible = bool(__rescall *)(void *thisptr, VPANEL vguiPanel);
	using _GetName = const char *(__rescall *)(void *thisptr, VPANEL vguiPanel);
	using _GetPanel = void *(__rescall *)(void *thisptr, VPANEL vguiPanel, const char *destinationModule);

	_SetVisible SetVisible = nullptr;
	_IsVisible IsVisible = nullptr;
	_GetName GetName = nullptr;
	_GetPanel GetPanel = nullptr;  // cast necessary !!!

	enum VGUI_STATE {
		VGUI_IDLE = 0,
		VGUI_LOADING,
		VGUI_LOADED = 5,
		VGUI_OVERWRITTEN,
		VGUI_RESET
	};

	int g_vguiState = 0;
	VPANEL g_menuPanel;
	std::vector<VPANEL> g_panels;
	VPANEL g_extrasBtn;

	uintptr_t g_ExtraInfos;
	GenericPanelList *g_pInfoList;
	void *g_extrasDialog;
	void *g_pScheme;

	void *g_CBaseModPanel;

public:
	std::map<int, int> g_chapterImgs;
	std::string g_curDirectory;
	std::vector<std::string> g_demos;

	void InitImgs();
	int GetImageId(const char *pImageName);
	bool IsMenuOpened();
	void OverrideMenu(bool state);
	void EnumerateFiles(CUtlVector<ExtraInfo_t> &m_ExtraInfos, std::string path);

public:
	// IPanel::PaintTraverse
	DECL_DETOUR(PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce);

	// CExtrasDialog::PopulateSaveGameList
	DECL_DETOUR(PopulateFromScript);

	// CExtrasDialog::ApplySchemeSettings
	DECL_DETOUR(ApplySchemeSettings, void *pScheme);

	// MainMenu::OnCommand
	DECL_DETOUR(MainMenuOnCommand, const char *command);

	// InGameMainMenu::OnCommand
	DECL_DETOUR(InGameMainMenuOnCommand, const char *command);
	
	// CBaseModPanel::OpenWindow
	DECL_DETOUR_T(void *, OpenWindow, const int &wt, void *caller, bool hidePrevious, KeyValues *pParameter);

	bool Init() override;
	void Shutdown() override;
	const char *Name() override { return MODULE("vgui2"); }
};

extern VGui *vgui;