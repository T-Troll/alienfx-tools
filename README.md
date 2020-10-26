# Alienfx tools
A bunch of tools for Alienware AlienFX/Dell LightFX controls:
- alienfx-probe - Looking up for low-level devices, shows it's IDs, then check it and define device and light names.
- alienfx-cli - Make changes and check status of your AlienFX/LightFX lights from the command line.
- AlienFX Universal haptics BETA - Visualize any sound around you (microphone, audio player, game, movie).
- AlienFX Ambient lights BETA - Visualize screen picture as ambient light (from desktop, game, video player).
- AlienFX GUI BETA - Lightweighted light control tool for persons who looking AWCC alternative. It only control lights, but can do a way more tricks then AWCC about it.
<br>More will follow!

## Requirements
- Alienware light device present into the system and have USBHID driver active.
- (Optional) For `alienfx-cli` and `alienfx-probe` high-level support, Alienware LightFX DLLs should be installed on your computer. These are automatically installed with Alienware Command Center and should be picked up by this program. You also should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu then "Allow 3rd-party applications" in older Desktop version 
- Windows 10 (binary files for x64 only, but you can compile project for x86 as well).

Device checked: `Alienware m15R1`, `Alienware m17R1`, `Alienware M13R2`, `Dell G5` (should work with any Alienware device with API 1.0 or later)

## Known issues
- On some devices, some functions from high-level SDK can works incorrectly: can't retrieve positions and colors, can't set zone to action. This may fixed in upcoming AWCC updates.
- `alienfx-cli set-tempo` command doesn't work with high-level SDK (bug in SDK, low-level only).<br>
- `alienfx-cli` `set-zone` and `set-zone-action` commands not supported with low-level SDK (no zones defined).<br>
- Only one device per time can be controlled trough low-level SDK, but you can choose which one.
- Brightness is not supported for low-level API, just ignored now.
- Hardware light effects (<s>pulse</s>(fixed in 0.9.3), morph) doesn't supported for older devices.
- Hardware light effects can't work with software light effects at the same time (hardware bug, Update command stop all effects).
- Some colors (f.e. 128,64,64) presented to wrong color on light device. Looks like a bug in hardware color mix system. I'm working on investigation and software fix.
- DirectX12 games didn't allow to access GPU or frame, so `alienfx-ambient` didn't work, and `alienfx-gui` can't handle GPU load for it correctly.
- <b>WARNING!</b> In case you run `alienfx-gui`, `alienfx-haptics` or `alienfx-ambient` for a long time (1 hour+) and have AWCC installed and running, you can meet significant system slowdown, die to `WMI Host Process` high CPU usage. It's a bug into `AWCCService` AWCC component, producing excessive calls "Throttling Idle Tasks" to WMI. Quick fix: Stop AWCCService if you plan to use gui, haptics or ambient for a long time. I'll contact Dell about this issue, as well as look for workaround in my code.
- <b>DO NOT</b> use alienfx-gui with hardware power button setup and monitroing with other app switching light colors - it can provide unexpected results (see below)! But you can use any of my apps, they have a check for this situation, so it's safe.
- <b>WARNING!</b> Using hardware power button, especially for events, can provide hardware light system freeze in rare situations! If lights are freezed, shutdown you notebook (some lights can stay on after shutdown), disconnect power adapter and wait about 15 sec (or then lights come off), then start it back.

