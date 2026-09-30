"use client";

import { useEffect, useState } from "react";
import { Badge, Panel, Stat } from "@/components/ui";

type Account = { id: number; username: string; email: string; gmLevel: number; expansion: number; essence: number; banned: boolean; lastLogin: string | null };
type Character = { id: number; guid: number; name: string; account: string; className: string; raceName: string; level: number; ilvl: number; prestige: number; hardcore: boolean; totalEssence: number; online: boolean; zone: string };
type Bot = { id: number; nick: string; classId: number; role: number; zone: string };
type LogRow = { id: number; at: string; characterName: string; amount: number; reason: string };

const ROLES = ["ДД", "Танк", "Хил"];

export default function PlayersManager() {
  const [accounts, setAccounts] = useState<Account[]>([]);
  const [characters, setCharacters] = useState<Character[]>([]);
  const [bots, setBots] = useState<Bot[]>([]);
  const [botTotal, setBotTotal] = useState(0);
  const [log, setLog] = useState<LogRow[]>([]);
  const [message, setMessage] = useState("");
  const [newAccount, setNewAccount] = useState({ username: "", gmLevel: 0 });
  const [grantAmount, setGrantAmount] = useState(1000);

  async function load() {
    const data = await fetch("/api/players", { cache: "no-store" }).then((r) => r.json());
    setAccounts(data.accounts ?? []);
    setCharacters(data.characters ?? []);
    setBots(data.bots ?? []);
    setBotTotal(data.botTotal ?? 0);
    setLog(data.essenceLog ?? []);
  }
  useEffect(() => { load(); }, []);

  async function grant(target: "account" | "character", id: number, name: string) {
    await fetch("/api/players/grant", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ target, id, amount: grantAmount, reason: "выдача через веб-панель" }),
    });
    setMessage(`${name}: ${grantAmount >= 0 ? "+" : ""}${grantAmount} Сущности пробуждения (1533).`);
    await load();
  }

  async function createAccount() {
    if (!newAccount.username.trim()) { setMessage("Укажите имя аккаунта."); return; }
    await fetch("/api/players", {
      method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(newAccount),
    });
    setMessage(`Аккаунт «${newAccount.username}» создан (GM ${newAccount.gmLevel}).`);
    setNewAccount({ username: "", gmLevel: 0 });
    await load();
  }

  const onlineNow = characters.filter((c) => c.online).length;
  const essencePool = characters.reduce((a, c) => a + c.totalEssence, 0);

  return (
    <div className="space-y-5">
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        <Stat label="Онлайн" value={onlineNow} tone="green" note="персонажи в мире" />
        <Stat label="Аккаунтов" value={accounts.length} note="auth-база" />
        <Stat label="Персонажей" value={characters.length} tone="amber" note="110 уровень / престиж" />
        <Stat label="Сущности на руках" value={essencePool.toLocaleString("ru-RU")} tone="blue" note="валюта 1533" />
      </div>

      {message ? <div className="lf-panel p-3 text-[13px] text-emerald-300 border-emerald-500/30">{message}</div> : null}

      <div className="lf-panel p-4 flex flex-wrap items-end gap-3">
        <label><div className="text-[11.5px] text-slate-500 mb-1">Сумма выдачи</div>
          <input className="lf-input lf-mono w-32" type="number" value={grantAmount} onChange={(e) => setGrantAmount(Number(e.target.value))} /></label>
        <span className="text-[12px] text-slate-500 pb-2">Сущность пробуждения (1533) — выдаётся аккаунту или персонажу кнопкой рядом.</span>
        <div className="ml-auto flex items-end gap-2">
          <label><div className="text-[11.5px] text-slate-500 mb-1">Новый аккаунт</div>
            <input className="lf-input w-44" placeholder="username" value={newAccount.username} onChange={(e) => setNewAccount({ ...newAccount, username: e.target.value })} /></label>
          <label><div className="text-[11.5px] text-slate-500 mb-1">GM</div>
            <select className="lf-input w-24" value={newAccount.gmLevel} onChange={(e) => setNewAccount({ ...newAccount, gmLevel: Number(e.target.value) })}>
              <option value={0}>0 игрок</option><option value={1}>1 модер</option><option value={2}>2 GM</option><option value={3}>3 админ</option>
            </select></label>
          <button className="lf-btn lf-btn-primary" onClick={createAccount}>Создать</button>
        </div>
      </div>

      <Panel title="Аккаунты" hint="auth">
        <div className="overflow-x-auto">
          <table className="lf-table w-full min-w-[760px]">
            <thead><tr><th>Аккаунт</th><th>E-mail</th><th>GM</th><th>Сущность</th><th>Последний вход</th><th></th></tr></thead>
            <tbody>
              {accounts.map((a) => (
                <tr key={a.id}>
                  <td className="text-slate-200 font-semibold">{a.username}</td>
                  <td className="text-slate-400 lf-mono text-[12px]">{a.email}</td>
                  <td><Badge tone={a.gmLevel >= 3 ? "warn" : a.gmLevel > 0 ? "info" : "neutral"}>{a.gmLevel}</Badge></td>
                  <td className="lf-mono text-amber-300">{a.essence.toLocaleString("ru-RU")}</td>
                  <td className="text-slate-500 text-[12px]">{a.lastLogin ? new Date(a.lastLogin).toLocaleString("ru-RU") : "—"}</td>
                  <td className="text-right">
                    <button className="lf-btn !py-1 !px-2 text-[12px]" onClick={() => grant("account", a.id, a.username)}>+{grantAmount}</button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Panel>

      <Panel title="Персонажи" hint="characters">
        <div className="overflow-x-auto">
          <table className="lf-table w-full min-w-[900px]">
            <thead><tr><th>Имя</th><th>Класс / раса</th><th>Уровень</th><th>ilvl</th><th>Престиж</th><th>Hardcore</th><th>Сущность</th><th>Зона</th><th></th></tr></thead>
            <tbody>
              {characters.map((c) => (
                <tr key={c.id}>
                  <td className="text-slate-100 font-semibold">
                    {c.name} {c.online ? <span className="ml-1 text-emerald-400 text-[11px]">online</span> : null}
                  </td>
                  <td className="text-slate-400 text-[12px]">{c.className} · {c.raceName}</td>
                  <td className="lf-mono">{c.level}</td>
                  <td className="lf-mono text-sky-300">{c.ilvl}</td>
                  <td className="lf-mono">{c.prestige ? `P${c.prestige}` : "—"}</td>
                  <td>{c.hardcore ? <Badge tone="bad">одна жизнь</Badge> : <span className="text-slate-600">—</span>}</td>
                  <td className="lf-mono text-amber-300">{c.totalEssence.toLocaleString("ru-RU")}</td>
                  <td className="text-slate-500 text-[12px]">{c.zone}</td>
                  <td className="text-right"><button className="lf-btn !py-1 !px-2 text-[12px]" onClick={() => grant("character", c.id, c.name)}>+{grantAmount}</button></td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Panel>

      <div className="grid lg:grid-cols-2 gap-4">
        <Panel title="ИИ-боты" hint={`${botTotal} ников в пуле`}>
          <div className="flex flex-wrap gap-1.5">
            {bots.map((b) => (
              <span key={b.id} className="lf-badge" title={`класс ${b.classId} · роль ${ROLES[b.role] ?? "ДД"} · ${b.zone}`}>{b.nick}</span>
            ))}
          </div>
          <p className="text-[12px] text-slate-500 mt-3 leading-relaxed">
            Боты гуляют по столицам, принимают приглашения в группу, берут роли танк/хил/дд, ассистируют лидеру,
            используют контроль и бурсты, пишут осмысленные фразы раз в 120–240 секунд и занимают пустые слоты на BG и аренах.
          </p>
        </Panel>
        <Panel title="Журнал операций с валютой" hint="последние 30">
          <table className="lf-table w-full">
            <thead><tr><th>Время</th><th>Кому</th><th>Сумма</th><th>Причина</th></tr></thead>
            <tbody>
              {log.map((row) => (
                <tr key={row.id}>
                  <td className="lf-mono text-slate-500 text-[11.5px]">{new Date(row.at).toLocaleString("ru-RU")}</td>
                  <td className="text-slate-300">{row.characterName}</td>
                  <td className={row.amount >= 0 ? "text-emerald-400 font-bold" : "text-rose-400 font-bold"}>{row.amount >= 0 ? "+" : ""}{row.amount}</td>
                  <td className="text-slate-500 text-[12px]">{row.reason}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </Panel>
      </div>
    </div>
  );
}
