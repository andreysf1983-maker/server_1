import { db } from "@/db";
import { lfWorldBosses, lfOnlineSnapshots } from "@/db/schema";
import { asc, sql } from "drizzle-orm";
import { parseBosses } from "@/lib/parse-sql";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

export async function GET() {
  await ensureSeededSafe();
  const rows = await db.select().from(lfWorldBosses).orderBy(asc(lfWorldBosses.entry));
  const zones = await db.select({ zone: lfWorldBosses.zone, n: sql<number>`count(*)::int` })
    .from(lfWorldBosses).groupBy(lfWorldBosses.zone).orderBy(sql`count(*) desc`).limit(12);
  return Response.json({
    bosses: rows,
    total: rows.length,
    zones,
    sqlFile: "sql/custom/custom_legionforge.sql",
    sourceRows: parseBosses().length,
  });
}

export async function PATCH(req: Request) {
  const body = (await req.json()) as { entry?: number; enabled?: boolean; essenceMin?: number; essenceMax?: number; respawnMinutes?: number };
  if (!body.entry) return Response.json({ ok: false, error: "entry обязателен" }, { status: 400 });
  const patch: Record<string, unknown> = {};
  if ("enabled" in body) patch.enabled = Boolean(body.enabled);
  if ("essenceMin" in body) patch.essenceMin = Number(body.essenceMin);
  if ("essenceMax" in body) patch.essenceMax = Number(body.essenceMax);
  if ("respawnMinutes" in body) patch.respawnMinutes = Number(body.respawnMinutes);
  const [row] = await db.update(lfWorldBosses).set(patch).where(sql`${lfWorldBosses.entry} = ${body.entry}`).returning();
  await db.insert(lfOnlineSnapshots).values({ at: new Date(), players: 0, essenceGranted: 0 });
  return Response.json({ ok: true, boss: row });
}
