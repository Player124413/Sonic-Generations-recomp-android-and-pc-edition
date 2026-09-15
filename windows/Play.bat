@echo off
rem Sonic Generations (Windows) launcher.
rem   - Drag-and-drop your extracted game folder onto this file, or
rem   - put the game in a "game" folder next to this file, or
rem   - run: Play.bat "C:\path\to\game"
setlocal
rem Generations is a native 30 FPS title: cap the guest vblank so a 60/120Hz+
rem display doesn't burn CPU/GPU on frames the game was never tuned for.
set REX_FPS_CAP=30
if not "%~1"=="" (
    set "GAME_DIR=%~1"
) else (
    set "GAME_DIR=%~dp0game"
)
"%~dp0sonicgenerations.exe" --game_data_root="%GAME_DIR%"
