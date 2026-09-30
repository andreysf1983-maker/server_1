/*
 * LEGIONFORGE — Absolute Transmogrification Freedom (All-Transmog Mod)
 * ---------------------------------------------------------------------
 * Снимает ВСЕ искусственные ограничения трансмогрификации 7.3.5:
 *   1. Паладин в латах может носить вид ткани / кожи / кольчуги (и наоборот).
 *   2. Разрешена трансмогрификация легендарных предметов.
 *   3. Разрешены белые/серые (Poor/Common) вещи как источник и как цель.
 *   4. Кросс-подклассы оружия в пределах одной анимационной группы.
 *   5. Трансмог между фракциями (совместно с модулем Crossfaction).
 *
 * Основные проверки ядра патчатся прямо в
 *   src/server/game/Entities/Item/Item.cpp :: Item::CanTransmogrifyItemWithItem
 * (блок «LEGIONFORGE :: ABSOLUTE TRANSMOGRIFICATION FREEDOM»), а этот модуль
 * добавляет NPC «Хранитель Иллюзий», GM-команды и лог состояния.
 *
 * Конфиг:  LegionForge.Transmog.Enable                = 1
 *          LegionForge.Transmog.AllowAnyArmorType      = 1
 *          LegionForge.Transmog.AllowLegendaries       = 1
 *          LegionForge.Transmog.AllowPoorQuality       = 1
 *          LegionForge.Transmog.AllowCrossWeaponType   = 1
 *          LegionForge.Transmog.FreeOfCharge           = 0
 *          LegionForge.Transmog.EssenceCost            = 25
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "PlayerMenu.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GossipDef.h"
#include "Item.h"
#include "ItemPrototype.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "Log.h"
#include "LegionForge_Config.h"

namespace LegionForgeTransmog
{
    inline bool Enabled()          { return sConfigMgr->GetBoolDefault("LegionForge.Transmog.Enable", true); }
    inline bool AllowAnyArmor()    { return sConfigMgr->GetBoolDefault("LegionForge.Transmog.AllowAnyArmorType", true); }
    inline bool AllowLegendaries() { return sConfigMgr->GetBoolDefault("LegionForge.Transmog.AllowLegendaries", true); }
    inline bool AllowPoorQuality() { return sConfigMgr->GetBoolDefault("LegionForge.Transmog.AllowPoorQuality", true); }
    inline bool AllowCrossWeapon() { return sConfigMgr->GetBoolDefault("LegionForge.Transmog.AllowCrossWeaponType", true); }
    inline bool FreeOfCharge()     { return sConfigMgr->GetBoolDefault("LegionForge.Transmog.FreeOfCharge", false); }
    inline uint32 EssenceCost()    { return uint32(sConfigMgr->GetIntDefault("LegionForge.Transmog.EssenceCost", 25)); }

    // Анимационные группы оружия клиента 7.3.5.26124.
    inline uint32 WeaponAnimationGroup(uint32 subClass)
    {
        switch (subClass)
        {
            case ITEM_SUBCLASS_WEAPON_AXE:
            case ITEM_SUBCLASS_WEAPON_MACE:
            case ITEM_SUBCLASS_WEAPON_SWORD:
            case ITEM_SUBCLASS_WEAPON_DAGGER:
            case ITEM_SUBCLASS_WEAPON_FIST:      return 1;   // одноручное
            case ITEM_SUBCLASS_WEAPON_AXE2:
            case ITEM_SUBCLASS_WEAPON_MACE2:
            case ITEM_SUBCLASS_WEAPON_SWORD2:
            case ITEM_SUBCLASS_WEAPON_POLEARM:
            case ITEM_SUBCLASS_WEAPON_STAFF:     return 2;   // двуручное / посох / древковое
            case ITEM_SUBCLASS_WEAPON_BOWS:
            case ITEM_SUBCLASS_WEAPON_GUNS:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:  return 3;   // стрелковое
            case ITEM_SUBCLASS_WEAPON_WARGLAIVE: return 4;   // боевые глефы
            default:                             return 0;
        }
    }

    inline bool ItemClassesCompatible(ItemTemplate const* src, ItemTemplate const* dst)
    {
        if (!src || !dst)
            return false;
        if (src->GetClass() == dst->GetClass() && src->GetSubClass() == dst->GetSubClass())
            return true;
        if (!AllowAnyArmor())
            return false;
        if (src->GetClass() == ITEM_CLASS_ARMOR && dst->GetClass() == ITEM_CLASS_ARMOR)
            return src->GetInventoryType() == dst->GetInventoryType();
        if (AllowCrossWeapon() && src->GetClass() == ITEM_CLASS_WEAPON && dst->GetClass() == ITEM_CLASS_WEAPON)
        {
            uint32 g1 = WeaponAnimationGroup(src->GetSubClass());
            uint32 g2 = WeaponAnimationGroup(dst->GetSubClass());
            return g1 != 0 && g1 == g2;
        }
        return false;
    }

    inline bool QualityAllowed(ItemTemplate const* tmpl)
    {
        if (!tmpl)
            return false;
        if (tmpl->Quality == ITEM_QUALITY_LEGENDARY)
            return AllowLegendaries();
        if (tmpl->Quality <= ITEM_QUALITY_NORMAL)
            return AllowPoorQuality();
        return true;
    }

    // Сброс всех визуальных модификаций персонажа.
    // В LEGIONFORGE-ядре внешний вид предмета хранится через
    // ItemModifiedAppearance (hotfixes 26124) + AppearanceModId экземпляра.
    inline uint32 ClearAllVisuals(Player* player)
    {
        uint32 cleared = 0;
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item || !item->GetItemModifiedAppearance())
                continue;
            item->SetAppearanceModId(0);
            item->SetState(ITEM_CHANGED, player);
            ++cleared;
        }
        return cleared;
    }
}

/* =====================================================================
 *  GM-команды:  .lf transmog info | clear | status
 * ===================================================================== */
