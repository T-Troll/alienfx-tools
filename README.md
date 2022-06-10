# AlienFx tools

AWCC is not needed anymore - here are light weighted tools for Alienware systems lights, fans, power profile control:
- [AlienFX Control](/Doc/alienfx-gui.md) - AWCC alternative in 500kb. You can control your system lights (including hardware and software effects such as system parameters monitoring, ambient lights, sound haptic), fans, temperatures, power settings, and a lot more.
- [AlienFX Monitor](/Doc/alienfx-mon.md) - System monitoring tool - you can check system health and put important parameters into tray.
- [AlienFX-CLI](/Doc/alienfx-cli.md) - Make changes and check status of your AlienFX lights from the CLI (command-line interface).
- [LightFX](/Doc/LightFX.md) - Dell LightFX library emulator. Supports all Dell's API functions using my low-level SDK. It can be used for any LightFX/AlienFX-compatible game.
- [AlienFan GUI control](/Doc/alienfan-gui.md) - simple fan and power control utility. Set your fan parameters according to any system temperature sensor, switch system power modes, etc.
- [AlienFan-CLI](/Doc/alienfan-cli.md) - Command-line interface tool for control fans (and lights for some systems) as well as some power settings from the command line.

## How does it work?

Light control tools work with USB/ACPI hardware devices directly, not requiring the installation of other tools/drivers.

- It's way faster. For older systems, the change rate can be up to 20cps. For modern, up to 120cps.
- It's flexible. I can use uncommon calls to set a broader range of effects and modes.
- Group lights, create light/fan Profiles for different situations, switch them by running games/applications.

For fan/power controls, instead of many other fan control tools, like `SpeedFan`, `HWINFO` or `Dell Fan Control`, this tool does not use direct EC (Embed controller) access and data modification.  
It utilizes proprietary Alienware function calls inside ACPI BIOS (the same used by AWCC).
- It's safer - BIOS still monitors fans and has no risk fans will stop under full load.
- It's universal - Most Alienware systems have the same interface.
- In some cases, this is the only way - for example, Alienware m15/m17R1 does not have EC control at all.

## Disclaimer

Starting from the release 4.2.1, **Anti-viruses can detect virus into project package**.  
It's not a virus but a kernel hack to load the driver. You should add `HwAcc.sys` and `kdl.dll` into the antivirus exception list or do not use fan control (light control will work without these files).

## Requirements
- Alienware light device/Alienware ACPI BIOS (for fan control) present into the system and have USB HID driver active (`alienfx-cli` can work even with missing devices, Dell LightFX needs to be present in the system).
- Windows 10 v1903 or later (64-bit only).
- If you plan to use fan control, "Memory integrity" (Settings->Update&Security->Security->Device Security->Core isolation) should be disabled.
- `alienfan-gui` and `-cli` always require Administrator rights to work (for communication with hardware).
- `alienfx-gui` require Administrator rights in some cases:
  - "Disable AWCC" selected in Settings (stopping AWCC service require Administrator privileges)
  - "Esif temperature" selected (access to ESIF values blocked from user account)
  - "Enable Fan control" selected (the same reason as for `alienfan-gui`)
- `alienfx-mon` require Administrator rights in case ESIF or BIOS monitoring enabled (the same reason as for `alienfx-gui`)
- `alienfx-cli` does not require Administrator privilege and can be run at any level.
- All the tools don't require an Internet connection, but `alienfan-gui`, `alienfx-mon` and `alienfx-gui` will connect to GitHub to check for updates if a connection is available.
- All the tools does not collect and share any personal data. Some hardware data collected (but not shared) during hardware detection process.

