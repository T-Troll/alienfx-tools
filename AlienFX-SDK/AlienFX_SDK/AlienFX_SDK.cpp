//#define WIN32_LEAN_AND_MEAN
#include "AlienFX_SDK.h"
#include "alienfx-controls.h"
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}
#include <SetupAPI.h>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib, "hid.lib")

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) printf("%s",string(_x_).c_str());
//#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
//#endif
#else
#define DebugPrint(_x_)
#endif

namespace AlienFX_SDK {

	vector<Afx_icommand> *Functions::SetMaskAndColor(DWORD index, byte type, Afx_action c1, Afx_action c2, byte tempo) {
		vector<Afx_icommand>* mods;
		if (version == API_V6)
			mods = new vector<Afx_icommand>({ {9,(byte)index}, { 10,c1.r }, { 11,c1.g }, { 12,c1.b } });
		else
			mods = new vector<Afx_icommand>({ {1, type}, { 2,(byte)chain },
			{ 3,(byte)((index & 0xFF0000) >> 16) }, { 4,(byte)((index & 0x00FF00) >> 8) }, { 5,(byte)(index & 0x0000FF) }});
		switch (version) {
		case API_V3:
			mods->insert(mods->end(),{{6,c1.r},{7,c1.g},{8,c1.b},{9,c2.r},{10,c2.g},{11,c2.b}});
			break;
		case API_V2:
			mods->insert(mods->end(),{{6,(byte)((c1.r & 0xf0) | ((c1.g & 0xf0) >> 4))},
						{7,(byte)((c1.b & 0xf0) | ((c2.r & 0xf0) >> 4))},
						{8,(byte)((c2.g & 0xf0) | ((c2.b & 0xf0) >> 4))}});
			break;
		case API_V6: {
			byte mask = (byte)(c1.r ^ c1.g ^ c1.b ^ index);
			switch (type) {
			case AlienFX_A_Color:
				mask ^= 8;
				mods->insert(mods->end(), { {13, bright}, {14,mask} });
				break;
			case AlienFX_A_Pulse:
				mask ^= byte(tempo ^ 1);
				mods->insert(mods->end(), { {3, 0xb}, {6, 0x88}, {8, 2}, {13, bright}, {14, tempo}, {15,mask} });
				break;
			case AlienFX_A_Morph: case AlienFX_A_Breathing:
				if (type == AlienFX_A_Breathing)
					c2 = { 0 };
				mask ^= (byte)(c2.r ^ c2.g ^c2.b ^ tempo ^ 4);
				mods->insert(mods->end(), { {3, 0xf}, {6, 0x8c}, {8, 1},
					{13,c2.r},{14,c2.g},{15,c2.b},
					{16, bright}, {17, 2}, {18, tempo}, {19,mask} });
				break;
			}
		} break;
		}
		return mods;
	}

	inline bool Functions::PrepareAndSend(const byte* command, vector<Afx_icommand> mods) {
		return PrepareAndSend(command, &mods);
	}

	bool Functions::PrepareAndSend(const byte *command, vector<Afx_icommand> *mods) {
		byte buffer[MAX_BUFFERSIZE]{ reportIDList[version] };
		DWORD written , size = command[0];
		BOOL res = false;

		if (version == API_V6 /*&& size != 3*/)
			FillMemory(buffer+1, MAX_BUFFERSIZE-1, 0xff);

		memcpy(buffer+1, command+1, size);

		if (mods) {
			for (auto i = mods->begin(); i < mods->end(); i++)
				buffer[i->i] = i->val;
			mods->clear();
		}

		switch (version) {
		case API_V2: case API_V3:
			res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, 0, 0, &written, NULL);
			//return HidD_SetOutputReport(devHandle, buffer, length);
			break;
		case API_V4:
			res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, 0, 0, &written, NULL);
			res &= DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL);
			break;
		case API_V5:
			res =  DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, 0, 0, &written, NULL);
			//return HidD_SetFeature(devHandle, buffer, length);
			break;
		case API_V6: {
			res = WriteFile(devHandle, buffer, length, &written, NULL);
			//if (size == 3)
			//	res &= ReadFile(devHandle, buffer, length, &written, NULL);
			break;
		}
		case API_V7:
			WriteFile(devHandle, buffer, length, &written, NULL);
			res =  ReadFile(devHandle, buffer, length, &written, NULL);
			break;
		case API_V8: {
			if (size == 4) {
				//res = HidD_SetFeature(devHandle, buffer, length);
				res = DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, 0, 0, &written, NULL);
				Sleep(7); // Need wait for ACK
			}
			else {
				res = WriteFile(devHandle, buffer, length, &written, NULL);
			}
		}
		}
#ifdef _DEBUG
		if (!res)
			DebugPrint("Device IO fail!\n");