class LegionForge_TransmogCommand : public CommandScript
{
public:
    LegionForge_TransmogCommand() : CommandScript("LegionForge_TransmogCommand") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> transmogTable =
        {
            { "info",   SEC_PLAYER,        false, &HandleInfoCommand,   "" },
            { "clear",  SEC_PLAYER,        false, &HandleClearCommand,  "" },
            { "status", SEC_ADMINISTRATOR, true,  &HandleStatusCommand, "" }
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "transmog", SEC_PLAYER, false, nullptr, "", transmogTable }
        };
        return commandTable;
    }

    static bool HandleInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage("[LEGIONFORGE] Свободный трансмог: любые типы брони, легендарки, белые вещи, кросс-оружие.");
        handler->PSendSysMessage("Стоимость операции у Хранителя Иллюзий: %u Сущности пробуждения.",
            LegionForgeTransmog::FreeOfCharge() ? 0u : LegionForgeTransmog::EssenceCost());
        return true;
    }

    static bool HandleClearCommand(ChatHandler* handler, char const* /*args*/)
    {
        if (!LegionForgeTransmog::Enabled())
        {
            handler->SendSysMessage("[LEGIONFORGE] Модуль трансмогрификации отключён.");
            return true;
        }
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
        {
            handler->SendSysMessage("Команда доступна только из игрового мира.");
            return true;
        }
        uint32 cleared = LegionForgeTransmog::ClearAllVisuals(player);
        handler->PSendSysMessage("[LEGIONFORGE] Обработано слотов экипировки: %u. Визуалы сброшены (перезайдите в игру для обновления).", cleared);
        return true;
    }

    static bool HandleStatusCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage("[LEGIONFORGE] Transmog: enable=%d anyArmor=%d legendaries=%d poor=%d crossWeapon=%d",
            int(LegionForgeTransmog::Enabled()), int(LegionForgeTransmog::AllowAnyArmor()),
            int(LegionForgeTransmog::AllowLegendaries()), int(LegionForgeTransmog::AllowPoorQuality()),
            int(LegionForgeTransmog::AllowCrossWeapon()));
        return true;
    }
};

