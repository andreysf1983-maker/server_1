import Link from "next/link";
import { db } from "@/db";
import {
  lfAccounts, lfBotNames, lfCharacters, lfEssenceLog, lfLegacyTomes,
  lfOnlineSnapshots, lfShopItems, lfWorldBosses,
} from "@/db/schema";
import { desc, sql } from "drizzle-orm";
import { ensureSeededSafe } from "@/lib/seed";
import { platformHealth, countSourceFiles, CLIENT_BUILD } from "@/lib/platform";
import { Badge, BarChart, PageHeader, Panel, Stat } from "@/components/ui";

export const dynamic = "force-dynamic";

export default async function DashboardPage() {
  await ensureSeededSafe();
  const health = platformHealth();
  const sources = countSourceFiles();
  const log = await db.select().from(lfEssenceLog).orderBy(desc(lfEssenceLog.at)).limit(12);
  const snapshots = await db.select().from(lfOnlineSnapshots).orderBy(desc(lfOnlineSnapshots.at)).limit(24);
  const chart = snapshots.slice().reverse().map((s, i) => ({ label: String(i % 4 === 0 ? new Date(s.at).getHours() : ""), value: s.players }));
  const stats = {
    online: Number((await db.select({ n: sql<number>`count(*)::int` }).from(lfCharacters).where(sql`${lfCharacters.online} = true`))[0]?.n ?? 0),
    accounts: Number((await db.select({ n: sql<number>`count(*)::int` }).from(lfAccounts))[0]?.n ?? 0),
    characters: Number((await db.select({ n: sql<number>`count(*)::int` }).from(lfCharacters))[0]?.n ?? 0),
    worldBosses: Number((await db.select({ n: sql<number>`count(*)::int` }).from(lfWorldBosses))[0]?.n ?? 0),
    shopItems: Number((await db.select({ n: sql<number>`count(*)::int` }).from(lfShopItems))[0]?.n ?? 0),
    legacyTomes: Number((await db.select({ n: sql<number>`count(*)::int` }).from(lfLegacyTomes))[0]?.n ?? 0),
    botNames: Number((await db.select({ n: sql<number>`count(*)::int` }).from(lfBotNames))[0]?.n ?? 0),
  };

  return (
    <div>
      <PageHeader
        title="Дашборд платформы LEGIONFORGE"
        subtitle="Автономная серверная платформа World of Warcraft: Legion 7.3.5. Ядро LegionForgeCore, кастомные модули, экономика на Сущности пробуждения (1533) и веб-конфигуратор сборки."
        right={
          <>
            <Badge tone={health.buildOk ? "ok" : "bad"}>BUILD {health.buildConfigured ?? "—"} / {CLIENT_BUILD}</Badge>
            <Badge tone="info">platform v3.1.0</Badge>
          </>
        }
      />

      <div className={`lf-panel p-4 mb-6 flex flex-wrap items-center gap-4 ${health.buildOk ? "border-emerald-500/30" : "border-rose-500/40"}`}>
        <div className="text-2xl">{health.buildOk ? "✅" : "⚠️"}</div>
        <div className="flex-1 min-w-[280px]">
          <div className="font-bold text-[14px]">
            {health.buildOk
              ? "Жёсткая привязка к клиенту 7.3.5 (Build 26124) подтверждена"
              : "Проверьте Game.Build.Version в server/configs/worldserver.conf"}
          </div>
          <div className="text-[12.5px] text-slate-400 mt-0.5">
            realm.Build в RealmList.cpp и Main.cpp берётся из LEGIONFORGE_CLIENT_BUILD, auth.realmlist.gamebuild = 26124,
            DB2/Hotfixes и опкоды BattlePay соответствуют билду. Готовность платформы: {health.readyScore}%.
          </div>
        </div>
        <Link href="/config" className="lf-btn">Открыть конфигуратор</Link>
      </div>

      <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mb-6">
        <Stat label="Игроков онлайн" value={stats?.online ?? 0} tone="green" note="зеркало characters.online" />
        <Stat label="Аккаунтов" value={stats?.accounts ?? 0} note="auth-база" />
        <Stat label="Персонажей" value={stats?.characters ?? 0} note="110 уровень, ilvl 985-1200" />
        <Stat label="Мировых боссов" value={stats?.worldBosses ?? 0} tone="amber" note="entry 900001-900100" />
        <Stat label="Товаров магазина" value={stats?.shopItems ?? 0} note="кнопка «W», валюта 1533" />
        <Stat label="Фолиантов" value={stats?.legacyTomes ?? 0} note="забытые способности" />
        <Stat label="Ников ботов" value={stats?.botNames ?? 0} tone="blue" note="живой чат 120-240 сек" />
        <Stat label="Кастомных скриптов" value={sources.customScripts} note="src/server/scripts/Custom" />
      </div>

      <div className="grid lg:grid-cols-3 gap-4 mb-6">
        <Panel title="Онлайн за 24 часа" hint="snapshots" className="lg:col-span-2">
          {chart.length ? <BarChart data={chart} height={150} /> : <div className="text-slate-500 text-sm">Нет данных телеметрии</div>}
          <div className="mt-4 grid grid-cols-3 gap-3 text-center">
            <div className="rounded-lg border border-[#232d3f] p-3">
              <div className="text-[11px] text-slate-500 uppercase">Пик</div>
              <div className="text-lg font-black lf-amber">{Math.max(0, ...snapshots.map((s) => s.players))}</div>
            </div>
            <div className="rounded-lg border border-[#232d3f] p-3">
              <div className="text-[11px] text-slate-500 uppercase">Среднее</div>
              <div className="text-lg font-black text-sky-300">
                {snapshots.length ? Math.round(snapshots.reduce((a, s) => a + s.players, 0) / snapshots.length) : 0}
              </div>
            </div>
            <div className="rounded-lg border border-[#232d3f] p-3">
              <div className="text-[11px] text-slate-500 uppercase">Сущности выдано</div>
              <div className="text-lg font-black text-emerald-400">
                {snapshots.reduce((a, s) => a + s.essenceGranted, 0).toLocaleString("ru-RU")}
              </div>
            </div>
          </div>
        </Panel>

        <Panel title="Готовность платформы" hint="9 проверок">
          <ul className="space-y-2">
            {health.checks.map((c) => (
              <li key={c.key} className="flex items-center gap-2 text-[13px]">
                <span className={c.ok ? "text-emerald-400" : "text-rose-400"}>{c.ok ? "●" : "○"}</span>
                <span className={c.ok ? "text-slate-300" : "text-slate-500"}>{c.label}</span>
              </li>
            ))}
          </ul>
          <div className="mt-4 text-[12px] text-slate-500 leading-relaxed">
            Положите дампы в <span className="lf-mono text-slate-400">/sql/base/</span> и карты в{" "}
            <span className="lf-mono text-slate-400">/server/data/</span>, затем запустите START.bat.
          </div>
        </Panel>
      </div>

      <div className="grid lg:grid-cols-2 gap-4">
        <Panel title="Журнал выдачи Сущности пробуждения" hint="валюта 1533">
          <table className="lf-table w-full">
            <thead><tr><th>Время</th><th>Персонаж</th><th>Сумма</th><th>Причина</th></tr></thead>
            <tbody>
              {log.map((row) => (
                <tr key={row.id}>
                  <td className="lf-mono text-slate-500">{new Date(row.at).toLocaleString("ru-RU", { hour: "2-digit", minute: "2-digit", day: "2-digit", month: "2-digit" })}</td>
                  <td className="text-slate-200">{row.characterName}</td>
                  <td className={row.amount >= 0 ? "text-emerald-400 font-bold" : "text-rose-400 font-bold"}>
                    {row.amount >= 0 ? "+" : ""}{row.amount}
                  </td>
                  <td className="text-slate-400">{row.reason}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </Panel>

        <Panel title="Жизненный цикл платформы" hint="START.bat">
          <ol className="space-y-3 text-[13px] text-slate-300">
            <li className="flex gap-3"><span className="lf-badge">1</span><span>Проверка и авто-установка портативной среды: Git, CMake+Ninja, .NET 8, Node.js, MySQL 8, OpenSSL, Boost, 7-Zip + поиск MSVC.</span></li>
            <li className="flex gap-3"><span className="lf-badge">2</span><span>Распаковка source.7z (если /source пуста), наложение патчей, ребрендинг, запуск MySQL и импорт дампов 7.3.5.26124 + custom_legionforge.sql.</span></li>
            <li className="flex gap-3"><span className="lf-badge">3</span><span>Умная маршрутизация: нет бинарников — вопрос о компиляции; есть — меню (запуск / пересборка / обновление БД), авто-выбор через 5 секунд.</span></li>
            <li className="flex gap-3"><span className="lf-badge">4</span><span>LegionForge_Manager.exe поднимает mysqld, bnetserver и worldserver, держит логи во вкладках и авто-рестартует ядро при падении.</span></li>
          </ol>
          <div className="mt-4 flex gap-2">
            <Link href="/files" className="lf-btn lf-btn-primary">Файлы платформы</Link>
            <Link href="/docs" className="lf-btn">README_RU.md</Link>
            <Link href="/promo" className="lf-btn">Промо-текст</Link>
          </div>
        </Panel>
      </div>
    </div>
  );
}
