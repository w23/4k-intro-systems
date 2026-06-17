@echo off
setlocal

call ..\setupenv.bat || exit /b 1

set CFLAGS=/nologo /Zi /Z7 /Od /JMC
set LDFLAGS=/subsystem:windows /DEBUG

call cl /Feintro.exe %CFLAGS% intro.c /link %LDFLAGS% user32.lib opengl32.lib gdi32.lib
