# AlienFX-SDK
Better AlienFX/LightFX SDK than Dell official's one without any limitations.

Dell official SDK does a very terrible job if you want to change LED color in quick succession. Their official SDK comes with 3 seconds delay and behaves pretty weird in general.
This SDK not only fixes up its issue and performs better but is written from scratch by reverse engineerig USB protocol. It sends byte data directly to USB which then changes zones color. This also removes the dependency from their Command Center software and works irrespective of settings set in stock software. In other words, you can finally achieve Rainbow effect across all LED's like it was intended without any lag.  This SDK also lets you change color of zones that are not possible with official SDK such as Macro keys, power button etc.

Please checkout Sample App for reference.

**Currently tested on AW13/R2, AW13/R3, AWM14x, AW15R2/R3, AW17R3/R4, AWm15/R1, AWm17/R1, Dell G5, but should be working with all alienware laptops.**

### Supported device API versions:
- 9 bytes 8 bit/color, reportID 2 control (v1)
- 9 bytes 4 bit/color, reportID 2 control (v2)
- 12 bytes 8 bit/color, reportID 2 control (v3)
- 34 bytes 8 bit/color, reportID 0 control (v4)
- 64 bytes 8 bit/color, featureID 0xcc control (v5)

### Supported hardware features:
- Support multiply devices detection and handling
- Support user-provided device, light or group (zone) names
- Change light color
- Change multiply lights color
- Change light hardware effect (except APIv5)
- Change multiply lights hardware effects (except APIv5, emulation at APIv1-v3)

### Initialization
```C++

//This is VID for all alienware laptops, use this while initializing, it might be different for external AW device like mouse/kb
int vid = 0x187c;
  
//Returns PID value if init is successful or -1 if failed. Takes Vendor ID as argument.
int pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::vid);

```

or:
```C++
// let's create informational object for the system
AlienFX_SDK::Mappings info_object;

// Load devices, lights and group names if any
info_object.LoadMappings();

// Let's scan devices into the system... - it's return in pairs VID-PID.
vector<pair<DWORD,DWORD>> device_list;
device_list = info_object.AlienFXEnumDevices();

// now let's init first of them
int pid = AlienFX_SDK::Functions::AlienFXInitialize(device_list[0].first, device_list[0].second);


```

### Set Color
```C++
//Make sure your device is ready to process new instructions before updating color
bool result = AlienFX_SDK::Functions::IsDeviceReady();
std::cout << "\nReady: " << result; 

//Takes index of the location you want to update as first argument and Red, Green and Blue values for others.
AlienFX_SDK::Functions::SetColor(3, 225, 134, 245);
AlienFX_SDK::Functions::SetColor(6, 25, 114, 245);

//This is important to apply the updated color changes. Should only be called once after you're done with new colors.
AlienFX_SDK::Functions::UpdateColors();
```
### Probe app
In [Releases](https://github.com/T-Troll/AlienFX-SDK/releases), you can find simple probe app - it shows DeviceID, DeviceVersion, then tries to switch lights to green position-by-position. Don't forget to share it's results for you gear!

### Projects using this SDK

[Project Aurora](https://github.com/antonpup/Aurora).
[Project AlienFx-tools](https://github.com/T-Troll/alienfx-tools).

##### Special Thanks
Thanks go to Ingrater (http://3d.benjamin-thaut.de/) for his work on AlienFX and providing the protocol for me to work on.
