import { db } from "@/db";
import { lfSettings, lfShopItems, lfWorldBosses, lfLegacyTomes, lfQuestFixes, lfModules } from "@/db/schema";
import { asc } from "drizzle-orm";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

const esc = (s: string) => String(s).replace(/\\/g, "\\\\").replace(/'/g, "\\'");

/** Генерирует готовый к импорту SQL из текущего состояния панели. */
export async function GET(req: Request) {
  await ensureSeededSafe();
  const what = new URL(req.url).searchParams.get("what") ?? "all";
  const out: string[] = [
    "-- ============================================================================",
    "--  LEGIONFORGE :: экспорт из веб-панели (" + new Date().toISOString() + ")",
    "--  Импорт: mysql -h127.0.0.1 -uroot -p legionforge_world < export.sql",
    "-- ============================================================================",
    "SET NAMES utf8mb4;",
    "",
  ];

  if (what === "all" || what === "shop") {
    const items = await db.select().from(lfShopItems).orderBy(asc(lfShopItems.category));
    out.push("-- Каталог магазина кнопки «W» (валюта 1533 — Сущность пробуждения)");
    out.push("REPLACE INTO `custom_legionforge_shop` (`Id`, `ItemId`, `Count`, `Cost`, `Category`, `NameRu`, `DescriptionRu`, `SortOrder`, `Enabled`) VALUES");
    out.push((items.length ? items.map((i, idx) =>
      `  (${idx + 1}, ${i.itemId}, ${i.count}, ${i.cost}, ${i.category}, '${esc(i.nameRu)}', '${esc(i.descriptionRu)}', ${i.sortOrder}, ${i.enabled ? 1 : 0})`
    ).join(",\n") : "  (1, 0, 1, 0, 1, 'пусто', '', 0, 0)") + ";");
    out.push("");
  }

  if (what === "all" || what === "bosses") {
    const bosses = await db.select().from(lfWorldBosses).orderBy(asc(lfWorldBosses.entry));
    out.push("-- Мировые боссы (тактики и лут)");
    out.push("REPLACE INTO `custom_legionforge_worldboss` (`Entry`, `NameRu`, `ZoneTextId`, `DisplayId`, `Phase1Pct`, `Phase2Pct`, `SpellMain`, `SpellAoe`, `SpellSummon`, `SummonEntry`, `SpellEnrage`, `TauntSpell`, `EssenceMin`, `EssenceMax`, `RespawnMinutes`, `Enabled`) VALUES");
    out.push((bosses.length ? bosses.map((b) =>
      `  (${b.entry}, '${esc(b.nameRu)}', ${1000 + (b.entry % 1000)}, ${b.displayId}, 70, 35, ${b.spellMain}, ${b.spellAoe}, 0, ${b.summonEntry}, ${b.spellEnrage}, 0, ${b.essenceMin}, ${b.essenceMax}, ${b.respawnMinutes}, ${b.enabled ? 1 : 0})`
    ).join(",\n") : "  (900001, 'пусто', 0, 0, 70, 35, 0, 0, 0, 0, 0, 0, 250, 500, 180, 0)") + ";");
    out.push("");
  }

  if (what === "all" || what === "tomes") {
    const tomes = await db.select().from(lfLegacyTomes).orderBy(asc(lfLegacyTomes.itemId));
    out.push("-- Фолианты забытых способностей");
    out.push("REPLACE INTO `custom_legionforge_tomes` (`ItemId`, `ClassId`, `SpellId`, `NameRu`, `Enabled`) VALUES");
    out.push((tomes.length ? tomes.map((t) => `  (${t.itemId}, ${t.classId}, ${t.spellId}, '${esc(t.nameRu)}', ${t.enabled ? 1 : 0})`).join(",\n") : "  (950200, 0, 0, 'пусто', 0)") + ";");
    out.push("");
  }

  if (what === "all" || what === "quests") {
    const quests = await db.select().from(lfQuestFixes).orderBy(asc(lfQuestFixes.questId));
    out.push("-- Авто-выполнение проблемных квестов");
    out.push("REPLACE INTO `custom_autocomplete_quests` (`QuestId`, `Reason`, `Compensation`, `Enabled`) VALUES");
    out.push((quests.length ? quests.map((q) => `  (${q.questId}, '${esc(q.reason)}', ${q.compensation}, ${q.enabled ? 1 : 0})`).join(",\n") : "  (0, 'пусто', 0, 0)") + ";");
    out.push("");
  }

  if (what === "all" || what === "settings") {
    const settings = await db.select().from(lfSettings);
    const modules = await db.select().from(lfModules);
    out.push("-- Настройки worldserver.conf");
    settings.forEach((s) => out.push(`-- ${s.key} = ${s.value}`));
    out.push("");
    out.push("-- Состояние модулей (modules.conf)");
    modules.forEach((m) => out.push(`-- ${m.configKey || m.key} = ${m.enabled ? 1 : 0}`));
    out.push("");
  }

  const body = out.join("\n");
  return new Response(body, {
    headers: {
      "Content-Type": "application/sql; charset=utf-8",
      "Content-Disposition": `attachment; filename="legionforge_export_${what}.sql"`,
    },
  });
}
