//#define WIN32_LEAN_AND_MEAN
#include "AlienFX_SDK.h"
#include "alienfx-controls.h"
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}
#include <SetupAPI.h>
#include <memory>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace AlienFX_SDK {

	vector<Afx_icommand> *Functions::SetMaskAndColor(DWORD index, byte type, Afx_colorcode c1, Afx_colorcode c2, byte tempo) {
		vector<Afx_icommand>* mods;
		if (version == API_V6)
			mods = new vector<Afx_icommand>({ {9,(byte)index}, { 10,c1.r }, { 11,c1.g }, { 12,c1.b } });
		else
			mods = new vector<Afx_icommand>({ {1, type}, { 2,(byte)chain },
			{ 3,(byte)((index & 0xFF0000) >> 16) }, { 4,(byte)((index & 0x00FF00) >> 8) }, { 5,(byte)(index & 0x0000FF) }});
		switch (version) {
		case API_V1:
			mods->insert(mods->end(),{{6,c1.r},{7,c1.g},{8,c1.b}});
			break;
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

	inline bool Functions::PrepareAndSend(const byte* command, byte size, vector<Afx_icommand> mods) {
		return PrepareAndSend(command, size, &mods);
	}

	bool Functions::PrepareAndSend(const byte *command, byte size, vector<Afx_icommand> *mods) {
		byte buffer[MAX_BUFFERSIZE];
		DWORD written;
		BOOL res = false;

		FillMemory(buffer, MAX_BUFFERSIZE, version == API_V6 && size != 3 ? 0xff : 0);

		memcpy(&buffer[1], command, size);
		buffer[0] = reportID;

		if (mods) {
			for (auto i = mods->begin(); i < mods->end(); i++)
				buffer[i->i] = i->val;
			mods->clear();
		}

		switch (version) {
		case API_V1: case API_V2: case API_V3:
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
			if (size == 3)
				res &= ReadFile(devHandle, buffer, length, &written, NULL);
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
				Sleep(6); // Need wait for ACK
			}
			else {
				res = WriteFile(devHandle, buffer, length, &written, NULL);
			}
		}
		}
#ifdef _DEBUG
		if (!res)
			OutputDebugString("Device IO fail!\n");
#endif
		return res;
	}

	bool Functions::SavePowerBlock(byte blID, Afx_lightblock act, bool needSave, bool needInverse) {
		byte mode;
		vector<Afx_icommand> separator = { {2, blID} };
		switch (act.act.front().type) {
		case AlienFX_A_Pulse:
			mode = 2; break;
		case AlienFX_A_Morph:
			mode = 1; break;
		default: mode = 3;
		}
		PrepareAndSend(COMMV1_saveGroup, sizeof(COMMV1_saveGroup), &separator);
		if (act.act.size() < 2)
			PrepareAndSend(COMMV1_color, sizeof(COMMV1_color), SetMaskAndColor(1 << act.index, mode,
																			   {act.act.front().b, act.act.front().g, act.act.front().r}));
		else
			PrepareAndSend(COMMV1_color, sizeof(COMMV1_color), SetMaskAndColor(1 << act.index, mode,
																			   {act.act.front().b, act.act.front().g, act.act.front().r},
																			   {act.act.back().b, act.act.back().g, act.act.back().r}));
		PrepareAndSend(COMMV1_saveGroup, sizeof(COMMV1_saveGroup), &separator);
		Loop();

		if (needInverse) {
			PrepareAndSend(COMMV1_saveGroup, sizeof(COMMV1_saveGroup), &separator);
			PrepareAndSend(COMMV1_color, sizeof(COMMV1_color), SetMaskAndColor(~((1 << act.index)), act.act.back().type,
																			   {act.act.front().b, act.act.front().g, act.act.front().r},
																			   {act.act.back().b, act.act.back().g, act.act.back().r}));
			PrepareAndSend(COMMV1_saveGroup, sizeof(COMMV1_saveGroup), &separator);
			Loop();
		}

		if (needSave) {
			PrepareAndSend(COMMV1_save, sizeof(COMMV1_save));
			Reset();
		}

		return true;
	}

	int Functions::AlienFXCheckDevice(string devPath, int vidd, int pidd)
	{
		if ((devHandle = CreateFile(devPath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_ANONYMOUS/*FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH*/, NULL)) != INVALID_HANDLE_VALUE) {
			PHIDP_PREPARSED_DATA prep_caps;
			HIDP_CAPS caps;
			std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES({ sizeof(HIDD_ATTRIBUTES) }));
			if (HidD_GetAttributes(devHandle, attributes.get()) &&
				(vidd == -1 || attributes->VendorID == vidd) && (pidd == -1 || attributes->ProductID == pidd)) {
				HidD_GetPreparsedData(devHandle, &prep_caps);
				HidP_GetCaps(prep_caps, &caps);
				HidD_FreePreparsedData(prep_caps);
				length = caps.OutputReportByteLength;
				vid = attributes->VendorID;
				pid = attributes->ProductID;
				switch (length) {
				case 0: {
					if (caps.Usage == 0xcc && vid == 0x0d62) {
						length = caps.FeatureReportByteLength;
						version = API_V5;
						reportID = 0xcc;
					}
				} break;
				case 8:
					if (vid == 0x187c) {
						version = API_V1;
						reportID = 2;
					}
					break;
				case 9:
					if (vid == 0x187c) {
						version = API_V2;
						reportID = 2;
					}
					break;
				case 12:
					if (vid == 0x187c) {
						version = API_V3;
						reportID = 2;
					}
					break;
				case 34:
					if (vid == 0x187c) {
						version = API_V4;
						reportID = 0;
					}
					break;
				case 65:
					switch (vid) {
					case 0x0424: case 0x187c:
						version = API_V6;
						reportID = 0;
						// device init
						PrepareAndSend(COMMV6_systemReset, sizeof(COMMV6_systemReset));
						break;
					case 0x0461:
						version = API_V7;
						reportID = 0;
						break;
					case 0x04f2:
						version = API_V8;
						reportID = 0xe;
						break;
					}
					break;
				}
			}
			attributes.release();
			if (version < 0) {
				CloseHandle(devHandle); devHandle = NULL;
			}
			else
				return pid;
		}
		return -1;
	}

	//Use this method for general devices, vid = -1 for any vid, pid = -1 for any pid.
	int Functions::AlienFXInitialize(int vidd, int pidd) {
		GUID guid;
		bool flag = true;
		pid = -1;
		DWORD dw = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo != INVALID_HANDLE_VALUE) {
			while (flag && SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData)) {
				dw++;
				DWORD dwRequiredSize = 0;
				SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL);
				std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new byte[dwRequiredSize]);
				deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
				if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL)) {
					flag = AlienFXCheckDevice(deviceInterfaceDetailData->DevicePath, vidd, pidd) < 0;
				}
				deviceInterfaceDetailData.release();
			}
			SetupDiDestroyDeviceInfoList(hDevInfo);
		}
		return pid;
	}

