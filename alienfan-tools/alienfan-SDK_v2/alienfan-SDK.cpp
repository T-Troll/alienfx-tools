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
		IEnumWbemClassObject* enum_obj;
		if (m_DiskService->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDisk", 0, NULL, &enum_obj) == S_OK) {
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
			parameters.uintVal = ALIENFAN_INTERFACE{ dev_controls[com], arg1, arg2, 0 }.args;
			m_InParamaters->Put(L"arg2", NULL, &parameters, 0);
			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				commandList[functionID[sysType][com]], 0, NULL, m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
				m_outParameters->Release();
			}
		}
		return result.intVal;
	}

	void Control::EnumSensors(IWbemServices* srv, const wchar_t* s_name, byte type) {
		IEnumWbemClassObject* enum_obj;
		if (srv->CreateInstanceEnum((BSTR)s_name, /*WBEM_FLAG_SHALLOW |*/ WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj) == S_OK) {
			IWbemClassObject* spInstance[32];
			ULONG uNumOfInstances;
			LPCWSTR instansePath = L"__Path", valuePath = L"Temperature";
			VARIANT cTemp{ VT_I4 }, instPath{ VT_BSTR };
			string name;

			switch (type) {
			case 0:
				name = "ESIF";
				break;
			case 2:
				name = "SSD";
				instansePath = L"StorageReliabilityCounter";
				break;
			case 4:
				valuePath = L"Value";
			}
			name += " sensor ";

			byte senID = 0;
			while (enum_obj->Next(3000, 32, spInstance, &uNumOfInstances) != WBEM_S_FALSE || uNumOfInstances) {
				for (byte ind = 0; ind < uNumOfInstances; ind++) {
					if (type == 4) { // OHM sensors
						VARIANT type{ 0 };
						spInstance[ind]->Get(L"SensorType", 0, &type, 0, 0);
						if (!wcscmp(type.bstrVal, L"Temperature")) {
							VARIANT vname{ 0 };
							spInstance[ind]->Get(L"Name", 0, &vname, 0, 0);
							name.clear();
							for (int i = 0; i < wcslen(vname.bstrVal); i++)
								name += vname.bstrVal[i];
						}
						else {
							continue;
						}
					}
					spInstance[ind]->Get(instansePath, 0, &instPath, 0, 0);
					spInstance[ind]->Get(valuePath, 0, &cTemp, 0, 0);
					spInstance[ind]->Release();
					if (type == 2 || cTemp.intVal > 0 || cTemp.fltVal > 0)
						sensors.push_back({ { senID++,type }, name + (type != 4 ? to_string(senID) : ""), instPath.bstrVal, (BSTR)valuePath });
				}
			}
			enum_obj->Release();
		}
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

				// check system type and fill inParams
				for (int type = 0; type < 2; type++)
					if (isSupported = (m_AWCCGetObj->GetMethod(commandList[functionID[type][getPowerID]], NULL, &m_InParamaters, nullptr) == S_OK && m_InParamaters)) {
						sysType = type;
#ifdef _TRACE_
						printf("Fan Control available, system type %d\n", sysType);
#endif
						systemID = CallWMIMethod(getSysID, 2);
#ifdef _TRACE_
						printf("System ID = %d\n", systemID);
#endif
						isGmode = m_AWCCGetObj->GetMethod(commandList[2], NULL, nullptr, nullptr) == S_OK;
#ifdef _TRACE_
						if (isGmode)
							printf("G-Mode available\n");
#endif
						if (isTcc = ((maxTCC = CallWMIMethod(getMaxTCC)) > 0)) {
							maxOffset = CallWMIMethod(getMaxOffset);
						}
#ifdef _TRACE_
						if (isTcc)
							printf("TCC control available\n");
#endif
						isXMP = CallWMIMethod(getXMP) >= 0;
#ifdef _TRACE_
						if (isXMP)
							printf("Memory XMP available\n");
#endif
						int fIndex = 0; unsigned funcID = CallWMIMethod(getPowerID, fIndex);

						powers.push_back(0); // Manual mode
						// Scan for avaliable data
						while (funcID && (funcID + 1)) {
							byte vkind = funcID & 0xff;
							if (funcID > 0x100) {
								// sensor
#ifdef _TRACE_
								printf("Sensor ID=%x found\n", funcID);
#endif
								sensors.push_back({ { vkind, 1 }, sensors.size() < 2 ? temp_names[sensors.size()] : "Sensor #" + to_string(sensors.size()) });
							}
							else {
								if (funcID > 0x8f) {
									// power mode
									powers.push_back(vkind);
#ifdef _TRACE_
									printf("Power ID=%x found\n", funcID);
#endif
								}
								else {
									// fan
									fans.push_back({ vkind, (byte)CallWMIMethod(getFanSensor, vkind) });
#ifdef _TRACE_
									printf("Fan ID=%x found\n", funcID);
#endif
								}
							}
							fIndex++;
							funcID = CallWMIMethod(getPowerID, fIndex);
						}
#ifdef _TRACE_
						printf("%d fans, %d sensors, %d Power modes found, last reply %x\n", (int) fans.size(), (int) sensors.size(), (int)powers.size(), funcID);
#endif
						if (sysType) {
							// Modes 1 and 2 for R7 desktop
							powers.push_back(1);
							powers.push_back(2);
						}

						// ESIF sensors
						EnumSensors(m_WbemServices, L"EsifDeviceInformation", 0);
						// SSD sensors
						EnumSensors(m_DiskService, L"MSFT_PhysicalDiskToStorageReliabilityCounter", 2);
						// OHM sensors
						if (m_OHMService) {
							EnumSensors(m_OHMService, L"Sensor", 4);
						}
						return true;
					}
			}
		}
		return false;
	}

	int Control::GetFanRPM(int fanID) {
		return fanID < fans.size() ? CallWMIMethod(getFanRPM, (byte)fans[fanID].id) : -1;
	}
	int Control::GetMaxRPM(int fanID)
	{
		return fanID < fans.size() ? CallWMIMethod(getMaxRPM, (byte)fans[fanID].id) : -1;
	}
	int Control::GetFanPercent(int fanID) {
		return fanID < fans.size() ? CallWMIMethod(getFanPercent, (byte)fans[fanID].id) : -1;
	}
	int Control::GetFanBoost(int fanID) {
		return fanID < fans.size() ? CallWMIMethod(getFanBoost, (byte)fans[fanID].id) : -1;
	}
	int Control::SetFanBoost(int fanID, byte value) {
		return fanID < fans.size() ? CallWMIMethod(setFanBoost, (byte)fans[fanID].id, value) : -1;
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
			case 4: // OHM
				serviceObject = m_OHMService;
				break;
			}
			if (serviceObject->GetObject(sensors[TempID].instance, NULL, nullptr, &sensorObject, nullptr) == S_OK) {
				sensorObject->Get(sensors[TempID].valueName, 0, &temp, 0, 0);
				sensorObject->Release();
				return sensors[TempID].type == 4 ? (int)temp.fltVal : temp.intVal;
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
	int Control::GetPower(bool raw) {
		int pl = CallWMIMethod(getPowerMode);
		if (raw || pl < 0) return pl;
		for (int i = 0; i < powers.size(); i++)
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
		return isGmode ? GetPower(true) < 0 || CallWMIMethod(getGMode) : 0;
	}

	int Control::GetTCC()
	{
		if (isTcc) {
			int curOffset = CallWMIMethod(getCurrentOffset);
			return maxTCC - curOffset;
		}
		return -1;
	}

	int Control::SetTCC(byte tccValue)
	{
		if (isTcc) {
			if (maxTCC - tccValue <= maxOffset)
				return CallWMIMethod(setOffset, maxTCC - tccValue);
		}
		return -1;
	}

	int Control::GetXMP()
	{	if (isXMP)
			return CallWMIMethod(getXMP);
		return -1;
	}

	int Control::SetXMP(byte memXMP)
	{
		if (isXMP) {
			int res = CallWMIMethod(setXMP, memXMP);
			return res ? -1 : res;
		}
		return -1;
	}

	Lights::Lights(Control *ac) {
		if (ac && ac->isAlienware &&
			ac->m_AWCCGetObj->GetMethod(colorList[0], NULL, &m_InParamaters, nullptr) == S_OK && m_InParamaters) {
			m_WbemServices = ac->m_WbemServices;
			m_instancePath = ac->m_instancePath;
#ifdef _TRACE_
			VARIANT res{ VT_UNKNOWN };
			CIMTYPE restype;
			m_InParamaters->Get(L"arg2", 0, &res, &restype, nullptr);
			printf("Light parameter type %x(%x)\n", restype, res.vt);
#endif
			isActivated = true;
		}
	}

	int Lights::CallWMIMethod(byte com, byte* arg1) {
		VARIANT result{ VT_I4 };
		result.intVal = -1;
		if (m_InParamaters) {
			IWbemClassObject* m_outParameters = NULL;
			VARIANT parameters{ VT_ARRAY | VT_UI1 };
			SAFEARRAY* args = SafeArrayCreateVector(VT_UI1, 0, 8);
#ifdef _TRACE_
			if (!args)
				printf("Light array creation failed!\n");
#endif
			//byte HUGEP* pdFreq;
			//SafeArrayAccessData(args, (void HUGEP * FAR*) &pdFreq);
			for (long i = 0; i < 8; i++) {
				HRESULT res = SafeArrayPutElement(args, &i, &arg1[i]);
#ifdef _TRACE_
				if (res != S_OK)
					printf("Light array element error %x\n", res);
#endif
				//pdFreq[i] = arg1[i];
			}
			//SafeArrayUnaccessData(args);
			parameters.pparray = &args;
			m_InParamaters->Put(L"arg2", NULL, &parameters, 0);
			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				colorList[com], 0, NULL, m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
				m_outParameters->Release();
			}
			SafeArrayDestroy(args);
		}
		return result.intVal;
	}

	bool Lights::SetBrightness(byte brightness) {
		byte param[8]{ 0 };
		param[4] = brightness;
		return CallWMIMethod(1, param) >= 0;
	}

	bool Lights::SetColor(byte id, byte r, byte g, byte b, bool save) {
		//byte param[8]{ r, g, b, id };
		byte param[8]{ id };
		param[4] = b;
		param[5] = g;
		param[6] = r;
		param[7] = save ? 0 : 0xff;
		return CallWMIMethod(0, param) >= 0;
	}

}
