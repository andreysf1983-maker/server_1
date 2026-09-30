// ============================================================================
//  LegionForge — Master Collector & Vendor NPC (Entry 950100)
//  Target: LegionForgeCore 7.3.5 (26124) · Client Build 7.3.5.26124
// ============================================================================
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "CharacterService.h"
#include "Chat.h"
#include "Creature.h"
#include "Item.h"
#include "Player.h"
#include "WorldSession.h"
#include "LegionForge_Config.h"

namespace LegionForge
{
    bool LearnLegacySpellChecked(Player* player, LegacySpellEntry const* entry, bool chargeCurrency);
    bool ApplyItemUpgrade(Player* player, Item* targetItem, bool isLegendaryUpgrade, bool chargeEssenceDirectly);
    bool IsLegendaryItem(Item const* item);
}

enum VendorGossipActions : uint32
{
    ACTION_MAIN_MENU          = 1,
    ACTION_SHOP_INFO          = 2,
    ACTION_MENU_UPGRADE_LEG   = 10,
    ACTION_MENU_UPGRADE_GEAR  = 11,
    ACTION_BUY_CONCENTRATE    = 12,
    ACTION_BUY_ENDGAME_ITEM   = 13,
    ACTION_MENU_LEGACY_SPELLS = 20,
    ACTION_MENU_SERVICES      = 30,
    ACTION_SVC_RENAME         = 31,
    ACTION_SVC_CUSTOMIZE      = 32,
    ACTION_SVC_RACE           = 33,
    ACTION_SVC_FACTION        = 34,

    ACTION_UPGRADE_LEG_SLOT_BASE  = 1000, // + slot
    ACTION_UPGRADE_GEAR_SLOT_BASE = 2000, // + slot
    ACTION_LEARN_LEGACY_BASE      = 3000  // + index in LegacySpells
};

class npc_legionforge_vendor : public CreatureScript
{
public:
    npc_legionforge_vendor() : CreatureScript("npc_legionforge_vendor") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return true;

