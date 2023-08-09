#include "Tier0.hpp"

bool Tier0::Init() {
	auto tier0 = Memory::GetModuleHandleByName(this->Name());
	if (tier0) {
		g_pMemAlloc = *Memory::GetSymbolAddress<IMemAlloc **>(tier0, "g_pMemAlloc");

		Memory::CloseModuleHandle(tier0);
	}

	return this->hasLoaded = tier0 && g_pMemAlloc;
}
void Tier0::Shutdown() {
}

Tier0 *tier0;