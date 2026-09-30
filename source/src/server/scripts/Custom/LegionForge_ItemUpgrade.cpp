// ============================================================================
//  LegionForge — Item Upgrade Chain (Legendaries -> 1200, Endgame 985 -> 1000)
//  Target: LegionForgeCore 7.3.5 (26124) · Client Build 7.3.5.26124
// ============================================================================
#include "ScriptMgr.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "Spell.h"
#include "WorldSession.h"
#include "LegionForge_Config.h"

namespace LegionForge
{
    inline bool IsLegendaryItem(Item const* item)
    {
        if (!item || !item->GetTemplate())
            return false;
        return item->GetTemplate()->GetQuality() == ITEM_QUALITY_LEGENDARY || item->GetTemplate()->IsLegionLegendary();
    }

    inline bool ApplyItemUpgrade(Player* player, Item* targetItem, bool isLegendaryUpgrade, bool chargeEssenceDirectly)
    {
        if (!player || !targetItem || !targetItem->GetTemplate())
            return false;

        ChatHandler ch(player->GetSession());
        uint32 currentIlvl = targetItem->GetItemLevel(player->getLevel());

        uint32 legCap = sConfigMgr->GetIntDefault("LegionForge.Upgrade.LegendaryCap", LEGENDARY_ILVL_CAP);
        uint32 legStep = sConfigMgr->GetIntDefault("LegionForge.Upgrade.LegendaryStep", LEGENDARY_UPGRADE_STEP);
        uint32 endMin = sConfigMgr->GetIntDefault("LegionForge.Upgrade.EndgameMin", ENDGAME_ILVL_MIN);
        uint32 endCap = sConfigMgr->GetIntDefault("LegionForge.Upgrade.EndgameCap", ENDGAME_ILVL_CAP);
        uint32 currencyId = sConfigMgr->GetIntDefault("LegionForge.CurrencyId", CURRENCY_WAKENING_ESSENCE);

        uint32 newIlvl = currentIlvl;
        uint32 essenceCost = 0;

        if (isLegendaryUpgrade)
        {
            if (!IsLegendaryItem(targetItem))
            {
                player->GetSession()->SendNotification("Концентрат силы можно применить только к легендарному предмету!");
                return false;
            }
            if (currentIlvl >= legCap)
            {
                player->GetSession()->SendNotification("Этот легендарный предмет уже достиг предела %u ilvl.", legCap);
                return false;
            }
            newIlvl = std::min(currentIlvl + legStep, legCap);
            essenceCost = sConfigMgr->GetIntDefault("LegionForge.Upgrade.ConcentrateCost", PRICE_CONCENTRATE);
        }
        else
        {
            if (IsLegendaryItem(targetItem))
            {
                player->GetSession()->SendNotification("Для легендарных предметов используйте Концентрат силы.");
                return false;
            }
            if (currentIlvl < endMin)
            {
                player->GetSession()->SendNotification("Эссенция закалки требует предмет минимум %u уровня (сейчас %u).", endMin, currentIlvl);
                return false;
            }
            if (currentIlvl >= endCap)
            {
                player->GetSession()->SendNotification("Этот предмет уже закалён до предела %u ilvl.", endCap);
                return false;
            }
            newIlvl = endCap;
            essenceCost = sConfigMgr->GetIntDefault("LegionForge.Upgrade.EndgameCost", PRICE_ENDGAME_TEMPER);
        }

        if (chargeEssenceDirectly)
        {
            if (player->GetCurrency(currencyId) < essenceCost)
            {
                player->GetSession()->SendNotification("Недостаточно Сущностей пробуждения (нужно %u).", essenceCost);
                return false;
            }
            player->ModifyCurrency(currencyId, -int32(essenceCost));
        }

        uint32 delta = newIlvl - currentIlvl;
        uint32 newScale = targetItem->GetScaleIlvl() + delta;

        bool wasEquipped = targetItem->IsEquipped();
        if (wasEquipped)
            player->_ApplyItemMods(targetItem, targetItem->GetSlot(), false);

        targetItem->SetScaleIlvl(newScale);
        targetItem->SetState(ITEM_CHANGED, player);

        if (wasEquipped)
        {
            player->_ApplyItemMods(targetItem, targetItem->GetSlot(), true);
            player->SetVisibleItemSlot(targetItem->GetSlot(), targetItem);
        }

        CharacterDatabase.PExecute(
            "REPLACE INTO `character_item_upgrade` (`item_guid`, `owner_guid`, `scale_ilvl`) VALUES (%u, %u, %u)",
            targetItem->GetGUIDLow(), player->GetGUIDLow(), newScale);

        player->GetSession()->SendNotification("Предмет улучшен: %u -> %u ilvl (сокеты сохранены)!", currentIlvl, newIlvl);
        ch.PSendSysMessage("|cff00FF88[LegionForge]|r Уровень предмета повышен с |cffFFD800%u|r до |cff00FF00%u ilvl|r!", currentIlvl, newIlvl);
        return true;
    }
}

