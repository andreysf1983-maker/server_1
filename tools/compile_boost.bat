@echo off
REM LEGIONFORGE :: tools/compile_boost.bat - сборка нужных библиотек Boost
setlocal
set "LF_TOOLS=%~dp0"
set "BOOST_SRC=%LF_TOOLS%boost\src"
set "BOOST_STAGE=%LF_TOOLS%boost\stage"
if not exist "%BOOST_SRC%\bootstrap.bat" ( echo [boost] Исходники Boost не найдены & exit /b 1 )
if not exist "%BOOST_STAGE%" mkdir "%BOOST_STAGE%"
pushd "%BOOST_SRC%"
call bootstrap.bat
b2 --with-system --with-filesystem --with-program_options --with-iostreams --with-thread ^
   --with-regex --with-date_time link=static runtime-link=shared threading=multi ^
   address-model=64 --stagedir="%BOOST_STAGE%" stage
popd
echo [boost] Готово: %BOOST_STAGE%\lib
endlocal & exit /b 0
