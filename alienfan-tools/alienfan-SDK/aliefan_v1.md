# Fan control tools with Alienfan V1 support

This set of utilities build against older V1 (Direct ACPI access) interface.  
This can be useful for AMD users with unsupported Intel sensor format into BIOS, as well as for some G-series owners (it use custom G-mode with highter RPM for some systems).

## Preparation

To make this version working at Windows 11:
- Open `regedit`
- Navigate to `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\CI\Config`
- Add/edit DWORD key named `VulnerableDriverBlocklistEnable` to value `0`
- Reboot

## Installation

- Download common release, install/unpack.
- Unpack this archive into the same folder, replacing files.
- Download release [6.4.3.2](https://github.com/T-Troll/alienfx-tools/releases/tag/6.4.3.2) .zip file.
- Unpack `kdl.dll` and `hwacc.sys` into the same folder as tools before.
- Run tools.

## Known issues

- Antivirus will complain about this files (in fact, about `kdl.dll`). Add it to exception list. Here are the [explanation why](https://github.com/T-Troll/alienfx-tools/wiki/Why-antivirus-complain-about-some-alienfx-tools-components%3F).
