import FileExplorer from "@/components/file-explorer";
import { Badge, PageHeader } from "@/components/ui";

export const dynamic = "force-dynamic";

export default function FilesPage() {
  return (
    <div>
      <PageHeader
        title="Файлы платформы LEGIONFORGE"
        subtitle="Полное дерево проекта: START.bat / PANEL.bat / legionforge.bin, портативные утилиты в /tools, модернизированные исходники ядра в /source (включая /src/server/scripts/Custom), готовые конфиги и бинарники в /server, базы данных в /sql, веб-панель и документация."
        right={<Badge tone="ok">скачивание файлов и архивов</Badge>}
      />
      <FileExplorer />
    </div>
  );
}
