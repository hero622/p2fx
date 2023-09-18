#include "GameRecorder.hpp"

#include "Modules/Client.hpp"
#include "Modules/Console.hpp"

GameRecorder *gameRecorder;

GameRecorder::GameRecorder() {
	this->hasLoaded = true;
}

void GameRecorder::OnPostToolMessage(HTOOLHANDLE hEntity, KeyValues *msg) {
	if (!(hEntity != HTOOLHANDLE_INVALID && msg))
		return;

	const char *msgName = msg->GetName();

	void *ent = client->GetEntity(client->g_ClientTools->ThisPtr(), hEntity);

	console->Warning("OnPostToolMessage(%u, %s)\n", hEntity, msgName);

	/* if (!strcmp(msgName, "entity_state")) {
		client->GetToolRecordingState(ent, msg);
	} */
}