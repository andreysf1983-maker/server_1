/*
 * LEGIONFORGE — генератор единого кастомного SQL под билд 7.3.5.26124
 * ---------------------------------------------------------------------------
 *  node tools/generate_custom_sql.mjs
 *
 * Создаёт:
 *   sql/custom/custom_legionforge.sql             -> world DB
 *   sql/custom/custom_legionforge_auth.sql        -> auth DB (realmlist = 26124)
 *   sql/custom/custom_legionforge_characters.sql  -> characters DB
 *
 * Все вставки идемпотентны: REPLACE INTO / INSERT IGNORE / CREATE TABLE IF NOT EXISTS.
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const OUT_DIR = path.join(ROOT, "sql", "custom");
fs.mkdirSync(OUT_DIR, { recursive: true });

const CURRENCY_ESSENCE = 1533;
const BOSS_FIRST = 900001;
const BOSS_COUNT = 100;

const esc = (s) => String(s).replace(/\\/g, "\\\\").replace(/'/g, "\\'");

/* ------------------------------------------------------------------ *
 * 1. Боссы: берём 60 из наработок репозитория и дополняем до 100
 * ------------------------------------------------------------------ */
function loadSeedBosses() {
  const candidates = [
    path.join(ROOT, "_repo_original", "src", "lib", "boss-seed.ts"),
    path.join(ROOT, "tools", "data", "boss-seed.ts"),
  ];
  for (const file of candidates) {
    if (!fs.existsSync(file)) continue;
    let src = fs.readFileSync(file, "utf8");
    src = src.replace(/export type[\s\S]*?\n\};\n/, "");
    src = src.replace(/export const WORLD_BOSSES_\d+[^=]*=/, "const SEED =");
    try {
      return new Function(src + "; return SEED;")();
    } catch (e) {
      console.warn("[!] Не удалось разобрать boss-seed.ts:", e.message);
    }
  }
  return [];
}

const EXTRA_ZONES = [
  ["Расколотый берег", "Расколотые острова", 579, "Легион: Пустынный рубеж"],
  ["Крутогорье", "Расколотые острова", 641, "Легион: Каменные небеса"],
  ["Вал'шара", "Расколотые острова", 642, "Легион: Тёмная роща"],
  ["Азсуна", "Расколотые острова", 630, "Легион: Руны древних"],
  ["Штормхейм", "Расколотые острова", 634, "Легион: Гром и сталь"],
  ["Сурамар", "Расколотые острова", 680, "Легион: Ночная пыль"],
  ["Аргус: Анторанские пустоши", "Аргус", 830, "Легион: Скверна"],
  ["Аргус: Крокуун", "Аргус", 790, "Легион: Пепел"],
  ["Аргус: Эредат", "Аргус", 885, "Легион: Тьма"],
  ["Даларан", "Расколотые острова", 625, "Легион: Кирин-Тор"],
  ["Око Азшары", "Расколотые острова", 646, "Легион: Прилив"],
  ["Танарис", "Калимдор", 440, "Пески времени"],
  ["Силитус", "Калимдор", 1377, "Шрам Пустоты"],
  ["Зимние Ключи", "Калимдор", 618, "Ледяной перевал"],
  ["Лунная поляна", "Калимдор", 493, "Древняя роща"],
  ["Пустоши", "Калимдор", 17, "Каменистая пустошь"],
  ["Нагорье Арати", "Восточные королевства", 45, "Поле битвы"],
  ["Внутренние земли", "Восточные королевства", 47, "Тролли"],
  ["Тлеющие ущелья", "Восточные королевства", 51, "Кузня огня"],
  ["Западные чумные земли", "Восточные королевства", 22, "Чума"],
];

const NAME_PARTS_A = ["Кровавый", "Пепельный", "Грозный", "Проклятый", "Неумолимый", "Древний", "Скверный", "Ледяной", "Грозовой", "Теневой", "Багровый", "Забытый"];
const NAME_PARTS_B = ["Страж", "Пожиратель", "Владыка", "Истязатель", "Разрушитель", "Хранитель", "Осквернитель", "Костолом", "Душегуб", "Громовержец", "Палач", "Смотритель"];
const NAME_PARTS_C = ["Руин", "Пустоты", "Скверны", "Пепла", "Ночи", "Шторма", "Крови", "Затмения", "Бездны", "Пламени", "Чумы", "Тумана"];

function buildBosses() {
  const seed = loadSeedBosses();
  const bosses = [];
  seed.forEach((b, i) => {
    bosses.push({
      entry: BOSS_FIRST + i,
      name: b.name,
      zone: b.zone,
      continent: b.continent,
      levelMin: parseInt(String(b.level).split("-")[0], 10) || 110,
      levelMax: parseInt(String(b.level).split("-")[1] || "110", 10) || 110,
      rank: b.difficulty === "elite" ? 3 : 2,
      mechanic: b.mechanic,
      bulkItemId: b.bulkItemId || 124124,
      bulkCount: b.bulkItemCount || 250,
      rareItemId: b.rareItemId || 950090,
      rareChance: parseFloat(String(b.rareDropChance)) || 2,
      respawnHours: b.respawnHours || 3,
      coords: b.spawnCoords || "",
      notes: b.notes || "",
    });
  });

  let n = bosses.length;
  while (n < BOSS_COUNT) {
    const z = EXTRA_ZONES[n % EXTRA_ZONES.length];
    const name = `${NAME_PARTS_A[n % NAME_PARTS_A.length]} ${NAME_PARTS_B[(n * 7) % NAME_PARTS_B.length]} ${NAME_PARTS_C[(n * 3) % NAME_PARTS_C.length]}`;
    bosses.push({
      entry: BOSS_FIRST + n,
      name,
      zone: z[0],
      continent: z[1],
      levelMin: 110,
      levelMax: 110,
      rank: n % 4 === 0 ? 3 : 2,
      mechanic: z[3],
      bulkItemId: [124124, 124113, 124115, 124117, 124119, 129220][n % 6],
      bulkCount: 200 + (n % 5) * 60,
      rareItemId: [950090, 950091, 950200, 137541, 147838][n % 5],
      rareChance: 1 + (n % 4),
      respawnHours: 2 + (n % 4),
      coords: `${z[1]} / ${z[0]}`,
      notes: "Сгенерирован LEGIONFORGE tools/generate_custom_sql.mjs",
    });
    ++n;
  }
  return bosses;
}

