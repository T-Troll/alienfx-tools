# alienfx-tools
A bunch of tools for Alienware LighFX controls:
- alienfx-cli - Make changes and check status of your AlienFX lights from the command line.
- AlienFX Universal haptics BETA - Visualize any sound around you (microphone, audio player, game, movie).
- (In development) AlienFX ambient lights - Visualize screen picture as ambient light (from desktop, game, video player)
<br>More will follow!

## Requirements
- Alienware LightFX DLLs installed on your computer. These are automatically installed with Alienware Command Center and 
should be picked up by this program.
- Windows 8+ (binary files for x64 only, but you can compile project for x86 as well).

Device checked: `Alienware m15R1`, `Alienware M13R2` (should work with any Alienware device with API 1.0 or later)

## Known issues
On some devices, some functions can work uncorrectly: can't retrieve positions and colors, can't set zone to action.
This may fixed in upcoming AWCC updates.

## Installation
Unzip the installation archive to any directory of your choise, run.<br>
<s>`alienfx-haptics` uses default audio `input` device as audio source, so you possible need to map you audio `output` to virtual input (or use Microphone/Line-In devices instead).
<br>For Realtek-based hardware, you can do it using `Stereo Mixer` audio device (look in `Control Panel - Sound - Recording`)
<br>For other hardware, virtual audio cable software (f.e. [VB-Audio VirtualCable](https://www.vb-audio.com/Cable/)) required to do this.</s>
<br>`alienfx-haptics` uses default output device (there are all sound from audio players, video players, games played) as an input, but you can switch it to default input device (f.e. microphone or line-in).

## alienfx-cli Usage
Run `alienfx-cli.exe` with a command and any options for that command. 
```
alienfx-cli.exe <command> <command options>
```
The following commands are available:
- `status` Showing AlienFX devices and their lights IDs and status
- `set-all <r> <g> <b> [br]` Sets all AlienFX lights to the specified color. Ex: `set-all 255 0 0` for red lights, `set-all 255 0 0 128` for dimmed red.
- `set-one dev-id light-id r g b [br]` Set one light to color provided. Check light IDs using `status` command first. Ex: `set-dev 0 1 0 0 255` - set light #2 at the device #1 to blue color.
- `set-zone zone r g b [br]` Set zone light to color provided.
- `set-action action dev light r g b [br r g b br]` Set light to color provided and enable action.
- `set-zone-action action zone r g b [br r g b br]` Set zone light to color provided and enable action.
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

## Tools Used
* Visual Studio Community 2019

## License
MIT

## Credits
Integration code and haptic algorythms- T-Troll<br>
API code and cli app is based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).<br>
AlienFX interaction code is from HunterZ's [UniLight](https://github.com/HunterZ/UniLight).<br>
Spectrum Analyzer code is based on Tnbuig's [Spectrum-Analyzer-15.6.11](https://github.com/tnbuig/Spectrum-Analyzer-15.6.11).<br>
FFT subroutine utilizes [Kiss FFT](https://sourceforge.net/projects/kissfft/) library.