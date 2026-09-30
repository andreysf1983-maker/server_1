import fs from "node:fs";
import path from "node:path";
import { SQL_DIR } from "./platform";

/**
 * LEGIONFORGE :: парсер сгенерированного custom_legionforge.sql.
 * Панель читает реальный игровой контент из того же файла, который
 * импортируется в MySQL — единый источник правды, без дублирования данных.
 */

export type ParsedBoss = {
  entry: number;
  nameRu: string;
  displayId: number;
  phase1: number;
  phase2: number;
  spellMain: number;
  spellAoe: number;
  summonEntry: number;
  spellEnrage: number;
  essenceMin: number;
  essenceMax: number;
  respawnMinutes: number;
  zone: string;
  levelMin: number;
  levelMax: number;
  rank: number;
};

export type ParsedShopItem = {
  id: number; itemId: number; count: number; cost: number;
  category: number; nameRu: string; descriptionRu: string; sortOrder: number;
};

export type ParsedTome = { itemId: number; classId: number; spellId: number; nameRu: string };
export type ParsedQuest = { questId: number; reason: string; compensation: number };

function readSql(): string {
  const file = path.join(SQL_DIR, "custom", "custom_legionforge.sql");
  if (!fs.existsSync(file)) return "";
  return fs.readFileSync(file, "utf8");
}

/** Достаёт тело VALUES после указанного заголовка (до первой точки с запятой). */
function sectionBody(sql: string, header: RegExp): string {
  const m = sql.match(header);
  if (!m || m.index === undefined) return "";
  const start = m.index + m[0].length;
  const end = sql.indexOf(";", start);
  return sql.slice(start, end === -1 ? undefined : end);
}

/** Разбивает тело VALUES на строки-кортежи, учитывая кавычки. */
function splitRows(body: string): string[][] {
  const rows: string[][] = [];
  let current: string[] = [];
  let field = "";
  let inString = false;
  let depth = 0;

  for (let i = 0; i < body.length; i++) {
    const ch = body[i];
    if (inString) {
      if (ch === "\\" && i + 1 < body.length) { field += ch + body[i + 1]; i++; continue; }
      if (ch === "'") { inString = false; }
      field += ch;
      continue;
    }
    if (ch === "'") { inString = true; field += ch; continue; }
    if (ch === "(") { depth++; if (depth === 1) { field = ""; current = []; } continue; }
    if (ch === ")") { depth--; if (depth === 0) { current.push(field.trim()); rows.push(current); } continue; }
    if (ch === "," && depth === 1) { current.push(field.trim()); field = ""; continue; }
    if (depth >= 1) field += ch;
  }
  return rows;
}

function unquote(v: string): string {
  const t = v.trim();
  if (t.startsWith("'") && t.endsWith("'")) return t.slice(1, -1).replace(/\\'/g, "'").replace(/\\\\/g, "\\");
  return t;
}
function num(v: string): number {
  const n = parseInt(unquote(v), 10);
  return Number.isFinite(n) ? n : 0;
}

export function parseBosses(): ParsedBoss[] {
  const sql = readSql();
  if (!sql) return [];

  const tactics = new Map<number, ParsedBoss>();
  for (const row of splitRows(sectionBody(sql, /REPLACE INTO `custom_legionforge_worldboss`[\s\S]*?VALUES/))) {
    if (row.length < 16) continue;
    tactics.set(num(row[0]), {
      entry: num(row[0]),
      nameRu: unquote(row[1]),
      displayId: num(row[3]),
      phase1: num(row[4]),
      phase2: num(row[5]),
      spellMain: num(row[6]),
      spellAoe: num(row[7]),
      summonEntry: num(row[9]),
      spellEnrage: num(row[10]),
      essenceMin: num(row[12]),
      essenceMax: num(row[13]),
      respawnMinutes: num(row[14]),
      zone: "", levelMin: 110, levelMax: 110, rank: 2,
    });
  }

  for (const row of splitRows(sectionBody(sql, /INSERT INTO `creature_template`[\s\S]*?VALUES/))) {
    if (row.length < 17) continue;
    const entry = num(row[0]);
    const boss = tactics.get(entry);
    if (!boss) continue;
    boss.nameRu = boss.nameRu || unquote(row[1]);
    boss.levelMin = num(row[3]);
    boss.levelMax = num(row[4]);
    boss.rank = num(row[10]);
    const sub = unquote(row[2]);
    const parts = sub.split("·");
    boss.zone = (parts[1] ?? "").trim();
  }

  return Array.from(tactics.values()).sort((a, b) => a.entry - b.entry);
}

export function parseShop(): ParsedShopItem[] {
  const sql = readSql();
  return splitRows(sectionBody(sql, /INSERT INTO `custom_legionforge_shop`[\s\S]*?VALUES/))
    .filter((r) => r.length >= 9)
    .map((r) => ({
      id: num(r[0]), itemId: num(r[1]), count: num(r[2]), cost: num(r[3]),
      category: num(r[4]), nameRu: unquote(r[5]), descriptionRu: unquote(r[6]), sortOrder: num(r[7]),
    }));
}

export function parseTomes(): ParsedTome[] {
  const sql = readSql();
  return splitRows(sectionBody(sql, /REPLACE INTO `custom_legionforge_tomes`[\s\S]*?VALUES/))
    .filter((r) => r.length >= 5)
    .map((r) => ({ itemId: num(r[0]), classId: num(r[1]), spellId: num(r[2]), nameRu: unquote(r[3]) }));
}

export function parseQuestFixes(): ParsedQuest[] {
  const sql = readSql();
  return splitRows(sectionBody(sql, /INSERT INTO `custom_autocomplete_quests`[\s\S]*?VALUES/))
    .filter((r) => r.length >= 4)
    .map((r) => ({ questId: num(r[0]), reason: unquote(r[1]), compensation: num(r[2]) }));
}

export function parseBotNames(): string[] {
  const sql = readSql();
  return splitRows(sectionBody(sql, /INSERT INTO `custom_legionforge_botnames`[\s\S]*?VALUES/))
    .map((r) => unquote(r[0] ?? ""))
    .filter(Boolean);
}
