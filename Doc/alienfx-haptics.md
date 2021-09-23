# Alienfx-haptics

![alienfx-haptics](https://github.com/T-Troll/alienfx-tools/tree/master/Doc/img/haptics.png)

`alienfx-haptics` is a viualisation system for any sound playing - it can both use microphone or system sound device.  
You AlienFX lights can react to any sound the way to set them up!

## Usage
Run `alienfx-haptics.exe`. Set the colors for lights and it’s mapping to respond the frequency.  
Keep app running or minimize, then start player or game of choice.
```
How it works
```
This application get audio stream from default output or input device (you can select it at the top menu), then made a real-time spectrum analysis.  
After that, spectrum powers grouped into 20 groups using octave scale.  
For each light found into the system, you can define group(s) it should react, as well as color for Lowest Hi power level into frequency group. If more, then one group is selected, power will be calculated as a medium power level across of them.  
It's also possible to compress diapason if group always not so or so high powered - use low-level and high-level sliders. Low-level slider define minimum power to react (all below will be treated as zero), and Hi-level slider defines maximum level (all above will be treated as maximum).  
"Clear” button set all colors to black and sliders to default value.
"Gauge" checkbox - change behavour for groups only. If Gauge on, all lights in group works as a peak indicator (hi-color below power level, low-color above power level, mixed in between).
"Refresh” button rescan all lights into the system (it’s useful if you connect/disconnect new light device) and restart audio capture stream (in case you switch or remove audio device).  
"Remove" button remove all lights settings across all lights. Use with care!  
"Minimize" button (or top menu minimize) will hide application into the system tray. Left-click the tray icon to open it back, right-click it to close application.  
"Axis" checkbox enable axis lines and marks at sound visualisation window.