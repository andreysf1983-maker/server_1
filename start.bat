@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
title LegionForge Developer Kit - Legion 7.3.5.26124

REM ==========================================================================
REM  LEGIONFORGE :: START.bat
REM  Полный цикл: портативная среда -> проверка исходников -> БД -> сборка -> запуск.
REM
REM  Исправлено:
REM   * добавлена проверка project.json (без него bootstrap.ps1 падал сразу);
REM   * путь к исходникам ядра: source\ (раньше печатался несуществующий server\source);
REM   * каталог бинарников: server\bin (раньше проверялся несуществующий server\runtime);
REM   * login-сервер 7.3.5 называется bnetserver.exe (раньше искали authserver.exe,
REM     из-за чего сборка никогда не считалась готовой и перезапускалась каждый раз);
REM   * корректная обработка отказа пользователя на каждом вопросе.
REM ==========================================================================

set "BOOTSTRAP=%CD%\tools\bootstrap.ps1"
if not exist "%BOOTSTRAP%" (
  echo [ERROR] tools\bootstrap.ps1 is missing.
  pause
  exit /b 1
)

if not exist "%CD%\project.json" (
  echo [ERROR] project.json is missing from the project root.
  echo         bootstrap.ps1 reads every path and tool version from it.
  pause
  exit /b 1
)

set "SRC_CMAKE=%CD%\source\CMakeLists.txt"
set "BIN_DIR=%CD%\server\bin"

echo ================================================================
echo  LegionForge Developer Kit - Legion 7.3.5.26124
echo ================================================================
echo  Step 1/4: Preparing portable tools and checking your server source.
echo           CMakeLists.txt must be at source\CMakeLists.txt
echo           (uses your locally installed Visual Studio, no winget)
echo ----------------------------------------------------------------
call :run Prepare
if errorlevel 1 goto :failed

if not exist "%SRC_CMAKE%" (
  echo.
  echo [ERROR] source\CMakeLists.txt was not found.
  echo         Put the LegionForgeCore 7.3.5 sources into the source\ folder.
  pause
  exit /b 1
)

REM --- Готова ли уже сборка? Проверяем оба возможных имени login-сервера. ---
set "LOGIN_EXE="
if exist "%BIN_DIR%\bnetserver.exe" set "LOGIN_EXE=%BIN_DIR%\bnetserver.exe"
if not defined LOGIN_EXE if exist "%BIN_DIR%\authserver.exe" set "LOGIN_EXE=%BIN_DIR%\authserver.exe"

if defined LOGIN_EXE (
  if exist "%BIN_DIR%\worldserver.exe" goto :ready
)

echo.
echo ================================================================
echo  No compiled server found yet.
echo  Release x64 build output goes to:  server\bin\
echo    - server\bin\bnetserver.exe
echo    - server\bin\worldserver.exe
echo ================================================================
set "DO_BUILD="
set /p "DO_BUILD=Build Release x64 now? [Y/N]: "
if /I not "!DO_BUILD!"=="Y" goto :panel
call :run Build
if errorlevel 1 goto :failed

:ready
echo.
echo ================================================================
echo  Compiled server is ready in:  %BIN_DIR%
echo  Start it later with tools\run_server.bat, or just re-run START.bat
echo  (it detects the build and skips straight to launch).
echo ================================================================
set "DO_RUN="
set /p "DO_RUN=Start the database, login server and WorldServer now? [Y/N]: "
if /I not "!DO_RUN!"=="Y" goto :panel
call :run Run
if errorlevel 1 goto :failed

:panel
echo.
set "DO_PANEL="
set /p "DO_PANEL=Open the LegionForge Control Center (web panel) now? [Y/N]: "
if /I "!DO_PANEL!"=="Y" (
  echo Launching the web panel at http://localhost:3000 ...
  powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%BOOTSTRAP%" -Mode Panel
)
echo.
echo Done. Build: server\bin  ^|  Panel: START_PANEL.bat or arguscore.bin.bat
echo Stop everything the kit started: Stop.bat
exit /b 0

:run
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%BOOTSTRAP%" -Mode %~1
if errorlevel 1 exit /b 1
exit /b 0

:failed
echo.
echo [ERROR] The requested action did not complete.
echo Completed downloads remain in cache\downloads.
echo Detailed logs (when applicable):
echo   server\logs\boost-install.log
echo   server\logs\boost-innoextract.log
echo   server\logs\mariadb.log
echo Re-run START.bat after reviewing the message above; completed steps are reused.
pause
exit /b 1
