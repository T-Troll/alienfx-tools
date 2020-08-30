# alienfx-tools
A bunch of tools for Alienware LighFX controls:
- alienfx-probe - Looking up for low-level device, shows it's ID and let you define light names.
- alienfx-cli - Make changes and check status of your AlienFX lights from the command line.
- AlienFX Universal haptics BETA - Visualize any sound around you (microphone, audio player, game, movie).
- AlienFX Ambient lights BETA - Visualize screen picture as ambient light (from desktop, game, video player).
- (In development) AlienFX Monitor - Visual feedback for CPU/RAM/HDD/SSD/Network loading.
<br>More will follow!

## Requirements
- Alienware device present into the system and have USBHID driver active.
- (Optional) For `alienfx-cli` and `alienfx-probe` high-level support, Alienware LightFX DLLs should be installed on your computer. These are automatically installed with Alienware Command Center and 
should be picked up by this program.
- (Optional) You should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu then "Allow 3rd-party applications" in older Desktop version 
- Windows 8+ (binary files for x64 only, but you can compile project for x86 as well).

Device checked: `Alienware m15R1`, `Alienware M13R2`, `Dell G5` (should work with any Alienware device with API 1.0 or later)

## Known issues
- On some devices, some functions can work incorrectly: can't retrieve positions and colors, can't set zone to action. This may fixed in upcoming AWCC updates.
- `alienfx-cli set-tempo` command doesn't work with high-level SDK (bug in SDK, low-level only).<br>
- `alienfx-cli` commands `set-zone` and `set-zone-action` not supported with low-level SDK (no zones defined).<br>
- `alienfx-cli` command `set-action` donsn't work with low-level SDK (work in progress)
- Only frist device found can be controlled trough low-level SDK - no multi-device support yet.
- Brightness is not supported for low-level API, just ignored now.
- <s>`Alienfx-ambient` stop working and should be restarted after screen off (sleep or timeout).</s> Fixed in 0.6.4.

## Installation
Unzip the installation archive to any directory of your choise, run.<br>
In case apps can't locate LightFX dll's, you can find it into /DLL folder and copy to apps folder one you need (for x32 or x64).</br>
After install, run `alienfx-probe` once to check and set light names (`alienfx-ambient` and `alienfx-haptics` will not works wihout it).</br> 

## alienfx-probe Usage
Run `alienfx-probe.exe`<br>
Application shows some info, then begins switch light to green and prompt to enter light name (you can enter name or ID from SDK - it's show as well), then change light to blue then set. If you didn't see which light is changed, just press ENTER to skip it.<br>
It check 15 first lights into the system.<br>
The purpose of this app is to check low-level API and to prepare to low-level SDK migration, this names are stored and will be used later in `alienfx-haptics` and `alienfx-ambient` as a light names for UI.

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
- `set-action=<action>,<dev-id>,<light-id>,r,g,b[,br,r,g,b[,br]]` Set light to color provided and enable action.
- `set-zone-action=<action>,<zone>,r,g,b[,br,r,g,b[,br]]` Set zone light to color provided and enable action.
- `set-tempo=<tempo>` Set next action tempo (in milliseconds).
- `low-level` Next commands pass trough low-level API (USB driver) instead of high-level.
- `high-level` - Next commands pass trough high-level API (Alienware LightFX).
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
<br><s>WARNING! Changing screen requires application restart to apply!</s> Fixed in 0.6.4.
<br>"Divider" parameter defines how many pixels in the row will be skipped - working with full-screen image sometimes very slow. Increasing this value increase update performance, but decrease dominant color extraction presision. Default value is 8, ok for 4k screen with i7 CPU, you can increase it if update lights wit a delay, or decrease if it works ok for you.
<br>"Brightness correction" slider removes some white component from color, made them not so close to white at high brighness and more vivid or darker. Leftmost position disable the correction, rightmost cut 50% white.

## Tools Used
* Visual Studio Community 2019

## License
MIT

## Credits
Integration code and haptic algorythms by T-Troll<br>
API code and cli app is based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).<br>
AlienFX interaction code is from HunterZ's [UniLight](https://github.com/HunterZ/UniLight).<br>
Spectrum Analyzer code is based on Tnbuig's [Spectrum-Analyzer-15.6.11](https://github.com/tnbuig/Spectrum-Analyzer-15.6.11).<br>
FFT subroutine utilizes [Kiss FFT](https://sourceforge.net/projects/kissfft/) library.<br>
DX Screen capture based on Daramkun's [DaramCam](https://github.com/daramkun/DaramCam) library.<br>
Dominant light extraction usues [OpenCV](https://github.com/opencv/opencv) library.
