#include "VGui.hpp"

#include "Command.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Surface.hpp"
#include "Tier0.hpp"
#include "Features/Demo/DemoParser.hpp"

#include <iostream>
#include <vector>
#include <algorithm>

REDECL(VGui::PaintTraverse);
REDECL(VGui::PopulateFromScript);
REDECL(VGui::ApplySchemeSettings);
REDECL(VGui::MainMenuOnCommand);
REDECL(VGui::InGameMainMenuOnCommand);
REDECL(VGui::OpenWindow);

extern Hook g_PopulateFromScriptHook;
extern Hook g_ApplySchemeSettingsHook;
extern Hook g_ActivateSelectedItemHook;
extern Hook g_MainMenuOnCommandHook;
extern Hook g_InGameMainMenuOnCommandHook;
extern Hook g_OpenWindowHook;

void VGui::InitImgs() {
	g_chapterImgs[0] = this->GetImageId("vgui/chapters/coopcommentary_chapter1");
	g_chapterImgs[1] = this->GetImageId("vgui/chapters/chapter1");
	g_chapterImgs[2] = this->GetImageId("vgui/chapters/chapter2");
	g_chapterImgs[3] = this->GetImageId("vgui/chapters/chapter3");
	g_chapterImgs[4] = this->GetImageId("vgui/chapters/chapter4");
	g_chapterImgs[5] = this->GetImageId("vgui/chapters/chapter5");
	g_chapterImgs[6] = this->GetImageId("vgui/chapters/chapter6");
	g_chapterImgs[7] = this->GetImageId("vgui/chapters/chapter7");
	g_chapterImgs[8] = this->GetImageId("vgui/chapters/chapter8");
	g_chapterImgs[9] = this->GetImageId("vgui/chapters/chapter9");
}

int VGui::GetImageId(const char *pImageName) {
	int nImageId = surface->CreateNewTextureID(surface->matsurface->ThisPtr(), false);
	surface->DrawSetTextureFile(surface->matsurface->ThisPtr(), nImageId, pImageName, true, false);

	return nImageId;
}

bool VGui::IsMenuOpened() {
	return this->IsVisible(this->ipanel->ThisPtr(), this->g_menuPanel);
}

void VGui::OverrideMenu(bool state) {
	if (this->g_vguiState < VGUI_LOADED || (this->g_vguiState == VGUI_OVERWRITTEN && state))
		return;

	for (auto panel : this->g_panels) {
		this->SetVisible(this->ipanel->ThisPtr(), panel, !state);
	}

	// you just kinda have to guess the module as far as i can tell:
	// find modules by searching for VGui_InitInterfacesList and VGui_InitMatSysInterfacesList in src
	// most stuff is in ClientDLL
	Label *label = (Label *)this->GetPanel(this->ipanel->ThisPtr(), this->g_extrasBtn, "ClientDLL");
	// BaseModHybridButton class inherits from Label so we can just use that
	label->SetText(state ? "DEMO VIEWER" : "#PORTAL2_MainMenu_Community");

	this->g_vguiState = state ? VGUI_OVERWRITTEN : VGUI_RESET;
}

ON_EVENT(SESSION_END) {
	vgui->g_panels.clear();
	vgui->g_vguiState = VGui::VGUI_IDLE;
}

