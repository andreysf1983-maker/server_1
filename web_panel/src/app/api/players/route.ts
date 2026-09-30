import { db } from "@/db";
import { lfAccounts, lfCharacters, lfBotNames, lfEssenceLog } from "@/db/schema";
import { desc, sql } from "drizzle-orm";
import { CLASS_NAMES, RACE_NAMES } from "@/lib/platform";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

export async function GET() {
  await ensureSeededSafe();
  const accounts = await db.select().from(lfAccounts).orderBy(desc(lfAccounts.essence));
  const characters = await db.select().from(lfCharacters).orderBy(desc(lfCharacters.totalEssence));
  const bots = await db.select().from(lfBotNames).limit(40);
  const log = await db.select().from(lfEssenceLog).orderBy(desc(lfEssenceLog.at)).limit(30);
  const [botCount] = await db.select({ n: sql<number>`count(*)::int` }).from(lfBotNames);

  return Response.json({
    accounts,
    characters: characters.map((c) => ({ ...c, className: CLASS_NAMES[c.classId] ?? "—", raceName: RACE_NAMES[c.raceId] ?? "—" })),
    bots: bots.slice(0, 40),
    botTotal: botCount?.n ?? 0,
    essenceLog: log,
  });
}

export async function POST(req: Request) {
  const body = (await req.json()) as { username?: string; email?: string; gmLevel?: number; expansion?: number };
  if (!body.username) return Response.json({ ok: false, error: "username обязателен" }, { status: 400 });
  const [row] = await db.insert(lfAccounts).values({
    username: String(body.username).toLowerCase(),
    email: String(body.email ?? `${body.username}@legionforge.gg`),
    gmLevel: Number(body.gmLevel ?? 0),
    expansion: Number(body.expansion ?? 6),
    essence: 0, banned: false,
  }).onConflictDoNothing().returning();
  return Response.json({ ok: true, account: row ?? null });
}
