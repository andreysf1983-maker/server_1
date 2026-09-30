import fs from "node:fs";
import path from "node:path";
import { db } from "@/db";
import { lfModules } from "@/db/schema";
import { eq } from "drizzle-orm";
import { CONFIGS_DIR, writeConfValues } from "@/lib/platform";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

export async function GET() {
  await ensureSeededSafe();
  const rows = await db.select().from(lfModules).orderBy(lfModules.category, lfModules.name);
  return Response.json({ modules: rows });
}

export async function PATCH(req: Request) {
  const body = (await req.json()) as { key?: string; enabled?: boolean; syncConf?: boolean };
  if (!body.key) return Response.json({ ok: false, error: "key обязателен" }, { status: 400 });
  await db.update(lfModules).set({ enabled: !!body.enabled }).where(eq(lfModules.key, body.key));

  const row = await db.select().from(lfModules).where(eq(lfModules.key, body.key)).limit(1);
  const configKey = row[0]?.configKey ?? "";
  let confChanged = 0;
  if (body.syncConf !== false && configKey) {
    const res = writeConfValues("worldserver.conf", { [configKey]: body.enabled ? "1" : "0" });
    confChanged = res.changed;
  }

  // Синхронизируем modules.conf
  const modulesFile = path.join(CONFIGS_DIR, "modules.conf");
  if (fs.existsSync(modulesFile)) {
    let text = fs.readFileSync(modulesFile, "utf8");
    const modKey = body.key.replace(/(^|-)(\w)/g, (_m, _p, c: string) => c.toUpperCase());
    const re = new RegExp(`^(Mod\\.${modKey}\\s*=\\s*)(\\d)`, "m");
    if (re.test(text)) {
      text = text.replace(re, (_m, p: string) => `${p}${body.enabled ? 1 : 0}`);
      fs.writeFileSync(modulesFile, text, "utf8");
    }
  }
  return Response.json({ ok: true, configKey, confChanged });
}
