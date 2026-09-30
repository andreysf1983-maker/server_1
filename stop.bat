@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
title LegionForge - Stop all processes

REM  Stops everything the LegionForge kit started from this folder:
REM    WorldServer, bnetserver/authserver, portable MariaDB, the web
REM    Control Center (Node.js), running builds (cmake/MSBuild) and scripts.
REM  Processes outside this project folder are never touched.
REM
REM  Usage:  Stop.bat           graceful stop (clean MariaDB shutdown)
REM          Stop.bat /force    kill everything immediately

if not exist "%CD%\tools\bootstrap.ps1" (
  echo [ERROR] tools\bootstrap.ps1 is missing.
  pause
  exit /b 1
)

set "LF_FORCE="
if /I "%~1"=="/force" set "LF_FORCE=-Force"
if /I "%~1"=="-force" set "LF_FORCE=-Force"

echo ================================================================
echo  LegionForge - stopping all processes started by the kit
echo ================================================================
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%CD%\tools\bootstrap.ps1" -Mode Stop %LF_FORCE%
if errorlevel 1 (
  echo.
  echo [ERROR] Some processes could not be stopped. See the message above.
  echo If the server was started as administrator, run Stop.bat as administrator.
  pause
  exit /b 1
)
echo.
echo All LegionForge processes are stopped.
timeout /t 5 >nul
exit /b 0
