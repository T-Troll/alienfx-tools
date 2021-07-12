#pragma once
#include <wtypes.h>
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

//namespace AlienFX_TOOLS
//{
    template<typename FX>
	static int UpdateLightList(HWND light_list, FX* fxhl) {
		int pos = -1;
		size_t lights = fxhl->afx_dev.GetMappings()->size();
		SendMessage(light_list, LB_RESETCONTENT, 0, 0);
		for (int i = 0; i < lights; i++) {
			AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(i);
			if (fxhl->LocateDev(lgh.devid)) {
				pos = (int) SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM) (lgh.name.c_str()));
				SendMessage(light_list, LB_SETITEMDATA, pos, i);
			}
		}
		RedrawWindow(light_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
		return pos;
	}

	template<typename CONF>
	class FXH {
	protected:
		CONF* config;
	public:
		FXH(CONF* conf) {
			config = conf;
			afx_dev.LoadMappings();
			FillDevs();
		};
		~FXH() {
			DeInit();
			if (devs.size() > 0) {
				for (int i = 0; i < devs.size(); i++)
					devs[i]->AlienFXClose();
				devs.clear();
			}
		};
		AlienFX_SDK::Mappings afx_dev;
		std::vector<AlienFX_SDK::Functions*> devs;
		AlienFX_SDK::Functions* LocateDev(int pid) {
			for (int i = 0; i < devs.size(); i++)
				if (devs[i]->GetPID() == pid)
					return devs[i];
			return nullptr;
		};
		size_t FillDevs() {
			vector<pair<DWORD, DWORD>> devList = afx_dev.AlienFXEnumDevices();
			if (devs.size() > 0) {
				for (int i = 0; i < devs.size(); i++)
					devs[i]->AlienFXClose();
				devs.clear();
			}
			for (int i = 0; i < devList.size(); i++) {
				AlienFX_SDK::Functions* dev = new AlienFX_SDK::Functions();
				int pid = dev->AlienFXInitialize(devList[i].first, devList[i].second);
				if (pid != -1) {
					devs.push_back(dev);
					dev->ToggleState(true, afx_dev.GetMappings(), false);
				}
			}
		};
		virtual void DeInit() {};
	};

//}