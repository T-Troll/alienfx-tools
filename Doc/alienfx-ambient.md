# alienfx-ambient 

![alienfx-ambient](https://github.com/T-Troll/alienfx-tools/tree/master/Doc/img/ambient.png)

`alienfx-ambient` is an ambient color visualisator.  
The screen is divided at 12 zones, and you can set up any of AlienFX lights follow the ambient color of the selected zones.  
It's useful to provide background ambient lights for a movie or a game.


## Usage
Run `alienfx-ambient.exe`. At first launch, set screen zones mapping to lights and parameters.  
Keep it running or minimize, then start video player or game of choice.
```
How it works
```
This application get shot of screen (primary or secondary), then divide it to several zones.  
For each zone, dominant color calculated (you can see it at the button in app interface).  
For each light found into the system, you can define zone(s) it should follow. If more, then one zone selected for light, it will try to blend zone colors into one.  
You can also select which screen to grab - primary or secondary, if you have more, then one. You can also press "Reset Devices and Capture" button to re-initialize screen capturing and lights list.  
"Quality" slider defines how many pixels will be skipped at analysis - working with full-screen image is very slow. Increasing this value decrease CPU load, but decrease dominant color extraction precision as well. Default value is 16, ok for i7 CPU, you can decrease it if lights updates are slow (or CPU usage is so high), or increase if it works ok for you.  
"Dimming" slider decreases the overall lights brightness - use it for better fit you current monitor brightness.  
"Gamma Correction" checkbox enables visual color gamma correction, make them more close to screen one.  
”Restart devices and capture” button is used to refresh light list according to devices found into the systems, as well as restart screen capture process.  
”Minimize” button (or top menu minimize) will hide application into the system tray. Left-click the tray icon to open it back, right-click it to close application.  
