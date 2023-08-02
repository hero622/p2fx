#pragma once
#include "Utils.hpp"

#define P2FX_PLUGIN_SIGNATURE "p2fx v" P2FX_VERSION

// CServerPlugin
#define CServerPlugin_m_Size 16
#define CServerPlugin_m_Plugins 4

class Plugin {
public:
	CPlugin *ptr;
	int index;

public:
	Plugin();
};
