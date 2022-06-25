// alienfan-SDK.cpp : Defines the functions for the static library.
//

#include "alienfan-SDK.h"
#include "alienfan-controls.h"

typedef BOOLEAN (WINAPI *ACPIF)(LPWSTR, LPWSTR);

static char tempNamePattern[] = "\\_SB.PCI0.LPCB.EC0.SEN1._STR",
			tempECDV1[] = "\\_SB.PCI0.LPCB.ECDV.KDRT",
			tempECDV2[] = "\\_SB.PC00.LPCB.ECDV.KDRT";

namespace AlienFan_SDK {

	Control::Control() {

		//printf("Driver activation started.\n");
		// do we already have service running?
		activated = (acc = OpenAcpiDevice()) != INVALID_HANDLE_VALUE && acc;
		if (!activated) {
			//printf("Device not activated, trying to load driver...\n");
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

				//wprintf(L"Loading driver from %s...\n", cpath.c_str());
				if (GetFileAttributes(cpath.c_str()) != INVALID_FILE_ATTRIBUTES) {

					HMODULE kdl = LoadLibrary(L"kdl.dll");
					if (kdl) {
						//printf("KDL loaded, trying... ");
						ACPIF oacpi = (ACPIF) GetProcAddress(kdl, "LoadKernelDriver");
						if (oacpi && oacpi((LPWSTR) cpath.c_str(), (LPWSTR) L"HwAcc")) {
							//printf("Driver loaded, trying to open it... ");
							activated = (acc = OpenAcpiDevice()) != INVALID_HANDLE_VALUE && acc;
							//printf("Loading complete - %s.\n", activated ? "success" : "failed");
						}
						// In any case, unload dll
						FreeLibrary(kdl);
					} //else
						//printf("KDL library not found!\n");
				} //else
					//printf("Driver file not found!\n");
#ifdef _SERVICE_WAY_
			}
#endif
		}
		//printf("Done.\n");
	}
	Control::~Control() {
		sensors.clear();
		fans.clear();
		powers.clear();
		boosts.clear();
		maxrpm.clear();
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

	int Control::ReadRamDirect(DWORD offset) {
		if (activated && aDev != -1) {
			PACPI_EVAL_OUTPUT_BUFFER res = NULL;
			PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0xFF000000 | offset);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			if (EvalAcpiMethod(acc, dev_c_controls.readCom.c_str(), (PVOID *) &res, acpiargs) && res) {
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
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0xFF000000 | offset);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, value);
			if (EvalAcpiMethod(acc, dev_c_controls.writeCom.c_str(), (PVOID *) &res, acpiargs) && res) {
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
	}

	bool Control::Probe() {
		// Additional temp sensor name pattern
		if (activated) {
			PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
			sensors.clear();
			fans.clear();
			powers.clear();
			//printf("Probing devices... ");
			// Check device type...
			for (int i = 0; i < NUM_DEVICES; i++) {
				aDev = i;
				// Probe...
				if ((systemID = RunMainCommand(devs[aDev].probe)) > 0) {
					// Alienware device detected!
					//printf("Device ID %x (API %d) found.\n", systemID, aDev);
					powers.push_back(0); // Unlocked power
					if (devs[aDev].commandControlled) {
						int fIndex = 0, funcID = 0;
						// Scan for available fans...
						//printf("Scanning data block...\n");
						while ((funcID = RunMainCommand(dev_controls.getPowerID, fIndex)) < 0x100
							   && funcID > 0 || funcID > 0x130) { // bugfix for 0x132 fan for R7
							fans.push_back(funcID & 0xff);
							boosts.push_back(100);
							maxrpm.push_back(0);
							fIndex++;
						}
						//printf("%d fans detected, last reply %d\n", fIndex, funcID);
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
								sensors.push_back({(short) funcID, name, 1});
							}
							fIndex++;
						} while ((funcID = RunMainCommand(dev_controls.getPowerID, fIndex)) > 0x100
								 && funcID < 0x1A0);
						//printf("%d sensors detected, last reply %d\n", HowManySensors(), funcID);
						if (aDev != 3 && funcID > 0) {
							do {
								// Power modes.
								powers.push_back(funcID & 0xff);
								fIndex++;
							} while ((funcID = RunMainCommand(dev_controls.getPowerID, fIndex)) > 0);
							//printf("%d power modes detected, last reply %d\n", HowManyPower(), funcID);
						}
						// patches...
						switch (aDev) {
						case 2: // for G5 - hidden performance boost
							powers.push_back(0xAB);
							break;
						case 3: case 4: // for Aurora R7 and Area 51 - powers is 1..2
							powers.push_back(1);
							powers.push_back(2);
							break;
						}
					} else {
						// for older command-controlled devices
						// EC Temps...
						for (short i = 0; i < dev_c_controls.numtemps; i++) {
							//ALIENFAN_SEN_INFO cur{i, temp_names[i], true};
							sensors.push_back({i, temp_names[i], true});
						}
						// EC fans...
						for (int i = 0; i < dev_c_controls.numfans; i++)
							fans.push_back(dev_c_controls.fanID[i]);
						// Powers...
						powers.push_back(0x8); // System auto
					}
					// Other temperature sensors
					for (short i = 0; i < 10; i++) {
						tempNamePattern[22] = i + '0';
						if (EvalAcpiMethod(acc, tempNamePattern, (PVOID *) &resName, NULL) && resName) {
							char *c_name = new char[resName->Argument[0].DataLength+1];
							wcstombs_s(NULL, c_name, resName->Argument[0].DataLength, (TCHAR *) resName->Argument[0].Data, resName->Argument[0].DataLength);
							sensors.push_back({i, c_name, 0});
							delete[] c_name;
							free(resName);
						}
					}
					//printf("%d SEN blocks detected\n", sensors.size());
					// ECDV temp sensors...
					short numECDV = 0; bool okECDV = false;
					PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
					acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, numECDV);
					//printf("Trying %s... ", tempECDV1);
					if (EvalAcpiMethod(acc, tempECDV1, (PVOID*)&resName, acpiargs))
						do {
							if (okECDV = (resName->Argument[0].Argument != 255)) {
								free(resName);
								sensors.push_back({ numECDV, "ECDV-" + to_string(numECDV + 1), 2 });
								numECDV++;
								acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, numECDV);
								EvalAcpiMethod(acc, tempECDV1, (PVOID*)&resName, acpiargs);
							} else
								free(resName);
						} while (okECDV);
					else {
						//printf("failed, trying %s...", tempECDV2);
						acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, numECDV);
						if (EvalAcpiMethod(acc, tempECDV2, (PVOID*)&resName, acpiargs))
							do {
								if (okECDV = (resName->Argument[0].Argument != 255)) {
									free(resName);
									sensors.push_back({ numECDV, "ECDV-" + to_string(numECDV + 1), 3 });
									numECDV++;
									acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, numECDV);
									EvalAcpiMethod(acc, tempECDV2, (PVOID*)&resName, acpiargs);
								}
								else
									free(resName);
							} while (okECDV);
						/*else
							printf("Failed.");*/
					}
					//printf("%d TZ sensors detected.\n", HowManySensors());
					// Set boost block
					//for (int i = 0; i < fans.size(); i++)
					//	boosts.push_back(devs[aDev].maxBoost);
					return true;
				}
				//else
				//	printf("%d returned\n", systemID);
			}
			aDev = -1;
			//printf("No devices found.\n");
		}
		return false;
	}
	int Control::GetFanRPM(int fanID) {
		if (fanID < fans.size())
			if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls.getFanRPM, (byte) fans[fanID]);
			else
				return ReadRamDirect(fans[fanID]) * 70; // max. 7000 RPM
		return -1;
	}
	int Control::GetFanPercent(int fanID) {
		if (fanID < fans.size())
			if (fanID < maxrpm.size() && maxrpm[fanID])
				return GetFanRPM(fanID) * 100 / maxrpm[fanID];
			else
				if (devs[aDev].commandControlled)
					return RunMainCommand(dev_controls.getFanPercent, (byte) fans[fanID]);
				else
					return ReadRamDirect(fans[fanID]);
		return -1;
	}
	int Control::GetFanBoost(int fanID, bool force) {
		if (fanID < fans.size()) {
			if (devs[aDev].commandControlled) {
				int value = RunMainCommand(dev_controls.getFanBoost, (byte) fans[fanID]);
				return force ? value : value * 100 / boosts[fanID];
			} else
				return ReadRamDirect(fans[fanID]);
		}
		return -1;
	}
	int Control::SetFanBoost(int fanID, byte value, bool force) {
		if (fanID < fans.size()) {
			if (devs[aDev].commandControlled) {
				int finalValue = force ? value : (int) value * boosts[fanID] / 100;
				return RunMainCommand(dev_controls.setFanBoost, (byte) fans[fanID], finalValue);
			} else {
				WriteRamDirect(fans[fanID] + 0x23, value + 1); // lock at 0 fix
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
			switch (sensors[TempID].type) {
			case 1: // AWCC
				if (devs[aDev].commandControlled)
					return RunMainCommand(dev_controls.getTemp, (byte)sensors[TempID].senIndex);
				else {
					if (EvalAcpiMethod(acc, dev_c_controls.getTemp[TempID].c_str(), (PVOID*)&res, NULL) && res) {
						int res_int = res->Argument[0].Argument;
						free(res);
						return res_int;
					}
				}
				break;
			case 0: // TZ
				tempValuePattern[22] = sensors[TempID].senIndex + '0';
				if (EvalAcpiMethod(acc, tempValuePattern, (PVOID*)&res, NULL) && res) {
					int res_int = (res->Argument[0].Argument - 0xaac) / 0xa;
					free(res);
					return res_int;
				}
				break;
			case 2: case 3: { // tempECDV
				PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX acpiargs;
				acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)PutIntArg(NULL, sensors[TempID].senIndex);
				if (EvalAcpiMethod(acc, sensors[TempID].type == 2 ? tempECDV1 : tempECDV2, (PVOID*)&res, acpiargs) && res) {
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
	int Control::SetPower(int level) {
		if (level < powers.size())
			if (devs[aDev].commandControlled)
				return RunMainCommand(dev_controls.setPower, powers[level]);
			else {
				return WriteRamDirect(dev_c_controls.unlock, powers[level]);
			}
		return -1;
	}
	int Control::GetPower() {
		if (devs[aDev].commandControlled) {
			int pl = RunMainCommand(dev_controls.getPower);
			for (int i = 0; pl >= 0 && i < powers.size(); i++)
				if (powers[i] == pl)
					return i;
		} else {
			// Always return Auto mode for system safety!
			return 1;
			//int pl = ReadRamDirect(dev_c_controls.unlock);
			//for (int i = 0; pl >= 0 && i < powers.size(); i++)
			//	if (powers[i] == pl)
			//		return i;
		}
		return -1;
	}
	int Control::SetGPU(int power) {
		if (power >= 0 && power < 5 && devs[aDev].commandControlled) {
			return RunGPUCommand(dev_controls.setGPUPower.com, power << 4 | dev_controls.setGPUPower.sub);
		}
		return -1;
	}

	int Control::SetGMode(bool state)
	{
		PACPI_EVAL_OUTPUT_BUFFER res = NULL;
		if (!EvalAcpiMethod(acc, "\\_SB.PCI0.LPC0.EC0._Q14", (PVOID*)&res, NULL))
			return RunMainCommand(dev_controls.setGMode, state);
		else
			return GetGMode();
	}

	int Control::GetGMode() {
		if (GetPower() < 0)
			return 1;
		else
			return RunMainCommand(dev_controls.getGMode);
	}

	HANDLE Control::GetHandle() {
		return acc;
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
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (!inCommand)
			Prepare();
		if (EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.SRST", (PVOID *) &resName, NULL)) {
			free(resName);
			Update();
			return true;
		}
		return false;
	}
	bool Lights::Prepare() {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (!inCommand && EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.ICPC", (PVOID *) &resName, NULL)) {
			free(resName);
			inCommand = true;
			return true;
		}
		return false;
	}
	bool Lights::Update() {
		PACPI_EVAL_OUTPUT_BUFFER resName = NULL;
		if (inCommand && EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.RCPC", (PVOID *) &resName, NULL)) {
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
		if (EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.SETC", (PVOID *) &resName, acpiargs)) {
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
		if (EvalAcpiMethod(acpi->GetHandle(), "\\_SB.AMW1.SETB", (PVOID *) &resName, acpiargs)) {
			free(resName);
			return true;
		}
		return false;
	}
	bool Lights::IsActivated() {
		return activated;
	}
}
