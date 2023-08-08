#include "VGui.hpp"

#include "Command.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "Surface.hpp"

REDECL(VGui::PaintTraverse);
REDECL(VGui::PopulateFromScript);

int VGui::GetImageId(const char *pImageName) {
	int nImageId = surface->CreateNewTextureID(surface->matsurface->ThisPtr(), true);
	surface->DrawSetTextureFile(surface->matsurface->ThisPtr(), nImageId, pImageName, true, false);

	return nImageId;
}

void VGui::OverrideMenu(bool state) {
	if (this->vguiState < VGUI_LOADED || (this->vguiState == VGUI_OVERWRITTEN && state))
		return;

	for (auto panel : this->panels) {
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
	Label *label = (Label *)vgui->GetPanel(vgui->ipanel->ThisPtr(), this->extrasBtn, "ClientDLL");
	// BaseModHybridButton class inherits from Label so we can just use that
	label->SetText(state ? "DEMO VIEWER" : "#L4D360UI_MainMenu_Extras");

	this->vguiState = state ? VGUI_OVERWRITTEN : VGUI_RESET;
}

ON_EVENT(FRAME) {
	vgui->OverrideMenu(true);
}

DETOUR(VGui::PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce) {
	auto name = vgui->GetName(vgui->ipanel->ThisPtr(), vguiPanel);
	
	if (vgui->vguiState < VGUI_LOADED) {
		if (!strcmp(name, "BtnPlaySolo") || !strcmp(name, "BtnCoOp") || !strcmp(name, "BtnCommunity") || !strcmp(name, "BtnEconUI")) {
			vgui->panels.push_back(vguiPanel);
			vgui->vguiState++;
		}
		if (!strcmp(name, "BtnExtras")) {
			vgui->extrasBtn = vguiPanel;
			vgui->vguiState++;
		}
	}

	return VGui::PaintTraverse(thisptr, vguiPanel, forceRepaint, allowForce);
}

extern Hook g_PopulateFromScriptHook;
DETOUR_T(void, VGui::PopulateFromScript) {
	if (vgui->vguiState == VGUI_RESET) {
		g_PopulateFromScriptHook.Disable();
		VGui::PopulateFromScript(thisptr);
		g_PopulateFromScriptHook.Enable();
		return;
	}

	uintptr_t m_ExtraInfosAddr = (uintptr_t)thisptr + 0x830;
	CUtlVector<ExtraInfo_t> *m_ExtraInfos = (CUtlVector<ExtraInfo_t> *)(m_ExtraInfosAddr);

	m_ExtraInfos->RemoveAll();

	int m_nImageId = vgui->GetImageId("vgui/chapters/chapter5");

	for (const auto &p : std::filesystem::directory_iterator(engine->GetGameDirectory())) {
		auto path = p.path();

		if (m_ExtraInfos->m_Size == 1170)
			break;

		if (path.extension() == ".dem") {
			std::string filename = path.stem().string();

			auto safepath = path.string().substr(path.string().find("portal2") + 8);

			ExtraInfo_t extraInfo;
			extraInfo.m_TitleString = filename.c_str();
			extraInfo.m_SubtitleString = filename.c_str();
			extraInfo.m_MapName = "";
			extraInfo.m_VideoName = "";
			extraInfo.m_URLName = "";
			extraInfo.m_Command = Utils::ssprintf("playdemo %s", safepath.c_str()).c_str();
			extraInfo.m_nImageId = m_nImageId;

			m_ExtraInfos->AddToTail(extraInfo);
		}
	}
}
Hook g_PopulateFromScriptHook(&VGui::PopulateFromScript_Hook);

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
	}

	return this->hasLoaded = this->ipanel;
}
void VGui::Shutdown() {
	this->OverrideMenu(false);

	Interface::Delete(this->ipanel);
}

VGui *vgui;