#include "GameRecord.hpp"

#include "Modules/Client.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Console.hpp"
#include "Modules/Server.hpp"
#include "Event.hpp"

Variable p2gr_record_players("p2gr_record_players", "1", "Include players in P2GR recordings.\n");
Variable p2gr_record_cameras("p2gr_record_cameras", "0", "Include cameras in P2GR recordings.\n");
Variable p2gr_record_viewmodels("p2gr_record_viewmodels", "0", "Include viewmodels in P2GR recordings.\n");
Variable p2gr_record_others("p2gr_record_others", "0", "Include others in P2GR recordings.\n");
Variable p2gr_framerate("p2gr_framerate", "30", 1, "Frame rate of P2GR recording.\n");

GameRecordFs::GameRecordFs()
	: recording(false) {
}

GameRecordFs::~GameRecordFs() {
	EndRecording();
}

bool GameRecordFs::GetRecording() {
	return recording;
}

bool GameRecordFs::StartRecording(const char *fileName, int version) {
	EndRecording();

	recording = true;

	ClearDictionary();
	file = 0;

	hiddenFileOffset = 0;
	hidden.clear();

	fopen_s(&file, fileName, "wb");

	if (file) {
		fputs("p2fxGameRecord", file);
		fputc('\0', file);

		fwrite(&version, sizeof(version), 1, file);
		return true;
	} else {
		recording = false;
		return false;
	}
}

void GameRecordFs::EndRecording() {
	if (!recording)
		return;

	if (file) {
		fclose(file);
	}

	ClearDictionary();

	recording = false;
}

void GameRecordFs::BeginFrame(float frameTime) {
	if (!recording) return;

	WriteDictionary("p2fxFrame");
	Write((float)frameTime);
	hiddenFileOffset = ftell(file);
	Write((int)0);
}

void GameRecordFs::EndFrame() {
	if (!recording) return;

	if (file && hiddenFileOffset && 0 < hidden.size()) {
		WriteDictionary("p2fxHidden");

		size_t curOffset = ftell(file);

		int offset = (int)(curOffset - hiddenFileOffset);

		fseek(file, hiddenFileOffset, SEEK_SET);
		Write((int)offset);
		fseek(file, curOffset, SEEK_SET);

		Write((int)hidden.size());

		for (std::set<int>::iterator it = hidden.begin(); it != hidden.end(); ++it) {
			Write((int)(*it));
		}

		hidden.clear();
		hiddenFileOffset = 0;
	}

	WriteDictionary("p2fxFrameEnd");
}

void GameRecordFs::WriteDictionary(const char *value) {
	int idx = GetDictionary(value);

	Write(idx);

	if (-1 == idx) {
		Write(value);
	}
}

void GameRecordFs::Write(bool value) {
	if (!file) return;

	unsigned char ucValue = value ? 1 : 0;

	fwrite(&ucValue, sizeof(ucValue), 1, file);
}

void GameRecordFs::Write(int value) {
	if (!file) return;

	fwrite(&value, sizeof(value), 1, file);
}

void GameRecordFs::Write(float value) {
	if (!file) return;

	fwrite(&value, sizeof(value), 1, file);
}

void GameRecordFs::Write(double value) {
	if (!file) return;

	fwrite(&value, sizeof(value), 1, file);
}

void GameRecordFs::Write(const char *value) {
	if (!file) return;

	fputs(value, file);
	fputc('\0', file);
}

void GameRecordFs::MarkHidden(int value) {
	hidden.insert(value);
}

FILE *GameRecordFs::GetFile() {
	return file;
}

void GameRecordFs::ClearDictionary() {
	dictionary.clear();
}

int GameRecordFs::GetDictionary(const char *value) {
	std::string sValue(value);

	std::map<std::string, int>::iterator it = dictionary.find(sValue);

	if (it != dictionary.end()) {
		const std::pair<const std::string, int> &pair = *it;
		return pair.second;
	}

	size_t oldDictSize = dictionary.size();

	dictionary[sValue] = oldDictSize;
	return -1;
}

GameRecord *gameRecord;