#ifndef NOACPILIGHTS
	int Functions::AlienFXInitialize(AlienFan_SDK::Control* acc) {
		if (acc) {
			version = vid = pid = API_ACPI;
			device = new AlienFan_SDK::Lights(acc);
			if (((AlienFan_SDK::Lights*)device)->isActivated)
				return pid;
		}
		return -1;
	}
#endif

	void Functions::Loop() {
		switch (version) {
		case API_V5:
			PrepareAndSend(COMMV5_loop, sizeof(COMMV5_loop));
			break;
		case API_V3: case API_V2: case API_V1:
		{
			PrepareAndSend(COMMV1_loop, sizeof(COMMV1_loop));
			chain++;
		} break;
		}
	}

	bool Functions::Reset() {
		BOOL result = false;
		switch (version) {
		case API_V5:
		{
			result = PrepareAndSend(COMMV5_reset, sizeof(COMMV5_reset));
			GetDeviceStatus();
		} break;
		case API_V4:
		{
			result = PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { {4, 4}/*, { 5, 0xff }*/ });
			result = PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { {4, 1}/*, { 5, 0xff }*/ });
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			result = PrepareAndSend(COMMV1_reset, sizeof(COMMV1_reset));
			WaitForReady();
			chain = 1;
		} break;
		}
		inSet = true;
		return result;
	}

	bool Functions::UpdateColors() {
		bool res = false;

		if (inSet) {
			switch (version) {
			case API_V5:
			{
				//PrepareAndSend(COMMV5_loop, sizeof(COMMV5_loop));
				res = PrepareAndSend(COMMV5_update, sizeof(COMMV5_update));
			} break;
			case API_V4:
			{
				res = PrepareAndSend(COMMV4_control, sizeof(COMMV4_control));
				WaitForReady();
			} break;
			case API_V3: case API_V2: case API_V1:
			{
				res = PrepareAndSend(COMMV1_update, sizeof(COMMV1_update));
				WaitForBusy();
				//chain = 1;
			} break;
			default: res = true;
			}
			inSet = false;
			Sleep(5); // Fix for ultra-fast updates, or next command will fail sometimes.
		}
		return res;

	}

	bool Functions::SetColor(unsigned index, Afx_colorcode c) {
		bool val = false;
		if (!inSet)
			Reset();
		switch (version) {
		case API_V8: {
			PrepareAndSend(COMMV8_readyToColor, sizeof(COMMV8_readyToColor));
			val = PrepareAndSend(COMMV8_colorSet, sizeof(COMMV8_colorSet), { {5,(byte)index},{11,c.r},{12,c.g},{13,c.b} });
		} break;
		case API_V7:
		{
			PrepareAndSend(COMMV7_status, sizeof(COMMV7_status));
			val = PrepareAndSend(COMMV7_control, sizeof(COMMV7_control), {{6,bright},{7,(byte)index},{8,c.r},{9,c.g},{10,c.b}});
		} break;
		case API_V6:
		{
			val = PrepareAndSend(COMMV6_colorSet, sizeof(COMMV6_colorSet), SetMaskAndColor(1<<index, AlienFX_A_Color, c));
		} break;
		case API_V5:
		{
			val = PrepareAndSend(COMMV5_colorSet, sizeof(COMMV5_colorSet), {{4,(byte)(index+1)},{5,c.r},{6,c.g},{7,c.b}});
		} break;
		case API_V4:
		{
			val = PrepareAndSend(COMMV4_setOneColor, sizeof(COMMV4_setOneColor), { {3,c.r}, {4,c.g}, {5,c.b},
				{7, 1}, {8, (byte)index } });
			//PrepareAndSend(COMMV4_colorSel, sizeof(COMMV4_colorSel), {{6,(byte)index}});
			//val = PrepareAndSend(COMMV4_colorSet, sizeof(COMMV4_colorSet), {{8,c.r}, {9,c.g}, {10,c.b}});
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			val = PrepareAndSend(COMMV1_color, sizeof(COMMV1_color), SetMaskAndColor(1<<index, 3, c));
		} break;
#ifndef NOACPILIGHTS
		case API_ACPI:
			val = ((AlienFan_SDK::Lights*)device)->SetColor(1 << index, c.r, c.g, c.b);
			break;
#endif
		default: return false;
		}
		Loop();
		return val;
	}

	void Functions::AddDataBlock(byte bPos, vector<Afx_icommand>* mods, Afx_lightblock* nc) {
		byte opType = 0x81;
		switch (nc->act.front().type) {
		case AlienFX_A_Pulse: opType = 0x82; break;
		case AlienFX_A_Morph: opType = 0x83; break;
		case AlienFX_A_Breathing: opType = 0x87; break;
		case AlienFX_A_Spectrum: opType = 0x88; break;
		//case AlienFX_A_Rainbow: opType = 0x81; break; // DEBUG, check for OFF
		}
		mods->insert(mods->end(), { {bPos,nc->index},{(byte)(bPos + 1),opType},{(byte)(bPos + 2),nc->act.front().tempo},
			{ (byte)(bPos + 3), 0xa5}, {(byte)(bPos + 4),nc->act.front().time }, {(byte)(bPos + 5), 0xa},
			{ (byte)(bPos + 6), nc->act.front().r},{ (byte)(bPos + 7),nc->act.front().g},{ (byte)(bPos + 8),nc->act.front().b} });
		if (nc->act.size() > 1)
			// add second color if present
			mods->insert(mods->end(), { { (byte)(bPos + 9),nc->act.back().r }, { (byte)(bPos + 10),nc->act.back().g }, { (byte)(bPos + 11),nc->act.back().b } });
	}

	bool Functions::SetMultiColor(vector<UCHAR> *lights, Afx_colorcode c) {
		bool val = false;
		if (!inSet) Reset();
		switch (version) {
		case API_V8: {
			vector<Afx_icommand> mods;
			byte bPos = 5, cnt = 1;
			PrepareAndSend(COMMV8_readyToColor, sizeof(COMMV8_readyToColor), { {2,(byte)lights->size()} });
			for (auto nc = lights->begin(); nc != lights->end(); nc++) {
				if (bPos + 15 > length) {
					// Send command and clear buffer...
					mods.push_back({ 4, cnt++ });
					val = PrepareAndSend(COMMV8_colorSet, sizeof(COMMV8_colorSet), &mods);
					bPos = 5;
				}

				Afx_lightblock act{ *nc, { {0,0,0,c.r,c.g,c.b} } };
				AddDataBlock(bPos, &mods, &act);
				bPos += 15;
			}
			if (bPos > 5) {
				mods.push_back({ 4, cnt });
				val = PrepareAndSend(COMMV8_colorSet, sizeof(COMMV8_colorSet), &mods);
			}
		} break;
		case API_V5:
		{
			vector<Afx_icommand> mods;
			byte bPos = 4;
			for (auto nc = lights->begin(); nc < lights->end(); nc++) {
				if (bPos + 4 > length) {
					// Send command and clear buffer...
					val = PrepareAndSend(COMMV5_colorSet, sizeof(COMMV5_colorSet), &mods);
					bPos = 4;
				}
				mods.insert(mods.end(), {
						{bPos,(byte)((*nc) + 1)},
						{(byte)(bPos+1),c.r},
						{(byte)(bPos+2),c.g},
						{(byte)(bPos+3),c.b}});
				bPos += 4;
			}
			if (bPos > 4)
				val = PrepareAndSend(COMMV5_colorSet, sizeof(COMMV5_colorSet), &mods);
			Loop();
		} break;
		case API_V4:
		{
			vector<Afx_icommand> mods;
			mods.push_back({7,(byte)lights->size()});
			for (int nc = 0; nc < lights->size(); nc++)
				mods.push_back({ (byte)(8 + nc), lights->at(nc)});
			//PrepareAndSend(COMMV4_colorSel, sizeof(COMMV4_colorSel), &mods);
			//val = PrepareAndSend(COMMV4_colorSet, sizeof(COMMV4_colorSet), {{8,c.r}, {9,c.g}, {10,c.b}});
			val = PrepareAndSend(COMMV4_setOneColor, sizeof(COMMV4_setOneColor), &mods);
		} break;
		case API_V3: case API_V2: case API_V1: case API_V6:
		{
			DWORD fmask = 0;
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				fmask |= 1 << (*nc);
			if (version == API_V6)
				val = PrepareAndSend(COMMV6_colorSet, sizeof(COMMV6_colorSet), SetMaskAndColor(fmask, AlienFX_A_Color, c));
			else
				val = PrepareAndSend(COMMV1_color, sizeof(COMMV1_color), SetMaskAndColor(fmask, 3, c));
			Loop();
		} break;
#ifndef NOACPILIGHTS
		case API_ACPI:
		{
			byte fmask = 0;
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				fmask |= 1 << (*nc);
			val = ((AlienFan_SDK::Lights*)device)->SetColor(fmask, c.r, c.g, c.b);
		} break;
#endif
		default:
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				val = SetColor(*nc, c);
		}
		return val;
	}

	bool Functions::SetMultiAction(vector<Afx_lightblock> *act, bool save) {
		bool val = true;
		if (save)
			SetPowerAction(act, save);
		else {
			if (!inSet) Reset();
			switch (version) {
			case API_V8: {
				byte bPos = 5, cnt = 1;
				vector<Afx_icommand> mods;
				PrepareAndSend(COMMV8_readyToColor, sizeof(COMMV8_readyToColor), { {2,(byte)act->size()} });
				for (auto nc = act->begin(); nc != act->end(); nc++) {
					if (bPos + 15 > length) {
						// Send command and clear buffer...
						mods.push_back({ 4, cnt++ });
						val = PrepareAndSend(COMMV8_colorSet, sizeof(COMMV8_colorSet), &mods);
						bPos = 5;
					}

					AddDataBlock(bPos, &mods, &(*nc));
					bPos += 15;
				}
				if (bPos > 5) {
					mods.push_back({ 4, cnt });
					val = PrepareAndSend(COMMV8_colorSet, sizeof(COMMV8_colorSet), &mods);
				}
			} break;
			case API_V5:
			{
				byte bPos = 4;
				vector<Afx_icommand> mods;
				for (auto nc = act->begin(); nc != act->end(); nc++) {
					if (bPos + 4 > length) {
						// Send command and clear buffer...
						val = PrepareAndSend(COMMV5_colorSet, sizeof(COMMV5_colorSet), &mods);
						bPos = 4;
					}

					mods.insert(mods.end(), {
								{bPos,(byte)(nc->index + 1)},
								{(byte)(bPos + 1),nc->act.front().r},
								{(byte)(bPos + 2),nc->act.front().g},
								{(byte)(bPos + 3),nc->act.front().b} });
					bPos += 4;
				}
				if (bPos > 4)
					val = PrepareAndSend(COMMV5_colorSet, sizeof(COMMV5_colorSet), &mods);
				Loop();
			} break;
			default:
			{
				for (auto nc = act->begin(); nc != act->end(); nc++)
					SetAction(&(*nc));
			} break;
			}
		}
		return val;
	}

	bool Functions::SetAction(Afx_lightblock *act, bool ipe) {
		bool res = false;
		vector<Afx_icommand> mods;
		if (act->act.size() > 1 || !ipe) {
			if (!inSet) Reset();
			switch (version) {
			case API_V8: {
				AddDataBlock(5, &mods, act);
				PrepareAndSend(COMMV8_readyToColor, sizeof(COMMV8_readyToColor));
				res = PrepareAndSend(COMMV8_colorSet, sizeof(COMMV8_colorSet), &mods);
			} break;
			case API_V7:
			{
				byte opType = 1;
				switch (act->act.front().type) {
				case AlienFX_A_Pulse: opType = 5; break;
				case AlienFX_A_Morph: opType = 3; break;
				case AlienFX_A_Breathing: opType = 2; break;
				case AlienFX_A_Spectrum: opType = 4; break;
				case AlienFX_A_Rainbow: opType = 6; break;
				}
				mods = {{5,opType},{6,bright},{7,act->index}};
				for (int ca = 0; ca < act->act.size(); ca++) {
					if (ca*3+10 < length)
						mods.insert(mods.end(), {
							{(byte)(ca*3+8), act->act[ca].r},
							{(byte)(ca*3+9), act->act[ca].g},
							{(byte)(ca*3+10), act->act[ca].b}});
				}
				res = PrepareAndSend(COMMV7_control, sizeof(COMMV7_control), &mods);
			} break;
			case API_V6:
				res = PrepareAndSend(COMMV6_colorSet, sizeof(COMMV6_colorSet), SetMaskAndColor(1 << act->index, act->act.front().type,
					{ act->act.front().b, act->act.front().g, act->act.front().r },
					{ act->act.back().b, act->act.back().g, act->act.back().r },
					act->act.front().tempo));
				break;
			case API_V4:
			{
				// SetPowerAction for power!
				if (ipe && act->act.front().type == AlienFX_A_Power) {
					vector<Afx_lightblock> t = { *act };
					return SetPowerAction(&t);
				}
				PrepareAndSend(COMMV4_colorSel, sizeof(COMMV4_colorSel), {{6,act->index}});
				byte bPos = 3;
				for (auto ca = act->act.begin(); ca != act->act.end(); ca++) {
					// 3 actions per record..
					byte opCode1 = 0xd0, opCode2 = ca->tempo;
					switch (ca->type) {
					case AlienFX_A_Pulse: case AlienFX_A_Breathing: opCode1 = 0xdc; break;
					case AlienFX_A_Morph: opCode1 = 0xcf; break;
					case AlienFX_A_Spectrum: opCode1 = 0x82; break;
					case AlienFX_A_Rainbow:	opCode1 = 0xac; break;
					case AlienFX_A_Power: opCode1 = 0xe8; break;
					default: opCode2 = 0xfa;
					}
					mods.insert(mods.end(), {
						{bPos,(byte)(ca->type < AlienFX_A_Breathing ? ca->type : AlienFX_A_Morph) },
								{(byte)(bPos + 1), ca->time},
								{(byte)(bPos + 2), opCode1},
								{(byte)(bPos + 4), opCode2},
								{(byte)(bPos + 5), ca->r},
								{(byte)(bPos + 6), ca->g},
								{(byte)(bPos + 7), ca->b}});
					bPos += 8;
					if (bPos + 8 >= length) {
						res = PrepareAndSend(COMMV4_colorSet, sizeof(COMMV4_colorSet), &mods);
						bPos = 3;
					}
				}
				if (bPos > 3) {
					res = PrepareAndSend(COMMV4_colorSet, sizeof(COMMV4_colorSet), &mods);
				}
			} break;
			case API_V3: case API_V2: case API_V1:
			{
				// SetPowerAction for power!
				if (act->act.front().type == AlienFX_A_Power) {
					vector<Afx_lightblock> t = { *act };
					return SetPowerAction(&t);
				}
				if (act->act.front().type != AlienFX_A_Color) {
					PrepareAndSend(COMMV1_setTempo, sizeof(COMMV1_setTempo),
						{{2,(byte) (((UINT) act->act.front().tempo << 3 & 0xff00) >> 8)},
							{3,(byte) ((UINT) act->act.front().tempo << 3 & 0xff)},
							{4,(byte) (((UINT) act->act.front().time << 5 & 0xff00) >> 8)},
							{5,(byte) ((UINT) act->act.front().time << 5 & 0xff)}});
				}
				for (auto ca = act->act.begin(); ca != act->act.end(); ca++) {
					Afx_colorcode c2{ 0 };
					byte actmode = 3;
					switch (ca->type) {
					case AlienFX_A_Morph: actmode = 1; break;
					case AlienFX_A_Pulse: actmode = 2; break;
					}
					if (act->act.size() > 1)
						c2 = (ca != act->act.end() - 1) ?
							Afx_colorcode{ (ca+1)->b, (ca+1)->g, (ca+1)->r } :
							Afx_colorcode{ act->act.front().b, act->act.front().g, act->act.front().r };
					res = PrepareAndSend(COMMV1_color, sizeof(COMMV1_color),
						SetMaskAndColor(1 << act->index, actmode, { ca->b, ca->g, ca->r }, c2));
				}
				Loop();
			} break;
			default: res = SetColor(act->index, {act->act.front().b, act->act.front().g, act->act.front().r});
			}
		} else
			if (act->act.size() == 1)
				res = SetColor(act->index, { act->act.front().b, act->act.front().g, act->act.front().r });
		return res;
	}

	bool Functions::SetPowerAction(vector<Afx_lightblock> *act, bool save) {
		Afx_lightblock* pwr = NULL;
		switch (version) {
		// ToDo - APIv8 profile save
		case API_V4:
		{
			UpdateColors();
			inSet = true;
			if (save) {
				PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { { 4, 4 }, { 6, 0x61 } });
				PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { { 4, 1 }, { 6, 0x61 } });
				for (auto ca = act->begin(); ca != act->end(); ca++)
					if (ca->act.front().type != AlienFX_A_Power)
						SetAction(&(*ca), false);
					//else
					//	pwr = &(*ca);
				PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { { 4, 2 }, { 6, 0x61 } });
				PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { { 4, 6 }, { 6, 0x61 } });
			}
			else {
				pwr = &act->front();
				// Now set power button....
				for (BYTE cid = 0x5b; cid < 0x61; cid++) {
					// Init query...
					PrepareAndSend(COMMV4_setPower, sizeof(COMMV4_setPower), { {6,cid},{4,4} });
					PrepareAndSend(COMMV4_setPower, sizeof(COMMV4_setPower), { {6,cid},{4,1} });
					// Now set color by type...
					Afx_lightblock tact{ pwr->index };
					switch (cid) {
					case 0x5b: // AC sleep
						tact.act.push_back(pwr->act.front());// afx_act{AlienFX_A_Power, 3, 0x64, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b});
						tact.act.push_back(Afx_action{ AlienFX_A_Power, 3, 0x64, 0, 0, 0 });
						break;
					case 0x5c: // AC power
						tact.act.push_back(pwr->act.front());// afx_act{AlienFX_A_Color, 0, 0, pwr->act.front().r, act[index].act.front().g, act[index].act.front().b});
						tact.act.front().type = AlienFX_A_Color;
						break;
					case 0x5d: // Charge
						tact.act.push_back(pwr->act.front());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.front().r, act[index].act.front().g, act[index].act.front().b});
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						break;
					case 0x5e: // Battery sleep
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.act.push_back(Afx_action{ AlienFX_A_Power, 3, 0x64, 0, 0, 0 });
						break;
					case 0x5f: // Batt power
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Color, 0, 0, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.act.front().type = AlienFX_A_Color;
						break;
					case 0x60: // Batt critical
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Color, 0, 0, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.act.front().type = AlienFX_A_Pulse;
					}
					SetAction(&tact, false);
					// And finish
					PrepareAndSend(COMMV4_setPower, sizeof(COMMV4_setPower), { {6,cid},{4,2} });
				}

				PrepareAndSend(COMMV4_control, sizeof(COMMV4_control), { { 4, 5 }/*, { 6, 0x61 }*/ });
				byte state = WaitForBusy();
