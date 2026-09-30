import fs from "node:fs";
import path from "node:path";
import AdmZip from "adm-zip";

/**
 * LEGIONFORGE :: доступ к файловой структуре платформы из веб-панели.
 * Корень платформы — каталог LEGIONFORGE рядом с приложением панели.
 */

/**
 * Корень платформы определяется автоматически, чтобы панель работала в двух
 * раскладках:
 *   1. классической — панель в /LEGIONFORGE/web_panel, платформа в /LEGIONFORGE;
 *   2. совмещённой  — исходники панели лежат прямо в корне /LEGIONFORGE.
 * Раньше путь был зашит как process.cwd() + "LEGIONFORGE", из-за чего во второй
 * раскладке все 9 проверок готовности возвращали false.
 */
function detectPlatformRoot(): string {
  const cwd = process.cwd();
  const override = process.env.LEGIONFORGE_ROOT;
  if (override && override.trim()) return path.resolve(override.trim());

  const candidates = [path.join(cwd, "LEGIONFORGE"), path.resolve(cwd, ".."), cwd];
  for (const candidate of candidates) {
    if (fs.existsSync(path.join(candidate, "source", "CMakeLists.txt"))) return candidate;
    if (fs.existsSync(path.join(candidate, "server", "configs", "worldserver.conf"))) return candidate;
  }
  return cwd;
}

export const PLATFORM_ROOT = detectPlatformRoot();
export const SOURCE_DIR = path.join(PLATFORM_ROOT, "source");
export const SERVER_DIR = path.join(PLATFORM_ROOT, "server");
export const CONFIGS_DIR = path.join(SERVER_DIR, "configs");
export const SQL_DIR = path.join(PLATFORM_ROOT, "sql");
export const DOCS_DIR = path.join(PLATFORM_ROOT, "docs");
export const TOOLS_DIR = path.join(PLATFORM_ROOT, "tools");

export const CLIENT_BUILD = 26124;
export const CLIENT_VERSION = "7.3.5";
export const PLATFORM_VERSION = "3.1.0";
export const ESSENCE_CURRENCY_ID = 1533;

export const SHOP_CATEGORIES: Record<number, string> = {
  1: "Редчайший трансмог",
  2: "Маунты",
  3: "Питомцы",
  4: "Услуги персонажа",
  5: "Реагенты улучшения",
  6: "Фолианты способностей",
};

export const CLASS_NAMES: Record<number, string> = {
  1: "Воин", 2: "Паладин", 3: "Охотник", 4: "Разбойник", 5: "Жрец",
  6: "Рыцарь смерти", 7: "Шаман", 8: "Маг", 9: "Чернокнижник",
  10: "Монах", 11: "Друид", 12: "Демон-охотник",
};

export const RACE_NAMES: Record<number, string> = {
  1: "Человек", 2: "Орк", 3: "Дворф", 4: "Ночной эльф", 5: "Нежить",
  6: "Таурен", 7: "Гном", 8: "Тролль", 9: "Гоблин", 10: "Эльф крови",
  11: "Дреней", 22: "Ворген", 24: "Пандарен", 25: "Пандарен (А)", 26: "Пандарен (О)",
};

export const ROLE_NAMES = ["ДД", "Танк", "Хил"];

export function safeJoin(relative: string): string | null {
  const cleaned = relative.replace(/\\/g, "/").replace(/^\/+/, "");
  const full = path.resolve(PLATFORM_ROOT, cleaned);
  if (!full.startsWith(PLATFORM_ROOT)) return null;
  return full;
}

export type FileNode = {
  name: string;
  path: string;
  type: "dir" | "file";
  size: number;
  children?: FileNode[];
};

const SKIP_DIRS = new Set(["node_modules", ".git", ".next", "_cache", "build"]);

export function buildTree(dir: string, relative: string, depth: number): FileNode[] {
  if (depth < 0 || !fs.existsSync(dir)) return [];
  const entries = fs.readdirSync(dir, { withFileTypes: true })
    .filter((e) => !SKIP_DIRS.has(e.name))
    .sort((a, b) => {
      if (a.isDirectory() !== b.isDirectory()) return a.isDirectory() ? -1 : 1;
      return a.name.localeCompare(b.name);
    });

  return entries.map((entry) => {
    const rel = relative ? `${relative}/${entry.name}` : entry.name;
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      return { name: entry.name, path: rel, type: "dir", size: 0, children: buildTree(full, rel, depth - 1) };
    }
    let size = 0;
    try { size = fs.statSync(full).size; } catch { /* ignore */ }
    return { name: entry.name, path: rel, type: "file", size };
  });
}

