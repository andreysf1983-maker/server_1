import {
  boolean,
  bigint,
  integer,
  pgTable,
  serial,
  text,
  timestamp,
} from "drizzle-orm/pg-core";

/**
 * LEGIONFORGE :: схема хранилища веб-панели (PostgreSQL + Drizzle).
 * Панель держит настройки платформы, каталог магазина, реестр мировых боссов,
 * фолианты, зеркало игровых аккаунтов/персонажей и телеметрию онлайна.
 */

export const lfSettings = pgTable("lf_settings", {
  key: text("key").primaryKey(),
  value: text("value").notNull(),
  label: text("label").notNull().default(""),
  category: text("category").notNull().default("general"),
  type: text("type").notNull().default("text"), // text | number | boolean
  hint: text("hint").notNull().default(""),
  updatedAt: timestamp("updated_at").defaultNow().notNull(),
});

export const lfModules = pgTable("lf_modules", {
  key: text("key").primaryKey(),
  name: text("name").notNull(),
  description: text("description").notNull().default(""),
  category: text("category").notNull().default("gameplay"),
  enabled: boolean("enabled").notNull().default(true),
  configKey: text("config_key").notNull().default(""),
});

export const lfShopItems = pgTable("lf_shop_items", {
  id: serial("id").primaryKey(),
  itemId: integer("item_id").notNull().default(0),
  count: integer("count").notNull().default(1),
  cost: integer("cost").notNull().default(0),
  category: integer("category").notNull().default(1),
  nameRu: text("name_ru").notNull(),
  descriptionRu: text("description_ru").notNull().default(""),
  sortOrder: integer("sort_order").notNull().default(0),
  enabled: boolean("enabled").notNull().default(true),
  updatedAt: timestamp("updated_at").defaultNow().notNull(),
});

export const lfWorldBosses = pgTable("lf_world_bosses", {
  entry: integer("entry").primaryKey(),
  nameRu: text("name_ru").notNull(),
  zone: text("zone").notNull().default(""),
  continent: text("continent").notNull().default(""),
  levelMin: integer("level_min").notNull().default(110),
  levelMax: integer("level_max").notNull().default(110),
  rank: integer("rank").notNull().default(2),
  essenceMin: integer("essence_min").notNull().default(250),
  essenceMax: integer("essence_max").notNull().default(500),
  respawnMinutes: integer("respawn_minutes").notNull().default(180),
  displayId: integer("display_id").notNull().default(0),
  spellMain: integer("spell_main").notNull().default(0),
  spellAoe: integer("spell_aoe").notNull().default(0),
  summonEntry: integer("summon_entry").notNull().default(0),
  spellEnrage: integer("spell_enrage").notNull().default(0),
  enabled: boolean("enabled").notNull().default(true),
});

export const lfLegacyTomes = pgTable("lf_legacy_tomes", {
  itemId: integer("item_id").primaryKey(),
  classId: integer("class_id").notNull().default(0),
  spellId: integer("spell_id").notNull().default(0),
  nameRu: text("name_ru").notNull(),
  enabled: boolean("enabled").notNull().default(true),
});

export const lfQuestFixes = pgTable("lf_quest_fixes", {
  questId: integer("quest_id").primaryKey(),
  reason: text("reason").notNull().default(""),
  compensation: integer("compensation").notNull().default(25),
  enabled: boolean("enabled").notNull().default(true),
});

export const lfAccounts = pgTable("lf_accounts", {
  id: serial("id").primaryKey(),
  username: text("username").notNull().unique(),
  email: text("email").notNull().default(""),
  gmLevel: integer("gm_level").notNull().default(0),
  expansion: integer("expansion").notNull().default(6),
  essence: bigint("essence", { mode: "number" }).notNull().default(0),
  banned: boolean("banned").notNull().default(false),
  lastLogin: timestamp("last_login"),
  createdAt: timestamp("created_at").defaultNow().notNull(),
});

export const lfCharacters = pgTable("lf_characters", {
  id: serial("id").primaryKey(),
  guid: integer("guid").notNull().unique(),
  name: text("name").notNull(),
  account: text("account").notNull().default(""),
  classId: integer("class_id").notNull().default(1),
  raceId: integer("race_id").notNull().default(1),
  level: integer("level").notNull().default(110),
  ilvl: integer("ilvl").notNull().default(985),
  prestige: integer("prestige").notNull().default(0),
  hardcore: boolean("hardcore").notNull().default(false),
  totalEssence: bigint("total_essence", { mode: "number" }).notNull().default(0),
  online: boolean("online").notNull().default(false),
  zone: text("zone").notNull().default("Даларан"),
});

export const lfOnlineSnapshots = pgTable("lf_online_snapshots", {
  id: serial("id").primaryKey(),
  at: timestamp("at").defaultNow().notNull(),
  players: integer("players").notNull().default(0),
  essenceGranted: integer("essence_granted").notNull().default(0),
});

export const lfEssenceLog = pgTable("lf_essence_log", {
  id: serial("id").primaryKey(),
  at: timestamp("at").defaultNow().notNull(),
  characterName: text("character_name").notNull().default(""),
  amount: integer("amount").notNull().default(0),
  reason: text("reason").notNull().default(""),
});

export const lfBotNames = pgTable("lf_bot_names", {
  id: serial("id").primaryKey(),
  nick: text("nick").notNull().unique(),
  classId: integer("class_id").notNull().default(1),
  role: integer("role").notNull().default(0),
  zone: text("zone").notNull().default(""),
  enabled: boolean("enabled").notNull().default(true),
});