DETOUR(VGui::PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce) {
	auto name = vgui->GetName(vgui->ipanel->ThisPtr(), vguiPanel);

	if (vgui->g_vguiState < VGUI_LOADED) {
		if (!strcmp(name, "CBaseModPanel")) {
			vgui->g_menuPanel = vguiPanel;
		} else if (!strcmp(name, "BtnPlaySolo") || !strcmp(name, "BtnCoOp") || !strcmp(name, "BtnExtras") || !strcmp(name, "BtnEconUI")) {
			vgui->g_panels.push_back(vguiPanel);
			vgui->g_vguiState++;
		} else if (!strcmp(name, "BtnCommunity")) {
			vgui->g_extrasBtn = vguiPanel;
			vgui->g_vguiState++;
		}
	} else if (vgui->g_vguiState == VGUI_LOADED) {
		vgui->OverrideMenu(true);
		vgui->g_curDirectory = engine->GetGameDirectory();
	}

	if (!strcmp(name, "BtnRestartLevel")) {
		Label *label = (Label *)vgui->GetPanel(vgui->ipanel->ThisPtr(), vguiPanel, "ClientDLL");
		label->SetText("NEXT DEMO");
	} else if (!strcmp(name, "BtnLeaderboards")) {
		Label *label = (Label *)vgui->GetPanel(vgui->ipanel->ThisPtr(), vguiPanel, "ClientDLL");
		label->SetText("CHANGE DEMO");
	}

	return VGui::PaintTraverse(thisptr, vguiPanel, forceRepaint, allowForce);
}

CON_COMMAND_F(p2fx_directory, "Internal command, do not use.\n", FCVAR_HIDDEN) {
	if (args.ArgC() == 2) {
		vgui->g_curDirectory = args[1];

		(*(CUtlVector<ExtraInfo_t> *)vgui->g_ExtraInfos).RemoveAll();
		vgui->g_pInfoList->RemoveAllPanelItems();

		VGui::PopulateFromScript(vgui->g_extrasDialog);

		g_ApplySchemeSettingsHook.Disable();
		VGui::ApplySchemeSettings(vgui->g_extrasDialog, vgui->g_pScheme);
		g_ApplySchemeSettingsHook.Enable();
	} else {
		return console->Print(p2fx_directory.ThisPtr()->m_pszHelpString);
	}
}

void VGui::EnumerateFiles(CUtlVector<ExtraInfo_t> &m_ExtraInfos, std::string path) {
	vgui->g_demos.clear();

	// back button
	if (std::filesystem::path(path) != std::filesystem::path(engine->GetGameDirectory())) {
		int nIndex = m_ExtraInfos.AddToTail();
		m_ExtraInfos[nIndex].m_TitleString = "../";
		m_ExtraInfos[nIndex].m_SubtitleString = "";
		m_ExtraInfos[nIndex].m_MapName = "";
		m_ExtraInfos[nIndex].m_VideoName = "";
		m_ExtraInfos[nIndex].m_URLName = "";
		m_ExtraInfos[nIndex].m_Command = Utils::ssprintf("p2fx_directory \"%s\"", std::filesystem::path(path).parent_path().string().c_str()).c_str();  // ghetto method bc im too lazy to hook ActivateSelectedItem and all that
		m_ExtraInfos[nIndex].m_nImageId = -1;
	}

	// list folders first
	for (const auto &p : std::filesystem::directory_iterator(path)) {
		if (m_ExtraInfos.m_Size == 1170)
			break;

		if (p.is_directory()) {
			// check if folder has any demos recursively
			bool containsDemo = false;
			for (const auto &dems : std::filesystem::recursive_directory_iterator(p)) {
				if (dems.path().extension() == ".dem") {
					containsDemo = true;
				}
			}
			if (!containsDemo)
				continue;

			auto dirpath = p.path();

			std::string dirname = dirpath.stem().string();

			int nIndex = m_ExtraInfos.AddToTail();
			m_ExtraInfos[nIndex].m_TitleString = Utils::ssprintf("%s/", dirname.c_str()).c_str();
			m_ExtraInfos[nIndex].m_SubtitleString = "";
			m_ExtraInfos[nIndex].m_MapName = "";
			m_ExtraInfos[nIndex].m_VideoName = "";
			m_ExtraInfos[nIndex].m_URLName = "";
			m_ExtraInfos[nIndex].m_Command = Utils::ssprintf("p2fx_directory \"%s\"", dirpath.string().c_str()).c_str();  // ghetto method bc im too lazy to hook ActivateSelectedItem and all that
			m_ExtraInfos[nIndex].m_nImageId = -1;
		}
	}
	// list demo files
	for (const auto &p : std::filesystem::directory_iterator(path)) {
		if (m_ExtraInfos.m_Size == 1170)
			break;

		auto filepath = p.path();

		if (filepath.extension() == ".dem") {
			std::string filename = filepath.stem().string();

			auto safepath = std::filesystem::relative(std::filesystem::path(filepath), std::filesystem::path(engine->GetGameDirectory()));

			Demo demo;
			DemoParser parser;
			if (parser.Parse(filepath.string(), &demo)) {
				parser.Adjust(&demo);

				int chapter = p2fx.game->chapters[demo.mapName];

				int nIndex = m_ExtraInfos.AddToTail();
				m_ExtraInfos[nIndex].m_TitleString = filename.c_str();
				m_ExtraInfos[nIndex].m_SubtitleString = Utils::ssprintf("Player: %s, Time: %.3f", demo.clientName, demo.playbackTime).c_str();
				m_ExtraInfos[nIndex].m_MapName = "";
				m_ExtraInfos[nIndex].m_VideoName = "";
				m_ExtraInfos[nIndex].m_URLName = "";
				m_ExtraInfos[nIndex].m_Command = Utils::ssprintf("playdemo \"%s\"", safepath.string().c_str()).c_str();
				m_ExtraInfos[nIndex].m_nImageId = g_chapterImgs[chapter];

				g_demos.push_back(safepath.string().c_str());
			}
		}
	}
}

