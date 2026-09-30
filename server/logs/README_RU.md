# /server/logs — логи и краш-дампы

| Файл | Источник |
|---|---|
| `WorldServer.log` / `WorldServer.log.*` | worldserver.exe (Appender из worldserver.conf) |
| `BNetServer.log` | bnetserver.exe |
| `MySQL.log` | портативный mysqld |
| `compile.log` | CMake + Ninja (tools/compile_server.bat) |
| `bootstrap.log` | подготовка среды и распаковка source.7z |
| `manager_build.log` | сборка LegionForge_Manager.exe |
| `*.dmp` | краш-дампы ядра (MiniDumpWriteDump) |

`LogsDir = "../logs"` задан в `server/configs/worldserver.conf` и `bnetserver.conf`.
