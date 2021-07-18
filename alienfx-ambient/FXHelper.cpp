#include "FXHelper.h"

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	unsigned shift = 256 - config->shift;
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		AlienFX_SDK::afx_act fin;
		unsigned r = 0, g = 0, b = 0, size = (unsigned) map.map.size();
		if (size > 0) {
			for (unsigned j = 0; j < size; j++) {
				r += img[3 * map.map[j] + 2];
				g += img[3 * map.map[j] + 1];
				b += img[3 * map.map[j]];
			}

			// Brightness correction...
			fin.r = (r * shift) / (256 * size);
			fin.g = (g * shift) / (256 * size);
			fin.b = (b *shift) / (256 * size);

			// Gamma correction...
			if (config->gammaCorrection) {
				fin.r = ((int)fin.r * fin.r) >> 8;
				fin.g = ((int)fin.g * fin.g) >> 8;
				fin.b = ((int)fin.g * fin.b) >> 8;
			}
			if (map.lightid > 0xffff) {
				// group
				AlienFX_SDK::group* grp = afx_dev.GetGroupById(map.lightid);
				if (grp) {
					vector<UCHAR> lIDs; vector<AlienFX_SDK::afx_act> lSets;
					vector<vector<AlienFX_SDK::afx_act>> fullSets;
					lSets.push_back(fin);
					for (int i = 0; i < grp->lights.size(); i++) {
						if (grp->lights[i]->devid == grp->lights.front()->devid) {
							lIDs.push_back((UCHAR)grp->lights[i]->lightid);
							fullSets.push_back(lSets);
						}
					}
					if (grp->lights.size()) {
						AlienFX_SDK::Functions* dev = LocateDev(grp->lights.front()->devid);
						if (dev && dev->IsDeviceReady())
							dev->SetMultiColor((int)lIDs.size(), lIDs.data(), fullSets);
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
	return 0;
}

void FXHelper::FadeToBlack()
{
	for (int i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		AlienFX_SDK::Functions* dev = LocateDev(map.devid);
		if (dev && !afx_dev.GetFlags(map.devid, map.lightid) && dev->IsDeviceReady()) {
			dev->SetColor(map.lightid, 0, 0, 0);
		}
	}
	UpdateColors();
}

