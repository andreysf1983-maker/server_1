import { db } from "@/db";
import {
  lfAccounts, lfBotNames, lfCharacters, lfLegacyTomes, lfOnlineSnapshots,
  lfQuestFixes, lfShopItems, lfWorldBosses,
} from "@/db/schema";
import { sql } from "drizzle-orm";
import { CLIENT_BUILD, PLATFORM_VERSION, countSourceFiles, platformHealth } from "@/lib/platform";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

async function count(table: any): Promise<number> {
  const rows = await db.select({ n: sql<number>`count(*)::int` }).from(table);
  return Number(rows[0]?.n ?? 0);
}

export async function GET() {
  await ensureSeededSafe();
  const health = platformHealth();
  const online = await db.select({ n: sql<number>`count(*)::int` }).from(lfCharacters)
    .where(sql`${lfCharacters.online} = true`);
  const snapshots = await db.select().from(lfOnlineSnapshots).orderBy(lfOnlineSnapshots.at);

  return Response.json({
    platform: { name: "LEGIONFORGE", version: PLATFORM_VERSION, clientVersion: "7.3.5", clientBuild: CLIENT_BUILD },
    health,
    sources: countSourceFiles(),
    stats: {
      online: Number(online[0]?.n ?? 0),
      accounts: await count(lfAccounts),
      characters: await count(lfCharacters),
      shopItems: await count(lfShopItems),
      worldBosses: await count(lfWorldBosses),
      legacyTomes: await count(lfLegacyTomes),
      questFixes: await count(lfQuestFixes),
      botNames: await count(lfBotNames),
    },
    onlineChart: snapshots.map((s) => ({ at: s.at.toISOString(), players: s.players, essence: s.essenceGranted })),
  });
}