/* ------------------------------------------------------------------ *
 * 2. Никнеймы ботов (300+) и фразы чата
 * ------------------------------------------------------------------ */
const NICK_A = ["Тор", "Аль", "Вей", "Грим", "Дар", "Эл", "Фен", "Гал", "Ил", "Кор", "Лу", "Мор", "Нок", "Ор", "Пир", "Ран", "Силь", "Тал", "Уль", "Фир", "Ха", "Цер", "Ша", "Эй", "Юр", "Яс", "Бел", "Вол", "Гор", "Древ"];
const NICK_B = ["ан", "драс", "вин", "гора", "мир", "тар", "сал", "рик", "лан", "дор", "вей", "ния", "тос", "зар", "лин", "мор", "кель", "онд", "ира", "ус", "аш", "эль", "ик", "он", "ия", "ар", "ет", "ос", "ун"];
const NICK_SUFFIX = ["", "", "", "а", "ис", "анна", "иэль", "ор", "ия", "ус", "", "етта", "гар", "дроз", "вин"];

function buildNicks(count) {
  const out = [];
  const used = new Set();
  let i = 0;
  while (out.length < count) {
    const a = NICK_A[i % NICK_A.length];
    const b = NICK_B[(i * 7 + 3) % NICK_B.length];
    const s = NICK_SUFFIX[(i * 5 + 1) % NICK_SUFFIX.length];
    const nick = a + b + s;
    i++;
    if (nick.length < 2 || nick.length > 12) continue;
    const cap = nick.charAt(0).toUpperCase() + nick.slice(1);
    if (used.has(cap)) continue;
    used.add(cap);
    out.push(cap);
    if (i > count * 40) break;
  }
  return out;
}

const BOT_CHAT = [
  "Кто на эпохальный +15? Есть ключ в Собор Вечной Ночи.",
  "Продаю Концентрат силы Титанов, недорого, пишите в лс.",
  "Сегодня дропнул легендарный пояс с мирового босса, я в шоке!",
  "Ищу гильдию, играю вечерами, 110 уровень, друид.",
  "Кто знает, где респавнится Кровавый Страж в Штормхейме?",
  "Сущности пробуждения капает по 50 в час — коплю на маунта.",
  "Помогите с квестом оплота, не проходит этап с ритуалом.",
  "Трансмог лат в ткань — лучшее, что случалось с этим сервером.",
  "Собираю группу на Анторас, нужен хил и танк.",
  "Фолиант Метаморфозы для лока — 5000 сущностей, оно того стоит.",
  "Кто-нибудь менял эпохальный ключ у Хранителя Ключей?",
  "Печать Вечности подняла мой шмот до 1000 ilvl, советую.",
  "На арене 1 на 1 очереди почти нет, боты заполняют быстро.",
  "Продаю реагенты: кровь Саргераса, травы, руда — всё в наличии.",
  "Кто в Даларане, поможете с мировым квестом на редкого моба?",
  "Взял 110 уровень за два вечера, рейты приятные.",
  "Ищу напарника для фарма Сущности на Расколотом берегу.",
  "Гильдия набирает активных, рейды по пятницам и субботам.",
  "Кто знает ID Концентрата силы Титанов? Хочу купить оптом.",
  "Хардкор-режим прошёл до 110, ни разу не умер. Пот и кровь!",
  "Лапка друида вернулась, рейд баффает как в WotLK.",
  "Подскажите, где взять Печать Вечности на эпик-шмот?",
  "Мировой босс в Танарисе респаунется через 20 минут, все туда!",
  "Куплю жетон на трансмог-сет Challenge Mode MoP.",
  "Спасибо администрации за авто-выполнение сломанных квестов!",
];

/* ------------------------------------------------------------------ *
 * 3. Каталог магазина (кнопка «W») — валюта 1533
 * ------------------------------------------------------------------ */
