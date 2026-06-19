@echo off
setlocal

call ..\setupenv.bat || exit /b 1

cl /c /GS- intro.c || exit /b 2

..\sointu-compile.exe -o music -e h,asm -arch=386 music.yml || exit /b 3

REM fixup alignment
powershell -Command "(gc music.asm) -replace 'align=256', 'align=64' | Out-File -encoding ASCII music.asm" || exit /b 4

..\nasm.exe -fwin32 -o music.obj music.asm || exit /b 5

..\crinkler.exe ^
	intro.obj music.obj ^
	kernel32.lib user32.lib opengl32.lib gdi32.lib winmm.lib ^
	/subsystem:windows /OUT:intro.exe /CRINKLER
