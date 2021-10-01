# alienfan-tools
Utilities for fan and power, and sometimes lights control for Alienware and Dell G-series devices trough direct calls into ACPI BIOS.  
This is the side step from my [alienfx-tools](https://github.com/T-Troll/alienfx-tools) project, dedicated to fan and power control subsystem.

Tools avaliable:
- `alienfan-cli` - simple fan control command line utility
- `alienfan-gui` - Fan control GUI application

## Disclaimer
- **Antiviruses will detect virus into package**. It's not a virus, in fact, but the kernel hack for load driver. If you don't like it:
  - Remove kdl.dll and drv64.dll from package (or you antivirus do so).
  - Use "Test mode" instalation method.
- This tools utilize low-level ACPI functions access, it can provide BSOD at start if supported ACPI bus not found! Use with care, at you own risk!

## How it works?
Instead of many other fan control tools, like `SpeedFan`, `HWINFO` or `Dell Fan Control`, this tools does not use direct EC (Embed controller) access and data modification.  
My tools utilize propietary Alienware function calls inside ACPI BIOS instead (the same as AWCC).
- It's more safe - BIOS still monitor fans and have no risk fans will be stopped at full load.
- It's more universal - Most Alienware systems have the same interface.
- In some cases, this is the only way - for example, Alienware m15/m17R1 does not have EC control at all.

## Known issues
- ACPI driver can hang into stopping state. Run `alienfan-cli` without parameters to reset it.
- Fan PRMs boost is quite fast, but if you lower boost, it took some time to slow down fans. 
- Power control only avaliable on some models, f.e. m15R1, m15R3.
- Power control modes not detected in power grow order, so check real PL1 after set using other tool, f.e. HWINFO.  
- Set Power control to non-zero value can block (lock back) fan control.
- As usual, AWCC service can interfere (reset values from time to time), so it's reccomended to stop it.

## Requrements
- Windows 10 x64 OS revision 1706 or higher. Any other OS **Does not supported!**
- There are 2 ways to prepare you system for driver loading:
  - You can go into BIOS and disable "Secure boot" option, reboot, then issue `bcdedit /set testsigning on` command from Administarator command prompt and reboot again. You should see "Test mode" watermark on desktop if you do all correct.
  - Do nothing, in this case driver will be installed using kernel hack. Some antivirus can complain, because it's close to rootkit way. Anyway, it will not stay in memory after reboot.
- Supported Dell/Alienware hardware.

## Installation
Unpack tools into folder, run exe.  
NB: You should have hwacc.sys into the same folder.

## Supported hardware
- Notebooks: `Alienware m15/17R1` or later, `Alienware Area51m-R1` or later.
- Should work, but not tested completely: `Alienware Area51m R2`
- Desktops: `Alienware Aurora R7` or later (GPU control not supported yet).
- GPU power control support NVidia GPU only. Radeon GPU need other commands i don't know for now.

### Tested gear:
- `Alienware m15R1`: 
  - 5 Power modes (unlocked 60W, 60W, 75W, 45W with 65W GPU lock, 45W with 90W GPU lock)
  - 4 GPU modes (90W...65W limit)
  - 7 temperature sensors
- `Alienware m15R4`:
  - 5 Power modes (unlocked 45W, 105W(!), 105W, 105W, 45W)
  - 4 GPU modes... But do nothing, just return "OK"
  - 7 Temperature sensors
- `Dell G5 15`:
  - 2 Power modes (but only one can be selected)
  - 2 Temperature sensors
  - PWM control
- `Dell G5 SE` (AMD)
  - 4 Power modes (one secret, but you can select it)
  - 2 Temperature sensors

## `alienfan-gui` usage

![alienfan-gui](/img/alienfan.png?raw=true)

GUI application for fan control.  
Then you start it, you will see 3 main windows - Temperaturs (with current reading), Fans (with checkboxes and current RPM) and Fan curve (graph).  
Also, "Power mode" dropdown avaliable to select global power mode.  
"GPU limit" slider define top maximal GPU board power limit shift. 0 (left) is no limit, 4 (right) - maximal limit. This feature only supported at some notebooks, and real limit can vary. You can use this value to keep more power at CPU or extend battery life.

NB: Fans can only be controled if Power Mode set to "Manual"!

```
How it works
```

Fan control is temerature sensor-driven, so first select one of temperature sensors.  
Then, select which fan(s) should react on it's readings - you can select from none to all in any combination.  
So select checkbox for fan(s) you need.

After you doing so, currently selected (checked or not) fan settings will be shown at "Fan Settings" window - you will see selected fan boost at the top, and the fan control curve (green lines).
Now play with fan control curve - it defines fan boost by temperature level. X axle is temperature, Y axle is boost level.  
You can left click (and drag until release mouse button) into the curve window to add point or select close point (if any) and move it.  
You can click right mouse button at the graph point to remove it.  
Big red dot represent current boost-in-action position.  

Please keep in mind:
- You can't remove first or last point of the curve.
- If you move first or last point, it will keep it's temperature after button release - but you can set other boost level for it.
- Then fan controlled by more, then one sensor, boost will be set to the maximal value across them. 

"Reset" button reset currently selected fan curve to default one (0-0 boost).

You can minimize application to tray pressing Minimize button (or the top one), left click on try icon restore application back, right click close application.
There are two settings into application menu "Settings":
- "Start with windows" - If checked, application will run at system start.
- "Start minimized" - If checked, application hide into tray icon after start.

## `alienfan-cli` usage
It's a simple CLI fan control utility for now, providing the same functionality as AWCC (well... a bit more already).
ACPI can't control fans directly (well... i'm working on it), so all you can do is set fan boost (More RPM).
Run `alienfan-cli [command[=value{,value}] [command...]]`. After start, it detect you gear and show number of fans, temperature sensors and power modes avaliable.
Avaliable commands:
- `usage`, `help` - Show short help
- `rpm` - Show current fan RPMs
- `temp` - Show known temperature sensors name and value
- `unlock` - Enable manual fan control
- `getpower` - Print current power mode
- `power=<value>` - Set system-defined power level. Possible levels autodetected from ACPI, see message at app start 
- `gpu=<value>` - Set GPU power limit. Possible values from 0 (no limit) to 4 (max. limit).
- `getfans` - Show current fan RPMs boost
- `setfandirect=<fanID>,<value>` - Set one fan to exact boost value. If fan is PWM-controlled, it will be direct PWM value.
- `setfans=<fan1>,<fan2>...` - Set fans RPM boost level (0..100 - in percent). Fan1 is for CPU fan, Fan2 for GPU one. Number of arguments should be the same as number of fans application detect
- `resetcolor` - Reset color system
- `setcolor=<mask>,r,g,b` - Set light(s) defined by mask to color
- `setcolormode=<mode>,<flag>` - Set light system mode
- `direct=<id>,<subid>[,val,val]` - Issue direct Alienware interface command (see below)  
- `directgpu=<id>,<value>` - Issue direct GPU interface command (see below)

