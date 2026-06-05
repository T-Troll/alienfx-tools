#define WIN32_LEAN_AND_MEAN

#include "alienfan-SDK.h"
#include "alienfan-controls.h"

#pragma comment(lib, "wbemuuid.lib")

#define _TRACE_

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
		CoCreateInstance(CLSID_WbemRefresher, NULL,	CLSCTX_INPROC_SERVER, IID_IWbemRefresher, (void**)&m_Refresher);
		m_Refresher->QueryInterface(IID_IWbemConfigureRefresher, (void**)&m_pConfig);
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\WMI", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &m_WbemServices);
		// Windows bug with disk drives list
		//m_WbemLocator->ConnectServer((BSTR)L"ROOT\\Microsoft\\Windows\\Storage", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_DiskService);
		//IEnumWbemClassObject* enum_obj;
		//if (m_DiskService->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDisk", 0, NULL, &enum_obj) == S_OK) {
		//	enum_obj->Release();
		//}
		//m_DiskService->Release();
		// End Windows bugfix
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\Microsoft\\Windows\\Storage\\Providers_v2", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &m_DiskService);
		m_WbemLocator->ConnectServer((BSTR)L"ROOT\\LibreHardwareMonitor", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &m_OHMService);
		m_WbemLocator->Release();
	}

	Control::~Control() {
		if (m_Refresher) {
			m_Refresher->Release();
			m_pConfig->Release();
		}
		if (m_AWCCGetObj)
			m_AWCCGetObj->Release();
		if (m_DiskService)
			m_DiskService->Release();
		if (m_OHMService)
			m_OHMService->Release();
		if (m_InParamaters)
			m_InParamaters->Release();
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
			if (SUCCEEDED(m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				commandList[functionID[sysType][com]], 0, NULL, m_InParamaters, &m_outParameters, NULL)) && m_outParameters) {
				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
				m_outParameters->Release();
			}
		}
		return result.intVal;
	}

	void Control::EnumSensors(IWbemServices* srv, const wchar_t* s_name, const LPCWSTR valuePath, string name, byte type) {
		long plID;
		IWbemHiPerfEnum* insts = NULL;
		m_pConfig->AddEnum(srv, (BSTR)s_name, 0, NULL, &insts, &plID);
		if (insts) {
			vector<IWbemObjectAccess*> spInstance;
			ULONG uNumOfInstances;
			m_Refresher->Refresh(0);
			insts->GetObjects(0, 0, spInstance.data(), &uNumOfInstances);
			if (uNumOfInstances) {
				spInstance.resize(uNumOfInstances);
				if (SUCCEEDED(insts->GetObjects(0, uNumOfInstances, spInstance.data(), &uNumOfInstances)) && uNumOfInstances) {
					VARIANT cTemp{ VT_I4 }, instPath{ VT_BSTR }, vtype{ VT_BSTR }, vname{ 0 };
					byte senID = 0;
					for (auto& i : spInstance) {
						string lname;
						if (type == 4) { // OHM sensors
							i->Get(L"SensorType", 0, &vtype, 0, 0);
							if (!wcscmp(vtype.bstrVal, L"Temperature")) {
								i->Get(L"Name", 0, &vname, 0, 0);
								for (int i = 0; i < wcslen(vname.bstrVal); i++)
									lname += vname.bstrVal[i];
							}
							else {
								i->Release();
								continue;
							}
						}
						else {
							lname = name + " sensor " + to_string(senID);
						}
						i->Get(valuePath, 0, &cTemp, 0, 0);
						if (type == 2 || cTemp.intVal > 0 || cTemp.fltVal > 0)
							sensors.push_back({ { senID++,type }, lname, i, (BSTR)valuePath });
						else
							i->Release();
					}

				}
			}
#ifdef _TRACE_
			printf("%d sensors of #%d added, %d total\n", uNumOfInstances, type, (int)sensors.size());
#endif
		}
	}

	bool Control::Probe(bool diskSensors) {
		IEnumWbemClassObject* enum_obj;
#ifdef _TRACE_
		if (m_WbemServices)
			printf("WMI connect succesful!\n");
#endif
		if (m_WbemServices && (isAlienware = SUCCEEDED(m_WbemServices->GetObject((BSTR)L"AWCCWmiMethodFunction", NULL, nullptr, &m_AWCCGetObj, nullptr)))) {
#ifdef _TRACE_
			printf("AWCC section detected!\n");
#endif
			// need to get instance
			if (SUCCEEDED(m_WbemServices->CreateInstanceEnum((BSTR)L"AWCCWmiMethodFunction", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj))) {

				IWbemClassObject* spInstance;
				ULONG uNumOfInstances;
				if (SUCCEEDED(enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances))) {
					spInstance->Get((BSTR)L"__Path", 0, &m_instancePath, 0, 0);
					spInstance->Release();
					enum_obj->Release();

					// check system type and fill inParams
					for (sysType = 0; sysType < 2; sysType++)
						if (isSupported = (SUCCEEDED(m_AWCCGetObj->GetMethod(commandList[functionID[sysType][getPowerID]], NULL, &m_InParamaters, nullptr)) && m_InParamaters)) {
#ifdef _TRACE_
							printf("Fan Control available, system type %d\n", sysType);
#endif
								systemID = CallWMIMethod(getSysID, 2);
#ifdef _TRACE_
								printf("System ID = %d\n", systemID);
#endif
							isGmode = SUCCEEDED(m_AWCCGetObj->GetMethod(commandList[2], NULL, nullptr, nullptr));
#ifdef _TRACE_
							if (isGmode)
								printf("G-Mode available\n");

#endif
							if (isTcc = SUCCEEDED(m_AWCCGetObj->GetMethod(commandList[6], NULL, nullptr, nullptr))) {
								maxTCC = CallWMIMethod(getMaxTCC);
								maxOffset = CallWMIMethod(getMaxOffset);
							}
#ifdef _TRACE_
							if (isTcc)
								printf("TCC control available\n");
#endif
							isXMP = SUCCEEDED(m_AWCCGetObj->GetMethod(commandList[7], NULL, nullptr, nullptr));
#ifdef _TRACE_
							if (isXMP)
								printf("Memory XMP available\n");
#endif
							int fIndex = 0, funcID;

							powers.push_back(0); // Manual mode
							// Scan for avaliable data
							while ((funcID = CallWMIMethod(getPowerID, fIndex)) > 0) {
								byte vkind = funcID & 0xff;
								if (funcID > 0x100 && funcID < 0x110) {
									// sensor
#ifdef _TRACE_
									printf("Sensor ID=%x found\n", funcID);
#endif
									sensors.push_back({ { vkind, 1 }, sensors.size() < 3 ? temp_names[sensors.size()] : "Sensor #" + to_string(sensors.size()) });
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
							}
#ifdef _TRACE_
							printf("%d fans, %d sensors, %d Power modes found, last reply %x\n", (int)fans.size(), (int)sensors.size(), (int)powers.size(), funcID);
#endif
							if (sysType) {
								// Modes 1 and 2 for R7 desktop
								powers.push_back(1);
								powers.push_back(2);
							}

							// ESIF sensors
							EnumSensors(m_WbemServices, L"EsifDeviceInformation", L"Temperature", "ESIF", 0);
							// SSD sensors
							if (diskSensors)
								EnumSensors(m_DiskService, L"MSFT_PhysicalDiskToStorageReliabilityCounter", L"Temperature", "SSD", 2);
							// OHM sensors
							EnumSensors(m_OHMService, L"Sensor", L"Value", "", 4);
							return true;
						}
				}
				enum_obj->Release();
			}
		}
#ifdef _TRACE_
		else
			printf("AWCC section NOT detected!\n");
#endif
		return false;
	}

	inline int Control::OperateFan(int function, int fanID) {
		return fanID < fans.size() ? CallWMIMethod(function, (byte)fans[fanID].id) : -1;
	}

	int Control::GetFanRPM(int fanID) {
		return OperateFan(getFanRPM, fanID);
	}
	int Control::GetMaxRPM(int fanID)
	{
		return OperateFan(getMaxRPM, fanID);
	}
	int Control::GetFanPercent(int fanID) {
		return OperateFan(getFanPercent, fanID);
	}
	int Control::GetFanBoost(int fanID) {
		return OperateFan(getFanBoost, fanID);
	}
	int Control::SetFanBoost(int fanID, byte value) {
		return fanID < fans.size() ? CallWMIMethod(setFanBoost, (byte)fans[fanID].id, value) : -1;
	}
	int Control::GetTempValue(int TempID) {
		if (TempID < sensors.size()) {
			if (sensors[TempID].type == 1) {
				// AWCC
				int awt = CallWMIMethod(getTemp, sensors[TempID].index);
				// Bugfix for AWCC temp - it can be up to 5000C!
				return awt > 150 ? -1 : awt;
			}
			else {
				// ESIF, SSD, OHM
				//m_Refresher->Refresh(0);
				VARIANT temp;
				sensors[TempID].instance->Get(sensors[TempID].valueName, 0, &temp, 0, 0);
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
				SetPower(0xab);
			return CallWMIMethod(setGMode, state);
		}
		return -1;
	}

	int Control::GetGMode() {
		if (isGmode) {
			int pm = GetPower(true);
			return pm == 0xab || pm < 0 /*|| CallWMIMethod(getGMode) > 0*/;
		}
		return 0;
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

//	Lights::Lights(Control *ac) {
//		if (ac && ac->isAlienware &&
//			ac->m_AWCCGetObj->GetMethod(colorList[0], NULL, &m_InParamaters, nullptr) == S_OK && m_InParamaters) {
//			m_WbemServices = ac->m_WbemServices;
//			m_instancePath = ac->m_instancePath;
//#ifdef _TRACE_
//			VARIANT res{ VT_UNKNOWN };
//			CIMTYPE restype;
//			m_InParamaters->Get(L"arg2", 0, &res, &restype, nullptr);
//			printf("Light parameter type %x(%x)\n", restype, res.vt);
//#endif
//			isActivated = true;
//		}
//	}
//
//	int Lights::CallWMIMethod(byte com, byte* arg1) {
//		VARIANT result{ VT_I4 };
//		result.intVal = -1;
//		if (m_InParamaters) {
//			IWbemClassObject* m_outParameters = NULL;
//			VARIANT parameters{ VT_ARRAY | VT_UI1 };
//			SAFEARRAY* args = SafeArrayCreateVector(VT_UI1, 0, 8);
//#ifdef _TRACE_
//			if (!args)
//				printf("Light array creation failed!\n");
//#endif
//			for (long i = 0; i < 8; i++) {
//				HRESULT res = SafeArrayPutElement(args, &i, &arg1[i]);
//#ifdef _TRACE_
//				if (res != S_OK)
//					printf("Light array element error %x\n", res);
//#endif
//			}
//			parameters.pparray = &args;
//			m_InParamaters->Put(L"arg2", NULL, &parameters, 0);
//			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
//				colorList[com], 0, NULL, m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
//				m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
//				m_outParameters->Release();
//			}
//			SafeArrayDestroy(args);
//		}
//		return result.intVal;
//	}
//
//	bool Lights::SetBrightness(byte brightness) {
//		byte param[8]{ 0 };
//		param[4] = brightness;
//		return CallWMIMethod(1, param) >= 0;
//	}
//
//	bool Lights::SetColor(byte id, byte r, byte g, byte b, bool save) {
//		byte param[8]{ id, 0, 0, 0, b, g, r, (byte)(save ? 0 : 0xff) };
//		return CallWMIMethod(0, param) >= 0;
//	}

}