GameRecord::GameRecord() {
	this->hasLoaded = true;

	gameRecordFs = new GameRecordFs();
}

GameRecord::~GameRecord() {
	delete gameRecordFs;
}

void GameRecord::StartRecording(const char *fileName) {
	engine->ExecuteCommand("host_timescale 0");
	engine->ExecuteCommand(Utils::ssprintf("host_framerate %d", p2gr_framerate.GetInt()).c_str());

	gameRecordFs->StartRecording(fileName, GetP2grVersion());

	if (GetRecording()) {
		client->EnableRecordingMode(client->g_ClientTools->ThisPtr(), true);
	}

	console->Print("P2FX: Started recording game.\n");
}

void GameRecord::EndRecording() {
	if (GetRecording()) {
		client->EnableRecordingMode(client->g_ClientTools->ThisPtr(), false);
	}

	gameRecordFs->EndRecording();

	engine->ExecuteCommand("host_timescale 1.0");
	engine->ExecuteCommand("host_framerate 0");

	console->Print("P2FX: Stopped recording.\n");
}

void GameRecord::Write(Vector const &value) {
	gameRecordFs->Write((float)value.x);
	gameRecordFs->Write((float)value.y);
	gameRecordFs->Write((float)value.z);
}

void GameRecord::Write(QAngle const &value) {
	gameRecordFs->Write((float)value.x);
	gameRecordFs->Write((float)value.y);
	gameRecordFs->Write((float)value.z);
}

void GameRecord::Write(Quaternion const &value) {
	gameRecordFs->Write((float)value.x);
	gameRecordFs->Write((float)value.y);
	gameRecordFs->Write((float)value.z);
	gameRecordFs->Write((float)value.w);
}

void GameRecord::WriteMatrix3x4(matrix3x4_t const &value) {
	gameRecordFs->Write(value.m_flMatVal[0][0]);
	gameRecordFs->Write(value.m_flMatVal[0][1]);
	gameRecordFs->Write(value.m_flMatVal[0][2]);
	gameRecordFs->Write(value.m_flMatVal[0][3]);

	gameRecordFs->Write(value.m_flMatVal[1][0]);
	gameRecordFs->Write(value.m_flMatVal[1][1]);
	gameRecordFs->Write(value.m_flMatVal[1][2]);
	gameRecordFs->Write(value.m_flMatVal[1][3]);

	gameRecordFs->Write(value.m_flMatVal[2][0]);
	gameRecordFs->Write(value.m_flMatVal[2][1]);
	gameRecordFs->Write(value.m_flMatVal[2][2]);
	gameRecordFs->Write(value.m_flMatVal[2][3]);
}

void GameRecord::WriteBones(bool hasBones, const matrix3x4_t &parentTransform) {
	gameRecordFs->Write((bool)hasBones);
	if (hasBones) {
		gameRecordFs->Write((int)boneStates.size());
		matrix3x4_t bones;
		matrix3x4_t tmp;
		matrix3x4_t parentInverse;
		InvertMatrix(parentTransform, parentInverse);
		for (size_t i = 0; i < boneStates.size(); i++) {
			auto &boneState = boneStates[i];

			if (!(boneState.flags & BONE_USED_BY_ANYTHING)) {
				bones.m_flMatVal[0][0] = 1;
				bones.m_flMatVal[0][1] = 0;
				bones.m_flMatVal[0][2] = 0;
				bones.m_flMatVal[0][3] = 0;
				bones.m_flMatVal[1][0] = 0;
				bones.m_flMatVal[1][1] = 1;
				bones.m_flMatVal[1][2] = 0;
				bones.m_flMatVal[1][3] = 0;
				bones.m_flMatVal[2][0] = 0;
				bones.m_flMatVal[2][1] = 0;
				bones.m_flMatVal[2][2] = 1;
				bones.m_flMatVal[2][3] = 0;
			} else {
				bool bRoot = boneState.parent == -1;

				if (!bRoot) {
					InvertMatrix(boneStates[boneState.parent].matrix, tmp);
					Math::ConcatTransforms(tmp, boneState.matrix, bones);
				} else {
					Math::ConcatTransforms(parentInverse, boneState.matrix, bones);
				}
			}

			WriteMatrix3x4(bones);
		}
	}
}

