# alienfx-cli
Make changes to your AlienFX lights from the command line.

## Requirements
- Alienware AlienFX DLLs installed on your computer. These are automatically installed with Alienware Command Center and 
should be picked up by this program.

## Installation
Unzip the installation archive to any directory of your choosing.

## Usage
Run `alienfx-cli.exe` with a command and any options for that command. 
```
alienfx-cli.exe <command> <command options>
```
The following commands are available:

- `set-color-all <r> <g> <b>` Sets all AlienFX lights to the specified color. Ex: `set-color-all 255 0 0` for red lights.

## Tools Used
* Visual Studio Community 2017

## License
MIT

## Credits
AlienFX interaction code is from HunterZ's [UniLight](https://github.com/HunterZ/UniLight).
