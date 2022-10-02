# AlienFX Control application

`AlienFX-GUI` is AWCC in 500kb. Not 500Mb!  

What it **cannot** do in compare to AWCC?
- No over clocking and down volt. Anyway, ThrottleStop or XTU do the job better.
- No audio control. Being honest, never use it into AWCC.
- No huge picture or animations, interface is as minimalistic as possible.
- No services and drivers which send you usage data in Dell, consume RAM and slow down system.

But what it can do, instead?
- Control AlienFX lights. Any way you can imagine:
  - Set the chain of effects to every light, or
  - Set the light to react any system event - CPU load, temperature, HDD access - name it.
  - Group the lights any way you wish and control group not as one light only, but as a gauge of LEDs.
  - Make lights react on screen colors (Ambient lights).
  - Make lights react on sound around (Haptics).
- Control system fans. Instead of AWCC, fan control is dynamic, so you can boost them any way you want. Or reset boost.
- Can have a profile for any application you run, switching not only light sets, but, for example, dim lights or only switch if application in foreground.
- Fast, less buggy and robust LightFX emulation for 3rd-party applications.

Read wiki page [How to start](https://github.com/T-Troll/alienfx-tools/wiki/How-to-start-(Beginner's-guide)-for-release-v6.x.x.x) for more details about initial setup.

## Main screen

Top-left is profile selection drop down. It present currently active profile and you can change it to other any time. All changes made will be applied to it.  
Top-right is software effect drop down, it defines currently active effect mode.  
There are 4 Software effect modes available:
 - Monitoring - Lights will react to current system state (f.e. RAM/CPU load, temperatures, network activity).
 - Ambient - Lights follow screen colors from you game or video player (well... any application or desktop).
 - Haptics - Lights will react on any sound played (captured by any game, audio/video player, messenger or even microphone).
 - Grid - Enables grid effect mode - spatial-based effects.
 - Off - Disable software effects.

Enabling any effect will stop hardware per-light effects - it's a hardware limitation.

"Refresh" button update all lights colors according to current profile settings (color sets, effects, etc).  
"Save" button saves configuration, "Minimize" button hides application into tray.

Tray menu (right-click on tray icon) available for fast switch functions, application will hide into tray completely then minimized.  

The rest of the window is control function tabs. There are 4 main control blocks available:

## Lights tab

All lights functions of you system controlled from here. It have 5 sub-tabs, but 2 blocks are common for most of it:
- Lights grid (down-right block). It present how lights located at you gear surface, as well as show light color prediction according to current light settings and effect mode.
- "Zones" block. It used for group lights to light zone for control it as one light. Press "+" button to add new zone (or select one from other profiles), "-" to remove selected zone, select (left-click or click-and-drag) cells into grid to add/remove assigned lights into zone. Pressing "X" button near grid remove all grid lights from current zone.

Rest of the screen is control block, it depends of function selected by tab: 

### Colors

!["Colors" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-colors-6.png?raw=true)

`"Colors"` tab is set color and hardware effects for light. This setting will remain even if you exit application.

Each light zone (except Power Button) can have up to 9 different hardware effects ("Actions") assigned to it, but some modes require more, then one action (f.e. Morph – 2, Spectrum – 7) to work correctly.  
Hardware button can only have exactly 2 actions (for AC and for battery).

Use "Actions" list to add/remove/select action. For each action, you can define its color, effect mode, speed (how fast to change the colour), and length (how long it plays).  
Available effect modes are:
- Color - Stay at solid color defined.
- Pulse - Switch between defined color and next color(s).
- Morph - Morph light from previous color to current one.
- Breath - Morph light from black to current color. (For devices with APIv4)
- Spectrum - Like a morph, but up to 9 colors can be defined. (For devices with APIv4)
- Rainbow - Like a Colour, but can use up to 9 colors. (For devices with APIv4)
- Power - for hardware power button.

Please keep in mind, mixing different event modes for one light can provide unexpected results, as well as last 2 modes can be unsupported for some lights (will do morph). But you can experiment.

"Gauge" block controls how to zone lights color will be altered depends on it position on the grid.  
For gauge type drop down, available modes are:
- Off - All zone lights will have the same color.
- Horizontal - fill zone from left to right.
- Vertical - fill zone from top to bottom.
- Diagonal (left) - fill zone from top-left point to bottom-right.
- Diagonal (right) - fill zone from top-right point to bottom-left.
- Radial - fill zone by ellipse from center to borders.

"Reverse" check box invert filling direction to right-to-left and bottom-to-top.
"Gradient" check box alter zone filling. In case zone have 2 colors, it will filled as gradient then enabled. For event/haptics gauge, it will filled by gradient color instead of peak indicator.

Please keep in mind - defining zone as "Gradient" disables hardware color effects!

### Events Monitoring

!["Events Monitoring" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-monitoring-6.png?raw=true)

This tab designed to control "Monitoring" software effect - it change zone colors based on system states and events - like power source changes, system load, temperatures.  
There are some different monitoring types and modes available:  

"Power State"  
Zone will act as software power button, and reflect current power mode - AC, battery, charge, low battery level.  

"Performance"  
Zone will reflect a performance indicator, available indicators are:
- CPU load - CPU load color mix from 0% ("Idle") to 100% ("Full load")
- RAM load - Zone color reflect used RAM percentage
- GPU load - Current GPU utilization percentage (top one across all GPUs if more, then one present GPUs into the system).
- Storage load - It's not exactly a load, but IDLE time. If you HDD/SSD not load - it's "idle", 100% busy - full load, and mix between.
- Network load - Current network traffic value against maximal value detected (across all network adapters into the system).
- Max. temperature - Maximal temperature in Celsius degree across all temperature sensors detected into the system.
- Battery level - Battery charge level in percent - 2nd color for full battery, 1st one for completely drained.
- Max. Fan RPM - Highest fan RPM (in percent of maximum) across all system fans. This indicator will only works if "Fan control" is enabled in "Settings".
- Power consumption - Current system power drain level. This indicator will only works if "ESID sensors" enabled at "Settings".

You can use "Minimal value" slider to define zone of no reaction - for example, for temperature it's nice to set it to the room temperature - only heat above it will change colour.  
This monitoring type support "Gauge" setting for zone, acting as a peak indicator for value.

"Event"  
Zone color switches between on and off values if system event occurs:
- Storage activity - Switch zone color every disk activity event (Storage IDLE above zero).
- Network activity - Switch zone color if any network traffic detected (across all adapters).
- System overheat - Switch zone color if system temperature above trigger value.
- Out of memory - Switch zone color if memory usage above trigger value.
- Low battery - Switch light if battery charged below trigger value.
- Language indicator - Light will have "off" color if first input language selected, and "on" color if any other selected.

Default trigger value is 95, you can change it using corresponding slider.  
  "Blink" check box switch triggered value to blink between on-off colours about 5 times per second.

"Use default color" check box, if activated, always use settings from "Colors" tab as a first color.

You can mix different monitoring type at once, f.e. different colours for same zone for both CPU load and system overheat event. In this case Event colour always override Performance one then triggered, as well as both override Power state.

### Ambient tab

!["Ambient" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-ambient-6.png?raw=true)

`Ambient` tab setting up "Ambient" (Ambient lights) software effect mode - in this mode, zone colors will follow colors at you system screen.

You can select which display (primary or all secondary) to follow, as well as define dimming to make light brightness be balanced with screen brightness.  
"Reset" button restart color capturing from screen, it's useful in some situations like DirectX 12 game quit.

"Screen zones" grid define which screen area current zone will follow. Click on corresponding are to add/remove it to zone reactions.
In case of "Ambient" effect mode active, button colors will filled from the screen one.
Sliders below and right to screen area grid used to change grid density, providing more or less areas screen will be divided. Default is 4x3.  

### Haptics tab

!["Haptics" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-haptics-6.png?raw=true)

It's about sound and audio haptics control.

Use "Audio source" block to select between audio inputs ("Output" is for all sound from the system, "Input" is for incoming sounds - Line In or Microphone).

In case "Haptics" effect active, "Frequency/level" window reflect current sound spectrum divided by 20 frequency groups.  

One or more Reactions Groups can be assigned to zone - each group define reaction for frequencies. It have two levels - high and low (red and green lines), final color for zone defined as "low-level" one if sound level below low, mix of the "low-level" and "high-level" if sound level between them and "high-level" if it's above high. Final zone color will be set as a mix of all group colors.  
Press "+" button to add Reaction group, "-" button to remove it, "X" button to reset it levels, colors and frequencies.

Click at desired frequency at "Frequency/level" window to add/remove it into group, click-and-drag the level mark to set it other value.  
Click color buttons for change group hi/low level colors.

### Grid Effect tab

!["Grid effect" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-grideffect.png?raw=true)

Grid effects is a group of software light effects, operating not with lights only, but also using it position on grid. It's like "Global effect" on some RGB keyboards, but have flexible controls and can reflect some system events.

Each effect have trigger to start, position to start, type, and phase.  
Then effect triggered, it's starting to change light colors from the trigger point according to direction, increasing it until "size" parameter reached, then stops.

At "Grid Effect" tab, selected zone, in general, define not the lights involved into effect, but it's borders on grid. So, create a zone covering all grid for full operation, or use one of the zones to limit effect area to the part of the grid.

Next, you should define trigger - the event which launch effect for this zone. Currently, available triggers are:
- Off - grid effect disabled for zone (default)
- Continues - grid effect always start at the beginning point of the zone (depend on its direction)
- Random - grid effect start at random point inside zone
- Keyboard - grid effect start from the position of the light with the name same as pressed key.
- Event - grid effect start if one of monitoring event happened (see Event Monitoring tab).

Continues and Random triggers are always on, so effect start again after finished. Other triggers are one-time, so effect stop then finished.  
For Keyboard trigger, light names should be the same as key name (please use capital letters, "A" right, "a" not), and common English other key names from the capital (f.e. "Space", "Esc").

Possible effect types:
- Running light - phase point(s) have "to" color, the rest have "from" color.
- Wave - Lights across phase point have a gradient between "From" and "To" colors based on "Width" parameter.
- Gradient - The same like "Wave", but gradient is from "To" color at borders into "From" color into the center.

The direction block is the same as "Gauge" block at "Colors" tab, it defines effect phase grow direction and type.

"Speed" is how fast an effect grows (change it phase). Possible value is from -80 (1/80 cell per phase) to +80 (80 cell in phase).  
"Size" is maximal effect size before stop (in cells).  
"Width" defines how many positions around phase point will be involved into color change (in cells).

"Circle" check box enables effect rollback - so after reaching size defined by "Size", it is starting to decrease phase until reach initial state.  
"Zone lights only" - in case this check box enabled, only lights including into zone will participate into effect, otherwise all lights defined by zone borders. Example: you have defined a zone with "WASD" lights. Without this check box, QWEASD lights will be involved, but only "WASD" if enabled.

### Devices and Grids

!["Devices and Grids" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-devices-6.png?raw=true)

This tab dedicated to assign device lights into grids, so it doesn't have Zones block.

Grids block is different, then other light tabs, it have some additional features:
- You can use sliders near grid to change it size.
- You can double-click grid name to edit it.
- You can use "+" and "-" tabs to add new grid/remove current one.

Top-left zone is about devices control - you can select device from list (double-click to change it's name). Press "Set white balance" button to change white color correction for selected device (useful for some old 4-bit-per-color devices).  
Button block is about settings load/save - press "Detect devices" to look for your device list into application database, use "Save Grids and Lights" and "Load Grids and Lights" to save/load current settings into .csv file (for backup or sharing).

Top-right zone controls currently selected (at selected device) light.  
Physical light ID provided after "Light:" mark, next 4 buttons used for navigation between lights:
- "|<" (or press Shift+Home) - navigate to first assigned light of the current device.
- "<" (or Shift+Left) - select previous light (by ID).
- ">" (Shift+Right) - select next light (by ID).
- ">|" (and also Shift+End) - navigate to last assigned light of the current device.
- "Reset" - Unassigns current light, delete it from all zones, clear it's name and parameters (forget light).

Check boxes define light type - "Power button" marks light as a hardware power button (be careful, but app will provide you some hints about it), "Indicator" light set it as a status indicator (like Caps Lock, Wifi, HDD) - some systems use controlled lights for it.  
Both Power button and indicators can be configured in settings to stay on then the rest of lights are off/dimmed.

"Highlight" button defines the color will be used for currently selected light - both for light grid and for physical light.  
Other assigned lights will have random colors at grid and black color for physical lights.

"Key" button provide dialog window to select current light name by pressing any keyboard key. It's also will be done automatically in case per-key RGB device active.

Then you find light position at you physical device (it's highlighted), assign it to the grid - click or click-and drag at grid zone to do it.  
Click again to remove grid sell from current light, right-click (or click-and-drag) to clear grid cell(s).  
Left-click at already assigned cell to select corresponding light (and it's device).

Optionally, you can set current light name using name field into "Light" block. Use "Key" button to assign light name by pressing corresponding keyboard key.

## Fans and Power

!["Fans and Power" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-fans-6.png?raw=true)

It's all about fan and some power settings control for your gear.

First, take a look at "Power mode" drop-down - it can control system predefined power modes. For manual fan control switch it to "Manual". You can edit mode name by selecting it and type new name.  
"G-Mode" check box enables and disables G-Mode (power boost) for supported hardware (all G-Series, some Alienware).
"Temperature sensors" list present all hardware sensors found at your BIOS (some SSD sensors can absent into this list), and their current and maximal temperature values.
"Fans" list presents all fans found into the system and their current RPMs.  
"GPU limit" slider define top maximal GPU board power limit shift. 0 (left) is no limit, 4 (right) - maximal limit. This feature only supported at some notebooks, and real limit can vary. You can use this slider to keep more power at CPU or extend battery life.  
"CPU Boost" drop-downs can be used to select active Windows Power Plan boost mode (separately for AC and battery power). This settings is extremely useful for Ryzen CPU, but even for Intel it provide a little performance boost (+3% at "Aggressive" for my gear).  

Additional "Fan curve" window at the right shows currently selected fan temperature/boost curve, as well as current boost.  

Fan control is temperature sensor-driven, so first select one of the temperature sensors. You can also change sensor name by double-click on it. In case you remove sensor name (leaving it empty), it will be restored back to default BIOS one.  
Then, select which fan(s) should react on its readings - you can select from none to all in any combination.  
So, select check box for fan(s) you need.

After you doing so, currently selected fan settings will be shown at "Fan Curve" window - you will see current fan boost at the top, and the fan control curve (green line).
The rest of the sensors controlling the same fan marked as yellow dotted lines.  
Big green and yellow points present proposed boost set for every curve, red one show current hardware boost settings.  
Now play with fan control curve - it defines fan boost by temperature level. X axle is temperature, Y axle is boost level.  
You can left click (and drag until release mouse button) into the curve window to add point or select close point (if any) and move it.  
You can click right mouse button at the graph point to remove it.  
Big red dot represent current boost and temperature position, yellow dots present current temperatures for other sensors involved into control of this fan.  

Please keep in mind:
- You can't remove first or last point of the curve.
- If you move first or last point, it will keep its temperature after button release - but you can set other boost level for it.
- Then fan controlled by more, then one sensor, boost will be set to the maximal value across them.  

"X" button above sensors list reset maximal temperature sensors value to current one.  
"X" button above fans list reset currently selected fan curve to default one (0-100 boost).

"Overboost" button will check possibility of currently selected fan to increase RPM even more, then default BIOS 100% boost.  
It will switch curve window to other mode - showing currently tested boost level and resulting RPM.  
This process can take some minutes, window with final results will be shown after the process ends, and they be used lately for this fan.  
You can press "Stop Overboost" button at any time to stop overboost check.

## Profiles

!["Profiles" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-profiles-6.png?raw=true)

`"Profiles"` tab control profile settings, like selecting default profile, per-profile monitoring control and automatic switch to this profile then the defined application run.  

Press "+" or "-" buttons to add or remove profile. New profile settings will be copied from currently selected profile.  
You can double-click or press Enter on selected profile into the list to edit its name.  

Each profile can have settings and application for trigger it. The settings are:
- "Effect mode" - Software effect mode for this profile: Monitoring, Ambient, Haptics, Off.
- "Default profile" - Default profile is the one used if "Profile auto switch" enabled, but running applications doesn't fit any other profile. There is can be only one Default profile, and it can't be deleted.
- "Priority profile" - If this flag enabled, this profile will be chosen upon others. Priority profile overrides "Only then active" setting of the other profiles. 
- "Dim lights" - Then profile activated, all lights will be dimmed.
- "Fan settings" - If selected, profile also keep fan control settings and restore it then activated.

"Device effects" button open profile settings for device effect (supported for some devices like RGB keyboards):  
![Device Effects](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui_deviceeffect.png?raw=true)  
At this dialog, you can select device supporting per-device effects and set it parameters - types, colors, tempo.  
APIv5 (per-key RGB notebook keyboards) support hardware effect, and APIv8 (external keyboards) support both hardware and key press effects.

The next block is "Triggers" - it define cases app should switch to this profile if "Profile auto switch" turned on at "Settings".
- "Keyboard" drop down will activate this profile in case corresponding key is pressed. Release the key to switch back to other profile.  
- "Power" block - if checked, profile will be activated if power source changed to checked state.  
- "Trigger applications" list define applications executable, which will activate selected profile. Press "+" button to select new application, or select one from the list and press "-" button to delete it.  
- "Only then active" - profile will be activated only in case of any application running and foreground (active) and have focus.

"Zones setting" block used operate with different setting blocks of the selected profile.
Check all types of zones you need to operate (colors, effect or fan settings), then press "Reset" button to remove it from selected profile, or "Copy active" button to copy it from active (selected in top drop down box) profile.

In case "Profile auto switch" turned on at "Settings", active profile will be selected automatically according to this rules:
- If any "Trigger application" from any profile running - "Default" profile selected.
- If any "Trigger application" from profile with "Priority" setting running - this profile will be selected.
- If any "Trigger application" from profile running, and it doesn't have "Only when active" trigger, this profile selected (random one of them if more, then one application found in many).
- If foreground application is one of the "Trigger application" of the profile with "Only when active" flag, and no other application belongs to profile with "Priority" flag running, this profile will be selected.
- Pressing a key or changing power source will always select first profile with this triggers active. It will stay active until other profile switch happened.

## Settings

!["Settings" tab](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/gui-settings-6.png?raw=true)

Global application and light system settings collected into this tab.

Application behavior control block at right:
- "Start with Windows" - Start application at Windows start. It will not work if application request run as admin level (see below) (default - off).
- "Check for update" - Enables online update check (default - on).
- "Start minimized" - Hide application window in system tray after start (default - off).
- "Disable AWCC" - Application will check active Alienware Control Center service at the each start and will try to stop it (and start back upon exit). It will require "Run as administrator" privilege (default - off).
- "Use BIOS sensors" - Read additional hardware data from Windows WMI and Alienware BIOS (if fan control enabled). It will require "Run as administrator" privilege (default - off).
- "Enable fan control" - Enables fan and power control functionality, as well as some additional (Alienware BIOS) temperature sensors. It will require "Run as administrator" privilege (default - off).

Light system control options at the left block:
- "Turn on lights" - Global light control system switch. All lights will be off and black if disabled (default - on).
  - "Keep Power/indicator on" - Power and Indicator lights stay on even if lights turned off (default - on).
  - "Turn off for battery" - Lights will turned off at battery power and back on at AC. (default - off). 
- "Lights follow screen state" - Dim/Fade to black lights then system screen dimmed/off (default - off).
- "Colour Gamma correction" - Enables colour correction to make them looks close to screen one. It keeps original LED colours if disabled (default - on).
- "Enable software effects" - Global software effect switch. If it's off, effects always disabled, otherwise effect mode defined by current profile (default - on).
- "Profile auto switch" - Switch between profiles according of their trigger applications start and finish (default - off).
  - "Do not switch for desktop" - Profile will not be changed in case start menu/tray/desktop activated by user (default - off).

Light dimming control at bottom:
- "Dim lights" - Dim system lights brightness to desired level (default - off).
  - "Dim Power/Indicator lights" - Power Button and Indicator lights will be dimmed as well - and have full brightness otherwise (default - off).
  - "Dim lights on battery" - Dim lights if system running at battery power, decreasing energy usage. Returns to full brightness if AC connected (default - on).
- "Dimming power" - Brightness decrease level for dimming. Values can be from 0 to 255, default is 92.

Interface settings:
- "Light names on grid" - Shows/hide light name printing at grid buttons.


## Keyboard shortcuts 

Global shortcuts (can be operated if application running):
- CTRL+SHIFT+F12 - enable/disable lights.
- CTRL+SHIFT+F11 - enable/disable light dimming.
- CTRL+SHIFT+F10 - enable/disable software effects.
- CTRL+SHIFT+F9 - enable/disable profile auto switch.
- CTRL+SHIFT+0..9 - switch active profile to profile #N (profile order is the same as at "Profiles" tab). "0" is switch to default profile.
- CTRL+ALT+0..5 - switch active power mode (in case fan control enabled). 0 is for Manual, 1..5 for system-defined.
- F18 (Fn+AlienFX) - cycle light mode (on-dim-off).
- F17 (Fn+G-key) - toggle G-Mode on and off ("Fan control" should be enabled).

Other shortcuts (operating then application active only):
- ALT+r - refresh all light colors
- ALT+m - minimize app window
- ALT+s - save configuration
- ALT+? - open this help page
- ALT+/ - about application
- ALT+x - quit

