@echo off
setlocal

call ..\setupenv.bat || exit /b 1

set CFLAGS=/nologo
set LDFLAGS=/subsystem:windows

call cl /Feintro.exe %CFLAGS% intro.c /link %LDFLAGS% user32.lib opengl32.lib gdi32.lib
