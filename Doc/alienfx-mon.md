## `alienfan-gui`

![alienfx-mon](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/mon.png?raw=true)

GUI application for system monitoring and tray icons.  
There are 3 main zones at main application window:
- Sensors list
- Selected sensor settings
- Application settings

```
How it works
```

By default, only Windows Performance-based sensors are enabled.  
It includes CPU load in percent, Memory load in percent, HDD load in percent, current Battery level, current GPU load (between all GPUs in system) and CPU/GPU temperatures (on some systems).

You can enable 2 more sensor sources (but both of them will require Administrator rights):
- BIOS sensors. This includes additional temperature sensors, as well as power usage (total and per component) in Watt.
- Alienware sensors. The values come from Fan control subsystem and includes even more temperature sensors, system fans RPMS and the percent of maximal RPM (not precise at some systems).

You can edit sensor name by double-click on it into sensor list. Clearing name will reset it to default BIOS value.  
"Show hidden" check box switch list between active and hidden sensors.  
Pressing "Reset Min/Max" button set minimal and maximal values of all sensors to current value.

"Selected sensor settings" block present operations and settings for currently selected sensor:
- First line is about system tray icon.
  - "Show in tray" check box enables tray icon with sensor value for this sensor
  - Color button next to it defines what color will be used. Click it to change the color
  - "Inverted" check box define how to draw icon. In case it enabled, icon will be drown by selected color and transparent background. If disabled, transparent value will be drawn on selected color background.
- Second line is about alarm/highlight settings.
  - "Highlight" check box will change sensor tray icon color to red if triggered
  - Edit box defines border value to trigger it
  - "Then lower" check box alter trigger logic. It will alarm/highlight then value equal or higher then border value if off, and lower then border value if off.
- "Reset" button set selected sensor minimal/maximal value to current value
- "Hide"("Unhide" in "Show hidden" mode) button hides sensor from list and disables it's icon and alarm, or restore it back to active list.

In tray menu, you can left-click any sensor icon to bring application to front or right-click for context menu.  
Then you click at main application icon, selection at list will not changes, in case you click to sensor tray icon it will be selected into list.  
From context menu you can restore main app window, pause/resume sensor value updates, reset minimal and maximal values (for selected or all sensors if you click sensor or main icon) and quit application.  
Hover the mouse above icon reveal small tool tip - with the sensor name, minimal, maximal and current value.

Settings block at the right part of application window provide additional functionality:
- "Start with Windows" - application will be run at Windows start
- "Check for updates" - application will check GitHub for newer version at start
- "Start minimized" - application will hide to system tray after start
- "Refresh every ... ms" - pause between sensor information updates


