import ShopEditor from "@/components/shop-editor";
import { Badge, PageHeader } from "@/components/ui";

export const dynamic = "force-dynamic";

export default function ShopPage() {
  return (
    <div>
      <PageHeader
        title="Внутриигровой магазин BattlePay (кнопка «W»)"
        subtitle="Полноценный магазин клиента 7.3.5.26124: открывается значком Blizzard на микро-панели. Валюта списания — Сущность пробуждения (1533), без реальных денег. Здесь же редактируются цены и состав каталога, а NPC «Хранитель Кузни» дублирует товары в Даларане и столицах."
        right={<Badge tone="warn">анти-P2W: всё фармится игрой</Badge>}
      />
      <ShopEditor />
    </div>
  );
}
