//#define WIN32_LEAN_AND_MEAN
#include "AlienFX_SDK.h"
#include "alienfx-controls.h"
extern "C" {
#include <initguid.h>
#include <hidclass.h>
#include <hidsdi.h>
}
#include <SetupAPI.h>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib, "hid.lib")

// debug print
#ifdef _DEBUG
//#define DebugPrint(_x_) printf("%s",string(_x_).c_str());
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

namespace AlienFX_SDK {

	vector<Afx_icommand> *Functions::SetMaskAndColor(vector<Afx_icommand>* mods, DWORD index, Afx_action c1, Afx_action c2, byte tempo) {
		if (version < API_V4) {
			// index mask generation
			Afx_colorcode c; c.ci = index;
			*mods = { {1, { v1OpCodes[c1.type], chain, c.r, c.g, c.b } } };
		}
		switch (version) {
		case API_V3:
			mods->push_back({6, {c1.r, c1.g,c1.b,c2.r,c2.g,c2.b}});
			break;
		case API_V2:
			mods->push_back({6, {(byte)((c1.r & 0xf0) | ((c1.g & 0xf0) >> 4)),
						(byte)((c1.b & 0xf0) | ((c2.r & 0xf0) >> 4)),
						(byte)((c2.g & 0xf0) | ((c2.b & 0xf0) >> 4))}});
			break;
		case API_V6: case API_V9: {
			vector<byte> command{ 0x51, v6OpCodes[c1.type], 0xd0, v6TCodes[c1.type], (byte)index, c1.r, c1.g, c1.b };
			byte mask = (byte)(c1.r ^ c1.g ^ c1.b ^ index);
			switch (c1.type) {
			case AlienFX_A_Color:
				mask ^= 8;
				command.insert(command.end(), {bright, mask});
				break;
			case AlienFX_A_Pulse:
				mask ^= byte(tempo ^ 1);
				command.insert(command.end(), { bright, tempo, mask} );
				break;
			case AlienFX_A_Breathing:
				c2 = { 0 };
			case AlienFX_A_Morph:
				mask ^= (byte)(c2.r ^ c2.g ^ c2.b ^ tempo ^ 4);
				command.insert(command.end(), { c2.r,c2.g,c2.b, bright, 2, tempo, mask} );
				break;
			}
			byte lpos = 3, cpos = 5;
			if (version == API_V9) {
				lpos = 7; cpos = 0x41;
			}
			*mods = { {cpos, { command } }, {lpos, {(byte)command.size(), 0} } };
		} break;
		}
		return mods;
	}

	inline bool Functions::PrepareAndSend(const byte* command, vector<Afx_icommand> mods) {
		return PrepareAndSend(command, &mods);
	}

	bool Functions::PrepareAndSend(const byte *command, vector<Afx_icommand> *mods) {
		byte buffer[MAX_BUFFERSIZE];
		DWORD written;
		BOOL res = false, needV8Feature = true;

		memset(buffer, version == API_V6 ? 0xff :0, length);
		memcpy(buffer, command, command[0] + 1);
		buffer[0] = reportIDList[version];

		if (mods) {
			for (auto i = mods->begin(); i < mods->end(); i++)
				memcpy(buffer + i->i, i->vval.data(), i->vval.size());
			needV8Feature = mods->front().vval.size() == 1;
			mods->clear();
		}

		if (devHandle) // Is device initialized?
			switch (version) {
			case API_V2: case API_V3: case API_V4: case API_V9:
				//res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, 0, 0, &written, NULL);
				return HidD_SetOutputReport(devHandle, buffer, length);
				//break;
			case API_V5:
				//res =  DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, 0, 0, &written, NULL);
				return HidD_SetFeature(devHandle, buffer, length);
				break;
			case API_V6:
				/*res =*/return WriteFile(devHandle, buffer, length, &written, NULL);
				//if (size == 3)
				//	res &= ReadFile(devHandle, buffer, length, &written, NULL);
				//break;
			case API_V7:
				WriteFile(devHandle, buffer, length, &written, NULL);
				return ReadFile(devHandle, buffer, length, &written, NULL);
				//break;
			case API_V8:
				if (needV8Feature) {
					res = HidD_SetFeature(devHandle, buffer, length);
					Sleep(7);
				}
				else
				{
					return WriteFile(devHandle, buffer, length, &written, NULL);
				}
			}
		return res;
	}

