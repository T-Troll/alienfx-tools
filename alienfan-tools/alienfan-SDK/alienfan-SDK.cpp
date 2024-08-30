
#include "alienfan-low.h"
#include "alienfan-SDK.h"
#include "alienfan-controls.h"

#pragma warning(disable:4242)

//#define _TRACE_

typedef BOOLEAN (WINAPI *ACPIF)(LPWSTR, LPWSTR);

static char pathSEN[] = "\\_SB.PCI0.LPCB.EC0.SEN",
			pathECDV[] = "\\_SB.PCI0.LPCB.ECDV.KDRT",
			pathCHRG[] = "\\_SB.PCI0.LPCB.EC0.EB0S";
namespace AlienFan_SDK {

	Control::Control() {

#ifdef _TRACE_
		printf("Driver activation started.\n");
#endif
		// do we already have service running?
		activated = (acc = OpenAcpiDevice()) != INVALID_HANDLE_VALUE && acc;
		if (!activated) {
#ifdef _TRACE_
			printf("Driver not activated, trying to load driver...\n");
#endif
			// We don't, so let's try to start it!
#ifdef _SERVICE_WAY_
			TCHAR  driverLocation[MAX_PATH]{0};
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
#endif
				// Let's try to load driver via kernel hack....
				wchar_t currentPath[MAX_PATH];
				GetModuleFileNameW(NULL, currentPath, MAX_PATH);
				wstring cpath = currentPath;
				cpath.resize(cpath.find_last_of(L"\\"));
				cpath += L"\\HwAcc.sys";
#ifdef _TRACE_
				wprintf(L"Loading driver from %s...\n", cpath.c_str());
#endif
				if (GetFileAttributesW(cpath.c_str()) != INVALID_FILE_ATTRIBUTES) {

					HMODULE kdl = LoadLibrary("kdl.dll");
					if (kdl) {
#ifdef _TRACE_
						printf("KDL loaded, trying... ");
#endif
						ACPIF oacpi = (ACPIF) GetProcAddress(kdl, "LoadKernelDriver");
						if (oacpi && oacpi((LPWSTR) cpath.c_str(), (LPWSTR) L"HwAcc")) {
#ifdef _TRACE_
							printf("Driver loaded, trying to open it... ");
#endif
							activated = (acc = OpenAcpiDevice()) != INVALID_HANDLE_VALUE && acc;
#ifdef _TRACE_
							printf("Loading complete - %s.\n", activated ? "success" : "failed");
#endif
						}
						// In any case, unload dll
						FreeLibrary(kdl);
					}
#ifdef _TRACE_
					else
						printf("KDL library not loaded!\n");
#endif
				}
#ifdef _TRACE_
				else
					printf("Driver file not found!\n");
#endif
#ifdef _SERVICE_WAY_
			}
#endif
		}
	}
	Control::~Control() {
		CloseAcpiDevice(acc);
#ifdef _SERVICE_WAY_
		UnloadService();
#endif
	}

#ifdef _SERVICE_WAY_
	void Control::UnloadService() {
		if (scManager) {
			CloseAcpiDevice(acc);
			StopService(scManager);
			RemoveService(scManager);
			CloseServiceHandle(scManager);
			scManager = NULL;
		}
	}
