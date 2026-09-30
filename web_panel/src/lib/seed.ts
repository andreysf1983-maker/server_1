import { db } from "@/db";
import {
  lfAccounts, lfBotNames, lfCharacters, lfEssenceLog, lfLegacyTomes, lfModules,
  lfOnlineSnapshots, lfQuestFixes, lfSettings, lfShopItems, lfWorldBosses,
} from "@/db/schema";
import { sql } from "drizzle-orm";
import { parseBosses, parseBotNames, parseQuestFixes, parseShop, parseTomes } from "./parse-sql";
import { CLASS_NAMES, CLIENT_BUILD, ESSENCE_CURRENCY_ID, PLATFORM_VERSION, RACE_NAMES } from "./platform";

/**
 * LEGIONFORGE :: наполнение хранилища панели.
 * Контент берётся из реального SQL-файла платформы, настройки — из
 * worldserver.conf / modules.conf. Вызывается лениво при первом обращении.
 */

const DEFAULT_SETTINGS: Array<[string, string, string, string, string, string]> = [
  // key, value, label, category, type, hint
  ["Game.Build.Version", String(CLIENT_BUILD), "Билд клиента", "core", "number", "Жёстко 26124 для WoW 7.3.5"],
  ["RealmName", "LEGIONFORGE | 7.3.5 | x1", "Название сервера", "core", "text", "Видно в списке реалмов"],
  ["Motd", "Добро пожаловать на LEGIONFORGE 7.3.5 (26124)!", "MOTD (сообщение дня)", "core", "text", "Показывается при входе в мир"],
  ["MaxPlayers", "300", "Максимум игроков", "core", "number", "Лимит одновременного онлайна"],
  ["Rate.XP.Kill", "1", "Рейт опыта за мобов", "rates", "number", "x1 = близзард"],
  ["Rate.XP.Quest", "1", "Рейт опыта за квесты", "rates", "number", ""],
  ["Rate.Drop.Item.Epic", "1", "Рейт дропа (эпик)", "rates", "number", ""],
  ["Rate.Drop.Item.Legendary", "1", "Рейт дропа (легендарный)", "rates", "number", ""],
  ["Rate.Gold.Quest", "1", "Рейт золота за квесты", "rates", "number", ""],
  ["Rate.Rest.InGame", "1", "Рейт отдыха (онлайн)", "rates", "number", ""],
  ["LegionForge.CurrencyId", String(ESSENCE_CURRENCY_ID), "Валюта экономики", "economy", "number", "1533 = Сущность пробуждения"],
  ["LegionForge.OnlineBonus.Amount", "50", "Бонус за онлайн (в час)", "economy", "number", "Сущности пробуждения"],
  ["LegionForge.OnlineBonus.IntervalMinutes", "60", "Интервал бонуса (мин)", "economy", "number", ""],
  ["LegionForge.Upgrade.LegendaryCap", "1200", "Потолок ilvl легендарок", "economy", "number", "Концентрат силы Титанов"],
  ["LegionForge.Upgrade.EndgameCap", "1000", "Потолок ilvl эпика", "economy", "number", "Печать Вечности"],
  ["LegionForge.Legacy.TomeCost", "5000", "Цена фолианта", "economy", "number", "Сущности пробуждения"],
  ["LegionForge.Transmog.EssenceCost", "25", "Цена трансмога", "economy", "number", "За операцию у Хранителя Иллюзий"],
  ["LegionForge.WorldBoss.RespawnMinutes", "180", "Респаун мировых боссов (мин)", "world", "number", ""],
  ["LegionForge.Bots.MaxOnline", "40", "Лимит ботов онлайн", "world", "number", ""],
  ["LegionForge.Bots.ChatMinSeconds", "120", "Живой чат: мин. интервал (сек)", "world", "number", ""],
];

