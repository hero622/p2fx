#include "GameRecorder.hpp"

#include "Modules/Client.hpp"
#include "Modules/Console.hpp"

GameRecorder *gameRecorder;

GameRecorder::GameRecorder() {
	this->hasLoaded = true;
}

void GameRecorder::StartRecording() {
	client->EnableRecordingMode(client->g_ClientTools->ThisPtr(), true);
}

void GameRecorder::EndRecording() {
	client->EnableRecordingMode(client->g_ClientTools->ThisPtr(), false);
}

void GameRecorder::OnPostToolMessage(HTOOLHANDLE hEntity, KeyValues *msg) {
	if (!(hEntity != HTOOLHANDLE_INVALID && msg))
		return;

	const char *msgName = msg->GetName();

	console->DevMsg("OnPostToolMessage(%u, %s)\n", hEntity, msgName);

	if (!strcmp(msgName, "entity_state")) {

	} else if (!strcmp(msgName, "created")) {
		if (client->ShouldRecord(client->g_ClientTools->ThisPtr(), hEntity)) {
			client->SetRecording(client->g_ClientTools->ThisPtr(), hEntity, true);
		}
	}
}

CON_COMMAND(p2fx_startrecording, "") {
	gameRecorder->StartRecording();
}

CON_COMMAND(p2fx_endrecording, "") {
	gameRecorder->EndRecording();
}