## Installation
- Download the latest release archive or installer package from [here](https://github.com/T-Troll/alienfx-tools/releases).  
- (Optional) `Ambient` effect mode uses DirectX for screen capturing, so you need to download and install it from [here](https://www.microsoft.com/en-us/download/details.aspx?id=35). Other modes don't require it, so you need it if you plan to use `Ambient` effects only.
- (Optional) For LightFX-enabled games/applications, copy `LightFx.dll` into game/application folder.
- (Optional) For `alienfx-cli` high-level support, both of my emulated (see above) or Alienware LightFX DLLs should be installed on your computer. These are installed automatically with Alienware Command Center, and the program should pick them up. You also should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu, then "Allow 3rd-party applications" in older Desktop version. 
- Unpack the archive to any directory of your choice or just run the installer.  
- (Optional) For the fan control, it's highly recommended to set correct overboost values and maximal fans RPM. You can do it at GUI apps first start or by running `alienfx-cli setover` command.
- Run `alienfx-gui` or `alienfx-cli probe` to check and set light names (all tools will have limited functionality without this step).  

Please read [How to start](https://github.com/T-Troll/alienfx-tools/wiki/How-to-start-(Beginner's-guide)-for-release-v6.x.x.x)) guide first!

## Supported hardware:

Light control: Virtually any Alienware/Dell G-series notebook and desktop, some Alienware mouses. Monitor support in progress.  
Fan control: Modern Alienware/Dell G-Series notebooks (any m-series, x-series, Area51m), Aurora R7 desktop and later models, some older notebooks (13R2 and compatible).

Project Wiki has [more details and the list of tested devices](https://github.com/T-Troll/alienfx-tools/wiki/Supported-and-tested-devices-list).  
If your device is not supported, you can [help me support it](https://github.com/T-Troll/alienfx-tools/wiki/How-to-collect-data-for-the-new-light-device).  
For fan control - Send me the ACPI dump from [RW Everything](http://rweverything.com/) for analysis.

## Known issues
- Hardware light effects like breathing, spectrum, rainbow only supported at APIv4 (Tron) lights.
- Hardware light effects and global effect didn't work with software effects at the same time for APIv4-v5 (hardware bug, "Update" command stop all effects). Disable monitoring in `alienfx-gui` to use it.
- DirectX12 games didn't allow access to GPU or frame, so `Ambient` effect will not work, and `alienfx-gui` can't handle GPU load for it correctly.
- Setting a hardware power button light, especially for events, can provide hardware light system acting slow right after color update! `alienfx-gui` will switch to the "Devices" tab or quit with visible delay.
- **WARNING!** I strongly recommend stopping AWCCService if you plan to use `alienfx-gui` application with "Power Button" related features. Keeping it working can provide unexpected results up to light system freeze (for APIv4).
- **WARNING!** There is a well-known bug in DirectX at the Hybrid graphics (Intel+Nvidia) notebooks, preventing the `Ambient` effect from capturing the screen. If you have only one screen (notebook panel) connected but set Nvidia as a "Preferred GPU" in the Nvidia panel, please add `alienfx-gui` with "integrated GPU" setting at "Program settings" for the same monitor. It will not work at the default setting in this case.
- **WARNING!** In rare case light system freeze, shutdown or hibernate your notebook (some lights can stay on after shutdown), disconnect power adapter and wait about 15 seconds (or until all lights turn off), then start it back.

## Support

Join Discord [support server](https://discord.gg/XU6UJbN9J5)

## How to build from source code

Prerequisites:
- Visual Studio Community 2019
- Microsoft Windows Driver Development Kit (WDK) v10.0 or higher

Build process:
- Clone repository
- Open solution into your Visual Studio, build.

## ToDo list

- [ ] New events
  - [x] Input locale
  - [ ] Missed notifications (toasts)
  - [x] Keyboard events (key press reaction)
- [ ] Grid (positional) effects
  - [ ] Static full-grid effects
  - [ ] Dynamic full-grid effects
  - [ ] Event or key-triggered full-grid effects
- [ ] New devices support
  - [ ] Monitors
  - [x] Mouses
  - [ ] Keyboards
- [x] Keyboard mapper for easy RGB keyboard setup
- [x] Overboost GUI
- [x] Windows 11 support

## License

MIT. You can use these tools for any non-commercial or commercial use, modify it any way - supposing you provide a link to this page from your product page and mention me as one of the authors.

## Credits

Functionality idea and code, new devices support, haptic and ambient algorithms by T-Troll.  
Low-level SDK based on Gurjot95's [AlienFX_SDK](https://github.com/Gurjot95/AlienFX-SDK).  
High-level API code and alienfx-cli based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).  
Spectrum Analyzer is based on Tnbuig's [Spectrum-Analyzer-15.6.11](https://github.com/tnbuig/Spectrum-Analyzer-15.6.11).  
FFT subroutine utilizes [Kiss FFT](https://sourceforge.net/projects/kissfft/) library.  
DXGi Screen capture based on Bryal's [DXGCap](https://github.com/bryal/DXGCap) example.  
ACPI driver based on kdshk's [WindowsHwAccess](https://github.com/kdshk/WindowsHwAccess).  
Kernel loading hack based on hfiref0x's [KDU](https://github.com/hfiref0x/KDU)

Special thanks to [DavidLapous](https://github.com/DavidLapous) for inspiration and advice!  
Special thanks to [theotherJohnC](https://github.com/theotherJohnC) for the Performance Boost idea!  
Per-Key RGB devices testing and a lot of support by [rirozizo](https://github.com/rirozizo).  
Aurora R7 testing by [Raoul Duke](https://github.com/raould).  
Support for mouse and a lot of testing by [Billybobertjoe](https://github.com/Billybobertjoe)  
A lot of G-mode ideas and testing from [Hellohi3654](https://github.com/Hellohi3654)  
Special thanks to [PhSMu](https://github.com/PhSMu) for ideas, Dell G-series testing, and artwork.
