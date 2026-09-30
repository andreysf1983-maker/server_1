@echo off
REM ==========================================================================
REM  LEGIONFORGE :: PANEL.bat - запуск веб-панели администратора и конфигуратора
REM
REM  Исправлено:
REM   * панель собирается и стартует ИЗ КОРНЯ проекта - именно так работает
REM     bootstrap.ps1 -Mode Panel (он ищет node_modules\ и .next\BUILD_ID в корне).
REM     Старая версия уходила в web_panel\, где нет ни package-lock.json для
REM     "npm ci", ни node_modules, и сборка падала;
REM   * портативный Node.js ищется и в tools\node, и в tools\nodejs
REM     (download_tools.ps1 ставит его в tools\nodejs, bootstrap.ps1 - в tools\node);
REM   * триггер legionforge.bin больше не обязателен: если файла нет, панель
REM     стартует на порту по умолчанию, а не завершается с ошибкой;
REM   * sync_panel_root.bat теперь синхронизирует КОРЕНЬ -> web_panel,
REM     а не наоборот, поэтому исходники не затирались пустотой.
REM ==========================================================================
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
cd /d "%~dp0"
title LEGIONFORGE :: Web Panel
color 0A

set "LF_ROOT=%~dp0"
if "!LF_ROOT:~-1!"=="\" set "LF_ROOT=!LF_ROOT:~0,-1!"
set "LF_PORT=3000"
set "LF_MODE=production"
set "LF_BOOTSTRAP=!LF_ROOT!\tools\bootstrap.ps1"

echo.
echo   ====================================================================
echo    LEGIONFORGE :: ВЕБ-ПАНЕЛЬ АДМИНИСТРАТОРА И КОНФИГУРАТОР СБОРКИ
echo   ====================================================================
echo.

if not exist "!LF_BOOTSTRAP!" (
    echo [ОШИБКА] Не найден tools\bootstrap.ps1 - панель не может стартовать.
    pause
    exit /b 1
)

REM --- [1/4] Триггер legionforge.bin (необязательный) -----------------------
if exist "!LF_ROOT!\legionforge.bin" (
    echo [1/4] Читаю триггер legionforge.bin ...
    for /f "usebackq eol=# tokens=1,* delims==" %%A in ("!LF_ROOT!\legionforge.bin") do (
        if /I "%%A"=="PANEL_PORT" set "LF_PORT=%%B"
        if /I "%%A"=="PANEL_MODE" set "LF_MODE=%%B"
    )
) else (
    echo [1/4] Триггер legionforge.bin не найден - использую значения по умолчанию.
)
echo       Режим панели : !LF_MODE!
echo       Порт панели   : !LF_PORT!

REM --- [2/4] Портативный Node.js (tools\node или tools\nodejs) --------------
echo [2/4] Проверяю портативный Node.js ...
set "LF_NODE="
if exist "!LF_ROOT!\tools\node\node.exe"   set "LF_NODE=!LF_ROOT!\tools\node"
if not defined LF_NODE if exist "!LF_ROOT!\tools\nodejs\node.exe" set "LF_NODE=!LF_ROOT!\tools\nodejs"

if not defined LF_NODE (
    echo       Скачиваю Portable Node.js 20 LTS ...
    powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "!LF_ROOT!\tools\download_tools.ps1" -Only nodejs
    if exist "!LF_ROOT!\tools\nodejs\node.exe" set "LF_NODE=!LF_ROOT!\tools\nodejs"
)
if defined LF_NODE set "PATH=!LF_NODE!;!PATH!"

REM --- [3/4] Синхронизация web_panel и зависимостей -------------------------
echo [3/4] Синхронизирую исходники панели и устанавливаю зависимости ...
if exist "!LF_ROOT!\tools\sync_panel_root.bat" call "!LF_ROOT!\tools\sync_panel_root.bat"

if not exist "!LF_ROOT!\package.json" (
    echo [ОШИБКА] В корне проекта нет package.json - панель не собрать.
    pause
    exit /b 1
)

if not exist "!LF_ROOT!\node_modules\next" (
    echo       Устанавливаю зависимости панели ^(npm install^) ...
    call npm install --no-audit --no-fund
    if errorlevel 1 (
        echo [ОШИБКА] npm install завершился с ошибкой.
        pause
        exit /b 1
    )
) else (
    echo       node_modules уже установлены - пропускаю.
)

REM --- [4/4] Сборка и запуск -------------------------------------------------
if not exist "!LF_ROOT!\.next\BUILD_ID" (
    echo [4/4] Собираю панель ^(production build^) ...
    call npm run build
    if errorlevel 1 (
        echo [ОШИБКА] Сборка панели завершилась с ошибкой.
        pause
        exit /b 1
    )
) else (
    echo [4/4] Сборка уже есть - пропускаю.
)

echo.
echo   ====================================================================
echo    ПАНЕЛЬ РАБОТАЕТ:  http://127.0.0.1:!LF_PORT!
echo    Не закрывайте это окно - оно держит процесс панели.
echo    Остановить панель: нажмите Ctrl+C или закройте окно ^(или Stop.bat^).
echo   ====================================================================
echo.
start "" "http://127.0.0.1:!LF_PORT!"
set "PORT=!LF_PORT!"
call npm run start
endlocal
