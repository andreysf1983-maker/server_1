@echo off
setlocal EnableExtensions
cd /d "%~dp0\.."
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%CD%\tools\bootstrap.ps1" -Mode Prepare
if errorlevel 1 pause