        ShowMainMenu(player, creature);
        return true;
    }

    void ShowMainMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        uint32 currencyId = sConfigMgr->GetIntDefault("LegionForge.CurrencyId", LegionForge::CURRENCY_WAKENING_ESSENCE);
        uint32 balance = player->GetCurrency(currencyId);

        std::ostringstream balText;
        balText << "Ваш баланс: " << balance << " Сущностей пробуждения";
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, balText.str().c_str(), GOSSIP_SENDER_MAIN, ACTION_SHOP_INFO);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Улучшить надетую легендарку (+5 ilvl до 1200) — 800 Сущностей", GOSSIP_SENDER_MAIN, ACTION_MENU_UPGRADE_LEG);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Закалить эндгейм-экипировку (985 -> 1000 ilvl) — 1500 Сущностей", GOSSIP_SENDER_MAIN, ACTION_MENU_UPGRADE_GEAR);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "Купить реагент «Концентрат силы» в сумку (800 Сущностей)", GOSSIP_SENDER_MAIN, ACTION_BUY_CONCENTRATE);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "Купить реагент «Эссенция закалки» в сумку (1500 Сущностей)", GOSSIP_SENDER_MAIN, ACTION_BUY_ENDGAME_ITEM);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Забытые способности вашего класса (5000 Сущностей)", GOSSIP_SENDER_MAIN, ACTION_MENU_LEGACY_SPELLS);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Услуги персонажа (имя, внешность, раса, фракция)", GOSSIP_SENDER_MAIN, ACTION_MENU_SERVICES);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (!player || !creature)
            return true;

        player->PlayerTalkClass->ClearMenus();
        uint32 currencyId = sConfigMgr->GetIntDefault("LegionForge.CurrencyId", LegionForge::CURRENCY_WAKENING_ESSENCE);

        if (action == ACTION_MAIN_MENU)
        {
            ShowMainMenu(player, creature);
            return true;
        }
        if (action == ACTION_SHOP_INFO)
        {
            player->GetSession()->SendNotification("Откройте магазин кнопкой W на микроменю — все товары продаются за Сущности пробуждения!");
            ShowMainMenu(player, creature);
            return true;
        }
        if (action == ACTION_BUY_CONCENTRATE)
        {
            BuyReagent(player, LegionForge::ITEM_CONCENTRATE_CUSTOM, LegionForge::PRICE_CONCENTRATE);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
        if (action == ACTION_BUY_ENDGAME_ITEM)
        {
            BuyReagent(player, LegionForge::ITEM_ENDGAME_CUSTOM, LegionForge::PRICE_ENDGAME_TEMPER);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
        if (action == ACTION_MENU_UPGRADE_LEG)
        {
            bool found = false;
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    if (LegionForge::IsLegendaryItem(item))
                    {
                        found = true;
                        std::ostringstream ss;
                        ss << "Улучшить слот #" << uint32(slot) << " (" << item->GetItemLevel(player->getLevel()) << " ilvl -> +5)";
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, ss.str().c_str(), GOSSIP_SENDER_MAIN, ACTION_UPGRADE_LEG_SLOT_BASE + slot);
                    }
                }
            }
            if (!found)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "На вас нет надетых легендарных предметов", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Назад", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }
        if (action == ACTION_MENU_UPGRADE_GEAR)
        {
            bool found = false;
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    uint32 ilvl = item->GetItemLevel(player->getLevel());
                    if (!LegionForge::IsLegendaryItem(item) && ilvl >= LegionForge::ENDGAME_ILVL_MIN && ilvl < LegionForge::ENDGAME_ILVL_CAP)
                    {
                        found = true;
                        std::ostringstream ss;
                        ss << "Закалить слот #" << uint32(slot) << " (" << ilvl << " -> 1000 ilvl)";
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, ss.str().c_str(), GOSSIP_SENDER_MAIN, ACTION_UPGRADE_GEAR_SLOT_BASE + slot);
                    }
                }
            }
            if (!found)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Нет надетой экипировки 985–999 ilvl", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Назад", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }
        if (action == ACTION_MENU_LEGACY_SPELLS)
        {
            for (uint32 i = 0; i < LegionForge::LegacySpellsCount; ++i)
            {
                auto const& e = LegionForge::LegacySpells[i];
                if (e.classId == player->getClass())
                {
                    std::ostringstream ss;
                    ss << e.nameRu << (player->HasSpell(e.spellId) ? " [Уже изучено]" : " (5000 Сущностей)");
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, ss.str().c_str(), GOSSIP_SENDER_MAIN, ACTION_LEARN_LEGACY_BASE + i);
                }
            }
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Назад", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }
        if (action == ACTION_MENU_SERVICES)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Смена имени персонажа (1000 Сущностей)", GOSSIP_SENDER_MAIN, ACTION_SVC_RENAME);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Смена внешности и пола (600 Сущностей)", GOSSIP_SENDER_MAIN, ACTION_SVC_CUSTOMIZE);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Смена расы (3000 Сущностей)", GOSSIP_SENDER_MAIN, ACTION_SVC_RACE);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Смена фракции (5000 Сущностей)", GOSSIP_SENDER_MAIN, ACTION_SVC_FACTION);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Назад", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }

        if (action >= ACTION_SVC_RENAME && action <= ACTION_SVC_FACTION)
        {
            uint32 cost = (action == ACTION_SVC_RENAME) ? 1000 : (action == ACTION_SVC_CUSTOMIZE) ? 600 : (action == ACTION_SVC_RACE) ? 3000 : 5000;
            if (player->GetCurrency(currencyId) < cost)
            {
                player->GetSession()->SendNotification("Недостаточно Сущностей пробуждения (требуется %u).", cost);
                player->CLOSE_GOSSIP_MENU();
                return true;
            }
            player->ModifyCurrency(currencyId, -int32(cost));
            if (action == ACTION_SVC_RENAME)
                sCharacterService->SetRename(player);
            else if (action == ACTION_SVC_CUSTOMIZE)
                sCharacterService->Customize(player);
            else if (action == ACTION_SVC_RACE)
                sCharacterService->ChangeRace(player);
            else if (action == ACTION_SVC_FACTION)
                sCharacterService->ChangeFaction(player);

            player->GetSession()->SendNotification("Услуга активирована! Перезайдите в меню выбора персонажей.");
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        if (action >= ACTION_UPGRADE_LEG_SLOT_BASE && action < ACTION_UPGRADE_LEG_SLOT_BASE + EQUIPMENT_SLOT_END)
        {
            uint8 slot = uint8(action - ACTION_UPGRADE_LEG_SLOT_BASE);
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                LegionForge::ApplyItemUpgrade(player, item, true, true);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        if (action >= ACTION_UPGRADE_GEAR_SLOT_BASE && action < ACTION_UPGRADE_GEAR_SLOT_BASE + EQUIPMENT_SLOT_END)
        {
            uint8 slot = uint8(action - ACTION_UPGRADE_GEAR_SLOT_BASE);
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                LegionForge::ApplyItemUpgrade(player, item, false, true);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        if (action >= ACTION_LEARN_LEGACY_BASE && action < ACTION_LEARN_LEGACY_BASE + LegionForge::LegacySpellsCount)
        {
            uint32 idx = action - ACTION_LEARN_LEGACY_BASE;
            LegionForge::LearnLegacySpellChecked(player, &LegionForge::LegacySpells[idx], true);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        player->CLOSE_GOSSIP_MENU();
        return true;
    }

private:
    void BuyReagent(Player* player, uint32 itemId, uint32 price)
    {
        uint32 currencyId = sConfigMgr->GetIntDefault("LegionForge.CurrencyId", LegionForge::CURRENCY_WAKENING_ESSENCE);
        if (player->GetCurrency(currencyId) < price)
        {
            player->GetSession()->SendNotification("Недостаточно Сущностей пробуждения.");
            return;
        }
        if (player->AddItem(itemId, 1))
        {
            player->ModifyCurrency(currencyId, -int32(price));
            player->GetSession()->SendNotification("Реагент добавлен в вашу сумку!");
        }
        else
        {
            player->GetSession()->SendNotification("Освободите место в сумке или используйте прямое улучшение в меню НПС.");
        }
    }
};

void AddSC_LegionForge_VendorNPC()
{
    new npc_legionforge_vendor();
}
