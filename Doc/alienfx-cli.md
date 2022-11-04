# alienfx-cli 

`alienfx-cli` is a command-line interface to control AliennFX/LightFX devices.

## Usage

Run `alienfx-cli.exe` with a command and any options for that command. `alienfx-cli` uses low-level (USB driver) access by default, but you can switch to high-level SDK (Alienware LightFX) issuing `high-level` command. 
```
alienfx-cli.exe [command=option,option,option] ... [command=option,option,option] [loop]
```
The following commands are available:
- `status` Showing AlienFX device IDs and their lights IDs and status. Output is different for low- and high- level SDKs.
- `set-all=r,g,b[,br]` Sets all AlienFX lights to the specified color. Ex: `set-all=255,0,0` for red lights, `set-all=255,0,0,128` for dimmed red. NB: For low-level, it requires lights setup using `alienfx-probe`/-gui to work correctly!
- `set-one=<dev>,<light>,r,g,b[,br]` Set one light to color provided. Ex: `set-one=0,1,0,0,255` - set light #2 at the device #1 to blue color.
- `set-zone=<zone>,r,g,b[,br]` Set zone (see possible zones list below) light to color provided.
- `set-action=<dev>,<light>,<action>,r,g,b[,br[,<action>,r,g,b,br]]` Set light to color provided and enable action. You can define up to 9 actions in this command, but only first 1 or 2 will be used for high-level API and for older devices.
- `set-zone-action=<action>,<zone>,r,g,b[,br,r,g,b[,br]]` Set zone light to color provided and enable action.
- `set-power=<dev>,<light>,r,g,b,r,g,b` Set light as a hardware power button. First color for AC, 2nd for battery power. This command only works with low-level API.
- `set-tempo=<tempo>` Set next action tempo (in milliseconds).
- `set-global=<dev>,<type>,<mode>,r,g,b,r2,g2,b2` Set global effect mode (v5 and v8 devices)
- `set-dim=[<dev>,]brightness` Set device (or all devices) hardware brightness (dimming) level (from 0 to 255, low-level and API v4-v5 only).
- `low-level` Next commands pass trough low-level API (USB driver) instead of high-level.
- `high-level` Next commands pass trough high-level API (Alienware LightFX), if it's available.
- `loop` Special command to continue all command query endlessly, until user interrupt it. It's provide possibility to keep colors even if awcc reset it. Should be last command in chain.
- `probe[=-a|=<lights>]` Probe light devices present into the system and set devices and light names.

Supported Zones: `left, right, top, bottom, front, rear` for high-level, any group ID (see in `status`) for low-level. 
Supported Actions: `pulse, morph (you need 2 colors for morph), color (disable action)`. For APIv4 and APIv9 devices, `breath, spectrum, rainbow` also supported. APIv5 not supported.  
Supported global effect types:  
For APIv5 - 0 - Color(off), 2 - Breathing, 3 - Single-color Wave, 4 - Dual-color Wave, 8 - Pulse, 9 -Mixed Pulse, 10 - Night Rider, 11 - Laser.  
For APIv9 - 0 - Black(Off), 1 - Morph, 2- Pulse, 3 - Back morph, 7 - Breath, 8 - Rainbow, 15 - Wave, 16 - Rainbow wave, 17 - Circle wave, 19 - Reset to firmware default.  
Supported global effect modes (APIv9 only): 1 - Permanent effect, 2 - On key press only.


For `probe` command, `a` for show additional device info, `l` for define number of lights, `d` for deviceID and (optionally) lightid. Can be combined or absent.  
By default, first 23 lights (or 136 for keyboard devices) for all devices will be checked.

You can check possible "dev" and "light" ID values using `status` command. For "dev", values out of range is ignored. for "light", you can use any value between 0 and 255.  
`set-all`, `lightson`, `lightsoff` commands alter all lights for all detected devices.
