# alienfx-cli 

`alienfx-cli` is a command-line interface to control AlienFX/LightFX devices.  
It can use both low-level (USB driver) access by default, but you can switch to high-level SDK (Alienware LightFX) issuing `high-level` command.

## Usage

Run `alienfx-cli.exe` without parameters for help.

```
alienfx-cli.exe [command=option,option,option] ... [command=option,option,option] [loop]
```

The following commands are available:
- `status` Showing AlienFX device IDs and their lights IDs and status. Output is different for low- and high- level SDKs.
- `setall=r,g,b` Sets all AlienFX lights found into the system to the specified color. Ex: `setall=255,0,0` for red lights. For low-level, it requires lights setup using `alienfx-probe`/-gui to work correctly!
- `setone=<dev>,<light>,r,g,b` Set one light to color provided. Ex: `setone=0,1,0,0` - set light #2 at the device #1 to blue color.
- `setzone=<zone>,r,g,b` Set zone (see possible zones list below) light to color provided.
- `setaction=<dev>,<light>,<action>,r,g,b[,<action>,r,g,b]` Set light to color provided and enable action. You can define up to 9 actions in this command, but only first and last will be used for devices without APIv4.
- `setzoneaction=<zone>,,<action>,r,g,b[,<action>,r,g,b]` Set zone light to color provided and enable action.
- `setpower=<dev>,<light>,r,g,b,r,g,b` Set light as a hardware power button. First color for AC, 2nd for battery power. This command only works with low-level API.
- `settempo=<tempo>[,length]` Set next action tempo (0..255) and phase length (for low-level, 0..255).
- `setglobal=<dev>,<type>,<mode>[,r,g,b[,r2,g2,b2]]` Set global effect mode (v5 and v8 devices)
- `setdim=[<dev>,]brightness` if device provided, set device hardware brightness (dimming) level (from 0 to 255, low-level and API v4-v5 only). If device not provided, set software brightness level for next light operations.
- `lowlevel` Next commands pass trough low-level API (USB driver) instead of high-level.
- `highlevel` Next commands pass trough high-level API (Alienware LightFX), if it's available.
- `loop` Special command to continue all command query endlessly, until user interrupt it. It's provide possibility to keep colors even if awcc reset it. Should be last command in chain.
- `probe[l][d][,lights][,devID[,lightID]]` Probe light devices present into the system and set devices and light names.

Supported Zones: `left, right, top, bottom, front, rear` for high level, any zone ID (see list in `status`) for low level.

Supported Actions: `pulse, morph (you need 2 colors for morph), color (disable action)`. For APIv4 and APIv8 devices, `breath, spectrum, rainbow` also supported. APIv5 not supported.

Supported global effect types:  
For APIv5 - 0 - Color(off), 2 - Breathing, 3 - Single-color Wave, 4 - Dual-color Wave, 8 - Pulse, 9 -Mixed Pulse, 10 - Night Rider, 11 - Laser.  
For APIv8 - 0 - Off, 1 - Color or Morph, 2 - Pulse, 3 - Back Morph, 7 - Breath, 8 - Spectrum, 9 - One key (Key press only), 10 - Circle out (Key press only), 11 - Wave out (Key press only), 12 - Right wave (Key press only), 13 - Default cyan, 14 - Rain Drop (Key press only), 15 - Wave, 16 - Rainbow wave, 17 - Circle wave, 18 - Random white (Key press only), 19 - Reset to default.  
Supported global effect modes (APIv8 only): 1 - Permanent effect, 2 - On key press effect. This value ignored for APIv5.
For APIv8, number of colors defines color mode. It will be Spectrum if no colors provided, 1-color if one only and 2 colors in case you set.

For `probe` command, `l` for define number of lights, `d` for deviceID and (optionally) lightid. Can be combined or absent.  
By default, first 23 lights (or 136 for keyboard devices) for all devices will be checked.

You can check possible "dev", "light" and "zone" ID values using `status` command. For "dev" and "zone", values out of range is ignored. for "light", you can use any value between 0 and 255.

