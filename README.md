# alienfx-tools
Make changes and check status to your AlienFX lights from the command line.
<br>(In development) Foobar2000 AlienFX haptic plugin
<br>(In development) PotPlayer AlienFX ambient lights
<br>More will follow!

## Requirements
- Alienware AlienFX DLLs installed on your computer. These are automatically installed with Alienware Command Center and 
should be picked up by this program.

Device checked: `Alienware m15R1`, `Alienware M13R2` (should work with any Alienware device with API 1.0 or later)

## Known issues
On some devices, some functions can work uncorrectly: can't retrieve positions and colors, can't set zone to action.
This may fixed in upcoming AWCC updates.

## Installation
Unzip the installation archive to any directory of your choosing.

## Usage
Run `alienfx-cli.exe` with a command and any options for that command. 
```
alienfx-cli.exe <command> <command options>
```
The following commands are available:
- `status` Showing AlienFX devices and their lights IDs and status
- `set-all <r> <g> <b> [br]` Sets all AlienFX lights to the specified color. Ex: `set-all 255 0 0` for red lights, `set-all 255 0 0 128` for dimmed red.
- `set-dev dev-id light-id r g b [br]` Set one light to color provided. Check light IDs using `status` command first. Ex: `set-dev 0 1 0 0 255` - set light #2 at the device #1 to blue color.
- `set-zone zone r g b [br]` Set zone light to color provided.
- `set-action action dev light r g b [br r g b br]` Set light to color provided and enable action.
- `set-zone-action action zone r g b [br r g b br]` Set zone light to color provided and enable action.
<br>Supported Zones: `left, right, top, bottom, front, rear`
<br>Supported Actions: `pulse, morph (you need 2 colors for morth), color (disable action)`

## Tools Used
* Visual Studio Community 2019

## License
MIT

## Credits
API code and cli app is based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).
AlienFX interaction code is from HunterZ's [UniLight](https://github.com/HunterZ/UniLight).