	bool Functions::SavePowerBlock(byte blID, Afx_lightblock* act, bool needSave, bool needInverse) {
		vector<Afx_icommand> mods;
		PrepareAndSend(COMMV1_saveGroup, { {2, {blID}} });
		if (act->act.size() < 2)
			PrepareAndSend(COMMV1_color, SetMaskAndColor(&mods, 1 << act->index, act->act.front()));
		else
			PrepareAndSend(COMMV1_color, SetMaskAndColor(&mods, 1 << act->index, act->act.front(), act->act.back()));
		PrepareAndSend(COMMV1_saveGroup, { {2, {blID}} });
		PrepareAndSend(COMMV1_loop);
		chain++;

		if (needInverse) {
			PrepareAndSend(COMMV1_saveGroup, { {2, {blID}} });
			PrepareAndSend(COMMV1_color, SetMaskAndColor(&mods, ~((1 << act->index)), act->act.front(), act->act.back()));
			PrepareAndSend(COMMV1_saveGroup, { {2, {blID}} });
			PrepareAndSend(COMMV1_loop);
chain++;
		}

		if (needSave) {
			PrepareAndSend(COMMV1_save);
			Reset();
		}

		return true;
	}

	bool Functions::AlienFXProbeDevice(void* hDevInfo, void* devData, WORD vidd, WORD pidd) {
		DWORD dwRequiredSize = 0;
		version = API_UNKNOWN;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, (PSP_DEVICE_INTERFACE_DATA)devData, NULL, 0, &dwRequiredSize, NULL);
		SP_DEVICE_INTERFACE_DETAIL_DATA* deviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)new byte[dwRequiredSize];
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		if (SetupDiGetDeviceInterfaceDetail(hDevInfo, (PSP_DEVICE_INTERFACE_DATA)devData, deviceInterfaceDetailData, dwRequiredSize, NULL, NULL)
			&& (devHandle = CreateFile(deviceInterfaceDetailData->DevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN /*| FILE_FLAG_WRITE_THROUGH*/, NULL)) != INVALID_HANDLE_VALUE) {
			HIDD_ATTRIBUTES attributes{ sizeof(HIDD_ATTRIBUTES) };
			PHIDP_PREPARSED_DATA prep_caps;
			HIDP_CAPS caps;
			if (HidD_GetAttributes(devHandle, &attributes) && (!vidd || attributes.VendorID == vidd) && (!pidd || attributes.ProductID == pidd)
				&& HidD_GetPreparsedData(devHandle, &prep_caps)) {
				HidP_GetCaps(prep_caps, &caps);
				HidD_FreePreparsedData(prep_caps);
				length = caps.OutputReportByteLength;
				pid = attributes.ProductID;
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
						break;
					case 193:
						version = API_V9;
					}
					break;
				default:
					if (length == 65)
						switch (vid) {
						case 0x0424: // Microchip
							if (pid != 0x274c)
								version = API_V6;
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
				CloseHandle(devHandle);
				devHandle = NULL;
			}
			else {
				wchar_t descbuf[256];
				description.clear();
				if (HidD_GetManufacturerString(devHandle, descbuf, 255))
					for (int i = 0; i < wcslen(descbuf); i++)
						description += descbuf[i];
				description += " ";
				if (HidD_GetProductString(devHandle, descbuf, 255))
					for (int i = 0; i < wcslen(descbuf); i++)
						description += descbuf[i];
			}
			//DebugPrint("Probe done, type " + to_string(version) + "\n");
		}
		delete[] deviceInterfaceDetailData;
		return version != API_UNKNOWN;
	}

	//Use this method for general devices, vid = 0 for any vid, pid = 0 for any pid.
	bool Functions::AlienFXInitialize(WORD vidd, WORD pidd) {
		DWORD dwRequiredSize = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };
		devHandle = NULL;

		HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo != INVALID_HANDLE_VALUE) {
			for (DWORD dw = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_HID, dw, &deviceInterfaceData) &&
				(deviceInterfaceData.Flags & SPINT_ACTIVE) &&
				!AlienFXProbeDevice(hDevInfo, &deviceInterfaceData, vidd, pidd); dw++);
			SetupDiDestroyDeviceInfoList(hDevInfo);
		}
		return version != API_UNKNOWN;
	}

