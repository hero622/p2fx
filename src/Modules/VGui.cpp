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

REDECL(VGui::PaintTraverse);
REDECL(VGui::PopulateFromScript);
REDECL(VGui::ApplySchemeSettings);

void VGui::InitImgs() {
	g_chapterImgs[0] = vgui->GetImageId("vgui/chapters/coopcommentary_chapter1");
	g_chapterImgs[1] = vgui->GetImageId("vgui/chapters/chapter1");
	g_chapterImgs[2] = vgui->GetImageId("vgui/chapters/chapter2");
	g_chapterImgs[3] = vgui->GetImageId("vgui/chapters/chapter3");
	g_chapterImgs[4] = vgui->GetImageId("vgui/chapters/chapter4");
	g_chapterImgs[5] = vgui->GetImageId("vgui/chapters/chapter5");
	g_chapterImgs[6] = vgui->GetImageId("vgui/chapters/chapter6");
	g_chapterImgs[7] = vgui->GetImageId("vgui/chapters/chapter7");
	g_chapterImgs[8] = vgui->GetImageId("vgui/chapters/chapter8");
	g_chapterImgs[9] = vgui->GetImageId("vgui/chapters/chapter9");
}

int VGui::GetImageId(const char *pImageName) {
	int nImageId = surface->CreateNewTextureID(surface->matsurface->ThisPtr(), false);
	surface->DrawSetTextureFile(surface->matsurface->ThisPtr(), nImageId, pImageName, true, false);

	return nImageId;
}

extern Hook g_PopulateFromScriptHook;
extern Hook g_ApplySchemeSettingsHook;
extern Hook g_ActivateSelectedItemHook;

void VGui::OverrideMenu(bool state) {
	if (this->g_vguiState < VGUI_LOADED || (this->g_vguiState == VGUI_OVERWRITTEN && state))
		return;

	for (auto panel : this->g_panels) {
		vgui->SetVisible(vgui->ipanel->ThisPtr(), panel, !state);
	}

	// you just kinda have to guess the module as far as i can tell:
	// find modules by searching for VGui_InitInterfacesList and VGui_InitMatSysInterfacesList in src
	// some ones i found that seem promising (case doesnt matter i think):
	// ClientDLL (most useful)
	// CLIENT
	// MATSURFACE
	// GAMEDLL
	// GameUI
	Label *label = (Label *)vgui->GetPanel(vgui->ipanel->ThisPtr(), this->g_extrasBtn, "ClientDLL");
	// BaseModHybridButton class inherits from Label so we can just use that
	label->SetText(state ? "DEMO VIEWER" : "#L4D360UI_MainMenu_Extras");

	this->g_vguiState = state ? VGUI_OVERWRITTEN : VGUI_RESET;

	if (this->g_vguiState == VGUI_RESET) {
		(*(CUtlVector<ExtraInfo_t> *)vgui->g_ExtraInfos).RemoveAll();
		this->g_pInfoList->RemoveAllPanelItems();

		g_PopulateFromScriptHook.Disable(true);
		VGui::PopulateFromScript(this->g_extrasDialog);

		g_ApplySchemeSettingsHook.Disable(true);
		VGui::ApplySchemeSettings(this->g_extrasDialog, this->g_pScheme);
	}
}

ON_EVENT(SESSION_END) {
	vgui->g_panels.clear();
	vgui->g_vguiState = VGui::VGUI_IDLE;
}

DETOUR(VGui::PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce) {
	auto name = vgui->GetName(vgui->ipanel->ThisPtr(), vguiPanel);
	
	if (vgui->g_vguiState < VGUI_LOADED) {
		if (!strcmp(name, "BtnPlaySolo") || !strcmp(name, "BtnCoOp") || !strcmp(name, "BtnCommunity") || !strcmp(name, "BtnEconUI")) {
			vgui->g_panels.push_back(vguiPanel);
			vgui->g_vguiState++;
		}
		if (!strcmp(name, "BtnExtras")) {
			vgui->g_extrasBtn = vguiPanel;
			vgui->g_vguiState++;
		}
	} else if (vgui->g_vguiState == VGUI_LOADED) {
		vgui->OverrideMenu(true);
		vgui->g_curDirectory = engine->GetGameDirectory();
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
	// back button
	if (std::filesystem::path(path) != std::filesystem::path(engine->GetGameDirectory())) {
		int nIndex = m_ExtraInfos.AddToTail();
		m_ExtraInfos[nIndex].m_TitleString = "..";
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
			m_ExtraInfos[nIndex].m_TitleString = dirname.c_str();
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
			}
		}
	}
}

DETOUR_T(void, VGui::PopulateFromScript) {
	vgui->g_ExtraInfos = (uintptr_t)thisptr + 0x830;
	auto &m_ExtraInfos = *(CUtlVector<ExtraInfo_t> *)vgui->g_ExtraInfos;

	vgui->EnumerateFiles(m_ExtraInfos, vgui->g_curDirectory);
}
Hook g_PopulateFromScriptHook(&VGui::PopulateFromScript_Hook);

DETOUR_T(void, VGui::ApplySchemeSettings, void *pScheme) {
	vgui->g_extrasDialog = thisptr;
	vgui->g_pScheme = pScheme;
	vgui->g_pInfoList = *reinterpret_cast<GenericPanelList **>((uintptr_t)thisptr + 0x84C);

	g_ApplySchemeSettingsHook.Disable();
	VGui::ApplySchemeSettings(thisptr, pScheme);
	g_ApplySchemeSettingsHook.Enable();
}
Hook g_ApplySchemeSettingsHook(&VGui::ApplySchemeSettings_Hook);

bool VGui::Init() {
	this->ipanel = Interface::Create(this->Name(), "VGUI_Panel009");

	if (this->ipanel) {
		this->SetVisible = this->ipanel->Original<_SetVisible>(14);
		this->GetName = this->ipanel->Original<_GetName>(36);
		this->GetPanel = this->ipanel->Original<_GetPanel>(55);

		this->ipanel->Hook(VGui::PaintTraverse_Hook, VGui::PaintTraverse, 41);

		// vgui stuff are all over everywhere, base level stuff is in vgui2, most panels are in client
		VGui::PopulateFromScript = (decltype(VGui::PopulateFromScript))Memory::Scan(MODULE("client"), "55 8B EC 83 EC 1C 6A 24 89 4D F8 E8 ? ? ? ? 83 C4 04 85 C0 74 11 68 ? ? ? ? 8B C8 E8 ? ? ? ? 89 45 FC EB 07");
		g_PopulateFromScriptHook.SetFunc(VGui::PopulateFromScript);

		VGui::ApplySchemeSettings = (decltype(VGui::ApplySchemeSettings))Memory::Scan(MODULE("client"), "55 8B EC 8B 45 08 56 57 50 8B F1 E8 ? ? ? ? 6A 00 68 ? ? ? ? 68 ? ? ? ? 6A 00 6A 00 68 ? ? ? ? 8B CE E8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14 68 ? ? ? ? 89 86 ? ? ? ? E8 ? ? ? ?");
		g_ApplySchemeSettingsHook.SetFunc(VGui::ApplySchemeSettings);

		this->InitImgs();
	}

	return this->hasLoaded = this->ipanel;
}
void VGui::Shutdown() {
	this->OverrideMenu(false);

	Interface::Delete(this->ipanel);
}

VGui *vgui;