export function readFileSafe(relative: string, maxBytes = 400_000): { content: string; truncated: boolean; size: number } | null {
  const full = safeJoin(relative);
  if (!full || !fs.existsSync(full) || !fs.statSync(full).isFile()) return null;
  const size = fs.statSync(full).size;
  const fd = fs.openSync(full, "r");
  const buffer = Buffer.alloc(Math.min(size, maxBytes));
  fs.readSync(fd, buffer, 0, buffer.length, 0);
  fs.closeSync(fd);
  return { content: buffer.toString("utf8"), truncated: size > maxBytes, size };
}

export function createZip(relatives: string[]): Buffer {
  const zip = new AdmZip();
  for (const rel of relatives) {
    const full = safeJoin(rel);
    if (!full || !fs.existsSync(full)) continue;
    const stat = fs.statSync(full);
    if (stat.isDirectory()) zip.addLocalFolder(full, rel);
    else zip.addLocalFile(full, path.dirname(rel));
  }
  return zip.toBuffer();
}

/** Ключи worldserver.conf, которые редактируются в конфигураторе панели. */
export const CONF_KEYS = [
  "Game.Build.Version", "RealmName", "Motd", "MaxPlayers",
  "Rate.XP.Kill", "Rate.XP.Quest", "Rate.Drop.Item.Epic", "Rate.Drop.Item.Legendary",
  "Rate.Gold.Quest", "Rate.Rest.InGame",
  "LegionForge.CurrencyId",
  "LegionForge.OnlineBonus.Enable", "LegionForge.OnlineBonus.Amount", "LegionForge.OnlineBonus.IntervalMinutes",
  "LegionForge.BattlePay.Enable", "LegionForge.BattlePay.CurrencyId",
  "LegionForge.Transmog.Enable", "LegionForge.Transmog.AllowAnyArmorType", "LegionForge.Transmog.AllowLegendaries",
  "LegionForge.Transmog.AllowCrossWeaponType", "LegionForge.Transmog.EssenceCost",
  "LegionForge.Upgrade.LegendaryCap", "LegionForge.Upgrade.LegendaryStep", "LegionForge.Upgrade.ConcentrateCost",
  "LegionForge.Upgrade.EndgameMin", "LegionForge.Upgrade.EndgameCap", "LegionForge.Upgrade.EndgameCost",
  "LegionForge.Legacy.Enable", "LegionForge.Legacy.TomeCost",
  "LegionForge.WorldBoss.Enable", "LegionForge.WorldBoss.RespawnMinutes", "LegionForge.WorldBoss.ScalePerPlayer",
  "LegionForge.Bots.Enable", "LegionForge.Bots.MaxOnline", "LegionForge.Bots.ChatMinSeconds",
  "LegionForge.Crossfaction.Enable", "LegionForge.Prestige.Enable", "LegionForge.Hardcore.Enable",
  "LegionForge.QoL.MythicPlus.Enable", "LegionForge.QoL.MultiProf.MaxSlots",
  "LegionForge.BrokenQuests.Enable", "LegionForge.Mod.Solocraft", "LegionForge.Mod.DuelReset",
] as const;

export type ConfMap = Record<string, string>;

export function readConf(file: string): { text: string; values: ConfMap } {
  const full = path.join(CONFIGS_DIR, file);
  if (!fs.existsSync(full)) return { text: "", values: {} };
  const text = fs.readFileSync(full, "utf8");
  const values: ConfMap = {};
  for (const line of text.split(/\r?\n/)) {
    const m = line.match(/^\s*([A-Za-z0-9_.]+)\s*=\s*(.*)$/);
    if (!m) continue;
    const raw = m[2];
    const inline = raw.indexOf("#");
    values[m[1]] = (inline >= 0 ? raw.slice(0, inline) : raw).trim().replace(/^"|"$/g, "");
  }
  return { text, values };
}

