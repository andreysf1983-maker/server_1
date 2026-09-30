import { marked } from "marked";
import fs from "node:fs";
import path from "node:path";
import { DOCS_DIR } from "@/lib/platform";
import { PageHeader, Panel } from "@/components/ui";

export const dynamic = "force-dynamic";

export default function DocsPage() {
  const file = path.join(DOCS_DIR, "README_RU.md");
  const md = fs.existsSync(file) ? fs.readFileSync(file, "utf8") : "# README_RU.md не найден";
  const html = marked.parse(md, { async: false }) as string;

  return (
    <div>
      <PageHeader
        title="Документация платформы"
        subtitle="docs/README_RU.md — архитектура, первый запуск через START.bat, работа с PANEL.bat, справочник GM-команд, ID предметов и боссов, редактирование магазина BattlePay."
      />
      <Panel>
        <article
          className="lf-markdown text-[13.5px] leading-relaxed text-slate-300"
          dangerouslySetInnerHTML={{ __html: html }}
        />
      </Panel>
      <style>{`
        .lf-markdown h1 { font-size: 22px; font-weight: 900; color: #f5a623; margin: 18px 0 10px; }
        .lf-markdown h2 { font-size: 17px; font-weight: 800; color: #e6edf7; margin: 22px 0 8px; border-bottom: 1px solid #232d3f; padding-bottom: 6px; }
        .lf-markdown h3 { font-size: 14px; font-weight: 700; color: #cbd5e1; margin: 16px 0 6px; }
        .lf-markdown p { margin: 8px 0; }
        .lf-markdown ul, .lf-markdown ol { margin: 8px 0 8px 20px; }
        .lf-markdown li { margin: 3px 0; }
        .lf-markdown code { background: #0d121b; border: 1px solid #232d3f; border-radius: 5px; padding: 1px 5px; font-size: 12px; color: #7dd3fc; }
        .lf-markdown pre { background: #0a0e16; border: 1px solid #232d3f; border-radius: 10px; padding: 14px; overflow-x: auto; margin: 12px 0; }
        .lf-markdown pre code { background: transparent; border: 0; padding: 0; color: #a5f3c4; }
        .lf-markdown table { width: 100%; border-collapse: collapse; margin: 12px 0; font-size: 12.5px; }
        .lf-markdown th { text-align: left; padding: 7px 9px; border-bottom: 1px solid #2a3648; color: #94a3b8; text-transform: uppercase; font-size: 10.5px; letter-spacing: .06em; }
        .lf-markdown td { padding: 7px 9px; border-bottom: 1px solid #161d29; }
        .lf-markdown blockquote { border-left: 3px solid #f5a623; padding: 6px 14px; background: rgba(245,166,35,.06); margin: 12px 0; border-radius: 0 8px 8px 0; }
        .lf-markdown hr { border: 0; border-top: 1px solid #232d3f; margin: 18px 0; }
        .lf-markdown a { color: #7dd3fc; text-decoration: underline; }
        .lf-markdown strong { color: #f5a623; }
      `}</style>
    </div>
  );
}
