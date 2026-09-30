"use client";

import { useEffect, useMemo, useState } from "react";
import { Badge, Panel, Stat, Toggle } from "@/components/ui";

type Boss = {
  entry: number; nameRu: string; zone: string; continent: string; levelMin: number; levelMax: number;
  rank: number; essenceMin: number; essenceMax: number; respawnMinutes: number;
  displayId: number; spellMain: number; spellAoe: number; summonEntry: number; spellEnrage: number; enabled: boolean;
};

export default function BossManager() {
  const [bosses, setBosses] = useState<Boss[]>([]);
  const [zones, setZones] = useState<Array<{ zone: string; n: number }>>([]);
  const [query, setQuery] = useState("");
  const [zone, setZone] = useState("");
  const [message, setMessage] = useState("");

  async function load() {
    const data = await fetch("/api/world-bosses", { cache: "no-store" }).then((r) => r.json());
    setBosses(data.bosses ?? []);
    setZones(data.zones ?? []);
  }
  useEffect(() => { load(); }, []);

  const visible = useMemo(() => bosses.filter((b) => {
    const q = query.trim().toLowerCase();
    const byText = !q || b.nameRu.toLowerCase().includes(q) || String(b.entry).includes(q) || b.zone.toLowerCase().includes(q);
    const byZone = !zone || b.zone === zone;
    return byText && byZone;
  }), [bosses, query, zone]);

  async function patch(boss: Boss, body: Record<string, unknown>) {
    setBosses((prev) => prev.map((b) => (b.entry === boss.entry ? { ...b, ...body } as Boss : b)));
    await fetch("/api/world-bosses", {
      method: "PATCH", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ entry: boss.entry, ...body }),
    });
    setMessage(`Босс #${boss.entry} обновлён. В игре: .lf worldboss reload`);
  }

  const enabled = bosses.filter((b) => b.enabled).length;

  return (
    <div className="space-y-5">
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        <Stat label="Всего боссов" value={bosses.length} tone="amber" note="entry 900001-900100" />
        <Stat label="Активных" value={enabled} tone="green" note="участвуют в респауне" />
        <Stat label="Сущности за убийство" value="250–500" tone="blue" note="каждому участнику группы" />
        <Stat label="Масштаб под группу" value="+18%/игрок" note="расчёт на 2–10 сильных игроков" />
      </div>

      {message ? <div className="lf-panel p-3 text-[13px] text-emerald-300 border-emerald-500/30">{message}</div> : null}

      <div className="lf-panel p-4 flex flex-wrap items-center gap-3">
        <input className="lf-input max-w-xs" placeholder="Поиск по имени, entry или зоне..." value={query} onChange={(e) => setQuery(e.target.value)} />
        <select className="lf-input w-auto" value={zone} onChange={(e) => setZone(e.target.value)}>
          <option value="">Все зоны</option>
          {zones.map((z) => <option key={z.zone} value={z.zone}>{z.zone} ({z.n})</option>)}
        </select>
        <span className="text-[12.5px] text-slate-400">показано: {visible.length}</span>
        <a className="lf-btn ml-auto" href="/api/export/sql?what=bosses">Экспорт боссов в SQL</a>
      </div>

      <Panel title="Реестр мировых боссов" hint="custom_legionforge_worldboss">
        <div className="overflow-x-auto max-h-[640px] overflow-y-auto">
          <table className="lf-table w-full min-w-[1000px]">
            <thead className="sticky top-0 bg-[#111825]">
              <tr>
                <th>Entry</th><th>Имя</th><th>Зона</th><th>Уровень</th><th>Ранг</th>
                <th>Сущность (мин/макс)</th><th>Респаун, мин</th><th>Фазы</th><th>Активен</th>
              </tr>
            </thead>
            <tbody>
              {visible.map((b) => (
                <tr key={b.entry}>
                  <td className="lf-mono text-sky-300">{b.entry}</td>
                  <td className="text-slate-200 font-semibold">{b.nameRu}</td>
                  <td className="text-slate-400">{b.zone || "—"}<div className="text-[10.5px] text-slate-600">{b.continent}</div></td>
                  <td className="lf-mono">{b.levelMin}–{b.levelMax}</td>
                  <td><Badge tone={b.rank >= 3 ? "warn" : "neutral"}>{b.rank >= 3 ? "world boss" : "rare elite"}</Badge></td>
                  <td>
                    <div className="flex gap-1">
                      <input className="lf-input lf-mono w-20" type="number" defaultValue={b.essenceMin}
                        onBlur={(e) => Number(e.target.value) !== b.essenceMin && patch(b, { essenceMin: Number(e.target.value) })} />
                      <input className="lf-input lf-mono w-20" type="number" defaultValue={b.essenceMax}
                        onBlur={(e) => Number(e.target.value) !== b.essenceMax && patch(b, { essenceMax: Number(e.target.value) })} />
                    </div>
                  </td>
                  <td>
                    <input className="lf-input lf-mono w-24" type="number" defaultValue={b.respawnMinutes}
                      onBlur={(e) => Number(e.target.value) !== b.respawnMinutes && patch(b, { respawnMinutes: Number(e.target.value) })} />
                  </td>
                  <td className="lf-mono text-[11.5px] text-slate-500">
                    {b.spellMain ? `#${b.spellMain}` : "—"} / {b.spellAoe ? `#${b.spellAoe}` : "—"} / {b.spellEnrage ? `#${b.spellEnrage}` : "—"}
                  </td>
                  <td><Toggle checked={b.enabled} onChange={(v) => patch(b, { enabled: v })} /></td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Panel>
    </div>
  );
}
