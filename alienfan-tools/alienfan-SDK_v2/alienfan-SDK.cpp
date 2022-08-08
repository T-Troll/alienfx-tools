// alienfan-SDK.cpp : Defines the functions for the static library.
//

#include "alienfan-SDK.h"
#include "alienfan-controls.h"

#pragma comment(lib, "wbemuuid.lib")

//#define _TRACE_

//typedef BOOLEAN (WINAPI *ACPIF)(LPWSTR, LPWSTR);
//
//static char tempNamePattern[] = "\\_SB.PCI0.LPCB.EC0.SEN1._STR",
//			tempECDV1[] = "\\_SB.PCI0.LPCB.ECDV.KDRT";// ,
//			//tempECDV2[] = "\\_SB.PC00.LPCB.ECDV.KDRT";

namespace AlienFan_SDK {

	Control::Control() {

#ifdef _TRACE_
		printf("WMI activation started.\n");
#endif
		CoInitializeEx(nullptr, COINIT::COINIT_MULTITHREADED);// COINIT_APARTMENTTHREADED);
		CoInitializeSecurity(nullptr,
			-1,
			nullptr,
			nullptr,
			RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr,
			EOAC_NONE,
			nullptr);

		CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&m_WbemLocator);
		if (m_WbemLocator->ConnectServer((BSTR)L"ROOT\\WMI", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_WbemServices) == S_OK) {
			activated = true;
		}
		m_WbemLocator->Release();
	}
	Control::~Control() {
		sensors.clear();
		fans.clear();
		powers.clear();
		boosts.clear();
		maxrpm.clear();
		m_WbemServices->Release();
	}

	//int Control::ReadRamDirect(DWORD offset) {
	//	if (activated && aDev != -1) {
	//		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
	//		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0xFF000000 | offset);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		if (EvalAcpiMethod(acc, dev_c_controls.readCom.c_str(), (PVOID *) &res, acpiargs) && res) {
	//			int res_int = res->Argument[0].Argument;
	//			free(res);
	//			return res_int;
	//		}
	//	}
	//	return -1;
	//}

	//int Control::WriteRamDirect(DWORD offset, byte value) {
	//	if (activated && aDev != -1) {
	//		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
	//		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0xFF000000 | offset);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
	//		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, value);
	//		if (EvalAcpiMethod(acc, dev_c_controls.writeCom.c_str(), (PVOID *) &res, acpiargs) && res) {
	//			int res_int = res->Argument[0].Argument;
	//			free(res);
	//			return res_int;
	//		}
	//	}
	//	return -1;
	//}

	int Control::CallWMIMethod(ALIENFAN_COMMAND com, byte arg1, byte arg2) {
		BYTE operand[4]{ com.sub, arg1, arg2, 0 };
		IWbemClassObject* m_InParamaters = NULL, * m_outParameters = NULL;
		if (m_AWCCGetObj->GetMethod(commandList[com.com], NULL, &m_InParamaters, nullptr) == S_OK) {
			VARIANT parameters = { 0 }, result{ 0 };
			parameters.vt = VT_I4;
			parameters.uintVal = *((DWORD*)operand);
			m_InParamaters->Put((BSTR)L"arg2", NULL, &parameters, 0);
			if (m_WbemServices->ExecMethod(m_instancePath.bstrVal,
				commandList[com.com], 0, NULL,
				m_InParamaters,
				&m_outParameters,
				NULL) == S_OK && m_outParameters) {
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

	/*int Control::RunMainCommand(ALIENFAN_COMMAND com, byte value1, byte value2) {
		if (activated && com.com && aDev != -1) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			BYTE operand[4]{com.sub, value1, value2, 0};
			com.com -= devs[aDev].delta * 4;
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, com.com);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutBuffArg(acpiargs, 4, operand);
			if (EvalAcpiMethod(acc, devs[aDev].mainCommand.c_str(), (PVOID *) &res, acpiargs) && res) {
				int res_int = res->Argument[0].Argument;
				free(res);
				return res_int;
			}
		}
		return -1;
	}

	int Control::RunGPUCommand(short com, DWORD packed) {
		if (activated && com && aDev != -1) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;

			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0x100);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, com);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutBuffArg(acpiargs, 4, (UCHAR*)&packed);
			if (EvalAcpiMethod(acc, devs[aDev].gpuCommand.c_str(), (PVOID *) &res, acpiargs) && res) {
				int res_int = res->Argument[0].Argument;
				free(res);
				return res_int;
			}
		}
		return -1;
	}*/

	bool Control::Probe() {
		if (activated && m_WbemServices->GetObject((BSTR)L"AWCCWmiMethodFunction", NULL, nullptr, &m_AWCCGetObj, nullptr) == S_OK) {
#ifdef _TRACE_
			printf("AWCC section detected!\n");
#endif
			// need to get instance
			IEnumWbemClassObject* enum_obj;
			m_WbemServices->CreateInstanceEnum((BSTR)L"AWCCWmiMethodFunction", WBEM_FLAG_FORWARD_ONLY/*WBEM_FLAG_RETURN_IMMEDIATELY*/, NULL, &enum_obj);
			IWbemClassObject* spInstance;
			ULONG uNumOfInstances = 0;
			enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
			spInstance->Get((BSTR)L"__Path", 0, &m_instancePath, 0, 0);
			spInstance->Release();
			enum_obj->Release();
			IWbemClassObject* m_InParamaters = NULL;
			// now let's check methods
			if (m_AWCCGetObj->GetMethod(commandList[0], NULL, &m_InParamaters, nullptr) == S_OK) {
				m_InParamaters->Release();
				// Let's get device ID...
				systemID = CallWMIMethod({ 0, 2 }, 0);
#ifdef _TRACE_
				printf("System information available, ID=%x!\n", systemID);
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
						if (sIndex < temp_names.size()) {
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
					if (HIWORD(systemID) == 500)
						powers.push_back(0xAB);
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
				aDev = 0;
			}
			if (m_AWCCGetObj->GetMethod(commandList[2], NULL, &m_InParamaters, nullptr) == S_OK) {
#ifdef _TRACE_
				printf("G-Mode available!\n");
#endif
				m_InParamaters->Release();
				haveGmode = true;
			}
			// ESIF temperature sensors
			IWbemClassObject* m_ESIFObject = NULL;
			if (m_WbemServices->GetObject((BSTR)L"EsifDeviceInformation", NULL, nullptr, &m_ESIFObject, nullptr) == S_OK) {
				m_WbemServices->CreateInstanceEnum((BSTR)L"EsifDeviceInformation", WBEM_FLAG_FORWARD_ONLY/*WBEM_FLAG_RETURN_IMMEDIATELY*/, NULL, &enum_obj);
				//IWbemClassObject* spInstance;
				//ULONG uNumOfInstances = 0;
				enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				int numESIF = 0;
				while (uNumOfInstances) {
					VARIANT instPath, cTemp;
					spInstance->Get((BSTR)L"__Path", 0, &instPath, 0, 0);
					spInstance->Get((BSTR)L"Temperature", 0, &cTemp, 0, 0);
					spInstance->Release();
					if (cTemp.uintVal > 0) {
						sensors.push_back({ (short)numESIF, "ESIF sensor #" + to_string(numESIF + 1), 0, instPath.bstrVal });
						numESIF++;
					}
					enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
				}
				enum_obj->Release();
				m_ESIFObject->Release();
#ifdef _TRACE_
				printf("ESIF data available, %d sensors added!\n", numESIF);
#endif
			}
			return true;
		}
		return false;
	}

	int Control::GetFanRPM(int fanID) {
		if (fanID < fans.size())
			//if (devs[aDev].commandControlled)
				return CallWMIMethod(dev_controls.getFanRPM, (byte) fans[fanID]);
			//else
			//	return ReadRamDirect(fans[fanID]) * 70; // max. 7000 RPM
		return -1;
	}
	int Control::GetFanPercent(int fanID) {
		if (fanID < fans.size())
			if (fanID < maxrpm.size() && maxrpm[fanID])
				return GetFanRPM(fanID) * 100 / maxrpm[fanID];
			else
				//if (devs[aDev].commandControlled)
					return CallWMIMethod(dev_controls.getFanPercent, (byte) fans[fanID]);
				//else
				//	return ReadRamDirect(fans[fanID]);
		return -1;
	}
	int Control::GetFanBoost(int fanID, bool force) {
		if (fanID < fans.size()) {
			//if (devs[aDev].commandControlled) {
				int value = CallWMIMethod(dev_controls.getFanBoost, (byte) fans[fanID]);
				return force ? value : value * 100 / boosts[fanID];
			//} else
			//	return ReadRamDirect(fans[fanID]);
		}
		return -1;
	}
	int Control::SetFanBoost(int fanID, byte value, bool force) {
		if (fanID < fans.size()) {
			//if (devs[aDev].commandControlled) {
				int finalValue = force ? value : (int) value * boosts[fanID] / 100;
				return CallWMIMethod(dev_controls.setFanBoost, (byte) fans[fanID], finalValue);
			//} else {
			//	WriteRamDirect(fans[fanID] + 0x23, value + 1); // lock at 0 fix
			//	return WriteRamDirect(fans[fanID], value);
			//}
		}
		return -1;
	}
	int Control::GetTempValue(int TempID) {
		if (TempID < sensors.size()) {
			switch (sensors[TempID].type) {
			case 1: // AWCC
				return CallWMIMethod(dev_controls.getTemp, (byte)sensors[TempID].senIndex);
				break;
			case 0: {// ESIF
				IWbemClassObject* esifObject = NULL;
				if (m_WbemServices->GetObject(sensors[TempID].instance, NULL, nullptr, &esifObject, nullptr) == S_OK) {
					VARIANT temp;
					esifObject->Get((BSTR)L"Temperature", 0, &temp, 0, 0);
					esifObject->Release();
					return temp.uintVal;
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
	int Control::SetPower(int level) {
		if (level < powers.size())
			//if (devs[aDev].commandControlled)
				return CallWMIMethod(dev_controls.setPower, powers[level]);
			//else {
			//	return WriteRamDirect(dev_c_controls.unlock, powers[level]);
			//}
		return -1;
	}
	int Control::GetPower() {
		//if (devs[aDev].commandControlled) {
			int pl = CallWMIMethod(dev_controls.getPower);
			for (int i = 0; pl >= 0 && i < powers.size(); i++)
				if (powers[i] == pl)
					return i;
		//} else {
		//	// Always return Auto mode for system safety!
		//	return 1;
		//	//int pl = ReadRamDirect(dev_c_controls.unlock);
		//	//for (int i = 0; pl >= 0 && i < powers.size(); i++)
		//	//	if (powers[i] == pl)
		//	//		return i;
		//}
		return -1;
	}
	int Control::SetGPU(int power) {
		/*if (power >= 0 && power < 5 && devs[aDev].commandControlled) {
			return RunGPUCommand(dev_controls.setGPUPower.com, power << 4 | dev_controls.setGPUPower.sub);
		}*/
		return -1;
	}

	int Control::SetGMode(bool state)
	{
		if (haveGmode)
			return CallWMIMethod(dev_controls.setGMode, state);
		return -1;
	}

	int Control::GetGMode() {
		if (GetPower() < 0)
			return 1;
		else
			if (haveGmode)
				return CallWMIMethod(dev_controls.getGMode);
		return -1;
	}

	bool Control::IsActivated() {
		return activated;
	}
	int Control::HowManyFans() {
		return (int)fans.size();
	}
	int Control::HowManyPower() {
		return (int)powers.size();
	}
	int Control::HowManySensors() {
		return (int)sensors.size();
	}
	int Control::GetVersion() {
		return aDev + 1;
	}
	Lights::Lights(Control *ac) {
		acpi = ac;
		// Probe lights...
		if (Prepare())
			activated = true;
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
