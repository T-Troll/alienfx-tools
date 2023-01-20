#define WIN32_LEAN_AND_MEAN

#include "alienfan-SDK.h"
#include "alienfan-controls.h"

#pragma comment(lib, "wbemuuid.lib")

//#define _TRACE_

namespace AlienFan_SDK {

	//DWORD WINAPI DPTFInitFunc(LPVOID lpParam);

	//HANDLE updateAllowed = NULL;

	Control::Control() {

#ifdef _TRACE_
		printf("WMI activation started.\n");
#endif
		//if (!updateAllowed)
		//	updateAllowed = CreateEvent(NULL, true, true, NULL);
		//else
		//	SetEvent(updateAllowed);
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
		IEnumWbemClassObject* enum_obj = NULL;
		if (m_DiskService->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDisk", 0, NULL, &enum_obj) == S_OK) {
			//IWbemClassObject* spInstance;
			//ULONG uNumOfInstances = 0;
			//enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
			enum_obj->Release();
		}
		m_DiskService->Release();
		// End Windows bugfix
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\Microsoft\\Windows\\Storage\\Providers_v2", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_DiskService);
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\LibreHardwareMonitor", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_OHMService);
		m_WbemLocator->Release();
	}

	Control::~Control() {
		//ResetEvent(updateAllowed);
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
			parameters.uintVal = ALIENFAN_INTERFACE{ dev_controls[com], arg1, arg2, 0 }.args;
			m_InParamaters->Put((BSTR)L"arg2", NULL, &parameters, 0);
			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				commandList[functionID[sysType][com]], 0, NULL, m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
				m_outParameters->Release();
			}
		}
		return result.intVal;
	}

	void Control::EnumSensors(IEnumWbemClassObject* enum_obj, byte type) {
		IWbemClassObject* spInstance[64];
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
		case 4:
			valuePath = (BSTR)L"Value";
		}

		enum_obj->Next(3000, 64, spInstance, &uNumOfInstances);
		byte senID = 0;
		for (byte ind = 0; ind < uNumOfInstances; ind++) {
			if (type == 4) { // OHM sensors
				VARIANT type;
				spInstance[ind]->Get((BSTR)L"SensorType", 0, &type, 0, 0);
				if (type.bstrVal == wstring(L"Temperature")) {
					VARIANT vname;
					spInstance[ind]->Get((BSTR)L"Name", 0, &vname, 0, 0);
					wstring sname{ vname.bstrVal };
					name = string(sname.begin(), sname.end());
				}
				else {
					continue;
				}
			}
			spInstance[ind]->Get(instansePath, 0, &instPath, 0, 0);
			spInstance[ind]->Get(valuePath, 0, &cTemp, 0, 0);
			spInstance[ind]->Release();
			if (cTemp.uintVal > 0 || cTemp.fltVal > 0)
				sensors.push_back({ { senID++,type }, name + (type != 4 ? to_string(senID) : ""), instPath.bstrVal, valuePath });
		}
		enum_obj->Release();
