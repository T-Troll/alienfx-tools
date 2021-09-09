cd /d "C:\work\DevStudio\alienfan-tools\HwAcc" &msbuild "HwAcc.vcxproj" /t:sdvViewer /p:configuration="Release" /p:platform="x64" /p:SolutionDir="C:\work\DevStudio\alienfan-tools" 
exit %errorlevel% 