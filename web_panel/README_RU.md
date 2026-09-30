# /web_panel — веб-панель администратора и конфигуратор сборки LEGIONFORGE

Панель — это Next.js 16 (App Router) + PostgreSQL (Drizzle ORM). Она входит в состав
платформы и запускается скриптом **PANEL.bat** из корня `/LEGIONFORGE`.

## Состав

| Путь | Назначение |
|---|---|
| `src/app/*` | Страницы панели: дашборд, конфигуратор, магазин, боссы, фолианты, игроки, файлы, документация, промо |
| `src/app/api/*` | REST API: `status`, `settings`, `modules`, `shop`, `world-bosses`, `legacy-spells`, `players`, `files`, `export/sql`, `seed` |
| `src/lib/platform.ts` | Доступ к файловой структуре платформы, парсинг/запись `worldserver.conf` и `modules.conf`, zip-архивы |
| `src/lib/parse-sql.ts` | Парсер `sql/custom/custom_legionforge.sql` — панель показывает реальный игровой контент |
| `src/lib/seed.ts` | Ленивое наполнение хранилища панели (боссы, магазин, фолианты, аккаунты, телеметрия) |
| `src/db/schema.ts` | Схема PostgreSQL (lf_settings, lf_modules, lf_shop_items, lf_world_bosses, ...) |
| `server/mysql-bridge.mjs` | Мост к живой игровой MySQL: онлайн, аккаунты, персонажи, баланс Сущности (1533), контроль `gamebuild = 26124` |
| `sync_panel.bat` | Синхронизация исходников панели из корня проекта |
| `.env.example` | Шаблон переменных окружения (DATABASE_URL + параметры MySQL-моста) |

## Запуск

```bat
cd /LEGIONFORGE
PANEL.bat
```

Скрипт прочитает триггер `legionforge.bin` (порт и режим), проверит портативный Node.js в
`/tools/nodejs/`, выполнит `npm install` и `npm run build`, запустит сервер и откроет браузер
по адресу `http://127.0.0.1:3000`. Консоль останется открытой — это индикатор работы панели.

Ручной запуск:

```bat
cd /LEGIONFORGE/web_panel
copy .env.example .env
..\tools\nodejs\npm.cmd install
..\tools\nodejs\npm.cmd run build
..\tools\nodejs\npm.cmd run start
```

Синхронизация с игровыми базами (необязательно):

```bat
cd /LEGIONFORGE/web_panel
..\tools\nodejs\npm.cmd i mysql2 pg
..\tools\nodejs\node.exe server\mysql-bridge.mjs
```

## Что умеет панель

* **Дашборд** — готовность платформы (9 проверок), контроль билда 26124, онлайн за 24 часа, журнал выдачи Сущности.
* **Конфигуратор** — рейты, имя реалма, MOTD, экономика, потолки ilvl и 17 переключателей модулей;
  сохранение пишет значения прямо в `server/configs/worldserver.conf` и `modules.conf`.
* **Магазин «W»** — CRUD каталога BattlePay, категории, цены в Сущности пробуждения, экспорт в SQL.
* **Мировые боссы** — реестр 100 боссов с редактированием лута и респауна.
* **Фолианты и квесты** — 50 возвращённых способностей по классам и список авто-выполняемых квестов.
* **Игроки и боты** — аккаунты, персонажи, выдача валюты, пул из 320 ников ботов.
* **Файлы платформы** — дерево, предпросмотр, скачивание файла или zip-набора.
* **Документация и промо** — `docs/README_RU.md` и `docs/PROMO_TEXT.txt` в оформленном виде.
