# AlienFx tools

AWCC is not needed anymore - here are light weighted tools for Alienware systems lights, fans, power profile control:
- [AlienFX Control](/Doc/alienfx-gui.md) - AWCC alternative in 500kb. You can control your system lights (including hardware and software effects such as system parameters monitoring, ambient lights, sound haptic), fans, temperatures, power settings, and a lot more.
- [AlienFX Monitor](/Doc/alienfx-mon.md) - System monitoring tool - you can check system health and put important parameters into tray.
- [AlienFX-CLI](/Doc/alienfx-cli.md) - Make changes and check status of your AlienFX lights from the CLI (command-line interface).
- [LightFX](/Doc/LightFX.md) - Dell LightFX library emulator. Supports all Dell's API functions using my low-level SDK. It can be used for any LightFX/AlienFX-compatible game.
- [AlienFan GUI control](/Doc/alienfan-gui.md) - simple fan and power control utility. Set your fan parameters according to any system temperature sensor, switch system power modes, etc.
- [AlienFan-CLI](/Doc/alienfan-cli.md) - Command-line interface tool for control fans (and lights for some systems) as well as some power settings from the command line.
- AlienFX-Config - Simple script to backup/restore/delete tools configuration from registry. Run `alienfx-config.cmd` without parameters for usage.

## How does it work?

Light control tools work with USB/ACPI hardware devices directly, not requiring the installation of other tools/drivers.

- It's way faster. For older systems, the change rate can be up to 20cps. For modern, up to 120cps.
- It's flexible. I can use uncommon calls to set a broader range of effects and modes.
- Group lights, create light/fan Profiles for different situations, switch them by running games/applications/power source/keyboard shortcut.

For fan/power control, instead of many other fan control tools, like `SpeedFan`, `HWINFO` or `Dell Fan Control`, this tool does not use direct EC (Embed controller) access and data modification.  
It utilizes proprietary Alienware function calls inside ACPI BIOS (the same used by AWCC).

- It's safer - BIOS still monitors fans and has no risk fans will stop under full load.
- It's universal - Most Alienware/Dell G-series systems have the same interface.
- In some cases, this is the only way - for example, Alienware m15/m17R1 does not have EC control.

## Requirements
- Alienware light device/Alienware ACPI BIOS (for fan control) present into the system and have USB HID driver active (`alienfx-cli` can work even with missing devices, Dell LightFX needs to be present in the system).
- Windows 10 v1903 or later (64-bit only). Windows 11 supported.

## Security and privacy

In case you install tools version between `4.2.1` and `6.4.3.2`, **Anti-viruses can detect virus into project package**.  
It's not a virus, you can read why it happened [here](https://github.com/T-Troll/alienfx-tools/wiki/Why-antivirus-complain-about-some-alienfx-tools-components%3F).  
Please add application folder into anti-virus exception list.

- `alienfan-gui` and `alienfan-cli` always require Administrator rights (for communication with hardware).
- `alienfx-gui` require Administrator rights in some cases (this functions will be disabled in case you chancel UAC request):
  - "Disable AWCC" selected in Settings (stopping AWCC service require Administrator privileges)
  - "Use BIOS sensors" selected (access to ESIF values blocked from user account)
  - "Enable Fan control" selected (the same reason as for `alienfan-gui`)
- `alienfx-mon` require Administrator rights in case BIOS or Alienware monitoring enabled (the same reason as for `alienfx-gui`)
- `alienfx-cli` don't require Administrator privilege and can be run at any level.

- All the tools don't require an Internet connection, but `alienfan-gui`, `alienfx-mon` and `alienfx-gui` will connect to GitHub to check for updates if a connection is available.
- All the tools don't collect or share any personal data. Some hardware data collected (but not shared) during hardware detection process.

