# AlienFX-SDK

Better AlienFX/LightFX SDK than Dell official's one without any limitations:
- Better performance. Update rate can be very high for modern devices.
- Tiny footprint both in code and RAM.
- Full hardware features support - per-key and per-device effect set, hardware brightness, hardware power buttons.
- Using direct hardware access, so no additional software required.

### Supported devices:

Any Alienware/Dell G-series hardware with RGB lights from 2010 to 2022. Including mouses, keyboards, monitors.  
Read more details about supported devices and models [here](https://github.com/T-Troll/alienfx-tools/wiki/Supported-and-tested-devices-list)

### Supported device API versions:

- ACPI-controlled lights - 3 lights, 8 bit/color (v0) - Aurora R6/R7 (using this API require AlienFan-SDK library from [AlienFX-Tools](https://github.com/T-Troll/alienfx-tools) project).
- 9 bytes 8 bit/color, reportID 2 control (v1) - Ancient notebooks - deprecated and removed.
- 9 bytes 4 bit/color, reportID 2 control (v2) - Older notebooks (like m14/17x, 13R1/R2)
- 12 bytes 8 bit/color, reportID 2 control (v3) - Old notebooks (like 15R5)
- 34 bytes 8 bit/color, reportID 0 control (v4) - Modern notebooks/desktop (all m-series, x-series, Dell g-series, Aurora R8+)
- 64 bytes 8 bit/color, featureID 0xcc control (v5) - Modern notebooks internal per-key RGB keyboard (all m- and x-series)
- 65 bytes 8 bit/color, interrupt control (v6) - Mouses
- 65 bytes 8 bit/color, featureID control (v7) - Monitors
- 65 bytes 8 bit/color, Interrupt control (v8) - External keyboards

Check compatibility list and API details [here](https://github.com/T-Troll/alienfx-tools/wiki/Supported-and-tested-devices-list).

Some notebooks can have 2 devices - APIv4 (for logo, power button, etc) and APIv5 for keyboard.

### Supported hardware features:
- Multiply devices detection and handling
- User-provided device, light or group (zone) names
- Change light color
- Change multiply lights color
- Change light hardware effect (except APIv0 and v5)
- Change multiply lights hardware effects (except APIv0 and APIv5)
- Hardware-backed global light off/on/dim (dim is software for v6 and should be done by application)
- Global (all lights) hardware light effects (APIv5, v8)
- Power/indicator button light control (AC/battery hardware switch and effects)

### Initialization
```C++

//This is VID for all alienware devices, use this while initializing, it might be different for external AW device like mouse/kb
int vid = 0x187c;

// This is device object
AlienFX_SDK::Functions device;
  
//Returns PID value if init is successful or -1 if failed. Takes Vendor ID as argument. If more, then one device present first one returned.
int pid = device.AlienFXInitialize(vid);

```

or:
```C++
// let's create informational object for the system
AlienFX_SDK::Mappings info_object;

// Load devices, lights and group names if any
info_object.LoadMappings();

// Let's scan devices into the system... - it's return links to device object
vector<AlienFX_SDK::Functions*> device_list;
device_list = info_object.AlienFXEnumDevices();

// now let's check the version of the first device..
int api_version = device_list[0]->GetVersion();

```

Or even just fill and init internal device structures:
```C++
// let's create informational object for the system
AlienFX_SDK::Mappings info_object;

// Load devices, lights and group names if any
info_object.LoadMappings();

// No fill and init all active devices
info_object.AlienFXAssignDevices(true);

// Then check how many active devices we have
size_t how_many_devices = info_object.fxdevs.size();

```

### Set Color
```C++
// define color/effect structure...
AlienFX_SDK::Afx_action c = {0,0,0,225,134,245};

// Set color for LightID 3 at single device
device.SetColor(3, c);
// But SetColor is a wrapper for SetAction, so next line is equivalent (also it use info_object devices list object)
info_object.fxdevs[0].dev->SetAction(3, &c);

// Or set multiply lights at the same color
vector<byte> lights = { 0,2,3,4,5 };
device.SetMultiColor(&lights, c);

//This is important to apply the updated color changes for some devices. Should only be called once after you're done with new colors for all lights you want to change.
device.UpdateColors();
```

### Projects using this SDK

[Project Aurora](https://github.com/antonpup/Aurora).  
[Project AlienFx-tools](https://github.com/T-Troll/alienfx-tools).

##### Special Thanks
Thanks go to Ingrater (http://3d.benjamin-thaut.de/) for his work on AlienFX and providing the protocol for me to work on.
Welcome joined investigation with [OpenRGB](https://gitlab.com/CalcProgrammer1/OpenRGB/) team, providing clarification with APIv4 (tron) interface!
