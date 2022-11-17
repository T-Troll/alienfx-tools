## `alienfan-cli`

It's a simple CLI fan control utility for now, providing the same functionality as AWCC (well... a bit more already).
ACPI calls can't control fans directly for modern gear (but can set direct fan percent at older one), so all you can do is set fan boost (Increase RPM above BIOS level).
Run `alienfan-cli [command[=value{,value}] [command...]]`. After start, it detect you gear and show number of fans, temperature sensors and power modes available.

Available commands:
- `rpm[=fanID]` - Show fan RPM(s) for all fans or for fan with this ID only.
- `percent[=fanID]` - Show current fan RPM(s) in percent of maximum (some systems have non-linear scale) for all fans or for fan with this ID only.
- `temp=[sensorID]` - Show known temperature sensor name and value for all sensors or for selected only.
- `unlock` - Enable manual fan control
- `getpower` - Print current power mode
- `setpower=<value>` - Set system-defined power level. Possible levels auto-detected from ACPI, see message at app start 
- `setgpu=<value>` - Set GPU power limit. Possible values from 0 (no limit) to 4 (max. limit).
- `setperf=<ac>,<dc>` - Set CPU Performance boost for AC and battery to desired level. Performance boost can be in 0..4 - disabled, enabled, aggressive, efficient, efficient aggressive.
- `getfans[=mode]` - Show current fan boost values.
- `setfans=<fan1>,<fan2>...[,mode]` - Set fans RPM boost level (0..100 - in percent). Fan1 is for CPU fan, Fan2 for GPU one. Number of arguments should be the same as number of fans application detect
- `setover[=fanID[,boost]]` - Set overboost for selected fan to boost (manual or auto)
- `setgmode=<mode>` - Switch G-mode (power boost) on and off for supported hardware. 1 is on, 0 is off.
- `gmode` - Show current G-mode status (on, off, error - if not supported).
- `dump` - List all avaliable Alienware methods (it's useful for new devices support)

`setover` command probe and set maximal possible overboost for fans (test more, then 100% of the BIOS boost value) and how much it can be overboosted.  
Without parameters, it check all fans into the system one-by one. With one parameter, it check only one fan defined by it. With two parameters, it set boost from second one and only check RPMs for it.

**For information:** For my system is better run this command at system idle state (it provide more precise results), but for some systems it can be better to run it at fully loaded system, running some bench app like Heaven Benchmark. Please keep in mind, full-auto test took some minutes, so you benchmark should be long enough for this.

**WARNING:** Stop all fan-control software before start this tool, or results will be incorrect!

`getfans` and `setfans` commands can work into 2 different modes. If mode is zero or absent, boost value will be calculated between 0 and 100% depends of the fan overboost. If mode is non-zero, raw boost value will be displayed/set. 
FanID and SensorID is a digit from 0 to fan/sensor count found into the system.

**Warning:** Setting Power level to non-zero value can disable manual fan control!  
**Warning:** color-related commands (for Aurora R6/R7 light control) temporary unavailable (do nothing)!