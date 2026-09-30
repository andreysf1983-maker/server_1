@echo off
setlocal EnableExtensions
cd /d "%~dp0\.."
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%CD%\tools\bootstrap.ps1" -Mode Build
if errorlevel 1 pause
