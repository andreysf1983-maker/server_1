@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
title LegionForge Control Center

if not exist "%CD%\tools\bootstrap.ps1" (
  echo [ERROR] tools\bootstrap.ps1 is missing.
  pause
  exit /b 1
)

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%CD%\tools\bootstrap.ps1" -Mode Panel
if errorlevel 1 (
  echo [ERROR] Control Center could not be started.
  pause
  exit /b 1
)
