#include "FXHelper.h"

struct devset {
	WORD did;
	vector<UCHAR> lIDs;
};

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	unsigned shift = 255 - config->shift;
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
			fin.r = r / size;
			fin.g = g / size;
			fin.b = b / size;

			// Gamma correction...
			if (config->gammaCorrection) {
				fin.r = ((int)fin.r * fin.r) >> 8;
				fin.g = ((int)fin.g * fin.g) >> 8;
				fin.b = ((int)fin.b * fin.b) >> 8;
			}
			if (map.lightid > 0xffff) {
				// group
				AlienFX_SDK::group* grp = afx_dev.GetGroupById(map.lightid);
				if (grp) {
					vector<devset> devsets;
					for (int i = 0; i < grp->lights.size(); i++) {
						// do we have devset?
						int dind;
						for (dind = 0; dind < devsets.size(); dind++)
							if (grp->lights[i]->devid == devsets[dind].did)
								break;
						if (dind == devsets.size()) {
							// need new set...
							devset nset = {(WORD)grp->lights[i]->devid};
							devsets.push_back(nset);
						}
						devsets[dind].lIDs.push_back((UCHAR)grp->lights[i]->lightid);
					}
					if (grp->lights.size()) {
						for (int dind = 0; dind < devsets.size(); dind++) {
							AlienFX_SDK::Functions* dev = LocateDev(devsets[dind].did);
							if (dev && dev->IsDeviceReady())
								if (dev->GetVersion() < 4) {
									fin.r = (r * shift) >> 8;
									fin.g = (g * shift) >> 8;
									fin.b = (b * shift) >> 8;
								}
							dev->SetMultiLights((int) devsets[dind].lIDs.size(), devsets[dind].lIDs.data(), fin.r, fin.g, fin.b);
						}
					}
				}
			} else {
				AlienFX_SDK::Functions* dev = LocateDev(map.devid);
				if (dev && dev->IsDeviceReady()) {
					if (dev->GetVersion() < 4) {
						fin.r = (r * shift) >> 8;
						fin.g = (g * shift) >> 8;
						fin.b = (b * shift) >> 8;
					}
					dev->SetColor(map.lightid, fin.r, fin.g, fin.b);
				}
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

void FXHelper::ChangeState() {
	byte bright = (byte) (255 - config->shift);
	for (int i = 0; i < devs.size(); i++) {
		devs[i]->ToggleState(bright, afx_dev.GetMappings(), false);
	}
}

