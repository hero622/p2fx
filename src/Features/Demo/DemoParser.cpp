#include "DemoParser.hpp"

#include "Command.hpp"
#include "Features/Hud/Hud.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Variable.hpp"

#include <fstream>
#include <sstream>
#include <string>

DemoParser::DemoParser()
	: headerOnly(false)
	, outputMode()
	, hasAlignmentByte(true)
	, maxSplitScreenClients(2) {
}
std::string DemoParser::DecodeCustomData(char *data) {
	if (data[0] == 0x03 || data[0] == 0x04) {  // Entity input data
		std::optional<int> slot;
		if (data[0] == 0x04) {
			slot = data[1];
			++data;
		}

		char *targetname = data + 1;
		size_t targetnameLen = strlen(targetname);
		char *classname = data + 2 + targetnameLen;
		size_t classnameLen = strlen(classname);
		char *inputname = data + 3 + targetnameLen + classnameLen;
		size_t inputnameLen = strlen(inputname);
		char *parameter = data + 4 + targetnameLen + classnameLen + inputnameLen;

		//	console->Print("%s %s %s %s\n", targetname, classname, inputname, parameter);

		return Utils::ssprintf("%s %s %s %s", targetname, classname, inputname, parameter);
	}

	if (data[0] == 0x05) {  // Portal placement
		int slot = data[1];
		PortalColor portal = data[2] ? PortalColor::ORANGE : PortalColor::BLUE;
		Vector pos;
		pos.x = *(float *)(data + 3);
		pos.y = *(float *)(data + 7);
		pos.z = *(float *)(data + 11);

		return Utils::ssprintf("%f %f %f %d %d", pos.x, pos.y, pos.z, slot, portal);
	}

	if (data[0] == 0x06) {  // CM flags
		int slot = data[1];

		return Utils::ssprintf("%d", slot);
	}

	if (data[0] == 0x07) {  // Crouch fly
		int slot = data[1];

		return Utils::ssprintf("%d", slot);
	}

	if (data[0] == 0x0A) {  // Speedrun time
		data += 1;

		speedrunTime.nSplits = *(size_t *)(data);
		data += 4;

		speedrunTime.splits = (SplitInfo *)malloc(speedrunTime.nSplits * sizeof(speedrunTime.splits[0]));

		for (size_t i = 0; i < speedrunTime.nSplits; ++i) {
			speedrunTime.splits[i].name = data;
			data += strlen(speedrunTime.splits[i].name) + 1;

			speedrunTime.splits[i].nSegments = *(size_t *)(data);
			data += 4;

			speedrunTime.splits[i].segments = (Segment *)malloc(speedrunTime.splits[i].nSegments * sizeof(speedrunTime.splits[i].segments[0]));

			for (size_t j = 0; j < speedrunTime.splits[i].nSegments; ++j) {
				speedrunTime.splits[i].segments[j].name = data;
				data += strlen(speedrunTime.splits[i].segments[j].name) + 1;

				speedrunTime.splits[i].segments[j].ticks = *(int *)(data);
				data += 4;
			}
		}
	}

	return std::string();
}