#ifdef _TRACE_
		printf("%d sensors of #%d added, %d total\n", uNumOfInstances, type, (int)sensors.size());
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
				isAlienware = true;

				if (m_AWCCGetObj->GetMethod(commandList[2], NULL, nullptr, nullptr) == S_OK) {
#ifdef _TRACE_
					printf("G-Mode available\n");
#endif
					isGmode = true;
				}

				// check system type and fill inParams
				for (int type = 0; type < 2; type++)
					if (m_AWCCGetObj->GetMethod(commandList[functionID[type][getPowerID]], NULL, &m_InParamaters, nullptr) == S_OK && m_InParamaters) {
						sysType = type;
						break;
					}

				if (isSupported = (sysType >= 0)) {
#ifdef _TRACE_
					printf("Fan Control available, system type %d\n", sysType);
#endif
					systemID = CallWMIMethod(getSysID, 2);
#ifdef _TRACE_
					printf("System ID = %d\n", systemID);
#endif
					int fIndex = 0, funcID;
					// Scan for available fans...
					while ((funcID = CallWMIMethod(getPowerID, fIndex)) < 0x100 && (funcID > 0 || funcID > 0x12f)) { // bugfix for 0x132 fan for R7
						fans.push_back({ (byte)(funcID & 0xff), 0xff });
#ifdef _TRACE_
						printf("Fan ID=%x found\n", funcID);
#endif
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
#ifdef _TRACE_
							printf("Power ID=%x found\n", funcID);
#endif
							fIndex++;
						} while ((funcID = CallWMIMethod(getPowerID, fIndex)) && funcID > 0);
#ifdef _TRACE_
						printf("%d Power modes found\n", (int)powers.size());
#endif
					}

					// fan mappings...
					for (auto fan = fans.begin(); fan != fans.end(); fan++) {
						ALIENFAN_SEN_INFO sen = { (byte)CallWMIMethod(getFanSensor, fan->id), 1 };
						for (byte i = 0; i < sensors.size(); i++)
							if (sensors[i].sid == sen.sid) {
								fan->type = i;
								break;
							}
					}
				}
			}
			// ESIF sensors
			if (m_WbemServices->CreateInstanceEnum((BSTR)L"EsifDeviceInformation", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				EnumSensors(enum_obj, 0);
				//CreateThread(NULL, 0, DPTFInitFunc, this, 0, NULL);
			}
			//else
			//	DPTFdone = true;
			// SSD sensors
			if (/*m_DiskService && */m_DiskService->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDiskToStorageReliabilityCounter", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				EnumSensors(enum_obj, 2);
			}
			// AMD sensors
			//if (m_WbemServices->CreateInstanceEnum((BSTR)L"WMI_ThermalQuery", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
			//	EnumSensors(enum_obj, 3);
			//}
			// OHM sensors
			if (m_OHMService && m_OHMService->CreateInstanceEnum((BSTR)L"Sensor", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
				EnumSensors(enum_obj, 4);
			}
		}
		return isSupported;
	}

	int Control::GetFanRPM(int fanID) {
		return fanID < fans.size() ? CallWMIMethod(getFanRPM, (byte)fans[fanID].id) : -1;
		//if (fanID < fans.size())
		//	return CallWMIMethod(getFanRPM, (byte) fans[fanID].id);
		//return -1;
	}
	int Control::GetMaxRPM(int fanID)
	{
		return fanID < fans.size() ? CallWMIMethod(getMaxRPM, (byte)fans[fanID].id) : -1;
		//if (fanID < fans.size())
		//	return CallWMIMethod(getMaxRPM, (byte)fans[fanID].id);
		//return -1;
	}
	int Control::GetFanPercent(int fanID) {
		return fanID < fans.size() ? CallWMIMethod(getFanPercent, (byte)fans[fanID].id) : -1;
		//if (fanID < fans.size())
		//	return CallWMIMethod(getFanPercent, (byte) fans[fanID].id);
		//return -1;
	}
	int Control::GetFanBoost(int fanID) {
		return fanID < fans.size() ? CallWMIMethod(getFanBoost, (byte)fans[fanID].id) : -1;
		//if (fanID < fans.size()) {
		//	return CallWMIMethod(getFanBoost, (byte)fans[fanID].id);
		//}
		//return -1;
	}
	int Control::SetFanBoost(int fanID, byte value) {
		return fanID < fans.size() ? CallWMIMethod(setFanBoost, (byte)fans[fanID].id, value) : -1;
		//if (fanID < fans.size()) {
		//	return CallWMIMethod(setFanBoost, (byte)fans[fanID].id, value);
		//}
		//return -1;
	}
	int Control::GetTempValue(int TempID) {
		IWbemClassObject* sensorObject = NULL;
		IWbemServices* serviceObject = m_WbemServices;
		VARIANT temp;
		if (TempID < sensors.size()) {
			switch (sensors[TempID].type) {
			case 1: { // AWCC
				int awt = CallWMIMethod(getTemp, sensors[TempID].index);
				// Bugfix for AWCC temp - it can be up to 5000C!
				return awt > 200 ? -1 : awt;
			} break;
			case 2: // SSD
				serviceObject = m_DiskService;
				break;
			//case 3: // DPTF
			//	if (DPTFdone)
			//		return ((DPTFHelper*)dptf)->GetTemp(sensors[TempID].index);
			//	return -1;
			case 4: // OHM
				serviceObject = m_OHMService;
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
		if (isGmode) {
			if (state)
				SetPower(0xAB);
			return CallWMIMethod(setGMode, state);
		}
		return -1;
	}

	int Control::GetGMode() {
		return isGmode ? GetPower() < 0 ? 1 : CallWMIMethod(getGMode) : 0;
	}

	Lights::Lights(Control *ac) {
		if (ac && ac->isAlienware &&
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

	//string GetTag(string xml, string tag, size_t& pos) {
	//	size_t firstpos = xml.find("<" + tag + ">", pos);
	//	if (firstpos != string::npos) {
	//		firstpos += tag.length() + 2;
	//		pos = xml.find("</" + tag + ">", firstpos);
	//		return xml.substr(firstpos, pos - firstpos);
	//	}
	//	else {
	//		pos = string::npos;
	//		return "";
	//	}
	//}

	//string ReadFromESIF(string command, HANDLE g_hChildStd_IN_Wr, HANDLE g_hChildStd_OUT_Rd, PROCESS_INFORMATION* proc) {
	//	DWORD written;
	//	string outpart;
	//	WriteFile(g_hChildStd_IN_Wr, command.c_str(), (DWORD)command.length(), &written, NULL);
	//	while (outpart.find("Returned:") == string::npos) {
	//		while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && written) {
	//			char* buffer = new char[written + 1]{ 0 };
	//			ReadFile(g_hChildStd_OUT_Rd, buffer, written, &written, NULL);
	//			outpart += buffer;
	//			delete[] buffer;
	//		}
	//	}
	//	if (outpart.find("</result>") != string::npos) {
	//		size_t pos = 0;
	//		return GetTag(outpart, "result", pos);
	//	}
	//	else
	//		return "";
	//}

	//DWORD WINAPI DPTFInitFunc(LPVOID lpParam) {
	//	Control* src = (Control*)lpParam;
	//	string wdName;
	//	SECURITY_ATTRIBUTES attr{ sizeof(SECURITY_ATTRIBUTES), NULL, true };
	//	STARTUPINFO sinfo{ sizeof(STARTUPINFO), 0 };
	//	HANDLE g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, initHandle = NULL;
	//	PROCESS_INFORMATION proc;
	//	wdName.resize(2048);
	//	wdName.resize(GetWindowsDirectory((LPSTR)wdName.data(), 2047));
	//	wdName += "\\system32\\DriverStore\\FileRepository\\";
	//	WIN32_FIND_DATA file;
	//	HANDLE search_handle = FindFirstFile((wdName + "dptf_cpu*").c_str(), &file);
	//	if (search_handle != INVALID_HANDLE_VALUE)
	//	{
	//		wdName += string(file.cFileName) + "\\esif_uf.exe";
	//		FindClose(search_handle);
	//		CreatePipe(&g_hChildStd_OUT_Rd, &sinfo.hStdOutput, &attr, 0);
	//		CreatePipe(&sinfo.hStdInput, &g_hChildStd_IN_Wr, &attr, 0);
	//		DWORD flags = PIPE_NOWAIT;
	//		SetNamedPipeHandleState(g_hChildStd_IN_Wr, &flags, NULL, NULL);
	//		sinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	//		sinfo.wShowWindow = SW_HIDE;
	//		sinfo.hStdError = sinfo.hStdOutput;
	//		HWND cur = GetForegroundWindow();
	//		if (CreateProcess(NULL, (LPSTR)(wdName + " client").c_str(),
	//			NULL, NULL, true,
	//			CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &proc)) {
	//			SetForegroundWindow(cur);
	//			// Start init...
	//			size_t pos = 0;
	//			string parts = ReadFromESIF("format xml\nparticipants\nexit\n", g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, &proc);
	//			string part;
	//			int idc = 0;
	//			if (parts.size() && WaitForSingleObject(updateAllowed, 0) != WAIT_TIMEOUT) {
	//				while (pos != string::npos) {
	//					part = GetTag(parts, "participant", pos);
	//					size_t descpos = 0;
	//					byte sID = atoi(GetTag(part, "UpId", descpos).c_str());
	//					string name = GetTag(part, "desc", descpos);
	//					int dcount = atoi(GetTag(part, "domainCount", descpos).c_str());
	//					// check domains...
	//					for (int i = 0; i < dcount; i++) {
	//						size_t dPos = 0;
	//						string domain = GetTag(part, "domain", descpos);
	//						string dName = GetTag(domain, "name", dPos);
	//						char* p;
	//						if (strtol(GetTag(domain, "capability", dPos).c_str(), &p, 16) & 0x80) {
	//							for (auto sen = src->sensors.begin(); sen != src->sensors.end(); sen++)
	//								if (!sen->type && sen->index == idc) {
	//									sen->name = name + (dcount > 1 ? " (" + dName + ")" : "");
	//									break;
	//								}
	//							idc++;
	//						}
	//					}
	//				}
	//			}
	//			CloseHandle(proc.hProcess);
	//			CloseHandle(proc.hThread);
	//		}
	//		CloseHandle(sinfo.hStdInput);
	//		CloseHandle(g_hChildStd_IN_Wr);
	//		CloseHandle(g_hChildStd_OUT_Rd);
	//		CloseHandle(sinfo.hStdOutput);
	//	}
	//	src->DPTFdone = true;
	//	return 0;
	//}
}
