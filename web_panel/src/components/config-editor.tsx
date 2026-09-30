"use client";

import { useEffect, useMemo, useState } from "react";
import { Badge, Panel, Toggle } from "@/components/ui";

type Setting = { key: string; value: string; label: string; category: string; type: string; hint: string };
type Module = { key: string; name: string; description: string; category: string; enabled: boolean; configKey: string };

const CATEGORY_LABELS: Record<string, string> = {
  core: "Ядро и реалм",
  rates: "Рейты",
  economy: "Экономика (Сущность пробуждения)",
  world: "Мир и боты",
};

export default function ConfigEditor() {
  const [settings, setSettings] = useState<Setting[]>([]);
  const [modules, setModules] = useState<Module[]>([]);
  const [confLines, setConfLines] = useState(0);
  const [dirty, setDirty] = useState<Record<string, string>>({});
  const [message, setMessage] = useState<string>("");
  const [busy, setBusy] = useState(false);

  async function load() {
    const [s, m] = await Promise.all([
      fetch("/api/settings", { cache: "no-store" }).then((r) => r.json()),
      fetch("/api/modules", { cache: "no-store" }).then((r) => r.json()),
    ]);
    setSettings(s.settings ?? []);
    setConfLines(s.conf?.lines ?? 0);
    setModules(m.modules ?? []);
  }

  useEffect(() => { load(); }, []);

  const grouped = useMemo(() => {
    const map = new Map<string, Setting[]>();
    for (const s of settings) {
      const arr = map.get(s.category) ?? [];
      arr.push(s);
      map.set(s.category, arr);
    }
    return Array.from(map.entries());
  }, [settings]);

  const moduleGroups = useMemo(() => {
    const map = new Map<string, Module[]>();
    for (const m of modules) {
      const arr = map.get(m.category) ?? [];
      arr.push(m);
      map.set(m.category, arr);
    }
    return Array.from(map.entries());
  }, [modules]);

  async function save(applyToConf: boolean) {
    setBusy(true);
    setMessage("");
    try {
      const res = await fetch("/api/settings", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ updates: dirty, applyToConf }),
      }).then((r) => r.json());
      setMessage(applyToConf
        ? `Сохранено ${res.updated ?? 0} параметров, worldserver.conf обновлён (${res.conf?.changed ?? 0} строк).`
        : `Сохранено ${res.updated ?? 0} параметров в панели (конфиг не тронут).`);
      setDirty({});
      await load();
    } catch (e) {
      setMessage("Ошибка сохранения: " + String(e));
    } finally {
      setBusy(false);
    }
  }

  async function toggleModule(m: Module, enabled: boolean) {
    setModules((prev) => prev.map((x) => (x.key === m.key ? { ...x, enabled } : x)));
    await fetch("/api/modules", {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ key: m.key, enabled, syncConf: true }),
    });
    setMessage(`Модуль «${m.name}» ${enabled ? "включён" : "выключен"} и записан в worldserver.conf / modules.conf.`);
  }

  const dirtyCount = Object.keys(dirty).length;

  return (
    <div className="space-y-5">
      <div className="lf-panel p-4 flex flex-wrap items-center gap-3">
        <Badge tone={dirtyCount ? "warn" : "ok"}>{dirtyCount ? `изменено: ${dirtyCount}` : "без изменений"}</Badge>
        <span className="text-[12.5px] text-slate-400">
          worldserver.conf: {confLines.toLocaleString("ru-RU")} строк · секция LEGIONFORGE редактируется визуально
        </span>
        <div className="ml-auto flex gap-2">
          <button className="lf-btn" disabled={busy || !dirtyCount} onClick={() => save(false)}>Сохранить в панели</button>
          <button className="lf-btn lf-btn-primary" disabled={busy || !dirtyCount} onClick={() => save(true)}>
            Сохранить и записать в worldserver.conf
          </button>
        </div>
      </div>

      {message ? (
        <div className="lf-panel p-3 text-[13px] text-emerald-300 border-emerald-500/30">{message}</div>
      ) : null}

      <Panel title="Кастомные модули платформы" hint="переключатели пишутся в конфиги">
        <div className="grid md:grid-cols-2 gap-x-8 gap-y-3">
          {moduleGroups.map(([category, list]) => (
            <div key={category}>
              <div className="text-[11px] uppercase tracking-[0.12em] text-slate-500 font-bold mb-2">{category}</div>
              <div className="space-y-2">
                {list.map((m) => (
                  <div key={m.key} className="flex items-start gap-3 rounded-lg border border-[#1e2735] bg-[#0d121b] px-3 py-2">
                    <div className="pt-1"><Toggle checked={m.enabled} onChange={(v) => toggleModule(m, v)} /></div>
                    <div className="min-w-0">
                      <div className="text-[13px] font-semibold text-slate-200">{m.name}</div>
                      <div className="text-[11.5px] text-slate-500 leading-snug">{m.description}</div>
                      {m.configKey ? <div className="lf-mono text-[10.5px] text-slate-600 mt-0.5">{m.configKey}</div> : null}
                    </div>
                  </div>
                ))}
              </div>
            </div>
          ))}
        </div>
      </Panel>

      {grouped.map(([category, list]) => (
        <Panel key={category} title={CATEGORY_LABELS[category] ?? category} hint={`${list.length} параметров`}>
          <div className="grid md:grid-cols-2 xl:grid-cols-3 gap-3">
            {list.map((s) => (
              <label key={s.key} className="block">
                <div className="text-[12px] font-semibold text-slate-300 mb-1">{s.label || s.key}</div>
                <input
                  className="lf-input lf-mono"
                  defaultValue={s.value}
                  onChange={(e) => setDirty((d) => ({ ...d, [s.key]: e.target.value }))}
                />
                <div className="lf-mono text-[10.5px] text-slate-600 mt-1">{s.key}{s.hint ? ` · ${s.hint}` : ""}</div>
              </label>
            ))}
          </div>
        </Panel>
      ))}
    </div>
  );
}
