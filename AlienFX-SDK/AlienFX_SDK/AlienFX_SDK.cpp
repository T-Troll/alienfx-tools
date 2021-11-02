#define WIN32_LEAN_AND_MEAN
#include "AlienFX_SDK.h"
#include "alienfx-controls.h"
#include <iostream>
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
#ifndef NOACPILIGHTS
#include "alienfan-low.h"
#endif
}

#pragma comment(lib, "hid.lib")

namespace AlienFX_SDK {	

	void Functions::SetMaskAndColor(int index, byte *buffer, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2) {
		unsigned mask = 1 << index;
		buffer[2] = (byte) chain;
		buffer[3] = (mask & 0xFF0000) >> 16;
		buffer[4] = (mask & 0x00FF00) >> 8;
		buffer[5] = (mask & 0x0000FF);
		switch (version) {
		case API_L_V1:
			buffer[6] = r1;
			buffer[7] = g1;
			buffer[8] = b1;
			break;
		case API_L_V3:
			buffer[6] = r1;
			buffer[7] = g1;
			buffer[8] = b1;
			buffer[9] = r2;
			buffer[10] = g2;
			buffer[11] = b2;
			break;
		case API_L_V2:
			buffer[6] = (r1 & 0xf0) | ((g1 & 0xf0) >> 4); // 4-bit color!
			buffer[7] = (b1 & 0xf0) | ((r2 & 0xf0) >> 4);
			buffer[8] = (g2 & 0xf0) | ((b2 & 0xf0) >> 4); // 4-bit color!
			break;
		}

		return;
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
								switch (caps.OutputReportByteLength) {
								case 0: 
									length = caps.FeatureReportByteLength;
									version = 5;
									break;
								case 8: length = caps.OutputReportByteLength;
									version = 1;
									break;
								case 9: 
									length = caps.OutputReportByteLength;
									version = 2;
									break;
								case 12:
									length = caps.OutputReportByteLength;
									version = 3;
									break;
								case 34:
									length = caps.OutputReportByteLength;
									version = 4;
									break;
								case 65: // TEMPORARY - Monitors
									length = caps.OutputReportByteLength;
									version = 6;
									break;
								//default: length = caps.OutputReportByteLength;
								}

								this->vid = attributes->VendorID;
								pid = attributes->ProductID;
								flag = true;
#ifdef _DEBUG
								wchar_t buff[2048];
								swprintf_s(buff, 2047, L"Init: VID: %#x, PID: %#x, Version: %d, Length: %d\n",
										   attributes->VendorID, attributes->ProductID, attributes->VersionNumber, length);
								OutputDebugString(buff);
								cout << "Attributes - length: " << attributes->Size << ", version: " << attributes->VersionNumber << endl;
								wprintf(L"Path: %s\n%s", devicePath.c_str(), buff);
#endif
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
		if (acc && acc != INVALID_HANDLE_VALUE) {
			devHandle = acc;
			vid = pid = length = API_L_ACPI;
			version = 0;
			if (Reset()) {
				return pid;
			}
		}
		return -1;
	}

