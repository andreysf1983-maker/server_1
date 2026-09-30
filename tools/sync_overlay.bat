@echo off
REM LEGIONFORGE :: tools/sync_overlay.bat - обновить /patches/overlay из /source
setlocal
set "LF_ROOT=%~dp0.."
for %%I in ("%LF_ROOT%") do set "LF_ROOT=%%~fI"
set "SRC=%LF_ROOT%\source"
set "OVL=%LF_ROOT%\patches\overlay"
for %%F in (
  "src\common\LegionForgeVersion.h"
  "src\common\Define.h"
  "src\server\game\Entities\Item\Item.cpp"
  "src\server\shared\Realm\RealmList.cpp"
  "src\server\worldserver\Main.cpp"
  "src\server\scripts\ScriptLoader.cpp"
) do (
  if exist "%SRC%\%%~F" (
    if not exist "%OVL%\%%~dpF" mkdir "%OVL%\%%~dpF"
    copy /y "%SRC%\%%~F" "%OVL%\%%~F" >nul
    echo   [overlay] %%~F
  )
)
xcopy /y /i /s "%SRC%\src\server\scripts\Custom\LegionForge_*" "%OVL%\src\server\scripts\Custom\" >nul
echo   [OK] Overlay обновлён.
endlocal
