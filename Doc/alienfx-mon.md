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
- ESIF sensors. This includes additional temperature sensors, as well as power usage (total and per component) in Watt.
- BIOS sensors. The values come from AlienFan subsystem and includes even more temperature sensors, system fans RPMS and the percent of maximal RPM (not precise at some systems).

You can select sensor from the list, edit it's name by double-click or enable/disable sensor presentation at system tray and background color for it.  
You can hide sensor from active list selecting "Hidden" check box, and switch list between active and hidden sensors selecting "Show hidden".
Pressing "Reset Min/Max" button set minimal and maximal values of all sensors to current value.

In tray menu, you can left-click any sensor icon to bring application to front or right-click to quit application.  
Hover the mouse above icon reveal small tool tip - with the sensor name, minimal, maximal and current value.

Settings block at the right part of application window provide additional functionality:
- "Start with Windows" - application will be run at Windows start
- "Check for updates" - application will check GitHub for newer version at start
- "Start minimized" - application will hide to system tray after start
- "Refresh every..." - pause between sensor information updates


