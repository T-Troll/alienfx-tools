#define WIN32_LEAN_AND_MEAN
#include "AlienFX_SDK.h"
#include "alienfx-controls.h"
#ifndef NOACPILIGHTS
#include "alienfan-low.h"
#else
#include <SetupAPI.h>
#pragma comment(lib,"setupapi.lib")
#endif
#include <memory>
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

#pragma comment(lib, "hid.lib")

namespace AlienFX_SDK {	

	vector<pair<byte, byte>> *Functions::SetMaskAndColor(DWORD index, byte type, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2) {
		vector<pair<byte, byte>> *mods = new vector<pair<byte, byte>>({{1, type},{2,(byte)chain},
										 {3,(byte)((index & 0xFF0000) >> 16)},
										 {4,(byte)((index & 0x00FF00) >> 8)},
										 {5,(byte)(index & 0x0000FF)}});
		switch (version) {
		case API_L_V1:
			mods->insert(mods->end(),{{6,r1},{7,g1},{8,b1}});
			break;
		case API_L_V3:
			mods->insert(mods->end(),{{6,r1},{7,g1},{8,b1},{9,r2},{10,g2},{11,b2}});
			break;
		case API_L_V2:
			mods->insert(mods->end(),{{6,(r1 & 0xf0) | ((g1 & 0xf0) >> 4)},
						{7,(b1 & 0xf0) | ((r2 & 0xf0) >> 4)},
						{8,(g2 & 0xf0) | ((b2 & 0xf0) >> 4)}});
			break;
		}
		return mods;
	}

	bool Functions::SetAcpiColor(byte mask, byte r, byte g, byte b) {
#ifndef NOACPILIGHTS
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = NULL;
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, r);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, g);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, b);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, mask);
		if (EvalAcpiMethodArgs(devHandle, "\\_SB.AMW1.SETC", acpiargs, (PVOID *) &resName)) {
			free(resName);
			return true;
		}