#endif
		return res;
	}

	bool Functions::SavePowerBlock(byte blID, Afx_lightblock* act, bool needSave, bool needInverse) {
		byte mode;
		vector<Afx_icommand> separator = { {2, blID} };
		switch (act->act.front().type) {
		case AlienFX_A_Pulse:
			mode = 2; break;
		case AlienFX_A_Morph:
			mode = 1; break;
		default: mode = 3;
		}
		PrepareAndSend(COMMV1_saveGroup, &separator);
		if (act->act.size() < 2)
			PrepareAndSend(COMMV1_color, SetMaskAndColor(1 << act->index, mode,
				{act->act.front().b, act->act.front().g, act->act.front().r}));
		else
			PrepareAndSend(COMMV1_color, SetMaskAndColor(1 << act->index, mode,
				{act->act.front().b, act->act.front().g, act->act.front().r},
				{act->act.back().b, act->act.back().g, act->act.back().r}));
		PrepareAndSend(COMMV1_saveGroup, &separator);
		PrepareAndSend(COMMV1_loop);
		chain++;

		if (needInverse) {
			PrepareAndSend(COMMV1_saveGroup, &separator);
			PrepareAndSend(COMMV1_color, SetMaskAndColor(~((1 << act->index)), act->act.back().type,
				{act->act.front().b, act->act.front().g, act->act.front().r},
				{act->act.back().b, act->act.back().g, act->act.back().r}));
			PrepareAndSend(COMMV1_saveGroup, &separator);
			PrepareAndSend(COMMV1_loop);
			chain++;
		}

		if (needSave) {
			PrepareAndSend(COMMV1_save);
			Reset();
		}

		return true;
	}

	bool Functions::ProbeDevice(void* hDevInfo, void* devData, int vidd, int pidd) {
		DWORD dwRequiredSize = 0;
		version = API_UNKNOWN;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, (PSP_DEVICE_INTERFACE_DATA)devData, NULL, 0, &dwRequiredSize, NULL);
		SP_DEVICE_INTERFACE_DETAIL_DATA* deviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)new byte[dwRequiredSize];
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		if (SetupDiGetDeviceInterfaceDetail(hDevInfo, (PSP_DEVICE_INTERFACE_DATA)devData, deviceInterfaceDetailData, dwRequiredSize, NULL, NULL) &&
			(devHandle = CreateFile(deviceInterfaceDetailData->DevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_ANONYMOUS/*FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH*/, NULL)) != INVALID_HANDLE_VALUE) {
			HIDD_ATTRIBUTES attributes{ sizeof(HIDD_ATTRIBUTES) };
			PHIDP_PREPARSED_DATA prep_caps;
			HIDP_CAPS caps;
			if (HidD_GetAttributes(devHandle, &attributes) &&
				(vidd == -1 || attributes.VendorID == vidd) && (pidd == -1 || attributes.ProductID == pidd) &&
				HidD_GetPreparsedData(devHandle, &prep_caps) && HidP_GetCaps(prep_caps, &caps) == HIDP_STATUS_SUCCESS) {
				HidD_FreePreparsedData(prep_caps);
				length = caps.OutputReportByteLength;
				switch (vid = attributes.VendorID) {
				case 0x0d62: // Darfon
					if (caps.Usage == 0xcc && !length) {
						length = caps.FeatureReportByteLength;
						version = API_V5;
					}
					break;
				case 0x187c: // Alienware
					switch (length) {
					case 9:
						version = API_V2;
						break;
					case 12:
						version = API_V3;
						break;
					case 34:
						version = API_V4;
						break;
					case 65:
						version = API_V6;
						//PrepareAndSend(COMMV6_systemReset);
						break;
					}
					break;
				default:
					if (length == 65)
						switch (vid) {
						case 0x0424: // Microchip
							version = API_V6;
							//PrepareAndSend(COMMV6_systemReset);
							break;
						case 0x0461: // Primax
							version = API_V7;
							break;
						case 0x04f2:  // Chicony
							version = API_V8;
							break;
						}
				}
			}
			if (version == API_UNKNOWN) {
				CloseHandle(devHandle); devHandle = NULL;
				pid = -1;
			}
			else
				pid = attributes.ProductID;
			DebugPrint("Probe done, type " + to_string(version) + "\n");
		}
		delete[] deviceInterfaceDetailData;
		return pid >= 0;
	}

	//Use this method for general devices, vid = -1 for any vid, pid = -1 for any pid.
	int Functions::AlienFXInitialize(int vidd, int pidd) {
		GUID guid;
		DWORD dwRequiredSize = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo != INVALID_HANDLE_VALUE) {
			for (DWORD dw = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData) &&
				(deviceInterfaceData.Flags & SPINT_ACTIVE) &&
				!ProbeDevice(hDevInfo, &deviceInterfaceData, vidd, pidd); dw++) ;
			SetupDiDestroyDeviceInfoList(hDevInfo);
		}
		return pid;
	}

#ifndef NOACPILIGHTS
	int Functions::AlienFXInitialize(AlienFan_SDK::Control* acc) {
		if (acc) {
			vid = 0x187c;
			pid = 0xffff;
			version = API_ACPI;
			device = new AlienFan_SDK::Lights(acc);
			if (((AlienFan_SDK::Lights*)device)->isActivated)
				return pid;
		}
		return -1;
	}