void DemoParser::Adjust(Demo *demo) {
	auto ipt = demo->IntervalPerTick();
	demo->playbackTicks = demo->LastTick();
	demo->playbackTime = ipt * demo->playbackTicks;
}
bool DemoParser::Parse(std::string filePath, Demo *demo, bool ghostRequest) {
	bool gotFirstPositivePacket = false;
	bool gotSync = false;
	try {
		if (filePath.substr(filePath.length() - 4, 4) != ".dem")
			filePath += ".dem";

		console->DevMsg("Trying to parse \"%s\"...\n", filePath.c_str());

		std::ifstream file(filePath, std::ios::in | std::ios::binary);
		if (!file.good())
			return false;

		file.read(demo->demoFileStamp, sizeof(demo->demoFileStamp));
		file.read((char *)&demo->demoProtocol, sizeof(demo->demoProtocol));
		file.read((char *)&demo->networkProtocol, sizeof(demo->networkProtocol));
		file.read(demo->serverName, sizeof(demo->serverName));
		file.read(demo->clientName, sizeof(demo->clientName));
		file.read(demo->mapName, sizeof(demo->mapName));
		file.read(demo->gameDirectory, sizeof(demo->gameDirectory));
		file.read((char *)&demo->playbackTime, sizeof(demo->playbackTime));
		file.read((char *)&demo->playbackTicks, sizeof(demo->playbackTicks));
		file.read((char *)&demo->playbackFrames, sizeof(demo->playbackFrames));
		file.read((char *)&demo->signOnLength, sizeof(demo->signOnLength));

		demo->segmentTicks = -1;

		if (!headerOnly) {
			//	Ghosts
			bool waitForNext = false;
			int lastTick = 0;

			if (demo->demoProtocol != 4) {
				this->hasAlignmentByte = false;
				this->maxSplitScreenClients = 1;
			}

			while (!file.eof() && !file.bad()) {
				unsigned char cmd;
				int32_t tick;

				file.read((char *)&cmd, sizeof(cmd));
				if (cmd == 0x07)  // Stop
					break;

				file.read((char *)&tick, sizeof(tick));

				// Save positive ticks to keep adjustments simple
				if (tick >= 0)
					demo->messageTicks.push_back(tick);

				if (this->hasAlignmentByte)
					file.ignore(1);

				switch (cmd) {
				case 0x01:  // SignOn
				case 0x02:  // Packet
				{
					if (tick > 0 && gotSync && !gotFirstPositivePacket) {
						demo->firstPositivePacketTick = tick;
						gotFirstPositivePacket = true;
					}

					file.ignore((this->maxSplitScreenClients * 76) + 4 + 4);

					int32_t length;
					file.read((char *)&length, sizeof(length));
					file.ignore(length);
					break;
				}
				case 0x03:  // SyncTick
					gotSync = true;
					continue;
				case 0x04:  // ConsoleCmd
				{
					int32_t length;
					file.read((char *)&length, sizeof(length));
					std::string cmd(length, ' ');
					file.read(&cmd[0], length);
					if (!ghostRequest && outputMode >= 1) {
						console->Msg("[%i] %s\n", tick, cmd.c_str());
					}

					if (cmd.find("__END__") != std::string::npos) {
						console->ColorMsg(Color(0, 255, 0, 255), "Segment length -> %d ticks: %.3fs\n", tick, tick / 60.f);
						demo->segmentTicks = tick;
					}
					break;
				}
				case 0x05:  // UserCmd
				{
					int32_t cmd;
					int32_t length;
					file.read((char *)&cmd, sizeof(cmd));
					file.read((char *)&length, sizeof(length));
					file.ignore(length);
					break;
				}
				case 0x06:  // DataTables
				{
					int32_t length;
					file.read((char *)&length, sizeof(length));
					file.ignore(length);
					break;
				}
				case 0x08:  // CustomData or StringTables
				{
					if (demo->demoProtocol == 4) {
						int32_t unk;
						int32_t length;
						file.read((char *)&unk, sizeof(unk));
						file.read((char *)&length, sizeof(length));

						if (ghostRequest) {
							char *data = new char[length];
							file.read(data, length);

							this->DecodeCustomData(data + 8);
						} else {
							file.ignore(length);
						}
					} else {
						int32_t length;
						file.read((char *)&length, sizeof(length));
						file.ignore(length);
					}
					break;
				}
				case 0x09:  // StringTables
				{
					if (demo->demoProtocol != 4)
						return false;
					int32_t length;
					file.read((char *)&length, sizeof(length));
					file.ignore(length);
					break;
				}
				default:
					return false;
				}
			}
		}
		file.close();
	} catch (const std::exception &ex) {
		console->Warning(
			"P2FX: Error occurred when trying to parse the demo file.\n"
			"If you think this is an issue, report it at: https://github.com/hero622/p2fx/issues\n"
			"%s\n",
			std::string(ex.what()));
		return false;
	}
	return true;
}
