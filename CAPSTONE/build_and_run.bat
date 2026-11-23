@echo off
REM Builds and runs Prison Escape using the standard raylib Windows installer layout (C:\raylib).
set PATH=C:\raylib\w64devkit\bin;%PATH%
gcc main.c -o prison.exe -O2 -Wall -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
if errorlevel 1 (echo BUILD FAILED & pause & exit /b 1)
prison.exe
