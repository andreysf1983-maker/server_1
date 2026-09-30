/*
 * LEGIONFORGE — In-Game Shop (BattlePay, кнопка «W») за Сущность пробуждения
 * ---------------------------------------------------------------------
 * Клиент 7.3.5.26124 открывает магазин Blizzard по кнопке «W» на микро-панели.
 * Ядро LEGIONFORGE отдаёт каталог из БД, а валютой магазина является
 * родная валюта Легиона — Сущность пробуждения (Currency ID 1533),
 * поэтому магазин работает БЕЗ реальных денег и без pay-to-win.
 *
 * Данный модуль:
 *   - проверяет и логирует валюту магазина при старте мира;
 *   - даёт GM-команды управления каталогом (.lf shop ...);
 *   - добавляет NPC «Хранитель Кузни», дублирующего товары магазина
 *     (для игроков, у которых кнопка «W» недоступна).
 *
 * Конфиг: LegionForge.BattlePay.Enable      = 1
 *         LegionForge.BattlePay.CurrencyId  = 1533
 *         LegionForge.BattlePay.NpcEntry    = 950100
 */
#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "PlayerMenu.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "Log.h"
#include "LegionForge_Config.h"

namespace LegionForgeShop
{
    struct ShopEntry
    {
        uint32 Id = 0;
        uint32 ItemId = 0;
        uint32 Count = 1;
        uint32 Cost = 0;
        uint8  Category = 0;   // 1 трансмог, 2 маунты, 3 питомцы, 4 услуги, 5 реагенты, 6 фолианты
        std::string NameRu;
    };

    std::vector<ShopEntry> Catalog;

    inline bool Enabled() { return sConfigMgr->GetBoolDefault("LegionForge.BattlePay.Enable", true); }
    inline uint32 Currency() { return uint32(sConfigMgr->GetIntDefault("LegionForge.BattlePay.CurrencyId",
        LegionForge::CURRENCY_WAKENING_ESSENCE)); }

    inline void LoadCatalog()
    {
        Catalog.clear();
        if (QueryResult result = WorldDatabase.Query(
            "SELECT Id, ItemId, Count, Cost, Category, NameRu FROM custom_legionforge_shop WHERE Enabled = 1 ORDER BY Category, Cost"))
        {
            do
            {
                Field* f = result->Fetch();
                ShopEntry e;
                e.Id = f[0].GetUInt32(); e.ItemId = f[1].GetUInt32(); e.Count = f[2].GetUInt32();
                e.Cost = f[3].GetUInt32(); e.Category = f[4].GetUInt8(); e.NameRu = f[5].GetString();
                Catalog.push_back(e);
            } while (result->NextRow());
        }
        LOG_INFO("server.loading", "[LEGIONFORGE][BattlePay] позиций в каталоге: %u (валюта %u)",
            uint32(Catalog.size()), Currency());
    }

    inline char const* CategoryName(uint8 c)
    {
        switch (c)
        {
            case 1: return "Редчайший трансмог";
            case 2: return "Маунты";
            case 3: return "Питомцы";
            case 4: return "Услуги персонажа";
            case 5: return "Реагенты улучшения";
            case 6: return "Фолианты забытых способностей";
            default: return "Прочее";
        }
    }
}

/* =====================================================================
 *  NPC «Хранитель Кузни» — зеркало магазина кнопки «W»
 * ===================================================================== */
class LegionForge_ForgeKeeper : public CreatureScript
{
public:
    LegionForge_ForgeKeeper() : CreatureScript("LegionForge_ForgeKeeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return false;
        ShowCategories(player, creature);
        return true;
    }

