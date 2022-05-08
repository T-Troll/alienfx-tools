# AlienFX Control (ex-Alienfx-gui)

`Alienfx-gui` is AWCC in 500kb. Not 500Mb!  

What it **cannot** do in compare to AWCC?
- No overclocking. Anyway, ThrottleStop or XTU do the job better.
- No audio control. Being honest, never use it in AWCC.
- No huge picture or animations, interface is as minimalistic as possible.
- No services and drivers which send you usage data in Dell, consume RAM and slow down system.

But what it can do, instead?
- Control AlienFX lights. Any way you can imagine:
  - Set the chain of effects to every light, or
  - Set the light to react any system event - CPU load, temperature, HDD access - name it.
  - Group the lights any way you wish and control group not as one light only, but as a gauge of LEDs.
  - Make lights react on screen colors (Ambient lights).
  - Make lights react on sound around (Haptics).
  - It fast!
- Control system fans. Instead of AWCC, fan control is dynamic, so you can boost them any way you want. Or reset boost.
- Can have a profile for any application you run, switching not only light sets, but, for example, dim lights or only switch if application in foreground.
- Fast, less buggy and robust LightFX support (emulation).

## Usage
Run `alienfx-gui.exe`.  
First, use "Devices and Lights" tab to configure out devices and lights settings, if you don't run `Alienfx-probe` yet.  
Use "Colour" tab for simple lights setup (these colours and modes will stay as default until AWCC run or modified by another app), even after reboot.  
There are 4 Software effect modes available:
 - Monitoring - Assign event for lights to react on (power state, performance indicator, or just activity light), as well as a colour for reaction at "Monitoring" tab.
 - Ambient - Assign lights to change colors with the screen zones.
 - Haptics - Lights will react on sound played by the system or external sounds. 
 - Off - Disable software effects.
Tray menu (right-click on tray button) available all the time for some fast switch functions, application hide to tray completely then minimized.  
```
How it works
```
![Color tab](/Doc/img/gui-color.png?raw=true)

`"Color"` tab is set hardware colors and effects for light. This setting will remain even if you exit application.  

Each light (except Power Button) can have up to 9 different hardware effects assigned to it, but some modes require more, then one effect (f.e. Morph – 2, Spectrum – 7) to work correctly.  
Use "Effects" list to add/remove/select effect. For each effect, you can define its colour, effect mode, speed (how fast to change the colour), and length (how long it plays).  
Available effect modes are:
- Color - Stay at solid color defined.
- Pulse - Switch between defined color and next color(s).
- Morph - Morph light from previous color to current one.
- Breath - Morph light from black to current color. (For devices with APIv4)
- Spectrum - Like a morph, but up to 9 colors can be defined. (For devices with APIv4)
- Rainbow - Like a Colour, but can use up to 9 colors. (For devices with APIv4)

Please keep in mind, mixing different event modes for one light can provide unexpected results, as well as last 2 modes can be unsupported for some lights (will do nothing). But you can experiment.  

"Set All" button copies current light effects to all lights into the list (it’s useful if you need to have all lights into the same colour and mode).  

"Light Information" block reveals color information and prediction for currently selected light. It shows all settings for it across all tabs and modes, and give a possibility to predict final color range, as well as modify it components (remove light parameters and group participation).

![Monitoring tab](/Doc/img/gui-monitoring.png?raw=true)

`"Monitoring"` tab designed for system events monitoring mode for Software effects and change lights to reflect it - like power events, system load, temperatures.  
There are some different monitoring sources and modes available:  

"Light for colour and monitoring"  
If this mode is active, selected light will use colours from "Colour" tab as initial monitoring colour. If this mode disabled, light will use colour from "Monitoring" tab, and also will stop update if monitoring off.  

"Power State"  
Light acts as software power button, and reflect current power mode - AC, battery, charge, low battery level.  

"Performance"  
Light acts like a performance indicator, reflecting system parameters:
- CPU Load - CPU load colour mix from 0% ("calm") to 100% ("Active")
- RAM Load - The same for used RAM percentage
- GPU Load - The same for utilized GPU percentage (top one across GPUs if more, then one present into the system).
- HDD Load - It's not exactly a load, but IDLE time. If idle - it's "calm", 100% busy - active, and mix between.
- Network load - Current network traffic value against maximal value detected (across all network adapters into the system).
- Max. Temperature - Maximal temperature in Celsius degree across all temperature sensors detected into the system.
- Battery level - Battery charge level in percent (100=discharged, 0=full).
- Max. Fan RPM - Maximal RPM (in percent of maximum) across all system fans. This indicator will only works if "Fan control" is enabled.
- Power consumption - Current system power drain level. This indicator will only works if "ESID sensors" enabled at "Settings".
  
