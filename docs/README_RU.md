# ⚒ LEGIONFORGE — автономная серверная платформа World of Warcraft: Legion 7.3.5 (Build 26124)

> Полностью портативная, white-label платформа: ядро C++, умные лаунчеры, GUI-менеджер,
> веб-панель администратора и конфигуратор сборки, кастомный геймплей и экономика
> на родной валюте Легиона — **Сущность пробуждения (Currency ID 1533)**.

| Параметр | Значение |
|---|---|
| Версия клиента | **7.3.5** |
| Build | **26124** (жёстко зафиксирован в ядре и в `auth.realmlist.gamebuild`) |
| Ядро | `LegionForgeCore` (ребрендинг выполнен во всех 3500+ файлах исходников) |
| Платформа | LEGIONFORGE v3.1.0 |
| Валюта экономики | Сущность пробуждения (1533) |
| Кастомные мировые боссы | 100 шт. (entry 900001–900100) |
| Фолианты забытых способностей | 50 шт. (item 950200–950249) |
| Пул ников ИИ-ботов | 320+ |
| Веб-панель | Next.js 16 + PostgreSQL (Drizzle ORM), http://127.0.0.1:3000 |

---

## 1. Структура проекта

```text
/LEGIONFORGE/
├── START.bat                <- умный запускатор: установка + компиляция + старт
├── PANEL.bat                <- запуск веб-панели и конфигуратора сборки
├── Stop.bat                 <- корректная остановка всех процессов
├── legionforge.bin          <- KEY=VALUE триггер панели (сигнатура LFBIN1)
├── source.7z                <- оригинальный архив исходников ядра (для чистой установки)
│
├── /tools/                  <- 100% PORTABLE утилиты (скачиваются автоматически)
│   ├── /git/  /cmake/  /dotnet/  /nodejs/  /mysql/  /openssl/  /boost/  /7zip/
│   ├── bootstrap_env.bat    <- проверка среды
│   ├── download_tools.ps1   <- авто-загрузка портативных утилит
│   ├── compile_server.bat   <- CMake + Ninja -> Release x64 -> /server/bin
│   ├── build_manager.bat    <- сборка LegionForge_Manager.exe (.NET 8 WPF)
│   ├── apply_patches.ps1    <- наложение overlay-патчей и фиксация билда 26124
│   ├── rebrand_source.ps1   <- ребрендинг под Windows (защита trinity_string)
│   ├── start_mysql.bat      <- инициализация и запуск портативного MySQL
│   ├── run_server.bat       <- консольный запуск bnetserver + worldserver
│   ├── compile_boost.bat    <- сборка нужных библиотек Boost
│   ├── generate_custom_sql.mjs <- генератор кастомного SQL (боссы/боты/магазин)
│   ├── my.ini.template      <- преднастроенный конфиг MySQL
│   └── /manager_src/        <- исходники GUI (WPF, .NET 8)
│
├── /source/                 <- распакованные и модернизированные исходники ядра
│   └── /src/server/scripts/Custom/   <- ВСЕ кастомные C++ моды
│
├── /server/                 <- готовый скомпилированный сервер
│   ├── /bin/                <- worldserver.exe, bnetserver.exe, LegionForge_Manager.exe, .dll
│   ├── /configs/            <- worldserver.conf, bnetserver.conf, modules.conf
│   ├── /data/               <- dbc, db2, maps, vmaps, mmaps, gt, cameras
│   ├── /logs/               <- логи и краш-дампы
│   └── /backups/            <- бэкапы БД (создаёт GUI-менеджер)
│
├── /sql/
│   ├── /base/               <- СЮДА положите ваши дампы: auth.sql, characters.sql,
│   │                           world.sql, hotfixes.sql (7.3.5.26124)
│   └── /custom/             <- custom_legionforge.sql (+ _auth.sql, + _characters.sql)
│
├── /web_panel/              <- веб-панель администратора и конфигуратор
├── /patches/overlay/        <- ключевые пропатченные файлы ядра
└── /docs/                   <- README_RU.md (этот файл), PROMO_TEXT.txt
```

### Куда положить ваши файлы с локального компьютера

| Что | Куда |
|---|---|
| `auth.sql` | `/LEGIONFORGE/sql/base/auth.sql` |
| `characters.sql` | `/LEGIONFORGE/sql/base/characters.sql` |
| `world.sql` | `/LEGIONFORGE/sql/base/world.sql` |
| `hotfixes.sql` | `/LEGIONFORGE/sql/base/hotfixes.sql` |
| `cameras, dbc, gt, maps, mmaps, vmaps` | `/LEGIONFORGE/server/data/<имя папки>` |