const DEFAULT_MODULES: Array<[string, string, string, string, string]> = [
  // key, name, description, category, configKey
  ["transmog-freedom", "Свободная трансмогрификация", "Латы в ткань, легендарки, белые вещи, кросс-оружие", "gameplay", "LegionForge.Transmog.Enable"],
  ["legacy-spells", "Фолианты забытых способностей", "50 возвращённых спеллов WotLK/Cata/MoP/WoD с проверкой класса", "gameplay", "LegionForge.Legacy.Enable"],
  ["item-upgrade", "Система улучшения экипировки", "Концентрат силы Титанов (+5 ilvl до 1200) и Печать Вечности (985→1000)", "gameplay", "LegionForge.Upgrade.LegendaryCap"],
  ["online-reward", "Бонус за онлайн", "+50 Сущности пробуждения каждый час активным игрокам", "economy", "LegionForge.OnlineBonus.Enable"],
  ["battlepay-shop", "Магазин BattlePay (кнопка «W»)", "Полный каталог за Сущность пробуждения, без реальных денег", "economy", "LegionForge.BattlePay.Enable"],
  ["world-bosses", "100 мировых боссов", "Фазы, лужи, слуги, энрейдж, анонсы и богатый дроп", "pve", "LegionForge.WorldBoss.Enable"],
  ["smart-bots", "Умные ИИ-боты", "Живой чат, прогулки, роли, автозаполнение PvP", "world", "LegionForge.Bots.Enable"],
  ["broken-quests", "Авто-выполнение проблемных квестов", "Оплоты классов, Аргус, Сурамар + компенсация 25 Сущности", "qol", "LegionForge.BrokenQuests.Enable"],
  ["solocraft", "Умный Solocraft", "Динамический баланс подземелий под 1–3 игроков", "pve", "LegionForge.Mod.Solocraft"],
  ["crossfaction", "Межфракционная игра", "Общие группы, гильдии, чат и торговля", "world", "LegionForge.Crossfaction.Enable"],
  ["prestige", "Система престижа", "Сброс уровня за крылья-ауры, титулы и +% к добыче Сущности", "progression", "LegionForge.Prestige.Enable"],
  ["hardcore", "Hardcore «Одна жизнь»", "Обет на 1 уровне, блокировка при смерти, награда x3 на 110", "progression", "LegionForge.Hardcore.Enable"],
  ["arena-1v1", "Рейтинговая арена 1 на 1", "Регистратор в Даларане, защита от чистых хилов", "pvp", "LegionForge.Mod.DuelReset"],
  ["mythic-plus-qol", "Mythic+ QoL", "Обмен ключа на другой данж и телепорт группы ко входу", "qol", "LegionForge.QoL.MythicPlus.Enable"],
  ["racial-swapper", "Смена расовых способностей", "Внешность одной расы, пассивки другой", "qol", "LegionForge.QoL.RacialSwap.Enable"],
  ["duel-reset", "Duel Reset", "После дуэли: 100% HP/маны и сброс КД от 30 секунд", "pvp", "LegionForge.QoL.DuelReset.Enable"],
  ["multi-professions", "Мульти-профессии", "До 4 основных профессий на персонаже", "qol", "LegionForge.QoL.MultiProf.Enable"],
];

const DEMO_CHARACTERS: Array<[number, string, string, number, number, number, number, number, boolean, string]> = [
  [1, "Траллмар", "admin", 7, 2, 110, 1000, 12500, false, "Даларан"],
  [2, "Сильванасша", "admin", 9, 5, 110, 995, 8400, false, "Сурамар"],
  [3, "Кельтасар", "player1", 8, 10, 110, 988, 5200, true, "Аргус"],
  [4, "Дренейка", "player2", 5, 11, 108, 960, 1800, true, "Вал'шара"],
  [5, "Громмаш", "player3", 1, 2, 110, 999, 15200, false, "Штормхейм"],
  [6, "Иллидана", "player4", 12, 10, 110, 990, 7300, true, "Расколотый берег"],
  [7, "Медивхон", "player5", 8, 1, 105, 940, 900, false, "Крутогорье"],
  [8, "Артасия", "player6", 6, 4, 110, 1015, 22100, false, "Даларан"],
  [9, "Волчара", "player7", 11, 22, 101, 910, 400, true, "Лунная поляна"],
  [10, "Зулджинкс", "player8", 4, 8, 110, 985, 3100, false, "Азсуна"],
];

let seeded = false;

