#include "FXHelper.h"

void FXHelper::Refresh(int * freq)
{
	for (unsigned i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		if (!map.map.empty()) {
			double power = 0.0;
			Colorcode from, to, fin;
			from.ci = map.colorfrom.ci; to.ci = map.colorto.ci;
			// here need to check less bars...
			for (int j = 0; j < map.map.size(); j++)
				power += (freq[map.map[j]] > map.lowcut ? freq[map.map[j]] < map.hicut ? freq[map.map[j]] - map.lowcut : map.hicut - map.lowcut : 0);
			if (map.map.size() > 1)
				power = power / (map.map.size() * (map.hicut - map.lowcut));
			fin.cs.red = (unsigned char)((1.0 - power) * from.cs.red + power * to.cs.red);
			fin.cs.green = (unsigned char)((1.0 - power) * from.cs.green + power * to.cs.green);
			fin.cs.blue = (unsigned char)((1.0 - power) * from.cs.blue + power * to.cs.blue);
			//Don't need gamma!
			//fin.cs.red = (fin.cs.red * fin.cs.red) >> 8;
			//fin.cs.green = (fin.cs.green * fin.cs.green) >> 8;
			//fin.cs.blue = (fin.cs.blue * fin.cs.blue) >> 8;
			if (map.lightid > 0xffff) {
				// group
				AlienFX_SDK::group* grp = afx_dev.GetGroupById(map.lightid);
				if (grp) {
					vector<UCHAR> lIDs; vector<AlienFX_SDK::afx_act> lSets;
					vector<vector<AlienFX_SDK::afx_act>> fullSets;
					AlienFX_SDK::afx_act l_from = {0,0,0,from.cs.red,from.cs.green, from.cs.blue},
						l_to = {0,0,0,to.cs.red,to.cs.green, to.cs.blue},
						l_fin = {0,0,0,fin.cs.red,fin.cs.green, fin.cs.blue};
					lSets.push_back(l_fin);
					for (int i = 0; i < grp->lights.size(); i++) {
						if (grp->lights[i]->devid == grp->lights.front()->devid) {
							if (map.flags) {
								// gauge
								lSets.clear();
								if (((double) i) / grp->lights.size() < power) {
									if (((double) i + 1) / grp->lights.size() < power)
										//dev->SetColor(grp->lights[i]->lightid, to.cs.red, to.cs.green, to.cs.blue);
										lSets.push_back(l_to);
									else {
										// recalc...
										double newPower = (power - ((double) i) / grp->lights.size()) * grp->lights.size();
										l_fin.r = (unsigned char) ((1.0 - newPower) * from.cs.red + newPower * to.cs.red);
										l_fin.g = (unsigned char) ((1.0 - newPower) * from.cs.green + newPower * to.cs.green);
										l_fin.b = (unsigned char) ((1.0 - newPower) * from.cs.blue + newPower * to.cs.blue);
										//dev->SetColor(grp->lights[i]->lightid, fin.cs.red, fin.cs.green, fin.cs.blue);
										lSets.push_back(l_fin);
									}
								} else
									//dev->SetColor(grp->lights[i]->lightid, from.cs.red, from.cs.green, from.cs.blue);
									lSets.push_back(l_from);
								fullSets.push_back(lSets);
							} //else {
								//dev->SetColor(grp->lights[i]->lightid, fin.cs.red, fin.cs.green, fin.cs.blue);
							//}
							lIDs.push_back((UCHAR)grp->lights[i]->lightid);
						}
					}
					if (grp->lights.size()) {
						AlienFX_SDK::Functions* dev = LocateDev(grp->lights.front()->devid);
						if (dev && dev->IsDeviceReady())
							if (map.flags)
								dev->SetMultiColor((int) lIDs.size(), lIDs.data(), fullSets);
							else
								dev->SetMultiLights((int) lIDs.size(), lIDs.data(), l_fin.r, l_fin.r, l_fin.b);
					}
				}
			} else {
				AlienFX_SDK::Functions* dev = LocateDev(map.devid);
				if (dev && dev->IsDeviceReady())
					dev->SetColor(map.lightid, fin.cs.red, fin.cs.green, fin.cs.blue);
			}
		}
	}
	UpdateColors();
}

void FXHelper::FadeToBlack()
{
	for (int i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		AlienFX_SDK::Functions* dev = LocateDev(map.devid);
		if (dev && afx_dev.GetFlags(map.devid, map.lightid) == 0 && dev->IsDeviceReady()) {
			dev->SetColor(map.lightid, 0, 0, 0);
		}
	}
	UpdateColors();
}
