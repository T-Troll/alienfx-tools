# alienfx-gui

`alienfx-gui` is AWCC in 500kb. Not 500Mb!  

What it **can nott** do in compare to AWCC?
- No overclocking. Anyway, ThrottleStop or XTU do the job better.
- No audio control. Being honest, never use it in AWCC.
- No huge picture or animations, interface is as minimalistic as possible.
- No Dell LightFX support (but i can emulate it, if needed).
- No services and drivers which send you usage data in Dell, consume RAM and slow down system.

But what it can do, instead?
- Control AlienFX lights. Any way you can imagine:
  - Set the chains of effects to every light, or
  - Set the light to react any system event - CPU load, temperature, HDD access - name it.
  - Group the lights any way you wish and control group not as one light only, but as a gauge of LEDs.
  - It fast!
- Control system fans. Instead of AWCC, fan control are dynamic, so you can boost them any way you want. Or unboost.
- Can have a profile for any application you run, switching not only light sets, but, for example, dim lights or only switch if application in foreground.

## Usage
Run `alienfx-gui.exe`.  
First, use "Devices and Lights" tab to configure out devices and lights settings, if you don't run `alienfx-probe` yet.  
Use "Color" tab for simple lights setup (this colors and modes will stay as default until AWCC run or modified by other app), even after reboot.  
You can also assign event for light to react on (power state, performance indicator, or just activity light), as well as a color for reaction at "Monitoring" tab.  
Tray menu (right-click on tray button) avaliable all the time for some fast switch functins, application hide to tray completely then minimized.  
```
How it works
```

`"Color"` tab is set hardware colors and effects for light. This setting will remains even if you exit application.  
Each light (except Power Button) can have up to 9 different hardware effects assigned to it, but some modes require more, then one effect (f.e. Morph – 2, Spectrum – 7) to work correctly.  
Use "Effects" list to add/remove/select effect. For each effect, you can define its color, effect mode, speed (how fast to change the color), and length (how long it plays).  
Available effect modes are:
- Color - Stay at solid color defined.
- Pulse - Switch between defined color and black.
- Morph - Morph light from previous color to current one.
- Breath - Morph light from black to current color. (for devices with APIv4)
- Spectrum - Like a morph, but up to 9 colors can be defined. (for devices with APIv4)
- Rainbow - Like a Color, but can use up to 9 colors. (for devices with APIv4)

Please keep in mind, mixing different event modes for one light can provide unexpected results, as well as last 2 modes can be unsupported for some lights (will do nothing). But you can experiment.  
“Set All” button copy current light effects to all lights into the list (it’s useful if you need to have all lights into the same color and mode).  

`"Monitoring"` tab designed for system events monitoring and change lights to reflect it - like power events, system load, temperatures.  
There are some different monitoring types avaliable:  

"Light for color and monitoring"  
If this mode is active, selected light will use colors from "Color" tab as initial monitoring color. If this mode disabled, light will use color from "Monitoring" tab, and also will stop update if monitoring off.  

"Power State"  
Light acts as software power button, and reflect current power mode - AC, battery, charge, low battery level.  

"Performance"  
Light acts like a performance indicator, reflecting system parameters:
- CPU Load - CPU load color mix from 0% ("calm") to 100% ("Active")
- RAM Load - The same for used RAM percentage
- GPU Load - The same for utilized GPU percentage (top one across GPUs if more, then one present into the system).
- HDD Load - It's not exactly a load, but IDLE time. If idle - it's "calm", 100% busy - active, and mix between.
- Network load - Current network traffic value against maximal value detected (across all network adapters into the system).
- Max. Temperature - Maximal temperature in Celsius degree across all temperature sensors detected into the system.
- Battery level - Battery charge level in percent (100=discharged, 0=full).
  
