# alienfx-tools
Make changes and check status to your AlienFX lights from the command line.
(In development) Foobar2000 AlienFX haptic plugin
(In development) PotPlayer AlienFX ambient lights
More will follow!

## Requirements
- Alienware AlienFX DLLs installed on your computer. These are automatically installed with Alienware Command Center and 
should be picked up by this program.

Device checked: `Alienware m15R1`, `Alienware M13R2` (should work with any Alienware device with API 2.0+)

## Installation
Unzip the installation archive to any directory of your choosing.

## Usage
Run `alienfx-cli.exe` with a command and any options for that command. 
```
alienfx-cli.exe <command> <command options>
```
The following commands are available:
- `status` Showing AlienFX devices and their lights id and status
- `set-color-all <r> <g> <b> [br]` Sets all AlienFX lights to the specified color. Ex: `set-color-all 255 0 0` for red lights.
- `set-color dev-id light-id r g b [br]` Set one light to color provided. Check light IDs using `status` command first.

## Tools Used
* Visual Studio Community 2019

## License
MIT

## Credits
API code and cli app is based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).
AlienFX interaction code is from HunterZ's [UniLight](https://github.com/HunterZ/UniLight).
