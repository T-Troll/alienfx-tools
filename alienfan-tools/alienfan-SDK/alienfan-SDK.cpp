// alienfan-SDK.cpp : Defines the functions for the static library.
//

#include "alienfan-SDK.h"
#include "alienfan-controls.h"
#include "alienfan-low.h"

typedef BOOLEAN (WINAPI *ACPIF)(LPWSTR, LPWSTR);

namespace AlienFan_SDK {

	Control::Control() {

		//printf("Driver activation started.\n");
		// do we already have service runnning?
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
			if (EvalAcpiMethodArgs(acc, dev_c_controls[cDev].readCom.c_str(), acpiargs, (PVOID *) &res) && res) {
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
			if (EvalAcpiMethodArgs(acc, dev_c_controls[cDev].writeCom.c_str(), acpiargs, (PVOID *) &res) && res) {
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
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(NULL, 0);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutIntArg(acpiargs, com.com);
			acpiargs = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) PutBuffArg(acpiargs, 4, operand);
			if (EvalAcpiMethodArgs(acc, devs[aDev].mainCommand.c_str(), acpiargs, (PVOID *) &res) && res) {
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
			//printf("Probing devices... ");
			// Check device type...
			for (int i = 0; i < NUM_DEVICES; i++) {
				aDev = i;
				cDev = devs[aDev].controlID;
				// Probe...
				if (systemID = RunMainCommand(devs[aDev].probe) > 0) {
					// Alienware device detected!
					//printf("Device ID %x found.\n", systemID);
					powers.push_back(0); // Unlocked power
					if (devs[aDev].commandControlled) {
						int fIndex = 0, funcID = 0;
						// Scan for avaliable fans...
						//printf("Scanning data block...\n");
						while ((funcID = RunMainCommand(dev_controls[cDev].getPowerID, fIndex)) < 0x100
							   && funcID > 0 || funcID > 0x130) { // bugfix for 0x132 fan for R7
							fans.push_back(funcID & 0xff);
							fIndex++;
						}
						//printf("%d fans detected, last reply %d\n", fIndex, funcID);
						int firstSenIndex = fIndex;
						// AWCC temperature sensors.
						do {
							string name;
							int sIndex = fIndex - firstSenIndex;
							// Check temperature, disable if -1
							if (RunMainCommand(dev_controls[cDev].getTemp, (byte) funcID) > 0) {
								if (sIndex < temp_names.size()) {
									name = temp_names[sIndex];
								} else
									name = "Sensor #" + to_string(sIndex);
								sensors.push_back({(short) funcID, name, true});
							}
							fIndex++;
						} while ((funcID = RunMainCommand(dev_controls[cDev].getPowerID, fIndex)) > 0x100
								 && funcID < 0x1A0);
						//printf("%d sensors detected, last reply %d\n", HowManySensors(), funcID);
						if (aDev != 3 && funcID > 0) {
							do {
								// Power modes.
								powers.push_back(funcID & 0xff);
								fIndex++;
							} while ((funcID = RunMainCommand(dev_controls[cDev].getPowerID, fIndex)) > 0);
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
						for (short i = 0; i < dev_c_controls[cDev].numtemps; i++) {
							//ALIENFAN_SEN_INFO cur{i, temp_names[i], true};
							sensors.push_back({i, temp_names[i], true});
						}
						// EC fans...
						for (int i = 0; i < dev_c_controls[cDev].numfans; i++)
							fans.push_back(dev_c_controls[cDev].fanID[i]);
						// Powers...
						powers.push_back(0x8); // System auto
					}
					// Other temperature sensors
					for (short i = 0; i < 10; i++) {
						tempNamePattern[22] = i + '0';
						if (EvalAcpiMethod(acc, tempNamePattern, (PVOID *) &resName) && resName) {
							char *c_name = new char[resName->Argument[0].DataLength+1];
							wcstombs_s(NULL, c_name, resName->Argument[0].DataLength, (TCHAR *) resName->Argument[0].Data, resName->Argument[0].DataLength);
							sensors.push_back({i, c_name, false});
							delete[] c_name;
							free(resName);
						}
					}
					//printf("%d TZ sensors detected.\n", HowManySensors());
					// Set boost block
					for (int i = 0; i < fans.size(); i++)
						boosts.push_back(devs[aDev].maxBoost);
					return true;
				}
			}
			aDev = -1;
			//printf("No device found.\n");
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
	int Control::GetFanValue(int fanID, bool force) {
		if (fanID < fans.size()) {
			if (devs[aDev].commandControlled) {
				int value = RunMainCommand(dev_controls[cDev].getFanBoost, (byte) fans[fanID]);
				return force ? value : value * 100 / boosts[fanID];
			} else
				return ReadRamDirect(fans[fanID]);
		}
		return -1;
	}
	int Control::SetFanValue(int fanID, byte value, bool force) {
		if (fanID < fans.size()) {
			if (devs[aDev].commandControlled) {
				int finalValue = force ? value : (int) value * boosts[fanID] / 100;
				//#ifdef _DEBUG
				//	wstring msg = L"Boost for fan#" + to_wstring(fanID) + L" changed to " + to_wstring(finalValue) + L"\n";
				//	OutputDebugString(msg.c_str());
				//#endif
				return RunMainCommand(dev_controls[cDev].setFanBoost, (byte) fans[fanID], finalValue);
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
			if (sensors[TempID].isFromAWC)
				if (devs[aDev].commandControlled)
					return RunMainCommand(dev_controls[cDev].getTemp, (byte) sensors[TempID].senIndex);
				else {
					if (EvalAcpiMethod(acc, dev_c_controls[cDev].getTemp[TempID].c_str(), (PVOID *) &res) && res) {
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
				return RunMainCommand(dev_controls[cDev].setPower, powers[level]);
			else {
				return WriteRamDirect(dev_c_controls[cDev].unlock, powers[level]);
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
			// Always return Auto mode for system safety!
			return 1;
			//int pl = ReadRamDirect(dev_c_controls[cDev].unlock);
			//for (int i = 0; pl >= 0 && i < powers.size(); i++)
			//	if (powers[i] == pl)
			//		return i;
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