После этого достаточно запустить `START.bat` — он сам создаст базы, зальёт дампы,
применит кастомный контент и проверит, что `realmlist.gamebuild = 26124`.

---

## 2. Первый запуск (START.bat)

1. Распакуйте проект в любую папку **без пробелов и кириллицы** (например `D:\LEGIONFORGE`).
2. Положите дампы БД в `/sql/base/` и карты в `/server/data/` (см. таблицу выше).
3. Запустите `START.bat`.

Что делает скрипт:

**Шаг 1 — среда.** Проверяет `/tools/` (Git, CMake+Ninja, .NET 8, Node.js, MySQL 8,
OpenSSL, Boost, 7-Zip). Чего не хватает — скачивает `download_tools.ps1` и распаковывает.
Отдельно ищет MSVC (Visual Studio Build Tools) через `vswhere.exe`.

**Шаг 2 — исходники и БД.** Если `/source/` пуста — распаковывает `source.7z`, накладывает
патчи (`apply_patches.ps1`) и выполняет ребрендинг (`rebrand_source.ps1`). Запускает
портативный MySQL, создаёт 4 базы (`legionforge_auth`, `legionforge_characters`,
`legionforge_world`, `legionforge_hotfixes`), заливает дампы и кастомный SQL, печатает
контрольный `SELECT id, name, gamebuild FROM realmlist;`.

**Шаг 3 — маршрутизация.**
* Если `worldserver.exe`/`bnetserver.exe` **нет** → вопрос `«Начать компиляцию сейчас? [Y/N]»`.
  При `Y`: CMake (Ninja, Release x64) → сборка → копирование `.exe`/`.dll` в `/server/bin`
  и конфигов в `/server/configs`.
* Если бинарники **уже есть** → меню:
  * `[1]` Запустить сервер через `LegionForge_Manager.exe` (**авто-выбор через 5 секунд**)
  * `[2]` Перекомпилировать ядро
  * `[3]` Обновить только базу данных (`custom_legionforge.sql`)
  * `[4]` Перезалить все базы с нуля
  * `[5]` Выйти

---

## 3. Веб-панель (PANEL.bat + legionforge.bin)

`PANEL.bat` читает триггер `legionforge.bin` (порт, режим), проверяет портативный Node.js
в `/tools/nodejs/`, при необходимости доустанавливает зависимости, выполняет production-сборку
`/web_panel`, запускает локальный сервер и открывает браузер. Консоль остаётся открытой и
показывает статус — закрыли окно, панель остановилась.

Возможности панели:

* **Дашборд** — статус процессов, онлайн, аккаунты, персонажи, проверка `gamebuild = 26124`,
  график онлайна, журнал выдачи Сущности пробуждения.
* **Конфигуратор сборки** — визуальное редактирование `worldserver.conf` и `modules.conf`:
  рейты, имя реалма, MOTD, стартовое сообщение в чате, включение/выключение всех кастомных
  модулей, цены магазина, потолок ilvl, бонус за онлайн.
* **Магазин BattlePay (кнопка «W»)** — редактор каталога: категория, предмет, количество,
  цена в Сущности пробуждения, экспорт в SQL.
* **Мировые боссы** — 100 записей: имя, зона, уровень, ранг, фазы, лут Сущности, респаун.
* **Фолианты** — список забытых способностей с проверкой класса.
* **Игроки** — аккаунты и персонажи, выдача валюты, GM-уровень, престиж/hardcore.
* **Файлы** — дерево всей платформы, просмотр и скачивание любого файла, выгрузка архивом.
* **Документация и промо** — этот README и `PROMO_TEXT.txt` в красивом виде.

Панель работает с PostgreSQL (Drizzle ORM) как со своим хранилищем настроек и зеркалом
игровых данных; экспорт в SQL даёт готовые файлы для импорта в мир-базу MySQL.

---

## 4. Кастомные системы (C++ и SQL)

Все модули лежат в `/source/src/server/scripts/Custom/` и регистрируются одной функцией
`AddSC_LegionForge_Custom()` из `ScriptLoader.cpp` (вызывается в `AddCustomScripts()`).

