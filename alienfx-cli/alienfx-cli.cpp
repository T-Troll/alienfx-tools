#include <iostream>
#include <windows.h>

#include "stdafx.h"
#include "LFXUtil.h"
#include "LFXDecl.h"

namespace 
{
	LFXUtil::LFXUtilC lfxUtil;
}

void printUsage() 
{
	std::cerr << "Usage: alienfx-cli [command] [command options]" << std::endl
		<< "Commands: " << std::endl
		<< "\tset-all r g b [br]\t\t\t\t- set all device colors to provided" << std::endl
		<< "\tset-dev dev light r g b [br]\t\t\t- set one light to color provided." << std::endl
		<< "\tset-zone zone r g b [br]\t\t\t- set zone light to color provided." << std::endl
		<< "\tset-action action dev light r g b [br r g b br]\t- set light to color provided and enable action." << std::endl
		<< "\tset-zone-action action zone r g b [br r g b br]\t- set zone light to color provided and enable action." << std::endl
		<< "\tstatus\t\t\t\t\t\t- shows devices and lights id's, name and status" << std::endl << std::endl
		<< "Zones: left, right, top, bottom, front, rear" << std::endl
		<< "Actions: pulse, morph (you need 2 colors for morth), color (disable action)" << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc < 2) 
	{
		printUsage();
		return 1;
	}

	const ResultT& result(lfxUtil.InitLFX());
	if (!result.first)
		return 1;

	const char* command = argv[1];
	if (std::string(command) == "set-all")
	{
		unsigned char br = 0xff;
		if (argc < 5) 
		{
			printUsage();
			return 1;
		}

		if (argc == 6) br = atoi(argv[5]);

		unsigned zoneCode = LFX_ALL;

		lfxUtil.SetLFXColor(zoneCode, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), br);
		//lfxUtil.GetStatus();
		lfxUtil.Release();
		return 0;
	} 

	if (std::string(command) == "set-dev")
	{
		unsigned char br = 0xff;
		if (argc < 7)
		{
			printUsage();
			return 1;
		}

		if (argc == 8) br = atoi(argv[7]);

		lfxUtil.SetOneLFXColor(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), br);
		//lfxUtil.GetStatus();
		lfxUtil.Release();
		return 0;
	}

	if (std::string(command) == "set-action")
	{
		unsigned char br = 0xff;
		if (argc < 8)
		{
			printUsage();
			return 1;
		}

		const char* action = argv[2];
		unsigned actionCode = LFX_ACTION_COLOR;

		if (std::string(action) == "pulse") {
			actionCode = LFX_ACTION_PULSE;
		}
		if (std::string(action) == "morph") {
			actionCode = LFX_ACTION_MORPH;
		}
		if (std::string(action) == "color") {
			actionCode = LFX_ACTION_COLOR;
		}

		if (argc == 9) br = atoi(argv[8]);

		if (argc > 9)
			lfxUtil.SetLFXAction(actionCode, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10]), atoi(argv[11]), atoi(argv[12]));
		else
			lfxUtil.SetLFXAction(actionCode, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), br, 0, 0, 0, 0);
		//lfxUtil.GetStatus();
		lfxUtil.Release();
		return 0;
	}

	if (std::string(command) == "set-zone")
	{
		unsigned char br = 0xff;
		if (argc < 6)
		{
			printUsage();
			return 1;
		}

		if (argc == 7) br = atoi(argv[6]);

		const char* zone = argv[2];
		unsigned zoneCode = LFX_ALL;

		if (std::string(zone) == "left") {
			zoneCode = LFX_ALL_LEFT;
		}
		if (std::string(zone) == "right") {
			zoneCode = LFX_ALL_RIGHT;
		}
		if (std::string(zone) == "top") {
			zoneCode = LFX_ALL_UPPER;
		}
		if (std::string(zone) == "bottom") {
			zoneCode = LFX_ALL_LOWER;
		}
		if (std::string(zone) == "front") {
			zoneCode = LFX_ALL_FRONT;
		}
		if (std::string(zone) == "rear") {
			zoneCode = LFX_ALL_REAR;
		}

		lfxUtil.SetLFXColor(zoneCode, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), br);
		//lfxUtil.GetStatus();
		lfxUtil.Release();
		return 0;
	}

	if (std::string(command) == "set-zone-action")
	{
		unsigned char br = 0xff;
		if (argc < 7)
		{
			printUsage();
			return 1;
		}

		const char* action = argv[2];
		unsigned actionCode = LFX_ACTION_COLOR;

		if (std::string(action) == "pulse") {
			actionCode = LFX_ACTION_PULSE;
		}
		if (std::string(action) == "morph") {
			actionCode = LFX_ACTION_MORPH;
		}
		if (std::string(action) == "color") {
			actionCode = LFX_ACTION_COLOR;
		}

		const char* zone = argv[3];
		unsigned zoneCode = LFX_ALL;

		if (std::string(zone) == "left") {
			zoneCode = LFX_ALL_LEFT;
		}
		if (std::string(zone) == "right") {
			zoneCode = LFX_ALL_RIGHT;
		}
		if (std::string(zone) == "top") {
			zoneCode = LFX_ALL_UPPER;
		}
		if (std::string(zone) == "bottom") {
			zoneCode = LFX_ALL_LOWER;
		}
		if (std::string(zone) == "front") {
			zoneCode = LFX_ALL_FRONT;
		}
		if (std::string(zone) == "rear") {
			zoneCode = LFX_ALL_REAR;
		}

		if (argc == 8) br = atoi(argv[7]);

		if (argc > 8)
			lfxUtil.SetLFXZoneAction(actionCode, zoneCode, atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10]), atoi(argv[11]));
		else
			lfxUtil.SetLFXZoneAction(actionCode, zoneCode, atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), br, 0, 0, 0, 0);
		//lfxUtil.GetStatus();
		lfxUtil.Release();
		return 0;
	}

	if (std::string(command) == "status")
	{
		lfxUtil.GetStatus();
		lfxUtil.Release();

		return 0;

	}

	std::cerr << "Unknown command " << command << std::endl;
	printUsage();

    return 1;
}

