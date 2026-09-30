"use client";

import { useEffect, useState } from "react";
import { Badge, Panel } from "@/components/ui";

type Node = { name: string; path: string; type: "dir" | "file"; size: number; children?: Node[] };

const BUNDLES: Array<{ label: string; items: string; hint: string }> = [
  { label: "Лаунчеры + конфиги + SQL + docs", items: "START.bat,PANEL.bat,Stop.bat,legionforge.bin,server/configs,sql,docs", hint: "готовый к запуску минимум" },
  { label: "Кастомные C++ модули", items: "source/src/server/scripts/Custom,patches", hint: "все системы LEGIONFORGE" },
  { label: "Портативные утилиты (tools)", items: "tools", hint: "скрипты сборки и загрузки среды" },
  { label: "Веб-панель (web_panel)", items: "web_panel", hint: "исходники и деплой панели" },
];

function humanSize(bytes: number) {
  if (!bytes) return "";
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
}

function Tree({ nodes, depth, selected, onSelect, expanded, toggle }: {
  nodes: Node[]; depth: number; selected: string | null;
  onSelect: (n: Node) => void; expanded: Set<string>; toggle: (p: string) => void;
}) {
  return (
    <ul className="text-[12.5px]">
      {nodes.map((n) => {
        const isOpen = expanded.has(n.path);
        return (
          <li key={n.path}>
            <div
              className={`flex items-center gap-2 py-[3px] pr-2 rounded cursor-pointer hover:bg-white/5 ${selected === n.path ? "bg-amber-500/10 text-amber-300" : "text-slate-300"}`}
              style={{ paddingLeft: 8 + depth * 14 }}
              onClick={() => (n.type === "dir" ? toggle(n.path) : onSelect(n))}
            >
              <span className="w-4 text-center opacity-70">{n.type === "dir" ? (isOpen ? "▾" : "▸") : "·"}</span>
              <span className={n.type === "dir" ? "font-semibold text-slate-200" : "lf-mono"}>{n.name}</span>
              {n.type === "file" && n.size ? <span className="ml-auto text-[10.5px] text-slate-600 lf-mono">{humanSize(n.size)}</span> : null}
            </div>
            {n.type === "dir" && isOpen && n.children ? (
              <Tree nodes={n.children} depth={depth + 1} selected={selected} onSelect={onSelect} expanded={expanded} toggle={toggle} />
            ) : null}
          </li>
        );
      })}
    </ul>
  );
}

export default function FileExplorer() {
  const [tree, setTree] = useState<Node[]>([]);
  const [selected, setSelected] = useState<Node | null>(null);
  const [content, setContent] = useState<string>("");
  const [meta, setMeta] = useState<{ size: number; truncated: boolean; binary: boolean } | null>(null);
  const [expanded, setExpanded] = useState<Set<string>>(new Set(["docs", "sql", "server", "tools", "source/src/server/scripts"]));
  const [loading, setLoading] = useState(false);

  async function load() {
    const data = await fetch("/api/files?depth=4", { cache: "no-store" }).then((r) => r.json());
    setTree(data.tree ?? []);
  }
  useEffect(() => { load(); }, []);

  async function select(n: Node) {
    setSelected(n);
    setLoading(true);
    setContent("");
    const data = await fetch(`/api/files/content?path=${encodeURIComponent(n.path)}`, { cache: "no-store" }).then((r) => r.json());
    setContent(data.binary ? "" : data.content ?? "");
    setMeta({ size: data.size ?? n.size, truncated: Boolean(data.truncated), binary: Boolean(data.binary) });
    setLoading(false);
  }

  function toggle(p: string) {
    setExpanded((prev) => {
      const next = new Set(prev);
      if (next.has(p)) next.delete(p); else next.add(p);
      return next;
    });
  }

  const lineCount = content ? content.split("\n").length : 0;

  return (
    <div className="space-y-5">
      <Panel title="Готовые наборы для скачивания" hint="zip-архивы">
        <div className="grid md:grid-cols-2 gap-3">
          {BUNDLES.map((b) => (
            <a key={b.label} href={`/api/files/download?bundle=${encodeURIComponent(b.items)}`}
               className="lf-panel p-4 flex items-center gap-3 hover:border-amber-500/40 transition-colors">
              <div className="text-xl">📦</div>
              <div className="min-w-0">
                <div className="text-[13px] font-semibold text-slate-200">{b.label}</div>
                <div className="text-[11.5px] text-slate-500">{b.hint}</div>
              </div>
              <span className="lf-badge ml-auto shrink-0">ZIP</span>
            </a>
          ))}
        </div>
      </Panel>

      <div className="grid lg:grid-cols-[380px_1fr] gap-4">
        <Panel title="Дерево /LEGIONFORGE" hint="кликабельно">
          <div className="max-h-[620px] overflow-y-auto pr-1">
            <Tree nodes={tree} depth={0} selected={selected?.path ?? null} onSelect={select} expanded={expanded} toggle={toggle} />
          </div>
        </Panel>

        <Panel
          title={selected ? selected.path : "Выберите файл"}
          hint={selected ? `${humanSize(meta?.size ?? selected.size)} · ${lineCount} строк` : "просмотр и скачивание"}
        >
          {selected ? (
            <>
              <div className="flex flex-wrap items-center gap-2 mb-3">
                <Badge tone="info">{meta?.binary ? "бинарный файл" : "текстовый файл"}</Badge>
                {meta?.truncated ? <Badge tone="warn">показано первые 500 КБ</Badge> : null}
                <a className="lf-btn ml-auto" href={`/api/files/download?path=${encodeURIComponent(selected.path)}`}>Скачать файл</a>
              </div>
              {loading ? (
                <div className="text-slate-500 text-[13px]">Загрузка ...</div>
              ) : meta?.binary ? (
                <div className="text-slate-500 text-[13px]">
                  Предпросмотр бинарного файла недоступен — используйте кнопку «Скачать файл».
                </div>
              ) : (
                <pre className="lf-mono text-[11.5px] leading-[1.5] text-slate-300 bg-[#080c13] border border-[#1c2534] rounded-lg p-3 overflow-auto max-h-[560px] whitespace-pre-wrap">
                  {content}
                </pre>
              )}
            </>
          ) : (
            <div className="text-slate-500 text-[13px]">
              Слева — полная структура платформы: лаунчеры, портативные утилиты, исходники ядра с кастомными модулями,
              конфиги сервера, SQL-базы, веб-панель и документация. Любой файл можно просмотреть и скачать.
            </div>
          )}
        </Panel>
      </div>
    </div>
  );
}