DETOUR(VGui::PopulateFromScript) {
	vgui->g_ExtraInfos = (uintptr_t)thisptr + 0x830;
	auto &m_ExtraInfos = *(CUtlVector<ExtraInfo_t> *)vgui->g_ExtraInfos;

	vgui->EnumerateFiles(m_ExtraInfos, vgui->g_curDirectory);

	return 0;
}
Hook g_PopulateFromScriptHook(&VGui::PopulateFromScript_Hook);

DETOUR(VGui::ApplySchemeSettings, void *pScheme) {
	vgui->g_extrasDialog = thisptr;
	vgui->g_pScheme = pScheme;
	vgui->g_pInfoList = *reinterpret_cast<GenericPanelList **>((uintptr_t)thisptr + 0x84C);

	g_ApplySchemeSettingsHook.Disable();
	auto ret = VGui::ApplySchemeSettings(thisptr, pScheme);
	g_ApplySchemeSettingsHook.Enable();

	return ret;
}
Hook g_ApplySchemeSettingsHook(&VGui::ApplySchemeSettings_Hook);

DETOUR(VGui::MainMenuOnCommand, const char *command) {
	if (!strcmp(command, "CreateChambers")) {
		command = "Extras";
	}

	g_MainMenuOnCommandHook.Disable();
	auto ret = VGui::MainMenuOnCommand(thisptr, command);
	g_MainMenuOnCommandHook.Enable();

	return ret;
}
Hook g_MainMenuOnCommandHook(&VGui::MainMenuOnCommand_Hook);

DETOUR(VGui::InGameMainMenuOnCommand, const char *command) {
	if (!strcmp(command, "RestartLevel")) {
		// previous demo
		int curIdx = std::distance(vgui->g_demos.begin(), std::find(vgui->g_demos.begin(), vgui->g_demos.end(), std::string(engine->demoplayer->DemoName)));
		engine->ExecuteCommand(Utils::ssprintf("playdemo %s", vgui->g_demos[curIdx + 1]).c_str(), true);
		return 0;
	} else if (!strcmp(command, "Leaderboards_")) {
		// change demo
		engine->ExecuteCommand("gameui_preventescape", true);              // GameUI().PreventEngineHideGameUI();
		VGui::OpenWindow(vgui->g_CBaseModPanel, 63, thisptr, true, NULL);  // WT_EXTRAS = 63
		return 0;
	}

	g_InGameMainMenuOnCommandHook.Disable();
	auto ret = VGui::InGameMainMenuOnCommand(thisptr, command);
	g_InGameMainMenuOnCommandHook.Enable();

	return ret;
}
Hook g_InGameMainMenuOnCommandHook(&VGui::InGameMainMenuOnCommand_Hook);