    void ShowCategories(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        char buf[256];
        snprintf(buf, sizeof(buf), "Баланс: %u Сущности пробуждения (валюта магазина)",
            player->GetCurrency(LegionForgeShop::Currency()));
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, buf, GOSSIP_SENDER_MAIN, 0);
        for (uint8 c = 1; c <= 6; ++c)
        {
            snprintf(buf, sizeof(buf), "%s", LegionForgeShop::CategoryName(c));
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, buf, GOSSIP_SENDER_MAIN, 100 + c);
        }
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Открыть магазин кнопкой «W» на микро-панели", GOSSIP_SENDER_MAIN, 900);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowCategory(Player* player, Creature* creature, uint8 category)
    {
        player->PlayerTalkClass->ClearMenus();
        char buf[256];
        uint32 shown = 0;
        for (auto const& e : LegionForgeShop::Catalog)
        {
            if (e.Category != category)
                continue;
            snprintf(buf, sizeof(buf), "%s — %u Сущности (x%u)", e.NameRu.c_str(), e.Cost, e.Count);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, buf, GOSSIP_SENDER_MAIN, 1000 + e.Id);
            if (++shown >= 20) break;
        }
        if (!shown)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Категория пуста — обновите каталог (.lf shop reload)", GOSSIP_SENDER_MAIN, 0);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Назад", GOSSIP_SENDER_MAIN, 1);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (!player || !creature)
            return true;
        player->CLOSE_GOSSIP_MENU();

        if (action == 0) { ShowCategories(player, creature); return true; }
        if (action == 1) { ShowCategories(player, creature); return true; }
        if (action == 900)
        {
            player->GetSession()->SendNotification(
                "[LEGIONFORGE] Магазин открывается кнопкой «W» на микро-панели. Валюта — Сущность пробуждения (1533).");
            return true;
        }
        if (action >= 101 && action <= 106) { ShowCategory(player, creature, uint8(action - 100)); return true; }
        if (action >= 1001)
        {
            uint32 wanted = action - 1000;
            for (auto const& e : LegionForgeShop::Catalog)
            {
                if (e.Id != wanted)
                    continue;
                if (player->GetCurrency(LegionForgeShop::Currency()) < e.Cost)
                {
                    player->GetSession()->SendNotification(
                        "[LEGIONFORGE] Недостаточно Сущности пробуждения: нужно %u, у вас %u.",
                        e.Cost, player->GetCurrency(LegionForgeShop::Currency()));
                    return true;
                }
                player->ModifyCurrency(LegionForgeShop::Currency(), -int32(e.Cost));
                player->AddItem(e.ItemId, e.Count);
                player->GetSession()->SendNotification(
                    "|cff00ff00[LEGIONFORGE]|r Куплено: %s (x%u) за %u Сущности пробуждения.",
                    e.NameRu.c_str(), e.Count, e.Cost);
                return true;
            }
            player->GetSession()->SendNotification("[LEGIONFORGE] Товар не найден в каталоге. Выполните .lf shop reload");
            return true;
        }
        return true;
    }
};

/* =====================================================================
 *  Мир + GM-команды каталога
 * ===================================================================== */
class LegionForge_BattlePayWorld : public WorldScript
{
public:
    LegionForge_BattlePayWorld() : WorldScript("LegionForge_BattlePayWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        if (LegionForgeShop::Enabled())
            LegionForgeShop::LoadCatalog();
    }

    void OnStartup() override
    {
        LOG_INFO("server.loading", ">> LEGIONFORGE BattlePay Shop ..... : %s", LegionForgeShop::Enabled() ? "ON" : "OFF");
        LOG_INFO("server.loading", "   Валюта магазина (кнопка «W») ... : %u (Сущность пробуждения)",
            LegionForgeShop::Currency());
        LOG_INFO("server.loading", "   Позиций каталога ............... : %u", uint32(LegionForgeShop::Catalog.size()));
    }
};

class LegionForge_ShopCommand : public CommandScript
{
public:
    LegionForge_ShopCommand() : CommandScript("LegionForge_ShopCommand") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> shopTable =
        {
            { "reload",   SEC_ADMINISTRATOR, true,  &HandleReloadCommand,   "" },
            { "list",     SEC_ADMINISTRATOR, false, &HandleListCommand,     "" },
            { "currency", SEC_ADMINISTRATOR, true,  &HandleCurrencyCommand, "" }
        };
        static std::vector<ChatCommand> table =
        {
            { "shop", SEC_ADMINISTRATOR, false, nullptr, "", shopTable }
        };
        return table;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        LegionForgeShop::LoadCatalog();
        handler->SendSysMessage("[LEGIONFORGE] Каталог магазина перезагружен.");
        return true;
    }

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        for (auto const& e : LegionForgeShop::Catalog)
            handler->PSendSysMessage("#%u [%s] %s (item %u x%u) = %u Сущности",
                e.Id, LegionForgeShop::CategoryName(e.Category), e.NameRu.c_str(), e.ItemId, e.Count, e.Cost);
        return true;
    }

    static bool HandleCurrencyCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage("[LEGIONFORGE] Валюта магазина: %u (ожидается 1533 — Сущность пробуждения).",
            LegionForgeShop::Currency());
        return true;
    }
};

void AddSC_LegionForge_BattlePayEssence()
{
    new LegionForge_ForgeKeeper();
    new LegionForge_BattlePayWorld();
    new LegionForge_ShopCommand();
}
