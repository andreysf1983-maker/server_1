import { readFileSafe } from "@/lib/platform";

export const dynamic = "force-dynamic";

const TEXT_EXT = /\.(txt|md|bat|ps1|conf|sql|cpp|h|hpp|c|cmake|ts|tsx|js|mjs|json|xml|cs|csproj|xaml|ini|bin|dist|yml|yaml|log)$/i;

export async function GET(req: Request) {
  const url = new URL(req.url);
  const rel = url.searchParams.get("path") ?? "";
  if (!rel) return Response.json({ ok: false, error: "path обязателен" }, { status: 400 });
  const file = readFileSafe(rel, 500_000);
  if (!file) return Response.json({ ok: false, error: "файл не найден" }, { status: 404 });
  if (!TEXT_EXT.test(rel)) {
    return Response.json({ ok: true, path: rel, size: file.size, binary: true, content: "" });
  }
  return Response.json({ ok: true, path: rel, size: file.size, truncated: file.truncated, binary: false, content: file.content });
}
