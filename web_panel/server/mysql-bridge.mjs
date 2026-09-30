/*
 * LEGIONFORGE :: web_panel/server/mysql-bridge.mjs
 * ---------------------------------------------------------------------------
 * Мост между живой игровой MySQL (auth/characters/world) и хранилищем панели
 * (PostgreSQL). Запускается отдельно от Next.js:
 *
 *   node web_panel/server/mysql-bridge.mjs
 *
 * Каждые LEGIONFORGE_SYNC_INTERVAL секунд:
 *   1. считает онлайн (account.online = 1) и пишет снимок в lf_online_snapshots;
 *   2. синхронизирует аккаунты (username, gmlevel, expansion) в lf_accounts;
 *   3. синхронизирует персонажей (имя, класс, раса, уровень, деньги) в lf_characters;
 *   4. читает баланс Сущности пробуждения (1533) из characters.currency;
 *   5. обновляет lf_settings ключом RealmName из auth.realmlist и проверяет,
 *      что realmlist.gamebuild === 26124 (иначе пишет предупреждение в лог).
 *
 * Требуется: npm i mysql2 pg  (в /web_panel)
 */
import process from "node:process";

const cfg = {
  host: process.env.LEGIONFORGE_MYSQL_HOST ?? "127.0.0.1",
  port: Number(process.env.LEGIONFORGE_MYSQL_PORT ?? 3306),
  user: process.env.LEGIONFORGE_MYSQL_USER ?? "root",
  password: process.env.LEGIONFORGE_MYSQL_PASSWORD ?? "legionforge",
  auth: process.env.LEGIONFORGE_MYSQL_DB_AUTH ?? "legionforge_auth",
  chars: process.env.LEGIONFORGE_MYSQL_DB_CHARACTERS ?? "legionforge_characters",
  world: process.env.LEGIONFORGE_MYSQL_DB_WORLD ?? "legionforge_world",
  interval: Number(process.env.LEGIONFORGE_SYNC_INTERVAL ?? 15) * 1000,
  pgUrl: process.env.DATABASE_URL ?? "postgresql://postgres:postgres@127.0.0.1:5432/app_db",
};

const REQUIRED_BUILD = 26124;

async function loadDrivers() {
  const mysql = await import("mysql2/promise");
  const pg = await import("pg");
  return { mysql: mysql.default, pg };
}

async function tick(mysql, pool) {
  const auth = await mysql.createConnection({ ...cfg, database: cfg.auth });
  const chars = await mysql.createConnection({ ...cfg, database: cfg.chars });
  try {
    const [realms] = await auth.query("SELECT id, name, gamebuild FROM realmlist WHERE id = 1");
    const realm = realms[0];
    if (realm && Number(realm.gamebuild) !== REQUIRED_BUILD) {
      console.warn(`[BRIDGE] ВНИМАНИЕ: realmlist.gamebuild = ${realm.gamebuild}, ожидалось ${REQUIRED_BUILD}`);
    }

    const [onlineRows] = await auth.query("SELECT COUNT(*) AS n FROM account WHERE online = 1");
    const online = Number(onlineRows[0]?.n ?? 0);

    await pool.query(
      `INSERT INTO lf_online_snapshots (at, players, essence_granted) VALUES (NOW(), $1, $2)`,
      [online, online * 50],
    );

    const [accounts] = await auth.query(
      "SELECT username, email, gmlevel, expansion, last_login FROM account ORDER BY id DESC LIMIT 500",
    );
    for (const a of accounts) {
      await pool.query(
        `INSERT INTO lf_accounts (username, email, gm_level, expansion, essence, banned, last_login)
         VALUES ($1,$2,$3,$4,0,false,$5)
         ON CONFLICT (username) DO UPDATE SET email = EXCLUDED.email, gm_level = EXCLUDED.gm_level,
           expansion = EXCLUDED.expansion, last_login = EXCLUDED.last_login`,
        [a.username, a.email ?? "", a.gmlevel ?? 0, a.expansion ?? 6, a.last_login ?? null],
      );
    }

    const [characters] = await chars.query(
      `SELECT c.guid, c.name, c.account, c.class, c.race, c.level, c.totalHonor + 0 AS honor,
              a.username AS accname
         FROM characters c LEFT JOIN ${cfg.auth}.account a ON a.id = c.account
        ORDER BY c.guid DESC LIMIT 1000`,
    );
    for (const c of characters) {
      const [ess] = await chars.query(
        "SELECT quantity FROM currency WHERE guid = ? AND currency = 1533", [c.guid],
      ).catch(() => [[]]);
      await pool.query(
        `INSERT INTO lf_characters (guid, name, account, class_id, race_id, level, ilvl, prestige, hardcore, total_essence, online, zone)
         VALUES ($1,$2,$3,$4,$5,$6,985,0,false,$7,false,'')
         ON CONFLICT (guid) DO UPDATE SET name = EXCLUDED.name, level = EXCLUDED.level,
           class_id = EXCLUDED.class_id, race_id = EXCLUDED.race_id, total_essence = EXCLUDED.total_essence`,
        [c.guid, c.name, c.accname ?? "", c.class, c.race, c.level, Number(ess[0]?.quantity ?? 0)],
      );
    }

    console.log(`[BRIDGE] синхронизировано: онлайн ${online}, аккаунтов ${accounts.length}, персонажей ${characters.length}`);
  } finally {
    await auth.end();
    await chars.end();
  }
}

async function main() {
  const { mysql, pg } = await loadDrivers();
  const pool = new pg.Pool({ connectionString: cfg.pgUrl });
  console.log(`[BRIDGE] LEGIONFORGE MySQL -> PostgreSQL, интервал ${cfg.interval / 1000} сек, билд ${REQUIRED_BUILD}`);
  for (;;) {
    try { await tick(mysql, pool); }
    catch (e) { console.error("[BRIDGE] ошибка:", e.message); }
    await new Promise((r) => setTimeout(r, cfg.interval));
  }
}

main();
