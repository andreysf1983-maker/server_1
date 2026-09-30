import ConfigEditor from "@/components/config-editor";
import { Badge, PageHeader } from "@/components/ui";

export const dynamic = "force-dynamic";

export default function ConfigPage() {
  return (
    <div>
      <PageHeader
        title="Конфигуратор сборки"
        subtitle="Визуальное редактирование worldserver.conf и modules.conf без блокнота: рейты, имя реалма, MOTD, экономика на Сущности пробуждения, потолки ilvl и включение/отключение всех кастомных модулей."
        right={<Badge tone="info">запись прямо в server/configs/</Badge>}
      />
      <ConfigEditor />
    </div>
  );
}
