## `alienfan-gui`

![alienfan-gui](/Doc/img/alienfan.png?raw=true)

GUI application for fan control.  
Then you start it, you will see 2 main windows - Temperaturs (with current and maximal (press "x" button at list head to reset it to current values) readings) and Fans (with checkboxes and current RPM), as well as separate Fan curve (graph).  
Also, "Power mode" dropdown avaliable to select global power mode. Power modes are system-specific, so exact power limit valuses depends of you gear, you can use third-party tools to check it. You can edit mode name by selecting it and type new name.  
"GPU limit" slider define top maximal GPU power limit shift for some models. 0 (left) is no limit, 4 (right) - maximal limit. This feature only supported at some notebooks, and real limit can vary. You can use this value to keep more power at CPU or extend battery life.  
"CPU Boost" dropdowns can be used to select active Windows Power Plan boost mode (separately for AC and battery power). This settings is extremely useful for Ryzen CPU, but even for Intel it provide a little performance boost (+3% at "Aggressive" for my gear).  

**Important:** Fans can only be controled if Power Mode set to "Manual", and will be defined by BIOS value at other modes!

```
How it works
```

Fan control is temerature sensor-driven, so first select one of the temperature sensors.  
Then, choose fan(s) tied to it's reading - you can select from none to all in any combination.  
Select checkbox for fan(s) you need.

After you doing so, currently selected (checked or not) fan settings will be shown at "Fan Settings" window - you will see selected fan boost at the top, and the fan control curve (green lines).  
Selected fan curve presented in green color, rest of the fans attached to the same sensor marked as yellow.  
Now play with fan control curve - it defines fan boost by temperature level. X axle is temperature, Y axle is boost level.  
You can left click (and drag until mouse button release) into the curve window to add point or select close point (if any) and move it.  
You can click right mouse button at the graph point to remove it.  
Big red dot represent current boost and temperature position.  

Please keep in mind:
- You can't remove first or last point of the curve.
- If you move first or last point, it will keep it's temperature after button release - but you can set other boost level for it.
- Then fan controlled by more, then one sensor, boost will be set to the maximal value across them. 

"Reset" button reset currently selected fan curve to default one (0-100 boost).

You can minimize application to tray pressing Minimize button (or the top one), left click on try icon restore application back, right click will close application.
There are two settings into application top menu under "Settings":
- "Start with windows" - If checked, application will run at system start.
- "Start minimized" - If checked, application hide into tray icon after start.