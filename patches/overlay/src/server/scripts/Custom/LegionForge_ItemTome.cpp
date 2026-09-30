// ============================================================================
//  LegionForge — Legacy Ability Tomes & Class-Checked Spell Trainer
//  Target: LegionForgeCore 7.3.5 (26124) · Client Build 7.3.5.26124
// ============================================================================
#include "ScriptMgr.h"
#include "Chat.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Player.h"
#include "SpellMgr.h"
#include "WorldSession.h"
#include "LegionForge_Config.h"

namespace LegionForge
{
    inline LegacySpellEntry const* FindLegacyByTomeItem(uint32 itemId)
    {
        for (uint32 i = 0; i < LegacySpellsCount; ++i)
            if (LegacySpells[i].tomeItemId == itemId)
                return &LegacySpells[i];
        return nullptr;
    }

    inline LegacySpellEntry const* FindLegacyBySpellId(uint32 spellId)
    {
        for (uint32 i = 0; i < LegacySpellsCount; ++i)
            if (LegacySpells[i].spellId == spellId)
                return &LegacySpells[i];
        return nullptr;
    }

    inline bool LearnLegacySpellChecked(Player* player, LegacySpellEntry const* entry, bool chargeCurrency)
    {
        if (!player || !entry)
            return false;

        ChatHandler ch(player->GetSession());

        if (player->getClass() != entry->classId)
        {
            player->GetSession()->SendNotification("Ваш класс не может изучить эту способность!");
            ch.PSendSysMessage("|cffFF4444[LegionForge]|r Способность «%s» недоступна вашему классу.", entry->nameRu);
            return false;
        }

        if (!sSpellMgr->GetSpellInfo(entry->spellId))
        {
            ch.PSendSysMessage("|cffFF4444[LegionForge]|r Заклинание #%u отсутствует в базе DBC/DB2.", entry->spellId);
            return false;
        }

        if (player->HasSpell(entry->spellId))
        {
            player->GetSession()->SendNotification("Эта способность уже изучена.");
            return false;
        }

        uint32 cost = sConfigMgr->GetIntDefault("LegionForge.Legacy.TomeCost", PRICE_LEGACY_TOME);
        uint32 currencyId = sConfigMgr->GetIntDefault("LegionForge.CurrencyId", CURRENCY_WAKENING_ESSENCE);

        if (chargeCurrency)
        {
            if (player->GetCurrency(currencyId) < cost)
            {
                player->GetSession()->SendNotification("Недостаточно Сущностей пробуждения (требуется %u).", cost);
                return false;
            }
            player->ModifyCurrency(currencyId, -int32(cost));
        }

        player->LearnSpell(entry->spellId, false);
        player->GetSession()->SendNotification("Изучена забытая способность: %s!", entry->nameRu);
        ch.PSendSysMessage("|cff00FF88[LegionForge]|r Вы изучили забытую способность |cffFFD800[%s]|r (SpellID: %u)!", entry->nameRu, entry->spellId);
        return true;
    }
}

class item_legionforge_tome : public ItemScript
{
public:
    item_legionforge_tome() : ItemScript("item_legacy_tome") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item)
            return true;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            return true;

        LegionForge::LegacySpellEntry const* entry = LegionForge::FindLegacyByTomeItem(proto->GetId());
        if (!entry)
        {
            // Fallback: check ItemEffect.db2 if attached
            for (ItemEffectEntry const* eff : proto->Effects)
            {
                if (eff && eff->SpellID)
                {
                    if (LegionForge::LegacySpellEntry const* bySpell = LegionForge::FindLegacyBySpellId(eff->SpellID))
                    {
                        entry = bySpell;
                        break;
                    }
                }
            }
        }

        if (!entry)
        {
            player->GetSession()->SendNotification("Том не связан ни с одной забытой способностью.");
            return true;
        }

        if (LegionForge::LearnLegacySpellChecked(player, entry, false))
            player->DestroyItemCount(proto->GetId(), 1, true);

        return true;
    }
};

class command_legionforge_legacy : public CommandScript
{
public:
    command_legionforge_legacy() : CommandScript("command_legionforge_legacy") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> legacySubTable =
        {
            { "list",  SEC_PLAYER, false, &HandleListCommand,  "" },
            { "learn", SEC_PLAYER, false, &HandleLearnCommand, "" }
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "legacy", SEC_PLAYER, false, nullptr, "", legacySubTable }
        };
        return commandTable;
    }

    static bool HandleListCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return true;

        uint32 cost = sConfigMgr->GetIntDefault("LegionForge.Legacy.TomeCost", LegionForge::PRICE_LEGACY_TOME);
        handler->PSendSysMessage("|cff37A7FF[LegionForge]|r Забытые способности для вашего класса (цена: %u Сущностей):", cost);
        for (uint32 i = 0; i < LegionForge::LegacySpellsCount; ++i)
        {
            auto const& e = LegionForge::LegacySpells[i];
            if (e.classId == player->getClass())
            {
                bool known = player->HasSpell(e.spellId);
                handler->PSendSysMessage("  SpellID |cffFFD800%u|r — %s %s",
                    e.spellId, e.nameRu, known ? "|cff00FF00[изучено]|r" : "|cffAAAAAA(.legacy learn <SpellID>)|r");
            }
        }
        return true;
    }

    static bool HandleLearnCommand(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player || !args || !*args)
        {
            handler->PSendSysMessage("Использование: .legacy learn <SpellID> (список: .legacy list)");
            return true;
        }

        uint32 spellId = uint32(atoi(args));
        LegionForge::LegacySpellEntry const* entry = LegionForge::FindLegacyBySpellId(spellId);
        if (!entry)
        {
            handler->PSendSysMessage("|cffFF4444[LegionForge]|r Способность #%u не входит в список разрешённых Legacy-умений.", spellId);
            return true;
        }

        LegionForge::LearnLegacySpellChecked(player, entry, true);
        return true;
    }
};

void AddSC_LegionForge_ItemTome()
{
    new item_legionforge_tome();
    new command_legionforge_legacy();
}
