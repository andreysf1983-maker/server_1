import { db } from "@/db";
import { lfShopItems } from "@/db/schema";
import { eq } from "drizzle-orm";

export const dynamic = "force-dynamic";

type Ctx = { params: Promise<{ id: string }> };

export async function PATCH(req: Request, ctx: Ctx) {
  const { id } = await ctx.params;
  const body = (await req.json()) as Record<string, unknown>;
  const patch: Record<string, unknown> = { updatedAt: new Date() };
  for (const key of ["itemId", "count", "cost", "category", "sortOrder"]) {
    if (key in body) patch[key] = Number(body[key]);
  }
  for (const key of ["nameRu", "descriptionRu"]) {
    if (key in body) patch[key] = String(body[key]);
  }
  if ("enabled" in body) patch.enabled = Boolean(body.enabled);
  const [row] = await db.update(lfShopItems).set(patch).where(eq(lfShopItems.id, Number(id))).returning();
  if (!row) return Response.json({ ok: false, error: "не найдено" }, { status: 404 });
  return Response.json({ ok: true, item: row });
}

export async function DELETE(_req: Request, ctx: Ctx) {
  const { id } = await ctx.params;
  await db.delete(lfShopItems).where(eq(lfShopItems.id, Number(id)));
  return Response.json({ ok: true });
}