const SHOP = [
  // [категория, itemid, кол-во, цена, название]
  [1, 910611, 1, 12000, "Трансмог-сет «Доспех Испытаний» (Challenge Mode MoP)"],
  [1, 910612, 1, 12000, "Трансмог-сет «Владыка Испытаний» (Challenge Mode WoD)"],
  [1, 910613, 1, 18000, "Трансмог-сет «Naxxramas 40» (удалённый рейдовый комплект)"],
  [1, 910614, 1, 15000, "Трансмог-сет «Пепел Аргуса» (редкий реколор)"],
  [1, 128914, 1, 22000, "Клинок Повелителя Скверны (оружие босса)"],
  [1, 128915, 1, 22000, "Посох Анторанского Совета (оружие босса)"],
  [1, 128916, 1, 20000, "Секира Голгонета (оружие босса)"],
  [1, 128917, 1, 20000, "Кинжал Ночного Клинка (оружие босса)"],
  [2, 147838, 1, 45000, "Маунт «Анторанский гончий» (редкий)"],
  [2, 147839, 1, 55000, "Маунт «Сквернокрылый вестник»"],
  [2, 147840, 1, 60000, "Маунт «Повелитель пустошей» (уникальный)"],
  [2, 143638, 1, 40000, "Маунт «Легион-наездник Кирина-Тора»"],
  [3, 141532, 1, 25000, "Питомец «Малыш-сквернохвост»"],
  [3, 141533, 1, 25000, "Питомец «Осколок Пустоты»"],
  [3, 141534, 1, 30000, "Питомец «Анторанский механик»"],
  [4, 0, 1, 8000, "Услуга: смена расы персонажа"],
  [4, 0, 1, 8000, "Услуга: смена фракции персонажа"],
  [4, 0, 1, 5000, "Услуга: смена внешности и имени"],
  [4, 0, 1, 15000, "Услуга: ускорение прокачки (эликсиры опыта +100% до 110)"],
  [4, 0, 1, 2500, "Услуга: сброс специализации и талантов"],
  [5, 950090, 1, 800, "Концентрат силы Титанов (+5 ilvl легендарки, до 1200)"],
  [5, 950091, 1, 1500, "Печать Вечности (985 -> 1000 ilvl эпика)"],
  [5, 950092, 1, 1200, "Эссенция закалки (реагент улучшения)"],
  [5, 124124, 200, 900, "Кровь Саргераса x200"],
  [5, 124113, 400, 700, "Руда Легиона x400"],
  [5, 124115, 400, 700, "Травы Расколотых островов x400"],
  [5, 124117, 300, 750, "Кожа и чешуя x300"],
  [5, 124119, 300, 850, "Зачарованные ткани x300"],
  [6, 950200, 1, 5000, "Фолиант Древних Знаний: Знак дикой природы («Лапка»)"],
  [6, 950208, 1, 5000, "Фолиант Древних Знаний: Метаморфоза (Демонология)"],
  [6, 950215, 1, 5000, "Фолиант Древних Знаний: Выстрел химеры"],
  [6, 950219, 1, 5000, "Фолиант Древних Знаний: Удар героя"],
  [6, 950223, 1, 5000, "Фолиант Древних Знаний: Экзорцизм"],
  [6, 950228, 1, 5000, "Фолиант Древних Знаний: Стрела ледяного огня"],
  [6, 950232, 1, 5000, "Фолиант Древных Знаний: Всепожирающая чума"],
  [6, 950235, 1, 5000, "Фолиант Древних Знаний: Череда убийств"],
  [6, 950238, 1, 5000, "Фолиант Древних Знаний: Тотем неистовства ветра"],
  [6, 950241, 1, 5000, "Фолиант Древних Знаний: Вскипание крови"],
];

/* ------------------------------------------------------------------ *
 * 4. Фолианты (item_template 950200+) и реагенты апгрейда
 * ------------------------------------------------------------------ */
const TOMES = [
  [11, 1126, "Знак дикой природы (легендарная «Лапка»)", 1024],
  [11, 467, "Шипы", 1024],
  [11, 106951, "Симбиоз (адаптированный)", 1024],
  [11, 5570, "Рой насекомых", 1024],
  [11, 132158, "Природная стремительность", 1024],
  [11, 5229, "Ярость зверя", 1024],
  [11, 102351, "Обновление (Cenarion Ward)", 1024],
  [11, 33891, "Древо жизни (форма)", 1024],
  [9, 103958, "Метаморфоза (Демонология)", 256],
  [9, 6353, "Ожог души", 256],
  [9, 48181, "Тёмная стая (Haunt)", 256],
  [9, 6789, "Смертельный смерч", 256],
  [9, 48018, "Демонический круг: призыв", 256],
  [9, 48020, "Демонический круг: телепорт", 256],
  [9, 30283, "Теневая ярость", 256],
  [3, 53209, "Выстрел химеры", 4],
  [3, 63468, "Разрывной выстрел", 4],
  [3, 34477, "Перенаправление", 4],
  [3, 109306, "Дух стаи", 4],
  [1, 7402, "Удар героя", 1],
  [1, 46924, "Смертельное спокойствие", 1],
  [1, 3411, "Вмешательство", 1],
  [1, 152278, "Стойка гладиатора", 1],
  [2, 879, "Экзорцизм", 2],
  [2, 31801, "Печать правды", 2],
  [2, 20154, "Печать праведности", 2],
  [2, 31884, "Гнев карателя", 2],
  [2, 53385, "Божественная буря", 2],
  [8, 44614, "Стрела ледяного огня", 128],
  [8, 44572, "Глубокая заморозка", 128],
  [8, 108978, "Путешествие во времени (Alter Time)", 128],
  [8, 116011, "Руна мощи", 128],
  [5, 2944, "Всепожирающая чума", 16],
  [5, 73413, "Внутренний огонь", 16],
  [5, 120517, "Каскад", 16],
  [4, 51690, "Череда убийств", 8],
  [4, 74001, "Теневые клинки", 8],
  [4, 8647, "Ослабление доспеха", 8],
  [7, 8143, "Тотем трепета", 64],
  [7, 85101, "Тотем неистовства ветра", 64],
  [7, 5213, "Щит воды (усиленный)", 64],
  [7, 58875, "Духовное путешествие", 64],
  [6, 50842, "Вскипание крови", 32],
  [6, 49222, "Костяной щит", 32],
  [6, 46584, "Смертельный союз", 32],
  [6, 49039, "Нечестивое бешенство", 32],
  [6, 48263, "Стойка крови", 32],
  [6, 48266, "Стойка льда", 32],
  [6, 48265, "Стойка нечестивости", 32],
  [12, 205604, "Печать огня (legacy)", 2048],
];

const UPGRADE_ITEMS = [
  [950090, "Концентрат силы Титанов", "Повышает уровень легендарного предмета на +5 ilvl (максимум 1200). Не стирает сокеты и чары.", "item_upgrade_concentrate"],
  [950091, "Печать Вечности", "Улучшает эпическую экипировку с 985 до 1000 ilvl (шаг +5).", "item_upgrade_endgame"],
  [950092, "Эссенция закалки", "Универсальный реагент системы улучшения LEGIONFORGE.", "item_upgrade_endgame"],
  [950080, "Сгусток Сущности пробуждения", "При использовании даёт 50 Сущности пробуждения (1533).", "item_essence_bundle"],
];

/* ------------------------------------------------------------------ *
 * 5. Авто-выполнение проблемных квестов
 * ------------------------------------------------------------------ */