## Installation
- Download the latest release archive or installer package from [here](https://github.com/T-Troll/alienfx-tools/releases).
- Unpack the archive to any directory of your choice or just run the installer.
- In case you are using .zip archive and have Intel system, copy files from `DPTF` folder to `<Windows folder>\System32\drivers\DriverData\Intel\DPTF\dv\`
- Run `alienfx-gui` or `alienfx-cli probe` to check and set light names (light control tools will have limited functionality without this step).
- (Optional) `Ambient` effect mode uses DirectX for screen capturing, so you need to download and install it from [here](https://www.microsoft.com/en-us/download/details.aspx?id=35). Other modes don't require it, so you need it if you plan to use `Ambient` effects only.
- (Optional) For LightFX-enabled games/applications, copy `LightFx.dll` into game/application folder or into `%windir%\System32` for use it with all light-enabled applications.
- (Optional) For `alienfx-cli` high-level support, both of my emulated (see above) or Alienware LightFX DLLs should be installed on your computer. These are installed automatically with Alienware Command Center, and the program should pick them up. You also should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu, then "Allow 3rd-party applications" in older Desktop version. 
- (Optional) For the fan control, it's highly recommended to set correct max. boost values. You can do it at any GUI app or by running `alienfx-cli setover` command.
- (Optional) You can install and run `Libre Hardware Monitor` before running fan control app - this provide more sensors to control.

Please read [How to start](https://github.com/T-Troll/alienfx-tools/wiki/How-to-start-(Beginner's-guide)-for-release-v6.x.x.x) guide first!

## Supported hardware:

Light control: Virtually any Alienware/Dell G-series (with Alienware keyboard) notebook and desktop, some Alienware mouses, Alienware keyboards, Alienware monitors.  
Fan control: Modern Alienware/Dell G-Series notebooks (any m-series, x-series, Area51m), Aurora R7 (and later model) desktops.

Project Wiki has [more details and the list of tested devices](https://github.com/T-Troll/alienfx-tools/wiki/Supported-and-tested-devices-list).  
If your light device is not supported, you can [help me support it](https://github.com/T-Troll/alienfx-tools/wiki/How-to-collect-data-for-the-new-light-device).  
In case of issues with fan control, please open issue here or contact me via Discord support server.

## Light system reset

- In rare case light system freeze, shutdown or hibernate your system (some lights can stay on after shutdown), disconnect all cables, including power adapter, and wait about 15 seconds (or until all lights turn off), then start it back.
- In rare case you have Alienware monitor, and `AlienFX-GUI` or `AlienFX-CLI` freeze at start for more, then 6 seconds, disconnect you monitor USB cable or reset monitor settings to default (hardware bug).

## Project Support and support for the project

Join Discord [support server](https://discord.gg/XU6UJbN9J5) in case you have any questions.

Author does not accept any donates, except you can share any Alienware/Dell hardware for testing. Contact me in Discord about it.  
Feel free to promote this project online if you like it instead!

## How to build from source code

Prerequisites:
- Visual Studio Community 2019
- (Optional) Microsoft WDK (Driver kit) should be installed in case you want to build against older V1 fan SDK.

Build process:
- Clone repository
- Open solution from your Visual Studio, build.

## ToDo list

- [ ] Ryzen ACPI sensors/control (AMD)
- [ ] Grid effect shapes (like text, image, etc)
- [ ] Power and battery charge control
- [x] Restore ACPI light API support - beta
- [ ] Automatic overclock/underclock based on desired system temperature and load

## License

MIT. You can use these tools for any non-commercial or commercial use, modify it any way - supposing you provide a link to this page from your product page and mention me as one of the authors.

## Credits

Functionality idea and code, new devices support, haptic and ambient algorithms by T-Troll.  
Low-level SDK based on Gurjot95's [AlienFX_SDK](https://github.com/Gurjot95/AlienFX-SDK).  
High-level API code and alienfx-cli based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).  
Spectrum Analyzer is based on Tnbuig's [Spectrum-Analyzer-15.6.11](https://github.com/tnbuig/Spectrum-Analyzer-15.6.11).  
FFT subroutine utilizes [Kiss FFT](https://sourceforge.net/projects/kissfft/) library.  
DXGi Screen capture based on Bryal's [DXGCap](https://github.com/bryal/DXGCap) example.  

Special thanks to [DavidLapous](https://github.com/DavidLapous) for inspiration and advice!  
Special thanks to [theotherJohnC](https://github.com/theotherJohnC) for the Performance Boost idea!  
Per-Key RGB devices testing and a lot of support by [rirozizo](https://github.com/rirozizo).  
Aurora R7 testing by [Raoul Duke](https://github.com/raould).  
Support for mouse and a lot of testing by [Billybobertjoe](https://github.com/Billybobertjoe)  
A lot of G-mode ideas and testing from [Hellohi3654](https://github.com/Hellohi3654)  
Alienware m15R6 device mapping and testing by [Professor-Plays](https://github.com/profpjlalvarenga)  
Special thanks to [PhSMu](https://github.com/PhSMu) for ideas, Dell G-series testing, and artwork.
