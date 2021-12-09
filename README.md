# Alienfx tools
AWCC don't needed anymore - here are light weighted tools for Alienware systems lights,fans,power profile control:
- [AlienFX Control](/Doc/alienfx-gui.md) - AWCC alternative in 500kb. You can control you system lights (including hardware and software effects such as system parameters monitoring, ambient lights, sound haptic), fans, temperatures, power settings and a lot more.
- [alienfx-cli](/Doc/alienfx-cli.md) - Make changes and check status of your AlienFX lights from the CLI (command line interface).
- [LightFX](/Doc/LightFX.md) - Dell LightFX library emulator. Support all Dell's API functions using my low-level SDK.
- [alienfx-probe](/Doc/alienfx-probe.md) - CLI application to probe devices and lights and name it for using into other applications.

Some additional tools added from my other [`Alienfan-tools`](https://github.com/T-Troll/alienfan-tools) project:
- Alienfan GUI control - simple fan and power control utility. Set you fan parameters according to any system temperature sensor, switch system power modes...
- Alienfan CLI - Command line interface tool for control fans and power from command line.
- Alienfan Overboost - CLI tool for manual/automatic fan overboost (set higher RPM that BIOS suggest).  
Readme is avaliable [here](https://github.com/T-Troll/alienfan-tools).

## Disclaimer
Starting from the release 4.2.1, **Antiviruses can detect virus into project package**.  
It's not a virus, in fact, but the kernel hack for load driver. You should add `HwAcc.sys`, `kdl.dll` and `drv64.dll` into antivirus excetion list or do not use fan control.

## Requirements
- Alienware light device present into the system and have USB HID driver active (`alienfx-cli` can work even if device not found, but Dell LightFX present into system).
- Windows 10 v1803 or later (binary files for 64-bit only, but you can compile project for 32-bit as well).
- `alienfan-gui` and `alienfan-cli` always require Administrator rights to work (for communication with hardware).
- `alienfx-gui` requre Administrator rights in some cases:
  - "Disable AWCC" selected in Settings (stopping AWCC service require Administrator privilegy)
  - "Esif temperature" selected (access to ESIF values blocked from user account)
  - "Enable Fan control" selected (the same reason as for `alienfan-gui`)

## Installation
- Download latest release archive or installer package from [here](https://github.com/T-Troll/alienfx-tools/releases).  
- (Optional) `Ambient` effect mode uses DirectX for screen capturing, so you need to download and install it from [here](https://www.microsoft.com/en-us/download/details.aspx?id=35). Other modes doesn't require it, so you need it in case if you plan to use `Ambient` effects only.
- (Optional) For LightFX-enabled games/applications, copy `LightFx.dll` into game/application folder.
- (Optional) For `alienfx-cli` and `alienfx-probe` high-level support, any of my emulated (see above) or Alienware LightFX DLLs should be installed on your computer. These are automatically installed with Alienware Command Center and should be picked up by this program. You also should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu then "Allow 3rd-party applications" in older Desktop version. 
- (Optional) If you plan to use `alienfx-gui` fan control or any of Aliefan tools, check [alienfan readme](https://github.com/T-Troll/alienfan-tools) for possible configuration alternatives.
- Unpack the archive to any directory of your choise or just run installer.  
- After installation, run `alienfx-gui` or `alienfx-probe` to check and set light names (all apps will have limited to no functionality without this step).  

Run any tool you need from this folder or start menu!

## Supported hardware:

Virtually any Alienware/Dell G-series notebook and desktop.  
Project Wiki have [more details and the list of tested devices](https://github.com/T-Troll/alienfx-tools/wiki/Supported-and-tested-devices-list).  
If your device not supported, you can [help me to support it](https://github.com/T-Troll/alienfx-tools/wiki/How-to-collect-data-for-the-new-light-device).

## Known issues
- Some High-level (Dell) SDK functions can not work as designed. This may be fixed in upcoming AWCC updates. It's not an `alienfx-cli` bug.
- Hardware light effects breathing, spectrum, rainbow doesn't supported for older (APIv1-v3) devices.
- Hardware light effects (and global effect) didn't work with software light effects at the same time for APIv4-v5 (hardware bug, "Update" command stop all effects). Disable monitoring in `alienfx-gui` to use it.
- DirectX12 games didn't allow to access GPU or frame, so `Ambient` effect will not work, and `alienfx-gui` can't handle GPU load for it correctly.
- Using hardware power button, especially for events, can provide hardware light system acting slow right after color update! `alienfx-gui` will switch to "Devices" tab or quit with visible delay.
- **WARNING!** Strongly recommended to stop AWCCService if you plan to use `alienfx-gui` application with "Power Button"-related features. Keep it working can provide unexpected results up to light system freeze (for APIv4).
- **WARNING!** There are well-known bug in DirectX at the Hybrid graphics (Intel+Nvidia) notebooks, which can prevent `Ambient` effect from capture screen. If you have only one screen (notebook panel) connected, but set Nvidia as a "Preferred GPU" in Nvidia panel, please add `alienfx-ambient` with "integrated GPU" setting at "Program settings" into the same panel. It will not work at default setting in this case.
- **WARNING!** In rare case light system freeze, shutdown or hibernate you notebook (some lights can stay on after shutdown), disconnect power adapter and wait about 15 seconds (or until all lights turn off), then start it back.
- **WARNING!** Fan control at Windows 11 can provide system crash at start! Add all .dll and .sys files at antivirus exceptions! It's also reccomended to disable "Memory Integrity" into "Settings/Privacy & Security/Windows Security/Device Security"!

## Support

Join Discord [support server](https://discord.gg/XU6UJbN9J5)

## How to build from source code

Pre-requisites:
- Visual Studio Community 2019
- Misrosoft DDK v10.0 or higher

Build process:
- Clone repository
- Open solution into you Visual Studio, build.

## ToDo list

- [ ] New events
  - [x] Input locale
  - [ ] Missed notifications (toasts)
  - [ ] Keyboard events (f.e pressed ALT, CTRL, SHIFT, Fn will change scheme)
- [ ] New devices support
  - [ ] Monitors
  - [x] Mouses
  - [ ] Keyboards
- [ ] Keyboard mapper for easy RGB keyboard setup
- [ ] Full-scale power button effects (same as common effects)
- [x] Windows 11 support

## Tools Used
* Visual Studio Community 2019

## License
MIT. You can use these tools for any non-commercial or commercial use, modify it any way - supposing you provide a link to this page from you product page and mention me as the one of authors.

## Credits
Functionality idea and code, new devices support, haptic and ambient algorithms by T-Troll.  
Low-level SDK based on Gurjot95's [AlienFX_SDK](https://github.com/Gurjot95/AlienFX-SDK).  
High-level API code and cli app is based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).  
Spectrum Analyzer is based on Tnbuig's [Spectrum-Analyzer-15.6.11](https://github.com/tnbuig/Spectrum-Analyzer-15.6.11).  
FFT subroutine utilizes [Kiss FFT](https://sourceforge.net/projects/kissfft/) library.  
DXGi Screen capture based on Bryal's [DXGCap](https://github.com/bryal/DXGCap) example.  
Per-Key RGB devices testing and a lot of support by [rirozizo](https://github.com/rirozizo).  
Aurora R7 testing by [Raoul Duke](https://github.com/raould).  
Special thanks to [PhSMu](https://github.com/PhSMu) for ideas, testing and artwork.
