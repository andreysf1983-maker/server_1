import PlayersManager from "@/components/players-manager";
import { Badge, PageHeader } from "@/components/ui";

export const dynamic = "force-dynamic";

export default function PlayersPage() {
  return (
    <div>
      <PageHeader
        title="Игроки, аккаунты и ИИ-боты"
        subtitle="Статистика онлайна, управление аккаунтами, выдача Сущности пробуждения (1533), престиж и Hardcore-статусы персонажей, пул ников умных ботов."
        right={<Badge tone="info">анти-инфляция: +50 в час за онлайн</Badge>}
      />
      <PlayersManager />
    </div>
  );
}
