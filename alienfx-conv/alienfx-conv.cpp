// alienfx-conv.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include "ConfigHandler.h"

int main()
{
	printf("AlienFX-Tool configuration converter v1.0.0.0\nLoading old configuration... ");
	ConfigHandler conf;
	conf.Load();
	printf("Done, %zd profiles, %zd ambient, %zd haptics.\n", conf.profiles.size(), conf.amb_conf->zones.size(), conf.hap_conf->haptics.size());
	for (auto p = conf.profiles.begin(); p < conf.profiles.end(); p++) {
		printf("%s, ID%d, %zd fans, %zd mappings (", (*p)->name.c_str(), (*p)->id, (*p)->fansets.fanControls.size(), (*p)->lightsets.size());
		int colorsize = 0, effsize = 0, ambsize = 0, hapsize = 0;
		for (auto map = (*p)->lightsets.begin(); map < (*p)->lightsets.end(); map++) {
			colorsize += map->color.size();
			effsize += map->events[0].state + map->events[1].state + map->events[2].state;
			ambsize += map->ambients.size();
			hapsize += map->haptics.size();
			//printf("\tGroup ID%d - %zd colors, %d effects, %zd ambient, %zd haptics, name %s\n", map->group->gid, map->color.size(),
			//	map->events[0].state + map->events[1].state + map->events[2].state,map->ambients.size(), map->haptics.size(),
			//	map->group->name.c_str());
		}
		printf("%d colors, %d effects, %d ambient, %d haptics)\n", colorsize, effsize, ambsize, hapsize );
	}
	char name[256]{ 0 };
	printf("Do you want to clear light grids (it's needed if you set it from alienfx-pos)? ");
	gets_s(name, 255);
	if (name[0] == 'y' || name[0] == 'Y') {
		conf.afx_dev.GetGrids()->clear();
		printf("Grids cleared.\n");
	}
	printf("Do you want to clear old light settings (from version 5 tools)? ");
	gets_s(name, 255);
	if (name[0] == 'y' || name[0] == 'Y') {
		conf.ClearEvents();
		printf("Events cleared!\n");
	}
	printf("Saving new configuration...\n");
	conf.afx_dev.SaveMappings();
	conf.Save();
	printf("Done, press Enter to finish!\n");
	gets_s(name, 255);
}

