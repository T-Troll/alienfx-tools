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

	vector<icommand> *Functions::SetMaskAndColor(DWORD index, byte type, Colorcode c1, Colorcode c2, byte tempo, byte length) {
		vector<icommand> *mods = new vector<icommand>({{1, type},{2,(byte)chain},
										 {3,(byte)((index & 0xFF0000) >> 16)},
										 {4,(byte)((index & 0x00FF00) >> 8)},
										 {5,(byte)(index & 0x0000FF)}});
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
			mods->insert(mods->end(), { {9,(byte)index},
								 {10,c1.r},{11,c1.g},{12,c1.b}});
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

	inline bool Functions::PrepareAndSend(const byte* command, byte size, vector<icommand> mods) {
		return PrepareAndSend(command, size, &mods);
	}

	bool Functions::PrepareAndSend(const byte *command, byte size, vector<icommand> *mods) {
		byte buffer[MAX_BUFFERSIZE];
		DWORD written;

		FillMemory(buffer, MAX_BUFFERSIZE, version == API_V6 && size != 3 ? 0xff : 0);

		memcpy(&buffer[1], command, size);
		buffer[0] = reportID;

		if (mods) {
			for (auto i = mods->begin(); i < mods->end(); i++)
				buffer[i->i] = i->val;
			mods->clear();
		}

		switch (version) {
		case API_V1: case API_V2: case API_V3: case API_V4:
			return DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, 0, 0, &written, NULL);
			//return HidD_SetOutputReport(devHandle, buffer, length);
		case API_V5:
			return DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, 0, 0, &written, NULL);
			//return HidD_SetFeature(devHandle, buffer, length);
		case API_V6: {
			bool res = WriteFile(devHandle, buffer, length, &written, NULL);
			if (size == 3)
				ReadFile(devHandle, buffer, length, &written, NULL);
			return res;
		}
		case API_V7:
			WriteFile(devHandle, buffer, length, &written, NULL);
			return ReadFile(devHandle, buffer, length, &written, NULL);
		case API_V8: {
			if (size == 4) {
				//res = HidD_SetFeature(devHandle, buffer, length);
				bool res = DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, 0, 0, &written, NULL);
				Sleep(6); // Need wait for ACK
				return res;
			}
			else {
				return WriteFile(devHandle, buffer, length, &written, NULL);//res
			}
		}
		}
		return false;
	}

	//bool Functions::PrepareAndSend(const byte *command, byte size, vector<icommand> mods) {
	//	return PrepareAndSend(command, size, &mods);
	//}

	bool Functions::SavePowerBlock(byte blID, act_block act, bool needSave, bool needInverse) {
		byte mode;
		switch (act.act.front().type) {
		case AlienFX_A_Pulse:
			mode = 2; break;
		case AlienFX_A_Morph:
			mode = 1; break;
		default: mode = 3;
		}
		PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
		if (act.act.size() < 2)
			PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(1 << act.index, mode,
																			   {act.act.front().b, act.act.front().g, act.act.front().r}));
		else
			PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(1 << act.index, mode,
																			   {act.act.front().b, act.act.front().g, act.act.front().r},
																			   {act.act.back().b, act.act.back().g, act.act.back().r}));
		PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
		Loop();

		if (needInverse) {
			PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
			PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(~((1 << act.index)), act.act.back().type,
																			   {act.act.front().b, act.act.front().g, act.act.front().r},
																			   {act.act.back().b, act.act.back().g, act.act.back().r}));
			PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
			Loop();
		}

		if (needSave) {
			PrepareAndSend(COMMV1.save, sizeof(COMMV1.save));
			Reset();
		}

		return true;
	}

	int Functions::AlienFXCheckDevice(string devPath, int vidd, int pidd)
	{
		if ((devHandle = CreateFile(devPath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL)) != INVALID_HANDLE_VALUE) {
			PHIDP_PREPARSED_DATA prep_caps;
			HIDP_CAPS caps;
			std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES({ sizeof(HIDD_ATTRIBUTES) }));
			if (HidD_GetAttributes(devHandle, attributes.get()) &&
				(vidd == -1 || attributes->VendorID == vidd) &&
				(pidd == -1 || attributes->ProductID == pidd)) {
				HidD_GetPreparsedData(devHandle, &prep_caps);
				HidP_GetCaps(prep_caps, &caps);
				HidD_FreePreparsedData(prep_caps);
				length = caps.OutputReportByteLength;
				vid = attributes->VendorID;
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
						PrepareAndSend(COMMV6.systemReset, sizeof(COMMV6.systemReset));
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
				default: CloseHandle(devHandle); devHandle = NULL;
				}
				pid = version < 0 ? version : attributes->ProductID;
				return pid;
			}
			CloseHandle(devHandle);
			devHandle = NULL;
			attributes.release();
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
			version = pid = API_ACPI;
			device = new AlienFan_SDK::Lights(acc);
			if (Reset()) {
				return pid;
			}
			delete device;
		}
		return -1;
	}
