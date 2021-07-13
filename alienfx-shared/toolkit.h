#pragma once
#include <wtypes.h>
#include "AlienFX_SDK.h"
#include <CommCtrl.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")

//namespace AlienFX_TOOLS
//{
    template<typename FX>
	static int UpdateLightList(HWND light_list, FX* fxhl, int flag = 0) {
		int pos = -1;
		size_t lights = fxhl->afx_dev.GetMappings()->size();
		size_t groups = fxhl->afx_dev.GetGroups()->size();
		SendMessage(light_list, LB_RESETCONTENT, 0, 0);
		for (int i = 0; i < groups; i++) {
			AlienFX_SDK::group grp = fxhl->afx_dev.GetGroups()->at(i);
			string fname = grp.name + " (Group)";
			pos = (int) SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM) (fname.c_str()));
			SendMessage(light_list, LB_SETITEMDATA, pos, grp.gid);
		}
		for (int i = 0; i < lights; i++) {
			AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(i);
			if (fxhl->LocateDev(lgh.devid) && !(lgh.flags & flag)) {
				pos = (int) SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM) (lgh.name.c_str()));
				SendMessage(light_list, LB_SETITEMDATA, pos, i);
			}
		}
		RedrawWindow(light_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
		return pos;
	}

	extern HINSTANCE hInst;
	HWND CreateToolTip(HWND hwndParent);
	void SetSlider(HWND tt, char* buff, int value);
	void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b);

	template<class CONF>
	class FXH {
	protected:
		CONF* config;
	public:
		FXH() {};
		FXH(CONF* conf) {
			config = conf;
			afx_dev.LoadMappings();
			FillDevs(config->stateOn, config->offPowerButton);
		};
		~FXH() {
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
		size_t FillDevs(bool state, bool power) {
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
					dev->ToggleState(state, afx_dev.GetMappings(), power);
				}
			}
			return devs.size();
		};
		void UpdateColors(int did = -1)
		{
			if (config->stateOn) {
				for (int i = 0; i < devs.size(); i++)
					if (did == -1 || did == devs[i]->GetPID())
						devs[i]->UpdateColors();
			}
		}
	};

//}