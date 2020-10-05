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
- Hardware light effects (pulse, morph) doesn't supported for older devices and can't work with hardware light effects at the same time (hardware limitation).
- DirectX12 games didn't allow to access GPU or frame, so `alienfx-ambient` didn't work, and `alienfx-gui` can't monitor GPU load for it.
- <b>WARNING!</b> In case you run `alienfx-haptics` or `alienfx-ambient` for a long time (1 hour+) and have AWCC installed and running, you can meet significant system slowdown, die to `WMI Host Process` high CPU usage. It's a bug into `AWCCService` AWCC component, producing excessive calls "Throttling Idle Tasks" to WMI. Quick fix: Stop AWCCService if you plan to use haptics or ambient for a long time. I'll contact Dell about this issue, as well as look for workaround in my code.

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
Run `alienfx-cli.exe` with a command and any options for that command. Bu default, `alienfx-cli` using high-level SDK (Alienware LightFX) if found, and low-level (USB driver) if not. You can switch it by command. 
```
alienfx-cli.exe [command=option,option,option] ... [command=option,option,option] [loop]
```
The following commands are available:
- `status` Showing AlienFX device IDs and their lights IDs and status. Output is different for low- and high- level SDKs.
- `set-all=r,g,b[,br]` Sets all AlienFX lights to the specified color. Ex: `set-all=255,0,0` for red lights, `set-all=255,0,0,128` for dimmed red.
- `set-one=<dev-id>,<light-id>,r,g,b[,br]` Set one light to color provided. Check light IDs using `status` command first. Ex: `set-dev=0,1,0,0,255` - set light #2 at the device #1 to blue color.
- `set-zone=<zone>,r,g,b[,br]` Set zone light to color provided.
- `set-action=<action>,<dev-id>,<light-id>,r,g,b[,br,<action2>,r,g,b[,br]]` Set light to color provided and enable action.
- `set-zone-action=<action>,<zone>,r,g,b[,br,r,g,b[,br]]` Set zone light to color provided and enable action.
- `set-tempo=<tempo>` Set next action tempo (in milliseconds).
- `low-level` Next commands pass trough low-level API (USB driver) instead of high-level.
- `high-level` - Next commands pass trough high-level API (Alienware LightFX), if it's avaliable.
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
Global parameter "Decay" defines how fast detected Peak Level decayed if not peak detected or increased if detected peak is higher (2xDecay). Keep it at default 10000 if you audio is 16-bit, alter to about 1000000 in case of 24-bit, set 0 to disable decay, or experiment with you own values.

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
First, use "Setup" tab to configure out devices, lights and system settings (once), if you don't run `alienfx-probe` yet.
Use "Color" tab for simple lights setup (this colors and modes will stay as default until AWCC run or modified by other app), even after reboot.</br>
You can also assign event for light to react on (power state, performance indicator, or just activity light), as well as a color for reaction at "Events" tab. In this case, first color from "Color" tab will be used for "calm" situation, and the color from "Events" tab will be uset for "active" situation.<br>
If the app minimized, it hide itsef at the tray, check thay menu (right-click on tray button) for some cool features as well.<br>
WARNING: Pulse and Morph modes doens't works for old devices. Pulse and Morph effects doesn't work if you use any Performance or Activily lights.

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