## Installation
Download latest release archive from [here](https://github.com/T-Troll/alienfx-tools/releases).<br>
Unzip the installation archive to any directory of your choise, run.<br>
After install, run `alienfx-probe` or `alienfx-gui` to check and set light names (`alienfx-ambient` and `alienfx-haptics` will not work correctly wihout this operation).</br> 

## alienfx-probe Usage
`alienfx-probe.exe` is a probe for light IDs of the low-level DSK, and it assign names for them (similar to alienfx-led-tester, but wider device support) as well.<br> 
Then run, it shows some info, then switch lights to green one-by-one and prompt to enter devices and lights name (you can enter name or ID from high-level SDK - it's also shown as a part of info). Then the name is set, light switched to blue. If you didn't see which light is changed, just press ENTER to skip it.<br>
It's check 16 first lights into the system by default, but you can change this value runnning `alienfx-probe.exe [number of lights]`.<br>
The purpose of this app is to check low-level API and to prepare light names for other apps, this names are stored and will be used in `alienfx-haptics` and `alienfx-ambient` as a light names for UI.

## alienfx-cli Usage
Run `alienfx-cli.exe` with a command and any options for that command. `alienfx-cli` uses low-level (USB driver) access by default, but you can switch to high-level SDK (Alienware LightFX) issuing `high-level` command. 
```
alienfx-cli.exe [command=option,option,option] ... [command=option,option,option] [loop]
```
The following commands are available:
- `status` Showing AlienFX device IDs and their lights IDs and status. Output is different for low- and high- level SDKs.
- `set-all=r,g,b[,br]` Sets all AlienFX lights to the specified color. Ex: `set-all=255,0,0` for red lights, `set-all=255,0,0,128` for dimmed red. NB: For low-level, it requires lights setup using `alienfx-probe`/-gui to work correctly!
- `set-one=<dev-id>,<light-id>,r,g,b[,br]` Set one light to color provided. Check light IDs using `status` command first. Ex: `set-dev=0,1,0,0,255` - set light #2 at the device #1 to blue color. Dev-id is ignored for low-level SDK.
- `set-zone=<zone>,r,g,b[,br]` Set zone light to color provided. This command only works with high-level API.
- `set-action=<action>,<dev-id>,<light-id>,r,g,b[,br,<action2>,r,g,b[,br]]` Set light to color provided and enable action. Dev-id is ignored for low-level SDK.
- `set-zone-action=<action>,<zone>,r,g,b[,br,r,g,b[,br]]` Set zone light to color provided and enable action. This command only works with high-level API.
- `set-power=<light-id>,r,g,b,r,g,b` Set light as a hardware power button. First color for AC, 2nd for battery power. This command only works with low-level API.
- `set-tempo=<tempo>` Set next action tempo (in milliseconds).
- `low-level` Next commands pass trough low-level API (USB driver) instead of high-level.
- `high-level` Next commands pass trough high-level API (Alienware LightFX), if it's avaliable.
- `loop` Special command to continue all command query endlessly, until user interrupt it. It's provide possibility to keep colors even if awcc reset it. Should be last command in chain.
<br>Supported Zones: `left, right, top, bottom, front, rear`
<br>Supported Actions: `pulse, morph (you need 2 colors for morth), color (disable action)`

## alienfx-haptics Usage
Run `alienfx-haptics.exe`. At first launch, choose `Parameters-Settings` from top menu for setting up light mappings and colors.
<br>Minimize, then start player or game of choice.
```
How it works
```
This application get audio stream from default input device, then made a real-time spectrum analysis.
<br>After that, spectrum powers gropped into 20 groups using octave scale.
<br>For each light found into the system, you can define group(s) it should react, as well as color for Zero (low) and Max (High) power.
<br>It's also possible to compress diapasone if group always not so or so high powered - use low-cut as a bottom range and hi-cut as a top one (values are 0-255).<br>
<s>Global parameter "Decay" defines how fast detected Peak Level decayed if not peak detected or increased if detected peak is higher (2xDecay). Keep it at default 10000 if you audio is 16-bit, alter to about 1000000 in case of 24-bit, set 0 to disable decay, or experiment with you own values.</s> Removed in 0.9.1

## alienfx-ambient Usage
Run `alienfx-ambient.exe`. At first launch, set screen zones mapping to lights and parameters.
<br>Minimize, then start video player or game of choice.
```
How it works
```
This application get shot of screen (privary or secondary), then divide it to several zones.
<br>For each zone, dominant color calculated (you can see it at the button in app interface).
<br>For each light found into the system, you can define zone(s) it should follow. If more, then one zone selected for light, it will try to blend zone colors into one.
<br>You can also select which screen to grab - primary or secondary, if you have more, then one. 
<br>"Divider" parameter defines how many pixels in the row will be skipped - working with full-screen image sometimes very slow. Increasing this value increase update performance, but decrease dominant color extraction presision. Default value is 8, ok for 4k screen with i7 CPU, you can increase it if update lights wit a delay, or decrease if it works ok for you.
<br>"Brightness correction" slider removes some white component from color, made them not so close to white at high brighness and more vivid or darker. Leftmost position disable the correction, rightmost cut 50% white.

## alienfx-gui Usage
Run `alienfx-gui.exe`. Select light, set it colors and patterns - it will set up immedately.<br>
First, use "Devices and Lights" tab to configure out devices and lights settings, if you don't run `alienfx-probe` yet.
Use "Color" tab for simple lights setup (this colors and modes will stay as default until AWCC run or modified by other app), even after reboot.</br>
You can also assign event for light to react on (power state, performance indicator, or just activity light), as well as a color for reaction at "Monitoring" tab.<br>
If "Use color settings as default" is active, first color from "Color" tab will be used for "calm" situation, and the second color from "Monitoring" tab will be uset for "active" situation, if it's not active - both colors will taken from "Monitoring" tab (and colors from "Color" tab if monitoring is disabled).<br>
You can mix different monitoring type at once, f.e. different colors for same light for both CPU load and "system overheat". In this case Status color always override Performance one, as well as both override Power one.<br>
Tray menu (right-click on tray button) avaliable all the time for some fast switch functins, application hide to tray completely then minimized.<br>
```
How it works
```
"Color" tab is set hardware color mode for light. This mode will remain even if you exit application.<br>
"Monitoring" tab designed for system events monitoring and change lights to reflect it - like power events, system load, temperatures.<br>
"Devices and lights" tab is an extended GUI for `alienfx-probe`, providing device and lights control, names modification, light testing and some other hardware-related settings. NB: If you want to add new light, type light ID into LightID box. If this ID already present in list, it will be overrided to first unused ID. Don't try to enter light name at this stage, it's always set to default for easy recognition, change it later for desired one.<br>
"Settings" tab is for application/global lights settings control - states, behaviour, dimming, as well as application settings.<br>
Keyboard shortcuts (any time):
- CTRL+SHIFT+F12 - enable/disable lights
- CTRL+SHIFT+F11 - dim/undim lights
- F18 (on Alienware keyboards it's mapped to Fn+AlienFX) - cycle light mode (on-dim-off)<br>
<br>Other shortcuts (only then application active):
- ALT+c - switch to Color tab
- ALT+m - switch to Monitoring tab
- ALT+d - switch to Device and Lights tab
- ALT+s - switch to Settings tab
- ALT+r - refresh all lights
- ALT+? - about app<br><br>
Monitoring events avaliable:<br>
System Load:
- CPU Load - CPU load color mix from 0% ("calm") to 100% ("Active")
- RAM Load - The same for used RAM percentage
- GPU Load - The same for utilized GPU percentage (top one across GPUs if more, then one present into the system).
- HDD Load - It's not exactly a load, but IDLE time. If idle - it's "calm", 100% busy - active, and mix between.
- Network load - Current network traffic value against maximal value detected (across all network adapters into the system).
- Max. Temperature - Maximal temperature in Celsius degrees (0=calm-100=active) across all temperature sensors detected into the system.
- Battery level - Battery charge level in percent (0=dischagred, 100=full).<br>
You can use "Minimal value" slider to define zone of no reaction - for example, for temperature it's nice to set it to the room temperature - only heat above it will change color.<br>
Status Led:
- Disk activity - Switch light every disk activity event (HDD IDLE above zero).
- Network activity - Switch light if any network traffic detected (across all adapters).
- System overheat - Switch light if sustem temperature above cut level (default 95C, but you can change it using slider below).
- Out of memory - Switch light if memory usage above 90% (you can change it by the same slider).<br>
"Blink" checkbox switch triggered value to blink between on-off colors 5 times per sec (well... about 5 times).
<br><br><b>WARNING:</b> Morph mode doens't works for old devices. Pulse and Morph effects doesn't work if you use any Performance or Activity events monitoring.

## Tools Used
* Visual Studio Community 2019

## License
MIT. You can use this tools for any non-commerical or commercial use, modify it any way - supposing you provide a link to this page from you product page, mention me as one of authors and full source code of you product avaliable for review.

## Credits
Functionality idea and code, new devices support, haptic and ambient algorythms by T-Troll<br>
Low-level SDK based on Gurjot95's [AlienFX_SDK](https://github.com/Gurjot95/AlienFX-SDK)<br>
API code and cli app is based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).<br>
Spectrum Analyzer UI is based on Tnbuig's [Spectrum-Analyzer-15.6.11](https://github.com/tnbuig/Spectrum-Analyzer-15.6.11).<br>
FFT subroutine utilizes [Kiss FFT](https://sourceforge.net/projects/kissfft/) library.<br>
DX Screen capture based on Daramkun's [DaramCam](https://github.com/daramkun/DaramCam) library.<br>
Dominant light extraction math usues [OpenCV](https://github.com/opencv/opencv) library.<br>
Special thanks to [PhSHu](https://github.com/PhSMu) for ideas, testing and arwork.
