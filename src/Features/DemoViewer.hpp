#pragma once

#include "Feature.hpp"

#include "Demo/DemoParser.hpp"

class DemoViewer : public Feature {
private:
	int gotoTick = -1;

public:
	int g_demoStart = -1;
	int g_demoPlaybackTicks = -1;
	DemoParser::SpeedrunTime g_demoSpeedrunTime;
	int g_demoSpeedrunLength = -1;

	DemoViewer();

	void Think();
	void ParseDemoData();
	void HandleGotoTick();
	void PauseAtEnd();
};

extern DemoViewer *demoViewer;