bool GameRecord::InvertMatrix(const matrix3x4_t &matrix, matrix3x4_t &out_matrix) {
	double b0[4] = {1, 0, 0, 0};
	double b1[4] = {0, 1, 0, 0};
	double b2[4] = {0, 0, 1, 0};
	double b3[4] = {0, 0, 0, 1};

	double M[4][4] = {
		matrix.m_flMatVal[0][0], matrix.m_flMatVal[0][1], matrix.m_flMatVal[0][2], matrix.m_flMatVal[0][3], matrix.m_flMatVal[1][0], matrix.m_flMatVal[1][1], matrix.m_flMatVal[1][2], matrix.m_flMatVal[1][3], matrix.m_flMatVal[2][0], matrix.m_flMatVal[2][1], matrix.m_flMatVal[2][2], matrix.m_flMatVal[2][3], 0, 0, 0, 1};

	unsigned char P[4];
	unsigned char Q[4];

	double L[4][4];
	double U[4][4];

	if (!Math::LUdecomposition(M, P, Q, L, U))
		return false;

	double inv0[4] = {1, 0, 0, 0};
	double inv1[4] = {0, 1, 0, 0};
	double inv2[4] = {0, 0, 1, 0};
	double inv3[4] = {0, 0, 0, 1};

	Math::SolveWithLU(L, U, P, Q, b0, inv0);
	Math::SolveWithLU(L, U, P, Q, b1, inv1);
	Math::SolveWithLU(L, U, P, Q, b2, inv2);
	Math::SolveWithLU(L, U, P, Q, b3, inv3);

	out_matrix.m_flMatVal[0][0] = (float)inv0[0];
	out_matrix.m_flMatVal[0][1] = (float)inv1[0];
	out_matrix.m_flMatVal[0][2] = (float)inv2[0];
	out_matrix.m_flMatVal[0][3] = (float)inv3[0];

	out_matrix.m_flMatVal[1][0] = (float)inv0[1];
	out_matrix.m_flMatVal[1][1] = (float)inv1[1];
	out_matrix.m_flMatVal[1][2] = (float)inv2[1];
	out_matrix.m_flMatVal[1][3] = (float)inv3[1];

	out_matrix.m_flMatVal[2][0] = (float)inv0[2];
	out_matrix.m_flMatVal[2][1] = (float)inv1[2];
	out_matrix.m_flMatVal[2][2] = (float)inv2[2];
	out_matrix.m_flMatVal[2][3] = (float)inv3[2];

	return true;
}

double GameRecord::ScaleFov(double width, double height, double fov) {
	if (!height) return fov;

	double engineAspectRatio = width / height;
	double defaultAscpectRatio = 4.0 / 3.0;
	double ratio = engineAspectRatio / defaultAscpectRatio;
	double halfAngle = 0.5 * fov * (2.0 * M_PI / 360.0);
	double t = ratio * tan(halfAngle);
	return 2.0 * atan(t) / (2.0 * M_PI / 360.0);
}

void GameRecord::CaptureBones(CStudioHdr *hdr, matrix3x4_t *pBoneState) {
	boneStates.clear();
	if (hdr != nullptr && pBoneState != nullptr) {
		boneStates.resize(hdr->numbones());
		for (int i = 0; i < hdr->numbones(); ++i) {
			const mstudiobone_t *pBone = hdr->pBone(i);

			auto &boneState = boneStates[i];

			boneState.parent = pBone->parent;
			boneState.flags = pBone->flags;
			boneState.matrix = pBoneState[i];
		}
	}
}

