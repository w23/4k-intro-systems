@echo off
setlocal

call ..\setupenv.bat || exit /b 1

cl /Feempty.exe empty.c /link /subsystem:windows
