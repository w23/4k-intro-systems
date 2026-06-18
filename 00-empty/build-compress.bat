@echo off
setlocal

call ..\setupenv.bat || exit /b 1

cl empty-nocrt.c

..\crinkler.exe ^
	empty-nocrt.obj kernel32.lib user32.lib ^
	/subsystem:windows /OUT:compressed.exe /CRINKLER