| Файл | Что делает |
|---|---|
| `LegionForge_Config.h` | Единые константы платформы (валюта 1533, ID предметов/NPC, потолок ilvl) |
| `LegionForge_Loader.cpp` | Реестр всех кастомных модулей |
| `LegionForge_OnlineReward.cpp` | +50 Сущности пробуждения каждый час (без AFK) |
| `LegionForge_ItemUpgrade.cpp` | «Концентрат силы Титанов» (+5 ilvl легендарки, потолок 1200), «Печать Вечности» (985 → 1000) |
| `LegionForge_ItemTome.cpp` | Обработчик фолиантов (learn-on-use, проверка класса) |
| `LegionForge_LegacySpells.cpp` | 50 возвращённых способностей WotLK/Cata/MoP/WoD, загрузка из БД |
| `LegionForge_TransmogFreedom.cpp` | Свободная трансмогрификация + NPC «Хранитель Иллюзий» + GM-команды |
| `LegionForge_BattlePayEssence.cpp` | Магазин кнопки «W» за Сущность + NPC «Хранитель Кузни» (зеркало каталога) |
| `LegionForge_WorldBosses.cpp` | Универсальный AI 100 мировых боссов: фазы, лужи, слуги, энрейдж, масштаб под группу 2–10 |
| `LegionForge_WorldBoss.cpp` | Исходный модуль анонсов мировых боссов (наработки репозитория) |
| `LegionForge_Bots.cpp` | Пул 320+ ников, живой чат (120–240 сек), прогулки, роли |
| `LegionForge_PlayerBots.cpp` | ИИ-напарник (компаньон-бот, автозаполнение PvP) |
| `LegionForge_BrokenQuests.cpp` | Авто-выполнение проблемных квестов + компенсация 25 Сущности |
| `LegionForge_Crossfaction.cpp` | Межфракционные группы, гильдии, чат, торговля |
| `LegionForge_QoL.cpp` | Mythic+ QoL (обмен ключа, телепорт группы), Duel Reset, мульти-профессии (до 4), смена расовых |
| `LegionForge_Prestige.cpp` | Престиж (до 5 рангов, +10% Сущности за ранг) и Hardcore «Одна жизнь» |
| `LegionForge_VendorNPC.cpp` | Единый сервисный NPC (улучшение, реагенты, услуги) |
| `LegionForge_CatalogMods.cpp` | Каталог GitHub-модов (Solocraft, 1v1, парagon и др.) |
| `Solocraft.cpp` | Умный Solocraft: динамический баланс подземелий под 1–3 игроков |
| `custom_arena_1v1.cpp` | Рейтинговая арена 1 на 1 |
| `duel.cpp` | Дуэли (база для Duel Reset) |

Пропатченные файлы ядра:

* `src/common/Define.h` — подключён `LegionForgeVersion.h`.
* `src/common/LegionForgeVersion.h` — бренд, версия, `LEGIONFORGE_CLIENT_BUILD 26124`
  + `static_assert` против смены билда.
* `src/server/game/Entities/Item/Item.cpp` — блок «ABSOLUTE TRANSMOGRIFICATION FREEDOM»
  в `Item::CanTransmogrifyItemWithItem` (управляется конфигом).
* `src/server/shared/Realm/RealmList.cpp`, `src/server/worldserver/Main.cpp` —
  `GetIntDefault("Game.Build.Version", LEGIONFORGE_CLIENT_BUILD)`.
* `src/server/scripts/ScriptLoader.cpp` — регистрация `AddSC_LegionForge_Custom()`.

---

## 5. Экономика (анти-инфляция)

* Валюта: **Сущность пробуждения (1533)** — новых фейковых валют нет.
* Бонус за онлайн: **+50 в час** активным (не AFK) игрокам.
* Добыча: финальные боссы подземелий **15–30**, рейдов **50–100**,
  кастомные мировые боссы **250–500**.
* Цены (по умолчанию): Концентрат силы Титанов **800**, Печать Вечности **1500**,
  Фолиант Древних Знаний **5000**, редкие маунты **40 000–60 000**,
  трансмог-сеты **12 000–22 000**, услуги персонажа **2 500–15 000**.
* При ~1200 Сущности в сутки топовые товары остаются целью на недели — P2W исключён.

---

## 6. Справочник GM-команд