#ifdef _DEBUG
				if (state)
					OutputDebugString("Power device ready timeout!\n");
#endif
				WaitForReady();
			}
			inSet = false;
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			if (!inSet) Reset();
			for (auto ca = act->begin(); ca != act->end(); ca++)
				if (ca->act.front().type != AlienFX_A_Power)
					SavePowerBlock(1, *ca, false);
				else
					pwr = &(*ca);
			//if ((pwr && act->size() > 1) || (!pwr && act->size()))
			//	PrepareAndSend(COMMV1_save, sizeof(COMMV1_save));
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
					SavePowerBlock(2, tact, true, true);
					// 08 05 - AC power
					tact = { pwr->index,
							{{AlienFX_A_Color, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}, {0}} };
					SavePowerBlock(5, tact, true);
					// 08 06 - charge
					tact = { pwr->index,
							{{AlienFX_A_Morph, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b},
							{0,0,0,pwr->act.back().r, pwr->act.back().g, pwr->act.back().b}} };
					SavePowerBlock(6, tact, false);
					tact = { pwr->index,
							{{AlienFX_A_Morph, 0 , 0, pwr->act.back().r, pwr->act.back().g, pwr->act.back().b},
							{0,0,0,pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}} };
					SavePowerBlock(6, tact, true);
					// 08 07 - Battery standby
					tact = { pwr->index,
							{{AlienFX_A_Morph, 0 , 0, pwr->act.back().r, pwr->act.back().g, pwr->act.back().b},	{2}} };
					SavePowerBlock(7, tact, true, true);
					// 08 08 - battery
					tact.act.front().type = AlienFX_A_Color;
					SavePowerBlock(8, tact, true);
					// 08 09 - battery critical
					tact.act.front().type = AlienFX_A_Pulse;
					SavePowerBlock(9, tact, true);
				}
				//int pind = powerMode ? 0 : 1;
				//SetColor(pwr->index, { pwr->act[pind].b, pwr->act[pind].g, pwr->act[pind].r });
			}
			if (act->size())
				PrepareAndSend(COMMV1_save, sizeof(COMMV1_save));
			if (pwr)
				SetColor(pwr->index, { pwr->act[!powerMode].b, pwr->act[!powerMode].g, pwr->act[!powerMode].r });
		} break;
		//default:
		//	// can't set action for other, just use color
		//	SetAction(&act->front());
		//	//SetColor(act->front().index, {act->front().act.front().b, act->act.front().g, act->act.front().r});
		}
		return true;
	}

	bool Functions::ToggleState(BYTE brightness, vector<Afx_light> *mappings, bool power) {

#ifdef _DEBUG
		char buff[2048];
		sprintf_s(buff, 2047, "State update: PID: %#x, brightness: %d, Power: %d\n",
			pid, brightness, power);
		OutputDebugString(buff);
#endif

		bright = ((UINT) brightness * 0x64) / 0xff;
		if (inSet) UpdateColors();
		switch (version) {
		case API_V8: {
			bright = brightness * 10 / 0xff;
			return PrepareAndSend(COMMV8_setBrightness, sizeof(COMMV8_setBrightness), { {2, bright} });
		} break;
		case API_V5:
		{
			Reset();
			PrepareAndSend(COMMV5_turnOnInit, sizeof(COMMV5_turnOnInit));
			PrepareAndSend(COMMV5_turnOnInit2, sizeof(COMMV5_turnOnInit2));
			return PrepareAndSend(COMMV5_turnOnSet, sizeof(COMMV5_turnOnSet), {{4,brightness}});
		} break;
		case API_V4:
		{
			vector<Afx_icommand> mods{{3,(byte)(0x64 - bright)}};
			byte pos = 6, pindex = 0;
			for (auto i = mappings->begin(); i < mappings->end(); i++)
				if (pos < length && (!i->flags || power)) {
					mods.push_back({pos,(byte)i->lightid});
					pos++; pindex++;
				}
			mods.push_back({5,pindex});
			return PrepareAndSend(COMMV4_turnOn, sizeof(COMMV4_turnOn), &mods);
		} break;
		case API_V3: case API_V1: case API_V2:
		{
			int res = PrepareAndSend(COMMV1_reset, sizeof(COMMV1_reset), {{2,(byte)(brightness? 4 : power? 3 : 1)}});
			WaitForReady();
			return res;
		} break;
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
		case API_V8: {
			PrepareAndSend(COMMV8_effectReady, sizeof(COMMV8_effectReady));
			return PrepareAndSend(COMMV8_effectSet, sizeof(COMMV8_effectSet),
				{	{3, effType},
				{4, act1.r}, {5, act1.g}, {6, act1.b},
				{7, act2.r}, {8, act2.g}, {9, act2.b},
				{10, tempo}, {13, mode}, {14, 2}
				});
		} break;
		case API_V5:
		{
			if (inSet)
				UpdateColors();
			Reset();
			if (effType < 2)
				PrepareAndSend(COMMV5_setEffect, sizeof(COMMV5_setEffect),
					{ {2,1}, {11,0xff}, {12,0xff}, {14,0xff}, {15,0xff}});
			else
				PrepareAndSend(COMMV5_setEffect, sizeof(COMMV5_setEffect),
					{ {2,effType}, {3,tempo},
						   {10,act1.r}, {11,act1.g}, {12,act1.b},
						   {13,act2.r}, {14,act2.g}, {15,act2.b},
						   {16,5}});
			if (effType < 2)
				PrepareAndSend(COMMV5_update, sizeof(COMMV5_update), { {3,0xfe}, {6,0xff}, {7,0xff} });
			else
				UpdateColors();
			return true;
		} break;
		}
		return false;
	}

	BYTE Functions::GetDeviceStatus() {

		byte buffer[MAX_BUFFERSIZE]{reportID};
		DWORD written;
		switch (version) {
		case API_V5:
		{
			PrepareAndSend(COMMV5_status, sizeof(COMMV5_status));
			buffer[1] = 0x93;
			//if (HidD_GetFeature(devHandle, buffer, length))
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_FEATURE, 0, 0, buffer, length, &written, NULL))
				return buffer[2];
		} break;
		case API_V4:
		{
			//if (HidD_GetInputReport(devHandle, buffer, length))
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
				return buffer[2];
#ifdef _DEBUG
			else {
				OutputDebugString(TEXT("System hangs!\n"));
			}
#endif
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			PrepareAndSend(COMMV1_status, sizeof(COMMV1_status));

			buffer[0] = 0x01;
			//HidD_GetInputReport(devHandle, buffer, length);
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
				return buffer[0] == 0x01 ? 0x06 : buffer[0];
		} break;
		}

		return 0;
	}

	BYTE Functions::WaitForReady() {
		byte status;// = GetDeviceStatus();
		switch (version) {
		case API_V3: case API_V2: case API_V1:
			for (int i = 0; i < 100 && ((status = GetDeviceStatus()) != ALIENFX_V2_READY || status != ALIENFX_V2_RESET); i++) {
				Sleep(5);
			}
			break;
		case API_V4:
			while (!(status = IsDeviceReady()))
				Sleep(20);
			break;
		default:
			status = GetDeviceStatus();
		}
		return status;
	}

	BYTE Functions::WaitForBusy() {
		byte status;// = GetDeviceStatus();
		switch (version) {
		case API_V3: case API_V2: case API_V1:
			for (int i = 0; i < 500 && ((status = GetDeviceStatus()) != ALIENFX_V2_BUSY || status != ALIENFX_V2_RESET); i++) {
				Sleep(10);
			}
			break;
		case API_V4:
			for (int i = 0; i < 500 && (status = IsDeviceReady()); i++)
				Sleep(20);
			break;
		default:
			status = GetDeviceStatus();
		}
		return status;
	}

	BYTE Functions::IsDeviceReady() {
		int status = GetDeviceStatus();
		switch (version) {
		case API_V5:
			return status != ALIENFX_V5_WAITUPDATE;
		case API_V4:
#ifdef _DEBUG
			status = status ? status == ALIENFX_V4_READY || status == ALIENFX_V4_WAITUPDATE || status == ALIENFX_V4_WASON : 0xff;
			if (!status)
				OutputDebugString("Device not ready!\n");
			return status;
#else
			return status ? status == ALIENFX_V4_READY || status == ALIENFX_V4_WAITUPDATE || status == ALIENFX_V4_WASON : 0xff;
#endif
		case API_V3: case API_V2: case API_V1:
			switch (status) {
			case ALIENFX_V2_READY:
				return 1;
			case ALIENFX_V2_BUSY:
				Reset();
				return WaitForReady() == ALIENFX_V2_READY;
			case ALIENFX_V2_RESET:
				return WaitForReady() == ALIENFX_V2_READY;
			}
			return 0;
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
			DWORD dw = 0;
			SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };
			while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData)) {
				dw++;
				DWORD dwRequiredSize = 0;
				SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL);
				std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new char[dwRequiredSize]);
				deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
				if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL)) {
					Functions* dev = new Functions();
					if (dev->AlienFXCheckDevice(deviceInterfaceDetailData->DevicePath) < 0)
						delete dev;
					else {
						devs.push_back(dev);
#ifdef _DEBUG
						char buff[2048];
						sprintf_s(buff, 2047, "Scan: VID: %#x, PID: %#x, Version: %d\n",
							dev->GetVID(), dev->GetPID(), dev->GetVersion());
						OutputDebugString(buff);
						printf("%s", buff);
#endif
					}
				}
				deviceInterfaceDetailData.release();
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

	void Mappings::AlienFXApplyDevices(vector<Functions*> devList, byte brightness, byte power) {
		activeLights = 0;
		activeDevices = (int)devList.size();
		// check old devices...
		for (auto i = fxdevs.begin(); i != fxdevs.end(); i++) {
			if (i->dev) {
				// is device still present?
				auto nDev = find_if(devList.begin(), devList.end(),
					[i](auto dev) {
						return dev->GetVID() == i->vid && dev->GetPID() == i->pid;
					});
				if (nDev == devList.end()) {
					// device not present
					delete i->dev;
					i->dev = NULL;
				}
				else {
					devList.erase(nDev);
					activeLights += (int)i->lights.size();
				}
			}
		}
		// add new devices...
		for (auto i = devList.begin(); i != devList.end(); i++) {
			Afx_device* dev = AddDeviceById((*i)->GetPID(), (*i)->GetVID());
			dev->dev = *i;
			dev->dev->ToggleState(brightness, &dev->lights, power);
			activeLights += (int)dev->lights.size();
		}
		devList.clear();
	}

	void Mappings::AlienFXAssignDevices(void* acc, byte brightness, byte power) {
		AlienFXApplyDevices(AlienFXEnumDevices(acc), brightness, power);
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

		groups.clear();
		grids.clear();

		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &mainKey, NULL);
		unsigned vindex;
		char kName[255], name[255];
		DWORD len, lend, dID;
		WORD lID, vid, pid;
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
			if (sscanf_s(kName, "Light%u-%hd", &dID, &lID) == 2) {
				RegGetValue(mainKey, kName, "Name", RRF_RT_REG_SZ, 0, name, &(lend = 255));
				RegGetValue(mainKey, kName, "Flags", RRF_RT_REG_DWORD, 0, &len, &(lend = sizeof(DWORD)));
				AddDeviceById(LOWORD(dID), HIWORD(dID))->lights.push_back({ lID, LOWORD(len), name });
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

		HKEY   hKey1, hKeyS;
		size_t numGroups = groups.size();
		size_t numGrids = grids.size();

		// Remove all maps!
		RegDeleteTree(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"));
		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, NULL);

		for (auto i = fxdevs.begin(); i < fxdevs.end(); i++) {
			// Saving device data..
			string name = "Dev#" + to_string(i->vid) + "_" + to_string(i->pid);
			RegSetValueEx( hKey1, name.c_str(), 0, REG_SZ, (BYTE *) i->name.c_str(), (DWORD) i->name.length() );
			name = "DevWhite#" + to_string(i->vid) + "_" + to_string(i->pid);
			RegSetValueEx( hKey1, name.c_str(), 0, REG_DWORD, (BYTE *) &i->white.ci, sizeof(DWORD));
			for (auto cl = i->lights.begin(); cl < i->lights.end(); cl++) {
				// Saving all lights from current device
				string name = "Light" + to_string(MAKELONG(i->pid, i->vid)) + "-" + to_string(cl->lightid);
				RegCreateKey(hKey1, name.c_str(), &hKeyS);
				RegSetValueEx(hKeyS, "Name", 0, REG_SZ, (BYTE*)cl->name.c_str(), (DWORD)cl->name.length());

				DWORD flags = cl->flags;
				RegSetValueEx(hKeyS, "Flags", 0, REG_DWORD, (BYTE*)&flags, sizeof(DWORD));
				RegCloseKey(hKeyS);
			}
		}

		for (int i = 0; i < numGroups; i++) {
			string name = "Group" + to_string(groups[i].gid);

			RegCreateKey(hKey1, name.c_str(), &hKeyS);
			RegSetValueEx(hKeyS, "Name", 0, REG_SZ, (BYTE *) groups[i].name.c_str(), (DWORD) groups[i].name.size());

			DWORD *grLights = new DWORD[groups[i].lights.size() * 2];

			for (int j = 0; j < groups[i].lights.size(); j++) {
				grLights[j * 2] = groups[i].lights[j].did;
				grLights[j * 2 + 1] = groups[i].lights[j].lid;
			}
			RegSetValueEx( hKeyS, "Lights", 0, REG_BINARY, (BYTE *) grLights, 2 * (DWORD) groups[i].lights.size() * sizeof(DWORD) );

			delete[] grLights;
			RegCloseKey(hKeyS);
		}

		for (int i = 0; i < numGrids; i++) {
			string name = "Grid" + to_string(grids[i].id);
			RegCreateKey(hKey1, name.c_str(), &hKeyS);
			RegSetValueEx(hKeyS, "Name", 0, REG_SZ, (BYTE*)grids[i].name.c_str(), (DWORD)grids[i].name.length());
			DWORD sizes = ((DWORD)grids[i].x << 8) | grids[i].y;
			RegSetValueEx(hKeyS, "Size", 0, REG_DWORD, (BYTE*)&sizes, sizeof(DWORD));
			RegSetValueEx(hKeyS, "Grid", 0, REG_BINARY, (BYTE*)grids[i].grid, grids[i].x * grids[i].y * sizeof(DWORD));
			RegCloseKey(hKeyS);
		}

		RegCloseKey(hKey1);
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

	// get PID for current device
	int Functions::GetPID() { return pid; };

	// get VID for current device
	int Functions::GetVID() { return vid; };

	// get API version for current device
	int Functions::GetVersion() { return version; };

	// get data length for current device
	int Functions::GetLength() { return length; };

	bool Functions::IsHaveGlobal()
	{
		switch (version) {
		case API_V5: case API_V8: return true;
		}
		return false;
	}
}