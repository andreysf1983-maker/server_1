@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
title LegionForge Build Panel (Control Center)

REM  arguscore.bin - launches the LegionForge build/control panel.
REM  The panel is the local Next.js "Control Center": configurator, shop
REM  editor, world-boss manager, module catalog and database tools.
REM  On first launch it also prepares the portable toolchain and builds the
REM  panel (this can take a while). Every later launch starts instantly.

if not exist "%CD%\tools\bootstrap.ps1" (
  echo [ERROR] tools\bootstrap.ps1 is missing. Run START.bat from the project root first.
  pause
  exit /b 1
)

echo ================================================================
echo  LegionForge Build Panel
echo  Opening the Control Center at http://localhost:3000
echo  (first run downloads portable tools and builds the panel)
echo ================================================================

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%CD%\tools\bootstrap.ps1" -Mode Panel
if errorlevel 1 (
  echo.
  echo [ERROR] The build panel could not be started.
  echo Make sure portable Node.js was installed by START.bat, then try again.
  pause
  exit /b 1
)
exit /b 0