You can use "Minimal value" slider to define zone of no reaction - for example, for temperature it's nice to set it to the room temperature - only heat above it will change color.  
"Gauge" checkbox changes behavour for groups only. If Gauge on, all lights in group works as a level indicator (100% color below indicator value, 0% color above indicator value, mixed in between.

"Event"  
Light switches between colors if system event occures:
- Disk activity - Switch light every disk activity event (HDD IDLE above zero).
- Network activity - Switch light if any network traffic detected (across all adapters).
- System overheat - Switch light if system temperature above cut level (default 95C, but you can change it using slider below).
- Out of memory - Switch light if memory usage above 90% (you can change it by the same slider).
- Low battery - Switch light if battery charged below the level defined by slider.
  
"Blink" checkbox switch triggered value to blink between on-off colors 6 times per sec.
You can mix different monitoring type at once, f.e. different colors for same light for both CPU load and system overheat event. In this case Event color always override Performance one then triggered, as well as both override Power state one.

`"Devices"` tab is an extended GUI for `alienfx-probe`, providing devices and lights names and settings, name modification, light testing and some other hardware-related settings.  
"Devices" dropdown shows the list of the light devices found into the system, as well as selected device status (ok/error), you can also edit their names here.  
"Reset" button refresh the devices list (useful after you disconnect/connect new device), as well as re-open each device in case it stuck.  
"Lights" list shows all lights defined for selected device. Use “Add”/”Remove” buttons to add new light or remove selected one.  
NB: If you want to add new light, type light ID into LightID box **before** pressing “Add” button. If this ID already present in list or absent, it will be changed to the first unused ID.  
Doubleclick or press Enter on selected light to edit it's name.  
"Reset light" button keep the light into the list, but removes all settings for this light from all profiles, so it will be not changed anymore until you set it up again.  
"Power button" checkbox set selected light as a "Hardware Power Button". After this operation, it will react to power source state (ac/battery/charging/sleep etc) automatically, but this kind of light change from the other app is a dangerous operation, and can provide unpleasant effects or light system hang.  
"Indicator" checkbox is for indicator lights (hdd, capslock, wifi) if present. Then checked, it will not turn off with screen/lights off (same like power), as well as will be disabled in other apps.
Selected light changes it color to the one defined by "Test color" button, and fade to black then unselected.

"Load Mappings" button loads pre-defined lights map (if exist) for you gear. Map files is a simple .csv defining all devices, it's names, lights and it's types and names. Useful for first start.  
"Save Mappings" button save current active devices and their lights into .csv file. Please, send me this file if you device is not into pre-defined mappings list yet, i'll add it.

`"Groups"` tab provides control for light groups. Each group can be selected and set at "Color" and "Monitoring" pages as a one light.  
Press [+] and [-] buttons to add/remove group, use dropdown to select it or change it name.  
If you have a group selected, "Group lights" list present the list of lights assigned to this group. Use [-->] and [<--] buttons to add and remove light from the group.  

`"Profiles"` tab control profile settings, like selecting default profile, per-profile monitoring control and automatic switch to this profile then the defined application run.  
You can doubleclick or press Enter on selected profile into the list to edit it's name.  
Each profile can have settings and application for trigger it. The settings are:
- "Application" - Defines application executable for trigger profile switch if "Profile auto switch" enabled.
- "Default profile" - Default profile is the one used if "Profile auto switch" enabled, but running applications doesn't fits any other profile. There is can be only one Default profile, and it can't be deleted.
- "Disable monitoring" - Then profile activated, monitoring functions are disabled, despite of global setting.
- "Dim lights" - Then profile activated, all lights are dimmed to common amount.
- "Only then active" - If "Profile auto switch" enabled, and application defined in profile running, profile will only be selected if application window active (have focus).

`"Fans" tab is UI for control system fans and temperature sensors.  
**WARNING** It will only avaliable if system configured for using low-level driver and enabled in "Settings" tab! Read "Requrements" in main readme.md for details!  

First, take a look at "Power mode" dropdown - it can control system pre-defined power modes. For manual fan control switch it to "Manual".  
"Temperature sensors" list present all hardware sensors found at you motheboard (some SSD sensors can absent into this list), and their current temperature values.
"Fans" list present all fans found into the system and their current RPMs.

Additional "Fan curve" window at the right shows currently selected fan temperature/boost curve, as well as current boost.  

Fan control is temerature sensor-driven, so first select one of temperature sensors.  
Then, select which fan(s) should react on it's readings - you can select from none to all in any combination.  
So select checkbox for fan(s) you need.

After you doing so, currently selected fan settings will be shown at "Fan Curve" window - you will see current fan boost at the top, and the fan control curve (green lines).
Now play with fan control curve - it defines fan boost by temperature level. X axle is temperature, Y axle is boost level.  
You can left click (and drag until release mouse button) into the curve window to add point or select close point (if any) and move it.  
You can click right mouse button at the graph point to remove it.  
Big red dot represent current boost-in-action position.  

Please keep in mind:
- You can't remove first or last point of the curve.
- If you move first or last point, it will keep it's temperature after button release - but you can set other boost level for it.
- Then fan controlled by more, then one sensor, boost will be set to the maximal value across them. 

`"Settings"` tab is for application/global lights settings control - states, behavior, dimming, as well as application settings:
- "Turn on lights" - Operate all lights into the system. It will be black if this option disabled (default - on).
- "Lights follow screen state" - Dim/Fade to black lights then system screen dimmed/off (default - off).
- "Power/indicator lights too" - Lights, marked as Power Button or Indicator follows the system state. Such lights will be always on if disabled (default - off).
- "Auto refresh lights" - All lights will be refreshed 6 times per second. It's useful if you have AWCC running, but still want to control lights (default - off).
- "Color Gamma correction" - Enables color correction to make them looks close to screen one. It keep original AWCC colors if disabled (default - on).
- "Dim lights" - Dim system lights brightness. It's useful for night/battery scenario (default - off).
- "Dim Power/Indicator lights too" - Lights, marked as Power Button or Indicator follows dim state. Such lights will be always at full brightness if disabled (default - off).
- "Dim lights on battery" - Automatically dim lights if system running at battery power, decreasing energy usage (default - on).
- "Dimming power" - Amount of the brightness decrease then dimmed. Values can be from 0 to 255, default is 92.
- "Start with Windows" - Start application at Windows start. It will not work if application request run as admin level (see below) (default - off).
- "Start minimized" - Hide application window in system tray after start.
- "Enable monitoring" - Application start to monitor system metrics (CPU/GPU/RAM load, etc) and refresh lights according to it (default - on).
- "Profile auto switch" - Switch between profiles available automatically, according of applications start and finish. This also block manual profile selection (default - off).
- "Disable AWCC" - Application will check active Alienware Control Center service at the each start and will try to stop it (and start back upon exit). It will require "Run as administrator" privilege (default - off).
- "Esif temperature" - Read hardware device temperature counters. If disabled, only system-wide ones will be read. It's useful for some Dell and Alienware systems, but also provide a lot of component temperature readings. It will require "Run as administrator" privilege (default - off).

"Global effects" block provide global control for light effects for current device. It's only work for APIv5 devices, provide some cool animations across all keys. 

Keyboard shortcuts (any time):
- CTRL+SHIFT+F12 - enable/disable lights
- CTRL+SHIFT+F11 - dim/undim lights
- CTRL+SHIFT+F10 - enable/disable system state monitoring
- F18 (on Alienware keyboards it's mapped to Fn+AlienFX) - cycle light mode (on-dim-off)

Other shortcuts (only then application active):
- ALT+1 - switch to "Colors" tab
- ALT+2 - switch to "Monitoring" tab
- ALT+3 - switch to "Devices and Lights" tab
- ALT+4 - switch to "Profiles" tab
- ALT+5 - switch to "Settings" tab
- ALT+r - refresh all lights
- ALT+m - minimize app window
- ALT+s - save configuration
- ALT+? - about app
- ALT+x - quit

**WARNING:** All hardware color effects stop working if you enable any Event monitoring. It’s a hardware bug – any light update operation restart all effects.  
