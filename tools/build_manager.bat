@echo off
REM LEGIONFORGE :: tools/build_manager.bat - сборка GUI LegionForge_Manager.exe (.NET 8)
setlocal
set "LF_ROOT=%~1"
if "%LF_ROOT%"=="" set "LF_ROOT=%~dp0.."
for %%I in ("%LF_ROOT%") do set "LF_ROOT=%%~fI"
set "PATH=%LF_ROOT%\tools\dotnet;%PATH%"
set "DOTNET_CLI_TELEMETRY_OPTOUT=1"
echo    [manager] Публикую LegionForge.Manager (WPF, .NET 8) ...
"%LF_ROOT%\tools\dotnet\dotnet.exe" publish "%LF_ROOT%\tools\manager_src\LegionForge.Manager.csproj" ^
    -c Release -r win-x64 --self-contained false ^
    -o "%LF_ROOT%\server\bin" -p:PublishSingleFile=true >> "%LF_ROOT%\server\logs\manager_build.log" 2>&1
if errorlevel 1 ( echo    [manager] Ошибка сборки - см. server\logs\manager_build.log & exit /b 1 )
if exist "%LF_ROOT%\server\bin\LegionForge.Manager.exe" rename "%LF_ROOT%\server\bin\LegionForge.Manager.exe" "LegionForge_Manager.exe"
echo    [manager] Готово: server\bin\LegionForge_Manager.exe
exit /b 0
