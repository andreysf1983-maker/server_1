# /server/bin — готовые исполняемые файлы

Сюда `tools/compile_server.bat` (вызывается из `START.bat`, шаг 3) копирует результат сборки:

| Файл | Назначение |
|---|---|
| `worldserver.exe` | Игровой мир (LegionForgeCore 7.3.5.26124) |
| `bnetserver.exe` | Battle.net-эмулятор (логин, список реалмов, BattlePay) |
| `LegionForge_Manager.exe` | GUI-менеджер (.NET 8 WPF): запуск, логи, авто-рестарт |
| `libmysql.dll`, `libcrypto-*.dll`, `libssl-*.dll` | Зависимости MySQL и OpenSSL |
| `mapextractor.exe`, `mmaps_generator.exe`, `vmap4extractor.exe` | Инструменты извлечения карт (опционально) |

Конфиги берутся из `../configs/` (`worldserver.conf`, `bnetserver.conf`),
данные карт — из `../data/`, логи пишутся в `../logs/`.

Если `worldserver.exe` отсутствует, `START.bat` предложит начать компиляцию.
