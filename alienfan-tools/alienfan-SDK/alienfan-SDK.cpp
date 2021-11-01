// alienfan-SDK.cpp : Defines the functions for the static library.
//

#include "alienfan-SDK.h"
#include "alienfan-controls.h"
#include "alienfan-low.h"
#include <iostream>

typedef BOOLEAN (WINAPI *ACPIF)(LPWSTR, LPWSTR);

namespace AlienFan_SDK {

	Control::Control() {

		// do we already have service runnning?
		activated = (acc = OpenAcpiDevice()) != INVALID_HANDLE_VALUE && acc;
		if (!activated) {
			// We don't, so let's try to start it!
			TCHAR  driverLocation[MAX_PATH] = {0};
		    wrongEnvironment = true;
			if (GetServiceName(driverLocation, MAX_PATH)) {
				if (scManager = OpenSCManager(
					NULL,                   // local machine
					NULL,                   // local database
					SC_MANAGER_ALL_ACCESS   // access required
				)) {
					InstallService(scManager, driverLocation);
					if (DemandService(scManager)) {
						activated = (acc = OpenAcpiDevice()) != INVALID_HANDLE_VALUE && acc;
						wrongEnvironment = !activated;
					} else {
						RemoveService(scManager);
						CloseServiceHandle(scManager);
						scManager = NULL;
					}
				}
			}
			if (wrongEnvironment) {
				// Let's try to load driver via kernel hack....
				wchar_t currentPath[MAX_PATH];
				GetModuleFileNameW(NULL, currentPath, MAX_PATH);
				wstring cpath = currentPath;
				cpath.resize(cpath.find_last_of(L"\\"));
				cpath += L"\\HwAcc.sys";

				HMODULE kdl = LoadLibrary(L"kdl.dll");
				if (kdl) {
					ACPIF oacpi = (ACPIF) GetProcAddress(kdl, "LoadKernelDriver");
					if (oacpi && oacpi((LPWSTR) cpath.c_str(), (LPWSTR) L"HwAcc"))
						activated = (acc = OpenAcpiDevice()) != INVALID_HANDLE_VALUE && acc;
					// In any case, unload dll
					FreeLibrary(kdl);
				}
			}

		}
	}
	Control::~Control() {
		sensors.clear();
		fans.clear();
		powers.clear();
		CloseAcpiDevice(acc);
		UnloadService();
	}

	void Control::UnloadService() {
		if (scManager) {
			CloseAcpiDevice(acc);
			StopService(scManager);
			RemoveService(scManager);
			CloseServiceHandle(scManager);
			scManager = NULL;
		}
	}

