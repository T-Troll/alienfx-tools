# Dell LightFX emulator

DLL library, which simulates Dell LightFX v2 interfaces, but uses low-level SDK to control lights.  
It supports any hardware SDK support (any Alienware/G-series notebook, most desktops).  
If you have any FightFX-enabled game or application, just copy DLL into path or into game/app folder to enable it.  
NB: You can use `alienfx-cli high-level` commands for test as well.

## Usage

This library support 100% of LightFX v2 functions and data structures, but with some differences:
- Some LightFX bugs fixed - for example, light query works now.
- **Mappings required** to map control IDs to light IDs and names! So first you need to define devices and lights into `alienfx-gui` or `alienfx-probe`!
- Light zones are mapped by light groups. Create corresponded group into `alienfx-gui` to define light zone - by naming group as below (case sensitive!) and add corresponding lights into it! Supported zones are:
  - Right
  - Left
  - Upper
  - Lower
  - Front
  - Rear
  - ALL (doens't need separate group, set all defined lights across all devices)
- No only 3 default action types (color, pulse, breath), but also additional 4 types supported for APIv4.

## Known issues
- Current light state and it's posiion can't be retrieved correctly for now (will be always zero).
- Default color set do the same like common one (but it's deprecated in LightFX). I plan to store color as default later on.