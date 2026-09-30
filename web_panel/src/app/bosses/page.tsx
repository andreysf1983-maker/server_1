import BossManager from "@/components/boss-manager";
import { Badge, PageHeader } from "@/components/ui";

export const dynamic = "force-dynamic";

export default function BossesPage() {
  return (
    <div>
      <PageHeader
        title="Кастомные мировые боссы"
        subtitle="100 уникальных мини-боссов Азерота, Расколотых островов и Аргуса с C++-тактиками: фазы по здоровью, лужи под случайной целью, призыв слуг, энрейдж через 8 минут и анонсы на весь сервер. Дроп — Сущность пробуждения, реагенты всех профессий Легиона, шанс на «Концентрат силы Титанов» и редких маунтов."
        right={<Badge tone="warn">LegionForge_WorldBosses.cpp</Badge>}
      />
      <BossManager />
    </div>
  );
}
