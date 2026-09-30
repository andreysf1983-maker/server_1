# LEGIONFORGE :: журнал исправлений

Что было сломано и что именно починено. Порядок — от первопричины к следствиям.

---

## 1. Все `.bat` имели LF-окончания строк (главная причина «не работают bat»)

**Симптом.** `START.bat`, `PANEL.bat`, `Stop.bat` открывались и сразу закрывались,
либо вели себя непредсказуемо: `goto` не находил метки, блоки `if ... ( ... )`
обрывались на середине.

**Причина.** В `.gitattributes` было единственное правило `* text=auto`. Git
нормализовал все текстовые файлы в LF, и при checkout на Windows пакетные файлы
тоже получали LF. `cmd.exe` не переносит LF в метках и многострочных блоках.

**Исправление.**
* В `.gitattributes` добавлены жёсткие правила `*.bat|*.cmd|*.ps1|*.psm1|*.psd1 text eol=crlf`,
  отдельные правила `eol=lf` для исходников ядра, веб-панели и SQL, и `binary`
  для исполняемых файлов и архивов.
* Все 20 Windows-скриптов в рабочем дереве переведены в CRLF.

> После применения `.gitattributes` выполните в репозитории
> `git add --renormalize .`, иначе Git продолжит хранить старые LF-версии.

---

## 2. Отсутствовал `project.json` — падал вообще любой лаунчер

**Симптом.** Любое обращение к `bootstrap.ps1` завершалось строкой
`project.json is missing from ...`.

**Причина.** `bootstrap.ps1` читает из манифеста все пути и версии инструментов
(`$Manifest.directories.*`, `$Manifest.toolchain.*`, `$Manifest.build.*`,
`$Manifest.project.clientBuild`) и бросает исключение, если файла нет. Файл
перечислен в `tools/launcher-manifest.sha256`, но в репозиторий не попал.

**Исправление.** Создан `project.json` со значениями, соответствующими реальной
раскладке репозитория:

| Поле | Значение |
|---|---|
| `project.clientBuild` | `26124` |
| `directories.serverSource` | `source` |
| `directories.serverRuntime` | `server/bin` |
| `directories.serverBuild` | `server/build` |
| `directories.downloads` | `cache/downloads` |
| `directories.logs` | `server/logs` |
| `toolchain.nodeMajor` | `20` |
| `toolchain.mariadbVersion` | `11.4.5` |
| `build.generator` | `Visual Studio 17 2022` |

---

## 3. Отсутствовал триггер `legionforge.bin`

**Симптом.** `PANEL.bat`: «Не найден триггер legionforge.bin — панель не может стартовать».

**Исправление.** Файл создан (`PANEL_PORT`, `PANEL_MODE`, `CLIENT_BUILD`, ...).
Дополнительно `PANEL.bat` больше не считает триггер обязательным: при его
отсутствии панель стартует на порту по умолчанию, а не завершается с ошибкой.

---

## 4. Login-сервер искали под именем `authserver`, а ядро собирает `bnetserver`

**Симптом.** Сборка считалась «не готовой» при каждом запуске, `START.bat` снова
предлагал компилировать; режим `Run` падал с «No compiled runtime exists»;
в статусе `compiled` всегда был `false`.

**Причина.** В Legion 7.3.5 login-сервер — это `bnetserver.exe`
(`source/src/server/bnetserver/`, `add_subdirectory(bnetserver)`). Имя
`authserver` осталось от старых веток и было зашито в
`PrepareServerConfig`, `StartServer`, `WriteStatus` и в проверку готовности `START.bat`.

**Исправление.** Добавлены функции `ResolveLoginServerBaseName`, которые
принимают `bnetserver` как основное имя и `authserver` как запасное.
`START.bat` проверяет оба варианта.

---

## 5. Панель собиралась не в том каталоге

**Симптом.** `PANEL.bat` падал на `npm ci` / `npm run build`.

**Причина.** `bootstrap.ps1 -Mode Panel` собирает и запускает панель **из корня
проекта** (проверяет `.\node_modules\next` и `.\.next\BUILD_ID`), а `PANEL.bat`
делал `pushd web_panel` и собирал там, где нет ни `node_modules`, ни
`package-lock.json`.

**Исправление.** Исходники панели (`web_panel/src`) перенесены в корневой `src/`,
конфиги — в корень проекта. `web_panel/` оставлен как зеркало и обновляется
скриптом `tools/sync_panel_root.bat` (направление — корень → `web_panel`).
`PANEL.bat` переписан под сборку в корне.

---

## 6. Два несовместимых набора путей к портативным утилитам