#ifndef NOACPILIGHTS
	bool Functions::AlienFXInitialize(AlienFan_SDK::Control* acc) {
		version = API_UNKNOWN;
		if (acc) {
			vid = 0x187c;
			pid = 0xffff;
			ACPIdevice = new AlienFan_SDK::Lights(acc);
			if (((AlienFan_SDK::Lights*)ACPIdevice)->isActivated) {
				version = API_ACPI;
				return true;
			}
		}
		return false;
	}
#endif

	bool Functions::Reset() {
		switch (version) {
		case API_V9:
			if (chain) {
				PrepareAndSend(COMMV9_update);
				GetDeviceStatus();
				chain = 0;
			}
			break;
		case API_V6:
			// need initial reset if not done
			if (chain) {
				inSet = PrepareAndSend(COMMV6_systemReset);
				chain = 0;
			} else
				inSet = true;
			break;
		case API_V5:
		{
			inSet = PrepareAndSend(COMMV5_reset);
			GetDeviceStatus();
		} break;
		case API_V4:
		{
			WaitForReady();
			PrepareAndSend(COMMV4_control, { { 4, {4} }/*, { 5, 0xff }*/ });
			inSet = PrepareAndSend(COMMV4_control, { {4, {1}}/*, { 5, 0xff }*/ });
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
			//case API_V9:
			//	inSet = !PrepareAndSend(COMMV9_update);
			//	GetDeviceStatus();
			//	break;
			case API_V5:
			{
				inSet = !PrepareAndSend(COMMV5_update);
			} break;
			case API_V4:
			{
				inSet = !PrepareAndSend(COMMV4_control);
			} break;
			case API_V3: case API_V2:
			{
				inSet = !PrepareAndSend(COMMV1_update);
				//WaitForBusy();
				DebugPrint("Post-update status: " + to_string(GetDeviceStatus()) + "\n");
			} break;
			default: inSet = false;
			}
		}
		return !inSet;

	}

	bool Functions::SetColor(byte index, Afx_action c) {
		Afx_lightblock act{ index, {c} };
		return SetAction(&act);
	}

	void Functions::AddV8DataBlock(byte bPos, vector<Afx_icommand>* mods, Afx_lightblock* act) {
		mods->push_back( {bPos, {act->index,v8OpCodes[act->act.front().type], act->act.front().tempo, 0xa5, act->act.front().time, 0xa,
			act->act.front().r, act->act.front().g,act->act.front().b,
			act->act.back().r,act->act.back().g,act->act.back().b,
			2} } );
	}

	void Functions::AddV5DataBlock(byte bPos, vector<Afx_icommand>* mods, byte index, Afx_action* c) {
		mods->push_back( {bPos, { (byte)(index + 1),c->r,c->g,c->b} });
	}

	bool Functions::SetMultiColor(vector<UCHAR> *lights, Afx_action c) {
		bool val = false;
		Afx_lightblock act{ 0, {c} };
		vector<Afx_icommand> mods;
		if (!inSet) Reset();
		switch (version) {
		case API_V8: {
			PrepareAndSend(COMMV8_readyToColor, { {2,{(byte)lights->size()}} });
			auto nc = lights->begin();
			for (byte cnt = 1; nc != lights->end(); cnt++) {
				for (byte bPos = 5; bPos < length && nc != lights->end(); bPos += 15) {
					act.index = *nc;
					AddV8DataBlock(bPos, &mods, &act);
					nc++;
				}
				if (mods.size()) {
					mods.push_back({ 4, {cnt} });
					val = PrepareAndSend(COMMV8_readyToColor, &mods);
				}
			}
		} break;
		case API_V5:
			for (auto nc = lights->begin(); nc != lights->end();) {
				for (byte bPos = 4; bPos < length && nc != lights->end(); bPos += 4) {
					AddV5DataBlock(bPos, &mods, *nc, &c);
					nc++;
				}
				if (mods.size())
					PrepareAndSend(COMMV5_colorSet, &mods);
			}
			val = PrepareAndSend(COMMV5_loop);
			break;
		case API_V4:
		{
			mods = { { 3, {c.r, c.g, c.b, 0, (byte)lights->size()} },
				{8, *lights} };
			val = PrepareAndSend(COMMV4_setOneColor, &mods);
		} break;
		case API_V3: case API_V2: case API_V6: case API_V9: case API_ACPI:
		{
			DWORD fmask = 0;
			for (auto nc = lights->begin(); nc < lights->end(); nc++)
				fmask |= 1 << (*nc);
			SetMaskAndColor(&mods, fmask, c);
			switch (version) {
			case API_V6:
				val = PrepareAndSend(COMMV6_colorSet, &mods);
				break;
			case API_V9:
				val = PrepareAndSend(COMMV9_colorSet, &mods);
				break;
#ifndef NOACPILIGHTS
			case API_ACPI:
				val = ((AlienFan_SDK::Lights*)ACPIdevice)->SetColor((byte)fmask, c.r, c.g, c.b);
				break;
#endif
			default:
				PrepareAndSend(COMMV1_color, &mods);
				val = PrepareAndSend(COMMV1_loop);
				chain++;
			}
		} break;
		default:
			for (auto nc = lights->begin(); nc < lights->end(); nc++) {
				act.index = *nc;
				val = SetAction(&act);
			}
		}
		return val;
	}

	bool Functions::SetMultiAction(vector<Afx_lightblock> *act, bool save) {
		bool val = true;
		vector<Afx_icommand> mods;
		if (save)
			return SetPowerAction(act, save);

		if (!inSet) Reset();
		switch (version) {
		case API_V8: {
			//byte bPos = 5, cnt = 0;
			PrepareAndSend(COMMV8_readyToColor, { {2,{(byte)act->size()}} });
			auto nc = act->begin();
			for (byte cnt = 1; nc != act->end(); cnt++) {
				for (byte bPos = 5; bPos < length && nc != act->end(); bPos+=15) {
					AddV8DataBlock(bPos, &mods, &(*nc));
					nc++;
				}
				mods.push_back({ 4, {cnt} });
				val = PrepareAndSend(COMMV8_readyToColor, &mods);
			}
		} break;
		case API_V5:
		{
			for (auto nc = act->begin(); nc != act->end();) {
				for (byte bPos = 4; bPos < length && nc != act->end(); bPos += 4) {
					AddV5DataBlock(bPos, &mods, nc->index, &nc->act.front());
					nc++;
				}
				PrepareAndSend(COMMV5_colorSet, &mods);
			}
			val = PrepareAndSend(COMMV5_loop);
		} break;
		default:
		{
			for (auto nc = act->begin(); nc != act->end(); nc++)
				val = SetAction(&(*nc));
		} break;
		}

		return val;
	}

	bool Functions::SetV4Action(Afx_lightblock* act) {
		bool res = false;
		vector<Afx_icommand> mods;
		PrepareAndSend(COMMV4_colorSel, { {6,{act->index}} });
		for (auto ca = act->act.begin(); ca != act->act.end();) {
			// 3 actions per record..
			for (byte bPos = 3; bPos < length && ca != act->act.end(); bPos += 8) {
				mods.push_back(
					{bPos, {(byte)(ca->type < AlienFX_A_Breathing ? ca->type : AlienFX_A_Morph), ca->time, v4OpCodes[ca->type], 0,
					(byte)(ca->type == AlienFX_A_Color ? 0xfa : ca->tempo), ca->r, ca->g, ca->b} });
				ca++;
			}
			res = PrepareAndSend(COMMV4_colorSet, &mods);
		}
		return res;
	}

	bool Functions::SetAction(Afx_lightblock* act) {
		if (act->act.empty())
			return false;
		if (!inSet) Reset();

		vector<Afx_icommand> mods;
		switch (version) {
		case API_V8:
			AddV8DataBlock(5, &mods, act);
			PrepareAndSend(COMMV8_readyToColor);
			return PrepareAndSend(COMMV8_readyToColor, &mods);
		case API_V7:
		{
			mods = { {5,{v7OpCodes[act->act.front().type],bright,act->index}} };
			for (int ca = 0; ca < act->act.size(); ca++) {
				if (ca * 3 + 10 < length)
					mods.push_back({ ca * 3 + 8, {	act->act.at(ca).r, act->act.at(ca).g, act->act.at(ca).b} });
			}
			return PrepareAndSend(COMMV7_control, &mods);
		}
		case API_V6:
			return PrepareAndSend(COMMV6_colorSet, SetMaskAndColor(&mods, 1 << act->index, act->act.front(), act->act.back(), act->act.front().tempo));
		case API_V9:
			return PrepareAndSend(COMMV9_colorSet, SetMaskAndColor(&mods, 1 << act->index, act->act.front(), act->act.back(), act->act.front().tempo));
		case API_V5:
			AddV5DataBlock(4, &mods, act->index, &act->act.front());
			PrepareAndSend(COMMV5_colorSet, &mods);
			return PrepareAndSend(COMMV5_loop);
		case API_V4:
			// check types and call
			switch (act->act.front().type) {
			case AlienFX_A_Color: // it's a color, so set as color
				return PrepareAndSend(COMMV4_setOneColor, { {3, {act->act.front().r,act->act.front().g,act->act.front().b, 0, 1, (byte)act->index } } });
			case AlienFX_A_Power: { // Set power
				vector<Afx_lightblock> t = { *act };
				return SetPowerAction(&t);
			} break;
			default: // Set action
				return SetV4Action(act);
			}
		case API_V3: case API_V2:
		{
			bool res = false;
			// check types and call
			switch (act->act.front().type) {
			case AlienFX_A_Power: { // SetPowerAction for power!
				vector<Afx_lightblock> t = { {*act} };
				return SetPowerAction(&t);
			}
			case AlienFX_A_Color:
				break;
			default:
				PrepareAndSend(COMMV1_setTempo,
					{ {2, { (byte)(((WORD)act->act.front().tempo << 3 & 0xff00) >> 8),
						(byte)((WORD)act->act.front().tempo << 3 & 0xff),
						(byte)(((WORD)act->act.front().time << 5 & 0xff00) >> 8),
						(byte)((WORD)act->act.front().time << 5 & 0xff)} } });
			}
			Afx_action c2{ 0 };
			for (auto ca = act->act.begin(); ca != act->act.end(); ca++) {
				if (act->act.size() > 1)
					c2 = ca + 1 != act->act.end() ? *(ca + 1) : act->act.front();
				DebugPrint("SDK: Set light " + to_string(act->index) + "\n");
				PrepareAndSend(COMMV1_color, SetMaskAndColor(&mods, 1 << act->index, *ca, c2));
			}
			//DebugPrint("SDK: Loop\n");
			res = PrepareAndSend(COMMV1_loop);
			chain++;
			return res;
		}
#ifndef NOACPILIGHTS
		case API_ACPI:
			return ((AlienFan_SDK::Lights*)ACPIdevice)->SetColor(1 << act->index, act->act.front().r, act->act.front().g, act->act.front().b);
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
				PrepareAndSend(COMMV4_control, { { 4, {4,0,0x61} } });
				PrepareAndSend(COMMV4_control, { { 4, {1,0,0x61} } });
				for (auto ca = act->begin(); ca != act->end(); ca++)
					if (ca->act.front().type != AlienFX_A_Power)
						SetV4Action(&(*ca));
				PrepareAndSend(COMMV4_control, { { 4, {2,0,0x61} } });
				PrepareAndSend(COMMV4_control, { { 4, {6,0,0x61} } });
			}
			else {
				pwr = &act->front();
				// Now set power button....
				for (BYTE cid = 0x5b; cid < 0x61; cid++) {
					// Init query...
					PrepareAndSend(COMMV4_setPower, { {4,{4,0,cid}} });
					PrepareAndSend(COMMV4_setPower, { {4,{1,0,cid}} });
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
					SetV4Action(&tact);
					// And finish
					PrepareAndSend(COMMV4_setPower, { {4,{2,0,cid}} });
				}

				PrepareAndSend(COMMV4_control, { { 4, {5} }/*, { 6, 0x61 }*/ });
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
				SetAction(pwr);
		} break;
		}
		return true;
	}

	bool Functions::SetBrightness(BYTE brightness, vector<Afx_light> *mappings, bool power) {

		DebugPrint("State update: PID: " + to_string(pid) + ", brightness: " + to_string(brightness) + ", power: " + to_string(power) + "\n");

		if (inSet) UpdateColors();
		int oldBr = bright;
		bright = (brightness * brightnessScale[version]) / 0xff;
		switch (version) {
		case API_V8:
			return PrepareAndSend(COMMV8_setBrightness, { {2, {bright}} });
		case API_V5:
			Reset();
			return PrepareAndSend(COMMV5_turnOnSet, { {4, {bright}} });
		case API_V4: {
			vector<byte> idlist;
			for (auto i = mappings->begin(); i < mappings->end(); i++)
				if (!i->flags || power) {
					idlist.push_back((byte)i->lightid);
				}
			vector<Afx_icommand> mods{ {3,{(byte)(0x64 - bright), 0, (byte)mappings->size()}},
										{ 6, idlist} };
			return PrepareAndSend(COMMV4_turnOn, &mods);
		}
		case API_V3: case API_V2:
			if (!bright || !oldBr) {
				PrepareAndSend(COMMV1_reset, { {2,{(byte)(brightness ? 4 : power ? 3 : 1)}} });
				WaitForReady();
			}
			return PrepareAndSend(COMMV1_dim, { { 2,{bright} } });
#ifndef NOACPILIGHTS
		case API_ACPI:
			bright = brightness * 0xf / 0xff;
			return ((AlienFan_SDK::Lights*)ACPIdevice)->SetBrightness(bright);
#endif // !NOACPILIGHTS
		}
		return false;
	}

	bool Functions::SetGlobalEffects(byte effType, byte mode, byte nc, byte tempo, Afx_action act1, Afx_action act2) {
		vector<Afx_icommand> mods;
		switch (version) {
		case API_V8:
			//PrepareAndSend(COMMV8_effectReset);
			PrepareAndSend(COMMV8_effectReady);
			//PrepareAndSend(COMMV8_effectReady, { {3, {0}} });
			return PrepareAndSend(COMMV8_effectReady, { {3, { effType, act1.r, act1.g, act1.b, act2.r, act2.g, act2.b,
				tempo, bright, 1, mode, nc} }});
		case API_V5:
			if (inSet)
				UpdateColors();
			Reset();
			if (effType < 2)
				PrepareAndSend(COMMV5_setEffect, { {2,{1}}, {11,{0xff,12,0xff}}, {14,{0xff,0xff} } });
			else
				PrepareAndSend(COMMV5_setEffect, { {2, {effType,tempo}},
						   {10,{ act1.r,act1.g,act1.b,act2.r,act2.g,act2.b, 5}}});
			if (effType < 2)
				return PrepareAndSend(COMMV5_update, { {3,{0xfe}}, {6,{0xff,0xff}} });
			else
				return UpdateColors();
		}
		return false;
	}

	BYTE Functions::GetDeviceStatus() {

		byte buffer[MAX_BUFFERSIZE];
		//DWORD written;
		if (devHandle)
			switch (version) {
			case API_V9:
				HidD_GetInputReport(devHandle, buffer, length);
				return 1;
			case API_V5:
			{
				PrepareAndSend(COMMV5_status);
				if (HidD_GetFeature(devHandle, buffer, length))
				//if (DeviceIoControl(devHandle, IOCTL_HID_GET_FEATURE, 0, 0, buffer, length, &written, NULL))
					return buffer[2];
			} break;
			case API_V4:
			{
				if (HidD_GetInputReport(devHandle, buffer, length))
				//if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
					return buffer[2];
			} break;
			case API_V3: case API_V2:
			{
				PrepareAndSend(COMMV1_status);
				if (HidD_GetInputReport(devHandle, buffer, length))
				//if (DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, &written, NULL))
					return buffer[0];
			} break;
			}
		return 0;
	}

	BYTE Functions::WaitForReady() {
		int i = 0;
		switch (version) {
		case API_V3: case API_V2:
			//if (!GetDeviceStatus())
			//	Reset();
			for (; i < 100 && GetDeviceStatus() != ALIENFX_V2_READY; i++) 
				Sleep(10);
			return i < 100;
		case API_V4:
			while (!IsDeviceReady()) Sleep(20);
			return 1;
		default:
			return GetDeviceStatus();
		}
	}

	BYTE Functions::WaitForBusy() {
		int i = 0;
		switch (version) {
		case API_V3: case API_V2:
			if (GetDeviceStatus())
				for (; i < 100 && GetDeviceStatus() != ALIENFX_V2_BUSY; i++) 
					Sleep(10);
			return i < 100;
			break;
		case API_V4: {
			for (; i < 500 && GetDeviceStatus() != ALIENFX_V4_BUSY; i++) Sleep(20);
			return i < 500;
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
		default:
			return !inSet;
		}
	}

	Functions::~Functions() {
		if (version != API_UNKNOWN) {
			CloseHandle(devHandle);
		}
#ifndef NOACPILIGHTS
		if (ACPIdevice) {
			delete (AlienFan_SDK::Lights*)ACPIdevice;
		}
#endif
	}

	Mappings::~Mappings() {
		for (auto i = fxdevs.begin(); i < fxdevs.end(); i++)
			if (i->dev) delete i->dev;
	}

	vector<Functions*> Mappings::AlienFXEnumDevices(void* acc) {
		vector<Functions*> devs;
		Functions* dev;

		HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo != INVALID_HANDLE_VALUE) {
			SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };
			for (DWORD dw = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_HID, dw, &deviceInterfaceData); dw++) {
				dev = new Functions();
				//DebugPrint("Testing device #" + to_string(dw) + ", ID=" + to_string(deviceInterfaceData.Reserved) + "\n");
				if ((deviceInterfaceData.Flags & SPINT_ACTIVE) && dev->AlienFXProbeDevice(hDevInfo, &deviceInterfaceData))
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
			dev = new AlienFX_SDK::Functions();
			if (dev->AlienFXInitialize((AlienFan_SDK::Control*)acc))
				devs.push_back(dev);
			else
				delete dev;
		}
