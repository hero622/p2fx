#include "P2FX.hpp"

#include "Version.hpp"

#include <cstring>
#include <ctime>
#include <curl/curl.h>

#ifdef _WIN32
#	include <filesystem>
#endif

#include "Cheats.hpp"
#include "Command.hpp"
#include "CrashHandler.hpp"
#include "Event.hpp"
#include "Features.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Modules.hpp"
#include "Variable.hpp"
#include "Input.hpp"

P2FX p2fx;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(P2FX, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, p2fx);


P2FX::P2FX()
	: modules(new Modules())
	, features(new Features())
	, cheats(new Cheats())
	, plugin(new Plugin())
	, game(Game::CreateNew()) {
}

bool P2FX::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	console = new Console();
	if (!console->Init())
		return false;

#ifdef _WIN32
	// The auto-updater can create this file on Windows; we should try
	// to delete it.
	try {
		if (std::filesystem::exists("p2fx.dll.old-auto")) {
			std::filesystem::remove("p2fx.dll.old-auto");
		}
		if (std::filesystem::exists("p2fx.pdb.old-auto")) {
			std::filesystem::remove("p2fx.pdb.old-auto");
		}
	} catch (...) {
	}
#endif

	if (this->game) {
		this->game->LoadOffsets();

		CrashHandler::Init();

		P2fxInitHandler::RunAll();

		curl_global_init(CURL_GLOBAL_ALL);

		tier1 = new Tier1();
		if (tier1->Init()) {
			this->features->AddFeature<Cvars>(&cvars);
			this->features->AddFeature<Session>(&session);
			SpeedrunTimer::Init();
			this->features->AddFeature<EntityList>(&entityList);
			this->features->AddFeature<FovChanger>(&fovChanger);
			this->features->AddFeature<Camera>(&camera);
			this->features->AddFeature<DemoViewer>(&demoViewer);

			this->modules->AddModule<InputSystem>(&inputSystem);
			this->modules->AddModule<Scheme>(&scheme);
			this->modules->AddModule<Surface>(&surface);
			this->modules->AddModule<VGui>(&vgui);
			this->modules->AddModule<Engine>(&engine);
			this->modules->AddModule<Client>(&client);
			this->modules->AddModule<Server>(&server);
			this->modules->AddModule<MaterialSystem>(&materialSystem);
			this->modules->AddModule<FileSystem>(&fileSystem);
			this->modules->AddModule<ShaderApi>(&shaderApi);
			this->modules->InitAll();

			if (engine && engine->hasLoaded) {
				Input::Init();

				engine->demoplayer->Init();
				engine->demorecorder->Init();

				this->cheats->Init();

				this->features->AddFeature<Listener>(&listener);

				if (listener) {
					listener->Init();
				}

				this->SearchPlugin();

				console->PrintActive("Loaded p2fx, Version %s\n", P2FX_VERSION);
				
				return true;
			} else {
				console->Warning("P2FX: Failed to load engine module!\n");
			}
		} else {
			console->Warning("P2FX: Failed to load tier1 module!\n");
		}
	} else {
		console->Warning("P2FX: Game not supported!\n");
	}

	console->Warning("P2FX: Failed to load p2fx!\n");

	if (p2fx.cheats) {
		p2fx.cheats->Shutdown();
	}
	if (p2fx.features) {
		p2fx.features->DeleteAll();
	}

	if (p2fx.modules) {
		p2fx.modules->ShutdownAll();
	}

	// This isn't in p2fx.modules
	if (tier1) {
		tier1->Shutdown();
	}

	Variable::ClearAllCallbacks();
	SAFE_DELETE(p2fx.features)
	SAFE_DELETE(p2fx.cheats)
	SAFE_DELETE(p2fx.modules)
	SAFE_DELETE(p2fx.plugin)
	SAFE_DELETE(p2fx.game)
	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
	return false;
}

// P2FX has to disable itself in the plugin list or the game might crash because of missing callbacks
// This is a race condition though
bool P2FX::GetPlugin() {
	auto s_ServerPlugin = reinterpret_cast<uintptr_t>(engine->s_ServerPlugin->ThisPtr());
	auto m_Size = *reinterpret_cast<int *>(s_ServerPlugin + CServerPlugin_m_Size);
	if (m_Size > 0) {
		auto m_Plugins = *reinterpret_cast<uintptr_t *>(s_ServerPlugin + CServerPlugin_m_Plugins);
		for (auto i = 0; i < m_Size; ++i) {
			auto ptr = *reinterpret_cast<CPlugin **>(m_Plugins + sizeof(uintptr_t) * i);
			if (!std::strcmp(ptr->m_szName, P2FX_PLUGIN_SIGNATURE)) {
				this->plugin->ptr = ptr;
				this->plugin->index = i;
				return true;
			}
		}
	}
	return false;
}
void P2FX::SearchPlugin() {
	this->findPluginThread = std::thread([this]() {
		GO_THE_FUCK_TO_SLEEP(1000);
		if (this->GetPlugin()) {
			this->plugin->ptr->m_bDisable = true;
		} else {
			console->DevWarning("P2FX: Failed to find P2FX in the plugin list!\nTry again with \"plugin_load\".\n");
		}
	});
	this->findPluginThread.detach();
}

