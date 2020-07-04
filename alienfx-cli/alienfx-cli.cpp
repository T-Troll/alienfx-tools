#include <iostream>
#include <windows.h>

#include "stdafx.h"
#include "LFXUtil.h"

namespace 
{
	LFXUtil::LFXUtilC lfxUtil;
}

void printUsage() 
{
	std::cerr << "Usage: alienfx-cli [command] [command options]" << std::endl << "Commands: " << std::endl 
		<< "\tset-color-all r g b [br]\t\t- set all device colors to provided" << std::endl
		<< "\tset-color dev-id light-id r g b [br]\t- set one light to color provided." << std::endl 
		<< "\tstatus\t\t\t\t\t- showing devices and lights id and status" << std::endl;
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
	if (std::string(command) == "set-color-all")
	{
		unsigned char br = 0xff;
		if (argc < 5) 
		{
			printUsage();
			return 1;
		}

		if (argc == 6) br = atoi(argv[5]);

		lfxUtil.SetLFXColor(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), br);
		return 0;
	} 

	if (std::string(command) == "set-color")
	{
		unsigned char br = 0xff;
		if (argc < 7)
		{
			printUsage();
			return 1;
		}

		if (argc == 8) br = atoi(argv[7]);

		lfxUtil.SetOneLFXColor(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), br);
		lfxUtil.GetStatus();
		return 0;
	}
	if (std::string(command) == "status")
	{
		lfxUtil.GetStatus();
		return 0;

	}

	std::cerr << "Unknown command " << command << std::endl;
	printUsage();

    return 1;
}