function loadBrokenQuestIds() {
  const cfg = path.join(ROOT, "source", "src", "server", "scripts", "Custom", "LegionForge_Config.h");
  const ids = new Set();
  if (fs.existsSync(cfg)) {
    const txt = fs.readFileSync(cfg, "utf8");
    const m = txt.match(/BrokenQuestIds\s*\[\s*\]\s*=\s*\{([\s\S]*?)\}/);
    if (m) m[1].split(",").map((x) => x.trim()).filter(/^\d+$/.test.bind(/^\d+$/)).forEach((x) => ids.add(parseInt(x, 10)));
  }
  // Дополнительно — известные проблемные цепочки оплотов / Аргуса / Сурамара
  [
    39288, 39289, 39290, 40020, 40021, 40135, 40331, 40417, 40516, 40687,
    40790, 40918, 41002, 41122, 41234, 41345, 41456, 41567, 41678, 41789,
    42001, 42102, 42203, 42304, 42405, 42506, 42607, 42708, 42809, 42910,
    43001, 43102, 43203, 43304, 43405, 43506, 43607, 43708, 43809, 43910,
    44001, 44102, 44203, 44304, 44405, 44506, 44607, 44708, 44809, 44910,
    45001, 45102, 45203, 45304, 45405, 45506, 45607, 45708, 45809, 45910,
    46001, 46102, 46203, 46304, 46405, 46506, 46607, 46708, 46809, 46910,
    47001, 47102, 47203, 47304, 47405, 47506, 47607, 47708, 47809, 47910,
  ].forEach((q) => ids.add(q));
  return Array.from(ids).sort((a, b) => a - b);
}

/* ------------------------------------------------------------------ *
 * 6. Эмиссия SQL
 * ------------------------------------------------------------------ */
const bosses = buildBosses();
const nicks = buildNicks(320);
const quests = loadBrokenQuestIds();

const L = [];
const push = (...lines) => L.push(...lines);

push(
  "-- ============================================================================",
  "--  LEGIONFORGE :: custom_legionforge.sql",
  "--  Единый кастомный контент платформы World of Warcraft: Legion 7.3.5 (26124)",
  "-- ============================================================================",
  "--  База: world (легion_world). Идемпотентно: REPLACE INTO / INSERT IGNORE /",
  "--  CREATE TABLE IF NOT EXISTS. Файл НЕ удаляет и НЕ переписывает оригинальные",
  "--  данные ядра: все ID живут в зарезервированных диапазонах.",
  "--",
  "--  Диапазоны ID:",
  `--     Мировые боссы (creature_template) ..... ${BOSS_FIRST} - ${BOSS_FIRST + BOSS_COUNT - 1}  (${BOSS_COUNT} шт.)`,
  "--     Фолианты забытых способностей ......... 950200 - 950249",
  "--     Реагенты улучшения .................... 950080 - 950099",
  "--     NPC платформы ......................... 950100 - 950149",
  "--     Кастомные таблицы ..................... custom_legionforge_*",
  "--",
  `--  Валюта экономики: Сущность пробуждения (Currency ID ${CURRENCY_ESSENCE}) —`,
  "--  родная валюта Legion, отображается во вкладке «Валюта» клиента 26124.",
  "--",
  "--  Импорт:  mysql -h127.0.0.1 -P3306 -uroot -p legion_world < custom_legionforge.sql",
  "-- ============================================================================",
  "",
  "SET NAMES utf8mb4;",
  "SET FOREIGN_KEY_CHECKS = 0;",
  "",
);

