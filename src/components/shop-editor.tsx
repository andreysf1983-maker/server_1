"use client";

import { useEffect, useMemo, useState } from "react";
import { Badge, Panel, Toggle } from "@/components/ui";

type ShopItem = {
  id: number; itemId: number; count: number; cost: number; category: number;
  nameRu: string; descriptionRu: string; sortOrder: number; enabled: boolean;
};

const CATEGORIES: Record<number, string> = {
  1: "Редчайший трансмог", 2: "Маунты", 3: "Питомцы",
  4: "Услуги персонажа", 5: "Реагенты улучшения", 6: "Фолианты способностей",
};

export default function ShopEditor() {
  const [items, setItems] = useState<ShopItem[]>([]);
  const [filter, setFilter] = useState<number | 0>(0);
  const [message, setMessage] = useState("");
  const [form, setForm] = useState({ nameRu: "", itemId: 0, count: 1, cost: 1000, category: 1, descriptionRu: "" });

  async function load() {
    const data = await fetch("/api/shop", { cache: "no-store" }).then((r) => r.json());
    setItems(data.items ?? []);
  }
  useEffect(() => { load(); }, []);

  const visible = useMemo(
    () => (filter ? items.filter((i) => i.category === filter) : items),
    [items, filter],
  );

  async function patch(id: number, body: Record<string, unknown>) {
    setItems((prev) => prev.map((i) => (i.id === id ? { ...i, ...body } as ShopItem : i)));
    await fetch(`/api/shop/${id}`, {
      method: "PATCH", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body),
    });
    setMessage(`Позиция #${id} обновлена. В игре выполните .lf shop reload`);
  }

  async function remove(id: number) {
    await fetch(`/api/shop/${id}`, { method: "DELETE" });
    setItems((prev) => prev.filter((i) => i.id !== id));
    setMessage(`Позиция #${id} удалена из каталога.`);
  }

  async function create() {
    if (!form.nameRu.trim()) { setMessage("Укажите название товара."); return; }
    const res = await fetch("/api/shop", {
      method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(form),
    }).then((r) => r.json());
    if (res?.item) setItems((prev) => [...prev, res.item]);
    setForm({ nameRu: "", itemId: 0, count: 1, cost: 1000, category: 1, descriptionRu: "" });
    setMessage("Товар добавлен в каталог магазина кнопки «W».");
  }

  const totalValue = items.reduce((a, i) => a + i.cost, 0);

  return (
    <div className="space-y-5">
      <div className="lf-panel p-4 flex flex-wrap items-center gap-3">
        <Badge tone="ok">валюта 1533 · Сущность пробуждения</Badge>
        <Badge>позиций: {items.length}</Badge>
        <Badge tone="info">суммарная цена каталога: {totalValue.toLocaleString("ru-RU")}</Badge>
        <div className="ml-auto flex gap-2">
          <select className="lf-input w-auto" value={filter} onChange={(e) => setFilter(Number(e.target.value))}>
            <option value={0}>Все категории</option>
            {Object.entries(CATEGORIES).map(([k, v]) => <option key={k} value={k}>{v}</option>)}
          </select>
          <a className="lf-btn" href="/api/export/sql?what=shop">Экспорт в SQL</a>
        </div>
      </div>

      {message ? <div className="lf-panel p-3 text-[13px] text-emerald-300 border-emerald-500/30">{message}</div> : null}

      <Panel title="Добавить товар" hint="каталог BattlePay 7.3.5.26124">
        <div className="grid md:grid-cols-6 gap-3">
          <label className="md:col-span-2"><div className="text-[11.5px] text-slate-500 mb-1">Название</div>
            <input className="lf-input" value={form.nameRu} onChange={(e) => setForm({ ...form, nameRu: e.target.value })} placeholder="Трансмог-сет «...»" /></label>
          <label><div className="text-[11.5px] text-slate-500 mb-1">Item ID</div>
            <input className="lf-input lf-mono" type="number" value={form.itemId} onChange={(e) => setForm({ ...form, itemId: Number(e.target.value) })} /></label>
          <label><div className="text-[11.5px] text-slate-500 mb-1">Кол-во</div>
            <input className="lf-input lf-mono" type="number" value={form.count} onChange={(e) => setForm({ ...form, count: Number(e.target.value) })} /></label>
          <label><div className="text-[11.5px] text-slate-500 mb-1">Цена</div>
            <input className="lf-input lf-mono" type="number" value={form.cost} onChange={(e) => setForm({ ...form, cost: Number(e.target.value) })} /></label>
          <label><div className="text-[11.5px] text-slate-500 mb-1">Категория</div>
            <select className="lf-input" value={form.category} onChange={(e) => setForm({ ...form, category: Number(e.target.value) })}>
              {Object.entries(CATEGORIES).map(([k, v]) => <option key={k} value={k}>{v}</option>)}
            </select></label>
        </div>
        <div className="mt-3 flex gap-2">
          <button className="lf-btn lf-btn-primary" onClick={create}>Добавить в магазин</button>
          <span className="text-[12px] text-slate-500 self-center">Для услуг персонажа Item ID = 0 (доставляется сервисом ядра).</span>
        </div>
      </Panel>

      <Panel title="Каталог магазина" hint={`${visible.length} позиций`}>
        <div className="overflow-x-auto">
          <table className="lf-table w-full min-w-[900px]">
            <thead>
              <tr>
                <th>#</th><th>Категория</th><th>Товар</th><th>Item ID</th><th>Кол-во</th>
                <th>Цена (1533)</th><th>Активен</th><th></th>
              </tr>
            </thead>
            <tbody>
              {visible.map((i) => (
                <tr key={i.id}>
                  <td className="lf-mono text-slate-500">{i.id}</td>
                  <td><span className="lf-badge">{CATEGORIES[i.category] ?? i.category}</span></td>
                  <td>
                    <div className="text-slate-200">{i.nameRu}</div>
                    {i.descriptionRu ? <div className="text-[11px] text-slate-500">{i.descriptionRu}</div> : null}
                  </td>
                  <td className="lf-mono text-sky-300">{i.itemId || "—"}</td>
                  <td>
                    <input className="lf-input lf-mono w-20" type="number" defaultValue={i.count}
                      onBlur={(e) => Number(e.target.value) !== i.count && patch(i.id, { count: Number(e.target.value) })} />
                  </td>
                  <td>
                    <input className="lf-input lf-mono w-28" type="number" defaultValue={i.cost}
                      onBlur={(e) => Number(e.target.value) !== i.cost && patch(i.id, { cost: Number(e.target.value) })} />
                  </td>
                  <td><Toggle checked={i.enabled} onChange={(v) => patch(i.id, { enabled: v })} /></td>
                  <td><button className="lf-btn !py-1 !px-2 text-[12px]" onClick={() => remove(i.id)}>удалить</button></td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Panel>
    </div>
  );
}