void GameRecord::OnPostToolMessage(HTOOLHANDLE hEntity, KeyValues *msg) {
	if (!(hEntity != HTOOLHANDLE_INVALID && msg))
		return;

	const char *msgName = msg->GetName();

	if (!strcmp(msgName, "entity_state")) {
		if (GetRecording()) {
			void *ent = client->GetEntity(client->g_ClientTools->ThisPtr(), hEntity);

			bool isPlayer = client->IsPlayer(client->g_ClientTools->ThisPtr(), ent);
			bool isWeapon = client->IsWeapon(client->g_ClientTools->ThisPtr(), ent);
			bool isViewModel = client->IsViewModel(client->g_ClientTools->ThisPtr(), ent);

			bool wasVisible = false;
			bool hasParentTransform = false;
			matrix3x4_t parentTransform;

			auto baseEntRs = (BaseEntityRecordingState_t *)(msg->GetPtr("baseentity"));

			if (baseEntRs && !baseEntRs->m_bVisible) {
				std::map<HTOOLHANDLE, bool>::iterator it = trackedHandles.find(hEntity);
				if (it != trackedHandles.end() && it->second) {
					MarkHidden((int)(it->first));

					it->second = false;
				}

				return;
			}

			if (p2gr_record_others.GetBool() || (p2gr_record_players.GetBool() && (isPlayer || isWeapon)) || (p2gr_record_viewmodels.GetBool() && isViewModel)) {
				WriteDictionary("entity_state");
				Write((int)hEntity);
				if (baseEntRs) {
					wasVisible = baseEntRs->m_bVisible;

					WriteDictionary("baseentity");
					WriteDictionary(baseEntRs->m_pModelName);
					Write((bool)wasVisible);

					hasParentTransform = true;
					Math::AngleMatrix(baseEntRs->m_vecRenderAngles, baseEntRs->m_vecRenderOrigin, parentTransform);
					WriteMatrix3x4(parentTransform);
				}

				trackedHandles[hEntity] = wasVisible;

				auto baseAnimRs = (BaseAnimatingRecordingState_t *)(msg->GetPtr("baseanimating"));
				if (baseAnimRs && hasParentTransform) {
					WriteDictionary("baseanimating");
					WriteBones(baseAnimRs->m_pBoneList != nullptr, parentTransform);
				}

				if (p2gr_record_cameras.GetBool()) {
					auto *camRs = (CameraRecordingState_t *)(msg->GetPtr("camera"));
					if (camRs) {
						int x, y;
						engine->GetScreenSize(nullptr, x, y);

						WriteDictionary("camera");
						Write((bool)camRs->m_bThirdPerson);
						Write(camRs->m_vecEyePosition);
						Write(camRs->m_vecEyeAngles);
						Write((float)ScaleFov(x, y, (float)camRs->m_flFOV));
					}
				}

				WriteDictionary("/");

				bool viewModel = msg->GetBool("viewmodel");

				Write((bool)viewModel);
			}
		}
	} else if (!strcmp(msgName, "deleted")) {
		std::map<HTOOLHANDLE, bool>::iterator it = trackedHandles.find(hEntity);
		if (it != trackedHandles.end()) {
			if (GetRecording()) {
				WriteDictionary("deleted");
				Write((int)(it->first));
			}

			trackedHandles.erase(it);
		}
	} 
	else if (!strcmp(msgName, "created")) {
		if (client->ShouldRecord(client->g_ClientTools->ThisPtr(), hEntity)) {
			client->SetRecording(client->g_ClientTools->ThisPtr(), hEntity, true);
		}
	}
}

void GameRecord::OnBeforeFrameRenderStart() {
	if (!gameRecordFs->GetRecording())
		return;
	
	gameRecordFs->BeginFrame(server->gpGlobals->absoluteframetime);
}

void GameRecord::OnAfterFrameRenderEnd() {
	if (!gameRecordFs->GetRecording())
		return;

	gameRecordFs->EndFrame();
}

CON_COMMAND(p2gr_start, "p2gr_start <filename> - Start P2GR recording.\n") {
	if (args.ArgC() >= 2) {
		gameRecord->StartRecording(Utils::ssprintf("%s.%s", args[1], "p2gr").c_str());
	} else {
		console->Print(p2gr_start.ThisPtr()->m_pszHelpString);
	}
}

CON_COMMAND(p2gr_end, "End P2GR recording.\n") {
	gameRecord->EndRecording();
}