# Alienfx tools
Main goal of this project is to create a bunch of lightweighted tools for Alienware AlienFX/Dell LightFX controls:
- alienfx-probe - Looking up for low-level devices, shows it's IDs, then check it and define device and light names.
- alienfx-cli - Make changes and check status of your AlienFX/LightFX lights from the command line.
- AlienFX Universal haptics - Visualize any sound around you (microphone, audio player, game, movie).
- AlienFX Ambient lights - Visualize screen picture as ambient light (from desktop, game, video player).
- AlienFX GUI Editor - Lightweighted light control tool (AWCC alternative). It only control lights, but can do a way more tricks then AWCC.

## Requirements
- Alienware light device present into the system and have USBHID driver active.
- `alienfx-ambient` uses DirectX for screen capturing, so you need to download and install it from [here](https://www.microsoft.com/en-us/download/details.aspx?id=35).
- (Optional) For `alienfx-cli` and `alienfx-probe` high-level support, Alienware LightFX DLLs should be installed on your computer. These are automatically installed with Alienware Command Center and should be picked up by this program. You also should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu then "Allow 3rd-party applications" in older Desktop version 
- Windows 10 (binary files for x64 only, but you can compile project for x86 as well).

Device checked: `Alienware m15R1-R4` (API v3), `Alienware m17R1` (API v3), `Alienware M13R2` (API v2), `Dell G5` (API v3), `Alienware M14x` (API v1) (should work with any Alienware device with API v1 or later, per-button light keyboard devices and some external devices like mouses doesn't supported now).

## Known issues
- Per-button light keyboard devices (API v4, 64 bytes command) does not supported (i'm working on it). But you still can control other lights (logo, power).
- External devices (mouse, display) not supported - they have different vendor ID (i'm working on it).
- Some High-level (Dell) SDK functions doesn't work as designed. This may be fixed in upcoming AWCC updates.
- `alienfx-cli` `set-zone` and `set-zone-action` commands not supported with low-level SDK (no zones defined).
- Hardware light effects morph, breathing, spectrum, rainbow doesn't supported for older (v1, v2) devices.
- Hardware light effects can't work with software light effects at the same time (hardware bug, "Update" command stop all effects).
- DirectX12 games didn't allow to access GPU or frame, so `alienfx-ambient` didn't work, and `alienfx-gui` can't handle GPU load for it correctly.
- <b>WARNING!</b> Strongly reccomended to stop AWCCService if you plan to use gui, haptics or ambient application. Keep it working can provide unexpected results, espectially if you handle Power Button in gui app.
- <b>WARNING!</b> Using hardware power button, especially for events, can provide hardware light system freeze in rare situations! If lights are freezed, shutdown or hibernate you notebook (some lights can stay on after shutdown), disconnect power adapter and wait about 15 sec (or until lights turn off), then start it back.
- <b>WARNING!</b> There are well-known bug in DirectX at the Hybrid graphics (Intel+Nvidia) notebooks, which can prevent `alienfx-ambient` from capture screen. If you have only one screen (notebook panel) connected, but set Nvidia as a "Preferred GPU" in Nvidia panel, please add `alienfx-ambient` with "integrated GPU" setting at "Program settings" into the same panel. It will not works at default setting in this case.

## Installation
Download latest release archive from [here](https://github.com/T-Troll/alienfx-tools/releases).<br>
Unpack the archive to any directory of your choise.<br>
After unpack, run `alienfx-probe` or `alienfx-gui` to check and set light names (`alienfx-ambient` and `alienfx-haptics` will not work correctly wihout this operation).</br>
Run any tool you need from this folder!

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
- `set-one=<dev-id>,<light-id>,r,g,b[,br]` Set one light to color provided. Check light IDs using `status` command first. Ex: `set-one=0,1,0,0,255` - set light #2 at the device #1 to blue color. For low-level SDK, current active device will be used if devID=0, otherwise it switch to device with ID provided.
- `set-zone=<zone>,r,g,b[,br]` Set zone (see possible zones list below) light to color provided. This command only works with high-level API.
- `set-action=<dev-id>,<light-id>,<action>,r,g,b[,br[,<action>,r,g,b,br]]` Set light to color provided and enable action. You can define up to 9 actions in this command, but only first 1 or 2 will be used for high-level API and for older devices. For low-level SDK, current active device will be used if devID=0, otherwise it switch to device with ID provided.
- `set-zone-action=<action>,<zone>,r,g,b[,br,r,g,b[,br]]` Set zone light to color provided and enable action. This command only works with high-level API.
- `set-power=<light-id>,r,g,b,r,g,b` Set light as a hardware power button. First color for AC, 2nd for battery power. This command only works with low-level API.
- `set-tempo=<tempo>` Set next action tempo (in milliseconds).
- `set-dev=<devID>` Switch active device to this ID (low-level only).
- `Update` Updates light status (for looped commands or old devices).
- `Reset` Reset current device.
- `low-level` Next commands pass trough low-level API (USB driver) instead of high-level.
- `high-level` Next commands pass trough high-level API (Alienware LightFX), if it's avaliable.
- `loop` Special command to continue all command query endlessly, until user interrupt it. It's provide possibility to keep colors even if awcc reset it. Should be last command in chain.

<br>Supported Zones: `left, right, top, bottom, front, rear`
<br>Supported Actions: `pulse, morph (you need 2 colors for morth), color (disable action)`. For low-level api V3, it also support `breath, spectrum, rainbow`.

## alienfx-haptics Usage
Run `alienfx-haptics.exe`. At first launch, set up light mappings and colors.
<br>Minimize, then start player or game of choice.
```
How it works
```
This application get audio stream from default output or input device (you can select it at the top menu), then made a real-time spectrum analysis.
<br>After that, spectrum powers gropped into 20 groups using octave scale.
<br>For each light found into the system, you can define group(s) it should react, as well as color for Zero (low) and Max (High) power.
<br>It's also possible to compress diapasone if group always not so or so high powered - use low-cut as a bottom range and hi-cut as a top one (values are 0-255).<br>

## alienfx-ambient Usage
Run `alienfx-ambient.exe`. At first launch, set screen zones mapping to lights and parameters.
<br>Minimize, then start video player or game of choice.
```
How it works
```
This application get shot of screen (privary or secondary), then divide it to several zones.
<br>For each zone, dominant color calculated (you can see it at the button in app interface).
<br>For each light found into the system, you can define zone(s) it should follow. If more, then one zone selected for light, it will try to blend zone colors into one.
<br>You can also select which screen to grab - primary or secondary, if you have more, then one. You can also press "Reset Devices and Capture" button to re-initialize screen capturing and lights list.
<br>"Quality" slider defines how many pixels will be skipped at analysis - working with full-screen image is very slow. Increasing this value decrease CPU load, but decrease dominant color extraction presision as well. Default value is 16, ok for i7 CPU, you can decrease it if lights updates are slow, or increase if it works ok for you.
<br>"Dimming" slider decreases the overall lights brigtness - use it for better fit you current monitor brightness.
<br>"Gamma Correction" checkbox enables visual color gamma correction, make them more close to screen one.

## alienfx-gui Usage
Run `alienfx-gui.exe`.<br>
First, use "Devices and Lights" tab to configure out devices and lights settings, if you don't run `alienfx-probe` yet.
Use "Color" tab for simple lights setup (this colors and modes will stay as default until AWCC run or modified by other app), even after reboot.</br>
You can also assign event for light to react on (power state, performance indicator, or just activity light), as well as a color for reaction at "Monitoring" tab.<br>
If "Use color settings as default" is active on "Monitoring" tab, first color from "Color" tab will be used for "calm" situation, and the second color from "Monitoring" tab will be uset for "active" situation, if it's not active - both colors will taken from "Monitoring" tab (and colors from "Color" tab if monitoring is disabled).<br>
You can mix different monitoring type at once, f.e. different colors for same light for both CPU load and "system overheat". In this case Status color always override Performance one then triggered, as well as both override Power one.<br>
Tray menu (right-click on tray button) avaliable all the time for some fast switch functins, application hide to tray completely then minimized.<br>
```
How it works
```
"Color" tab is set hardware color mode for light. This setting will remains even if you exit application.<br>
"Monitoring" tab designed for system events monitoring and change lights to reflect it - like power events, system load, temperatures.<br>
"Devices and lights" tab is an extended GUI for `alienfx-probe`, providing device and lights control, names modification, light testing and some other hardware-related settings. NB: If you want to add new light, type light ID into LightID box. If this ID already present in list, it will be overrided to first unused ID. Don't try to enter light name at this stage, it's always set to default for easy recognition, change it later for desired one.<br>
"Profiles" tab control profile settings, like selecting defalult profile, per-profile monitoring control and automatic switch to this profile then the defined application run.<br>
"Settings" tab is for application/global lights settings control - states, behaviour, dimming, as well as application settings:<br>
- "Turn on lights" - Operate all lights into the system. It will be black if this option disabled (default - on).
- "Turn off lights then screen off" - Fade all lights to black then system screen off (default - off).
- "Off power button too" - Hardware Power button light follows the system state. Power light will be always on if disabled (default - off).
- "Autorefresh" - All lights will be refreshed 6 times per second. It's useful if you have AWCC runinning, but still want to control lights (default - off).
- "Color Gamma correction" - Enables color correction to make them looks close to screen one. It keep original AWCC colors if disabled (default - on).
- "Dim lights" - Dim system lights brightness. It's useful for night/battery scenario (default - off).
- "Dim Power button" - Power button follows system dim state. Power button will always have full brightness if disabled (default - off).
- "Dim lights on battery" - Automatically dim lights if system runing at battery power, decreasing energy usage (default - on).
- "Dimming power" - Amount of the brightness decrease then dimmed. Values can be from 0 to 255, default is 92.
- "Start with Windows" - Start application at Windows start. It will not work if application request run as admin level (see below) (default - off).
- "Start minimized" - Hide application window in system tray after start.
- "Enable monitoring" - Application start to monitor system metrics (CPU/GPU/RAM load, etc) and refresh lights according to it (default - on).
- "Profile autoswitch" - Switch between prfiles avaliable automatically, according of applications start and finish. This also block manual profile selection (default - off).
- "Disable AWCC" - Application will check active Alienware Control Center service at the each start and will try to stop it (and start back upon exit). It will require "Run as administrator" priviledge (default - off).
- "Esif temperature" - Read hardware device temperature counters. If disabled, only system-wide ones will be read. It's useful for some Dell (not Alienware) systems. but also provide a lot of component temperature readings. It will require "Run as administrator" priviledge (default - off).

Keyboard shortcuts (any time):
- CTRL+SHIFT+F12 - enable/disable lights
- CTRL+SHIFT+F11 - dim/undim lights
- F18 (on Alienware keyboards it's mapped to Fn+AlienFX) - cycle light mode (on-dim-off)<br>

Other shortcuts (only then application active):
- ALT+c - switch to "Colors" tab
- ALT+m - switch to "Monitoring" tab
- ALT+d - switch to "Devices and Lights" tab
- ALT+p - switch to "Profiles" tab
- ALT+s - switch to "Settings" tab
- ALT+r - refresh all lights
- ALT+? - about app

Monitoring events avaliable:<br>
System Load:
- CPU Load - CPU load color mix from 0% ("calm") to 100% ("Active")
- RAM Load - The same for used RAM percentage
- GPU Load - The same for utilized GPU percentage (top one across GPUs if more, then one present into the system).
- HDD Load - It's not exactly a load, but IDLE time. If idle - it's "calm", 100% busy - active, and mix between.
- Network load - Current network traffic value against maximal value detected (across all network adapters into the system).
- Max. Temperature - Maximal temperature in Celsius degree across all temperature sensors detected into the system.
- Battery level - Battery charge level in percent (0=dischagred, 100=full).
<br>You can use "Minimal value" slider to define zone of no reaction - for example, for temperature it's nice to set it to the room temperature - only heat above it will change color.

Status Led:
- Disk activity - Switch light every disk activity event (HDD IDLE above zero).
- Network activity - Switch light if any network traffic detected (across all adapters).
- System overheat - Switch light if system temperature above cut level (default 95C, but you can change it using slider below).
- Out of memory - Switch light if memory usage above 90% (you can change it by the same slider).
<br>"Blink" checkbox switch triggered value to blink between on-off colors 4 times per sec.

<br><b>WARNING:</b> All color effects stop working if you enable any Event monitoring.

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
DXGi Screen capture based on Bryal's [DXGCap](https://github.com/bryal/DXGCap) example.<br>
Dominant light extraction math usues [OpenCV](https://github.com/opencv/opencv) library.<br>
Special thanks to [PhSHu](https://github.com/PhSMu) for ideas, testing and arwork.
