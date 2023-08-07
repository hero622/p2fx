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

#pragma push_macro("GetClassName")
#undef GetClassName

REDECL(VGui::PaintTraverse);

DETOUR(VGui::PaintTraverse, VPANEL vguiPanel, bool forceRepaint, bool allowForce) {
	auto name = vgui->GetName(vgui->ipanel->ThisPtr(), vguiPanel);

	if (!strcmp(name, "BtnPlaySolo")) {
		// you just kinda have to guess the module as far as i can tell:
		// find modules by searching for VGui_InitInterfacesList and VGui_InitMatSysInterfacesList in src
		// some ones i found that seem promising (case doesnt matter i think):
		// ClientDLL (most useful)
		// CLIENT
		// MATSURFACE
		// GAMEDLL
		// GameUI
		vgui_Panel *panel = vgui->GetPanel(vgui->ipanel->ThisPtr(), vguiPanel, "ClientDLL");


		// vgui_Label *label = (vgui_Label *)panel;

		// label->SetText(label, "YO");

		// console->Print("%s: %p\n", name, panel);
	}

	return VGui::PaintTraverse(thisptr, vguiPanel, forceRepaint, allowForce);
}

bool VGui::Init() {
	this->ipanel = Interface::Create(this->Name(), "VGUI_Panel009");

	if (this->ipanel) {
		this->GetName = this->ipanel->Original<_GetName>(36);
		this->GetPanel = this->ipanel->Original<_GetPanel>(55);

		this->ipanel->Hook(VGui::PaintTraverse_Hook, VGui::PaintTraverse, 41);
	}

	return this->hasLoaded = this->ipanel;
}
void VGui::Shutdown() {
	Interface::Delete(this->ipanel);
}

VGui *vgui;

#pragma pop_macro("GetClassName")