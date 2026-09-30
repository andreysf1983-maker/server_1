import { db } from "@/db";
import { lfLegacyTomes, lfQuestFixes } from "@/db/schema";
import { asc } from "drizzle-orm";
import { ensureSeededSafe } from "@/lib/seed";
import { CLASS_NAMES } from "@/lib/platform";
import { Badge, PageHeader, Panel } from "@/components/ui";

export const dynamic = "force-dynamic";

export default async function LegacyPage() {
  await ensureSeededSafe();
  const tomes = await db.select().from(lfLegacyTomes).orderBy(asc(lfLegacyTomes.itemId));
  const quests = await db.select().from(lfQuestFixes).orderBy(asc(lfQuestFixes.questId));

  const byClass = new Map<number, typeof tomes>();
  for (const t of tomes) {
    const arr = byClass.get(t.classId) ?? [];
    arr.push(t);
    byClass.set(t.classId, arr);
  }

  return (
    <div>
      <PageHeader
        title="Фолианты Древних Знаний и авто-выполнение квестов"
        subtitle="Возвращённые способности WotLK / Cataclysm / MoP / WoD из клиентских Spell.db2 билда 26124. Строгая проверка класса, проверка дубликатов, добавление в общую вкладку книги заклинаний."
        right={<Badge tone="warn">цена тома 5000 Сущности</Badge>}
      />

      <div className="grid md:grid-cols-2 xl:grid-cols-3 gap-4 mb-6">
        {Array.from(byClass.entries()).map(([classId, list]) => (
          <Panel key={classId} title={CLASS_NAMES[classId] ?? "Любой класс"} hint={`${list.length} шт.`}>
            <ul className="space-y-1.5">
              {list.map((t) => (
                <li key={t.itemId} className="flex items-center justify-between gap-2 text-[12.5px]">
                  <span className="text-slate-300">{t.nameRu}</span>
                  <span className="lf-mono text-[11px] text-slate-600 shrink-0">#{t.spellId}</span>
                </li>
              ))}
            </ul>
          </Panel>
        ))}
      </div>

      <Panel title="Авто-выполнение проблемных квестов" hint={`${quests.length} записей · таблица custom_autocomplete_quests`}>
        <p className="text-[12.5px] text-slate-400 mb-3">
          Хук PlayerScript сверяет ID квеста с таблицей, мгновенно засчитывает задание, открывает окно сдачи награды
          и пишет в чат: «[Система LegionForge]: Задание временно завершено автоматически. Приятной игры!» + компенсация 25 Сущности.
        </p>
        <div className="flex flex-wrap gap-1.5">
          {quests.map((q) => (
            <span key={q.questId} className="lf-badge" title={`${q.reason} · +${q.compensation} Сущности`}>
              {q.questId}
            </span>
          ))}
        </div>
      </Panel>
    </div>
  );
}