	void Functions::Loop() {
		byte buffer[MAX_BUFFERSIZE];
		ZeroMemory(buffer, length);
		switch (version) {
		case API_L_V5:
			memcpy(buffer, COMMV5.loop, sizeof(COMMV5.loop));
			HidD_SetFeature(devHandle, buffer, length);
			break;
		//case API_L_V4: {
		//	 //m15 require Input report as a confirmation, not output.
		//	 //WARNING!!! In latest firmware, this can provide up to 10sec(!) slowdown, so i disable status read. It works without it as well.
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	 //std::cout << "Status: 0x" << std::hex << (int) BufferN[2] << std::endl;
		//} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			memcpy(buffer, COMMV1.loop, sizeof(COMMV1.loop));
			HidD_SetOutputReport(devHandle, buffer, length);
			chain++;
		} break;
		}
		// DEBUG!
		//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, NULL, NULL);
	}

	bool Functions::Reset() {
		bool result = false;

		byte buffer[MAX_BUFFERSIZE] = {0};

		switch (version) {
		case API_L_V5:
		{
			memcpy(buffer, COMMV5.reset, sizeof(COMMV5.reset));
			result = HidD_SetFeature(devHandle, buffer, length);
		} break;
		case API_L_V4:
		{
			memcpy(buffer, COMMV4.reset, sizeof(COMMV4.reset));
			result = HidD_SetOutputReport(devHandle, buffer, length);
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			memcpy(buffer, COMMV1.reset, sizeof(COMMV1.reset));
			result = HidD_SetOutputReport(devHandle, buffer, length);
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
		//default: return false;
		}
		inSet = true;
		return result;
	}

	bool Functions::UpdateColors() {
		bool res = false;

		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (version) {
		case API_L_V5:
		{
			memcpy(buffer, COMMV5.update, sizeof(COMMV5.update));
			res = HidD_SetFeature(devHandle, buffer, length);
		} break;
		case API_L_V4:
		{
			memcpy(buffer, COMMV4.update, sizeof(COMMV4.update));
			res = HidD_SetOutputReport(devHandle, buffer, length);
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			memcpy(buffer, COMMV1.update, sizeof(COMMV1.update));
			res = HidD_SetOutputReport(devHandle, buffer, length);
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
		default: res=false;
		}
		//std::cout << "Update!" << std::endl;
		inSet = false;
		Sleep(5); // Fix for ultra-fast updates, or next command will fail sometimes.
		return res;

	}

	bool Functions::SetColor(unsigned index, byte r, byte g, byte b, bool loop) {
		bool val = false;
		if (!inSet)
			Reset();
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (version) {
		case API_L_V6:
		{
			FillMemory(buffer, MAX_BUFFERSIZE, 0xff);
			memcpy(buffer, COMMV6.colorSet, sizeof(COMMV6.colorSet));
			buffer[8] = 1 << index;
			buffer[9] = r;
			buffer[10] = g;
			buffer[11] = b;
			buffer[12] = bright;
		} break;
		case API_L_V5:
		{
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
			buffer[4] = index + 1;
			buffer[5] = r;
			buffer[6] = g;
			buffer[7] = b;
			int ret = HidD_SetFeature(devHandle, buffer, length);
			Loop();
			return ret;
		} break;
		case API_L_V4:
		{
			memcpy(buffer, COMMV4.colorSel, sizeof(COMMV4.colorSel));
			buffer[6] = index;
			val = HidD_SetOutputReport(devHandle, buffer, length);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));
			buffer[8] = r;
			buffer[9] = g;
			buffer[10] = b;
		} break;
		case API_L_V3: case API_L_V2:
		{
			memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
			SetMaskAndColor(index, buffer, r, g, b);
		} break;
		case API_L_V1:
		{
			memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
			SetMaskAndColor(index, buffer, r, g, b);

			if (index == 5) {
				buffer[1] = 0x83;
			}
		} break;
		case API_L_ACPI:
		{
			unsigned mask = 1 << index;
			val = SetAcpiColor(mask, r, g, b);
		} break;
		default: return false;
		}
		val = HidD_SetOutputReport(devHandle, buffer, length);
		if (loop) Loop();
		return val;
	}

	bool Functions::SetMultiLights(int numLights, UCHAR *lights, int r, int g, int b) {
		bool val = false;
		if (!inSet) Reset();
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (version) {
		case API_L_V6:
		{
			FillMemory(buffer, MAX_BUFFERSIZE, 0xff);
			memcpy(buffer, COMMV6.colorSet, sizeof(COMMV6.colorSet));
			for (int nc = 0; nc < numLights; nc++)
				buffer[8] |= 1 << lights[nc];
			buffer[9] = r;
			buffer[10] = g;
			buffer[11] = b;
			buffer[12] = bright;
			val = HidD_SetOutputReport(devHandle, buffer, length);
		} break;
		case API_L_V5:
		{
			memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
			int bPos = 4;
			for (int nc = 0; nc < numLights; nc++) {
				if (bPos < length) {
					buffer[bPos] = lights[nc] + 1;
					buffer[bPos + 1] = r;
					buffer[bPos + 2] = g;
					buffer[bPos + 3] = b;
					bPos += 4;
				} else {
					// Send command and clear buffer...
					val = HidD_SetFeature(devHandle, buffer, length);
					ZeroMemory(buffer, length);
					memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
					bPos = 4;
				}
			}
			if (bPos > 4)
				val = HidD_SetFeature(devHandle, buffer, length);
			Loop();
		} break;
		case API_L_V4:
		{
			memcpy(buffer, COMMV4.colorSel, sizeof(COMMV4.colorSel));
			buffer[5] = numLights;
			for (int nc = 0; nc < numLights; nc++)
				buffer[6 + nc] = lights[nc];
			val = HidD_SetOutputReport(devHandle, buffer, length);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));
			buffer[8] = r;
			buffer[9] = g;
			buffer[10] = b;
			val = HidD_SetOutputReport(devHandle, buffer, length);
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			// for older, you just need mix the mask!
			DWORD fmask = 0;
			memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
			SetMaskAndColor(0, buffer, r, g, b);
			for (int nc = 0; nc < numLights; nc++)
				fmask |= 1 << lights[nc];
			buffer[2] = (byte) chain;
			buffer[3] = (byte) ((fmask & 0xFF0000) >> 16);
			buffer[4] = (byte) ((fmask & 0x00FF00) >> 8);
			buffer[5] = (byte) (fmask & 0x0000FF);
			val = HidD_SetOutputReport(devHandle, buffer, length);
			Loop();
		} break;
		case API_L_ACPI:
		{
			byte fmask = 0;
			for (int nc = 0; nc < numLights; nc++)
				fmask |= 1 << lights[nc];
			val = SetAcpiColor(fmask, r, g, b);
		} break;
		//default:
		//{
		//	for (int nc = 0; nc < numLights; nc++)
		//		val = SetColor(lights[nc], r, g, b);
		//}
		}
		return val;
	}

	bool Functions::SetMultiColor(int size, UCHAR *lights, std::vector<vector<afx_act>> act, bool save) {
		byte buffer[MAX_BUFFERSIZE] = {0};
		bool val = true;
		switch (version) {
		case API_L_V5:
		{
			if (!inSet) Reset();
			memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
			int bPos = 4;
			for (int nc = 0; nc < size; nc++) {
				if (bPos + 4 < length) {
					buffer[bPos] = lights[nc] + 1;
					buffer[bPos + 1] = act[nc][0].r;
					buffer[bPos + 2] = act[nc][0].g;
					buffer[bPos + 3] = act[nc][0].b;
					bPos += 4;
				} else {
					// Send command and clear buffer...
					val = HidD_SetFeature(devHandle, buffer, length);
					ZeroMemory(buffer, length);
					memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
					bPos = 4;
				}
			}
			if (bPos > 4)
				val = HidD_SetFeature(devHandle, buffer, length);
			Loop();
		}break;
		case API_L_V4:
		{
			//if (save) {
			//	int pwi;
			//	for (pwi = 0; pwi < size; pwi++)
			//		if (act[pwi][0].type == AlienFX_A_Power) {
			//			//SetPowerAction(lights[pwi], act[pwi][0].r, act[pwi][0].g, act[pwi][0].b,
			//			//			   act[pwi][1].r, act[pwi][1].g, act[pwi][1].b);
			//			break;
			//		}
			//	if (pwi < size)
			//		SetPowerAction(lights[pwi], act[pwi][0].r, act[pwi][0].g, act[pwi][0].b,
			//					   act[pwi][1].r, act[pwi][1].g, act[pwi][1].b,
			//					   size, lights, &act);
			//}
			for (int nc = 0; nc < size; nc++)
				if (act[nc][0].type != AlienFX_A_Power)
					val = SetAction(lights[nc], act[nc]);
				else
					SetPowerAction(lights[nc], act[nc][0].r, act[nc][0].g, act[nc][0].b,
								   act[nc][1].r, act[nc][1].g, act[nc][1].b, size);
		} break;
		case API_L_V1: case API_L_V2: case API_L_V3:
		{
			if (save) {
				// Let's find - do we have power button?
				int pwi;
				for (pwi = 0; pwi < size; pwi++)
					if (act[pwi][0].type == AlienFX_A_Power) {
						break;
					}
				if (pwi < size)
					SetPowerAction(lights[pwi], act[pwi][0].r, act[pwi][0].g, act[pwi][0].b,
								   act[pwi][1].r, act[pwi][1].g, act[pwi][1].b,
								   size, lights, &act);
				else
					SetPowerAction(-1, 0, 0, 0,
								   0, 0, 0,
								   size, lights, &act);

			}
			for (int nc = 0; nc < size; nc++) {
				if (act[nc][0].type != AlienFX_A_Power)
					val = SetAction(lights[nc], act[nc]);
			}
		} break;
		default: //case API_L_ACPI:
		{
			for (int nc = 0; nc < size; nc++) {
				val = SetColor(lights[nc], act[nc][0].r, act[nc][0].g, act[nc][0].b);
			}
		} break;
		}
		return val;
	}

	bool Functions::SetAction(int index, std::vector<afx_act> act) {
		bool res = false;

		byte buffer[MAX_BUFFERSIZE] = {0};
		if (act.size() > 0) {
			if (!inSet) Reset();
			switch (version) {
			case API_L_V4:
			{
				int bPos = 3;
				memcpy(buffer, COMMV4.colorSel, sizeof(COMMV4.colorSel));
				buffer[6] = index;
				HidD_SetOutputReport(devHandle, buffer, length);
				ZeroMemory(buffer, length);
				memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));
				for (int ca = 0; ca < act.size(); ca++) {
					// 3 actions per record..
					buffer[bPos] = act[ca].type < AlienFX_A_Breathing ? act[ca].type : AlienFX_A_Morph;
					buffer[bPos + 1] = act[ca].time;
					buffer[bPos + 3] = 0;
					buffer[bPos + 4] = act[ca].tempo;
					buffer[bPos + 5] = act[ca].r;
					buffer[bPos + 6] = act[ca].g;
					buffer[bPos + 7] = act[ca].b;
					switch (act[ca].type) {
					case AlienFX_A_Color: buffer[bPos + 2] = 0xd0; buffer[bPos + 4] = 0xfa; break;
					case AlienFX_A_Pulse: buffer[bPos + 2] = 0xdc; break;
					case AlienFX_A_Morph: buffer[bPos + 2] = 0xcf; break;
					case AlienFX_A_Breathing: buffer[bPos + 2] = 0xdc; break;
					case AlienFX_A_Spectrum: buffer[bPos + 2] = 0x82; break;
					case AlienFX_A_Rainbow: buffer[bPos + 2] = 0xac; break;
					case AlienFX_A_Power: buffer[bPos + 2] = 0xe8; break;
					default: buffer[bPos + 2] = 0xd0; buffer[bPos + 4] = 0xfa; buffer[bPos] = AlienFX_A_Color;
					}
					bPos += 8;
					if (bPos + 8 >= length) {
						res = HidD_SetOutputReport(devHandle, buffer, length);
						ZeroMemory(buffer, length);
						memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));

						bPos = 3;
					}
				}
				if (bPos != 3) {
					res = HidD_SetOutputReport(devHandle, buffer, length);
				}
				return res;
			} break;
			case API_L_V3: case API_L_V2: case API_L_V1:
			{
				byte *tempBuffer = new byte[length];
				memcpy(tempBuffer, COMMV1.setTempo, sizeof(COMMV1.setTempo));
				tempBuffer[2] = (byte) (((UINT) act[0].tempo << 3 & 0xff00) >> 8);
				tempBuffer[3] = (byte) ((UINT) act[0].tempo << 3 & 0xff);
				tempBuffer[4] = (((UINT) act[0].time << 5 & 0xff00) >> 8);
				tempBuffer[5] = (byte) ((UINT) act[0].time << 5 & 0xff);
				HidD_SetOutputReport(devHandle, tempBuffer, length);
				memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
				for (size_t ca = 0; ca < act.size(); ca++) {
					switch (act[ca].type) {
					case AlienFX_A_Pulse:
					{
						buffer[1] = 0x2;
						SetMaskAndColor(index, buffer, act[ca].r, act[ca].g, act[ca].b);
						res = HidD_SetOutputReport(devHandle, buffer, length);
					} break;
					case AlienFX_A_Morph:
					{
						buffer[1] = 0x1;
						if (ca < act.size() - 1) {
							SetMaskAndColor(index, buffer, act[ca].r, act[ca].g, act[ca].b, act[ca + 1].r, act[ca + 1].g, act[ca + 1].b);
							res = HidD_SetOutputReport(devHandle, buffer, length);
						} else {
							SetMaskAndColor(index, buffer, act[ca].r, act[ca].g, act[ca].b, act[0].r, act[0].g, act[0].b);
							res = HidD_SetOutputReport(devHandle, buffer, length);
						}
					} break;
					default:
					{ //case AlienFX_A_Color:
						buffer[1] = 0x3;
						SetMaskAndColor(index, buffer, act[ca].r, act[ca].g, act[ca].b);
						res = HidD_SetOutputReport(devHandle, buffer, length);
					} //break;
					}
				}
				Loop();
			} break;
			default: res = SetColor(index, act[0].r, act[0].g, act[0].b);
			}
		}
		return res;
	}

	bool Functions::SetPowerAction(int index, BYTE Red, BYTE Green, BYTE Blue, BYTE Red2, BYTE Green2, BYTE Blue2,
								   int size, UCHAR *lights, std::vector<vector<afx_act>> *act) {
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (version) {
		case API_L_V4:
		{
			// Need to flush query...
			if (inSet) UpdateColors();
			inSet = true;
			// Now set....
			memcpy(buffer, COMMV4.setPower, sizeof(COMMV4.setPower));
			for (BYTE cid = 0x5b; cid < 0x61; cid++) {
				// Init query...
				buffer[6] = cid;
				buffer[4] = 4;
				HidD_SetOutputReport(devHandle, buffer, length);
				buffer[4] = 1;
				HidD_SetOutputReport(devHandle, buffer, length);
				// Now set color by type...
				std::vector<afx_act> act;
				switch (cid) {
				case 0x5b: // Alarm
					act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, Red, Green, Blue});
					act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, 0, 0, 0});
					SetAction(index, act);
					break;
				case 0x5c: // AC power
					act.push_back(afx_act{AlienFX_A_Color, 0, 0, Red, Green, Blue});
					SetAction(index, act);
					break;
				case 0x5d: // Charge
					act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, Red, Green, Blue});
					act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, Red2, Green2, Blue2});
					SetAction(index, act);
					break;
				case 0x5e: // Low batt
					act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, Red2, Green2, Blue2});
					act.push_back(afx_act{AlienFX_A_Power, 3, 0x64, 0, 0, 0});
					SetAction(index, act);
					break;
				case 0x5f: // Batt power
					act.push_back(afx_act{AlienFX_A_Color, 0, 0, Red2, Green2, Blue2});
					SetAction(index, act);
					break;
				case 0x60: // System sleep?
					act.push_back(afx_act{AlienFX_A_Color, 0, 0, Red2, Green2, Blue2});
					SetAction(index, act);
				}
				// And finish
				buffer[4] = 2;
				HidD_SetOutputReport(devHandle, buffer, length);
			}
			// Now (default) color set, if needed...
			//if (size > 1) {
			//	buffer[2] = 0x21; 
			//	buffer[4] = 4; 
			//	buffer[6] = 0x61;
			//	HidD_SetOutputReport(devHandle, buffer, length);
			//	buffer[4] = 1;
			//	HidD_SetOutputReport(devHandle, buffer, length);
			//	// Default color set here...
			//	for (int nc = 0; nc < size; nc++)
			//		if (lights[nc] != index) {
			//			SetAction(lights[nc], act->at(nc));
			//		}
			//	buffer[4] = 2;
			//	HidD_SetOutputReport(devHandle, buffer, length);
			//	buffer[4] = 6;
			//	HidD_SetOutputReport(devHandle, buffer, length);
			//}
			UpdateColors();

			BYTE res = 0;
			int count = 0;
			while ((res = IsDeviceReady()) && res != 255 && count < 20) {
				Sleep(50);
				count++;
			}
			while (!IsDeviceReady()) Sleep(100);
			// Close set
			//buffer[2] = 0x21; buffer[4] = 0x1; buffer[5] = 0xff; buffer[6] = 0xff;
			//HidD_SetOutputReport(devHandle, buffer, length);
			Reset();
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1:
		{
			if (!inSet) Reset();
			//now prepare buffers, we need 2 more for PreSave/Save
			byte *buf_presave = new byte[length],
				*buf_save = new byte[length];
			ZeroMemory(buf_presave, length);
			ZeroMemory(buf_save, length);
			memcpy(buf_presave, COMMV1.saveGroup, sizeof(COMMV1.saveGroup));
			memcpy(buf_save, COMMV1.save, sizeof(COMMV1.save));
			memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));

			// 08 01 - load on boot
			buf_presave[2] = 0x1;
			//AlienfxWaitForReady();
			for (int nc = 0; nc < size; nc++) {
				if (lights[nc] != index) {
					HidD_SetOutputReport(devHandle, buf_presave, length);
					//SetAction(lights[nc], act->at(nc));
					SetColor(lights[nc], act->at(nc)[0].r, act->at(nc)[0].g, act->at(nc)[0].b, false);
					HidD_SetOutputReport(devHandle, buf_presave, length);
					Loop();
				}
			}

			if (size > 1) {
				HidD_SetOutputReport(devHandle, buf_save, length);
				Reset();
			}

			if (index >= 0) {
				DWORD invMask = ~((1 << index));// | 0x8000); // what is 8000? Macro?
				// 08 02 - standby
				buf_presave[2] = 0x2;
				buffer[1] = 0x1;
				buffer[2] = 0x1;
				SetMaskAndColor(index, buffer, Red, Green, Blue);
				AlienfxWaitForReady();
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);
				SetMaskAndColor(index, buffer, 0, 0, 0, Red, Green, Blue);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_presave, length);
				// now inverse mask... let's constant.
				ZeroMemory(buffer, length);
				memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
				buffer[2] = 0x2;
				buffer[3] = (byte) ((invMask & 0xFF0000) >> 16);
				buffer[4] = (byte) ((invMask & 0x00FF00) >> 8);
				buffer[5] = (byte) (invMask & 0x0000FF);
				HidD_SetOutputReport(devHandle, buffer, length);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_save, length);
				Reset();
				// 08 05 - AC power
				buf_presave[2] = 0x5;
				SetMaskAndColor(index, buffer, Red, Green, Blue);
				buffer[2] = 0x1;
				AlienfxWaitForReady();
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_save, length);
				Reset();
				// 08 06 - charge
				buf_presave[2] = 0x6;
				buffer[1] = 0x1;
				buffer[2] = 0x1;
				SetMaskAndColor(index, buffer, Red, Green, Blue, Red2, Green2, Blue2);
				AlienfxWaitForReady();
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);
				SetMaskAndColor(index, buffer, Red2, Green2, Blue2, Red, Green, Blue);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_save, length);
				Reset();
				// 08 07 - Battery
				buf_presave[2] = 0x7;
				buffer[1] = 0x3;
				buffer[2] = 0x1;
				SetMaskAndColor(index, buffer, Red2, Green2, Blue2);
				AlienfxWaitForReady();
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);

				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_presave, length);
				// now inverse mask... let's constant.
				ZeroMemory(buffer, length);
				memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
				buffer[2] = 0x2;
				buffer[3] = (byte) ((invMask & 0xFF0000) >> 16);
				buffer[4] = (byte) ((invMask & 0x00FF00) >> 8);
				buffer[5] = (byte) (invMask & 0x0000FF);
				HidD_SetOutputReport(devHandle, buffer, length);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_save, length);
				Reset();
				// 08 08 - battery
				buf_presave[2] = 0x8;
				SetMaskAndColor(index, buffer, Red2, Green2, Blue2);
				buffer[1] = 0x3;
				buffer[2] = 0x1;
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_save, length);
				Reset();
				// 08 09 - batt critical
				buf_presave[2] = 0x9;
				buffer[1] = 0x2;
				HidD_SetOutputReport(devHandle, buf_presave, length);
				HidD_SetOutputReport(devHandle, buffer, length);
				HidD_SetOutputReport(devHandle, buf_presave, length);
				Loop();
				HidD_SetOutputReport(devHandle, buf_save, length);
				Reset();
			}
				// fix for immediate change
			for (int nc = 0; nc < size; nc++) {
				if (lights[nc] != index) {
					SetAction(lights[nc], act->at(nc));
				}
			}
			if (size > 1) {
				ZeroMemory(buffer, length);
				memcpy(buffer, COMMV1.apply, sizeof(COMMV1.apply));
				HidD_SetOutputReport(devHandle, buffer, length);
				UpdateColors();
			}

			if (index >= 0) {
				SetColor(index, Red, Green, Blue);
				UpdateColors();
				Reset();
			}

			delete[] buf_presave; delete[] buf_save;
		} break;
		default:
			// can't set action for old, just use color
			SetColor(index, Red, Green, Blue);
		}
		return true;
	}

	bool Functions::ToggleState(BYTE brightness, vector<mapping*> *mappings, bool power) {
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (version) {
		case API_L_V6:
			// Brigtness is global, so set it and update
			bright = ((UINT) brightness * 0x64) / 0xff;
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
			memcpy(buffer, COMMV5.turnOnInit, sizeof(COMMV5.turnOnInit));
			HidD_SetFeature(devHandle, buffer, length);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV5.turnOnInit2, sizeof(COMMV5.turnOnInit2));
			HidD_SetFeature(devHandle, buffer, length);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV5.turnOnSet, sizeof(COMMV5.turnOnSet));
			buffer[4] = brightness; // 00..ff
			return HidD_SetFeature(devHandle, buffer, length);
		} break;
		case API_L_V4:
		{
			if (inSet) UpdateColors();
			memcpy(buffer, COMMV4.prepareTurn, sizeof(COMMV4.prepareTurn));
			HidD_SetOutputReport(devHandle, buffer, length);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV4.turnOn, sizeof(COMMV4.turnOn));
			buffer[3] = 0x64 - (((UINT) brightness) * 0x64 / 0xff); // 00..64
			byte pos = 6, pindex = 0;
			for (int i = 0; i < mappings->size(); i++) {
				mapping* cur = mappings->at(i);
				if (cur->devid == pid && pos < length)
					if (!cur->flags || power) {
						buffer[pos] = (byte) cur->lightid;
						pos++; pindex++;
					}
			}
			buffer[5] = pindex;
			return HidD_SetOutputReport(devHandle, buffer, length);
		} break;
		case API_L_V3: case API_L_V1: case API_L_V2:
		{
			int res;
			memcpy(buffer, COMMV1.reset, sizeof(COMMV1.reset));
			if (!brightness) {
				if (power)
					buffer[2] = 0x3;
				else
					buffer[2] = 0x1;
			}

			res = HidD_SetOutputReport(devHandle, buffer, length);
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

	bool Functions::SetGlobalEffects(byte effType, int tempo, afx_act act1, afx_act act2) {
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (version) {
		case API_L_V5:
		{
			if (!inSet) Reset();
			memcpy(buffer, COMMV5.setEffect, sizeof(COMMV5.setEffect));
			buffer[2] = effType;
			// 0-f
			buffer[3] = (BYTE) tempo;
			//buffer[9] = 0;
			// colors...
			buffer[10] = act1.r;
			buffer[11] = act1.g;
			buffer[12] = act1.b;
			buffer[13] = act2.r;
			buffer[14] = act2.g;
			buffer[15] = act2.b;
			// ???? 0 for rainbow, 1 for breath, 5 for color.
			buffer[16] = 0x1;
			HidD_SetFeature(devHandle, buffer, length);
			UpdateColors();
			return true;
		} break;
		default: return true;
		}
		return false;
	}

	BYTE Functions::AlienfxGetDeviceStatus() {
		byte ret = 0;
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (version) {
		case API_L_V5:
		{
			memcpy(buffer, COMMV5.status, sizeof(COMMV5.status));
			HidD_SetOutputReport(devHandle, buffer, length);
			buffer[0] = 0xcc;
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
			memcpy(buffer, COMMV1.status, sizeof(COMMV1.status));
			HidD_SetOutputReport(devHandle, buffer, length);

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
		byte status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		for (int i = 0; i < 10 && (status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus()) != ALIENFX_V2_READY; i++) {
			if (status == ALIENFX_V2_RESET)
				return status;
			Sleep(50);
		}
		return status;
	}

	BYTE Functions::AlienfxWaitForBusy() {
		byte status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		for (int i = 0; i < 10 && (status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus()) != ALIENFX_V2_BUSY; i++) {
			if (status == ALIENFX_V2_RESET)
				return status;
			Sleep(50);
		}
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
		groups.clear();
		mappings.clear();
		devices.clear();
	}

	vector<pair<DWORD, DWORD>> Mappings::AlienFXEnumDevices() {
		vector<pair<DWORD, DWORD>> pids;
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
									pair<DWORD, DWORD> dev;
									dev.first = attributes->VendorID;
									dev.second = attributes->ProductID;
									pids.push_back(dev);
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

	devmap *Mappings::GetDeviceById(int devID, int vid) {
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

	mapping *Mappings::GetMappingById(int devID, int LightID) {
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i]->devid == devID && mappings[i]->lightid == LightID)
				return mappings[i];
		return nullptr;
	}

	vector<group> *Mappings::GetGroups() {
		return &groups;
	}

	void Mappings::AddMapping(int devID, int lightID, char *name, int flags) {
		mapping *map;
		if (!(map = GetMappingById(devID, lightID))) {
			map = new mapping;
			map->lightid = lightID;
			map->devid = devID;
			mappings.push_back(map);
			//delete map;
			//map = &mappings.back();
		}
		if (name != NULL)
			map->name = name;
		if (flags != -1)
			map->flags = flags;
	}

	group *Mappings::GetGroupById(int gID) {
		for (int i = 0; i < groups.size(); i++)
			if (groups[i].gid == gID)
				return &groups[i];
		return nullptr;
	}

	void Mappings::AddGroup(int gID, char *name, int lightNum, DWORD *lightlist) {
		mapping *map;
		group nGroup;
		nGroup.gid = gID;
		nGroup.name = string(name);
		for (int i = 0; i < lightNum / sizeof(DWORD); i += 2) {
			if (map = GetMappingById(lightlist[i], lightlist[i + 1]))
				nGroup.lights.push_back(map);
		}
		groups.push_back(nGroup);
	}

	void Mappings::LoadMappings() {
		DWORD  dwDisposition;
		HKEY   hKey1;

		devices.clear();
		mappings.clear();
		groups.clear();

		RegCreateKeyEx(HKEY_CURRENT_USER,
					   TEXT("SOFTWARE\\Alienfx_SDK"),
					   0,
					   NULL,
					   REG_OPTION_NON_VOLATILE,
					   KEY_ALL_ACCESS,
					   NULL,
					   &hKey1,
					   &dwDisposition);

		unsigned vindex = 0; mapping map; devmap dev;
		char kName[255], name[255], inarray[255];
		unsigned ret = 0;
		do {
			DWORD len = 255, lend = 255;
			ret = RegEnumValueA(
				hKey1,
				vindex,
				name,
				&len,
				NULL,
				NULL,
				(LPBYTE) inarray,
				&lend
			);
			// get id(s)...
			if (ret == ERROR_SUCCESS) {
				DWORD lID = 0, dID = 0;
				vindex++;
				if (sscanf_s((char *) name, "%d-%d", &dID, &lID) == 2) {
					// light name
					AddMapping(dID, lID, inarray, -1);
					continue;
				}
				if (sscanf_s((char *) name, "Flags%d-%d", &dID, &lID) == 2) {
					// light flags
					AddMapping(dID, lID, NULL, *(unsigned *) inarray);
					continue;
				}
				// New devID format
				if (sscanf_s((char *) name, "Dev#%d_%d", &dev.vid, &dev.devid) == 2) {
					std::string devname(inarray);
					dev.name = devname;
					devices.push_back(dev);
					continue;
				}
				// old devID format
				if (sscanf_s((char *) name, "Dev#%d", &dev.devid) == 1) {
					std::string devname(inarray);
					dev.vid = 0;
					dev.name = devname;
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
				DWORD lID = 0, dID = 0;
				if (sscanf_s((char *) kName, "Light%d-%d", &dID, &lID) == 2) {
					DWORD nameLen = 255, flags;
					RegGetValueA(hKey1, kName, "Name", RRF_RT_REG_SZ, 0, name, &nameLen);
					nameLen = sizeof(DWORD);
					RegGetValueA(hKey1, kName, "Flags", RRF_RT_REG_DWORD, 0, &flags, &nameLen);
					AddMapping(dID, lID, name, flags);
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
					DWORD nameLen = 255, maps[1024];
					RegGetValueA(hKey1, kName, "Name", RRF_RT_REG_SZ, 0, name, &nameLen);
					nameLen = 1024 * sizeof(DWORD);
					RegGetValueA(hKey1, kName, "Lights", RRF_RT_REG_DWORD, 0, &maps, &nameLen);
					AddGroup(gID, name, nameLen, maps);
				}
			}
		} while (ret == ERROR_SUCCESS);
		RegCloseKey(hKey1);
	}

	void Mappings::SaveMappings() {
		DWORD  dwDisposition;
		HKEY   hKey1, hKeyS;
		size_t numdevs = devices.size();
		size_t numlights = mappings.size();
		size_t numGroups = groups.size();
		if (numdevs == 0) return;

		// Remove all maps!
		RegDeleteTree(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfx_SDK"));

		RegCreateKeyEx(HKEY_CURRENT_USER,
					   TEXT("SOFTWARE\\Alienfx_SDK"),
					   0,
					   NULL,
					   REG_OPTION_NON_VOLATILE,
					   KEY_ALL_ACCESS,
					   NULL,
					   &hKey1,
					   &dwDisposition);
		//char name[1024];

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

			RegSetValueExA(
				hKeyS,
				"Name",
				0,
				REG_SZ,
				(BYTE *) mappings[i]->name.c_str(),
				(DWORD) mappings[i]->name.length()
			);

			RegSetValueExA(
				hKeyS,
				"Flags",
				0,
				REG_DWORD,
				(BYTE *) &mappings[i]->flags,
				sizeof(DWORD)
			);
			RegCloseKey(hKeyS);
		}

		for (int i = 0; i < numGroups; i++) {
			string name = "Group" + to_string(groups[i].gid);

			RegCreateKeyA(hKey1, name.c_str(), &hKeyS);

			RegSetValueExA(
				hKeyS,
				"Name",
				0,
				REG_SZ,
				(BYTE *) groups[i].name.c_str(),
				(DWORD) groups[i].name.size()
			);

			DWORD *grLights = new DWORD[groups[i].lights.size() * 2];

			for (int j = 0; j < groups[i].lights.size(); j++) {
				grLights[j * 2] = groups[i].lights[j]->devid;
				grLights[j * 2 + 1] = groups[i].lights[j]->lightid;
			}
			RegSetValueExA(
				hKeyS,
				"Lights",
				0,
				REG_BINARY,
				(BYTE *) grLights,
				2 * (DWORD) groups[i].lights.size() * sizeof(DWORD)
			);

			delete[] grLights;
			RegCloseKey(hKeyS);
		}

		RegCloseKey(hKey1);
	}

	std::vector<mapping*> *Mappings::GetMappings() {
		return &mappings;
	}

	int Mappings::GetFlags(int devid, int lightid) {
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i]->devid == devid && mappings[i]->lightid == lightid)
				return mappings[i]->flags;
		return 0;
	}

	void Mappings::SetFlags(int devid, int lightid, int flags) {
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