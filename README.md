# Alienfx tools
AWCC don't needed anymore - here are light weighted tools for Alienware systems lights,fans,power profile control:
- [AlienFX GUI Editor](/Doc/alienfx-gui.md) - AWCC alternative in 500kb. You can control you system lights, fans, temperatures, power settings and a lot more.
- [AlienFX Universal haptics](/Doc/alienfx-haptics.md) - Visualize any sound around you (microphone, audio player, game, movie).
- [AlienFX Ambient lights](/Doc/alienfx-ambient.md) - Visualize screen picture as ambient light (from desktop, game, video player).
- [alienfx-cli](/Doc/alienfx-cli.md) - Make changes and check status of your AlienFX lights from the CLI (command line interface).
- [LightFX](/Doc/LightFX.md) - Dell LightFX library emulator. Support all Dell's API functions using low-level SDK.
- [alienfx-probe](/Doc/alienfx-probe.md) - CLI application to probe devices and lights and name it for using into other applications.

Some additional tools added from my other [`Alienfan-tools`](https://github.com/T-Troll/alienfan-tools) project:
- Alienfan GUI control - simple fan and power control utility. Set you fan parameters according to any system temperature sensor, switch system power modes...
- Alienfan CLI - Command line interface tool for control fans and power from command line.  
Readme is avaliable [here](https://github.com/T-Troll/alienfan-tools).

## Requirements
- Alienware light device present into the system and have USB HID driver active (`alienfx-cli` can work even if device not found, but Dell LightFX present into system).
- Windows 10 v1803 or later (binary files for 64-bit only, but you can compile project for 32-bit as well).
- `alienfan-gui` and `alienfan-cli` always require Administrator rights to work (for communication with hardware).
- Other apps does not require Administrator rights, except `alienfx-gui` in some cases:
  - "Disable AWCC" selected in Settings (stopping AWCC service require Administrator privilegy)
  - "Esif temperature" selected (access to ESIF values blocked from user account)
  - "Enable Fan control" selected (the same reason as for `alienfan-gui`)

## Installation
- Download latest release archive or installer package from [here](https://github.com/T-Troll/alienfx-tools/releases).  
- (Optional) `alienfx-ambient` uses DirectX for screen capturing, so you need to download and install it from [here](https://www.microsoft.com/en-us/download/details.aspx?id=35). Other tools doesn't require it, so you need it in case if you plan to use Ambient only.
- (Optional) For LightFX-enabled games/applications, copy `LightFx.dll` into game/application folder.
- (Optional) For `alienfx-cli` and `alienfx-probe` high-level support, any of my emulated (see above) or Alienware LightFX DLLs should be installed on your computer. These are automatically installed with Alienware Command Center and should be picked up by this program. You also should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu then "Allow 3rd-party applications" in older Desktop version. 
- (Optional) If you plan to use `alienfx-gui` fan control or any of Aliefan tools, check [alienfan readme](https://github.com/T-Troll/alienfan-tools) for possible configuration alternatives.
- Unpack the archive to any directory of your choise or just run installer.  
- After installation, run `alienfx-gui` or `alienfx-probe` to check and set light names (all apps will have limited to no functionality without this step).  

Run any tool you need from this folder or start menu!

## Devices tested:
- `Alienware Area51m-R2` Per-key keyboard lights (API v4 + API v5)
- `Alienware m15R3-R4` Per-key keyboard lights (API v4 + API v5)
- `Alienware m15R1-R4` 4-zone keyboard ligths (API v4)
- `Alienware m17R1` (API v4) 
- `Dell G7/G5/G5SE` (API v4)
- `Alienware 17R5` (API v3)
- `Alienware 13R2` (API v2)
- `Alienware M14x` (API v1)

This tools can also support other Dell/Alienware devices:
- APIv1 - 8-bytes command, 24-bit color,
- APIv2 - 9-bytes command, 12-bit color,
- APIv3 - 12-bytes command, 24-bit color,
- APIv4 - 34-bytes command, 24-bit color,
- APIv5 - 64-bytes command, 24-bit color (DARFON OEM keyboards).

For fan/power control, any Alienware notebook starting from m15R1/m17R1 or later supported now.

External mouses, keyboards and monitors are not supported yet, feel free to open an issue if you want to add support for it, but be ready to help with testing!

## Known issues
- Some High-level (Dell) SDK functions can not work as designed. This may be fixed in upcoming AWCC updates. It's not an `alienfx-cli` bug.
- Hardware light effects breathing, spectrum, rainbow doesn't supported for older (APIv1-v3) devices.
- Hardware light effects (and global effect) didn't work with software light effects at the same time for APIv4-v5 (hardware bug, "Update" command stop all effects). Disable monitoring in `alienfx-gui` to use it.
- DirectX12 games didn't allow to access GPU or frame, so `alienfx-ambient` will not handle colors, and `alienfx-gui` can't handle GPU load for it correctly.
- Using hardware power button, especially for events, can provide hardware light system acting slow right after color update! `alienfx-gui` will switch to "Devices" tab or quit with visible delay.
- **WARNING!** Strongly recommended to stop AWCCService if you plan to use `alienfx-gui` application with "Power Button"-related features. Keep it working can provide unexpected results up to light system freeze (for APIv4).
- **WARNING!** There are well-known bug in DirectX at the Hybrid graphics (Intel+Nvidia) notebooks, which can prevent `alienfx-ambient` from capture screen. If you have only one screen (notebook panel) connected, but set Nvidia as a "Preferred GPU" in Nvidia panel, please add `alienfx-ambient` with "integrated GPU" setting at "Program settings" into the same panel. It will not work at default setting in this case.
- **WARNING!** In rare case light system freeze, shutdown or hibernate you notebook (some lights can stay on after shutdown), disconnect power adapter and wait about 15 seconds (or until all lights turn off), then start it back.

## How to build from source code

Pre-requisites:
- Visual Studio Community 2019
- Misrosoft DDK v10.0 or higher

Build process:
- Clone repository
- Clone alienfan-tools repository into `alienfan-tools` folder from [here](https://github.com/T-Troll/alienfan-tools)
- Open solution into you Visual Studio, build.

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
Special thanks to [PhSMu](https://github.com/PhSMu) for ideas, testing and artwork.
