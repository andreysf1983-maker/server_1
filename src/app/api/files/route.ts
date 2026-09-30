import { buildTree, PLATFORM_ROOT, readFileSafe } from "@/lib/platform";

export const dynamic = "force-dynamic";

export async function GET(req: Request) {
  const url = new URL(req.url);
  const depth = Math.min(Number(url.searchParams.get("depth") ?? 3), 6);
  const tree = buildTree(PLATFORM_ROOT, "", depth);
  const readme = readFileSafe("docs/README_RU.md", 4000);
  return Response.json({ root: "LEGIONFORGE", tree, readmePreview: readme?.content.slice(0, 900) ?? "" });
}