export function writeConfValues(file: string, updates: ConfMap): { ok: boolean; changed: number; error?: string } {
  const full = path.join(CONFIGS_DIR, file);
  if (!fs.existsSync(full)) return { ok: false, changed: 0, error: `Файл ${file} не найден` };
  let text = fs.readFileSync(full, "utf8");
  let changed = 0;
  const eol = text.includes("\r\n") ? "\r\n" : "\n";

  for (const [key, value] of Object.entries(updates)) {
    const escapedKey = key.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
    const re = new RegExp(`^([ \\t]*${escapedKey}[ \\t]*=[ \\t]*)([^#\\r\\n]*)`, "m");
    if (re.test(text)) {
      text = text.replace(re, (_m, prefix: string) => `${prefix}${value} `);
      changed++;
    } else {
      text += `${eol}${key} = ${value}${eol}`;
      changed++;
    }
  }
  fs.writeFileSync(full, text, "utf8");
  return { ok: true, changed };
}

export function platformHealth() {
  const checks = [
    { key: "source", label: "Исходники ядра (/source)", ok: fs.existsSync(path.join(SOURCE_DIR, "CMakeLists.txt")) },
    { key: "custom", label: "Кастомные C++ модули", ok: fs.existsSync(path.join(SOURCE_DIR, "src/server/scripts/Custom/LegionForge_Loader.cpp")) },
    { key: "bin", label: "Скомпилированные бинарники (/server/bin)", ok: fs.existsSync(path.join(SERVER_DIR, "bin/worldserver.exe")) },
    { key: "configs", label: "Конфиги (/server/configs)", ok: fs.existsSync(path.join(CONFIGS_DIR, "worldserver.conf")) },
    { key: "sqlbase", label: "Дампы БД (/sql/base/*.sql)", ok: fs.existsSync(path.join(SQL_DIR, "base/auth.sql")) },
    { key: "sqlcustom", label: "Кастомный SQL (/sql/custom)", ok: fs.existsSync(path.join(SQL_DIR, "custom/custom_legionforge.sql")) },
    { key: "maps", label: "Карты и DBC (/server/data/maps)", ok: fs.existsSync(path.join(SERVER_DIR, "data/maps")) },
    { key: "tools", label: "Портативные утилиты (/tools)", ok: fs.existsSync(path.join(TOOLS_DIR, "compile_server.bat")) },
    { key: "docs", label: "Документация (/docs)", ok: fs.existsSync(path.join(DOCS_DIR, "README_RU.md")) },
  ];

  const conf = readConf("worldserver.conf");
  const build = conf.values["Game.Build.Version"];

  return {
    checks,
    buildConfigured: build ?? null,
    buildOk: build === String(CLIENT_BUILD),
    confKeys: Object.keys(conf.values).length,
    readyScore: Math.round((checks.filter((c) => c.ok).length / checks.length) * 100),
  };
}

/** Рекурсивный подсчёт файлов ядра (без node_modules/.git/build-артефактов). */
function countFilesRecursive(dir: string, budget: number): { files: number; budget: number } {
  let files = 0;
  if (!fs.existsSync(dir)) return { files, budget };
  let stack: string[] = [dir];
  while (stack.length && budget > 0) {
    const current = stack.pop() as string;
    let entries: fs.Dirent[];
    try {
      entries = fs.readdirSync(current, { withFileTypes: true });
    } catch {
      continue;
    }
    for (const entry of entries) {
      if (budget <= 0) break;
      budget--;
      if (entry.isDirectory()) {
        if (!SKIP_DIRS.has(entry.name)) stack.push(path.join(current, entry.name));
      } else {
        files++;
      }
    }
  }
  return { files, budget };
}

export function countSourceFiles(): { files: number; customScripts: number } {
  const customDir = path.join(SOURCE_DIR, "src/server/scripts/Custom");
  let customScripts = 0;
  if (fs.existsSync(customDir)) {
    customScripts = fs
      .readdirSync(customDir)
      .filter((f) => f.startsWith("LegionForge_") && f.endsWith(".cpp")).length;
  }
  // Бюджет ограничивает обход, чтобы дашборд не тормозил на полном дереве ядра.
  const { files } = countFilesRecursive(SOURCE_DIR, 200_000);
  return { files, customScripts };
}
