@echo off
setlocal

call ..\setupenv.bat || exit /b 1

cl ^
	/Feintro-debug.exe ^
	/Z7 /DEBUG /Od /JMC /D_DEBUG ^
	intro.c ^
	/link ^
		user32.lib opengl32.lib gdi32.lib ^
		/subsystem:windows /DEBUG
