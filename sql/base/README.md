# database/import/ — ваши полные дампы баз

Положите сюда 4 файла (имена именно такие):

| Файл             | База MariaDB          |
|------------------|-----------------------|
| `auth.sql`       | `reborn_auth`         |
| `characters.sql` | `reborn_characters`   |
| `world.sql`      | `reborn_world`        |
| `hotfixes.sql`   | `reborn_hotfixes`     |

Импорт: `START.bat` → **4 Import databases** (Linux: `./scripts/linux/azeroth.sh db-import`).
После импорта автоматически применяются файлы из `database/custom/` (модуль Azeroth Reborn,
магазин и мировые боссы, выгруженные из веб-панели) и realm переименовывается в **Azeroth Reborn**.
Сами дампы в git не попадают (см. `.gitignore`).
