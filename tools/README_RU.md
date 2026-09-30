# tools

Это единая папка инструментов проекта LegionForge.

Файлы, хранящиеся в Git:

- `bootstrap.ps1` — общая логика Prepare, Build, Run, Panel и Status;
- `setup_env.bat` — обёртка Prepare;
- `compile_server.bat` — обёртка Build;
- `run_server.bat` — обёртка Run.

Папки, создаваемые автоматически и исключённые из Git:

- `git` — PortableGit;
- `cmake` — CMake;
- `node` — Node.js и npm;
- `mariadb` — автономная MariaDB;
- `boost` — предсобранный Boost 1.86.0 (msvc-14.3-64);
- `openssl` — портативный OpenSSL 3.5 LTS (x64);
- `innoextract` — портативный распаковщик (резервный путь установки Boost).
- Visual Studio используется локально установленный в системе (2022/2019).

Не переносите эти папки обратно в отдельный `toolchain/`. Для старой структуры bootstrap выполнит автоматическую миграцию при первом запуске.
