#pragma once
#include "Variable.hpp"
#include "Demo.hpp"

#include <map>
#include <string>

enum PortalColor {
	BLUE,
	ORANGE,
};

// Basic demo parser which can handle Portal 2 and Half-Life 2 demos
class DemoParser {
public:
	bool headerOnly;
	int outputMode;
	bool hasAlignmentByte;
	int maxSplitScreenClients;

public:
	struct Segment {
		char *name;
		int ticks;
	};
	struct SplitInfo {
		char *name;
		size_t nSegments = 0;
		Segment *segments;
	};
	struct SpeedrunTime {
		size_t nSplits = 0;
		SplitInfo *splits;
	} speedrunTime;

public:
	DemoParser();
	std::string DecodeCustomData(char *data);
	void Adjust(Demo *demo);
	bool Parse(std::string filePath, Demo *demo, bool ghostRequest = false);
};