export async function ensureSeeded(force = false): Promise<boolean> {
  if (seeded && !force) return false;
  const [{ n }] = await db.select({ n: sql<number>`count(*)::int` }).from(lfSettings);
  if (n > 0 && !force) { seeded = true; return false; }

  // Настройки
  for (const [key, value, label, category, type, hint] of DEFAULT_SETTINGS) {
    await db.insert(lfSettings).values({ key, value, label, category, type, hint })
      .onConflictDoUpdate({ target: lfSettings.key, set: { label, category, type, hint, updatedAt: new Date() } });
  }

  // Модули
  for (const [key, name, description, category, configKey] of DEFAULT_MODULES) {
    await db.insert(lfModules).values({ key, name, description, category, configKey, enabled: true })
      .onConflictDoNothing();
  }

  // Мировые боссы (из SQL платформы)
  const bosses = parseBosses();
  for (const b of bosses) {
    await db.insert(lfWorldBosses).values({
      entry: b.entry, nameRu: b.nameRu, zone: b.zone, continent: zoneContinent(b.zone),
      levelMin: b.levelMin, levelMax: b.levelMax, rank: b.rank,
      essenceMin: b.essenceMin, essenceMax: b.essenceMax, respawnMinutes: b.respawnMinutes,
      displayId: b.displayId, spellMain: b.spellMain, spellAoe: b.spellAoe,
      summonEntry: b.summonEntry, spellEnrage: b.spellEnrage, enabled: true,
    }).onConflictDoUpdate({ target: lfWorldBosses.entry, set: { nameRu: b.nameRu, zone: b.zone } });
  }

  // Магазин
  const shop = parseShop();
  for (const s of shop) {
    await db.insert(lfShopItems).values({
      itemId: s.itemId, count: s.count, cost: s.cost, category: s.category,
      nameRu: s.nameRu, descriptionRu: s.descriptionRu, sortOrder: s.sortOrder, enabled: true,
    });
  }

  // Фолианты
  for (const t of parseTomes()) {
    await db.insert(lfLegacyTomes).values({ itemId: t.itemId, classId: t.classId, spellId: t.spellId, nameRu: t.nameRu, enabled: true })
      .onConflictDoNothing();
  }

  // Авто-выполнение квестов
  for (const q of parseQuestFixes()) {
    await db.insert(lfQuestFixes).values({ questId: q.questId, reason: q.reason, compensation: q.compensation, enabled: true })
      .onConflictDoNothing();
  }

  // Боты
  const names = parseBotNames();
  for (let i = 0; i < names.length; i++) {
    await db.insert(lfBotNames).values({
      nick: names[i], classId: (i % 12) + 1, role: i % 3,
      zone: ["Штормград", "Оргриммар", "Даларан", "Стальгорн", "Подгород"][i % 5], enabled: true,
    }).onConflictDoNothing();
  }

  // Аккаунты и персонажи
  const accounts = ["admin", "gamemaster", "player1", "player2", "player3", "player4", "player5", "player6", "player7", "player8"];
  for (let i = 0; i < accounts.length; i++) {
    await db.insert(lfAccounts).values({
      username: accounts[i], email: `${accounts[i]}@legionforge.gg`,
      gmLevel: i < 2 ? 3 : 0, expansion: 6,
      essence: 5000 + i * 1750, banned: false, lastLogin: new Date(Date.now() - i * 3600_000),
    }).onConflictDoNothing();
  }

  for (const [guid, name, account, classId, raceId, level, ilvl, essence, online, zone] of DEMO_CHARACTERS) {
    await db.insert(lfCharacters).values({
      guid, name, account, classId, raceId, level, ilvl,
      prestige: ilvl >= 1000 ? 1 : 0, hardcore: name === "Волчара",
      totalEssence: essence, online, zone,
    }).onConflictDoNothing();
  }

  // Телеметрия онлайна за 24 часа
  for (let i = 23; i >= 0; i--) {
    const players = 8 + Math.round(Math.abs(Math.sin(i / 3)) * 46);
    await db.insert(lfOnlineSnapshots).values({
      at: new Date(Date.now() - i * 3600_000), players, essenceGranted: players * 50,
    });
  }

  // Журнал выдачи Сущности
  const reasons = ["онлайн-бонус", "мировой босс", "рейд: Аргус", "эпохальный +15", "авто-сдача квеста", "престиж"];
  for (let i = 0; i < 24; i++) {
    const ch = DEMO_CHARACTERS[i % DEMO_CHARACTERS.length];
    await db.insert(lfEssenceLog).values({
      at: new Date(Date.now() - i * 1800_000), characterName: ch[1],
      amount: reasons[i % reasons.length] === "онлайн-бонус" ? 50 : 150 + (i % 7) * 50,
      reason: reasons[i % reasons.length],
    });
  }

  seeded = true;
  return true;
}

function zoneContinent(zone: string): string {
  const lower = zone.toLowerCase();
  if (lower.includes("аргус")) return "Аргус";
  if (["штормград", "оргриммар", "стал", "подгород", "арати", "внутренние", "тлеющие", "чумные", "нагорье"].some((k) => lower.includes(k))) return "Восточные королевства";
  if (["даларан", "азсуна", "вал", "штормхейм", "крутогорье", "сурамар", "расколот", "око"].some((k) => lower.includes(k))) return "Расколотые острова";
  return "Калимдор";
}

export async function ensureSeededSafe(): Promise<void> {
  try { await ensureSeeded(); } catch (e) { console.error("[LEGIONFORGE] seed failed:", e); }
}

export const SEED_LABELS = { CLASS_NAMES, RACE_NAMES, PLATFORM_VERSION };
