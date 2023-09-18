#pragma once

#include "Feature.hpp"
#include "Utils.hpp"

class GameRecorder : public Feature {
public:
	GameRecorder();

	void OnPostToolMessage(HTOOLHANDLE hEntity, KeyValues *msg);
};

extern GameRecorder *gameRecorder;