# Патч BattlePay: оплата магазина Сущностями пробуждения

Ядро LegionForgeCore 7.3.5 (26124) по умолчанию списывает оплату в окне магазина (BattlePay,
кнопка **W**) с **баланса доната аккаунта** (`Player::GetDonateTokens` в
`src/server/game/BattlePay/BattlePayHandler.cpp`), а не с игровой валюты.

Чтобы магазин тратил **Сущности пробуждения (currency 1533)** — ту же валюту, на
которую в Legion покупаются и улучшаются легендарки, — внесите минимальное
изменение в проверку баланса. Оно затронет **только** функцию покупки и не
удалит и не сломает остальной BattlePay-код.

> Файл ядра: `src/server/game/BattlePay/BattlePayHandler.cpp`
> (точное имя файла/строки могут незначительно отличаться в вашей ревизии —
> ищите `GetDonateTokens` / `Purchase`).

## Что найти

Внутри обработчика покупки найдите проверку баланса доната, похожую на:

```cpp
uint32 balance = player->GetDonateTokens();
if (balance < product.NormalPriceFixedPoint)
{
    // отказано: недостаточно донат-валюты
}
```

## На что заменить

Замените источник баланса на игровую валюту Сущностей пробуждения и списывайте
её после успешной выдачи товара:

```cpp
// LegionForge: shop charges Wakening Essence (currency 1533) instead of donate.
constexpr uint32 ESSENCE_CURRENCY = 1533;

uint32 balance = player->GetCurrency(ESSENCE_CURRENCY);
if (balance < product.NormalPriceFixedPoint)
{
    // отказано: недостаточно Сущностей пробуждения
}

// ... после успешной выдачи предмета/услуги ...
player->ModifyCurrency(ESSENCE_CURRENCY,
                       -static_cast<int32_t>(product.NormalPriceFixedPoint));
```

## Заметки

- Цены в `battlepay_shop.sql` (экспорт из панели) уже указаны в единицах
  Сущностей — дополнительное масштабирование не требуется.
- Если хотите оставить донат-оплату как альтернативу, добавьте флаг в
  `battlepay_product.Flags` и выбирайте источник оплаты по нему.
- Изменение обратимо: достаточно вернуть исходную строку `GetDonateTokens()`.

Это точечный патч ядра. Всё остальное (каталог, товары, лут) живёт в базе
данных и панели и ядро не трогает.
