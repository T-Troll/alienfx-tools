#include <iostream>
#include <windows.h>

#include "stdafx.h"
#include "LFXUtil.h"
#include "LFXDecl.h"

namespace 
{
	LFXUtil::LFXUtilC lfxUtil;
}

using namespace std;

void printUsage() 
{
	cerr << "Usage: alienfx-cli [command=option,option,option] ... [command=option,option,option] [loop]" << endl
		<< "Commands:\tOptions:" << endl
		<< "set-all\t\tr,g,b[,br] - set all device lights." << endl
		<< "set-one\t\tdev,light,r,g,b[,br] - set one light." << endl
		<< "set-zone\tzone,r,g,b[,br] - set one zone lights." << endl
		<< "set-action\taction,dev,light,r,g,b[,br,r,g,b[,br]] - set light and enable it's action." << endl
		<< "set-zone-action\taction,zone,r,g,b[,br,r,g,b[,br]] - set all zone lights and enable it's action." << endl
		<< "set-tempo\ttempo - selt light action tempo (in milliseconds)" << endl
		<< "status\t\tshows devices and lights id's, names and statuses" << endl
		<< "loop\t\trepeat all commands endlessly, until user press ^c. Should be the last command." << endl << endl
		<< "Zones: left, right, top, bottom, front, rear" << endl
		<< "Actions: pulse, morph (you need 2 colors for morph), color (disable action)" << endl;
}