#endif

	void Functions::Loop() {
		switch (version) {
		case API_V5:
			PrepareAndSend(COMMV5.loop, sizeof(COMMV5.loop));
			break;
		//case API_V4: {
		//	 //m15 require Input report as a confirmation, not output.
		//	 //WARNING!!! In latest firmware, this can provide up to 10sec(!) slowdown, so i disable status read. It works without it as well.
		//	HidD_SetInputReport(devHandle, buffer, length);
		//} break;
		case API_V3: case API_V2: case API_V1:
		{
			PrepareAndSend(COMMV1.loop, sizeof(COMMV1.loop));
			chain++;
		} break;
		}
	}

	bool Functions::Reset() {
		bool result = false;
		switch (version) {
		case API_V6:
			result = PrepareAndSend(COMMV6.colorReset, sizeof(COMMV6.colorReset));
			break;
		case API_V5:
		{
			result = PrepareAndSend(COMMV5.reset, sizeof(COMMV5.reset));
			AlienfxGetDeviceStatus();
		} break;
		case API_V4:
		{
			result = PrepareAndSend(COMMV4.reset, sizeof(COMMV4.reset));
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			result = PrepareAndSend(COMMV1.reset, sizeof(COMMV1.reset));
			AlienfxWaitForReady();
			chain = 1;
		} break;
#ifndef NOACPILIGHTS
		case API_ACPI:
			result = device->Reset();
			break;
#endif
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
				//PrepareAndSend(COMMV5.loop, sizeof(COMMV5.loop));
				res = PrepareAndSend(COMMV5.update, sizeof(COMMV5.update));
			} break;
			case API_V4:
			{
				res = PrepareAndSend(COMMV4.update, sizeof(COMMV4.update));
				Sleep(10);
			} break;
			case API_V3: case API_V2: case API_V1:
			{
				res = PrepareAndSend(COMMV1.update, sizeof(COMMV1.update));
				AlienfxWaitForBusy();
				chain = 1;
			} break;
#ifndef NOACPILIGHTS
			case API_ACPI:
				res = device->Update();
				break;
#endif
			default: res = true;
			}
			//std::cout << "Update!" << std::endl;
			inSet = false;
			Sleep(5); // Fix for ultra-fast updates, or next command will fail sometimes.
		}
		return res;

	}

	bool Functions::SetColor(unsigned index, Colorcode c, bool loop) {
		bool val = false;
		if (!inSet)
			Reset();
		switch (version) {
		case API_V8: {
			PrepareAndSend(COMMV8.readyToColor, sizeof(COMMV8.readyToColor));
			val = PrepareAndSend(COMMV8.colorSet, sizeof(COMMV8.colorSet), { {5,(byte)index},{11,c.r},{12,c.g},{13,c.b} });
		} break;
		case API_V7:
		{
			PrepareAndSend(COMMV7.status, sizeof(COMMV7.status));
			val = PrepareAndSend(COMMV7.control, sizeof(COMMV7.control), {{6,bright},{7,(byte)index},{8,c.r},{9,c.g},{10,c.b}});
		} break;
		case API_V6:
		{
			val = PrepareAndSend(COMMV6.colorSet, sizeof(COMMV6.colorSet), SetMaskAndColor(1<<index, AlienFX_A_Color, c));
		} break;
		case API_V5:
		{
			val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), {{4,(byte)(index+1)},{5,c.r},{6,c.g},{7,c.b}});
		} break;
		case API_V4:
		{
			PrepareAndSend(COMMV4.colorSel, sizeof(COMMV4.colorSel), {{6,(byte)index}});
			val = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), {{8,c.r}, {9,c.g}, {10,c.b}});
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			val = PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(1<<index, 3, c));
		} break;
#ifndef NOACPILIGHTS
		case API_ACPI:
			val = device->SetColor(1 << index, c.r, c.g, c.b);
			break;
#endif
		default: return false;
		}
		if (loop) Loop();
		return val;
	}

	bool Functions::SetMultiColor(vector<UCHAR> *lights, Colorcode c) {
		bool val = false;
		if (!inSet) Reset();
		switch (version) {
		case API_V8: {
			vector<icommand> mods;
			byte bPos = 5, cnt = 1;
			PrepareAndSend(COMMV8.readyToColor, sizeof(COMMV8.readyToColor), { {2,(byte)lights->size()} });
			for (auto nc = lights->begin(); nc != lights->end(); nc++) {
				if (bPos + 15 > length) {
					// Send command and clear buffer...
					mods.push_back({ 4, cnt++ });
					val = PrepareAndSend(COMMV8.colorSet, sizeof(COMMV8.colorSet), &mods);
					bPos = 5;
				}

				mods.insert(mods.end(), { {bPos, *nc},{(byte)(bPos + 1),0x81},
					{ (byte)(bPos + 3), 0xa5}, {(byte)(bPos + 5), 0xa},
					{ (byte)(bPos + 6),c.r},{ (byte)(bPos + 7),c.g},{ (byte)(bPos + 8),c.b} });
				bPos += 15;
			}
			if (bPos > 5) {
				mods.push_back({ 4, cnt });
				val = PrepareAndSend(COMMV8.colorSet, sizeof(COMMV8.colorSet), &mods);
			}
		} break;
		case API_V6:
		{
			DWORD fmask = 0;
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				fmask |= 1 << (*nc);
			val = PrepareAndSend(COMMV6.colorSet, sizeof(COMMV6.colorSet), SetMaskAndColor(fmask, AlienFX_A_Color, c));
		} break;
		case API_V5:
		{
			vector<icommand> mods;
			byte bPos = 4;
			for (auto nc = lights->begin(); nc < lights->end(); nc++) {
				if (bPos + 4 > length) {
					// Send command and clear buffer...
					val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), &mods);
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
				val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), &mods);
			Loop();
		} break;
		case API_V4:
		{
			vector<icommand> mods;
			mods.push_back({5,(byte)lights->size()});
			for (int nc = 0; nc < lights->size(); nc++)
				mods.push_back({ (byte)(6 + nc), lights->at(nc)});
			PrepareAndSend(COMMV4.colorSel, sizeof(COMMV4.colorSel), &mods);
			val = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), {{8,c.r}, {9,c.g}, {10,c.b}});
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			DWORD fmask = 0;
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				fmask |= 1 << (*nc);
			val = PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(fmask, 3, c));
			Loop();
		} break;
