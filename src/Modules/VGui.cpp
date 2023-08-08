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

REDECL(VGui::PaintTraverse);
REDECL(VGui::PopulateSaveGameList);

DETOUR(VGui::PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce) {
	static int state = 0;

	if (state < 5) {
		auto name = vgui->GetName(vgui->ipanel->ThisPtr(), vguiPanel);

		state += 4;

		/* if (!strcmp(name, "BtnPlaySolo") || !strcmp(name, "BtnCoOp") || !strcmp(name, "BtnExtras") || !strcmp(name, "BtnEconUI")) {
			vgui->SetVisible(vgui->ipanel->ThisPtr(), vguiPanel, false);

			state++;
		} */

		if (!strcmp(name, "BtnCommunity")) {
			// you just kinda have to guess the module as far as i can tell:
			// find modules by searching for VGui_InitInterfacesList and VGui_InitMatSysInterfacesList in src
			// some ones i found that seem promising (case doesnt matter i think):
			// ClientDLL (most useful)W
			// CLIENT
			// MATSURFACE
			// GAMEDLL
			// GameUI
			BaseModHybridButton *button = (BaseModHybridButton *)vgui->GetPanel(vgui->ipanel->ThisPtr(), vguiPanel, "ClientDLL");

			button->SetText("DEMO VIEWER");

			state++;
		}
	}

	return VGui::PaintTraverse(thisptr, vguiPanel, forceRepaint, allowForce);
}

extern Hook g_PopulateSaveGameListHook;
DETOUR(VGui::PopulateSaveGameList) {
	g_PopulateSaveGameListHook.Disable();
	auto ret = VGui::PopulateSaveGameList(thisptr);
	g_PopulateSaveGameListHook.Enable();

	auto m_pSaveGameList = *reinterpret_cast<GenericPanelList **>((uintptr_t)thisptr + 0x848);

	m_pSaveGameList->RemoveAllPanelItems();

	// delete m_pSaveGameList;

	return ret;

	/* SaveGameItem *newItem = new SaveGameItem(m_pSaveGameList, "SaveGameItem");
	m_pSaveGameList->AddPanelItem(newItem, true);

	return 0; */
}
Hook g_PopulateSaveGameListHook(&VGui::PopulateSaveGameList_Hook);

bool VGui::Init() {
	this->ipanel = Interface::Create(this->Name(), "VGUI_Panel009");

	if (this->ipanel) {
		this->SetVisible = this->ipanel->Original<_SetVisible>(14);
		this->GetName = this->ipanel->Original<_GetName>(36);
		this->GetPanel = this->ipanel->Original<_GetPanel>(55);

		this->ipanel->Hook(VGui::PaintTraverse_Hook, VGui::PaintTraverse, 41);

		// vgui stuff are all over everywhere, base level stuff is in vgui2, most panels are in client
		VGui::PopulateSaveGameList = (decltype(VGui::PopulateSaveGameList))Memory::Scan(MODULE("client"), "55 8B EC 51 53 56 8B F1 8B 8E ? ? ? ? 8B 01 8B 90 ? ? ? ? 57 FF D2 8B 8E ? ? ? ? 8B 01 8B 90 ? ? ? ? 6A 00 FF D2 80 BE ? ? ? ? ? 75 0D 83 BE ? ? ? ? ? 0F 85 ? ? ? ?");
		g_PopulateSaveGameListHook.SetFunc(VGui::PopulateSaveGameList);
	}

	return this->hasLoaded = this->ipanel;
}
void VGui::Shutdown() {
	Interface::Delete(this->ipanel);
}

VGui *vgui;