import fs from "node:fs";
import path from "node:path";
import { DOCS_DIR } from "@/lib/platform";
import { PageHeader, Panel } from "@/components/ui";

export const dynamic = "force-dynamic";

export default function PromoPage() {
  const file = path.join(DOCS_DIR, "PROMO_TEXT.txt");
  const text = fs.existsSync(file) ? fs.readFileSync(file, "utf8") : "PROMO_TEXT.txt не найден";
  const lines = text.split(/\r?\n/).length;

  return (
    <div>
      <PageHeader
        title="Промо-текст для анонса сервера"
        subtitle={`docs/PROMO_TEXT.txt — готовый продающий текст для MMOTOP, Discord и ВКонтакте (${lines} строк). Можно разместить на главной странице веб-обвязки сервера.`}
      />
      <Panel title="PROMO_TEXT.txt" hint="моноширинный вид">
        <pre className="lf-mono text-[12.5px] leading-[1.55] text-amber-100/90 whitespace-pre-wrap overflow-x-auto">{text}</pre>
      </Panel>
    </div>
  );
}