You can use "Minimal value" slider to define zone of no reaction - for example, for temperature it's nice to set it to the room temperature - only heat above it will change colour.  
"Gauge" check box changes behavior for groups only. If Gauge on, all lights in group works as a level indicator (100% colour below indicator value, 0% colour above indicator value, mixed in between.

"Event" - Light switches between colours if system event occurs:
- Disk activity - Switch light every disk activity event (HDD IDLE above zero).
- Network activity - Switch light if any network traffic detected (across all adapters).
- System overheat - Switch light if system temperature above cut level (default 95C, but you can change it using slider below).
- Out of memory - Switch light if memory usage above 90% (you can change it by the same slider).
- Low battery - Switch light if battery charged below the level defined by slider.
- Language indicator - Light will have 1st color if first input language selected, and second color if any other selected.
  
"Blink" check box switch triggered value to blink between on-off colours 6 times per sec.
You can mix different monitoring type at once, f.e. different colours for same light for both CPU load and system overheat event. In this case Event colour always override Performance one then triggered, as well as both override Power state one.

![Ambient tab](/Doc/img/gui-ambient.png?raw=true)

`Ambient` tab defines ambient light effect mode. It get screen shot (for the primary or secondary display), then divide it to several zones.  
For each zone, dominant color calculated (you can see it at the button in app interface).  
For each light found into the system, you can define zone(s) it should follow by pressing corresponding grid button. If more, then one zone selected for light, it will try to blend zone colors into one.  
You can also select which display to grab - primary or secondary, if you have more, then one.  
Sliders below and right to zone grid can be used to change grid density, providing more or less light zones on the screen.  
"Dimming" slider decreases the overall lights brightness - use it for better fit you current monitor brightness.  
"Reset capture" button is used to refresh light list according to devices found into the systems, as well as restart screen capture process.

![Haptics tab](/Doc/img/gui-haptics.png?raw=true)

`Haptics` tab set up parameters for Haptics effect mode. This effect mode get audio stream from default output or input device (you can select it at the "Audio Source" block), then made a real-time spectrum analysis.  
After that, spectrum powers grouped into 20 groups using octave scale.  
For each light found into the system, you can define Group of frequency diapason(s) it should react, as well as color for Lowest Hi power level into frequency group. If more, then one group is selected, power will be calculated as a medium power level across of them.  
Use "+" and "-" buttons to add frequency group, then select freelances for this group.  
It's also possible to compress diapason if group always not so or so high powered - use low-level and high-level sliders. Low-level slider define minimum power to react (all below will be treated as zero), and Hi-level slider defines maximum level (all above will be treated as maximum).  
"Clear” button set all colors to black and sliders to default value.
"Gauge" check box - change behavior for groups only. If Gauge on, all lights in group works as a peak indicator (hi-color below power level, low-color above power level, mixed in between).  
"Axis" check box enable axis lines and marks at sound visualization window.

![Groups tab](/Doc/img/gui-groups.png?raw=true)

`"Groups"` tab provides control for light groups. Each group can be selected and set at "Colour" and "Monitoring" pages as a one light.  
Press [+] and [-] buttons to add/remove group, use drop-down to select it or change its name.  
If you have a group selected, "Group lights" list present the list of lights assigned to this group. Use [-->] and [<--] buttons to add and remove light from the group.  

![Profiles tab](/Doc/img/gui-profiles.png?raw=true)

`"Profiles"` tab control profile settings, like selecting default profile, per-profile monitoring control and automatic switch to this profile then the defined application run.  

Press "+" or "-" buttons to add or remove profile. New profile settings will be copied from currently selected profile.  
For clear light settings for selected profile press "Reset" button.  
"Copy active" button will copy all profile settings from the profile currently in action into system.  
You can double-click or press Enter on selected profile into the list to edit its name.  

Each profile can have settings and application for trigger it. The settings are:
- "Effect mode" - Software effect mode for this profile: Monitoring, Ambient, Haptics, Off (The same as "Disable monitoring" before).
- "Default profile" - Default profile is the one used if "Profile auto switch" enabled, but running applications doesn't fit any other profile. There is can be only one Default profile, and it can't be deleted.
- "Only then active" - If "Profile auto switch" enabled, and application defined in profile running, profile will only be selected if application window active (have focus).
- "Priority profile" - If this flag enabled, this profile will be chosen upon others. Priority profile overrides "Only then active" setting of the other profiles. 
- "Dim lights" - Then profile activated, all lights will be dimmed.
- "Fan settings" - If selected, profile also keep fan control settings and restore it then activated.
- "Global effects" - Enable global effects for supported devices for this profile. Global effect will be disabled by default, if this not selected for active profile.

"Global effects" block controls effect mode, temp and colors for APIv5 devices, provide some cool animations across all keys if enabled.

"Trigger applications" list define application executables, which will activate selected profile if running and "Profile auto switch" is on.  
Press "+" button to select new application, or select one from the list and press "-" button to delete it.

If application's "Profile auto switch" setting is on, active profile will be selected automatically according to this rules:
- If running application, belongs to any profile not found - "Default" profile selected.
- If running applications belongs to profile found and have no "Only when active" flag, "Priority" one will be chosen, or the random profile if no "Priority" found.
- If foreground application belongs to profile, and no other application belongs to Priority profile found, application-related profile will be selected, otherwise priority profile will be chosen.

![Devices tab](/Doc/img/gui-devices.png?raw=true)

`"Devices"` tab is an extended GUI for `Alienfx-probe`, providing devices and lights names and settings, name modification, light testing and some other hardware-related settings.  

"Devices" drop-down shows the list of the light devices found into the system, as well as selected device status (ok/error), you can also edit their names here.  
"Reset" button refresh the devices list (useful after you disconnect/connect new device), as well as re-open each device in case it stuck.  
"Lights" list shows all lights defined for selected device. Use “Add”/”Remove” buttons to add new light or remove selected one.  
NB: If you want to add new light, type light ID into LightID box **before** pressing “Add” button. If this ID already present in list or absent, it will be changed to the first unused ID.  
Double-click or press Enter on selected light to edit its name.  
"Reset light" button keep the light into the list, but removes all settings for this light from all profiles, so it will be not changed anymore until you set it up again.  
"Power button" check box set selected light as a "Hardware Power Button". After this operation, it will react to power source state (ac/battery/charging/sleep etc) automatically, but this kind of light change from the other app is a dangerous operation, and can provide unpleasant effects or light system hang.  
"Indicator" check box is for indicator lights (HDD, caps lock, WiFi) if present. Then checked, it will not turn off with screen/lights off (same like power), as well as will be disabled in other apps.
Selected light changes it colours to the one defined by "Test colour" button, and fade to black then unselected.

"Detect lights" button opens "New Devices detection" dialog with database to provide possible device/light names for you gear from it:
![New Devices dialog](/Doc/img/new_devices.png?raw=true)  
"Load Lights" button loads a backup (saved at this or similar device). If devices is different, it will try to apply as much similar configuration as possible.  
"Save Lights" button save a backup of current light and device names and settings.  

"White point" block - some devices (especially old v2 ones) have not correct color correction, so white color looks weird. You can tune it using this block.  
Use "Test White" button to switch testing color for all lights to white for result check.

![Fans tab](/Doc/img/gui-fans.png?raw=true)

`"Fans"` tab is UI for control system fans and temperature sensors.  

First, take a look at "Power mode" drop-down - it can control system predefined power modes. For manual fan control switch it to "Manual". You can edit mode name by selecting it and type new name.  
"Temperature sensors" list present all hardware sensors found at your motherboard (some SSD sensors can absent into this list), and their current and maximal (press "x" button at list head to reset it to current values) temperature values.
"Fans" list presents all fans found into the system and their current RPMs.  
"GPU limit" slider define top maximal GPU board power limit shift. 0 (left) is no limit, 4 (right) - maximal limit. This feature only supported at some notebooks, and real limit can vary. You can use this slider to keep more power at CPU or extend battery life.  
"CPU Boost" drop-downs can be used to select active Windows Power Plan boost mode (separately for AC and battery power). This settings is extremely useful for Ryzen CPU, but even for Intel it provide a little performance boost (+3% at "Aggressive" for my gear).  

Additional "Fan curve" window at the right shows currently selected fan temperature/boost curve, as well as current boost.  

Fan control is temperature sensor-driven, so first select one of temperature sensors.  
Then, select which fan(s) should react on its readings - you can select from none to all in any combination.  
So, select check box for fan(s) you need.

After you doing so, currently selected fan settings will be shown at "Fan Curve" window - you will see current fan boost at the top, and the fan control curve (green lines).
Now play with fan control curve - it defines fan boost by temperature level. X axle is temperature, Y axle is boost level.  
You can left click (and drag until release mouse button) into the curve window to add point or select close point (if any) and move it.  
You can click right mouse button at the graph point to remove it.  
Big red dot represents current boost-in-action position.  

Please keep in mind:
- You can't remove first or last point of the curve.
- If you move first or last point, it will keep its temperature after button release - but you can set other boost level for it.
- Then fan controlled by more, then one sensor, boost will be set to the maximal value across them. 

![Settings tab](/Doc/img/gui-settings.png?raw=true)

`"Settings"` tab is for application/global lights settings control - states, behavior, dimming, as well as application settings:
- "Turn on lights" - Operate all lights into the system. It will be black if this option disabled (default - on).
- "Lights follow screen state" - Dim/Fade to black lights then system screen dimmed/off (default - off).
- "Keep Power/indicator on" - Lights, marked as Power Button or Indicator will be always on, or follows the system state if disabled (default - on).
- "Turn off for battery" - Lights will turned off at battery power and back on at AC. (default - off).
- "Colour Gamma correction" - Enables colour correction to make them looks close to screen one. It keeps original AWCC colours if disabled (default - on).
- "Enable software effects" - Global software effect switch. If it's off, effects always disabled, if it's on - effect mode defined by current profile.
- "Dim lights" - Dim system lights brightness. It's useful for night/battery scenario (default - off).
- "Dim Power/Indicator lights" - Lights, marked as Power Button or Indicator follows dim state. Such lights will be always at full brightness if disabled (default - off).
- "Dim lights on battery" - Automatically dim lights if system running at battery power, decreasing energy usage (default - on).
- "Dimming power" - Amount of the brightness decrease then dimmed. Values can be from 0 to 255, default is 92.
- "Start with Windows" - Start application at Windows start. It will not work if application request run as admin level (see below) (default - off).
- "Start minimized" - Hide application window in system tray after start (default - off).
- "Check for update" - Enables online update check (default - on).
- "Enable monitoring" - Application start to monitor system metrics (CPU/GPU/RAM load, etc) and refresh lights according to it (default - on).
- "Profile auto switch" - Switch between profiles available automatically, according of applications start and finish. This also block manual profile selection (default - off).
- "Do not switch for desktop" - If enabled, profile auto switch will not change profile, if start menu/tray/desktop selected (default - off).
- "Disable AWCC" - Application will check active Alienware Control Center service at the each start and will try to stop it (and start back upon exit). It will require "Run as administrator" privilege (default - off).
- "ESIF sensors" - Read hardware devices data. If disabled, Power measure and Thermal zone temperature sensors not available. It will require "Run as administrator" privilege (default - off).
- "Enable fan control" - Enables fan and power control functionality, if available for you hardware.


## Keyboard shortcuts 

Global shortcuts (works all time application running):
- CTRL+SHIFT+F12 - enable/disable lights.
- CTRL+SHIFT+F11 - dim/undim lights for current profile
- CTRL+SHIFT+F10 - enable/disable software effects
- CTRL+SHIFT+F9 - enable/disable profile auto switch
- CTRL+SHIFT+1..9 - switch active profile to profile #N (profile order is the same as at "Profiles" tab)
- CTRL+ALT+0..5 - switch active power mode (in case fan control enabled). 0 is for Manual, 1..5 for system-defined
- F18 (on Alienware keyboards it's mapped to Fn+AlienFX) - cycle light mode (on-dim-off)
- F17 (G-key for Dell G-series laptops) - cycle between manual and performance power mode (in case Fan Control enabled)

Other shortcuts (only then application active):
- ALT+1..ALT+8 - switch to corresponding tab (from left to right)
- ALT+r - refresh all lights
- ALT+m - minimize app window
- ALT+s - save configuration
- ALT+? - about app
- ALT+x - quit

**WARNING:** All hardware colour effects stop working if you enable any software effect. It’s a hardware bug – any light update operation restarts all effects.  