#endif
		return devs;
	}

	void Mappings::AlienFXApplyDevices(bool activeOnly, vector<Functions*> devList) {
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
			if (dev->name.empty())
				dev->name = (*i)->description;
			if (!dev->dev) {
				dev->dev = *i;
				dev->version = (*i)->version;
			}
			else
				delete (*i);
			activeLights += (int)dev->lights.size();
		}
		devList.clear();
	}

	void Mappings::AlienFXAssignDevices(bool activeOnly, void* acc) {
		AlienFXApplyDevices(activeOnly, AlienFXEnumDevices(acc));
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
		//return &grids[id];
		for (auto pos = grids.begin(); pos != grids.end(); pos++)
			if (pos->id == id)
				return &(*pos);
		return nullptr;
	}

	Afx_device* Mappings::AddDeviceById(WORD pid, WORD vid)
	{
		Afx_device* dev = GetDeviceById(pid, vid);
		if (!dev) {
			fxdevs.push_back({ pid, vid, NULL });
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
		for (vindex = 0; RegEnumKey(mainKey, vindex, kName, 255) == ERROR_SUCCESS; vindex++) {
			if (sscanf_s(kName, "Group%d", &dID) == 1) {
				RegGetValue(mainKey, kName, "Name", RRF_RT_REG_SZ, 0, name, &(lend = 255));
				groups.push_back({ dID, name });
				vector<Afx_groupLight>* gl = &groups.back().lights;
				if (RegGetValue(mainKey, kName, "LightList", RRF_RT_REG_BINARY, 0, NULL, &lend) != ERROR_FILE_NOT_FOUND) {
					gl->resize(lend / sizeof(DWORD));
					RegGetValue(mainKey, kName, "LightList", RRF_RT_REG_BINARY, 0, gl->data(), &lend);
				}
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
			if (i->name.length())
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
			RegSetValueEx(hKeyStore, "Name", 0, REG_SZ, (BYTE *) i->name.c_str(), (DWORD) i->name.length());
			RegSetValueEx(hKeyStore, "LightList", 0, REG_BINARY, (BYTE*)i->lights.data(), (DWORD)i->lights.size() * sizeof(DWORD));
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
		return dev ? GetMappingByDev(dev, lid) : nullptr;
	}

	int Mappings::GetFlags(Afx_device* dev, WORD lightid) {
		Afx_light* lgh = GetMappingByDev(dev, lightid);
		return lgh ? lgh->flags : 0;
	}

	int Mappings::GetFlags(DWORD devID, WORD lightid)
	{
		Afx_device* dev = GetDeviceById(LOWORD(devID), HIWORD(devID));
		return dev ? GetFlags(dev, lightid) : 0;
	}

	bool Functions::IsHaveGlobal()
	{
		return version == API_V5 || version == API_V8;
		//switch (version) {
		//case API_V5: case API_V8: return true;
		//}
		//return false;
	}
}