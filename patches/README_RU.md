# /patches/ — ключевые пропатченные файлы ядра

`overlay/` повторяет структуру `/source/` и накладывается скриптом
`tools/apply_patches.ps1` при чистой распаковке `source.7z`:

* `src/common/LegionForgeVersion.h` — бренд и жёсткий билд 26124 (+ static_assert)
* `src/common/Define.h` — подключение заголовка версии
* `src/server/game/Entities/Item/Item.cpp` — свободная трансмогрификация
* `src/server/shared/Realm/RealmList.cpp`, `src/server/worldserver/Main.cpp` — билд 26124
* `src/server/scripts/ScriptLoader.cpp` — регистрация кастомных модулей
* `src/server/scripts/Custom/*` — все модули LEGIONFORGE

Обновить overlay из живого `/source`:
`tools/sync_overlay.bat`
