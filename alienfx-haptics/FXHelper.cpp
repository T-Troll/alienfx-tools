#include "FXHelper.h"

struct devset {
	WORD did;
	vector<UCHAR> lIDs;
	vector<vector<AlienFX_SDK::afx_act>> fullSets;
};

void FXHelper::Refresh(int * freq)
{
	for (unsigned i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		if (!map.map.empty()) {
			double power = 0.0;
			AlienFX_SDK::afx_act from = {0,0,0,map.colorfrom.cs.red,map.colorfrom.cs.green, map.colorfrom.cs.blue},
				to = {0,0,0,map.colorto.cs.red,map.colorto.cs.green, map.colorto.cs.blue},
				fin = {0};
			// here need to check less bars...
			for (int j = 0; j < map.map.size(); j++)
				power += (freq[map.map[j]] > map.lowcut ? freq[map.map[j]] < map.hicut ? freq[map.map[j]] - map.lowcut : map.hicut - map.lowcut : 0);
			if (map.map.size() > 1)
				power = power / (map.map.size() * (map.hicut - map.lowcut));
			fin.r = (unsigned char)((1.0 - power) * from.r + power * to.r);
			fin.g = (unsigned char)((1.0 - power) * from.g + power * to.g);
			fin.b = (unsigned char)((1.0 - power) * from.b + power * to.b);
			//Don't need gamma?
			fin.r = (fin.r * fin.r) >> 8;
			fin.g = (fin.g * fin.g) >> 8;
			fin.b = (fin.b * fin.b) >> 8;
			if (map.lightid > 0xffff) {
				// group
				AlienFX_SDK::group* grp = afx_dev.GetGroupById(map.lightid);
				if (grp) {
					vector<AlienFX_SDK::afx_act> lSets;
					vector<devset> devsets;
					lSets.push_back(fin);
					for (int i = 0; i < grp->lights.size(); i++) {
						int dind;
						for (dind = 0; dind < devsets.size(); dind++)
							if (grp->lights[i]->devid == devsets[dind].did)
								break;
						if (dind == devsets.size()) {
							// need new set...
							devset nset = {(WORD)grp->lights[i]->devid};
							devsets.push_back(nset);
						}
						if (map.flags) {
							// gauge
							lSets.clear();
							if (((double) i) / grp->lights.size() < power) {
								if (((double) i + 1) / grp->lights.size() < power)
									lSets.push_back(to);
								else {
									// recalc...
									double newPower = (power - ((double) i) / grp->lights.size()) * grp->lights.size();
									fin.r = (unsigned char) ((1.0 - newPower) * from.r + newPower * to.r);
									fin.g = (unsigned char) ((1.0 - newPower) * from.g + newPower * to.g);
									fin.b = (unsigned char) ((1.0 - newPower) * from.b + newPower * to.b);
									lSets.push_back(fin);
								}
							} else
								lSets.push_back(from);
							devsets[dind].fullSets.push_back(lSets);
						}
						devsets[dind].lIDs.push_back((UCHAR)grp->lights[i]->lightid);
					}
					if (grp->lights.size()) {
						for (int dind = 0; dind < devsets.size(); dind++) {
							AlienFX_SDK::Functions* dev = LocateDev(devsets[dind].did);
							if (dev && dev->IsDeviceReady())
								if (map.flags)
									dev->SetMultiColor((int) devsets[dind].lIDs.size(), devsets[dind].lIDs.data(), devsets[dind].fullSets);
								else
									dev->SetMultiLights((int) devsets[dind].lIDs.size(), devsets[dind].lIDs.data(), fin.r, fin.r, fin.b);
						}
					}
				}
			} else {
				AlienFX_SDK::Functions* dev = LocateDev(map.devid);
				if (dev && dev->IsDeviceReady())
					dev->SetColor(map.lightid, fin.r, fin.g, fin.b);
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
