import { db } from "@/db";
import { lfSettings } from "@/db/schema";
import { eq } from "drizzle-orm";
import { readConf, writeConfValues } from "@/lib/platform";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

export async function GET() {
  await ensureSeededSafe();
  const rows = await db.select().from(lfSettings).orderBy(lfSettings.category, lfSettings.key);
  const conf = readConf("worldserver.conf");
  return Response.json({
    settings: rows,
    conf: { file: "server/configs/worldserver.conf", keys: conf.values, lines: conf.text.split(/\r?\n/).length },
  });
}

export async function PATCH(req: Request) {
  const body = (await req.json()) as { updates?: Record<string, string>; applyToConf?: boolean };
  const updates = body.updates ?? {};
  for (const [key, value] of Object.entries(updates)) {
    await db.update(lfSettings).set({ value: String(value), updatedAt: new Date() }).where(eq(lfSettings.key, key));
  }
  let confResult = { ok: false, changed: 0, error: "не применялось" } as { ok: boolean; changed: number; error?: string };
  if (body.applyToConf !== false) {
    const confUpdates: Record<string, string> = {};
    for (const [key, value] of Object.entries(updates)) {
      if (/^[A-Za-z0-9_.]+$/.test(key)) confUpdates[key] = String(value);
    }
    if (Object.keys(confUpdates).length) confResult = writeConfValues("worldserver.conf", confUpdates);
  }
  return Response.json({ ok: true, updated: Object.keys(updates).length, conf: confResult });
}