	int Control::ReadRamDirect(DWORD offset) {
		if (activated && aDev != -1) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0xFF000000 + offset);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			if (EvalAcpiMethodArgs(acc, dev_c_controls[cDev].readCom, acpiargs, (PVOID *) &res) && res) {
				int res_int = res->Argument[0].Argument;
				free(res);
				return res_int;
			}
		}
		return -1;
	}

	int Control::WriteRamDirect(DWORD offset, byte value) {
		if (activated && aDev != -1) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0xFF000000 + offset);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, value);
			if (EvalAcpiMethodArgs(acc, dev_c_controls[cDev].writeCom, acpiargs, (PVOID *) &res) && res) {
				int res_int = res->Argument[0].Argument;
				free(res);
				return res_int;
			}
		}
		return -1;
	}

	int Control::RunMainCommand(ALIENFAN_COMMAND com, byte value1, byte value2) {
		if (activated && com.com && aDev != -1) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			BYTE operand[4] = {com.sub, value1, value2, 0};
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, com.com);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutBuffArg(acpiargs, 4, operand);
			if (EvalAcpiMethodArgs(acc, devs[aDev].mainCommand, acpiargs, (PVOID *) &res) && res) {
				int res_int = res->Argument[0].Argument;
				free(res);
				return res_int;
			}
		}
		return -1;
	}

	int Control::RunGPUCommand(short com, DWORD packed) {
		if (activated && com) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0x100);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, com);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutBuffArg(acpiargs, 4, (UCHAR*)&packed);
			if (EvalAcpiMethodArgs(acc, "\\_SB.PCI0.PEG0.PEGP.GPS", acpiargs, (PVOID *) &res) && res) {
				int res_int = res->Argument[0].Argument;
				free(res);
				return res_int;
			}
		}
		return -1;
	}

	bool Control::Probe() {
		// Additional temp sensor name pattern
		char tempNamePattern[] = "\\_SB.PCI0.LPCB.EC0.SEN1._STR";
		if (activated) {
			PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
			sensors.clear();
			fans.clear();
			powers.clear();
			// Check device type...
			for (int i = 0; i < NUM_DEVICES; i++) {
				aDev = i;
				cDev = devs[aDev].controlID;
				// Probe...
				if (RunMainCommand(devs[aDev].probe) >= 1) {
					// Alienware device detected!
					// Here is NEW detection block, Dell drop function 0x13 at G5...
					ALIENFAN_SEN_INFO cur = {0};
					powers.push_back(0);
					if (devs[aDev].commandControlled) {
						// for new one-command devices
						int fIndex = 0, funcID = 0;
						// Scan for avaliable fans...
						while ((funcID = RunMainCommand(dev_controls[cDev].getPowerID, fIndex)) < 0x101
							   && funcID != devs[aDev].errorCode) {
							fans.push_back(funcID);
							fIndex++;
						}
						int firstSenIndex = fIndex;
						// AWCC temperature sensors.
						while ((funcID = RunMainCommand(dev_controls[cDev].getPowerID, fIndex)) > 0x100
							   && funcID != devs[aDev].errorCode) {
							cur.senIndex = funcID;
							cur.isFromAWC = true;
							if (fIndex - firstSenIndex < 10) {
								cur.name = temp_names[fIndex - firstSenIndex];
							} else
								cur.name = "Sensor #" + to_string(fIndex - firstSenIndex);
							//switch (fIndex - firstSenIndex) {
							//case 0: cur.name = "CPU Internal Thermistor"; break;
							//case 1: cur.name = "GPU Internal Thermistor"; break;
							//default: cur.name = "FAN#" + to_string(i) + " sensor"; break;
							//}
							sensors.push_back(cur);
							fIndex++;
						}
						if (aDev != 3) {
							while ((funcID = RunMainCommand(dev_controls[cDev].getPowerID, fIndex)) >= 0
								   && funcID != devs[aDev].errorCode) {
								// Power modes.
								powers.push_back(funcID);
								fIndex++;
							}
						}
						// patches... 
						switch (aDev) {
						case 2: // for G5 - hidden performance boost
							powers.push_back(0xAB);
							break;
						case 3: // for Aurora - powers is 1..2
							powers.push_back(1);
							powers.push_back(2);
							break;
						}
					} else {
						// for older command-controlled devices
						// EC Temps...
						for (int i = 0; i < dev_c_controls[cDev].numtemps; i++) {
							cur.senIndex = i;
							cur.isFromAWC = true;
							cur.name = temp_names[i];
							sensors.push_back(cur);
						}
						// EC fans...
						fans.push_back(0x902); // CPU fan
						fans.push_back(0x929); // GPU fan
						// Powers...
						powers.push_back(0xff); // System auto
					}
					// Other temperature sensors
					for (int i = 0; i < 10; i++) {
						tempNamePattern[22] = i + '0';
						if (EvalAcpiMethod(acc, tempNamePattern, (PVOID *) &resName) && resName) {
							char *c_name = new char[1 + resName->Argument[0].DataLength];
							wcstombs_s(NULL, c_name, resName->Argument[0].DataLength, (TCHAR *) resName->Argument[0].Data, resName->Argument[0].DataLength);
							string senName = c_name;
							delete[] c_name;
							cur.senIndex = i;
							cur.name = senName;
							cur.isFromAWC = false;
							sensors.push_back(cur);
							free(resName);
						}
					}
					return true;
				}
			}
			aDev = -1;
		}
		return false;
	}
	int Control::GetFanRPM(int fanID) {
		if (fanID < fans.size())
			if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls[cDev].getFanRPM, (byte) fans[fanID]);
			else
				return ReadRamDirect(fans[fanID]) * 70; // max. 7000 RPM
		return -1;
	}
	int Control::GetFanPercent(int fanID) {
		if (fanID < fans.size())
			if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls[cDev].getFanPercent, (byte) fans[fanID]);
			else
				return ReadRamDirect(fans[fanID]);
		return -1;
	}
	int Control::GetFanValue(int fanID) {
		if (fanID < fans.size()) {
			if (devs[aDev].commandControlled) {
				int value = RunMainCommand(dev_controls[cDev].getFanBoost, (byte) fans[fanID]),
					finalValue = devs[aDev].pwmfans ?
					//(255 - RunMainCommand(dev_controls[devs[aDev].controlID].getFanBoost, (byte) fans[fanID])) * 100 / (255 - devs[aDev].minPwm) :
					value * 100 / devs[aDev].minPwm :
					value;
				return finalValue;
			} else
				return ReadRamDirect(fans[fanID]);
		}
		return -1;
	}
	int Control::SetFanValue(int fanID, byte value, bool force) {
		if (fanID < fans.size()) {
			if (devs[aDev].commandControlled) {
				int finalValue = devs[aDev].pwmfans && !force ? (int) value * devs[aDev].minPwm / 100 : value;
				//255 - (255 - devs[aDev].minPwm) * value / 100 : value;
				return RunMainCommand(dev_controls[cDev].setFanBoost, (byte) fans[fanID], finalValue) != devs[aDev].errorCode;
			} else {
				WriteRamDirect(fans[fanID] + 0x23, value);
				return WriteRamDirect(fans[fanID], value);
			}
		}
		return -1;
	}
	int Control::GetTempValue(int TempID) {
		// Additional temp sensor value pattern
		char tempValuePattern[] = "\\_SB.PCI0.LPCB.EC0.SEN1._TMP";
		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
		if (TempID < sensors.size()) {
			if (sensors[TempID].isFromAWC)
				if (devs[aDev].commandControlled)
					return RunMainCommand(dev_controls[cDev].getTemp, (byte) sensors[TempID].senIndex);
				else {
					if (EvalAcpiMethod(acc, dev_c_controls[cDev].getTemp[TempID], (PVOID *) &res) && res) {
						int res_int = res->Argument[0].Argument;
						free(res);
						return res_int;
					}
				}
			else {
				tempValuePattern[22] = sensors[TempID].senIndex + '0';
				if (EvalAcpiMethod(acc, tempValuePattern, (PVOID *) &res) && res) {
					int res_int = (res->Argument[0].Argument - 0xaac) / 0xa;
					free(res);
					return res_int;
				}
			}
		}
		return -1;
	}
	int Control::Unlock() {
		return SetPower(0);
	}
	int Control::SetPower(int level) {
		if (level < powers.size())
			if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls[cDev].setPower, (byte) powers[level]);
			else {
				byte iValue = powers[level] == 0 ? 0 : 8;
				return WriteRamDirect(dev_c_controls[cDev].unlock, iValue);
			}
		return -1;
	}
	int Control::GetPower() {
		if (devs[aDev].commandControlled) {
			int pl = RunMainCommand(dev_controls[cDev].getPower);
			for (int i = 0; pl >= 0 && i < powers.size(); i++)
				if (powers[i] == pl)
					return i;
		} else {
			return ReadRamDirect(dev_c_controls[cDev].unlock) > 0 ? 1 : 0;
			//PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			//if (EvalAcpiMethod(acc, dev_c_controls[cDev].unlockCom, (PVOID *) &res) && res) {
			//	int res_int = res->Argument[0].Argument;
			//	free(res);
			//	return res_int;
			//}
		}
		return -1;
	}
	int Control::SetGPU(int power) {
		if (power >= 0 && power < 5 && devs[aDev].commandControlled) {
			return RunGPUCommand(dev_controls[cDev].setGPUPower.com, power << 4 | dev_controls[cDev].setGPUPower.sub);
		}
		return -1;
	}
	HANDLE Control::GetHandle() {
		return acc;
	}
	bool Control::IsActivated() {
		return activated;
	}
	int Control::GetErrorCode() {
		return aDev != -1 ? devs[aDev].errorCode : -1;
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
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (!inCommand)
			Prepare();
		if (EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.SRST", (PVOID *) &resName)) {
			free(resName);
			Update();
			return true;
		}
		return false;
	}
	bool Lights::Prepare() {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (!inCommand && EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.ICPC", (PVOID *) &resName)) {
			free(resName);
			inCommand = true;
			return true;
		}
		return false;
	}
	bool Lights::Update() {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (inCommand && EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.RCPC", (PVOID *) &resName)) {
			free(resName);
			inCommand = false;
			return true;
		}
		return false;
	}
	bool Lights::SetColor(byte id, byte r, byte g, byte b) {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = NULL;
		if (!inCommand) {
			Prepare();
		}
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, r);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, g);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, b);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, id);
		if (EvalAcpiMethodArgs(acpi->GetHandle(), "\\_SB.AMW1.SETC", acpiargs, (PVOID *) &resName)) {
			free(resName);
			return true;
		}
		return false;
	}
	bool Lights::SetMode(byte mode, bool onoff) {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = NULL;
		if (!inCommand) {
			Prepare();
		}
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, mode);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, onoff);
		if (EvalAcpiMethodArgs(acpi->GetHandle(), "\\_SB.AMW1.SETB", acpiargs, (PVOID *) &resName)) {
			free(resName);
			return true;
		}
		return false;
	}
	bool Lights::IsActivated() {
		return activated;
	}
}
