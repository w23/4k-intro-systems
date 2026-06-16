@echo off
if defined VCINSTALLDIR exit /b 0

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"

if not defined VSPATH (
	echo Visual Studio with C++ tools not found.
	exit /b 1
)

call "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" x64_x86
exit /b %errorlevel%
