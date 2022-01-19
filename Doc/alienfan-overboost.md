## `alienfan-overboost`

This tool is a probe trying to determinate can you fan(s) be overboosted (setted more, then 100% of the BIOS boost value) and how much it can be overboosted.  
It set new boost value, wait until fan RPM will be steady, then check steady RPM against top reached.  
By default, just run it at IDLE system and wait some minutes until it define and store overboost values for every fan found into you system.  

You can also try to overboost only one fan, issue `alienfan-overboost <fan ID>` command, as well as set maximum boost manually using `alienfan-overboost <fan ID> <max. boost>` command.

NB: For my system is better run this tool at system idle state (it provide more precise results), but for some systems it can be better to run it at fully loaded system, running some bench app like Heaven Benchmark. Please keep in mind, full-auto test took some minutes, so you benchmark should be long enough for this.

**WARNING:** Stop all fan-control software before start this tool, or results will be incorrect!