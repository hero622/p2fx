#pragma once

#include "Feature.hpp"

class DemoViewer : public Feature {
private:
	int gotoTick = 0;

public:
	int g_demoStart = 0;
	int g_demoPlaybackTicks = 0;

	DemoViewer();

	void Think();
	void OnDemoStart();
	void ParseDemoData();
};

extern DemoViewer *demoViewer;