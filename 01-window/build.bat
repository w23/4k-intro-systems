@echo off
setlocal

call ..\setupenv.bat || exit /b 1

cl /c intro.c

..\crinkler.exe ^
	intro.obj ^
	kernel32.lib user32.lib opengl32.lib gdi32.lib ^
	/subsystem:windows /OUT:intro.exe /CRINKLER