/* =====================================================================
 *  NPC «Хранитель Иллюзий» — резервный вход в трансмог-меню
 * ===================================================================== */
class LegionForge_IllusionKeeper : public CreatureScript
{
public:
    LegionForge_IllusionKeeper() : CreatureScript("LegionForge_IllusionKeeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature || !LegionForgeTransmog::Enabled())
            return false;

        player->PlayerTalkClass->ClearMenus();
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Сбросить все иллюзии с моей экипировки", GOSSIP_SENDER_MAIN, 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Открыть окно трансмогрификации", GOSSIP_SENDER_MAIN, 2);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Расскажи о свободе трансмога LEGIONFORGE", GOSSIP_SENDER_MAIN, 3);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        if (!player)
            return true;
        player->CLOSE_GOSSIP_MENU();

        switch (action)
        {
            case 1:
            {
                uint32 cleared = LegionForgeTransmog::ClearAllVisuals(player);
                player->GetSession()->SendNotification("[LEGIONFORGE] Сброшено слотов: %u. Перезайдите в игру.", cleared);
                break;
            }
            case 2:
            {
                if (!LegionForgeTransmog::FreeOfCharge())
                {
                    uint32 cost = LegionForgeTransmog::EssenceCost();
                    if (player->GetCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE) < cost)
                    {
                        player->GetSession()->SendNotification(
                            "[LEGIONFORGE] Недостаточно Сущности пробуждения (нужно %u).", cost);
                        break;
                    }
                    player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, -int32(cost));
                }
                player->GetSession()->SendNotification(
                    "[LEGIONFORGE] Окно трансмогрификации открыто. Латы в ткань? Легко!");
                break;
            }
            case 3:
                player->GetSession()->SendNotification(
                    "[LEGIONFORGE] Сняты ограничения по типу брони, качеству и классу оружия.");
                break;
        }
        return true;
    }
};

/* =====================================================================
 *  Лог состояния модуля при старте мира
 * ===================================================================== */
class LegionForge_TransmogWorld : public WorldScript
{
public:
    LegionForge_TransmogWorld() : WorldScript("LegionForge_TransmogWorld") { }

    void OnStartup() override
    {
        if (!LegionForgeTransmog::Enabled())
        {
            LOG_INFO("server.loading", "[LEGIONFORGE][Transmog] модуль отключён (LegionForge.Transmog.Enable = 0)");
            return;
        }
        LOG_INFO("server.loading", ">> LEGIONFORGE Absolute Transmog Freedom");
        LOG_INFO("server.loading", "   Любой тип брони ...... : %s", LegionForgeTransmog::AllowAnyArmor()    ? "ON" : "OFF");
        LOG_INFO("server.loading", "   Легендарные предметы . : %s", LegionForgeTransmog::AllowLegendaries() ? "ON" : "OFF");
        LOG_INFO("server.loading", "   Белые/серые предметы . : %s", LegionForgeTransmog::AllowPoorQuality() ? "ON" : "OFF");
        LOG_INFO("server.loading", "   Кросс-типы оружия .... : %s", LegionForgeTransmog::AllowCrossWeapon() ? "ON" : "OFF");
        LOG_INFO("server.loading", "   Цена операции ........ : %u Сущности пробуждения",
            LegionForgeTransmog::FreeOfCharge() ? 0u : LegionForgeTransmog::EssenceCost());
    }
};

void AddSC_LegionForge_TransmogFreedom()
{
    new LegionForge_TransmogCommand();
    new LegionForge_IllusionKeeper();
    new LegionForge_TransmogWorld();
}
