## `alienfan-cli`

It's a simple CLI fan control utility for now, providing the same functionality as AWCC (well... a bit more already).
ACPI calls can't control fans directly for modern gear (but can set direct fan percent at older one), so all you can do is set fan boost (Increase RPM above BIOS level).
Run `alienfan-cli [command[=value{,value}] [command...]]`. After start, it detect you gear and show number of fans, temperature sensors and power modes available.

Available commands:
- `rpm[=fanID]` - Show fan RPM(s) for all fans or for this ID.
- `maxrpm[=fanID]` - Show BIOS-defined maximal RPM(s) for all fans or for this ID.
- `percent[=fanID]` - Show current fan RPM(s) in percent of maximum for all fans or for this ID only.
- `temp=[sensorID]` - Show known temperature sensor name and value for all sensors or for this ID only.
- `unlock` - Enable manual fan control
- `getpower` - Print current power mode
- `setpower=<value>` - Set system-defined power level. Possible levels auto-detected from ACPI.
- `setgpu=<value>` - Set GPU power limit. Possible values from 0 (no limit) to 4 (max. limit).
- `setperf=<ac>,<dc>` - Set CPU Performance boost for AC and battery to desired level. Performance boost can be in 0..4 - disabled, enabled, aggressive, efficient, efficient aggressive.
- `getfans[=[fanID,]mode]` - Print current fan boost value for all fans or for this ID only.
- `setfans=<fan1>,<fan2>...[,mode]` - Set fans RPM boost level (0..100 - in percent). Fan1 is for CPU fan, Fan2 for GPU one. Number of arguments should be the same as number of fans application detect
- `setover[=fanID[,boost]]` - Set maximal boost level for selected fan (manual or auto).
- `setgmode=<mode>` - Switch G-mode (power boost) on and off for supported hardware. 1 is on, 0 is off.
- `gmode` - Show current G-mode status (on, off, error - if not supported).
- `gettcc` - Show current CPU TCC offset level.
- `settcc=<value>` - Set CPU TCC offset to desired level (celsius degree).
- `getxmp` - Show selected memory XMP profile (0 - disabled).
- `setxmp` - Set memory XMP profile (0..2, 0 is disable)/
- `dump` - List all available Alienware methods (it's useful for new devices support).

In case you system have ACPI light control, 2 more commands available:
- `setcolor=<id>,r,g,b` - Set light with this ID to selected color. Id's depends of the system, try from 0 to 4.
- `setbrightness=<brightness>` - Set lights hardware brightness. Valid range is from, 0 (off) to 15 (full).

FanID and SensorID is a digit from 0 to fan/sensor count found into the system.

`getfans` and `setfans` commands can work into different modes. By default, boost value will be calculated depends of the max. fan RPMs. If mode is "raw", raw boost value from BIOS will be displayed/set.

`setover` command probe and set maximal possible boost for fans, if possible to set it above 100%, as well as detect maximal RPM for fan.
Without parameters, it check all fans into the system one-by one. With one parameter, it check only one fan defined by it. With two parameters, it set boost from second one and only check RPMs for it.  
**For information:** For my system is better run this command at system idle state (it provide more precise results), but for some systems it can be better to run it at fully loaded system, running some bench app like Heaven Benchmark. Please keep in mind, full-auto test took some minutes, so you benchmark should be long enough for this.

**WARNING:** Stop all fan-control software before start this tool, or results will be incorrect!  
**WARNING:** Setting Power level to non-zero value can disable manual fan control!  
