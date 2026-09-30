import { ensureSeeded } from "@/lib/seed";

export const dynamic = "force-dynamic";

export async function POST(req: Request) {
  const body = (await req.json().catch(() => ({}))) as { force?: boolean };
  const did = await ensureSeeded(Boolean(body.force));
  return Response.json({ ok: true, seeded: did, force: Boolean(body.force) });
}
