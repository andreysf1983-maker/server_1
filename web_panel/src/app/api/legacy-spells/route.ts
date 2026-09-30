import { db } from "@/db";
import { lfLegacyTomes, lfQuestFixes } from "@/db/schema";
import { asc } from "drizzle-orm";
import { CLASS_NAMES } from "@/lib/platform";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

export async function GET() {
  await ensureSeededSafe();
  const tomes = await db.select().from(lfLegacyTomes).orderBy(asc(lfLegacyTomes.itemId));
  const quests = await db.select().from(lfQuestFixes).orderBy(asc(lfQuestFixes.questId));
  return Response.json({
    tomes: tomes.map((t) => ({ ...t, className: CLASS_NAMES[t.classId] ?? "Любой класс" })),
    quests,
    tomeCost: 5000,
    currencyId: 1533,
  });
}
