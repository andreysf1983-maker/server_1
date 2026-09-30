import { db } from "@/db";
import { lfAccounts, lfCharacters, lfEssenceLog } from "@/db/schema";
import { eq, sql } from "drizzle-orm";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

/** Выдача/списание Сущности пробуждения (1533) аккаунту или персонажу. */
export async function POST(req: Request) {
  await ensureSeededSafe();
  const body = (await req.json()) as { target?: "account" | "character"; id?: number; amount?: number; reason?: string };
  const amount = Number(body.amount ?? 0);
  if (!body.id || !amount) return Response.json({ ok: false, error: "id и amount обязательны" }, { status: 400 });

  if ((body.target ?? "account") === "account") {
    const [row] = await db.update(lfAccounts)
      .set({ essence: sql`GREATEST(0, ${lfAccounts.essence} + ${amount})` })
      .where(eq(lfAccounts.id, body.id)).returning();
    if (!row) return Response.json({ ok: false, error: "аккаунт не найден" }, { status: 404 });
    await db.insert(lfEssenceLog).values({ characterName: row.username, amount, reason: body.reason ?? "выдача через панель" });
    return Response.json({ ok: true, account: row });
  }

  const [row] = await db.update(lfCharacters)
    .set({ totalEssence: sql`GREATEST(0, ${lfCharacters.totalEssence} + ${amount})` })
    .where(eq(lfCharacters.id, body.id)).returning();
  if (!row) return Response.json({ ok: false, error: "персонаж не найден" }, { status: 404 });
  await db.insert(lfEssenceLog).values({ characterName: row.name, amount, reason: body.reason ?? "выдача через панель" });
  return Response.json({ ok: true, character: row });
}
