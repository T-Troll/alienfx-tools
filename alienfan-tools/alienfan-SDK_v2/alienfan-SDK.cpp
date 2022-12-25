#define WIN32_LEAN_AND_MEAN

#include "alienfan-SDK.h"
#include "alienfan-controls.h"

#pragma comment(lib, "wbemuuid.lib")

//#define _TRACE_

namespace AlienFan_SDK {

	//int Control::Percent(int val, int from) {
	//	return val * 100 / from;
	//}

	DWORD WINAPI DPTFInitFunc(LPVOID lpParam);

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
		//case 3:
		//	name = "AMD Sensor ";
		//	valuePath = (BSTR)L"Temp";
		//	break;
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

		//next:
		//	enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
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
#ifdef _TRACE_
						printf("Fan ID=%x found\n", funcID);
#endif
						//boosts.push_back(100);
						//maxrpm.push_back(0);
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
			CreateThread(NULL, 0, DPTFInitFunc, this, 0, NULL);
		}
		return devFlags;
	}

	int Control::GetFanRPM(int fanID) {
		if (fanID < fans.size())
			return CallWMIMethod(getFanRPM, (byte) fans[fanID].id);
		return -1;
	}
	int Control::GetMaxRPM(int fanID)
	{
		if (fanID < fans.size())
			return CallWMIMethod(getMaxRPM, (byte)fans[fanID].id);
		return -1;
	}
	int Control::GetFanPercent(int fanID) {
		if (fanID < fans.size())
			//if (fanID < maxrpm.size() && maxrpm[fanID])
			//	return Percent(GetFanRPM(fanID),maxrpm[fanID]);
			//else
				return CallWMIMethod(getFanPercent, (byte) fans[fanID].id);
		return -1;
	}
	int Control::GetFanBoost(int fanID/*, bool force*/) {
		if (fanID < fans.size()) {
			//int value = CallWMIMethod(getFanBoost, (byte) fans[fanID].id);
			//return force ? value : Percent(value, boosts[fanID]);
			return CallWMIMethod(getFanBoost, (byte)fans[fanID].id);
		}
		return -1;
	}
	int Control::SetFanBoost(int fanID, byte value/*, bool force*/) {
		if (fanID < fans.size()) {
			//int finalValue = force ? value : (int) value * boosts[fanID] / 100;
			//return CallWMIMethod(setFanBoost, (byte) fans[fanID].id, finalValue);
			return CallWMIMethod(setFanBoost, (byte)fans[fanID].id, value);
		}
		return -1;
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

	string GetTag(string xml, string tag, size_t& pos) {
		size_t firstpos = xml.find("<" + tag + ">", pos);
		if (firstpos != string::npos) {
			firstpos += tag.length() + 2;
			pos = xml.find("</" + tag + ">", firstpos);
			return xml.substr(firstpos, pos - firstpos);
		}
		else {
			pos = string::npos;
			return "";
		}
	}

	string ReadFromESIF(string command, HANDLE g_hChildStd_IN_Wr, HANDLE g_hChildStd_OUT_Rd, PROCESS_INFORMATION* proc) {
		DWORD written;
		string outpart;
		byte e_command[] = "echo noop\n";
		if (WaitForSingleObject(proc->hProcess, 0) != WAIT_TIMEOUT)
			return "";
		WriteFile(g_hChildStd_IN_Wr, command.c_str(), (DWORD)command.length(), &written, NULL);
		//FlushProcessWriteBuffers();
		while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && written) {
			char* buffer = new char[written + 1]{ 0 };
			ReadFile(g_hChildStd_OUT_Rd, buffer, written, &written, NULL);
			outpart += buffer;
			delete[] buffer;
		}
		while (outpart.find("</result>") == string::npos && outpart.find("ESIF_E") == string::npos
			/*&& outpart.find("Error") == string::npos*/) {
			while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && !written) {
				for (int i = 0; (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && !written) && i < 40; i++) {
					if (WaitForSingleObject(proc->hProcess, 0) != WAIT_TIMEOUT)
						return "";
					WriteFile(g_hChildStd_IN_Wr, e_command, sizeof(e_command) - 1, &written, NULL);
					//FlushProcessWriteBuffers();
				}
				Sleep(5);
			}
			while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && written) {
				char* buffer = new char[written + 1]{ 0 };
				ReadFile(g_hChildStd_OUT_Rd, buffer, written, &written, NULL);
				outpart += buffer;
				delete[] buffer;
			}
		}
		if (outpart.find("ESIF_E") != string::npos /*|| outpart.find("Error") != string::npos*/)
			return "";
		else {
			size_t pos = 0;
			return GetTag(outpart, "result", pos);
		}
	}

	//int DPTFHelper::GetTemp(int id) {
	//	size_t pos = 0;
	//	string val = ReadFromESIF("getp_part " + to_string(id & 0xf) + " 14 D" + to_string(id >> 4) + "\n", true);
	//	if (val.empty())
	//		return -1;
	//	else
	//		return atoi(GetTag(val, "value", pos).c_str());
	//}

	DWORD WINAPI DPTFInitFunc(LPVOID lpParam) {
		Control* src = (Control*)lpParam;
		string wdName;
		SECURITY_ATTRIBUTES attr{ sizeof(SECURITY_ATTRIBUTES), NULL, true };
		STARTUPINFO sinfo{ sizeof(STARTUPINFO), 0 };
		HANDLE g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, initHandle = NULL;
		PROCESS_INFORMATION proc;
		wdName.resize(2048);
		wdName.resize(GetWindowsDirectory((LPSTR)wdName.data(), 2047));
		wdName += "\\system32\\DriverStore\\FileRepository\\";
		WIN32_FIND_DATA file;
		HANDLE search_handle = FindFirstFile((wdName + "dptf_cpu*").c_str(), &file);
		if (search_handle != INVALID_HANDLE_VALUE)
		{
			wdName += string(file.cFileName) + "\\esif_uf.exe";
			FindClose(search_handle);
			CreatePipe(&g_hChildStd_OUT_Rd, &sinfo.hStdOutput, &attr, 0);
			CreatePipe(&sinfo.hStdInput, &g_hChildStd_IN_Wr, &attr, 0);
			DWORD flags = PIPE_NOWAIT;
			SetNamedPipeHandleState(g_hChildStd_IN_Wr, &flags, NULL, NULL);
			sinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			sinfo.wShowWindow = SW_HIDE;
			sinfo.hStdError = sinfo.hStdOutput;
			HWND cur = GetForegroundWindow();
			if (CreateProcess(NULL/*(LPSTR)wdName.c_str()*/, (LPSTR)(wdName + " client").c_str(),
				NULL, NULL, true,
				CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &proc)) {
				SetForegroundWindow(cur);
				// Start init...
				size_t pos = 0;
				string parts = ReadFromESIF("format xml\nparticipants\nexit\n", g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, &proc);
				string part;
				int idc = 0;
				if (parts.size()) {
					while (pos != string::npos) {
						part = GetTag(parts, "participant", pos);
						size_t descpos = 0;
						byte sID = atoi(GetTag(part, "UpId", descpos).c_str());
						string name = GetTag(part, "desc", descpos);
						int dcount = atoi(GetTag(part, "domainCount", descpos).c_str());
						// check domains...
						for (int i = 0; i < dcount; i++) {
							size_t dPos = 0;
							string domain = GetTag(part, "domain", descpos);
							string dName = GetTag(domain, "name", dPos);
							char* p;
							if (strtol(GetTag(domain, "capability", dPos).c_str(), &p, 16) & 0x80) {
								for (auto sen = src->sensors.begin(); sen != src->sensors.end(); sen++)
									if (!sen->type && sen->index == idc) {
										sen->name = name + (dcount > 1 ? " (" + dName + ")" : "");
										break;
									}
								idc++;
							}
						}
					}
				}
				CloseHandle(proc.hProcess);
				CloseHandle(proc.hThread);
			}
			CloseHandle(sinfo.hStdInput);
			CloseHandle(g_hChildStd_IN_Wr);
			CloseHandle(g_hChildStd_OUT_Rd);
			CloseHandle(sinfo.hStdOutput);
		}
		src->DPTFdone = true;
		return 0;
	}


}