**Причина.** `download_tools.ps1` ставит Node.js в `tools\nodejs` и MySQL 8 в
`tools\mysql`, а `bootstrap.ps1` ждал `tools\node` и `tools\mariadb`.
Аналогично `bootstrap_env.bat` проверял Boost в `tools\boost\stage\lib`,
а `bootstrap.ps1` — в `tools\boost\boost_1_86_0\`.

**Исправление.** В `bootstrap.ps1` добавлены `ResolveNodeRoot`,
`ResolveDatabaseRoot`, `ResolveDatabaseServerExe`, `ResolveDatabaseClientExe`
и `GetDatabaseProcessNames` — принимаются обе раскладки
(`mariadbd.exe`/`mysqld.exe`, `mariadb.exe`/`mysql.exe`).
`bootstrap_env.bat` проверяет оба варианта и больше не качает уже установленное.

---

## 7. Конфиги сервера указывали на несуществующие базы данных

**Симптом.** `worldserver` не подключался к БД сразу после старта.

**Причина.** Три разные схемы в трёх местах:

| Где | Порт | Пользователь | Имена БД |
|---|---|---|---|
| `worldserver.conf` (было) | 3306 | `legionforge` / `root` | `auth`, `world`, `hotfixes`, `legionforge_characters` |
| `bnetserver.conf` (было) | 3306 | `root` | `legionforge_auth` |
| `bootstrap.ps1` создаёт | 3307 | `legion` | `legion_auth`, `legion_world`, `legion_characters`, `legion_hotfixes` |

**Исправление.** `worldserver.conf` и `bnetserver.conf` приведены к той схеме,
которую реально создаёт `bootstrap.ps1` (порт 3307, пользователь `legion`,
префикс `legion_`). `.env.example` панели приведён к тем же значениям.

---

## 8. Веб-панель: корень платформы определялся неверно

**Симптом.** Дашборд показывал 0 % готовности, хотя исходники, конфиги, SQL и
документация были на месте.

**Причина.** `src/lib/platform.ts` содержал
`PLATFORM_ROOT = path.join(process.cwd(), "LEGIONFORGE")` — раскладка, при
которой панель живёт в `/LEGIONFORGE/web_panel`. После переноса панели в корень
такого подкаталога нет, и все 9 проверок возвращали `false`.

**Исправление.** Добавлена функция `detectPlatformRoot()`, которая перебирает
`./LEGIONFORGE`, `..` и `.` и выбирает первый каталог, содержащий
`source/CMakeLists.txt` либо `server/configs/worldserver.conf`.
Путь можно задать явно переменной `LEGIONFORGE_ROOT`.

Также исправлена заглушка `countSourceFiles()`, которая всегда возвращала
`files: 0` — теперь рекурсивно считает файлы ядра и отдельно считает модули
`LegionForge_*.cpp`.

---

## Текущее состояние

Проверки дашборда (`GET /api/status`):

| Проверка | Статус |
|---|---|
| Исходники ядра `/source` | OK |
| Кастомные C++ модули (17 шт. `LegionForge_*.cpp`) | OK |
| Скомпилированные бинарники `/server/bin` | требуется сборка |
| Конфиги `/server/configs` | OK (730 ключей, `Game.Build.Version = 26124`) |
| Дампы БД `/sql/base` | OK |
| Кастомный SQL `/sql/custom` | OK |
| Карты и DBC `/server/data/maps` | требуется распаковать карты клиента |
| Портативные утилиты `/tools` | OK |
| Документация `/docs` | OK |

Готовность платформы: **78 %**. Остались два пункта, которые принципиально
нельзя получить из репозитория: скомпилированные бинарники (нужны Windows +
MSVC) и карты/DBC клиента 7.3.5 (извлекаются из игрового клиента).

Хранилище панели засеяно: 100 мировых боссов, 50 фолиантов, 38 позиций магазина,
81 исправленный квест, 320 ников ботов, 10 аккаунтов и 10 персонажей.

---

## Порядок запуска

```bat
git add --renormalize .        REM один раз, чтобы закрепить CRLF
START.bat                      REM среда -> исходники -> БД -> сборка -> запуск
PANEL.bat                      REM веб-панель: http://127.0.0.1:3000
Stop.bat                       REM остановить всё, что запустил комплект
```

Перед первым запуском положите:
1. дампы БД в `sql\base\` (`auth.sql`, `characters.sql`, `world.sql`, `hotfixes.sql`);
2. карты в `server\data\` (`cameras`, `dbc`, `db2`, `gt`, `maps`, `mmaps`, `vmaps`).
