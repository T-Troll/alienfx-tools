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
- `set-one=<dev-id>,<light-id>,r,g,b[,br]` Set one light to color provided. Check light IDs using `status` command first. Ex: `set-one=0,1,0,0,255` - set light #2 at the device #1 to blue color. For low-level SDK, current active device will be used if devID=0, otherwise it switch to device with PID provided.
- `set-zone=<zone>,r,g,b[,br]` Set zone (see possible zones list below) light to color provided.
- `set-action=<dev-id>,<light-id>,<action>,r,g,b[,br[,<action>,r,g,b,br]]` Set light to color provided and enable action. You can define up to 9 actions in this command, but only first 1 or 2 will be used for high-level API and for older devices. For low-level SDK, current active device will be used if devID=0, otherwise it switch to device with PID provided.
- `set-zone-action=<action>,<zone>,r,g,b[,br,r,g,b[,br]]` Set zone light to color provided and enable action.
- `set-power=<light-id>,r,g,b,r,g,b` Set light as a hardware power button. First color for AC, 2nd for battery power. This command only works with low-level API.
- `set-tempo=<tempo>` Set next action tempo (in milliseconds).
- `set-dev=<pid>` Switch active device to this PID (low-level only).
- `set-dim=<brigtness>` Set active device hardware brightness (dimming) level (from 0 to 255, low-level and API v4-v5 only).
- `lightson` Turn all current device lights on.
- `lightsoff` Turn all current device lights off.
- `update` Updates light status (for looped commands or old devices).
- `reset` Reset current device.
- `low-level` Next commands pass trough low-level API (USB driver) instead of high-level.
- `high-level` Next commands pass trough high-level API (Alienware LightFX), if it's available.
- `loop` Special command to continue all command query endlessly, until user interrupt it. It's provide possibility to keep colors even if awcc reset it. Should be last command in chain.

Supported Zones: `left, right, top, bottom, front, rear` for high-level, any group ID (see in `status`) for low-level. 
Supported Actions: `pulse, morph (you need 2 colors for morph), color (disable action)`. For APIv4 devices, `breath, spectrum, rainbow` also supported. APIv5 not supported.