int main(int argc, char* argv[])
{
	if (argc < 2) 
	{
		printUsage();
		return 1;
	}

	//const ResultT& result(lfxUtil.InitLFX());
	//if (!result.first)
	int res = lfxUtil.InitLFX();
	if ( res != -1) {
		switch (res) {
		case 0: cerr << "Can't load DLL library!" << endl; break;
		case 1: cerr << "Can't init library!" << endl; break;
		case 2: cerr << "No devices found!" << endl; break;
		default: cerr << "Unknown error!" << endl; break;
		}
		return 1;
	}
	const char* command = argv[1];
	for (int cc = 1; cc < argc; cc++) {
		Sleep(100);
		string arg = string(argv[cc]);
		size_t vid = arg.find_first_of('=');
		string command = arg.substr(0, vid);
		string values;
		vector<string> args;
		if (vid != string::npos) {
			size_t vpos = 0;
			values = arg.substr(vid + 1, arg.size());
			while (vpos < values.size()) {
				size_t tvpos = values.find(',', vpos);
				args.push_back(values.substr(vpos, tvpos-vpos));
				vpos = tvpos == string::npos ? values.size() : tvpos+1;
			}
		}
		if (command == "loop") {
			//cerr << "Executing " << command << endl;
			cc = 0;
			continue;
		}
		if (command == "status") {
			//cerr << "Executing " << command << endl;
			lfxUtil.GetStatus();
			continue;
		}
		if (command == "set-tempo") {
			if (args.size() < 1) {
				cerr << "set-tempo: Incorrect argument" << endl;
				continue;
			}
			unsigned tempo = atoi(args.at(0).c_str());
			lfxUtil.SetTempo(tempo);
			//lfxUtil.Update();
			continue;
		}
		if (command == "set-all") {
			//cerr << "Executing " << command << " " << values << endl;
			if (args.size() < 3) {
				cerr << "set-all: Incorrect arguments" << endl;
				continue;
			}
			unsigned zoneCode = LFX_ALL;
			static ColorU color;
			color.cs.red = atoi(args.at(0).c_str());
			color.cs.green = atoi(args.at(1).c_str());
			color.cs.blue = atoi(args.at(2).c_str());
			color.cs.brightness = args.size() > 3 ? atoi(args.at(3).c_str()) : 255;
			lfxUtil.SetLFXColor(zoneCode, color.ci);
			lfxUtil.Update(); 
			continue;
		}
		if (command == "set-one") {
			//cerr << "Executing " << command << " " << values << endl;
			if (args.size() < 5) {
				cerr << "set-one: Incorrect arguments" << endl;
				continue;
			}
			static ColorU color;
			color.cs.red = atoi(args.at(4).c_str());
			color.cs.green = atoi(args.at(3).c_str());
			color.cs.blue = atoi(args.at(2).c_str());
			color.cs.brightness = args.size() > 5 ? atoi(args.at(5).c_str()) : 255;
			lfxUtil.SetOneLFXColor(atoi(args.at(0).c_str()), atoi(args.at(1).c_str()), &color.ci);
			lfxUtil.Update(); 
			continue;
		}
		if (command == "set-zone") {
			//cerr << "Executing " << command << " " << values << endl;
			if (args.size() < 4) {
				cerr << "set-zone: Incorrect arguments" << endl;
				continue;
			}
			unsigned zoneCode = LFX_ALL;
			if (args.at(0) == "left") {
				zoneCode = LFX_ALL_LEFT;
			}
			if (args.at(0) == "right") {
				zoneCode = LFX_ALL_RIGHT;
			}
			if (args.at(0) == "top") {
				zoneCode = LFX_ALL_UPPER;
			}
			if (args.at(0) == "bottom") {
				zoneCode = LFX_ALL_LOWER;
			}
			if (args.at(0) == "front") {
				zoneCode = LFX_ALL_FRONT;
			}
			if (args.at(0) == "rear") {
				zoneCode = LFX_ALL_REAR;
			}
			static ColorU color;
			color.cs.red = atoi(args.at(1).c_str());
			color.cs.green = atoi(args.at(2).c_str());
			color.cs.blue = atoi(args.at(3).c_str());
			color.cs.brightness = args.size() > 4 ? atoi(args.at(4).c_str()) : 255;
			lfxUtil.SetLFXColor(zoneCode, color.ci);
			lfxUtil.Update();
			continue;
		}
		if (command == "set-action") {
			//cerr << "Executing " << command << " " << values << endl;
			if (args.size() < 6) {
				cerr << "set-action: Incorrect arguments" << endl;
				continue;
			}
			unsigned actionCode = LFX_ACTION_COLOR;
			if (args.at(0) == "pulse")
				actionCode = LFX_ACTION_PULSE;
			else if (args.at(0) == "morph")
				actionCode = LFX_ACTION_MORPH;
			static ColorU color, color2;
			color.cs.red = atoi(args.at(5).c_str());
			color.cs.green = atoi(args.at(4).c_str());
			color.cs.blue = atoi(args.at(3).c_str());
			color.cs.brightness = args.size() > 6 ? atoi(args.at(6).c_str()) : 255;
			if (args.size() > 7) {
				color2.cs.red = atoi(args.at(9).c_str());
				color2.cs.green = atoi(args.at(8).c_str());
				color2.cs.blue = atoi(args.at(7).c_str());
				color2.cs.brightness = args.size() > 10 ? atoi(args.at(10).c_str()) : 255;
			}
			lfxUtil.SetLFXAction(actionCode, atoi(args.at(1).c_str()), atoi(args.at(2).c_str()), &color.ci, &color2.ci);
			lfxUtil.Update(); 
			continue;
		}
		if (command == "set-zone-action") {
			//cerr << "Executing " << command << " " << values << endl;
			if (args.size() < 5) {
				cerr << "set-zone-action: Incorrect arguments" << endl;
				continue;
			}
			unsigned actionCode = LFX_ACTION_COLOR;
			if (args.at(0) == "pulse")
				actionCode = LFX_ACTION_PULSE;
			else if (args.at(0) == "morph")
				actionCode = LFX_ACTION_MORPH;
			static ColorU color, color2;
			unsigned zoneCode = LFX_ALL;
			if (args.at(1) == "left") {
				zoneCode = LFX_ALL_LEFT;
			}
			if (args.at(1) == "right") {
				zoneCode = LFX_ALL_RIGHT;
			}
			if (args.at(1) == "top") {
				zoneCode = LFX_ALL_UPPER;
			}
			if (args.at(1) == "bottom") {
				zoneCode = LFX_ALL_LOWER;
			}
			if (args.at(1) == "front") {
				zoneCode = LFX_ALL_FRONT;
			}
			if (args.at(1) == "rear") {
				zoneCode = LFX_ALL_REAR;
			}
			color.cs.red = atoi(args.at(2).c_str());
			color.cs.green = atoi(args.at(3).c_str());
			color.cs.blue = atoi(args.at(4).c_str());
			color.cs.brightness = args.size() > 5 ? atoi(args.at(5).c_str()) : 255;
			if (args.size() > 8) {
				color2.cs.red = atoi(args.at(6).c_str());
				color2.cs.green = atoi(args.at(7).c_str());
				color2.cs.blue = atoi(args.at(8).c_str());
				color2.cs.brightness = args.size() > 9 ? atoi(args.at(9).c_str()) : 255;
			}
			lfxUtil.SetLFXZoneAction(actionCode, zoneCode, color.ci, color2.ci);
			lfxUtil.Update();
			continue;
		}
		cerr << "Unknown command: " << command << endl;
	}
	lfxUtil.Release();

    return 1;
}