#endif

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

	int Control::RunMainCommand(ALIENFAN_COMMAND com, byte value1, byte value2) {
		if (aDev != -1) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			BYTE operand[4]{com.sub, value1, value2, 0};
			com.com -= devs[aDev].delta;
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
	}

	bool Control::Probe() {
		if (activated) {
			PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			//sensors.clear();
			//fans.clear();
			//powers.clear();
#ifdef _TRACE_
			printf("Probing devices... ");
#endif
			// Check device type...
			for (int i = 0; i < NUM_DEVICES; i++) {
				aDev = i;
				// Probe...
				if ((RunMainCommand(devs[aDev].probe)) > 0) {
					// Alienware device detected!
					isAlienware = isSupported = true;

					//if (devs[aDev].commandControlled) {

						systemID = RunMainCommand(dev_controls.getSystemID, 2);

#ifdef _TRACE_
						printf("Device ID %d (API %d) found.\n", systemID, aDev);
#endif

						// check G-mode...
						isGmode = RunMainCommand(dev_controls.getGMode) >= 0;

						int fIndex = 0, funcID = 0;
						// Scan for available fans...
						while ((funcID = RunMainCommand(dev_controls.getPowerID, fIndex)) < 0x100
							   && funcID > 0 || funcID > 0x130) { // bugfix for 0x132 fan for R7
							fans.push_back({ (byte)(funcID & 0xff), 0xff });
							fIndex++;
						}
#ifdef _TRACE_
						printf("%d fans detected, last reply %d\n", fIndex, funcID);
#endif
						int firstSenIndex = fIndex;
						// AWCC temperature sensors.
						do {
							string name;
							int sIndex = fIndex - firstSenIndex;
							// Check temperature, disable if -1
							if (RunMainCommand(dev_controls.getTemp, (byte) funcID) > 0) {
								if (sIndex < temp_names.size()) {
									name = temp_names[sIndex];
								} else
									name = "Sensor #" + to_string(sIndex);
								sensors.push_back({ {(byte)funcID, 1}, name});
							}
							fIndex++;
						} while ((funcID = RunMainCommand(dev_controls.getPowerID, fIndex)) > 0x100
								 && funcID < 0x1A0);
#ifdef _TRACE_
						printf("%d sensors detected, last reply %d\n", (int)sensors.size(), funcID);
#endif
						powers.push_back(0); // Unlocked power
						if (!devs[aDev].delta && funcID > 0) {
							do {
								// Power modes.
								powers.push_back(funcID & 0xff);
								fIndex++;
							} while ((funcID = RunMainCommand(dev_controls.getPowerID, fIndex)) && funcID > 0);
#ifdef _TRACE_
							printf("%d power modes detected, last reply %d\n", (int)powers.size(), funcID);
#endif
						}
						else {
							// for Aurora R7 and Area 51 - powers is 1..2
							powers.push_back(1);
							//powers.push_back(2);
						}
					//}
					//else {
					//	// for older command-controlled devices
					//	// EC Temps...
					//	for (short i = 0; i < dev_c_controls.numtemps; i++) {
					//		//ALIENFAN_SEN_INFO cur{i, temp_names[i], true};
					//		sensors.push_back({ {(byte)i, 1}, temp_names[i]});
					//	}
					//	// EC fans...
					//	for (int i = 0; i < dev_c_controls.numfans; i++)
					//		fans.push_back({ (byte)dev_c_controls.fanID[i], 0xff });
					//	// Powers...
					//	powers.push_back(0x8); // System auto
					//}
					// Other temperature sensors
					for (short i = 0; i < 10; i++) {
						string senPath = pathSEN + to_string(i) + "._STR";
						if (EvalAcpiMethod(acc, senPath.c_str(), (PVOID *) &resName, NULL) && resName) {
							char *c_name = new char[resName->Argument[0].DataLength+1];
							wcstombs_s(NULL, c_name, resName->Argument[0].DataLength, (wchar_t *) resName->Argument[0].Data, resName->Argument[0].DataLength);
							sensors.push_back({ {(byte)i, 0}, c_name });
							delete[] c_name;
							free(resName);
						}
					}
#ifdef _TRACE_
					printf("%zd SEN blocks detected\n", sensors.size());
#endif
					// ECDV temp sensors...
					for (byte sind = 2; sind < 4; sind++) {
						short numECDV = 0; bool okECDV = false;
						pathECDV[7] = (sind == 2) ? 'I' : '0';
						acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, numECDV);
						while (EvalAcpiMethod(acc, pathECDV, (PVOID*)&resName, acpiargs) && resName && resName->Argument[0].Argument < 110) {
							sensors.push_back({ {(byte)numECDV, (byte)sind}, "ECDV-" + to_string(numECDV + 1) });
							numECDV++;
							free(resName);
							acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, numECDV);
						}
						if (resName)
							free(resName);
#ifdef _TRACE_
						printf("%d ECDV-%d sensors found.\n", numECDV, sind);
#endif
					}

					// Charge control...
					if ((isCharge = EvalAcpiMethod(acc, pathCHRG, (PVOID*)&resName, NULL)) && resName) {
#ifdef _TRACE_
						printf("Charge control result %d\n", resName->Argument[0].Argument);
#endif
					}

					// fan mappings...
					for (auto fan = fans.begin(); fan != fans.end(); fan++) {
						ALIENFAN_SEN_INFO sen = { (byte)RunMainCommand(dev_controls.getFanType, fan->id), 1 };
						for (byte i = 0; i < sensors.size(); i++)
							if (sensors[i].sid == sen.sid) {
								fan->type = i;
								break;
							}
					}
					return true;
				}
			}
			aDev = -1;
