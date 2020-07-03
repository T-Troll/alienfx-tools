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
		<< "  set-color-all r g b - set all device colors to provided" << std::endl
		<< "  status - showing devices and lights status" << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc < 2) 
	{
		printUsage();
		return 1;
	}

	const char* command = argv[1];
	if (std::string(command) == "set-color-all")
	{
		if (argc < 5) 
		{
			printUsage();
			return 1;
		}

		lfxUtil.SetLFXColor(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
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