#endif

	bool Functions::Reset() {
		switch (version) {
		case API_V6:
			// need initial reset if not done
			if (chain) {
				PrepareAndSend(COMMV6_systemReset);
				chain = 0;
			}
			inSet = true;
			break;
		case API_V5:
		{
			inSet = PrepareAndSend(COMMV5_reset);
			GetDeviceStatus();
		} break;
		case API_V4:
		{
			PrepareAndSend(COMMV4_control, { {4, 4}/*, { 5, 0xff }*/ });
			inSet = PrepareAndSend(COMMV4_control, { {4, 1}/*, { 5, 0xff }*/ });
		} break;
		case API_V3: case API_V2:
		{
			chain = 1;
			inSet = PrepareAndSend(COMMV1_reset);
			WaitForReady();
			DebugPrint("Post-Reset status: " + to_string(GetDeviceStatus()) + "\n");
		} break;
		default: inSet = true;
		}
		return inSet;
	}

	bool Functions::UpdateColors() {
		if (inSet) {
			switch (version) {
			case API_V5:
			{
				inSet = !PrepareAndSend(COMMV5_update);
			} break;
			case API_V4:
			{
				inSet = !PrepareAndSend(COMMV4_control);
				WaitForReady();
			} break;
			case API_V3: case API_V2: 
			{
				inSet = !PrepareAndSend(COMMV1_update);
				DebugPrint("Post-update status: " + to_string(GetDeviceStatus()) + "\n");
			} break;
			default: inSet = false;
			}
			Sleep(5); // Fix for ultra-fast updates, or next command will fail sometimes.
		}
		return !inSet;

	}

	bool Functions::SetColor(byte index, Afx_action c) {
		vector<Afx_action> act{ c };
		return SetAction(index, &act);
	}

	byte Functions::AddV8DataBlock(byte bPos, vector<Afx_icommand>* mods, byte index, vector<Afx_action>* act) {
		mods->insert(mods->end(), { {bPos,index},{(byte)(bPos + 1),v8OpCodes[act->front().type]},
			{(byte)(bPos + 2),act->front().tempo},
			{ (byte)(bPos + 3), 0xa5}, {(byte)(bPos + 4),act->front().time }, {(byte)(bPos + 5), 0xa},
			{ (byte)(bPos + 6),act->front().r},{ (byte)(bPos + 7),act->front().g},{ (byte)(bPos + 8),act->front().b},
			{ (byte)(bPos + 9),act->back().r },{ (byte)(bPos + 10),act->back().g},{ (byte)(bPos + 11),act->back().b} });
		return bPos + 15;
	}

	byte Functions::AddV5DataBlock(byte bPos, vector<Afx_icommand>* mods, byte index, Afx_action* c) {
		mods->insert(mods->end(), { {bPos,(byte)(index + 1)},
			{(byte)(bPos + 1),c->r}, {(byte)(bPos + 2),c->g}, {(byte)(bPos + 3),c->b} });
		return bPos + 4;
	}

	bool Functions::SetMultiColor(vector<UCHAR> *lights, Afx_action c) {
		bool val = false;
		vector<Afx_action> act{ c };
		vector<Afx_icommand> mods;
		if (!inSet) Reset();
		switch (version) {
		case API_V8: {
			byte bPos = 5, cnt = 1;
			PrepareAndSend(COMMV8_readyToColor, { {2,(byte)lights->size()} });
			for (auto nc = lights->begin(); nc != lights->end(); nc++) {
				if (bPos + 15 > length) {
					// Send command and clear buffer...
					mods.push_back({ 4, cnt++ });
					val = PrepareAndSend(COMMV8_colorSet, &mods);
					bPos = 5;
				}
				bPos = AddV8DataBlock(bPos, &mods, *nc, &act);
			}
			if (bPos > 5) {
				mods.push_back({ 4, cnt });
				val = PrepareAndSend(COMMV8_colorSet, &mods);
			}
		} break;
		case API_V5:
		{
			byte bPos = 4;
			for (auto nc = lights->begin(); nc < lights->end(); nc++) {
				if (bPos + 4 > length) {
					val = PrepareAndSend(COMMV5_colorSet, &mods);
					bPos = 4;
				}
				bPos = AddV5DataBlock(bPos, &mods, *nc, &c);
			}
			if (bPos > 4)
				val = PrepareAndSend(COMMV5_colorSet, &mods);
			PrepareAndSend(COMMV5_loop);
		} break;
		case API_V4:
		{
			mods = { { 3, c.r }, { 4, c.g }, { 5, c.b }, {7, (byte)lights->size()} };
			for (int nc = 0; nc < lights->size(); nc++)
				mods.push_back({ (byte)(8 + nc), lights->at(nc)});
			val = PrepareAndSend(COMMV4_setOneColor, &mods);
		} break;
		case API_V3: case API_V2: case API_V6: case API_ACPI:
		{
			DWORD fmask = 0;
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				fmask |= 1 << (*nc);
			switch (version) {
			case API_V6:
				val = PrepareAndSend(COMMV6_colorSet, SetMaskAndColor(fmask, AlienFX_A_Color, c));
				break;
#ifndef NOACPILIGHTS
			case API_ACPI:
				val = ((AlienFan_SDK::Lights*)device)->SetColor((byte)fmask, c.r, c.g, c.b);
				break;
#endif
			default: {
				PrepareAndSend(COMMV1_color, SetMaskAndColor(fmask, 3, c));
				val = PrepareAndSend(COMMV1_loop);
				chain++;
			}
			}
		} break;
		default:
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				val = SetAction(*nc, &act);
		}
		return val;
	}

	bool Functions::SetMultiAction(vector<Afx_lightblock> *act, bool save) {
		bool val = true;
		vector<Afx_icommand> mods;
		if (save)
			SetPowerAction(act, save);
		else {
			if (!inSet) Reset();
			switch (version) {
			case API_V8: {
				byte bPos = 5, cnt = 1;
				PrepareAndSend(COMMV8_readyToColor, { {2,(byte)act->size()} });
				for (auto nc = act->begin(); nc != act->end(); nc++) {
					if (bPos + 15 > length) {
						// Send command and clear buffer...
						mods.push_back({ 4, cnt++ });
						val = PrepareAndSend(COMMV8_colorSet, &mods);
						bPos = 5;
					}
					bPos = AddV8DataBlock(bPos, &mods, nc->index, &nc->act);
				}
				if (bPos > 5) {
					mods.push_back({ 4, cnt });
					val = PrepareAndSend(COMMV8_colorSet, &mods);
				}
			} break;
			case API_V5:
			{
				byte bPos = 4;
				for (auto nc = act->begin(); nc != act->end(); nc++) {
					if (bPos + 4 > length) {
						PrepareAndSend(COMMV5_colorSet, &mods);
						bPos = 4;
					}
					bPos = AddV5DataBlock(bPos, &mods, nc->index, &nc->act.front());
				}
				if (bPos > 4)
					PrepareAndSend(COMMV5_colorSet, &mods);
				val = PrepareAndSend(COMMV5_loop);
			} break;
			default:
			{
				for (auto nc = act->begin(); nc != act->end(); nc++)
					val = SetAction(nc->index, &nc->act);
			} break;
			}
		}
		return val;
	}

	bool Functions::SetV4Action(byte index, vector<Afx_action>* act) {
		bool res = false;
		vector<Afx_icommand> mods;
		PrepareAndSend(COMMV4_colorSel, { {6,index} });
		byte bPos = 3;
		for (auto ca = act->begin(); ca != act->end(); ca++) {
			// 3 actions per record..
			mods.insert(mods.end(), {
				{bPos,(byte)(ca->type < AlienFX_A_Breathing ? ca->type : AlienFX_A_Morph) },
						{(byte)(bPos + 1), ca->time},
						{(byte)(bPos + 2), v4OpCodes[ca->type]},
						{(byte)(bPos + 4), (byte)(ca->type == AlienFX_A_Color ? 0xfa : ca->tempo) },
						{(byte)(bPos + 5), ca->r},
						{(byte)(bPos + 6), ca->g},
						{(byte)(bPos + 7), ca->b} });
			bPos += 8;
			if (bPos + 8 >= length) {
				res = PrepareAndSend(COMMV4_colorSet, &mods);
				bPos = 3;
			}
		}
		if (bPos > 3) {
			res = PrepareAndSend(COMMV4_colorSet, &mods);
		}
		return res;
	}

	bool Functions::SetAction(byte index, vector<Afx_action>* act) {
		vector<Afx_icommand> mods;

		if (!inSet) Reset();
		switch (version) {
		case API_V8:
			AddV8DataBlock(5, &mods, index, act);
			PrepareAndSend(COMMV8_readyToColor);
			return PrepareAndSend(COMMV8_colorSet, &mods);
		case API_V7:
		{
			mods = {{5,v7OpCodes[act->front().type]},{6,bright},{7,index}};
			for (int ca = 0; ca < act->size(); ca++) {
				if (ca*3+10 < length)
					mods.insert(mods.end(), {
						{(byte)(ca*3+8), act->at(ca).r},
						{(byte)(ca*3+9), act->at(ca).g},
						{(byte)(ca*3+10), act->at(ca).b}});
			}
			return PrepareAndSend(COMMV7_control, &mods);
		}
		case API_V6:
			return PrepareAndSend(COMMV6_colorSet, SetMaskAndColor(1 << index, act->front().type,
				{ act->front().b, act->front().g, act->front().r },
				{ act->back().b, act->back().g, act->back().r },
				act->front().tempo));
		case API_V5:
			AddV5DataBlock(4, &mods, index, &act->front());
			PrepareAndSend(COMMV5_colorSet, &mods);
			return PrepareAndSend(COMMV5_loop);
		case API_V4:
			// check types and call
			switch (act->front().type) {
			case AlienFX_A_Color: // it's a color, so set as color
				return PrepareAndSend(COMMV4_setOneColor, { {3,act->front().r}, {4,act->front().g}, {5,act->front().b}, {7, 1}, {8, (byte)index } });
			case AlienFX_A_Power: { // Set power
				vector<Afx_lightblock> t = { {index, *act} };
				return SetPowerAction(&t);
			} break;
			default: // Set action
				return SetV4Action(index, act);
			}
		case API_V3: case API_V2:
		{
			bool res = false;
			// check types and call
			switch (act->front().type) {
			case AlienFX_A_Power: { // SetPowerAction for power!
				vector<Afx_lightblock> t = { {index, *act} };
				return SetPowerAction(&t);
			} break;
			case AlienFX_A_Color:
				break;
			default:
				PrepareAndSend(COMMV1_setTempo,
					{ {2,(byte)(((UINT)act->front().tempo << 3 & 0xff00) >> 8)},
						{3,(byte)((UINT)act->front().tempo << 3 & 0xff)},
						{4,(byte)(((UINT)act->front().time << 5 & 0xff00) >> 8)},
						{5,(byte)((UINT)act->front().time << 5 & 0xff)} });
			}

			for (auto ca = act->begin(); ca != act->end(); ca++) {
				Afx_action c2{ 0 };
				byte actmode = 3;
				switch (ca->type) {
				case AlienFX_A_Morph: actmode = 1; break;
				case AlienFX_A_Pulse: actmode = 2; break;
				}
				if (act->size() > 1)
					c2 = (ca != act->end() - 1) ? *(ca + 1) : act->front();
				PrepareAndSend(COMMV1_color, SetMaskAndColor(1 << index, actmode, *ca, c2));
			}
			res = PrepareAndSend(COMMV1_loop);
			chain++;
			return res;
		}
#ifndef NOACPILIGHTS
		case API_ACPI:
			return ((AlienFan_SDK::Lights*)device)->SetColor(1 << index, act->front().r, act->front().g, act->front().b);
			break;
#endif
		}
		return false;
	}

	bool Functions::SetPowerAction(vector<Afx_lightblock> *act, bool save) {
		Afx_lightblock* pwr = NULL;
		switch (version) {
		// ToDo - APIv8 profile save
		case API_V4:
		{
			UpdateColors();
			if (save) {
				PrepareAndSend(COMMV4_control, { { 4, 4 }, { 6, 0x61 } });
				PrepareAndSend(COMMV4_control, { { 4, 1 }, { 6, 0x61 } });
				for (auto ca = act->begin(); ca != act->end(); ca++)
					if (ca->act.front().type != AlienFX_A_Power)
						SetV4Action(ca->index, &ca->act);
				PrepareAndSend(COMMV4_control, { { 4, 2 }, { 6, 0x61 } });
				PrepareAndSend(COMMV4_control, { { 4, 6 }, { 6, 0x61 }});
				//PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { { 4, 7 }, { 6, 0x61 } });
			}
			else {
				pwr = &act->front();
				// Now set power button....
				for (BYTE cid = 0x5b; cid < 0x61; cid++) {
					// Init query...
					PrepareAndSend(COMMV4_setPower, { {6,cid},{4,4} });
					PrepareAndSend(COMMV4_setPower, { {6,cid},{4,1} });
					// Now set color by type...
					vector<Afx_action> tact;
					switch (cid) {
					case 0x5b: // AC sleep
						tact.push_back(pwr->act.front());// afx_act{AlienFX_A_Power, 3, 0x64, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b});
						tact.push_back(Afx_action{ AlienFX_A_Power, 3, 0x64, 0, 0, 0 });
						break;
					case 0x5c: // AC power
						tact.push_back(pwr->act.front());// afx_act{AlienFX_A_Color, 0, 0, pwr->act.front().r, act[index].act.front().g, act[index].act.front().b});
						tact.front().type = AlienFX_A_Color;
						break;
					case 0x5d: // Charge
						tact.push_back(pwr->act.front());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.front().r, act[index].act.front().g, act[index].act.front().b});
						tact.push_back(pwr->act.back());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						break;
					case 0x5e: // Battery sleep
						tact.push_back(pwr->act.back());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.push_back(Afx_action{ AlienFX_A_Power, 3, 0x64, 0, 0, 0 });
						break;
					case 0x5f: // Batt power
						tact.push_back(pwr->act.back());// afx_act{AlienFX_A_Color, 0, 0, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.front().type = AlienFX_A_Color;
						break;
					case 0x60: // Batt critical
						tact.push_back(pwr->act.back());// afx_act{AlienFX_A_Color, 0, 0, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.front().type = AlienFX_A_Pulse;
					}
					SetV4Action(pwr->index, &tact);
					// And finish
					PrepareAndSend(COMMV4_setPower, { {6,cid},{4,2} });
				}

				PrepareAndSend(COMMV4_control, { { 4, 5 }/*, { 6, 0x61 }*/ });
#ifdef _DEBUG
				if (!WaitForBusy())
					DebugPrint("Power device busy timeout!\n");
#else
				WaitForBusy();
#endif
				WaitForReady();
			}
		} break;
		case API_V3: case API_V2:
		{
			if (!inSet) Reset();
			for (auto ca = act->begin(); ca != act->end(); ca++)
				if (ca->act.front().type != AlienFX_A_Power)
					SavePowerBlock(1, &(*ca), false);
				else
					pwr = &(*ca);
			if (pwr) {
				chain = 1;
				if (save) {
					// 08 02 - AC standby
					Afx_lightblock tact{ pwr->index,
								   {{AlienFX_A_Morph, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}, {2}} };
					/*SavePowerBlock(2, tact, false);*/
					//tact = {pwr->index,
					//	   {{AlienFX_A_Morph, 0 , 0, 0, 0, 0},
					//	   {2,0,0,pwr->act.back().r, pwr->act.back().g, pwr->act.back().b}}};
					SavePowerBlock(2, &tact, true, true);
					// 08 05 - AC power
					tact = { pwr->index,
							{{AlienFX_A_Color, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}, {0}} };
					SavePowerBlock(5, &tact, true);
					// 08 06 - charge
					tact = { pwr->index,
							{{AlienFX_A_Morph, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b},
							{0,0,0,pwr->act.back().r, pwr->act.back().g, pwr->act.back().b}} };
					SavePowerBlock(6, &tact, false);
					tact = { pwr->index,
							{{AlienFX_A_Morph, 0 , 0, pwr->act.back().r, pwr->act.back().g, pwr->act.back().b},
							{0,0,0,pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}} };
					SavePowerBlock(6, &tact, true);
					// 08 07 - Battery standby
					tact = { pwr->index,
							{{AlienFX_A_Morph, 0 , 0, pwr->act.back().r, pwr->act.back().g, pwr->act.back().b},	{2}} };
					SavePowerBlock(7, &tact, true, true);
					// 08 08 - battery
					tact.act.front().type = AlienFX_A_Color;
					SavePowerBlock(8, &tact, true);
					// 08 09 - battery critical
					tact.act.front().type = AlienFX_A_Pulse;
					SavePowerBlock(9, &tact, true);
				}
				//int pind = powerMode ? 0 : 1;
				//SetColor(pwr->index, { pwr->act[pind].b, pwr->act[pind].g, pwr->act[pind].r });
			}
			if (act->size())
				PrepareAndSend(COMMV1_save);
			if (pwr)
				SetAction(pwr->index, &pwr->act);
		} break;
		}
		return true;
	}

	bool Functions::SetBrightness(BYTE brightness, vector<Afx_light> *mappings, bool power) {

		DebugPrint("State update: PID: " + to_string(pid) + ", brightness: " + to_string(brightness) + ", power: " + to_string(power) + "\n");

		byte oldBright = bright;
		bright = ((UINT)brightness * 0x64) / 0xff;
		if (inSet) UpdateColors();
		switch (version) {
		case API_V8:
			return PrepareAndSend(COMMV8_setBrightness, { {2, (byte)(brightness * 10 / 0xff)} });
		case API_V5:
			Reset();
			return PrepareAndSend(COMMV5_turnOnSet, {{4,brightness}});
		case API_V4: {
			vector<Afx_icommand> mods{ {3,(byte)(0x64 - bright)} };
			byte pos = 6;
			for (auto i = mappings->begin(); i < mappings->end(); i++)
				if (pos < length && (!i->flags || power)) {
					mods.push_back({ pos,(byte)i->lightid });
					pos++;
				}
			mods.push_back({ 5,(byte)mappings->size() });
			return PrepareAndSend(COMMV4_turnOn, &mods);
		}
		case API_V3: case API_V2:
			if (!oldBright || !bright) {
				PrepareAndSend(COMMV1_reset, { {2,(byte)(bright ? 4 : power ? 3 : 1)} });
				WaitForReady();
			}
			return PrepareAndSend(COMMV1_dim, { { 2,bright } });
#ifndef NOACPILIGHTS
		case API_ACPI:
			bright = brightness * 0xf / 0xff;
			return ((AlienFan_SDK::Lights*)device)->SetBrightness(bright);
#endif // !NOACPILIGHTS
		}
		return false;
	}

	bool Functions::SetGlobalEffects(byte effType, byte mode, byte tempo, Afx_action act1, Afx_action act2) {
		vector<Afx_icommand> mods;
		switch (version) {
		case API_V8:
			PrepareAndSend(COMMV8_effectReady);
			return PrepareAndSend(COMMV8_effectSet, {{3, effType},
				{4, act1.r}, {5, act1.g}, {6, act1.b},
				{7, act2.r}, {8, act2.g}, {9, act2.b},
				{10, tempo}, {13, mode}, {14, 2}});
		case API_V5:
			if (inSet)
				UpdateColors();
			Reset();
			if (effType < 2)
				PrepareAndSend(COMMV5_setEffect, { {2,1}, {11,0xff}, {12,0xff}, {14,0xff}, {15,0xff}});
			else
				PrepareAndSend(COMMV5_setEffect, { {2,effType}, {3,tempo},
						   {10,act1.r}, {11,act1.g}, {12,act1.b},
						   {13,act2.r}, {14,act2.g}, {15,act2.b},
						   {16,5}});
			if (effType < 2)
				return PrepareAndSend(COMMV5_update, { {3,0xfe}, {6,0xff}, {7,0xff} });
			else
				return UpdateColors();
		}
		return false;
	}

	BYTE Functions::GetDeviceStatus() {

		byte buffer[MAX_BUFFERSIZE];
		DWORD written;
		byte res = 0;
		switch (version) {
		case API_V5:
		{
			PrepareAndSend(COMMV5_status);
			//if (HidD_GetFeature(devHandle, buffer, length))
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_FEATURE, 0, 0, buffer, length, &written, NULL))
				res = buffer[2];
		} break;
		case API_V4:
		{
			//if (HidD_GetInputReport(devHandle, buffer, length))
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
				res = buffer[2];
		} break;
		case API_V3: case API_V2:
		{
			PrepareAndSend(COMMV1_status);
			//HidD_GetInputReport(devHandle, buffer, length);
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
				res = /*buffer[0] == 0x01 ? ALIENFX_V2_RESET : */buffer[0];
		} break;
		}

		//DebugPrint("Status: " + to_string(written) + " bytes, " + to_string(res) + "\n");
		return res;
	}

	BYTE Functions::WaitForReady() {
		int i = 0;
		switch (version) {
		case API_V3: case API_V2:
			if (!GetDeviceStatus())
				Reset();
			for (; i < 200 && GetDeviceStatus() != ALIENFX_V2_READY/* && status != ALIENFX_V2_RESET*/; i++) {
				Sleep(10);
			}
			return i != 200;
		case API_V4:
			for (; !IsDeviceReady(); i++) {
				Sleep(20);
			}
			return 1;
		default:
			return GetDeviceStatus();
		}
	}

	BYTE Functions::WaitForBusy() {
		int i = 0;
		switch (version) {
		case API_V3: case API_V2:
			if (!GetDeviceStatus())
				return 0;
			for (; i < 200 && GetDeviceStatus() != ALIENFX_V2_BUSY; i++) {
				Sleep(10);
			}
			return i != 200;
			break;
		case API_V4: {
			for (; i < 500 && GetDeviceStatus() != ALIENFX_V4_BUSY; i++)
				Sleep(20);
			return i != 500;
		} break;
		default:
			return GetDeviceStatus();
		}
	}

	BYTE Functions::IsDeviceReady() {
		int status = GetDeviceStatus();
		switch (version) {
		case API_V5:
			return status != ALIENFX_V5_WAITUPDATE;
		case API_V4:
#ifdef _DEBUG
			status = status ? status != ALIENFX_V4_BUSY : 0xff;
			if (status == 0xff)
				DebugPrint("Device hang!\n");
			return status;
#else
			return status ? status != ALIENFX_V4_BUSY : 0xff;
#endif
		case API_V3: case API_V2:
			return status == ALIENFX_V2_READY ? 1 : Reset();
			//switch (status) {
			//case ALIENFX_V2_READY:
			//	return 1;
			//case ALIENFX_V2_BUSY: case 0:
			//	return Reset();
			//}
			//return 0;
		default:
			return !inSet;
		}
	}

	Functions::~Functions() {
		if (devHandle) {
			CloseHandle(devHandle);
		}
#ifndef NOACPILIGHTS
		if (device) {
			delete (AlienFan_SDK::Lights*)device;
		}
#endif
	}

	Mappings::~Mappings() {
		for (auto i = fxdevs.begin(); i < fxdevs.end(); i++) {
			if (i->dev) {
				delete i->dev;
			}
		}
	}

	vector<Functions*> Mappings::AlienFXEnumDevices(void* acc) {
		vector<Functions*> devs;
		GUID guid;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo != INVALID_HANDLE_VALUE) {
			SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };
			for (DWORD dw = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData); dw++) {
				Functions* dev = new Functions();
				//DebugPrint("Testing device #" + to_string(dw) + ", ID=" + to_string(deviceInterfaceData.Reserved) + "\n");
				if ((deviceInterfaceData.Flags & SPINT_ACTIVE) && dev->ProbeDevice(hDevInfo, &deviceInterfaceData))
				{
					devs.push_back(dev);
					DebugPrint("Scan #" + to_string(dw) + ": VID: " + to_string(dev->vid) + ", PID: " + to_string(dev->pid) + ", Version: "
						+ to_string(dev->version) + "\n");
				}
				else
					delete dev;
			}
			SetupDiDestroyDeviceInfoList(hDevInfo);
		}
