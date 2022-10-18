@echo off
echo Usage: alienfx-config [command]
echo Commands are:
echo 	backup  - save registry config to files. 4 config file will be created into current folder.
echo 	restore - load config files back to registry.
echo 	delete  - remove all tools configuration from registry.

echo Doing %1...
if %1% EQU backup (
	reg export HKCU\Software\Alienfx_SDK afx-lights.reg
	reg export HKCU\Software\Alienfan afx-fans.reg
	reg export HKCU\Software\Alienfxgui afx-gui.reg
	reg export HKCU\Software\Alienfxmon afx-mon.reg
)
if %1% EQU restore (
	reg import afx-lights.reg
	reg import afx-fans.reg
	reg import afx-gui.reg
	reg import afx-mon.reg
)
if %1% EQU delete (
	reg delete HKCU\Software\Alienfx_SDK
	reg delete HKCU\Software\Alienfan
	reg delete HKCU\Software\Alienfxgui
	reg delete HKCU\Software\Alienfxmon
)
echo Done.