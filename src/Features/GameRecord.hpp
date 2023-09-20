#pragma once

#include "Feature.hpp"
#include "Utils.hpp"
#include <map>
#include <set>

class GameRecordFs {
public:
	GameRecordFs();
	~GameRecordFs();

	bool GetRecording();

	bool StartRecording(const char *fileName, int version);

	void EndRecording();

	void BeginFrame(float frameTime);

	void EndFrame();

	void WriteDictionary(const char *value);

	void Write(bool value);
	void Write(int value);
	void Write(float value);
	void Write(double value);
	void Write(const char *value);

	void MarkHidden(int value);

	FILE *GetFile();

private:
	std::map<std::string, int> dictionary;

	size_t hiddenFileOffset;
	std::set<int> hidden;

	bool recording;
	FILE *file;

	void ClearDictionary();

	int GetDictionary(const char *value);
};

class GameRecord : public Feature {
private:
	struct BoneState {
		matrix3x4_t matrix;
		int parent;
		int flags;
	};
	std::vector<BoneState> boneStates;

	std::map<HTOOLHANDLE, bool> trackedHandles;
	
	GameRecordFs *gameRecordFs;

private:
	bool GetRecording() { return gameRecordFs->GetRecording(); }

	void WriteDictionary(const char *value) { gameRecordFs->WriteDictionary(value); }

	void Write(bool value) { gameRecordFs->Write(value); }
	void Write(int value) { gameRecordFs->Write(value); }
	void Write(float value) { gameRecordFs->Write(value); }
	void Write(double value) { gameRecordFs->Write(value); }

	void Write(const char *value) { gameRecordFs->Write(value); }

	void Write(Vector const &value);
	void Write(QAngle const &value);
	void Write(Quaternion const &value);
	void WriteMatrix3x4(matrix3x4_t const &value);
	void WriteBones(bool hasBones, const matrix3x4_t &parentTransform);

	void MarkHidden(int value) { gameRecordFs->MarkHidden(value); }

public:
	GameRecord();
	~GameRecord();

	int GetP2grVersion() {
		return 1;
	}

	void StartRecording(const char *fileName);
	void EndRecording();

	static bool InvertMatrix(const matrix3x4_t &matrix, matrix3x4_t &out_matrix);

	void CaptureBones(CStudioHdr *hdr, matrix3x4_t *pBoneState);

	void OnPostToolMessage(HTOOLHANDLE hEntity, KeyValues *msg);
	void OnBeforeFrameRenderStart();
	void OnAfterFrameRenderEnd();
};

extern GameRecord *gameRecord;