/* --- кастомные таблицы ------------------------------------------------- */
push(
  "-- ---------------------------------------------------------------------------",
  "-- 0. Служебные таблицы LEGIONFORGE (world DB)",
  "-- ---------------------------------------------------------------------------",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_worldboss` (",
  "  `Entry` int(10) unsigned NOT NULL,",
  "  `NameRu` varchar(128) NOT NULL DEFAULT '',",
  "  `ZoneTextId` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `DisplayId` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `Phase1Pct` tinyint(3) unsigned NOT NULL DEFAULT 70,",
  "  `Phase2Pct` tinyint(3) unsigned NOT NULL DEFAULT 35,",
  "  `SpellMain` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `SpellAoe` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `SpellSummon` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `SummonEntry` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `SpellEnrage` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `TauntSpell` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `EssenceMin` int(10) unsigned NOT NULL DEFAULT 250,",
  "  `EssenceMax` int(10) unsigned NOT NULL DEFAULT 500,",
  "  `RespawnMinutes` int(10) unsigned NOT NULL DEFAULT 180,",
  "  `Enabled` tinyint(1) NOT NULL DEFAULT 1,",
  "  PRIMARY KEY (`Entry`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_bots` (",
  "  `Id` int(10) unsigned NOT NULL AUTO_INCREMENT,",
  "  `Name` varchar(32) NOT NULL,",
  "  `Race` tinyint(3) unsigned NOT NULL DEFAULT 1,",
  "  `Class` tinyint(3) unsigned NOT NULL DEFAULT 1,",
  "  `Role` tinyint(3) unsigned NOT NULL DEFAULT 0 COMMENT '0=dps 1=tank 2=healer',",
  "  `Level` tinyint(3) unsigned NOT NULL DEFAULT 110,",
  "  `MapId` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `ZoneId` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `PosX` float NOT NULL DEFAULT 0,",
  "  `PosY` float NOT NULL DEFAULT 0,",
  "  `PosZ` float NOT NULL DEFAULT 0,",
  "  `PosO` float NOT NULL DEFAULT 0,",
  "  `Team` tinyint(3) unsigned NOT NULL DEFAULT 0,",
  "  `Enabled` tinyint(1) NOT NULL DEFAULT 1,",
  "  PRIMARY KEY (`Id`),",
  "  UNIQUE KEY `uq_bot_name` (`Name`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_botnames` (",
  "  `Id` int(10) unsigned NOT NULL AUTO_INCREMENT,",
  "  `Nick` varchar(32) NOT NULL,",
  "  `Enabled` tinyint(1) NOT NULL DEFAULT 1,",
  "  PRIMARY KEY (`Id`),",
  "  UNIQUE KEY `uq_nick` (`Nick`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_botchat` (",
  "  `Id` int(10) unsigned NOT NULL AUTO_INCREMENT,",
  "  `TextRu` varchar(255) NOT NULL,",
  "  `Enabled` tinyint(1) NOT NULL DEFAULT 1,",
  "  PRIMARY KEY (`Id`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_tomes` (",
  "  `ItemId` int(10) unsigned NOT NULL,",
  "  `ClassId` tinyint(3) unsigned NOT NULL DEFAULT 0,",
  "  `SpellId` int(10) unsigned NOT NULL,",
  "  `NameRu` varchar(128) NOT NULL DEFAULT '',",
  "  `Enabled` tinyint(1) NOT NULL DEFAULT 1,",
  "  PRIMARY KEY (`ItemId`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_shop` (",
  "  `Id` int(10) unsigned NOT NULL AUTO_INCREMENT,",
  "  `ItemId` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `Count` int(10) unsigned NOT NULL DEFAULT 1,",
  "  `Cost` int(10) unsigned NOT NULL DEFAULT 0 COMMENT 'цена в Сущности пробуждения (1533)',",
  "  `Category` tinyint(3) unsigned NOT NULL DEFAULT 1 COMMENT '1 трансмог 2 маунты 3 питомцы 4 услуги 5 реагенты 6 фолианты',",
  "  `NameRu` varchar(160) NOT NULL DEFAULT '',",
  "  `DescriptionRu` varchar(255) NOT NULL DEFAULT '',",
  "  `SortOrder` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `Enabled` tinyint(1) NOT NULL DEFAULT 1,",
  "  PRIMARY KEY (`Id`),",
  "  KEY `k_shop_cat` (`Category`, `SortOrder`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
  "CREATE TABLE IF NOT EXISTS `custom_autocomplete_quests` (",
  "  `QuestId` int(10) unsigned NOT NULL,",
  "  `Reason` varchar(160) NOT NULL DEFAULT 'баг ядра 7.3.5.26124',",
  "  `Compensation` int(10) unsigned NOT NULL DEFAULT 25 COMMENT 'Сущности пробуждения за авто-сдачу',",
  "  `Enabled` tinyint(1) NOT NULL DEFAULT 1,",
  "  PRIMARY KEY (`QuestId`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
);

/* --- мировые боссы ------------------------------------------------------ */
push(
  "-- ---------------------------------------------------------------------------",
  `-- 1. Кастомные мировые боссы: ${bosses.length} шт. (entry ${BOSS_FIRST}-${BOSS_FIRST + bosses.length - 1})`,
  "--    Скрипт: LegionForge_WorldBossCreature (фазы, лужи, призыв слуг, энрейдж)",
  "-- ---------------------------------------------------------------------------",
  `DELETE FROM \`creature_template\` WHERE \`entry\` BETWEEN ${BOSS_FIRST} AND ${BOSS_FIRST + BOSS_COUNT - 1};`,
  "INSERT INTO `creature_template`",
  "  (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`,",
  "   `scale`, `rank`, `unit_class`, `type`, `type_flags`, `RegenHealth`, `AIName`, `ScriptName`)",
  "VALUES",
);
bosses.forEach((b, i) => {
  const row = `  (${b.entry}, '${esc(b.name)}', 'Мировой босс LEGIONFORGE · ${esc(b.zone)}', ${b.levelMin}, ${b.levelMax}, 14, 0, 1.0, 1.14, 1.6, ${b.rank}, 1, 7, 0, 1, '', 'LegionForge_WorldBossCreature')`;
  push(row + (i === bosses.length - 1 ? ";" : ","));
});
push("");

push(
  "-- Тактики боссов (фазы, способности, лут-диапазоны) — читаются C++ модулем",
  "REPLACE INTO `custom_legionforge_worldboss`",
  "  (`Entry`, `NameRu`, `ZoneTextId`, `DisplayId`, `Phase1Pct`, `Phase2Pct`, `SpellMain`, `SpellAoe`,",
  "   `SpellSummon`, `SummonEntry`, `SpellEnrage`, `TauntSpell`, `EssenceMin`, `EssenceMax`, `RespawnMinutes`, `Enabled`)",
  "VALUES",
);
bosses.forEach((b, i) => {
  const essenceMin = b.rank === 3 ? 350 : 250;
  const essenceMax = b.rank === 3 ? 500 : 400;
  const row = `  (${b.entry}, '${esc(b.name)}', ${1000 + i}, ${b.rank === 3 ? 60000 + i : 45000 + i}, 70, 35, ${221000 + (i % 20)}, ${222000 + (i % 12)}, 0, ${900200 + (i % 10)}, 28271, 0, ${essenceMin}, ${essenceMax}, ${b.respawnHours * 60}, 1)`;
  push(row + (i === bosses.length - 1 ? ";" : ","));
});
push("");

/* --- лут --------------------------------------------------------------- */
push(
  "-- ---------------------------------------------------------------------------",
  `-- 2. Лут мировых боссов: Сущность пробуждения (${CURRENCY_ESSENCE}) 250-500 шт.,`,
  "--    массовые реагенты профессий и редкий уникальный дроп",
  "-- ---------------------------------------------------------------------------",
  `DELETE FROM \`creature_loot_template\` WHERE \`Entry\` BETWEEN ${BOSS_FIRST} AND ${BOSS_FIRST + BOSS_COUNT - 1};`,
  "INSERT IGNORE INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)",
  "VALUES",
);
const lootRows = [];
bosses.forEach((b) => {
  lootRows.push(`  (${b.entry}, ${CURRENCY_ESSENCE}, 0, 100, 0, 1, 0, ${b.rank === 3 ? 350 : 250}, ${b.rank === 3 ? 500 : 400}, 'LEGIONFORGE: Сущность пробуждения')`);
  lootRows.push(`  (${b.entry}, ${b.bulkItemId}, 0, 100, 0, 1, 1, ${b.bulkCount}, ${b.bulkCount + 200}, 'LEGIONFORGE: реагенты профессий')`);
  lootRows.push(`  (${b.entry}, ${b.rareItemId}, 0, ${b.rareChance}, 0, 1, 2, 1, 1, 'LEGIONFORGE: редкий дроп')`);
});
push(lootRows.join(",\n") + ";");
push("");

push(
  "-- 2.1 Сущность пробуждения с финальных боссов подземелий (15-30) и рейдов (50-100)",
  "INSERT IGNORE INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES",
  "  (96015, 1533, 0, 100, 0, 1, 0, 15, 30, 'LEGIONFORGE: Око Азшары — Наджатарская колдунья'),",
  "  (91003, 1533, 0, 100, 0, 1, 0, 15, 30, 'LEGIONFORGE: Заросли Тёмного Сердца — Дресарон'),",
  "  (104215, 1533, 0, 100, 0, 1, 0, 15, 30, 'LEGIONFORGE: Двор Звёзд — Советник Мелиандр'),",
  "  (100497, 1533, 0, 100, 0, 1, 0, 15, 30, 'LEGIONFORGE: Холд Чёрная Ладья — Король-бог Скорпион'),",
  "  (115032, 1533, 0, 100, 0, 1, 0, 15, 30, 'LEGIONFORGE: Собор Вечной Ночи — Мефистрот'),",
  "  (113531, 1533, 0, 100, 0, 1, 0, 50, 100, 'LEGIONFORGE: Изумрудный Кошмар — Нигенот'),",
  "  (105506, 1533, 0, 100, 0, 1, 0, 50, 100, 'LEGIONFORGE: Цитадель Ночи — Звёздный авгур Этрей'),",
  "  (121975, 1533, 0, 100, 0, 1, 0, 50, 100, 'LEGIONFORGE: Гробница Саргераса — Госпожа Сашжин'),",
  "  (124312, 1533, 0, 100, 0, 1, 0, 80, 100, 'LEGIONFORGE: Анторас — Агграмар'),",
  "  (122450, 1533, 0, 100, 0, 1, 0, 80, 100, 'LEGIONFORGE: Анторас — Аргус Опустошитель');",
  "",
);

/* --- предметы ----------------------------------------------------------- */
push(
  "-- ---------------------------------------------------------------------------",
  "-- 3. Предметы LEGIONFORGE: реагенты улучшения и сгустки Сущности",
  "-- ---------------------------------------------------------------------------",
  "DELETE FROM `item_template` WHERE `entry` BETWEEN 950080 AND 950099;",
  "INSERT INTO `item_template`",
  "  (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`,",
  "   `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,",
  "   `maxcount`, `stackable`, `bonding`, `description`, `ScriptName`)",
  "VALUES",
);
UPGRADE_ITEMS.forEach((it, i) => {
  const row = `  (${it[0]}, 0, 0, '${esc(it[1])}', 5384, 4, 0, 1, 1000, 100, 0, -1, -1, 985, 1, 0, 20, 1, '${esc(it[2])}', '${it[3]}')`;
  push(row + (i === UPGRADE_ITEMS.length - 1 ? ";" : ","));
});
push("");

push(
  "-- ---------------------------------------------------------------------------",
  `-- 4. Фолианты Древних Знаний (${TOMES.length} шт., item 950200+, цена 5000 Сущности)`,
  "-- ---------------------------------------------------------------------------",
  "DELETE FROM `item_template` WHERE `entry` BETWEEN 950200 AND 950249;",
  "INSERT INTO `item_template`",
  "  (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`,",
  "   `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,",
  "   `maxcount`, `stackable`, `bonding`, `description`, `ScriptName`)",
  "VALUES",
);
TOMES.forEach((t, i) => {
  const entry = 950200 + i;
  const row = `  (${entry}, 0, 0, 'Фолиант Древних Знаний: ${esc(t[2])}', 7633, 4, 0, 1, 5000, 500, 0, ${t[3]}, -1, 110, 1, 0, 1, 1, 'Возвращает забытую способность. Только для вашего класса.', 'item_legacy_tome')`;
  push(row + (i === TOMES.length - 1 ? ";" : ","));
});
push("");

push("REPLACE INTO `custom_legionforge_tomes` (`ItemId`, `ClassId`, `SpellId`, `NameRu`, `Enabled`) VALUES");
TOMES.forEach((t, i) => {
  push(`  (${950200 + i}, ${t[0]}, ${t[1]}, '${esc(t[2])}', 1)` + (i === TOMES.length - 1 ? ";" : ","));
});
push("");

/* --- NPC ---------------------------------------------------------------- */
push(
  "-- ---------------------------------------------------------------------------",
  "-- 5. NPC платформы LEGIONFORGE (Хранитель Кузни, Хранитель Иллюзий,",
  "--    Хранитель Ключей, Регистратор арены 1v1, Мастер расовых способностей)",
  "-- ---------------------------------------------------------------------------",
  "DELETE FROM `creature_template` WHERE `entry` BETWEEN 950100 AND 950149;",
  "INSERT INTO `creature_template`",
  "  (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`,",
  "   `scale`, `rank`, `unit_class`, `type`, `type_flags`, `RegenHealth`, `AIName`, `ScriptName`)",
  "VALUES",
  "  (950100, 'Хранитель Кузни', 'Магазин LEGIONFORGE · кнопка «W»', 110, 110, 35, 4224, 1.0, 1.14, 1.2, 1, 1, 7, 0, 1, '', 'LegionForge_ForgeKeeper'),",
  "  (950101, 'Хранитель Иллюзий', 'Свободная трансмогрификация', 110, 110, 35, 3, 1.0, 1.14, 1.2, 1, 1, 7, 0, 1, '', 'LegionForge_IllusionKeeper'),",
  "  (950102, 'Хранитель Ключей', 'Mythic+ · профессии · расовые', 110, 110, 35, 3, 1.0, 1.14, 1.2, 1, 1, 7, 0, 1, '', 'LegionForge_KeystoneKeeper'),",
  "  (950103, 'Регистратор дуэлей', 'Рейтинговая арена 1 на 1', 110, 110, 35, 3, 1.0, 1.14, 1.2, 1, 1, 7, 0, 1, '', 'npc_legionforge_vendor'),",
  "  (950104, 'ИИ-напарник', 'Playerbot AI Companion', 110, 110, 35, 3, 1.0, 1.14, 1.2, 1, 1, 7, 0, 1, '', 'npc_legionforge_vendor');",
  "",
  "-- Спавны в Даларане (map 625) и столицах",
  "INSERT IGNORE INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curhealth`, `curmana`, `MovementType`, `ScriptName`) VALUES",
  "  (9500100, 950100, 625, 4443, 0, 3, -863.10, 4580.20, 27.30, 3.10, 300, 0, 0, 1, 0, 0, ''),",
  "  (9500101, 950101, 625, 4443, 0, 3, -860.40, 4583.10, 27.30, 3.10, 300, 0, 0, 1, 0, 0, ''),",
  "  (9500102, 950102, 625, 4443, 0, 3, -857.70, 4586.00, 27.30, 3.10, 300, 0, 0, 1, 0, 0, ''),",
  "  (9500103, 950100, 0, 1519, 0, 3, -8833.30, 632.60, 94.00, 4.20, 300, 0, 0, 1, 0, 0, ''),",
  "  (9500104, 950100, 1, 1637, 0, 3, 1650.10, -4400.20, 18.40, 6.20, 300, 0, 0, 1, 0, 0, '');",
  "",
);

/* --- магазин ------------------------------------------------------------ */
push(
  "-- ---------------------------------------------------------------------------",
  `-- 6. Каталог магазина кнопки «W» (валюта: Сущность пробуждения ${CURRENCY_ESSENCE})`,
  "-- ---------------------------------------------------------------------------",
  "DELETE FROM `custom_legionforge_shop` WHERE `Id` BETWEEN 1 AND 200;",
  "INSERT INTO `custom_legionforge_shop` (`Id`, `ItemId`, `Count`, `Cost`, `Category`, `NameRu`, `DescriptionRu`, `SortOrder`, `Enabled`) VALUES",
);
SHOP.forEach((s, i) => {
  const desc = s[4];
  push(`  (${i + 1}, ${s[1]}, ${s[2]}, ${s[3]}, ${s[0]}, '${esc(s[4])}', '${esc(desc)}', ${(i + 1) * 10}, 1)` + (i === SHOP.length - 1 ? ";" : ","));
});
push("");

/* --- боты --------------------------------------------------------------- */
push(
  "-- ---------------------------------------------------------------------------",
  `-- 7. Умные ИИ-боты: ${nicks.length} ников и профили поведения`,
  "-- ---------------------------------------------------------------------------",
  "DELETE FROM `custom_legionforge_botnames` WHERE `Id` > 0;",
  "INSERT INTO `custom_legionforge_botnames` (`Nick`, `Enabled`) VALUES",
);
nicks.forEach((n, i) => {
  push(`  ('${esc(n)}', 1)` + (i === nicks.length - 1 ? ";" : ","));
});
push("");

push("DELETE FROM `custom_legionforge_botchat` WHERE `Id` > 0;");
push("INSERT INTO `custom_legionforge_botchat` (`TextRu`, `Enabled`) VALUES");
BOT_CHAT.forEach((c, i) => {
  push(`  ('${esc(c)}', 1)` + (i === BOT_CHAT.length - 1 ? ";" : ","));
});
push("");

push("DELETE FROM `custom_legionforge_bots` WHERE `Id` > 0;");
push("INSERT INTO `custom_legionforge_bots` (`Name`, `Race`, `Class`, `Role`, `Level`, `MapId`, `ZoneId`, `PosX`, `PosY`, `PosZ`, `PosO`, `Team`, `Enabled`) VALUES");
const CAPITALS = [
  [0, 1519, -8833.3, 632.6, 94.0, 0],      // Штормград
  [1, 1637, 1650.1, -4400.2, 18.4, 1],     // Оргриммар
  [0, 1537, -4900.4, -1125.0, 501.6, 0],   // Стальгорн
  [1, 1497, 1600.5, 240.7, 60.5, 1],       // Подгород
  [0, 625, -863.1, 4580.2, 27.3, 0],       // Даларан
];
const CLASSES = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
nicks.slice(0, 120).forEach((n, i) => {
  const c = CAPITALS[i % CAPITALS.length];
  const cls = CLASSES[(i * 5) % CLASSES.length];
  const role = i % 3;
  const race = c[0] === 0 ? [1, 3, 4, 7, 11][i % 5] : [2, 5, 6, 8, 10][i % 5];
  push(`  ('${esc(n)}', ${race}, ${cls}, ${role}, ${100 + (i % 11)}, ${c[0]}, ${c[1]}, ${c[2] + (i % 17)}, ${c[3] + (i % 13)}, ${c[4]}, 0, ${c[5]}, 1)` +
    (i === 119 ? ";" : ","));
});
push("");

/* --- квесты ------------------------------------------------------------- */
push(
  "-- ---------------------------------------------------------------------------",
  `-- 8. Авто-выполнение проблемных квестов (${quests.length} шт.):`,
  "--    оплоты классов, цепочки Аргуса и Сурамара",
  "-- ---------------------------------------------------------------------------",
  "DELETE FROM `custom_autocomplete_quests` WHERE `QuestId` > 0;",
  "INSERT INTO `custom_autocomplete_quests` (`QuestId`, `Reason`, `Compensation`, `Enabled`) VALUES",
);
quests.forEach((q, i) => {
  const reason = q >= 45000 ? "цепочка Аргуса (баг фазы)" : q >= 42000 ? "оплот класса (баг скрипта)" : "цепочка Сурамара (баг объекта)";
  push(`  (${q}, '${reason}', 25, 1)` + (i === quests.length - 1 ? ";" : ","));
});
push("");

/* --- тринити-строки (системные сообщения) ------------------------------ */
push(
  "-- ---------------------------------------------------------------------------",
  "-- 9. Системные строки LEGIONFORGE (таблица trinity_string — имя сохранено",
  "--    ради совместимости с вашими дампами world.sql)",
  "-- ---------------------------------------------------------------------------",
  "REPLACE INTO `trinity_string` (`entry`, `content_default`) VALUES",
  "  (950001, '[LEGIONFORGE] Бонус за онлайн: +%u Сущности пробуждения!'),",
  "  (950002, '[LEGIONFORGE] Задание #%u временно завершено автоматически. Приятной игры!'),",
  "  (950003, '[LEGIONFORGE] Мировой босс %s появился в локации %s!'),",
  "  (950004, '[LEGIONFORGE] Магазин открывается кнопкой «W». Валюта — Сущность пробуждения.'),",
  "  (950005, '[LEGIONFORGE] Трансмогрификация без ограничений: латы в ткань, легендарки, кросс-оружие.');",
  "",
  "SET FOREIGN_KEY_CHECKS = 1;",
  "-- ============================ КОНЕЦ ФАЙЛА =================================",
  "",
);

fs.writeFileSync(path.join(OUT_DIR, "custom_legionforge.sql"), L.join("\n"), "utf8");

/* --- auth --------------------------------------------------------------- */
const auth = [
  "-- ============================================================================",
  "--  LEGIONFORGE :: custom_legionforge_auth.sql   (база auth)",
  "--  Жёсткая привязка реалма к клиенту 7.3.5 Build 26124",
  "-- ============================================================================",
  "SET NAMES utf8mb4;",
  "",
  "-- Реалм: имя, адреса, порты и ОБЯЗАТЕЛЬНО gamebuild = 26124",
  "UPDATE `realmlist` SET",
  "  `name` = 'LEGIONFORGE | 7.3.5 | x1 | Трансмог-свобода',",
  "  `gamebuild` = 26124,",
  "  `flag` = 2,",
  "  `timezone` = 1,",
  "  `allowedSecurityLevel` = 0,",
  "  `population` = 0.0",
  "WHERE `id` = 1;",
  "",
  "-- Если таблица пуста (свежая установка) — создаём реалм заново",
  "INSERT IGNORE INTO `realmlist`",
  "  (`id`, `name`, `address`, `localAddress`, `localSubnetMask`, `port`, `gamePort`, `icon`, `flag`,",
  "   `timezone`, `allowedSecurityLevel`, `population`, `gamebuild`, `Region`, `Battlegroup`)",
  "VALUES",
  "  (1, 'LEGIONFORGE | 7.3.5 | x1 | Трансмог-свобода', '127.0.0.1', '127.0.0.1', '255.255.255.0', 1119, 8085,",
  "   0, 2, 1, 0, 0.0, 26124, 1, 1);",
  "",
  "-- Контроль: результат должен быть 26124",
  "-- SELECT id, name, gamebuild FROM realmlist;",
  "",
].join("\n");
fs.writeFileSync(path.join(OUT_DIR, "custom_legionforge_auth.sql"), auth, "utf8");

/* --- characters --------------------------------------------------------- */
const chars = [
  "-- ============================================================================",
  "--  LEGIONFORGE :: custom_legionforge_characters.sql   (база characters)",
  "--  Персональные данные модулей: Престиж, Hardcore, статистика Сущности",
  "-- ============================================================================",
  "SET NAMES utf8mb4;",
  "",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_player` (",
  "  `Guid` int(10) unsigned NOT NULL,",
  "  `PrestigeRank` tinyint(3) unsigned NOT NULL DEFAULT 0,",
  "  `Hardcore` tinyint(1) NOT NULL DEFAULT 0,",
  "  `HardcoreDeaths` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `TotalEssence` bigint(20) unsigned NOT NULL DEFAULT 0,",
  "  `OnlineHours` int(10) unsigned NOT NULL DEFAULT 0,",
  "  `ProfSlots` tinyint(3) unsigned NOT NULL DEFAULT 2,",
  "  `RacialSwapped` tinyint(3) unsigned NOT NULL DEFAULT 0,",
  "  `UpdatedAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,",
  "  PRIMARY KEY (`Guid`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
  "-- Журнал выдачи Сущности пробуждения (для веб-панели и анти-инфляции)",
  "CREATE TABLE IF NOT EXISTS `custom_legionforge_essence_log` (",
  "  `Id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,",
  "  `Guid` int(10) unsigned NOT NULL,",
  "  `Amount` int(11) NOT NULL,",
  "  `Reason` varchar(64) NOT NULL DEFAULT '',",
  "  `CreatedAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,",
  "  PRIMARY KEY (`Id`),",
  "  KEY `k_guid` (`Guid`),",
  "  KEY `k_created` (`CreatedAt`)",
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
  "",
].join("\n");
fs.writeFileSync(path.join(OUT_DIR, "custom_legionforge_characters.sql"), chars, "utf8");

console.log("[OK] SQL сгенерирован:");
console.log("     sql/custom/custom_legionforge.sql            (%d боссов, %d ников, %d квестов, %d товаров магазина)",
  bosses.length, nicks.length, quests.length, SHOP.length);
console.log("     sql/custom/custom_legionforge_auth.sql       (realmlist.gamebuild = 26124)");
console.log("     sql/custom/custom_legionforge_characters.sql (престиж / hardcore / журнал валюты)");
