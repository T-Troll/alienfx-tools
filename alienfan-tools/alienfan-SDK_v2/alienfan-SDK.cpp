#define WIN32_LEAN_AND_MEAN

#include "alienfan-SDK.h"
#include "alienfan-controls.h"

#pragma comment(lib, "wbemuuid.lib")

//#define _TRACE_

namespace AlienFan_SDK {

	int Control::Percent(int val, int from) {
		return val * 100 / from;
	}

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
		if (m_WbemLocator->ConnectServer((BSTR)L"ROOT\\Microsoft\\Windows\\Storage", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_DiskService) == S_OK) {
			IEnumWbemClassObject* enum_obj = NULL;
			if (m_DiskService->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDisk", 0, NULL, &enum_obj) == S_OK) {
				enum_obj->Release();
			}
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
		if (m_InParamaters)
			m_InParamaters->Release();
		if (m_AWCCGetObj)
			m_AWCCGetObj->Release();
		if (m_WbemServices)
			m_WbemServices->Release();
		CoUninitialize();
	}

	int Control::CallWMIMethod(byte com, byte arg1, byte arg2) {
		VARIANT result{ VT_I4 };
		result.intVal = -1;
		if (sysType >= 0) {
			IWbemClassObject* m_outParameters = NULL;
			VARIANT parameters = { VT_I4 };
			parameters.uintVal = ALIENFAN_INTERFACE{ dev_controls[com], arg1, arg2 }.args;
			m_InParamaters->Put((BSTR)L"arg2", NULL, &parameters, 0);
			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				commandList[functionID[sysType][com]], 0, NULL, m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
				m_outParameters->Release();
			}
		}
		return result.intVal;
	}

	//void GetSSDTNames() {
	//	// Get sensor names array
	//	/*vector<string> senNames;
	//	DWORD size = GetSystemFirmwareTable('ACPI', 'TDSS', NULL, 0);
	//	byte* buf = new byte[size];
	//	size = GetSystemFirmwareTable('ACPI', 'TDSS', buf, size);
	//	for (DWORD i = 0; i < size - 5; i++) {
	//		char name[5]{ 0 };
	//		memcpy(name, &buf[i], 4);
	//		if (!strcmp(name, "_STR")) {
	//			DWORD sPos = i + 8;
	//			wstring senName = (wchar_t*)&buf[sPos];
	//			senNames.push_back(string(senName.begin(), senName.end()));
	//			i = sPos + (DWORD) senName.length() * 2;
	//		}
	//	}
	//	delete[] buf;*/
	//}

	void Control::EnumSensors(IEnumWbemClassObject* enum_obj, byte type) {
		IWbemClassObject* spInstance;
		ULONG uNumOfInstances = 0;
		BSTR instansePath{ (BSTR)L"__Path" }, valuePath{ (BSTR)L"Temperature" };
		VARIANT cTemp, instPath;
		string name;

		switch (type) {
		case 0:
			name = "ESIF sensor ";
			break;
		case 2:
			name = "SSD sensor ";
			instansePath = (BSTR)L"StorageReliabilityCounter";
			break;
		case 3:
			name = "AMD Sensor ";
			valuePath = (BSTR)L"Temp";
			break;
		case 4:
			valuePath = (BSTR)L"Value";
		}

		enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
		byte ind;
		for (ind = 1; uNumOfInstances; ind++) {
			if (type == 4) { // OHM sensors
				VARIANT type;
				spInstance->Get((BSTR)L"SensorType", 0, &type, 0, 0);
				if (type.bstrVal == wstring(L"Temperature")) {
					VARIANT vname;
					spInstance->Get((BSTR)L"Name", 0, &vname, 0, 0);
					wstring sname{ vname.bstrVal };
					name = string(sname.begin(), sname.end());
				}
				else {
					goto next;
				}
			}
			spInstance->Get(instansePath, 0, &instPath, 0, 0);
			spInstance->Get(valuePath, 0, &cTemp, 0, 0);
			spInstance->Release();
			if (cTemp.uintVal > 0)
				sensors.push_back({ { ind,type }, name + (type != 4 ? to_string(ind) : ""), instPath.bstrVal, valuePath });
			next:
			enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
		}
		enum_obj->Release();
#ifdef _TRACE_
		printf("%d sensors of #%d added, %d total\n", ind-1, type, (int)sensors.size());
#endif
	}

	bool Control::Probe() {
		if (m_WbemServices && m_WbemServices->GetObject((BSTR)L"AWCCWmiMethodFunction", NULL, nullptr, &m_AWCCGetObj, nullptr) == S_OK) {
#ifdef _TRACE_
			printf("AWCC section detected!\n");
#endif
			// need to get instance
			IEnumWbemClassObject* enum_obj;

			if (m_WbemServices->CreateInstanceEnum((BSTR)L"AWCCWmiMethodFunction", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				IWbemClassObject* spInstance;
				ULONG uNumOfInstances = 0;
				enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				spInstance->Get((BSTR)L"__Path", 0, &m_instancePath, 0, 0);
				spInstance->Release();
				enum_obj->Release();
				devFlags |= DEV_FLAG_AWCC;

				if (m_AWCCGetObj->GetMethod(commandList[2], NULL, nullptr, nullptr) == S_OK) {
#ifdef _TRACE_
					printf("G-Mode available\n");
#endif
					devFlags |= DEV_FLAG_GMODE;
				}

				// check system is compatible and fill inParams
				if (m_AWCCGetObj->GetMethod(commandList[functionID[0][getPowerID]], NULL, &m_InParamaters, nullptr) == S_OK && m_InParamaters)
					sysType = 0;
				else
					if (m_AWCCGetObj->GetMethod(commandList[functionID[1][getPowerID]], NULL, &m_InParamaters, nullptr) == S_OK && m_InParamaters)
						sysType = 1;

				if (sysType >= 0) {
#ifdef _TRACE_
					printf("Fan Control available, system type %d\n", sysType);
#endif
					devFlags |= DEV_FLAG_CONTROL;
					systemID = CallWMIMethod(getSysID, 2);
#ifdef _TRACE_
					printf("System ID = %d\n", systemID);
#endif
					int fIndex = 0, funcID;
					// Scan for available fans...
					while ((funcID = CallWMIMethod(getPowerID, fIndex)) < 0x100 && (funcID > 0 || funcID > 0x12f)) { // bugfix for 0x132 fan for R7
						fans.push_back({ (byte)(funcID & 0xff), 0xff });
						boosts.push_back(100);
						maxrpm.push_back(0);
						fIndex++;
					}
#ifdef _TRACE_
					printf("%d Fans found\n", fIndex);
#endif
					// AWCC temperature sensors.
					do {
						sensors.push_back({ {(byte)funcID, 1}, sensors.size() < 2 ? temp_names[sensors.size()] : "Sensor #" + to_string(sensors.size()) });
						fIndex++;
					} while ((funcID = CallWMIMethod(getPowerID, fIndex)) > 0x100 && funcID < 0x1A0);
#ifdef _TRACE_
					printf("%d AWCC Temperature sensors found\n", (int)sensors.size());
#endif
					// Power modes.
					powers.push_back(0); // Manual mode
					if (funcID > 0) {
						do {
							powers.push_back(funcID & 0xff);
							fIndex++;
						} while ((funcID = CallWMIMethod(getPowerID, fIndex)) && funcID > 0);
#ifdef _TRACE_
						printf("%d Power modes found\n", (int)powers.size());
#endif
					}

					// fan mappings...
					for (auto fan = fans.begin(); fan != fans.end(); fan++) {
						int senID = CallWMIMethod(getFanSensor, fan->id);
						for (byte i = 0; i < sensors.size(); i++)
							if (sensors[i].index == senID) {
								fan->type = i;
								break;
							}
					}
				}
				else
					return false;
			}
			// ESIF sensors
			if (m_WbemServices->CreateInstanceEnum((BSTR)L"EsifDeviceInformation", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				EnumSensors(enum_obj, 0);
			}
			// SSD sensors
			if (m_DiskService && m_DiskService->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDiskToStorageReliabilityCounter", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				EnumSensors(enum_obj, 2);
			}
			// AMD sensors
			if (m_WbemServices->CreateInstanceEnum((BSTR)L"WMI_ThermalQuery", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				EnumSensors(enum_obj, 3);
			}
			// OHM sensors
			if (m_OHMService && m_OHMService->CreateInstanceEnum((BSTR)L"Sensor", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				EnumSensors(enum_obj, 4);
			}
		}
		return devFlags;
	}

	int Control::GetFanRPM(int fanID) {
		if (fanID < fans.size())
			return CallWMIMethod(getFanRPM, (byte) fans[fanID].id);
		return -1;
	}
	int Control::GetFanPercent(int fanID) {
		if (fanID < fans.size())
			if (fanID < maxrpm.size() && maxrpm[fanID])
				return Percent(GetFanRPM(fanID),maxrpm[fanID]);
			else
				return CallWMIMethod(getFanPercent, (byte) fans[fanID].id);
		return -1;
	}
	int Control::GetFanBoost(int fanID, bool force) {
		if (fanID < fans.size()) {
			int value = CallWMIMethod(getFanBoost, (byte) fans[fanID].id);
			return force ? value : Percent(value, boosts[fanID]);
		}
		return -1;
	}
	int Control::SetFanBoost(int fanID, byte value, bool force) {
		if (fanID < fans.size()) {
			int finalValue = force ? value : (int) value * boosts[fanID] / 100;
			return CallWMIMethod(setFanBoost, (byte) fans[fanID].id, finalValue);
		}
		return -1;
	}
	int Control::GetTempValue(int TempID) {
		IWbemClassObject* sensorObject = NULL;
		IWbemServices* serviceObject = m_WbemServices;
		VARIANT temp;
		if (TempID < sensors.size()) {
			switch (sensors[TempID].type) {
			//case 0:// ESIF
			//	serviceObject = m_WbemServices;
			//	if (m_WbemServices->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
			//		sensorObject->Get((BSTR)L"Temperature", 0, &temp, 0, 0);
			//		sensorObject->Release();
			//		return temp.uintVal;
			//	}
			//	break;
			case 1: { // AWCC
				int awt = CallWMIMethod(getTemp, sensors[TempID].index);
				// Bugfix for AWCC temp - it can be up to 5000C!
				return awt > 200 ? -1 : awt;
			} break;
			case 2: // SSD
				serviceObject = m_DiskService;
				//if (m_DiskService->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
				//	sensorObject->Get((BSTR)L"Temperature", 0, &temp, 0, 0);
				//	sensorObject->Release();
				//	return temp.uintVal;
				//}
				break;
			//case 3: // AMD
			//	serviceObject = m_WbemServices;
			//	if (m_WbemServices->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
			//		sensorObject->Get((BSTR)L"Temp", 0, &temp, 0, 0);
			//		sensorObject->Release();
			//		return temp.uintVal;
			//	}
			//	break;
			case 4: // OHM
				serviceObject = m_OHMService;
				//if (m_OHMService->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
				//	sensorObject->Get((BSTR)L"Value", 0, &temp, 0, 0);
				//	sensorObject->Release();
				//	return (int)temp.fltVal;
				//}
				break;
			}
			if (serviceObject->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
				sensorObject->Get(sensors[TempID].valueName, 0, &temp, 0, 0);
				sensorObject->Release();
				return sensors[TempID].type == 4 ? (int)temp.fltVal : temp.uintVal;
			}
		}
		return -1;
	}
	int Control::Unlock() {
		return SetPower(0);
	}
	int Control::SetPower(byte level) {
		return CallWMIMethod(setPowerMode, level);
	}
	int Control::GetPower() {
		int pl = CallWMIMethod(getPowerMode);
		for (int i = 0; pl >= 0 && i < powers.size(); i++)
			if (powers[i] == pl)
				return i;
		return -1;
	}

	int Control::SetGMode(bool state)
	{
		if (devFlags & DEV_FLAG_GMODE) {
			if (state)
				SetPower(0xAB);
			return CallWMIMethod(setGMode, state);
		}
		return -1;
	}

	int Control::GetGMode() {
		if (devFlags & DEV_FLAG_GMODE) {
			if (GetPower() < 0)
				return 1;
			else
				return CallWMIMethod(getGMode);
		}
		return -1;
	}

	Lights::Lights(Control *ac) {
		if (ac && ac->GetDeviceFlags() & DEV_FLAG_AWCC &&
			ac->m_AWCCGetObj->GetMethod(colorList[0], NULL, &m_InParamaters, nullptr) == S_OK && m_InParamaters) {
			m_WbemServices = ac->m_WbemServices;
			m_instancePath = ac->m_instancePath;
			isActivated = true;
		}
	}

	int Lights::CallWMIMethod(byte com, byte* arg1) {
		VARIANT result{ VT_I4 };
		result.intVal = -1;
		if (m_InParamaters) {
			IWbemClassObject* m_outParameters = NULL;
			VARIANT parameters = { VT_ARRAY };
			parameters.pbVal = arg1;
			m_InParamaters->Put((BSTR)L"arg2", NULL, &parameters, 0);
			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				colorList[com], 0, NULL, m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
				m_outParameters->Release();
			}
		}
		return result.intVal;
	}

	int Lights::SetBrightness(byte brightness) {
		byte param[8]{ brightness };
		return CallWMIMethod(1, param);
	}

	int Lights::SetColor(byte id, byte r, byte g, byte b) {
		byte param[8]{ id, r, g, b };
		return CallWMIMethod(1, param);
	}
}