// we only use this to obtain the CBaseModPanel Singleton
DETOUR_T(void *, VGui::OpenWindow, const int &wt, void *caller, bool hidePrevious, KeyValues *pParameter) {
	// this is a singleton so its fine if we only obtain it once
	vgui->g_CBaseModPanel = thisptr;

	g_OpenWindowHook.Disable(true);
	return VGui::OpenWindow(thisptr, wt, caller, hidePrevious, pParameter);
}
Hook g_OpenWindowHook(&VGui::OpenWindow_Hook);

bool VGui::Init() {
	this->ipanel = Interface::Create(this->Name(), "VGUI_Panel009");

	if (this->ipanel) {
		this->SetVisible = this->ipanel->Original<_SetVisible>(14);
		this->IsVisible = this->ipanel->Original<_IsVisible>(15);
		this->GetName = this->ipanel->Original<_GetName>(36);
		this->GetPanel = this->ipanel->Original<_GetPanel>(55);

		this->ipanel->Hook(VGui::PaintTraverse_Hook, VGui::PaintTraverse, 41);
	}

	this->InitImgs();

	// vgui stuff are all over everywhere, base level stuff is in vgui2, most panels are in client
	VGui::PopulateFromScript = (decltype(VGui::PopulateFromScript))Memory::Scan(MODULE("client"), "55 8B EC 83 EC 1C 6A 24 89 4D F8 E8 ? ? ? ? 83 C4 04 85 C0 74 11 68 ? ? ? ? 8B C8 E8 ? ? ? ? 89 45 FC EB 07");
	g_PopulateFromScriptHook.SetFunc(VGui::PopulateFromScript);

	VGui::ApplySchemeSettings = (decltype(VGui::ApplySchemeSettings))Memory::Scan(MODULE("client"), "55 8B EC 8B 45 08 56 57 50 8B F1 E8 ? ? ? ? 6A 00 68 ? ? ? ? 68 ? ? ? ? 6A 00 6A 00 68 ? ? ? ? 8B CE E8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14 68 ? ? ? ? 89 86 ? ? ? ? E8 ? ? ? ?");
	g_ApplySchemeSettingsHook.SetFunc(VGui::ApplySchemeSettings);

	VGui::OpenWindow = (decltype(VGui::OpenWindow))Memory::Scan(MODULE("client"), "55 8B EC 83 EC 10 53 56 8B F1 F3 0F 10 86 ? ? ? ? 0F 2E 05 ? ? ? ? 9F 57 F6 C4 44 7B 10 F3 0F 10 05 ? ? ? ? F3 0F 11 86 ? ? ? ?");
	g_OpenWindowHook.SetFunc(VGui::OpenWindow);

	VGui::MainMenuOnCommand = (decltype(VGui::MainMenuOnCommand))Memory::Scan(MODULE("client"), "55 8B EC 81 EC ? ? ? ? 53 56 57 8B F9 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B F0 89 75 FC E8 ? ? ? ? 8B 5D 08 85 C0 74 11 56 56 53 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 10");
	g_MainMenuOnCommandHook.SetFunc(VGui::MainMenuOnCommand);

	VGui::InGameMainMenuOnCommand = (decltype(VGui::InGameMainMenuOnCommand))Memory::Scan(MODULE("client"), "55 8B EC 83 EC 3C 53 56 57 8B F1 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B F8 E8 ? ? ? ? 8B 5D 08 85 C0 74 11 57 57 53 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 10");
	g_InGameMainMenuOnCommandHook.SetFunc(VGui::InGameMainMenuOnCommand);

	return this->hasLoaded = this->ipanel;
}
void VGui::Shutdown() {
	this->OverrideMenu(false);

	Interface::Delete(this->ipanel);
}

VGui *vgui;