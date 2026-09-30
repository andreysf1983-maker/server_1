@echo off
REM ==========================================================================
REM  LEGIONFORGE :: web_panel/sync_panel.bat
REM  Синхронизирует исходники панели из корня проекта в /web_panel и собирает
REM  production-билд. Вызывается автоматически из PANEL.bat.
REM ==========================================================================
setlocal
set "LF_ROOT=%~dp0.."
for %%I in ("%LF_ROOT%") do set "LF_ROOT=%%~fI"
set "PANEL=%LF_ROOT%\web_panel"
set "PATH=%LF_ROOT%\tools\nodejs;%PATH%"

echo   [sync] Копирую исходники панели в %PANEL% ...
for %%F in (package.json tsconfig.json next.config.ts postcss.config.mjs drizzle.config.json) do (
    if exist "%LF_ROOT%\%%F" copy /y "%LF_ROOT%\%%F" "%PANEL%\%%F" >nul
)
if exist "%LF_ROOT%\src" xcopy /y /i /s /q "%LF_ROOT%\src" "%PANEL%\src" >nul
if not exist "%PANEL%\.env" if exist "%PANEL%\.env.example" copy /y "%PANEL%\.env.example" "%PANEL%\.env" >nul

echo   [sync] Готово. Сборка выполняется из PANEL.bat.
endlocal & exit /b 0