#ifndef NOACPILIGHTS
		// add ACPI, if any
		if (acc) {
			Functions* devc = new AlienFX_SDK::Functions();
			if (devc->AlienFXInitialize((AlienFan_SDK::Control*)acc) < 0)
				delete devc;
			else
				devs.push_back(devc);
		}
#endif
		return devs;
	}

	void Mappings::AlienFXApplyDevices(bool activeOnly, vector<Functions*> devList, byte brightness, bool power) {
		activeLights = 0;
		activeDevices = (int)devList.size();

		// check old devices...
		for (auto i = fxdevs.begin(); i != fxdevs.end(); ) {
			auto nDev = devList.begin();
			for (; nDev != devList.end(); nDev++)
				if (i->vid == (*nDev)->vid && i->pid == (*nDev)->pid) {
					// Still present
					i++;
					break;
				}
			if (nDev == devList.end()) {
				// not found
				if (activeOnly)
					i = fxdevs.erase(i);
				else {
					delete i->dev;
					i->dev = NULL;
					i++;
				}
			}
		}

		// add new devices...
		for (auto i = devList.begin(); i != devList.end(); i++) {
			Afx_device* dev = AddDeviceById((*i)->pid, (*i)->vid);
			if (!dev->dev) {
				dev->dev = *i;
				dev->version = (*i)->version;
				dev->dev->SetBrightness(brightness, &dev->lights, power);
			}
			activeLights += (int)dev->lights.size();
		}
		devList.clear();
	}

	void Mappings::AlienFXAssignDevices(bool activeOnly, void* acc, byte brightness, bool power) {
		AlienFXApplyDevices(activeOnly, AlienFXEnumDevices(acc), brightness, power);
	}

	Afx_device* Mappings::GetDeviceById(WORD pid, WORD vid) {
		for (auto pos = fxdevs.begin(); pos < fxdevs.end(); pos++)
			if (pos->pid == pid && (!vid || pos->vid == vid)) {
				return &(*pos);
			}
		return nullptr;
	}

	Afx_grid* Mappings::GetGridByID(byte id)
	{
		for (auto pos = grids.begin(); pos < grids.end(); pos++)
			if (pos->id == id)
				return &(*pos);
		return nullptr;
	}

	Afx_device* Mappings::AddDeviceById(WORD pid, WORD vid)
	{
		Afx_device* dev = GetDeviceById(pid, vid);
		if (!dev) {
			fxdevs.push_back({ vid, pid, NULL });
			dev = &fxdevs.back();
		}
		return dev;
	}

	Afx_light *Mappings::GetMappingByDev(Afx_device* dev, WORD LightID) {
		if (dev) {
			for (auto pos = dev->lights.begin(); pos < dev->lights.end(); pos++)
				if (pos->lightid == LightID)
					return &(*pos);
		}
		return nullptr;
	}

	vector<Afx_group> *Mappings::GetGroups() {
		return &groups;
	}

	void Mappings::RemoveMapping(Afx_device* dev, WORD lightID)
	{
		if (dev) {
			for (auto del_map = dev->lights.begin(); del_map != dev->lights.end(); del_map++)
				if (del_map->lightid == lightID) {
					dev->lights.erase(del_map);
					return;
				}
		}
	}

	Afx_group *Mappings::GetGroupById(DWORD gID) {
		for (auto pos = groups.begin(); pos != groups.end(); pos++)
			if (pos->gid == gID)
				return &(*pos);
		return nullptr;
	}


	void Mappings::LoadMappings() {
		HKEY   mainKey;

		fxdevs.clear();
		groups.clear();
		grids.clear();

		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &mainKey, NULL);
		unsigned vindex;
		char kName[255], name[255];
		DWORD len, lend, dID;
		WORD vid, pid;
		byte lID;
		for (vindex = 0; RegEnumValue(mainKey, vindex, kName, &(len = 255), NULL, NULL, (LPBYTE)name, &(lend = 255)) == ERROR_SUCCESS; vindex++) {
			if (sscanf_s(kName, "Dev#%hd_%hd", &vid, &pid) == 2) {
				AddDeviceById(pid, vid)->name = string(name);
				continue;
			}
			if (sscanf_s(kName, "DevWhite#%hd_%hd", &vid, &pid) == 2) {
				AddDeviceById(pid, vid)->white.ci = ((DWORD*)name)[0];
				continue;
			}
		}
		for (vindex = 0; RegEnumKey(mainKey, vindex, kName, 255) == ERROR_SUCCESS; vindex++) {
			if (sscanf_s(kName, "Light%d-%hhd", &dID, &lID) == 2) {
				RegGetValue(mainKey, kName, "Name", RRF_RT_REG_SZ, 0, name, &(lend = 255));
				RegGetValue(mainKey, kName, "Flags", RRF_RT_REG_DWORD, 0, &len, &(lend = sizeof(DWORD)));
				AddDeviceById(LOWORD(dID), HIWORD(dID))->lights.push_back({ lID, { LOWORD(len), HIWORD(len) }, name });
			}
			if (sscanf_s(kName, "Grid%d", &dID) == 1) {
				RegGetValue(mainKey, kName, "Name", RRF_RT_REG_SZ, 0, name, &(lend = 255));
				RegGetValue(mainKey, kName, "Size", RRF_RT_REG_DWORD, 0, &len, &(lend = sizeof(DWORD)));
				byte x = HIBYTE(len), y = LOBYTE(len);
				Afx_groupLight* grid = new Afx_groupLight[x * y];
				RegGetValue(mainKey, kName, "Grid", RRF_RT_REG_BINARY, 0, grid, &(lend = x * y * sizeof(DWORD)));
				grids.push_back({ (byte)dID, x, y, name, grid });
			}
		}
		for (vindex = 0; RegEnumKey(mainKey, vindex, kName, 255) == ERROR_SUCCESS; vindex++)  {
			if (sscanf_s(kName, "Group%d", &dID) == 1) {
				RegGetValue(mainKey, kName, "Name", RRF_RT_REG_SZ, 0, name, &(lend = 255));
				RegGetValue(mainKey, kName, "Lights", RRF_RT_REG_BINARY, 0, NULL, &lend);
				len = lend / sizeof(DWORD);
				DWORD* maps = new DWORD[len];
				RegGetValue(mainKey, kName, "Lights", RRF_RT_REG_BINARY, 0, maps, &lend);
				groups.push_back({dID, name});
				for (unsigned i = 0; i < len; i += 2) {
					groups.back().lights.push_back({ LOWORD(maps[i]), (WORD)maps[i + 1] });
					if (GetFlags(LOWORD(maps[i]), (WORD)maps[i + 1]) & ALIENFX_FLAG_POWER)
						groups.back().have_power = true;
				}
				delete[] maps;
			}
		}
		RegCloseKey(mainKey);
	}

	void Mappings::SaveMappings() {

		HKEY   hKeybase, hKeyStore;
		size_t numGroups = groups.size();
		size_t numGrids = grids.size();

		// Remove all maps!
		RegDeleteTree(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"));
		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeybase, NULL);

		for (auto i = fxdevs.begin(); i != fxdevs.end(); i++) {
			// Saving device data..
			string devID = to_string(i->vid) + "_" + to_string(i->pid);
			string name = "Dev#" + devID;
			RegSetValueEx(hKeybase, name.c_str(), 0, REG_SZ, (BYTE *) i->name.c_str(), (DWORD) i->name.length() );
			name = "DevWhite#" + devID;
			RegSetValueEx(hKeybase, name.c_str(), 0, REG_DWORD, (BYTE *) &i->white.ci, sizeof(DWORD));
			for (auto cl = i->lights.begin(); cl < i->lights.end(); cl++) {
				// Saving all lights from current device
				string name = "Light" + to_string(MAKELONG(i->pid, i->vid)) + "-" + to_string(cl->lightid);
				RegCreateKey(hKeybase, name.c_str(), &hKeyStore);
				RegSetValueEx(hKeyStore, "Name", 0, REG_SZ, (BYTE*)cl->name.c_str(), (DWORD)cl->name.length());
				RegSetValueEx(hKeyStore, "Flags", 0, REG_DWORD, (BYTE*)&cl->data, sizeof(DWORD));
				RegCloseKey(hKeyStore);
			}
		}

		for (auto i = groups.begin(); i != groups.end(); i++) {
			string name = "Group" + to_string(i->gid);

			RegCreateKey(hKeybase, name.c_str(), &hKeyStore);
			RegSetValueEx(hKeyStore, "Name", 0, REG_SZ, (BYTE *) i->name.c_str(), (DWORD) i->name.size());

			DWORD *grLights = new DWORD[i->lights.size() * 2];

			for (int j = 0; j < i->lights.size(); j++) {
				grLights[j * 2] = i->lights[j].did;
				grLights[j * 2 + 1] = i->lights[j].lid;
			}
			RegSetValueEx(hKeyStore, "Lights", 0, REG_BINARY, (BYTE *) grLights, 2 * (DWORD)i->lights.size() * sizeof(DWORD) );

			delete[] grLights;
			RegCloseKey(hKeyStore);
		}

		for (auto i = grids.begin(); i != grids.end(); i++) {
			string name = "Grid" + to_string(i->id);
			RegCreateKey(hKeybase, name.c_str(), &hKeyStore);
			RegSetValueEx(hKeyStore, "Name", 0, REG_SZ, (BYTE*)i->name.c_str(), (DWORD)i->name.length());
			DWORD sizes = ((DWORD)i->x << 8) | i->y;
			RegSetValueEx(hKeyStore, "Size", 0, REG_DWORD, (BYTE*)&sizes, sizeof(DWORD));
			RegSetValueEx(hKeyStore, "Grid", 0, REG_BINARY, (BYTE*)i->grid, i->x * i->y * sizeof(DWORD));
			RegCloseKey(hKeyStore);
		}

		RegCloseKey(hKeybase);
	}

	Afx_light *Mappings::GetMappingByID(WORD pid, WORD lid) {
		Afx_device* dev = GetDeviceById(pid);
		if (dev)
			return GetMappingByDev(dev, lid);
		return nullptr;
	}

	int Mappings::GetFlags(Afx_device* dev, WORD lightid) {
		Afx_light* lgh = GetMappingByDev(dev, lightid);
		if (lgh)
			return lgh->flags;
		return 0;
	}

	int Mappings::GetFlags(DWORD devID, WORD lightid)
	{
		Afx_device* dev = GetDeviceById(LOWORD(devID), HIWORD(devID));
		if (dev)
			return GetFlags(dev, lightid);
		return 0;
	}

	//// get PID for current device
	//int Functions::GetPID() { return pid; };

	//// get VID for current device
	//int Functions::GetVID() { return vid; };

	//// get API version for current device
	//int Functions::GetVersion() { return version; };

	bool Functions::IsHaveGlobal()
	{
		switch (version) {
		case API_V5: case API_V8: return true;
		}
		return false;
	}
}