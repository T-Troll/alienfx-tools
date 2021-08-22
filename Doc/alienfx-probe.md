# alienfx-probe 

`alienfx-probe.exe` is a simple CLI interface for check all devices present into the system, assigning names for devices and lights for low-level DSK (similar to alienfx-led-tester, but with wider devices support).  
Use it if you don't like more sofisticated and flexible graphical interface from `alienfx-gui`.  

## Usage

Then run, it shows some info, switch lights to green one-by-one and prompt to enter devices and lights name.  
Then the name is set, light switched to blue. If you didn't see which light is changed, just press ENTER to skip it.  
It's check 23 first lights (or 136 for keyboard devices) by default, but you can change this value runnning `alienfx-probe.exe [number of lights]`.  
This names are stored and will be used in all application as a light names for UI.