| Команда | Описание |
|---|---|
| `.lf transmog info` | правила свободной трансмогрификации |
| `.lf transmog clear` | сброс всех иллюзий с экипировки |
| `.lf transmog status` | состояние модуля (для админа) |
| `.lf legacy reload` | перечитать список фолиантов из БД |
| `.lf legacy list` | показать все фолианты и их спеллы |
| `.lf legacy learn <id>` | выдать способность персонажу |
| `.lf worldboss reload` | перечитать таблицу мировых боссов |
| `.lf worldboss list` | список боссов, фазы и лут |
| `.lf bot reload` | перечитать профили ботов |
| `.lf bot list` | показать профили ботов |
| `.lf bot info` | справка по ИИ-напарнику |
| `.lf shop reload` | перечитать каталог магазина |
| `.lf shop list` | весь каталог с ценами |
| `.lf shop currency` | проверка валюты магазина (1533) |
| `.lf prestige` | принять Престиж (на 110 уровне) |
| `.lf hardcore` | принять обет «Одна жизнь» (на 1 уровне) |
| `.questfix` | вручную довыполнить проблемный квест |

---

## 7. Таблицы БД платформы

**world** (`legionforge_world`):
`custom_legionforge_worldboss`, `custom_legionforge_bots`, `custom_legionforge_botnames`,
`custom_legionforge_botchat`, `custom_legionforge_tomes`, `custom_legionforge_shop`,
`custom_autocomplete_quests`, `trinity_string` (записи 950001–950005 — системные строки LEGIONFORGE).

**characters** (`legionforge_characters`):
`custom_legionforge_player` (престиж, hardcore, слоты профессий), `custom_legionforge_essence_log` (журнал валюты).

**auth** (`legionforge_auth`):
`realmlist.gamebuild = 26124`, имя реалма `LEGIONFORGE | 7.3.5 | x1`.

---

## 8. Как редактировать магазин BattlePay

**Способ 1 — веб-панель (рекомендуется):** `PANEL.bat` → раздел «Магазин» → изменить
цену/категорию/количество → «Сохранить» → «Экспорт в SQL» → импортировать файл в
`legionforge_world` → в игре `.lf shop reload`.

**Способ 2 — SQL:** правьте таблицу `custom_legionforge_shop`
(`Id, ItemId, Count, Cost, Category, NameRu, DescriptionRu, SortOrder, Enabled`)
или секцию 6 файла `sql/custom/custom_legionforge.sql`.

**Способ 3 — конфиг:** цены системных операций задаются в `server/configs/worldserver.conf`
(секция `LEGIONFORGE :: КАСТОМНЫЕ МОДУЛИ`).

Категории: `1` трансмог · `2` маунты · `3` питомцы · `4` услуги · `5` реагенты · `6` фолианты.
Валюта списания — `LegionForge.BattlePay.CurrencyId` (1533).

---

## 9. Проверка перед сдачей (чек-лист)

1. `SELECT gamebuild FROM legionforge_auth.realmlist;` → **26124**.
2. `Game.Build.Version = 26124` в `worldserver.conf` и `bnetserver.conf`.
3. `grep -r "LegionForgeCore\|LegionForgeCore\|LEGIONFORGE\|LEGIONFORGE" source/` → пусто
   (сохранены только `trinity_string` как имя таблицы и игровой контент: Argus, Sunwell, Nordrassil, Ashamane).
4. Все кастомные скрипты зарегистрированы в `LegionForge_Loader.cpp` → `AddSC_LegionForge_Custom()`
   → `ScriptLoader.cpp::AddCustomScripts()`.
5. `START.bat` отрабатывает оба сценария: первая сборка и быстрый повторный запуск.
6. Все вставки в `custom_legionforge.sql` идемпотентны (`REPLACE INTO` / `INSERT IGNORE` /
   `CREATE TABLE IF NOT EXISTS`) и используют явные списки колонок.
7. Логи: `/server/logs/` (WorldServer.log, BNetServer.log, MySQL.log, compile.log, bootstrap.log).

---

## 10. Устранение неполадок

| Симптом | Решение |
|---|---|
| `LNK2019` при сборке | Перезапустите `tools/compile_boost.bat`, проверьте `BOOST_ROOT` в `compile_server.bat` |
| Клиент не видит реалм | Проверьте `gamebuild = 26124` и `Bnetserver.Server.IP = 0.0.0.0` |
| Магазин «W» пустой | `.lf shop reload`, проверьте `custom_legionforge_shop` |
| Трансмог не даёт латы→ткань | `LegionForge.Transmog.AllowAnyArmorType = 1` и пересоберите ядро (патч в `Item.cpp`) |
| Мировые боссы без тактик | `.lf worldboss reload`, проверьте `custom_legionforge_worldboss` |
| Панель не открывается | `netstat -ano | findstr :3000`, смените `PANEL_PORT` в `legionforge.bin` |

---

**LEGIONFORGE** · 7.3.5 Build 26124 · Собрано, пропатчено и задокументировано под ключ.
