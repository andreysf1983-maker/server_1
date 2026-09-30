import { db } from "@/db";
import { sql } from "drizzle-orm";
import { platformHealth, CLIENT_BUILD } from "@/lib/platform";

export const dynamic = "force-dynamic";

export async function GET() {
  try {
    await db.execute(sql`select 1`);
    const health = platformHealth();
    return Response.json({
      ok: true,
      platform: "LEGIONFORGE",
      clientBuild: CLIENT_BUILD,
      buildConfigured: health.buildConfigured,
      buildOk: health.buildOk,
      readyScore: health.readyScore,
    });
  } catch {
    return Response.json({ ok: false }, { status: 500 });
  }
}