#endif
		return false;
	}

	bool Functions::PrepareAndSend(const byte *command, byte size, vector<pair<byte, byte>> *mods) {
		byte buffer[MAX_BUFFERSIZE]{0};
		DWORD written;

		if (version == API_L_V6)
			FillMemory(buffer, MAX_BUFFERSIZE, 0xff);

		memcpy(&buffer[1], command, size);
		buffer[0] = reportID;

		for (int i = 0; mods && i < mods->size(); i++)
			buffer[(*mods)[i].first] = (*mods)[i].second;

		switch (version) {
		case API_L_V1: case API_L_V2: case API_L_V3: case API_L_V4:
			return HidD_SetOutputReport(devHandle, buffer, length);
		case API_L_V5:
			return HidD_SetFeature(devHandle, buffer, length);
		case API_L_V6:
			return WriteFile(devHandle, buffer, length, &written, NULL);
		case API_L_V7:
			WriteFile(devHandle, buffer, length, &written, NULL);
			return ReadFile(devHandle, buffer, length, &written, NULL);
		}
		return false;
	}

	bool Functions::PrepareAndSend(const byte *command, byte size, vector<pair<byte, byte>> mods) {
		return PrepareAndSend(command, size, &mods);
	}

	bool Functions::SavePowerBlock(byte blID, act_block act, bool needSave, bool needInverse) {
		byte mode;
		switch (act.act[0].type) {
		case AlienFX_A_Pulse:
			mode = 2; break;
		case AlienFX_A_Morph:
			mode = 1; break;
		default: mode = 3;
		}
		PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
		if (act.act.size() < 2)
			PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(1 << act.index, mode,
																			    act.act[0].r, act.act[0].g, act.act[0].b));
		else
			PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(1 << act.index, mode,
																			   act.act[0].r, act.act[0].g, act.act[0].b,
																			   act.act[1].r, act.act[1].g, act.act[1].b));
		PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
		Loop();

		if (needInverse) {
			PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
			PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(~((1 << act.index)), act.act[1].type,
																			   act.act[0].r, act.act[0].g, act.act[0].b,
																			   act.act[1].r, act.act[1].g, act.act[1].b));
			PrepareAndSend(COMMV1.saveGroup, sizeof(COMMV1.saveGroup), {{2,blID}});
			Loop();
		}

		if (needSave) {
			PrepareAndSend(COMMV1.save, sizeof(COMMV1.save));
			Reset();
		}
			
		return true;
	}

	//Use this method for general devices, pid = -1 for full scan
	int Functions::AlienFXInitialize(int vid, int pidd) {
		GUID guid;
		bool flag = false;
		pid = -1;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE) {
			return pid;
		}
		unsigned int dw = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData{sizeof(SP_DEVICE_INTERFACE_DATA)};

		while (!flag) {
			if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData)) {
				flag = true;
				continue;
			}
			dw++;
			DWORD dwRequiredSize = 0;
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL)) {
				continue;
			}

			std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA *)new char[dwRequiredSize]);
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL)) {
				std::wstring devicePath = deviceInterfaceDetailData->DevicePath;
				devHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				if (devHandle != INVALID_HANDLE_VALUE) {
					std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
					attributes->Size = sizeof(HIDD_ATTRIBUTES);
					if (HidD_GetAttributes(devHandle, attributes.get())) {
						if (((!vid || attributes->VendorID == vid) && (pidd == -1 || attributes->ProductID == pidd))) {
							// Check API version...
							PHIDP_PREPARSED_DATA prep_caps;
							HIDP_CAPS caps;
							HidD_GetPreparsedData(devHandle, &prep_caps);
							HidP_GetCaps(prep_caps, &caps);
							HidD_FreePreparsedData(prep_caps);

							if (caps.OutputReportByteLength || caps.Usage == 0xcc) {

								// Yes, now so easy...
								length = caps.OutputReportByteLength;
								switch (caps.OutputReportByteLength) {
								case 0: 
									length = caps.FeatureReportByteLength;
									version = 5;
									reportID = 0xcc;
									break;
								case 8: 
									version = 1;
									reportID = 2;
									break;
								case 9: 
									version = 2;
									reportID = 2;
									break;
								case 12:
									version = 3;
									reportID = 2;
									break;
								case 34:
									version = 4;
									reportID = 0;
									break;
								case 65:
									switch (attributes->VendorID) {
									case 0x0424:
										version = 6;
										break;
									case 0x0461:
										version = 7;
										break;
									}
									reportID = 0;
									break;
								//default: length = caps.OutputReportByteLength;
								}

								this->vid = attributes->VendorID;
								pid = attributes->ProductID;
								flag = true;
//#ifdef _DEBUG
//								wchar_t buff[2048];
//								swprintf_s(buff, 2047, L"Init: VID: %#x, PID: %#x, Version: %d, Length: %d\n",
//										   attributes->VendorID, attributes->ProductID, attributes->VersionNumber, length);
//								OutputDebugString(buff);
//								cout << "Attributes - length: " << attributes->Size << ", version: " << attributes->VersionNumber << endl;
//								wprintf(L"Path: %s\n%s", devicePath.c_str(), buff);
//#endif
							}
						}
					}
					if (!flag)
						CloseHandle(devHandle);
				}
			}
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return pid;
	}

	int Functions::AlienFXInitialize(HANDLE acc) {
#ifndef NOACPILIGHTS
		if (acc && acc != INVALID_HANDLE_VALUE) {
			devHandle = acc;
			vid = pid = length = API_L_ACPI;
			version = 0;
			if (Reset()) {
				return pid;
			}
		}
#endif
		return -1;
	}

	void Functions::Loop() {
		//byte buffer[MAX_BUFFERSIZE];
		//ZeroMemory(buffer, length);
		switch (version) {
		case API_L_V7:
			//PrepareAndSend(COMMV7.update, sizeof(COMMV7.update), {{9,1}});
			break;
		case API_L_V5:
			PrepareAndSend(COMMV5.loop, sizeof(COMMV5.loop));
			break;
		//case API_L_V4: {
		//	 //m15 require Input report as a confirmation, not output.
		//	 //WARNING!!! In latest firmware, this can provide up to 10sec(!) slowdown, so i disable status read. It works without it as well.
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	 //std::cout << "Status: 0x" << std::hex << (int) BufferN[2] << std::endl;
		//} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			PrepareAndSend(COMMV1.loop, sizeof(COMMV1.loop));
			chain++;
		} break;
		}
	}

	bool Functions::Reset() {
		bool result = false;
		switch (version) {
		case API_L_V7:
		{
			result = PrepareAndSend(COMMV7.status, sizeof(COMMV7.status));
			//result = PrepareAndSend(COMMV7.ack, sizeof(COMMV7.ack));
		} break;
		case API_L_V5:
		{
			result = PrepareAndSend(COMMV5.reset, sizeof(COMMV5.reset));
		} break;
		case API_L_V4:
		{
			result = PrepareAndSend(COMMV4.reset, sizeof(COMMV4.reset));
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			result = PrepareAndSend(COMMV1.reset, sizeof(COMMV1.reset));
			AlienfxWaitForReady();
			chain = 1;
		} break;
		case API_L_ACPI:
		{
#ifndef NOACPILIGHTS
			PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
			if (!inSet && EvalAcpiMethod(devHandle, "\\_SB.AMW1.ICPC", (PVOID *) &resName)) {
				free(resName);
				result = true;
			}
#endif
		} break;
		}
		inSet = true;
		return result;
	}

	bool Functions::UpdateColors() {
		bool res = false;

		if (inSet) {
			switch (version) {
			case API_L_V7:
			{
				//PrepareAndSend(COMMV7.status, sizeof(COMMV7.status));
				//PrepareAndSend(COMMV7.control, sizeof(COMMV7.control), {{6,bright}});
				//res = PrepareAndSend(COMMV7.update, sizeof(COMMV7.update), {{8,1}});
			} break;
			case API_L_V5:
			{
				res = PrepareAndSend(COMMV5.update, sizeof(COMMV5.update));
			} break;
			case API_L_V4:
			{
				res = PrepareAndSend(COMMV4.update, sizeof(COMMV4.update));
			} break;
			case API_L_V3: case API_L_V2: case API_L_V1:
			{
				res = PrepareAndSend(COMMV1.update, sizeof(COMMV1.update));
				AlienfxWaitForBusy();
				chain = 1;
			} break;
			case API_L_ACPI:
			{
#ifndef NOACPILIGHTS
				PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
				if (EvalAcpiMethod(devHandle, "\\_SB.AMW1.RCPC", (PVOID *) &resName)) {
					free(resName);
					res = true;
				}
#endif
			} break;
			default: res = false;
			}
			//std::cout << "Update!" << std::endl;
			inSet = false;
			Sleep(5); // Fix for ultra-fast updates, or next command will fail sometimes.
		}
		return res;

	}

	bool Functions::SetColor(unsigned index, byte r, byte g, byte b, bool loop) {
		bool val = false;
		if (!inSet)
			Reset();

		switch (version) {
		case API_L_V7:
		{
			PrepareAndSend(COMMV7.status, sizeof(COMMV7.status));
			val = PrepareAndSend(COMMV7.control, sizeof(COMMV7.control), {{6,bright},{7,index},{8,r},{9,g},{10,b}});
			//val = PrepareAndSend(COMMV7.colorSet, sizeof(COMMV7.colorSet), {{5,(byte)(index+1)},{6,r},{7,g},{8,b}});
		} break;
		case API_L_V6:
		{
			val = PrepareAndSend(COMMV6.colorSet, sizeof(COMMV6.colorSet), {{9,(byte)(1 << index)},{10,r},{11,g},{12,b},{13,bright}});
		} break;
		case API_L_V5:
		{
			val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), {{4,(byte)(index+1)},{5,r},{6,g},{7,b}});
		} break;
		case API_L_V4:
		{
			PrepareAndSend(COMMV4.colorSel, sizeof(COMMV4.colorSel), {{6,index}});
			val = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), {{8,r}, {9,g}, {10,b}});
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			val = PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(1<<index, 3, r, g, b));
		} break;
		case API_L_ACPI:
		{
			val = SetAcpiColor(1 << index, r, g, b);
		} break;
		default: return false;
		}
		if (loop) Loop();
		return val;
	}

	bool Functions::SetMultiLights(vector<UCHAR> *lights, int r, int g, int b) {
		bool val = false;
		if (!inSet) Reset();

		switch (version) {
		case API_L_V6:
		{
			byte mask = 0;
			for (int nc = 0; nc < lights->size(); nc++)
				mask |= 1 << (*lights)[nc];
			val = PrepareAndSend(COMMV6.colorSet, sizeof(COMMV6.colorSet), {
				{9,mask},{10,r},{11,g},{12,b},{13,bright}});
		} break;
		case API_L_V5:
		{
			vector<pair<byte, byte>> mods;
			int bPos = 4;
			for (int nc = 0; nc < lights->size(); nc++) {
				if (bPos + 4 < length) {
					mods.insert(mods.end(), {
						{bPos,(*lights)[nc] + 1},{bPos+1,r},{bPos+2,g},{bPos+3,b}});
					bPos += 4;
				} else {
					// Send command and clear buffer...
					val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), mods);
					mods.clear();
					bPos = 4;
				}
			}
			if (bPos > 4)
				val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), mods);
			Loop();
		} break;
		case API_L_V4:
		{
			vector<pair<byte, byte>> mods{{5,(byte)lights->size()}};
			for (int nc = 0; nc < lights->size(); nc++)
				mods.push_back({6 + nc, (*lights)[nc]});
			PrepareAndSend(COMMV4.colorSel, sizeof(COMMV4.colorSel), mods);
			val = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), {{8,r}, {9,g}, {10,b}});
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			// for older, you just need mix the mask!
			DWORD fmask = 0;
			for (int nc = 0; nc < lights->size(); nc++)
				fmask |= 1 << (*lights)[nc];
			val = PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), SetMaskAndColor(fmask, 3, r, g, b));
			Loop();
		} break;
		case API_L_ACPI:
		{
			byte fmask = 0;
			for (int nc = 0; nc < lights->size(); nc++)
				fmask |= 1 << (*lights)[nc];
			val = SetAcpiColor(fmask, r, g, b);
		} break;
		default:
		{
			for (int nc = 0; nc < lights->size(); nc++)
				val = SetColor((*lights)[nc], r, g, b);
		}
		}
		return val;
	}

	bool Functions::SetMultiColor(vector<act_block> *act, bool save) {

		bool val = true;
		if (!inSet) Reset();
		switch (version) {
		case API_L_V5:
		{
			int bPos = 4;
			vector<pair<byte, byte>> mods;
			for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++) {
				if (bPos + 4 < length) {
					mods.insert(mods.end(), {
						        {bPos,nc->index + 1},
								{bPos+1,nc->act[0].r},
								{bPos+2,nc->act[0].g},
								{bPos+3,nc->act[0].b}});
					bPos += 4;
				} else {
					// Send command and clear buffer...
					val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), mods);
					mods.clear();
					bPos = 4;
				}
			}
			if (bPos > 4)
				val = PrepareAndSend(COMMV5.colorSet, sizeof(COMMV5.colorSet), mods);
			Loop();
		}break;
		case API_L_V1: case API_L_V2: case API_L_V3: case API_L_V4: 
		{
			if (save)
				SetPowerAction(act);
			else
				for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++)
					if (nc->act[0].type != AlienFX_A_Power)
						val = SetAction(&(*nc));
					else {
						vector<act_block> tact{{*nc}};
						val = SetPowerAction(&tact);
					}
		} break;
		default: //case API_L_ACPI:
		{
			for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++) {
				val = SetColor(nc->index, nc->act[0].r, nc->act[0].g, nc->act[0].b);
			}
		} break;
		}
		return val;
	}

	bool Functions::SetAction(act_block *act) {
		bool res = false;

		if (act->act.size() > 0) {
			if (!inSet) Reset();
			switch (version) {
			case API_L_V4:
			{
				PrepareAndSend(COMMV4.colorSel, sizeof(COMMV4.colorSel), {{6,act->index}});
				int bPos = 3;
				vector<pair<byte, byte>> mods;
				for (int ca = 0; ca < act->act.size(); ca++) {
					// 3 actions per record..
					mods.insert(mods.end(), {
						 {bPos,act->act[ca].type < AlienFX_A_Breathing ? act->act[ca].type : AlienFX_A_Morph },
						 {bPos + 1, act->act[ca].time},
						 {bPos + 4, act->act[ca].tempo},
						 {bPos + 5, act->act[ca].r},
						 {bPos + 6, act->act[ca].g},
						 {bPos + 7, act->act[ca].b}});
					switch (act->act[ca].type) {
					/*case AlienFX_A_Color: 
						mods.push_back({bPos+2, 0xd0});
						mods.push_back({bPos+4, 0xfa});
						break;*/
					case AlienFX_A_Pulse:
						mods.push_back({bPos+2, 0xdc});
						break;
					case AlienFX_A_Morph:
						mods.push_back({bPos+2, 0xcf});
						break;
					case AlienFX_A_Breathing:
						mods.push_back({bPos+2, 0xdc});
						break;
					case AlienFX_A_Spectrum: 
						mods.push_back({bPos+2, 0x82});
						break;
					case AlienFX_A_Rainbow:
						mods.push_back({bPos+2, 0xac});
						break;
					case AlienFX_A_Power: 
						mods.push_back({bPos+2, 0xe8});
						break;
					default: 
						mods.push_back({bPos+2, 0xd0});
						mods.push_back({bPos+4, 0xfa});
					}
					bPos += 8;
					if (bPos + 8 >= length) {
						res = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), mods);
						mods.clear();
						bPos = 3;
					}
				}
				if (bPos != 3) {
					res = PrepareAndSend(COMMV4.colorSet, sizeof(COMMV4.colorSet), mods);
				}
				return res;
			} break;
			case API_L_V3: case API_L_V2: case API_L_V1:
			{
				if (act->act[0].type != AlienFX_A_Color) {
					PrepareAndSend(COMMV1.setTempo, sizeof(COMMV1.setTempo), {
						           {2,(byte) (((UINT) act->act[0].tempo << 3 & 0xff00) >> 8)},
								   {3,(byte) ((UINT) act->act[0].tempo << 3 & 0xff)},
								   {4,(byte) (((UINT) act->act[0].time << 5 & 0xff00) >> 8)},
								   {5,(byte) ((UINT) act->act[0].time << 5 & 0xff)}});
				}
				for (size_t ca = 0; ca < act->act.size(); ca++) {
					vector<pair<byte, byte>> *mods;
					switch (act->act[ca].type) {
					case AlienFX_A_Pulse:
					{
						mods = SetMaskAndColor(1 << act->index, 2, act->act[ca].r, act->act[ca].g, act->act[ca].b);
					} break;
					case AlienFX_A_Morph:
					{
						if (ca < act->act.size() - 1) {
							mods = SetMaskAndColor(1 << act->index, 1, act->act[ca].r, act->act[ca].g, act->act[ca].b, act->act[ca + 1].r, act->act[ca + 1].g, act->act[ca + 1].b);

						} else {
							mods = SetMaskAndColor(1 << act->index, 1, act->act[ca].r, act->act[ca].g, act->act[ca].b, act->act[0].r, act->act[0].g, act->act[0].b);
						}
					} break;
					default:
					{ //case AlienFX_A_Color:
						mods = SetMaskAndColor(1 << act->index, 3, act->act[ca].r, act->act[ca].g, act->act[ca].b);
					} //break;
					}
					res = PrepareAndSend(COMMV1.color, sizeof(COMMV1.color), mods);
				}
				Loop();
			} break;
			default: res = SetColor(act->index, act->act[0].r, act->act[0].g, act->act[0].b);
			}
		}
		return res;
	}

	bool Functions::SetPowerAction(vector<act_block> *act) {
		// Let's find power button (if any)
		act_block *pwr = NULL;
		for (vector<act_block>::iterator pwi = act->begin(); pwi != act->end(); pwi++)
			if (pwi->act[0].type == AlienFX_A_Power) {
				pwr = &(*pwi);
				break;
			}
		switch (version) {
		case API_L_V4:
		{
			// Need to flush query...
			if (inSet) UpdateColors();
			inSet = true;
			// Now set....
			if (pwr) {
				//memcpy(buffer, COMMV4.setPower, sizeof(COMMV4.setPower));
				for (BYTE cid = 0x5b; cid < 0x61; cid++) {
					// Init query...
					PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,cid},{4,4}});
					PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,cid},{4,1}});
					// Now set color by type...
					act_block tact{pwr->index};
					switch (cid) {
					case 0x5b: // Alarm
						tact.act.push_back(pwr->act[0]);// afx_act{AlienFX_A_Power, 3, 0x64, pwr->act[0].r, pwr->act[0].g, pwr->act[0].b});
						tact.act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, 0, 0, 0});
						break;
					case 0x5c: // AC power
						tact.act.push_back(pwr->act[0]);// afx_act{AlienFX_A_Color, 0, 0, pwr->act[0].r, act[index].act[0].g, act[index].act[0].b});
						tact.act[0].type = AlienFX_A_Color;
						break;
					case 0x5d: // Charge
						tact.act.push_back(pwr->act[0]);// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act[0].r, act[index].act[0].g, act[index].act[0].b});
						tact.act.push_back(pwr->act[1]);// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act[1].r, act[index].act[1].g, act[index].act[1].b});
						break;
					case 0x5e: // Low batt
						tact.act.push_back(pwr->act[1]);// afx_act{AlienFX_A_Power, 3, 0x64, act[index].act[1].r, act[index].act[1].g, act[index].act[1].b});
						tact.act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, 0, 0, 0});
						break;
					case 0x5f: // Batt power
						tact.act.push_back(pwr->act[1]);// afx_act{AlienFX_A_Color, 0, 0, act[index].act[1].r, act[index].act[1].g, act[index].act[1].b});
						tact.act[0].type = AlienFX_A_Color;
						break;
					case 0x60: // System sleep?
						tact.act.push_back(pwr->act[1]);// afx_act{AlienFX_A_Color, 0, 0, act[index].act[1].r, act[index].act[1].g, act[index].act[1].b});
						tact.act[0].type = AlienFX_A_Color;
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
					if (nc->act[0].type != AlienFX_A_Power) {
						SetAction(&(*nc));
					}
				PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,0x61},{4,2}});
				PrepareAndSend(COMMV4.setPower, sizeof(COMMV4.setPower), {{6,0x61},{4,6}});
			}
			UpdateColors();

			BYTE res = 0;
			int count = 0;
			while ((res = IsDeviceReady()) && res != 255 && count < 20) {
				Sleep(50);
				count++;
			}
			while (!IsDeviceReady()) Sleep(50);
			// Close set
			//buffer[2] = 0x21; buffer[4] = 0x1; buffer[5] = 0xff; buffer[6] = 0xff;
			//HidD_SetOutputReport(devHandle, buffer, length);
			Reset();
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			if (!inSet) Reset();

			// 08 01 - load on boot
			for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++)
				if (nc->act[0].type != AlienFX_A_Power)
					SavePowerBlock(1, *nc, false);

			if (!pwr || act->size() > 1) {
				PrepareAndSend(COMMV1.save, sizeof(COMMV1.save));
				Reset();
			}

			if (pwr) {
				//DWORD invMask = ~((1 << index));// | 0x8000); // what is 8000? Macro?
				chain = 1;
				// 08 02 - standby
				act_block tact{pwr->index,
							   {{AlienFX_A_Morph, 0 , 0, pwr->act[0].r, pwr->act[0].g, pwr->act[0].b},
								{2,0,0,0,0,0}}};
				//SavePowerBlock(2, index, act, false);
				//act = {{AlienFX_A_Morph, 0 , 0, 0, 0, 0},
				//	   {2,0,0,Red,Green,Blue}};
				SavePowerBlock(2, tact, true, true);
				tact = {pwr->index,
						{{AlienFX_A_Color, 0 , 0, pwr->act[0].r, pwr->act[0].g, pwr->act[0].b},
						{0,0,0,0,0,0}}};
				// 08 05 - AC power
				SavePowerBlock(5, tact, true);
				tact = {pwr->index,
						{{AlienFX_A_Morph, 0 , 0, pwr->act[0].r, pwr->act[0].g, pwr->act[0].b},
						{0,0,0,pwr->act[1].r, pwr->act[1].g, pwr->act[1].b}}};
				// 08 06 - charge
				SavePowerBlock(6, tact, false);
				tact = {pwr->index,
						{{AlienFX_A_Morph, 0 , 0, pwr->act[1].r, pwr->act[1].g, pwr->act[1].b},
						{0,0,0,pwr->act[0].r, pwr->act[0].g, pwr->act[0].b}}};
				SavePowerBlock(6, tact, true);
				// 08 07 - Battery
				tact = {pwr->index,
						{{AlienFX_A_Color, 0 , 0, pwr->act[1].r, pwr->act[1].g, pwr->act[1].b},
						{2,0,0,0,0,0}}};
				SavePowerBlock(7, tact, true, true);
				// 08 08 - battery
				SavePowerBlock(8, tact, true);
				// 08 09 - batt critical
				tact.act[0].type = AlienFX_A_Pulse;
				SavePowerBlock(9, tact, true);
			}
			// fix for massive light change
			for (vector<act_block>::iterator nc = act->begin(); nc != act->end(); nc++)
				if (nc->act[0].type != AlienFX_A_Power) {
					SetAction(&(*nc));
				}

			if (pwr && act->size() == 1) {
				// Fix for immediate power button change
				int pind = powerMode ? 0 : 1;
				SetColor(pwr->index, pwr->act[pind].r, pwr->act[pind].g, pwr->act[pind].b);
			} 
			//else {
			//	PrepareAndSend(COMMV1.apply, sizeof(COMMV1.apply));
			//}
			UpdateColors();
		} break;
		default:
			// can't set action for old, just use color
			if (pwr)
				SetColor(pwr->index, pwr->act[0].r, pwr->act[0].g, pwr->act[0].b);
		}
		return true;
	}

	bool Functions::ToggleState(BYTE brightness, vector<mapping*> *mappings, bool power) {

		bright = ((UINT) brightness * 0x64) / 0xff;
		switch (version) {
		case API_L_V7:
			PrepareAndSend(COMMV7.status, sizeof(COMMV7.status));
			return PrepareAndSend(COMMV7.control, sizeof(COMMV7.control), {{6,bright}});
		case API_L_V6:
			if (!brightness)
				for (int i = 0; i < mappings->size(); i++) {
					mapping* cur = mappings->at(i);
					if (cur->devid == pid) {
						if (!cur->flags || power)
							SetColor(cur->lightid, 0, 0, 0);
					}
				}
			break;
		case API_L_V5:
		{
			if (inSet) { 
				UpdateColors();
				Reset();
			}
			PrepareAndSend(COMMV5.turnOnInit, sizeof(COMMV5.turnOnInit));
			PrepareAndSend(COMMV5.turnOnInit2, sizeof(COMMV5.turnOnInit2));
			return PrepareAndSend(COMMV5.turnOnSet, sizeof(COMMV5.turnOnSet), {{4,brightness}});
		} break;
		case API_L_V4:
		{
			if (inSet) UpdateColors();
			PrepareAndSend(COMMV4.prepareTurn, sizeof(COMMV4.prepareTurn));
			vector<pair<byte, byte>> mods{{3,(byte)(0x64 - bright)}};
			byte pos = 6, pindex = 0;
			for (int i = 0; i < mappings->size(); i++) {
				mapping* cur = mappings->at(i);
				if (cur->devid == pid && pos < length)
					if (!cur->flags || power) {
						mods.push_back({pos,(byte)cur->lightid});
						pos++; pindex++;
					}
			}
			mods.push_back({5,pindex});
			return PrepareAndSend(COMMV4.turnOn, sizeof(COMMV4.turnOn), mods);
		} break;
		case API_L_V3: case API_L_V1: case API_L_V2:
		{
			int res = PrepareAndSend(COMMV1.reset, sizeof(COMMV1.reset), {{2,brightness? 4 : power? 3 : 1}});
			AlienfxWaitForReady();
			return res;
		} break;
		case API_L_ACPI:
			if (!brightness)
				// it should be SetMode here, but it only have 10 grades, so do software.
				for (int i = 0; i < mappings->size(); i++) {
					mapping* cur = mappings->at(i);
					if (cur->devid == pid) {
						if (!cur->flags || power)
							SetColor(cur->lightid, 0, 0, 0);
					}
				}
			break;
		}
		return false;
	}

	bool Functions::SetGlobalEffects(byte effType, byte tempo, afx_act act1, afx_act act2) {
		switch (version) {
		case API_L_V5:
		{
			if (!inSet) Reset();
			PrepareAndSend(COMMV5.setEffect, sizeof(COMMV5.setEffect), {
						   {2,effType}, {3,tempo},
						   {10,act1.r}, {11,act1.g}, {12,act1.b},
						   {13,act2.r}, {14,act2.g}, {15,act2.b},
						   {16,1}});
			UpdateColors();
			return true;
		} break;
		default: return true;
		}
		return false;
	}

	BYTE Functions::AlienfxGetDeviceStatus() {
		byte ret = 0;
		byte buffer[MAX_BUFFERSIZE]{reportID};
		switch (version) {
		case API_L_V5:
		{
			//memcpy(buffer, COMMV5.status, sizeof(COMMV5.status));
			//HidD_SetFeature(devHandle, buffer, length);
			PrepareAndSend(COMMV5.status, sizeof(COMMV5.status));
			//buffer[0] = 0xcc;
			if (HidD_GetFeature(devHandle, buffer, length))
				ret = buffer[2];
		} break;
		case API_L_V4:
		{
			if (HidD_GetInputReport(devHandle, buffer, length))
				ret = buffer[2];
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			PrepareAndSend(COMMV1.status, sizeof(COMMV1.status));

			buffer[0] = 0x01;
			HidD_GetInputReport(devHandle, buffer, length);

			if (buffer[0] == 0x01)
				ret = 0x06;
			else ret = buffer[0];
		} break;
		}
#ifdef _DEBUG
		if (ret == 0) {
			OutputDebugString(TEXT("System hangs!\n"));
		}
#endif
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
		//OutputDebugString((wstring(L"AWFB count - ") + to_wstring(i) + L"\n").c_str());
		return status;
	}

	BYTE Functions::IsDeviceReady() {
		int status;
		switch (version) {
		case API_L_V5:
			status = AlienfxGetDeviceStatus();
			return status == ALIENFX_V5_STARTCOMMAND || status == ALIENFX_V5_INCOMMAND;
		case API_L_V4:
			status = AlienfxGetDeviceStatus();
			if (status)
				return status == ALIENFX_V4_READY || status == ALIENFX_V4_WAITUPDATE || status == ALIENFX_V4_WASON;
			else
				return -1;
		case API_L_V3: case API_L_V2: case API_L_V1:
			switch (AlienfxGetDeviceStatus()) {
			case ALIENFX_V2_READY:
				return 1;
			case ALIENFX_V2_BUSY:
				Reset();
				return AlienfxWaitForReady() == ALIENFX_V2_READY;
			case ALIENFX_V2_RESET:
				return AlienfxWaitForReady() == ALIENFX_V2_READY;
			}
			return 0;
		case API_L_ACPI:
			return !inSet;
		}
		return 1;
	}

	bool Functions::AlienFXClose() {
		bool result = false;
		if (length != API_L_ACPI && devHandle != NULL) {
			result = CloseHandle(devHandle);
		}
		return result;
	}

	bool Functions::AlienFXChangeDevice(int nvid, int npid, HANDLE acc) {
		int res;
		if (pid != (-1) && length != API_L_ACPI && devHandle != NULL)
			CloseHandle(devHandle);
		if (nvid == API_L_ACPI)
			res = AlienFXInitialize(acc);
		else
			res = AlienFXInitialize(nvid, npid);
		if (res != (-1)) {
			pid = npid;
			Reset();
			return true;
		}
		return false;
	}

	Mappings::~Mappings() {
		for (int i = 0; i < fxdevs.size(); i++) {
			fxdevs[i].dev->AlienFXClose();
			delete fxdevs[i].dev;
		}
		fxdevs.clear();
		groups.clear();
		mappings.clear();
		devices.clear();
	}

	vector<pair<WORD, WORD>> Mappings::AlienFXEnumDevices() {
		vector<pair<WORD, WORD>> pids;
		GUID guid;
		bool flag = false;
		HANDLE tdevHandle;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE) {
			return pids;
		}
		unsigned int dw = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

		while (!flag) {
			deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData)) {
				flag = true;
				continue;
			}
			dw++;
			DWORD dwRequiredSize = 0;
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL)) {
				continue;
			}
			std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA *)new char[dwRequiredSize]);
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL)) {
				std::wstring devicePath = deviceInterfaceDetailData->DevicePath;
				tdevHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				if (tdevHandle != INVALID_HANDLE_VALUE) {
					std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
					attributes->Size = sizeof(HIDD_ATTRIBUTES);
					if (HidD_GetAttributes(tdevHandle, attributes.get())) {
						for (unsigned i = 0; i < NUM_VIDS; i++) {
							if (attributes->VendorID == vids[i]) {
								PHIDP_PREPARSED_DATA prep_caps;
								HIDP_CAPS caps;
								HidD_GetPreparsedData(tdevHandle, &prep_caps);
								HidP_GetCaps(prep_caps, &caps);
								HidD_FreePreparsedData(prep_caps);

								if (caps.OutputReportByteLength || caps.Usage == 0xcc) {
#ifdef _DEBUG
									wchar_t buff[2048];
									swprintf_s(buff, 2047, L"Scan: VID: %#x, PID: %#x, Version: %d, Length: %d\n",
											   attributes->VendorID, attributes->ProductID, attributes->VersionNumber, attributes->Size);
									OutputDebugString(buff);
									wprintf(L"%s", buff);
#endif
									pids.push_back({attributes->VendorID,attributes->ProductID});
									break;
								}
							}
						}
					}
					CloseHandle(tdevHandle);
				}
			}
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return pids;
	}

	void Mappings::AlienFXAssignDevices(HANDLE acc, byte brightness, byte power) {
		vector<pair<WORD, WORD>> devList = AlienFXEnumDevices();
		for (int i = 0; i < fxdevs.size(); i++) {
			fxdevs[i].dev->AlienFXClose();
			delete fxdevs[i].dev;
		}
		if (acc) devList.push_back({0,0});
		fxdevs.clear();
		for (int i = 0; i < devList.size(); i++) {
			afx_device dev{new AlienFX_SDK::Functions()};
			int pid = -1;
			if (devList[i].second)
				pid = dev.dev->AlienFXInitialize(devList[i].first, devList[i].second);
			else
				pid = dev.dev->AlienFXInitialize(acc);
			if (pid != -1) {
				// attach device description...
				dev.desc = GetDeviceById(pid, dev.dev->GetVid());
				// now attach mappings...
				for (int j = 0; j < GetMappings()->size(); j++)
					if ((*GetMappings())[j]->devid == pid)
						dev.lights.push_back((*GetMappings())[j]);
				dev.dev->ToggleState(brightness, &mappings, power);
				fxdevs.push_back(dev);
			}
		}
	}

	devmap *Mappings::GetDeviceById(WORD devID, WORD vid) {
		for (int i = 0; i < devices.size(); i++)
			if (devices[i].devid == devID)
				// vid check, if any
				if (vid && devices[i].vid)
					if (vid == devices[i].vid)
						return &devices[i];
					else
						return nullptr;
				else
					return &devices[i];
		return nullptr;
	}

	mapping *Mappings::GetMappingById(WORD devID, WORD LightID) {
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i]->devid == devID && mappings[i]->lightid == LightID)
				return mappings[i];
		return nullptr;
	}

	vector<group> *Mappings::GetGroups() {
		return &groups;
	}

	void Mappings::AddMapping(WORD devID, WORD lightID, char *name, WORD flags) {
		mapping *map;
		if (!(map = GetMappingById(devID, lightID))) {
			WORD vid = 0;
			devmap *dev = GetDeviceById(devID);
			if (dev) vid = dev->vid;
			map = new mapping{vid,devID,lightID};
			mappings.push_back(map);
		}
		if (name != NULL)
			map->name = name;
		if (flags != -1)
			map->flags = flags;
	}

	group *Mappings::GetGroupById(DWORD gID) {
		for (int i = 0; i < groups.size(); i++)
			if (groups[i].gid == gID)
				return &groups[i];
		return nullptr;
	}


	void Mappings::LoadMappings() {
		//DWORD  dwDisposition;
		HKEY   hKey1;

		devices.clear();
		mappings.clear();
		groups.clear();

		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, NULL);// &dwDisposition);

		unsigned vindex = 0; mapping map; devmap dev;
		char kName[255], name[255], inarray[255];
		unsigned ret = 0;
		do {
			DWORD len = 255, lend = 255;
			ret = RegEnumValueA( hKey1, vindex, name, &len, NULL, NULL, (LPBYTE) inarray, &lend );
			// get id(s)...
			if (ret == ERROR_SUCCESS) {
				WORD lID = 0, dID = 0;
				vindex++;
				if (sscanf_s((char *) name, "%hd-%hd", &dID, &lID) == 2) {
					// light name
					AddMapping(dID, lID, inarray, -1);
					continue;
				}
				if (sscanf_s((char *) name, "Flags%hd-%hd", &dID, &lID) == 2) {
					// light flags
					AddMapping(dID, lID, NULL, (WORD)*(DWORD *)inarray);
					continue;
				}
				// New devID format
				if (sscanf_s((char *) name, "Dev#%hd_%hd", &dev.vid, &dev.devid) == 2) {
					dev.name = string(inarray);
					devices.push_back(dev);
					continue;
				}
			}
		} while (ret == ERROR_SUCCESS);
		vindex = 0;
		do {
			ret = RegEnumKeyA(hKey1, vindex, kName, 255);
			vindex++;
			if (ret == ERROR_SUCCESS) {
				WORD lID = 0, dID = 0;
				if (sscanf_s((char *) kName, "Light%hd-%hd", &dID, &lID) == 2) {
					DWORD nameLen = 255, flags;
					RegGetValueA(hKey1, kName, "Name", RRF_RT_REG_SZ, 0, name, &nameLen);
					nameLen = sizeof(DWORD);
					RegGetValueA(hKey1, kName, "Flags", RRF_RT_REG_DWORD, 0, &flags, &nameLen);
					AddMapping(dID, lID, name, (WORD)flags);
				}
			}
		} while (ret == ERROR_SUCCESS);
		vindex = 0;
		do {
			ret = RegEnumKeyA(hKey1, vindex, kName, 255);
			vindex++;
			if (ret == ERROR_SUCCESS) {
				DWORD gID = 0;
				if (sscanf_s((char *) kName, "Group%d", &gID) == 1) {
					DWORD nameLen = 255, *maps;
					RegGetValueA(hKey1, kName, "Name", RRF_RT_REG_SZ, 0, name, &nameLen);
					RegGetValueA(hKey1, kName, "Lights", REG_BINARY, 0, NULL, &nameLen);
					maps = new DWORD[nameLen / sizeof(DWORD)];
					RegGetValueA(hKey1, kName, "Lights", REG_BINARY, 0, maps, &nameLen);
					group nGroup{gID,string(name)};
					mapping *map;
					for (int i = 0; i < nameLen / sizeof(DWORD); i += 2) {
						if (map = GetMappingById(LOWORD(maps[i]), LOWORD(maps[i + 1])))
							nGroup.lights.push_back(map);
					}
					delete[] maps;
					groups.push_back(nGroup);
				}
			}
		} while (ret == ERROR_SUCCESS);
		RegCloseKey(hKey1);
	}

	void Mappings::SaveMappings() {
		//DWORD  dwDisposition;
		HKEY   hKey1, hKeyS;
		size_t numdevs = devices.size();
		size_t numlights = mappings.size();
		size_t numGroups = groups.size();
		if (numdevs == 0) return;

		// Remove all maps!
		RegDeleteTree(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"));

		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, NULL);// &dwDisposition);

		for (int i = 0; i < numdevs; i++) {
			//preparing name
			string name = "Dev#" + to_string(devices[i].vid) + "_" + to_string(devices[i].devid);

			RegSetValueExA(
				hKey1,
				name.c_str(),
				0,
				REG_SZ,
				(BYTE *) devices[i].name.c_str(),
				(DWORD) devices[i].name.size()
			);
		}

		for (int i = 0; i < numlights; i++) {
			// new format
			string name = "Light" + to_string(mappings[i]->devid) + "-" + to_string(mappings[i]->lightid);

			RegCreateKeyA(hKey1, name.c_str(), &hKeyS);
			RegSetValueExA(hKeyS, "Name", 0, REG_SZ, (BYTE *) mappings[i]->name.c_str(), (DWORD) mappings[i]->name.length());

			DWORD flags = mappings[i]->flags;
			RegSetValueExA( hKeyS, "Flags", 0, REG_DWORD, (BYTE*)&flags, sizeof(DWORD) );
			RegCloseKey(hKeyS);
		}

		for (int i = 0; i < numGroups; i++) {
			string name = "Group" + to_string(groups[i].gid);

			RegCreateKeyA(hKey1, name.c_str(), &hKeyS);
			RegSetValueExA(hKeyS, "Name", 0, REG_SZ, (BYTE *) groups[i].name.c_str(), (DWORD) groups[i].name.size());

			DWORD *grLights = new DWORD[groups[i].lights.size() * 2];

			for (int j = 0; j < groups[i].lights.size(); j++) {
				grLights[j * 2] = groups[i].lights[j]->devid;
				grLights[j * 2 + 1] = groups[i].lights[j]->lightid;
			}
			RegSetValueExA( hKeyS, "Lights", 0, REG_BINARY, (BYTE *) grLights, 2 * (DWORD) groups[i].lights.size() * sizeof(DWORD) );

			delete[] grLights;
			RegCloseKey(hKeyS);
		}

		RegCloseKey(hKey1);
	}

	std::vector<mapping*> *Mappings::GetMappings() {
		return &mappings;
	}

	int Mappings::GetFlags(WORD devid, WORD lightid) {
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i]->devid == devid && mappings[i]->lightid == lightid)
				return mappings[i]->flags;
		return 0;
	}

	void Mappings::SetFlags(WORD devid, WORD lightid, WORD flags) {
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i]->devid == devid && mappings[i]->lightid == lightid) {
				mappings[i]->flags = flags;
				return;
			}
	}

	std::vector<devmap> *Mappings::GetDevices() {
		return &devices;
	}

	int Functions::GetPID() {
		return pid;
	}

	int Functions::GetVid() {
		return vid;
	}

	int Functions::GetVersion() {
		return version;
		//switch (length) {
		//case API_L_V5: return 5; break;
		//case API_L_V4: return 4; break;
		//case API_L_V3: return 3; break;
		//case API_L_V2: return 2; break;
		//case API_L_V1: return 1; break;
		//default: return 0;
		//}
		return length;
	}
}