class item_legionforge_upgrade_concentrate : public ItemScript
{
public:
    item_legionforge_upgrade_concentrate() : ItemScript("item_upgrade_concentrate") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& targets) override
    {
        if (!player || !item)
            return true;

        Item* targetItem = targets.GetItemTarget();
        if (!targetItem)
        {
            // Fallback: find the first equipped legendary below cap
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                if (Item* eq = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    if (LegionForge::IsLegendaryItem(eq) && eq->GetItemLevel(player->getLevel()) < LegionForge::LEGENDARY_ILVL_CAP)
                    {
                        targetItem = eq;
                        break;
                    }
                }
            }
        }

        if (!targetItem)
        {
            player->GetSession()->SendNotification("Примените Концентрат силы на легендарный предмет (или наденьте его).");
            return true;
        }

        if (LegionForge::ApplyItemUpgrade(player, targetItem, true, false))
            player->DestroyItemCount(item->GetEntry(), 1, true);

        return true;
    }
};

class item_legionforge_upgrade_endgame : public ItemScript
{
public:
    item_legionforge_upgrade_endgame() : ItemScript("item_upgrade_endgame") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& targets) override
    {
        if (!player || !item)
            return true;

        Item* targetItem = targets.GetItemTarget();
        if (!targetItem)
        {
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                if (Item* eq = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    uint32 ilvl = eq->GetItemLevel(player->getLevel());
                    if (!LegionForge::IsLegendaryItem(eq) && ilvl >= LegionForge::ENDGAME_ILVL_MIN && ilvl < LegionForge::ENDGAME_ILVL_CAP)
                    {
                        targetItem = eq;
                        break;
                    }
                }
            }
        }

        if (!targetItem)
        {
            player->GetSession()->SendNotification("Примените Эссенцию закалки на экипировку 985+ ilvl.");
            return true;
        }

        if (LegionForge::ApplyItemUpgrade(player, targetItem, false, false))
            player->DestroyItemCount(item->GetEntry(), 1, true);

        return true;
    }
};

class player_legionforge_item_upgrade_persistence : public PlayerScript
{
public:
    player_legionforge_item_upgrade_persistence() : PlayerScript("player_legionforge_item_upgrade_persistence") { }

    void OnLogin(Player* player) override
    {
        if (!player)
            return;

        QueryResult result = CharacterDatabase.PQuery(
            "SELECT `item_guid`, `scale_ilvl` FROM `character_item_upgrade` WHERE `owner_guid` = %u",
            player->GetGUIDLow());
        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            uint32 itemGuidLow = fields[0].GetUInt32();
            uint32 scaleIlvl = fields[1].GetUInt32();

            ObjectGuid itemGuid = ObjectGuid::Create<HighGuid::Item>(itemGuidLow);
            if (Item* item = player->GetItemByGuid(itemGuid))
            {
                bool equipped = item->IsEquipped();
                if (equipped)
                    player->_ApplyItemMods(item, item->GetSlot(), false);

                item->SetScaleIlvl(scaleIlvl);

                if (equipped)
                    player->_ApplyItemMods(item, item->GetSlot(), true);
            }
        } while (result->NextRow());
    }
};

void AddSC_LegionForge_ItemUpgrade()
{
    new item_legionforge_upgrade_concentrate();
    new item_legionforge_upgrade_endgame();
    new player_legionforge_item_upgrade_persistence();
}