NB: Setting Power level to non-zero value disables manual fan control!  

`direct` command is for testing various functions of the main Alienware ACPI function.  
If default functions doesn't works, you can check and try to find you system subset.

For example: issuing command `direct=20,5,50` return fan RPM for fan #1 on laptop, but for desktop the command should be different.

You can check possible commands and values yourself, opening you system ACPI dump file and searching for `Method(WMAX` function.  
It accept 3 parameters - first is not used, second is a command, and the third is byte array of sumcommand and 2 value bytes.  
Looking inside this method can reveal commands supported for you system and what they do.  
For example, for Aurora R7 command `direct=3,N` return fan ID for fan N or -1 (fail) if fan absent.

You can share commands you find with me, and i'll add it into applications.

`directgpu` command doing the same for GPU subsystem, altering some chip settings.

NB: for both `direct` commands, all values are not decimal, but hex (like c8, a3, etc)! It made easy to test values found into ACPI dump.

## ToDo:
- [x] Temperature sensors reading
- [x] Eliminate "Test mode" requirement
- [ ] Additional hardware support (thanks for ACPI dumps, provided by `alienfx-tools` community!):
  - [x] Alienware m15/m17 (any release)
  - [ ] Older Alienware (need to repair my old one to check)
  - [x] Area 51m
  - [x] Dell G3/G5 (non-AMD)
  - [x] Dell G5 SE (AMD)
  - [x] Dell G5 15 (AMD)
  - [x] Desktops (Aurora R7 and later)
- [x] SDK lib for easy sharing
- [X] GUI 
- [X] Temp-RPM curves and indirect RPM control
- [x] CPU power limit control
- [x] GPU power limit contol
- [x] Direct fan RPM control (for G15 only for now)
- [ ] Dynamic power distribution (CPU/GPU power share distribution)
- [x] `alienfx-gui` integration

## Tools Used
* Visual Studio Community 2019

## License
MIT. You can use these tools for any non-commercial or commercial use, modify it any way - supposing you provide a link to this page from you product page and mention me as the one of authors.

## Credits
Idea, code and hardware support by T-Troll.  
ACPI driver based on kdshk's [WindowsHwAccess](https://github.com/kdshk/WindowsHwAccess).  
Kernel loading hack based on hfiref0x's [KDU](https://github.com/hfiref0x/KDU)  
Special thanks to [DavidLapous](https://github.com/DavidLapous) for inspiration and advices!


