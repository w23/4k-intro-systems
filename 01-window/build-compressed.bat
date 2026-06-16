@echo off
setlocal

call ..\setupenv.bat || exit /b 1

set CFLAGS=/nologo /DCOMPRESSED
set LDFLAGS=/subsystem:windows /OUT:intro-compressed.exe /CRINKLER /REPORT:report.html

call cl %CFLAGS% intro.c
call ..\crinkler.exe %LDFLAGS% intro.obj kernel32.lib user32.lib opengl32.lib gdi32.lib