void P2FX::Unload() {
	if (unloading) return;
	unloading = true;

	curl_global_cleanup();

	networkManager.Disconnect();

	Variable::ClearAllCallbacks();

	Hook::DisableAll();

	Input::Shutdown();

	if (p2fx.cheats) {
		p2fx.cheats->Shutdown();
	}
	if (p2fx.features) {
		p2fx.features->DeleteAll();
	}

	if (p2fx.GetPlugin()) {
		// P2FX has to unhook CEngine some ticks before unloading the module
		auto unload = std::string("plugin_unload ") + std::to_string(p2fx.plugin->index);
		engine->SendToCommandBuffer(unload.c_str(), SAFE_UNLOAD_TICK_DELAY);
	}

	if (p2fx.modules) {
		p2fx.modules->ShutdownAll();
	}

	// This isn't in p2fx.modules
	if (tier1) {
		tier1->Shutdown();
	}

	SAFE_DELETE(p2fx.features)
	SAFE_DELETE(p2fx.cheats)
	SAFE_DELETE(p2fx.modules)
	SAFE_DELETE(p2fx.plugin)
	SAFE_DELETE(p2fx.game)

	console->Print("Cya :)\n");

	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
}

CON_COMMAND(p2fx_about, "p2fx_about - prints info about P2FX plugin\n") {
	console->Print("p2fx is a speedrun plugin for Source Engine games.\n");
	console->Print("More information at: https://github.com/Zyntex1/p2fx or https://wiki.portal2.sr/P2FX\n");
	console->Print("Game: %s\n", p2fx.game->Version());
	console->Print("Version: " P2FX_VERSION "\n");
	console->Print("Built: " P2FX_BUILT "\n");
}
CON_COMMAND(p2fx_cvars_dump, "p2fx_cvars_dump - dumps all cvars to a file\n") {
	std::ofstream file("game.cvars", std::ios::out | std::ios::trunc | std::ios::binary);
	auto result = cvars->Dump(file);
	file.close();

	console->Print("Dumped %i cvars to game.cvars!\n", result);
}
CON_COMMAND(p2fx_cvars_dump_doc, "p2fx_cvars_dump_doc - dumps all P2FX cvars to a file\n") {
	std::ofstream file("p2fx.cvars", std::ios::out | std::ios::trunc | std::ios::binary);
	auto result = cvars->DumpDoc(file);
	file.close();

	console->Print("Dumped %i cvars to p2fx.cvars!\n", result);
}
CON_COMMAND(p2fx_cvars_lock, "p2fx_cvars_lock - restores default flags of unlocked cvars\n") {
	cvars->Lock();
}
CON_COMMAND(p2fx_cvars_unlock, "p2fx_cvars_unlock - unlocks all special cvars\n") {
	cvars->Unlock();
}
CON_COMMAND(p2fx_cvarlist, "p2fx_cvarlist - lists all P2FX cvars and unlocked engine cvars\n") {
	cvars->ListAll();
}
CON_COMMAND(p2fx_exit, "p2fx_exit - removes all function hooks, registered commands and unloads the module\n") {
	p2fx.Unload();
}

#pragma region Unused callbacks
void P2FX::Pause() {
}
void P2FX::UnPause() {
}
const char *P2FX::GetPluginDescription() {
	return P2FX_PLUGIN_SIGNATURE;
}
void P2FX::LevelInit(char const *pMapName) {
}
void P2FX::ServerActivate(void *pEdictList, int edictCount, int clientMax) {
}
void P2FX::GameFrame(bool simulating) {
}
void P2FX::LevelShutdown() {
}
void P2FX::ClientFullyConnect(void *pEdict) {
}
void P2FX::ClientActive(void *pEntity) {
}
void P2FX::ClientDisconnect(void *pEntity) {
}
void P2FX::ClientPutInServer(void *pEntity, char const *playername) {
}
void P2FX::SetCommandClient(int index) {
}
void P2FX::ClientSettingsChanged(void *pEdict) {
}
int P2FX::ClientConnect(bool *bAllowConnect, void *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return 0;
}
int P2FX::ClientCommand(void *pEntity, const void *&args) {
	return 0;
}
int P2FX::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
	return 0;
}
void P2FX::OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) {
}
void P2FX::OnEdictAllocated(void *edict) {
}
void P2FX::OnEdictFreed(const void *edict) {
}
#pragma endregion
