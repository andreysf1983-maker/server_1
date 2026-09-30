import { db } from "@/db";
import { lfShopItems } from "@/db/schema";
import { asc, desc, eq } from "drizzle-orm";
import { SHOP_CATEGORIES } from "@/lib/platform";
import { ensureSeededSafe } from "@/lib/seed";

export const dynamic = "force-dynamic";

export async function GET(req: Request) {
  await ensureSeededSafe();
  const url = new URL(req.url);
  const category = url.searchParams.get("category");
  const rows = category
    ? await db.select().from(lfShopItems).where(eq(lfShopItems.category, Number(category))).orderBy(asc(lfShopItems.sortOrder))
    : await db.select().from(lfShopItems).orderBy(asc(lfShopItems.category), desc(lfShopItems.cost));
  return Response.json({
    items: rows,
    categories: SHOP_CATEGORIES,
    currencyId: 1533,
    currencyName: "Сущность пробуждения",
  });
}

export async function POST(req: Request) {
  const body = (await req.json()) as Record<string, unknown>;
  if (!body.nameRu) return Response.json({ ok: false, error: "nameRu обязателен" }, { status: 400 });
  const [row] = await db.insert(lfShopItems).values({
    itemId: Number(body.itemId ?? 0),
    count: Number(body.count ?? 1),
    cost: Number(body.cost ?? 0),
    category: Number(body.category ?? 1),
    nameRu: String(body.nameRu),
    descriptionRu: String(body.descriptionRu ?? ""),
    sortOrder: Number(body.sortOrder ?? 0),
    enabled: body.enabled !== false,
  }).returning();
  return Response.json({ ok: true, item: row });
}
