# Dell LightFX emulator

DLL library, which simulates Dell LightFX v2 interfaces, but uses low-level SDK to control lights.  
It supports any SDK supported devices (any Alienware/G-series notebook, most desktops), except ACPI-controlled lights (some Aurora desktops).  
If you have any FightFX-enabled game or application, just copy DLL into path or into game/app folder to enable it.  
NB: You can use `alienfx-cli high-level` commands for test it as well.

## Usage

This library support 100% of LightFX v2 functions and data structures, but with some differences:
- Some LightFX bugs fixed - for example, light query works now, light positions retrieved correctly, if defined.
- **Grid position required** to map control IDs to light IDs names and position! So first you need to define devices and lights into `alienfx-gui` or `alienfx-cli probe`!
- Light zones and positions are mapped from light grids. Each grid divided to left-right-center-top-bottom blocks. "ALL" is the special group, containing all defined lights across all devices.
- Not only 3 default action types (color, pulse, breath), but also additional 4 types supported for APIv4.
- Light position only have 3 values for now - from 0 (leftmost, front one, bottom one) to 2 (rightmost, back one, top one). All lights put into the center (value 1) by default.

## Known issues
- Current light color can't be retrieved correctly (will be always zero). Hardware limitation.
- Default color set do the same like common one (but it's deprecated in Dell LightFX as well).
