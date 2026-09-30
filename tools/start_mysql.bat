@echo off
REM LEGIONFORGE :: tools/start_mysql.bat - инициализация и запуск портативного MySQL
setlocal
set "LF_ROOT=%~dp0.."
for %%I in ("%LF_ROOT%") do set "LF_ROOT=%%~fI"
set "MYSQL_HOME=%LF_ROOT%\tools\mysql"
if not exist "%MYSQL_HOME%\my.ini" copy /y "%LF_ROOT%\tools\my.ini.template" "%MYSQL_HOME%\my.ini" >nul
if not exist "%MYSQL_HOME%\data\mysql" (
    echo        Инициализирую каталог данных MySQL ...
    "%MYSQL_HOME%\bin\mysqld.exe" --defaults-file="%MYSQL_HOME%\my.ini" --initialize-insecure --console
)
echo        Запускаю mysqld (порт 3306, root/legionforge) ...
start "LEGIONFORGE MySQL" /min "%MYSQL_HOME%\bin\mysqld.exe" --defaults-file="%MYSQL_HOME%\my.ini" --console
timeout /t 5 /nobreak >nul
"%MYSQL_HOME%\bin\mysql.exe" --host=127.0.0.1 --port=3306 --user=root -e "ALTER USER 'root'@'localhost' IDENTIFIED BY 'legionforge'; FLUSH PRIVILEGES;" >nul 2>&1
endlocal & exit /b 0
