@echo off
REM LEGIONFORGE :: tools/sync_panel_root.bat
REM Синхронизирует исходники веб-панели из корня проекта в /web_panel.
setlocal
set "LF_ROOT=%~dp0.."
for %%I in ("%LF_ROOT%") do set "LF_ROOT=%%~fI"
call "%LF_ROOT%\web_panel\sync_panel.bat"
endlocal & exit /b 0