#ifndef NOACPILIGHTS
		case API_ACPI:
		{
			DWORD fmask = 0;
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				fmask |= 1 << (*nc);
			val = device->SetColor((byte)fmask, c.r, c.g, c.b);
		} break;
#endif
		default:
		{
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				val = SetColor(*nc, c);
		}
		}
		return val;
	}

	bool Functions::SetMultiAction(vector<act_block> *act, bool save) {

		bool val = true;
		if (!inSet) Reset();
		switch (version) {
		case API_V8: {
			if (save)
				break;
			byte bPos = 5, cnt = 1;
			vector<icommand> mods;
			PrepareAndSend(COMMV8.readyToColor, sizeof(COMMV8.readyToColor), { {2,(byte)act->size()} });
			for (auto nc = act->begin(); nc != act->end(); nc++) {
				byte opType = 0x81;
				if (bPos + 15 > length) {
					// Send command and clear buffer...
					mods.push_back({ 4, cnt++ });
					val = PrepareAndSend(COMMV8.colorSet, sizeof(COMMV8.colorSet), &mods);
					bPos = 5;
				}

				switch (nc->act.front().type) {
				case AlienFX_A_Pulse: opType = 0x82; break;
				case AlienFX_A_Morph: opType = 0x83; break;
				case AlienFX_A_Breathing: opType = 0x87; break;
				case AlienFX_A_Spectrum: opType = 0x88; break;
				//case AlienFX_A_Rainbow: opType = 0x81; break; // DEBUG, check for OFF
				}
				mods.insert(mods.end(), { {bPos,nc->index},{(byte)(bPos + 1),opType},{(byte)(bPos + 2),nc->act.front().tempo},
					{ (byte)(bPos + 3), 0xa5}, {(byte)(bPos + 4),nc->act.front().time }, {(byte)(bPos + 5), 0xa},
					{ (byte)(bPos + 6), nc->act.front().r},{ (byte)(bPos + 7),nc->act.front().g},{ (byte)(bPos + 8),nc->act.front().b} });
				if (nc->act.size() > 1)
					// add second color if present
					mods.insert(mods.end(), { { (byte)(bPos + 9),nc->act.back().r }, { (byte)(bPos + 10),nc->act.back().g }, { (byte)(bPos + 11),nc->act.back().b } });

				bPos += 15;
			}
			if (bPos > 5) {
				mods.push_back({ 4, cnt });
				val = PrepareAndSend(COMMV8.colorSet, sizeof(COMMV8.colorSet), &mods);
			}
		} break;
		case API_V7:
		{
			if (save)
				SetPowerAction(act);
			else
				for (auto nc = act->begin(); nc != act->end(); nc++)
						val = SetAction(&(*nc));
		} break;
		case API_V5:
		{
			byte bPos = 4;
			vector<icommand> mods;
			for (auto nc = act->begin(); nc != act->end(); nc++) {
				if (bPos + 4 > length) {
					// Send command and clear buffer...
					val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), &mods);
					bPos = 4;
				}
				//if (bPos < length) {
					mods.insert(mods.end(), {
								{bPos,(byte)(nc->index + 1)},
								{(byte)(bPos + 1),nc->act.front().r},
								{(byte)(bPos + 2),nc->act.front().g},
								{(byte)(bPos + 3),nc->act.front().b} });
					bPos += 4;
				//}
				//else {
				//	// Send command and clear buffer...
				//	val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), &mods);
				//	bPos = 4;
				//	nc--;
				//}
			}
			if (bPos > 4)
				val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), &mods);
			Loop();
		}break;
		case API_V1: case API_V2: case API_V3: case API_V4:
		{
			if (save)
				SetPowerAction(act);
			else
				for (auto nc = act->begin(); nc != act->end(); nc++)
					if (nc->act.front().type != AlienFX_A_Power)
						val = SetAction(&(*nc));
					else {
						vector<act_block> tact{{*nc}};
						val = SetPowerAction(&tact);
					}
		} break;
		default:
		{
			for (auto nc = act->begin(); nc != act->end(); nc++) {
				val = SetAction(&(*nc));
			}
		} break;
		}
		return val;
	}

	bool Functions::SetAction(act_block *act) {
		bool res = false;
		vector<icommand> mods;
		if (act->act.size()) {
			if (!inSet) Reset();
			switch (version) {
			case API_V8: {
				byte opType = 0x81;
				switch (act->act.front().type) {
				case AlienFX_A_Pulse: opType = 0x82; break;
				case AlienFX_A_Morph: opType = 0x83; break;
				case AlienFX_A_Breathing: opType = 0x87; break;
				case AlienFX_A_Spectrum: opType = 0x88; break;
				//case AlienFX_A_Rainbow: opType = 0x88; break;
				}
				mods = { {5,act->index},{6,opType},{7,act->act.front().tempo},
					{11,act->act.front().r},{12,act->act.front().g},{13,act->act.front().b} };
				// add second color if present
				if (act->act.size() > 1) {
					mods.insert(mods.end(), {
						{ 14,act->act.back().r },
						{ 15,act->act.back().g },
						{ 16,act->act.back().b },
						{ 18,2 } });
				}
				PrepareAndSend(COMMV8.readyToColor, sizeof(COMMV8.readyToColor));
				res = PrepareAndSend(COMMV8.colorSet, sizeof(COMMV8.colorSet), &mods);
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
					mods.insert(mods.end(), {
						{(byte)(ca*3+8), act->act[ca].r},
						{(byte)(ca*3+9), act->act[ca].g},
						{(byte)(ca*3+10), act->act[ca].b}});
				}
				res =PrepareAndSend(COMMV7.control, sizeof(COMMV7.control), &mods);
			} break;
			case API_V6:
				res = PrepareAndSend(COMMV6.colorSet, sizeof(COMMV6.colorSet), SetMaskAndColor(1 << act->index, act->act.front().type,
					{ act->act.front().b, act->act.front().g, act->act.front().r },
					{ act->act.back().b, act->act.back().g, act->act.back().r },
					act->act.front().tempo, act->act.front().time));
				break;
			case API_V4:
			{
				PrepareAndSend(COMMV4.colorSel, sizeof(COMMV4.colorSel), {{6,act->index}});
				byte bPos = 3;
				for (int ca = 0; ca < act->act.size(); ca++) {
					// 3 actions per record..
					byte opCode1 = 0xd0, opCode2 = act->act[ca].tempo;
					switch (act->act[ca].type) {
					case AlienFX_A_Pulse:
						opCode1 = 0xdc;
						break;
					case AlienFX_A_Morph:
						opCode1 = 0xcf;
						break;
					case AlienFX_A_Breathing:
						opCode1 = 0xdc;
						break;
					case AlienFX_A_Spectrum:
						opCode1 = 0x82;
						break;
					case AlienFX_A_Rainbow:
						opCode1 = 0xac;
						break;
					case AlienFX_A_Power:
						opCode1 = 0xe8;
						break;
					default:
						opCode2 = 0xfa;
					}
					mods.insert(mods.end(), {
						{bPos,(byte)(act->act[ca].type < AlienFX_A_Breathing ? act->act[ca].type : AlienFX_A_Morph) },
								{(byte)(bPos + 1), act->act[ca].time},
								{(byte)(bPos + 2), opCode1},
								{(byte)(bPos + 4), opCode2},
								{(byte)(bPos + 5), act->act[ca].r},
								{(byte)(bPos + 6), act->act[ca].g},
								{(byte)(bPos + 7), act->act[ca].b}});
					bPos += 8;
					if (bPos + 8 >= length) {
						res = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), &mods);
						bPos = 3;
					}
				}
				if (bPos > 3) {
					res = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), &mods);
				}
				return res;
			} break;
			case API_V3: case API_V2: case API_V1:
			{
				if (act->act.front().type != AlienFX_A_Color) {
					PrepareAndSend(COMMV1.setTempo, sizeof(COMMV1.setTempo),
						{{2,(byte) (((UINT) act->act.front().tempo << 3 & 0xff00) >> 8)},
								   {3,(byte) ((UINT) act->act.front().tempo << 3 & 0xff)},
								   {4,(byte) (((UINT) act->act.front().time << 5 & 0xff00) >> 8)},
								   {5,(byte) ((UINT) act->act.front().time << 5 & 0xff)}});
				}
				for (size_t ca = 0; ca < act->act.size(); ca++) {
					vector<icommand> *mods;
					switch (act->act[ca].type) {
					case AlienFX_A_Pulse:
					{
						if (act->act.size() == 1)
							mods = SetMaskAndColor(1 << act->index, 2, {act->act[ca].b, act->act[ca].g, act->act[ca].r});
						else
							if (ca < act->act.size() - 1) {
								mods = SetMaskAndColor(1 << act->index, 2, {act->act[ca].b, act->act[ca].g, act->act[ca].r},
													   {act->act[ca + 1].b, act->act[ca + 1].g, act->act[ca + 1].r});

							} else {
								mods = SetMaskAndColor(1 << act->index, 2, {act->act[ca].b, act->act[ca].g, act->act[ca].r},
													   {act->act.front().b, act->act.front().g, act->act.front().r});
							}
					} break;
					case AlienFX_A_Morph:
					{
						if (ca < act->act.size() - 1) {
							mods = SetMaskAndColor(1 << act->index, 1, {act->act[ca].b, act->act[ca].g, act->act[ca].r},
												   {act->act[ca + 1].b, act->act[ca + 1].g, act->act[ca + 1].r});

						} else {
							mods = SetMaskAndColor(1 << act->index, 1, {act->act[ca].b, act->act[ca].g, act->act[ca].r},
												   {act->act.front().b, act->act.front().g, act->act.front().r});
						}
					} break;
					default:
					{ //case AlienFX_A_Color:
						mods = SetMaskAndColor(1 << act->index, 3, {act->act[ca].b, act->act[ca].g, act->act[ca].r});
					} //break;
					}
					res = PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), mods);
				}
				Loop();
			} break;
			default: res = SetColor(act->index, {act->act.front().b, act->act.front().g, act->act.front().r});
			}
		}
		return res;
	}

	bool Functions::SetPowerAction(vector<act_block> *act) {
		// Let's find power button (if any)
		act_block *pwr = NULL;
		for (vector<act_block>::iterator pwi = act->begin(); pwi != act->end(); pwi++)
			if (pwi->act.front().type == AlienFX_A_Power) {
				pwr = &(*pwi);
				break;
			}
		switch (version) {
		//case API_L_V9:
		//	PrepareAndSend(COMMV8.resetLow, sizeof(COMMV8.resetLow));
		//	PrepareAndSend(COMMV8.resetHigh, sizeof(COMMV8.resetHigh));
		//	break;
		case API_V7:
		{
			for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++)
					SetAction(&(*nc));
			PrepareAndSend(COMMV7.update, sizeof(COMMV7.update));
		} break;
		case API_V4:
		{
			// Need to flush query...
			if (inSet) UpdateColors();
			inSet = true;
			// Now set....
			if (pwr) {
				for (BYTE cid = 0x5b; cid < 0x61; cid++) {
					// Init query...
					PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,cid},{4,4}});
					PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,cid},{4,1}});
					// Now set color by type...
					act_block tact{pwr->index};
					switch (cid) {
					case 0x5b: // Alarm
						tact.act.push_back(pwr->act.front());// afx_act{AlienFX_A_Power, 3, 0x64, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b});
						tact.act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, 0, 0, 0});
						break;
					case 0x5c: // AC power
						tact.act.push_back(pwr->act.front());// afx_act{AlienFX_A_Color, 0, 0, pwr->act.front().r, act[index].act.front().g, act[index].act.front().b});
						tact.act.front().type = AlienFX_A_Color;
						break;
					case 0x5d: // Charge
						tact.act.push_back(pwr->act.front());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.front().r, act[index].act.front().g, act[index].act.front().b});
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						break;
					case 0x5e: // Low batt
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, 0, 0, 0});
						break;
					case 0x5f: // Batt power
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Color, 0, 0, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.act.front().type = AlienFX_A_Color;
						break;
					case 0x60: // System sleep?
						tact.act.push_back(pwr->act.back());// afx_act{AlienFX_A_Color, 0, 0, act[index].act.back().r, act[index].act.back().g, act[index].act.back().b});
						tact.act.front().type = AlienFX_A_Color;
					}
					SetAction(&tact);
					// And finish
					PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,cid},{4,2}});
				}
			}
			// Now (default) color set, if needed...
			if (!pwr || act->size() > 1) {
				PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,0x61},{4,4}});
				PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,0x61},{4,1}});
				// Default color set here...
				for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++)
					if (nc->act.front().type != AlienFX_A_Power) {
						SetAction(&(*nc));
					}
				PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), { {6,0x61},{4,2} });
				PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), { {6,0x61},{4,6} });
			}
			UpdateColors();

			if (pwr) {
				int count = 0;
				while ((signed char)IsDeviceReady() > 0 && count++ < 20) {
					Sleep(50);
				}
#ifdef _DEBUG
				if (count == 20)
					OutputDebugString("Power device ready timeout!\n");
#endif
			}
			while (!IsDeviceReady()) Sleep(50);
			Reset();
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			if (!inSet) Reset();

			// 08 01 - load on boot
			for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++)
				if (nc->act.front().type != AlienFX_A_Power)
					SavePowerBlock(1, *nc, false);

			if (!pwr || act->size() > 1) {
				PrepareAndSend(COMMV1.save, sizeof(COMMV1.save));
				Reset();
			}

			if (pwr) {
				chain = 1;
				// 08 02 - AC standby
				act_block tact{pwr->index,
							   {{AlienFX_A_Morph, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}, {2}}};
				/*SavePowerBlock(2, tact, false);*/
				//tact = {pwr->index,
				//	   {{AlienFX_A_Morph, 0 , 0, 0, 0, 0},
				//	   {2,0,0,pwr->act.back().r, pwr->act.back().g, pwr->act.back().b}}};
				SavePowerBlock(2, tact, true, true);
				// 08 05 - AC power
				tact = {pwr->index,
						{{AlienFX_A_Color, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}, {0}}};
				SavePowerBlock(5, tact, true);
				// 08 06 - charge
				tact = {pwr->index,
						{{AlienFX_A_Morph, 0 , 0, pwr->act.front().r, pwr->act.front().g, pwr->act.front().b},
						{0,0,0,pwr->act.back().r, pwr->act.back().g, pwr->act.back().b}}};
				SavePowerBlock(6, tact, false);
				tact = {pwr->index,
						{{AlienFX_A_Morph, 0 , 0, pwr->act.back().r, pwr->act.back().g, pwr->act.back().b},
						{0,0,0,pwr->act.front().r, pwr->act.front().g, pwr->act.front().b}}};
				SavePowerBlock(6, tact, true);
				// 08 07 - Battery standby
				tact = {pwr->index,
						{{AlienFX_A_Morph, 0 , 0, pwr->act.back().r, pwr->act.back().g, pwr->act.back().b},
						{2}}};
				SavePowerBlock(7, tact, true, true);
				// 08 08 - battery
				tact.act.front().type = AlienFX_A_Color;
				SavePowerBlock(8, tact, true);
				// 08 09 - battery critical
				tact.act.front().type = AlienFX_A_Pulse;
				SavePowerBlock(9, tact, true);
			}
			// fix for massive light change
			for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++)
				if (nc->act.front().type != AlienFX_A_Power) {
					SetAction(&(*nc));
				}

			if (pwr && act->size() == 1) {
				// Fix for immediate power button change
				int pind = powerMode ? 0 : 1;
				SetColor(pwr->index, {pwr->act[pind].b, pwr->act[pind].g, pwr->act[pind].r});
			}
			//else {
			//	PrepareAndSend(COMMV1.apply, sizeof(COMMV1.apply));
			//}
			UpdateColors();
		} break;
		default:
			// can't set action for other, just use color
			if (pwr)
				SetColor(pwr->index, {pwr->act.front().b, pwr->act.front().g, pwr->act.front().r});
		}
		return true;
	}

	bool Functions::ToggleState(BYTE brightness, vector<mapping> *mappings, bool power) {

#ifdef _DEBUG
		char buff[2048];
		sprintf_s(buff, 2047, "State update: PID: %#x, brightness: %d, Power: %d\n",
			pid, brightness, power);
		OutputDebugString(buff);
#endif

		bright = ((UINT) brightness * 0x64) / 0xff;
		switch (version) {
		case API_V8: {
			byte br = brightness * 10 / 255;
			return PrepareAndSend(COMMV8.setBrightness, sizeof(COMMV8.setBrightness), { {2, br} });
		} break;
		// This is not used in GUI, and don't work correctly in CLI
		/*case API_V7: case API_ACPI:
			if (!brightness)
				for (auto i = mappings->begin(); i < mappings->end(); i++) {
					if (!i->flags || power)
						SetColor(i->lightid, {0});
				}
			break;
		case API_V6:
		{
			vector<icommand> mods{ {9,0xf}, {13,bright}, {14,7} };
			PrepareAndSend(COMMV6.colorSet, sizeof(COMMV6.colorSet), &mods);
		} break;*/
		case API_V5:
		{
			if (inSet) {
				UpdateColors();
				Reset();
			}
			PrepareAndSend(COMMV5.turnOnInit, sizeof(COMMV5.turnOnInit));
			PrepareAndSend(COMMV5.turnOnInit2, sizeof(COMMV5.turnOnInit2));
			return PrepareAndSend(COMMV5.turnOnSet, sizeof(COMMV5.turnOnSet), {{4,brightness}});
		} break;
		case API_V4:
		{
			if (inSet) UpdateColors();
			PrepareAndSend(COMMV4.prepareTurn, sizeof(COMMV4.prepareTurn));
			vector<icommand> mods{{3,(byte)(0x64 - bright)}};
			byte pos = 6, pindex = 0;
			for (auto i = mappings->begin(); i < mappings->end(); i++)
				if (pos < length && (!i->flags || power)) {
					mods.push_back({pos,(byte)i->lightid});
					pos++; pindex++;
				}
			mods.push_back({5,pindex});
			return PrepareAndSend(COMMV4.turnOn, sizeof(COMMV4.turnOn), &mods);
		} break;
		case API_V3: case API_V1: case API_V2:
		{
			int res = PrepareAndSend(COMMV1.reset, sizeof(COMMV1.reset), {{2,(byte)(brightness? 4 : power? 3 : 1)}});
			AlienfxWaitForReady();
			return res;
		} break;
		}
		return false;
	}

	bool Functions::SetGlobalEffects(byte effType, byte mode, byte tempo, afx_act act1, afx_act act2) {
		vector<icommand> mods;
		switch (version) {
		case API_V8: {
			PrepareAndSend(COMMV8.effectReady, sizeof(COMMV8.effectReady));
			return PrepareAndSend(COMMV8.effectSet, sizeof(COMMV8.effectSet),
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
				PrepareAndSend(COMMV5.setEffect, sizeof(COMMV5.setEffect),
					{ {2,1}, {11,0xff}, {12,0xff}, {14,0xff}, {15,0xff}});
			else
				PrepareAndSend(COMMV5.setEffect, sizeof(COMMV5.setEffect),
					{ {2,effType}, {3,tempo},
						   {10,act1.r}, {11,act1.g}, {12,act1.b},
						   {13,act2.r}, {14,act2.g}, {15,act2.b},
						   {16,5}});
			if (effType < 2)
				PrepareAndSend(COMMV5.update, sizeof(COMMV5.update), { {3,0xfe}, {6,0xff}, {7,0xff} });
			else
				UpdateColors();
			return true;
		} break;
		//default: return true;
		}
		return false;
	}

	BYTE Functions::AlienfxGetDeviceStatus() {
		byte ret = 0;
		byte buffer[MAX_BUFFERSIZE]{reportID};
		DWORD written;
		switch (version) {
		case API_V5:
		{
			PrepareAndSend(COMMV5.status, sizeof(COMMV5.status));
			buffer[1] = 0x93;
			//if (HidD_GetFeature(devHandle, buffer, length))
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_FEATURE, 0, 0, buffer, length, &written, NULL))
				ret = buffer[2];
		} break;
		case API_V4:
		{
			//if (HidD_GetInputReport(devHandle, buffer, length))
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
				ret = buffer[2];
#ifdef _DEBUG
			else {
				OutputDebugString(TEXT("System hangs!\n"));
			}
#endif
		} break;
		case API_V3: case API_V2: case API_V1:
		{
			PrepareAndSend(COMMV1.status, sizeof(COMMV1.status));

			buffer[0] = 0x01;
			//HidD_GetInputReport(devHandle, buffer, length);
			if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
				if (buffer[0] == 0x01)
					ret = 0x06;
				else ret = buffer[0];
		} break;
		}

		return ret;
	}

	BYTE Functions::AlienfxWaitForReady() {
		byte status = AlienfxGetDeviceStatus();
		int i;
		for (i = 0; i < 100 && (status = AlienfxGetDeviceStatus()) != ALIENFX_V2_READY; i++) {
			if (status == ALIENFX_V2_RESET)
				return status;
			Sleep(5);
		}
		//OutputDebugString((wstring(L"AWFR count - ") + to_wstring(i) + L"\n").c_str());
		return status;
	}

	BYTE Functions::AlienfxWaitForBusy() {
		byte status = AlienfxGetDeviceStatus();
		int i;
		for (i = 0; i < 500 && (status = AlienfxGetDeviceStatus()) != ALIENFX_V2_BUSY; i++) {
			if (status == ALIENFX_V2_RESET)
				return status;
			Sleep(10);
		}
		return status;
	}

	BYTE Functions::IsDeviceReady() {
		int status = AlienfxGetDeviceStatus();
		switch (version) {
		case API_V5:
			return status != ALIENFX_V5_WAITUPDATE;
		case API_V4:
			return status ? status == ALIENFX_V4_READY || status == ALIENFX_V4_WAITUPDATE || status == ALIENFX_V4_WASON : 0xff;
		case API_V3: case API_V2: case API_V1:
			switch (status) {
			case ALIENFX_V2_READY:
				return 1;
			case ALIENFX_V2_BUSY:
				Reset();
				return AlienfxWaitForReady() == ALIENFX_V2_READY;
			case ALIENFX_V2_RESET:
				return AlienfxWaitForReady() == ALIENFX_V2_READY;
			}
			return 0;
		default:
			return !inSet;
		}
	}

	Functions::~Functions() {
		AlienFXClose();
	}

	void Functions::AlienFXClose() {
		if (length != API_ACPI && devHandle != NULL) {
			CloseHandle(devHandle);
			devHandle = NULL;
		}
	}

	/*bool Functions::AlienFXChangeDevice(int nvid, int npid, HANDLE acc) {
		int res;
		if (pid != (-1) && length != API_ACPI && devHandle != NULL)
			CloseHandle(devHandle);
		if (nvid == API_ACPI)
			res = AlienFXInitialize(acc);
		else
			res = AlienFXInitialize(nvid, npid);
		if (res != (-1)) {
			pid = npid;
			Reset();
			return true;
		}
		return false;
	}*/

	Mappings::~Mappings() {
		for (auto i = fxdevs.begin(); i < fxdevs.end(); i++) {
			if (i->dev) {
				delete i->dev;
			}
		}
		fxdevs.clear();
		groups.clear();
	}

	vector<Functions*> Mappings::AlienFXEnumDevices() {
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
		return devs;
	}

	void Mappings::AlienFXAssignDevices(void* acc, byte brightness, byte power) {
		vector<Functions*> devList = AlienFXEnumDevices();

		for (int i = 0; i < fxdevs.size(); i++)
			if (fxdevs[i].dev) {
				delete fxdevs[i].dev;
				fxdevs[i].dev = NULL;
			}

		activeLights = 0;
		// check/add devices...
		for (int i = 0; i < devList.size(); i++) {
			afx_device* dev = AddDeviceById(MAKELPARAM(devList[i]->GetPID(), devList[i]->GetVID()));
			dev->dev = devList[i];
			dev->dev->ToggleState(brightness, &dev->lights, power);
			activeLights += (int)dev->lights.size();
		}
		// add ACPI, if any
#ifndef NOACPILIGHTS
		if (acc) {
			Functions* devc = new AlienFX_SDK::Functions();
			if (devc->AlienFXInitialize((AlienFan_SDK::Control*)acc) > 0) {
				afx_device* dev = AddDeviceById(MAKELPARAM(API_ACPI, 0));
				dev->dev = devc;
				dev->dev->ToggleState(brightness, &dev->lights, power);
				activeLights += (int)dev->lights.size();
			}
			else
				delete devc;
		}
#endif
	}

	afx_device* Mappings::GetDeviceById(DWORD devID) {
		for (auto pos = fxdevs.begin(); pos < fxdevs.end(); pos++)
			if (pos->pid == LOWORD(devID) && (!HIWORD(devID) || pos->vid == HIWORD(devID))) {
				return &(*pos);
			}
		return nullptr;
	}

	lightgrid* Mappings::GetGridByID(byte id)
	{
		for (auto pos = grids.begin(); pos < grids.end(); pos++)
			if (pos->id == id)
				return &(*pos);
		return nullptr;
	}

	afx_device* Mappings::AddDeviceById(DWORD devID)
	{
		afx_device* dev = GetDeviceById(devID);
		if (!dev) {
			fxdevs.push_back({ HIWORD(devID), LOWORD(devID), NULL });
			dev = &fxdevs.back();
		}
		return dev;
	}

	mapping *Mappings::GetMappingById(afx_device* dev, WORD LightID) {
		if (dev) {
			for (auto pos = dev->lights.begin(); pos < dev->lights.end(); pos++)
				if (pos->lightid == LightID)
					return &(*pos);
		}
		return nullptr;
	}

	vector<group> *Mappings::GetGroups() {
		return &groups;
	}

	void Mappings::AddMappingByDev(afx_device* dev, WORD lightID, const char *name, WORD flags) {
		if (dev) {
			mapping* map;
			if (!(map = GetMappingById(dev, lightID))) {
				dev->lights.push_back({ lightID });
				map = &dev->lights.back();
			}
			map->name = name;
			map->flags = flags;
		}
	}

	void Mappings::AddMapping(DWORD devID, WORD lightID, const char* name, WORD flags) {
		afx_device* dev = AddDeviceById(devID);
		mapping* map;
		if (!(map = GetMappingById(dev, lightID))) {
			dev->lights.push_back({ lightID });
			map = &dev->lights.back();
		}
		map->name = name;
		map->flags = flags;
	}

	void Mappings::RemoveMapping(afx_device* dev, WORD lightID)
	{
		if (dev) {
			auto del_map = find_if(dev->lights.begin(), dev->lights.end(),
				[lightID](auto t) {
					return t.lightid == lightID;
				});
			if (del_map != dev->lights.end()) {
				dev->lights.erase(del_map);
			}
		}
	}

	group *Mappings::GetGroupById(DWORD gID) {
		auto pos = find_if(groups.begin(), groups.end(),
			[gID](group t) {
				return t.gid == gID;
			});
		if (pos != groups.end())
			return &(*pos);
		return nullptr;
	}


	void Mappings::LoadMappings() {
		HKEY   hKey1;

		groups.clear();
		grids.clear();

		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, NULL);
		unsigned vindex; mapping map; afx_device* dev;
		char kName[255], name[255];
		DWORD len = 255, lend = 255, dID;
		WORD lID = 0, vid, pid;
		for (vindex = 0; RegEnumValue(hKey1, vindex, kName, &len, NULL, NULL, (LPBYTE) name, &lend) == ERROR_SUCCESS; vindex++) {
			if (sscanf_s(kName, "Dev#%hd_%hd", &vid, &pid) == 2) {
				dev = AddDeviceById(MAKELPARAM(pid, vid));
				dev->name = string(name);
			}
			if (sscanf_s(kName, "DevWhite#%hd_%hd", &vid, &pid) == 2) {
				dev = AddDeviceById(MAKELPARAM(pid, vid));
				dev->white.ci = ((DWORD *) name)[0];
			}
			len = 255, lend = 255;
		}
		for (vindex = 0; RegEnumKey(hKey1, vindex, kName, 255) == ERROR_SUCCESS; vindex++) {
			if (sscanf_s(kName, "Light%d-%hd", &dID, &lID) == 2) {
				DWORD nameLen = 255, flags;
				RegGetValue(hKey1, kName, "Name", RRF_RT_REG_SZ, 0, name, &nameLen);
				nameLen = sizeof(DWORD);
				RegGetValue(hKey1, kName, "Flags", RRF_RT_REG_DWORD, 0, &flags, &nameLen);
				AddMapping(dID, lID, name, LOWORD(flags));
			}
		}
		for (vindex = 0; RegEnumKey(hKey1, vindex, kName, 255) == ERROR_SUCCESS; vindex++)  {
			if (sscanf_s((char *) kName, "Group%d", &dID) == 1) {
				DWORD nameLen = 255, *maps;
				RegGetValue(hKey1, kName, "Name", RRF_RT_REG_SZ, 0, name, &nameLen);
				nameLen = 0;
				RegGetValue(hKey1, kName, "Lights", RRF_RT_REG_BINARY, 0, NULL, &nameLen);
				maps = new DWORD[nameLen / sizeof(DWORD)];
				RegGetValue(hKey1, kName, "Lights", RRF_RT_REG_BINARY, 0, maps, &nameLen);
				groups.push_back({dID, name});
				for (int i = 0; i < nameLen / sizeof(DWORD); i += 2) {
					dev = GetDeviceById(maps[i]);
					if (dev) {
						int flags = GetFlags(dev, (WORD)maps[i + 1]);
						groups.back().lights.push_back(MAKELPARAM(LOWORD(maps[i]), maps[i + 1]));
						if (flags & ALIENFX_FLAG_POWER)
							groups.back().have_power = true;
					}
				}
				delete maps;
			}
		}
		for (vindex = 0; RegEnumKey(hKey1, vindex, kName, 255) == ERROR_SUCCESS; vindex++) {
			if (sscanf_s((char*)kName, "Grid%d", &dID) == 1) {
				DWORD nameLen = 255, *grid, sizes;
				RegGetValue(hKey1, kName, "Name", RRF_RT_REG_SZ, 0, name, &nameLen);
				nameLen = sizeof(DWORD);
				RegGetValue(hKey1, kName, "Size", RRF_RT_REG_DWORD, 0, &sizes, &nameLen);
				byte x = (byte)(sizes >> 8), y = (byte)(sizes & 0xff);
				nameLen = x*y*sizeof(DWORD);
				grid = new DWORD[x * y];
				RegGetValue(hKey1, kName, "Grid", RRF_RT_REG_BINARY, 0, grid, &nameLen);
				grids.push_back({ (byte)dID, x, y, name, grid });
				//memcpy(grids.back().grid, grid, MAXGRIDSIZE * sizeof(DWORD));
			}
		}
		RegCloseKey(hKey1);
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
				grLights[j * 2] = LOWORD(groups[i].lights[j]);
				grLights[j * 2 + 1] = HIWORD(groups[i].lights[j]);
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

	std::vector<mapping> *Mappings::GetMappings(DWORD devID) {
		afx_device* dev = GetDeviceById(devID);
		if (dev)
			return &dev->lights;
		return nullptr;
	}

	int Mappings::GetFlags(afx_device* dev, WORD lightid) {
		mapping* lgh = GetMappingById(dev, lightid);
		if (lgh)
			return lgh->flags;
		return 0;
	}

	int Mappings::GetFlags(DWORD devID, WORD lightid)
	{
		afx_device* dev = GetDeviceById(devID);
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