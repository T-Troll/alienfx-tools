// alienfan-SDK.cpp : Defines the functions for the static library.
#define WIN32_LEAN_AND_MEAN

#include "alienfan-SDK.h"
#include "alienfan-controls.h"

#pragma comment(lib, "wbemuuid.lib")

//#define _TRACE_

namespace AlienFan_SDK {

	Control::Control() {

#ifdef _TRACE_
		printf("WMI activation started.\n");
#endif
		IWbemLocator* m_WbemLocator;
		CoInitializeEx(nullptr, COINIT::COINIT_MULTITHREADED);
		CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
			RPC_C_AUTHN_LEVEL_NONE, //RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr, EOAC_NONE, nullptr);

		CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&m_WbemLocator);
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\WMI", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_WbemServices);
		// Windows bug with disk drives list
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\Microsoft\\Windows\\Storage", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_DiskService);
		if (m_DiskService) {
			IWbemClassObject* spInstance = NULL;
			m_DiskService->GetObject((BSTR)L"MSFT_PhysicalDisk", NULL, nullptr, &spInstance, nullptr);
			spInstance->Release();
			m_DiskService->Release();
		}
		// End Windows bugfix
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\Microsoft\\Windows\\Storage\\Providers_v2", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_DiskService);
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\LibreHardwareMonitor", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_OHMService);
		m_WbemLocator->Release();
	}

	Control::~Control() {
		if (m_DiskService)
			m_DiskService->Release();
		if (m_OHMService)
			m_OHMService->Release();
		m_WbemServices->Release();
		CoUninitialize();
	}

	int Control::CallWMIMethod(ALIENFAN_COMMAND com, byte arg1, byte arg2) {
		BYTE operand[4]{ com.sub, arg1, arg2, 0 };
		IWbemClassObject* m_InParamaters = NULL, * m_outParameters = NULL;
		if (m_AWCCGetObj->GetMethod(commandList[com.com], NULL, &m_InParamaters, NULL) == S_OK) {
			VARIANT parameters = { 0 }, result{ 0 };
			parameters.vt = VT_I4;
			parameters.uintVal = *((DWORD*)operand);
			m_InParamaters->Put((BSTR)L"arg2", NULL, &parameters, 0);
			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				commandList[com.com], 0, NULL, m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
				m_InParamaters->Release();
				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
				m_outParameters->Release();
				m_outParameters = NULL;
				return result.uintVal;
			}
			m_InParamaters->Release();
		}
		return -1;
	}

	bool Control::Probe() {
		if (m_WbemServices && m_WbemServices->GetObject((BSTR)L"AWCCWmiMethodFunction", NULL, nullptr, &m_AWCCGetObj, nullptr) == S_OK) {
#ifdef _TRACE_
			printf("AWCC section detected!\n");
#endif
			// need to get instance
			IEnumWbemClassObject* enum_obj;
			m_WbemServices->CreateInstanceEnum((BSTR)L"AWCCWmiMethodFunction", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj);
			if (!enum_obj) return false;
			IWbemClassObject* spInstance;
			ULONG uNumOfInstances = 0;
			enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
			spInstance->Get((BSTR)L"__Path", 0, &m_instancePath, 0, 0);
			spInstance->Release();
			enum_obj->Release();
			IWbemClassObject* m_InParamaters = NULL;
			devFlags |= DEV_FLAG_AWCC;
			// now let's check methods
			if (m_AWCCGetObj->GetMethod(commandList[0], NULL, &m_InParamaters, nullptr) == S_OK) {
				m_InParamaters->Release();
				// Let's get device ID...
				systemID = CallWMIMethod(dev_controls.getSysID, 2);
				devFlags |= DEV_FLAG_INFO;
#ifdef _TRACE_
				printf("System information available, ID = %d!\n", systemID);
#endif
				int fIndex = 0, funcID = 0;
				// Scan for available fans...
				while ((funcID = CallWMIMethod(dev_controls.getPowerID, fIndex)) < 0x100 && funcID > 0 || funcID > 0x130) { // bugfix for 0x132 fan for R7
					fans.push_back(funcID & 0xff);
					boosts.push_back(100);
					maxrpm.push_back(0);
					fIndex++;
				}
#ifdef _TRACE_
				printf("%d Fans found\n", fIndex);
#endif

				int firstSenIndex = fIndex;
				// AWCC temperature sensors.
				string name;
				do {
					// Check temperature, disable if -1
					int sIndex = fIndex - firstSenIndex;
					if (CallWMIMethod(dev_controls.getTemp, (byte)funcID) > 0) {
						if (sIndex < 2/*temp_names.size()*/) {
							name = temp_names[sIndex];
						}
						else
							name = "Sensor #" + to_string(sIndex);
						sensors.push_back({ (short)funcID, name, 1 });
					}
					fIndex++;
				} while ((funcID = CallWMIMethod(dev_controls.getPowerID, fIndex)) > 0x100 && funcID < 0x1A0);
#ifdef _TRACE_
				printf("%d Temperature sensors found\n", fIndex - firstSenIndex);
#endif

				// Power modes.
				powers.push_back(0); // Unlocked power
				int firstPM = fIndex;
				if (funcID > 0) {
					do {
						powers.push_back(funcID & 0xff);
						fIndex++;
					} while ((funcID = CallWMIMethod(dev_controls.getPowerID, fIndex)) && funcID > 0);
					// Hidden power mode for Dell G-series
					//if (HIWORD(systemID) == 500)
					//	powers.push_back(0xAB);
#ifdef _TRACE_
					printf("%d Power modes found\n", (int)powers.size());
#endif
				}
			}
			if (m_AWCCGetObj->GetMethod(commandList[1], NULL, &m_InParamaters, nullptr) == S_OK) {
#ifdef _TRACE_
				printf("Fan control available!\n");
#endif
				m_InParamaters->Release();
				devFlags |= DEV_FLAG_CONTROL;
			}
			if (m_AWCCGetObj->GetMethod(commandList[2], NULL, &m_InParamaters, nullptr) == S_OK) {
#ifdef _TRACE_
				printf("G-Mode available!\n");
#endif
				m_InParamaters->Release();
				devFlags |= DEV_FLAG_GMODE;
			}
			// ESIF temperature sensors
			if (m_WbemServices->CreateInstanceEnum((BSTR)L"EsifDeviceInformation", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				// Get sensor names array
				vector<string> senNames;
				DWORD size = GetSystemFirmwareTable('ACPI', 'TDSS', NULL, 0);
				byte* buf = new byte[size];
				size = GetSystemFirmwareTable('ACPI', 'TDSS', buf, size);
				for (DWORD i = 0; i < size - 5; i++) {
					char name[5]{ 0 };
					memcpy(name, &buf[i], 4);
					if (!strcmp(name, "_STR")) {
						DWORD sPos = i + 8;
						wstring senName = (wchar_t*)&buf[sPos];
						senNames.push_back(string(senName.begin(), senName.end()));
						i = sPos + (DWORD) senName.length() * 2;
					}
				}
				delete[] buf;
				//m_WbemServices->CreateInstanceEnum((BSTR)L"EsifDeviceInformation", WBEM_FLAG_FORWARD_ONLY/*WBEM_FLAG_RETURN_IMMEDIATELY*/, NULL, &enum_obj);
				enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				int numESIF = 0;
				VARIANT instPath, cTemp;
				while (uNumOfInstances) {
					spInstance->Get((BSTR)L"__Path", 0, &instPath, 0, 0);
					spInstance->Get((BSTR)L"Temperature", 0, &cTemp, 0, 0);
					spInstance->Release();
					if (cTemp.uintVal > 0) {
						sensors.push_back({ (short)numESIF,
							numESIF < senNames.size() ? senNames[numESIF] : "ESIF sensor #" + to_string(numESIF + 1),
							0, instPath.bstrVal });
						numESIF++;
					}
					enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				}
				enum_obj->Release();
				//m_ESIFObject->Release();
#ifdef _TRACE_
				printf("ESIF data available, %d sensors (%d names) added!\n", numESIF, (int)senNames.size());
#endif
			}
//			if (m_WbemServices->GetObject((BSTR)L"AMD_ACPI", NULL, nullptr, &m_ESIFObject, nullptr) == S_OK) {
//				m_ESIFObject->Release();
//#ifdef _TRACE_
//				printf("AMD data available, %d sensors added!\n", 0);
//#endif
//			}
			if (m_DiskService && m_DiskService->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDiskToStorageReliabilityCounter", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				IWbemClassObject* spInstance;
				ULONG uNumOfInstances = 0;
				enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				short ssdsen = 0;
				VARIANT instPath;
				while (uNumOfInstances) {
					spInstance->Get((BSTR)L"StorageReliabilityCounter", 0, &instPath, 0, 0);
					sensors.push_back({ ssdsen, "SSD " + to_string(ssdsen + 1) + " Sensor", 2, instPath.bstrVal });
					spInstance->Release();
					enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
					ssdsen++;
				}
				enum_obj->Release();
#ifdef _TRACE_
				printf("Disk data available, %d sensors added!\n", ssdsen);
#endif
			}
			if (m_OHMService && m_OHMService->CreateInstanceEnum((BSTR)L"Sensor", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				//m_OHMService->CreateInstanceEnum((BSTR)L"Sensor", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj);
				enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				short numOHM = 0;
				VARIANT instPath, type, name;
				while (uNumOfInstances) {
					spInstance->Get((BSTR)L"__Path", 0, &instPath, 0, 0);
					spInstance->Get((BSTR)L"SensorType", 0, &type, 0, 0);
					spInstance->Get((BSTR)L"Name", 0, &name, 0, 0);
					spInstance->Release();
					wstring tn{ type.bstrVal };
					if (tn == L"Temperature") {
						wstring sname{ name.bstrVal };
						sensors.push_back({ numOHM,
							string(sname.begin(), sname.end()),
							4, instPath.bstrVal });
						numOHM++;
					}
					enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				}
				enum_obj->Release();
				//m_ESIFObject->Release();
#ifdef _TRACE_
				printf("LHM data available, %d sensors added!\n", numOHM);
#endif
			}
			return true;
		}
		return false;
	}

	int Control::GetFanRPM(int fanID) {
		if (fanID < fans.size())
			return CallWMIMethod(dev_controls.getFanRPM, (byte) fans[fanID]);
		return -1;
	}
	int Control::GetFanPercent(int fanID) {
		if (fanID < fans.size())
			if (fanID < maxrpm.size() && maxrpm[fanID])
				return GetFanRPM(fanID) * 100 / maxrpm[fanID];
			else
				return CallWMIMethod(dev_controls.getFanPercent, (byte) fans[fanID]);
		return -1;
	}
	int Control::GetFanBoost(int fanID, bool force) {
		if (fanID < fans.size()) {
			int value = CallWMIMethod(dev_controls.getFanBoost, (byte) fans[fanID]);
			return force ? value : value * 100 / boosts[fanID];
		}
		return -1;
	}
	int Control::SetFanBoost(int fanID, byte value, bool force) {
		if (fanID < fans.size()) {
			int finalValue = force ? value : (int) value * boosts[fanID] / 100;
			return CallWMIMethod(dev_controls.setFanBoost, (byte) fans[fanID], finalValue);
		}
		return -1;
	}
	int Control::GetTempValue(int TempID) {
		IWbemClassObject* sensorObject = NULL;
		if (TempID < sensors.size()) {
			switch (sensors[TempID].type) {
			case 1: // AWCC
				return CallWMIMethod(dev_controls.getTemp, (byte)sensors[TempID].senIndex);
				break;
			case 0: {// ESIF
				if (m_WbemServices->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
					VARIANT temp;
					sensorObject->Get((BSTR)L"Temperature", 0, &temp, 0, 0);
					sensorObject->Release();
					return temp.uintVal;
				}
			} break;
			case 2: // SSD
				if (m_DiskService && m_DiskService->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
					VARIANT temp;
					sensorObject->Get((BSTR)L"Temperature", 0, &temp, 0, 0);
					sensorObject->Release();
					return temp.uintVal;
				}
				break;
			case 4: {// OHM
				if (m_OHMService && m_OHMService->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
					VARIANT temp;
					sensorObject->Get((BSTR)L"Value", 0, &temp, 0, 0);
					sensorObject->Release();
					float t = temp.fltVal;
					return (int)temp.fltVal;
				}
			} break;
			//case 2: case 3: { // tempECDV
			//	PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			//	acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, sensors[TempID].senIndex);
			//	tempECDV1[7] = (sensors[TempID].type == 2) ? 'I' : '0';
			//	if (EvalAcpiMethod(acc, tempECDV1, (PVOID*)&res, acpiargs) && res) {
			//		int res_int = res->Argument[0].Argument;
			//		free(res);
			//		return res_int;
			//	}
			//} break;
			}
		}
		return -1;
	}
	int Control::Unlock() {
		return SetPower(0);
	}
	int Control::SetPower(byte level) {
		return CallWMIMethod(dev_controls.setPower, level);
	}
	int Control::GetPower() {
		int pl = CallWMIMethod(dev_controls.getPower);
		if (pl != 0xAB) // G-mode active
			for (int i = 0; pl >= 0 && i < powers.size(); i++)
				if (powers[i] == pl)
					return i;
		return -1;
	}
	//int Control::SetGPU(int power) {
	//	/*if (power >= 0 && power < 5 && devs[aDev].commandControlled) {
	//		return RunGPUCommand(dev_controls.setGPUPower.com, power << 4 | dev_controls.setGPUPower.sub);
	//	}*/
	//	return -1;
	//}

	int Control::SetGMode(bool state)
	{
		if (devFlags & DEV_FLAG_GMODE) {
			if (state)
				SetPower(0xAB);
			return CallWMIMethod(dev_controls.setGMode, state);
		}
		return -1;
	}

	int Control::GetGMode() {
		if (devFlags & DEV_FLAG_GMODE) {
			if (GetPower() < 0)
				return 1;
			else
				return CallWMIMethod(dev_controls.getGMode);
		}
		return -1;
	}

	Lights::Lights(Control *ac) {
		//acpi = ac;
		// Probe lights...
		//if (Prepare())
		//	activated = true;
	}
	bool Lights::Reset() {
		//PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		//if (!inCommand)
		//	Prepare();
		//if (EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.SRST", (PVOID *) &resName, NULL)) {
		//	free(resName);
		//	Update();
		//	return true;
		//}
		return false;
	}
	bool Lights::Prepare() {
		//PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		//if (!inCommand && EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.ICPC", (PVOID *) &resName, NULL)) {
		//	free(resName);
		//	inCommand = true;
		//	return true;
		//}
		return false;
	}
	bool Lights::Update() {
		//PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		//if (inCommand && EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.RCPC", (PVOID *) &resName, NULL)) {
		//	free(resName);
		//	inCommand = false;
		//	return true;
		//}
		return false;
	}
	bool Lights::SetColor(byte id, byte r, byte g, byte b) {
		//PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		//PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = NULL;
		//if (!inCommand) {
		//	Prepare();
		//}
		//acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, r);
		//acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, g);
		//acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, b);
		//acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, id);
		//if (EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.SETC", (PVOID *) &resName, acpiargs)) {
		//	free(resName);
		//	return true;
		//}
		return false;
	}
	bool Lights::SetMode(byte mode, bool onoff) {
		//PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		//PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = NULL;
		//if (!inCommand) {
		//	Prepare();
		//}
		//acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, mode);
		//acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, onoff);
		//if (EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.SETB", (PVOID *) &resName, acpiargs)) {
		//	free(resName);
		//	return true;
		//}
		return false;
	}
	bool Lights::IsActivated() {
		return activated;
	}
}
