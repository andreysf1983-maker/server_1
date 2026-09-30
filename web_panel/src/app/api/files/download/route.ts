import fs from "node:fs";
import path from "node:path";
import { createZip, safeJoin } from "@/lib/platform";

export const dynamic = "force-dynamic";

/** Отдаёт отдельный файл платформы или zip-архив с набором файлов/папок. */
export async function GET(req: Request) {
  const url = new URL(req.url);
  const rel = url.searchParams.get("path");
  const bundle = url.searchParams.get("bundle");

  if (bundle) {
    const items = bundle.split(",").map((s) => s.trim()).filter(Boolean);
    const zip = createZip(items);
    return new Response(new Uint8Array(zip), {
      headers: {
        "Content-Type": "application/zip",
        "Content-Disposition": `attachment; filename="legionforge_bundle.zip"`,
        "Content-Length": String(zip.length),
      },
    });
  }

  if (!rel) return Response.json({ ok: false, error: "path обязателен" }, { status: 400 });
  const full = safeJoin(rel);
  if (!full || !fs.existsSync(full)) return Response.json({ ok: false, error: "не найдено" }, { status: 404 });

  if (fs.statSync(full).isDirectory()) {
    const zip = createZip([rel]);
    return new Response(new Uint8Array(zip), {
      headers: {
        "Content-Type": "application/zip",
        "Content-Disposition": `attachment; filename="${path.basename(rel)}.zip"`,
      },
    });
  }

  const data = fs.readFileSync(full);
  return new Response(new Uint8Array(data), {
    headers: {
      "Content-Type": "application/octet-stream",
      "Content-Disposition": `attachment; filename="${path.basename(full)}"`,
      "Content-Length": String(data.length),
    },
  });
}
