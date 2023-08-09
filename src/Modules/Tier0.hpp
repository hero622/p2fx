#pragma once

#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"

class Tier0 : public Module {
public:
	bool Init() override;
	void Shutdown() override;
	const char *Name() override { return MODULE("tier0"); }
};

extern Tier0 *tier0;