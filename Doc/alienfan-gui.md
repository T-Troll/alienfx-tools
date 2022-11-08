## `alienfan-gui`

![AlienFan-GUI](https://github.com/T-Troll/alienfx-tools/blob/master/Doc/img/alienfan.png?raw=true)

GUI application for fan control.  
Then you start it, you will see 2 main windows - Temperatures (with current and maximal (press "x" button at list head to reset it to current values) readings) and Fans (with check boxes and current RPM), as well as separate Fan curve (graph).  
Also, "Power mode" drop down available to select global power mode. Power modes are system-specific, so exact power limit values depends of you gear, you can use third-party tools to check it. You can edit mode name by selecting it and type new name.  
"G-Mode" check box enables and disables G-Mode (power boost) for supported hardware (all G-Series, some Alienware).
"GPU limit" slider define top maximal GPU power limit shift for some models. 0 (left) is no limit, 4 (right) - maximal limit. This feature only supported at some notebooks, and real limit can vary. You can use this value to keep more power at CPU or extend battery life.  
"CPU Boost" drop downs can be used to select active Windows Power Plan boost mode (separately for AC and battery power). This settings is extremely useful for Ryzen CPU, but even for Intel it provide a little performance boost (+3% at "Aggressive" for my gear).  

**Important:** Fans can only be controlled if Power Mode set to "Manual", and will be defined by BIOS value at other modes!

```
How it works
```

Fan control is temperature sensor-driven, so first select one of the temperature sensors. You can also change sensor name by double-click on it. In case you remove sensor name (leaving it empty), it will be restored back to default BIOS one.  
Then, choose fan(s) tied to it's reading - you can select from none to all in any combination.  
Select check box for fan(s) you need.

After you doing so, currently selected fan settings will be shown at "Fan Curve" window - you will see current fan boost at the top, and the fan control curve (green line).
The rest of the sensors controlling the same fan marked as yellow dotted lines.  
Big green and yellow points present proposed boost set for every curve, red one show current hardware boost settings.  
Now play with fan control curve - it defines fan boost by temperature level. X axle is temperature, Y axle is boost level.  
You can left click (and drag until mouse button release) into the curve window to add point or select close point (if any) and move it.  
You can click right mouse button at the graph point to remove it.  
Big red dot represent current boost and temperature position, yellow dots present current temperatures for other sensors involved into control of this fan.  

Please keep in mind:
- You can't remove first or last point of the curve.
- If you move first or last point, it will keep it's temperature after button release - but you can set other boost level for it.
- Then fan controlled by more, then one sensor, boost will be set to the maximal value across them. 

"X" button above sensors list reset maximal temperature sensors value to current one.  
"X" button above fans list reset currently selected fan curve to default one (0-100 boost).

"Overboost" button will check possibility of currently selected fan to increase RPM even more, then 100% boost.  
It will switch curve window to other mode - showing currently tested boost level and resulting RPM.  
This process can take some minutes, window with final results will be shown after the process ends, and they be used lately for this fan.  
You can press "Stop Overboost" button at any time to stop overboost check.

You can minimize application to tray pressing Minimize button (or the top one), left click on try icon restore application back, right click brings you the context menu for power mode/g-mode change.

There are some settings into application top menu under "Settings":
- "Start with windows" - Application will run at system start.
- "Start minimized" - Application hide into tray icon after start.
- "Keyboard shortcuts" - Global keyboard hotkeys enabled.
- "Check for update" - Application will made online update check at start.
- "Disable AWCC" - In case AWCC service running, it will be stopped and started back then application quit.

## Keyboard shortcuts

Global shortcuts (works all time application running):
- CTRL+ALT+0..N - switch active power mode. 0 is for Manual, 1..N for system-defined
- F17 (G-key for Dell G-series laptops) - Enable/disable G-Mode (performance mode)