#ifdef _TRACE_
			printf("No devices found.\n");
#endif
		}
		return false;
	}
	int Control::GetFanRPM(int fanID) {
		if (fanID < fans.size())
			//if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls.getFanRPM, (byte) fans[fanID].id);
			//else
			//	return ReadRamDirect(fans[fanID].id) * 70; // max. 7000 RPM
		return -1;
	}
	int Control::GetFanPercent(int fanID) {
		if (fanID < fans.size())
			//if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls.getFanPercent, (byte) fans[fanID].id);
			//else
			//	return ReadRamDirect(fans[fanID].id);
		return -1;
	}
	int Control::GetFanBoost(int fanID) {
		if (fanID < fans.size()) {
			//if (devs[aDev].commandControlled) {
				return RunMainCommand(dev_controls.getFanBoost, (byte)fans[fanID].id);
			//} else
			//	return ReadRamDirect(fans[fanID].id);
		}
		return -1;
	}
	int Control::SetFanBoost(int fanID, byte value) {
		if (fanID < fans.size()) {
			//if (devs[aDev].commandControlled) {
				return RunMainCommand(dev_controls.setFanBoost, (byte) fans[fanID].id, value);
			//} else {
			//	WriteRamDirect(fans[fanID].id + 0x23, value + 1); // lock at 0 fix
//	return WriteRamDirect(fans[fanID].id, value);
//}
		}
		return -1;
	}
	int Control::GetTempValue(int TempID) {
		// Additional temp sensor value pattern
		//char tempValuePattern[] = "\\_SB.PCI0.LPCB.EC0.SEN1._TMP";
		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
		if (TempID < sensors.size()) {
			switch (sensors[TempID].type) {
			case 1: // AWCC
				//if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls.getTemp, (byte)sensors[TempID].index);
				/*else {
					if (EvalAcpiMethod(acc, dev_c_controls.getTemp[TempID].c_str(), (PVOID*)&res, NULL) && res) {
						int res_int = res->Argument[0].Argument;
						free(res);
						return res_int;
					}
				}*/
				break;
			case 0: { // TZ
				//tempValuePattern[22] = sensors[TempID].index + '0';
				string senPath = pathSEN + to_string(sensors[TempID].index) + "._TMP";
				if (EvalAcpiMethod(acc, senPath.c_str(), (PVOID*)&res, NULL) && res) {
					int res_int = (res->Argument[0].Argument - 0xaac) / 0xa;
					free(res);
					return res_int;
				}
			} break;
			case 2: case 3: { // tempECDV
				PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
				acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, sensors[TempID].index);
				pathECDV[7] = (sensors[TempID].type == 2) ? 'I' : '0';
				if (EvalAcpiMethod(acc, pathECDV, (PVOID*)&res, acpiargs) && res) {
					int res_int = res->Argument[0].Argument;
					free(res);
					return res_int;
				}
			} break;
			}
		}
		return -1;
	}
	int Control::Unlock() {
		return SetPower(0);
	}
	int Control::SetPower(byte level) {
		// ToDo: make correct mode set for devs[aDev].delta
		lastMode = level;
		if (devs[aDev].delta && !level)
			++level;
		//if (level < powers.size())
			//if (devs[aDev].commandControlled)
			return RunMainCommand(dev_controls.setPower, level);
		//else {
		//	return WriteRamDirect(dev_c_controls.unlock, level);
		//}
		//return -1;
	}
	int Control::GetPower(bool raw) {
		if (devs[aDev].delta)
			return lastMode;
		int pl = RunMainCommand(dev_controls.getPower);
		if (raw || pl < 0)
			return pl;
		for (int i = 0; pl >= 0 && i < powers.size(); i++)
			if (powers[i] == pl)
				return i;
		return -1;
	}
	int Control::SetGPU(int power) {
		if (power >= 0 && power < 5 /*&& devs[aDev].commandControlled*/) {
			return RunGPUCommand(dev_controls.setGPUPower.com, power << 4 | dev_controls.setGPUPower.sub);
		}
		return -1;
	}

	int Control::SetGMode(bool state)
	{
		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
		if (!EvalAcpiMethod(acc, "\\_SB.PCI0.LPC0.EC0._Q14", (PVOID*)&res, NULL)) {
			if (state)
				SetPower(0xAB);
			return RunMainCommand(dev_controls.setGMode, state);
		}
		else
			return GetGMode();
	}

	int Control::GetGMode() {
		return isGmode ? GetPower() < 0 || RunMainCommand(dev_controls.getGMode) : 0;
	}

	int Control::GetMaxRPM(int fanID) {
		return RunMainCommand(dev_controls.getMaxRPM, fans[fanID].id);
	}

	int Control::GetCharge() {
		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
		int res_int = -1;
		if (EvalAcpiMethod(acc, pathCHRG, (PVOID*)&res, NULL) && res) {
			res_int = res->Argument[0].Argument;
			free(res);
		}
		return res_int;
	}

	int Control::SetCharge(byte val) {
		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, val);
		if (EvalAcpiMethod(acc, pathCHRG, (PVOID*)&res, acpiargs) && res) {
			int res_int = res->Argument[0].Argument;
			free(res);
			return res_int;
		}
		return -1;
	}

	//HANDLE Control::GetHandle() {
	//	return acc;
	//}
	//bool Control::IsActivated() {
	//	return activated;
	//}
	//int Control::HowManyFans() {
	//	return (int)fans.size();
	//}
	//int Control::HowManyPower() {
	//	return (int)powers.size();
	//}
	//int Control::HowManySensors() {
	//	return (int)sensors.size();
	//}
	//int Control::GetVersion() {
	//	return aDev + 1;
	//}
	Lights::Lights(Control *ac) {
		acpi = ac;
		// Probe lights...
		if (Prepare())
			isActivated = true;
	}
	bool Lights::Reset() {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (!inCommand)
			Prepare();
		if (EvalAcpiMethod(acpi->acc, "\\_SB.AMW1.SRST", (PVOID *) &resName, NULL)) {
			free(resName);
			Update();
			return true;
		}
		return false;
	}
	bool Lights::Prepare() {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (!inCommand && EvalAcpiMethod(acpi->acc, "\\_SB.AMW1.ICPC", (PVOID *) &resName, NULL)) {
			free(resName);
			inCommand = true;
			return true;
		}
		return false;
	}
	bool Lights::Update() {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (inCommand && EvalAcpiMethod(acpi->acc, "\\_SB.AMW1.RCPC", (PVOID *) &resName, NULL)) {
			free(resName);
			inCommand = false;
			return true;
		}
		return false;
	}
	bool Lights::SetColor(byte mask, byte r, byte g, byte b, bool save) {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = NULL;
		if (!inCommand) {
			Prepare();
		}
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, r);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, g);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, b);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, mask);
		if (EvalAcpiMethod(acpi->acc, "\\_SB.AMW1.SETC", (PVOID *) &resName, acpiargs)) {
			free(resName);
			return true;
		}
		return false;
	}

	bool Lights::SetBrightness(byte mode) {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs = NULL;
		if (!inCommand) {
			Prepare();
		}
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, mode);
		acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 1);
		if (EvalAcpiMethod(acpi->acc, "\\_SB.AMW1.SETB", (PVOID *) &resName, acpiargs)) {
			free(resName);
			return true;
		}
		return false;
	}
}
