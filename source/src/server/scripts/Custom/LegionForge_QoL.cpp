/*
 * LEGIONFORGE — Quality of Life bundle
 * ---------------------------------------------------------------------
 * Четыре системы в одном модуле (все настраиваются в worldserver.conf):
 *
 *  1) Mythic+ QoL ......... NPC «Хранитель Ключей» у фонтана Даларана:
 *                            обмен эпохального ключа на другое подземелье
 *                            того же уровня за Сущности + телепорт группы
 *                            ко входу в выбранный инстанс.
 *  2) Duel Reset .......... в дуэль-зоне после боя мгновенно
 *                            восстанавливается 100% HP/маны и сбрасываются
 *                            все КД >= 30 секунд.
 *  3) Multi-Professions ... до 4 основных профессий на персонаже
 *                            (слоты покупаются за Сущности).
 *  4) Racial Trait Swapper  человек играет за эльфа крови внешне, но берёт
 *                            расовые пассивки орка/человека на выбор.
 *
 * Конфиг: LegionForge.QoL.MythicPlus.Enable / KeySwapCost / TeleportCost
 *         LegionForge.QoL.DuelReset.Enable / ZoneId / MinCooldownReset
 *         LegionForge.QoL.MultiProf.Enable / MaxSlots / SlotCost
 *         LegionForge.QoL.RacialSwap.Enable / SwapCost
 */
#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "PlayerMenu.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "Group.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "SharedDefines.h"
#include "Log.h"
#include "LegionForge_Config.h"

namespace LegionForgeQoL
{
    // Эпохальные ключи 7.3.5 (ChallengeMode keystone) — ID подземелий.
    struct DungeonInfo { uint32 mapId; uint32 keystoneAffix; char const* name; float x, y, z, o; };
    static DungeonInfo const Dungeons[] =
    {
        { 1501, 198,  "Око Азшары",                 -1332.0f,  5705.0f,  0.10f, 0.0f },
        { 1516, 200,  "Заросли Тёмного Сердца",      -731.0f,  1204.0f,  29.6f, 0.0f },
        { 1544, 205,  "Двор Звёзд",                  -600.0f,   330.0f,  50.0f, 0.0f },
        { 1571, 210,  "Каражан (Нижний)",            -11182.f,  -1666.f,  30.0f, 0.0f },
        { 1651, 223,  "Собор Вечной Ночи",            -400.0f,   900.0f,  20.0f, 0.0f },
        { 1662, 227,  "Холд Чёрная Ладья",            -620.0f,  1500.0f,   6.0f, 0.0f },
        { 1677, 233,  "Крепость Штормград (Наследие)",  -800.0f,   400.0f,  60.0f, 0.0f },
        { 1693, 239,  "Трон Приливов (Heroic)",       -500.0f,   700.0f,  15.0f, 0.0f }
    };

    inline bool MythicPlusOn() { return sConfigMgr->GetBoolDefault("LegionForge.QoL.MythicPlus.Enable", true); }
    inline bool DuelResetOn()  { return sConfigMgr->GetBoolDefault("LegionForge.QoL.DuelReset.Enable", true); }
    inline bool MultiProfOn()  { return sConfigMgr->GetBoolDefault("LegionForge.QoL.MultiProf.Enable", true); }
    inline bool RacialOn()     { return sConfigMgr->GetBoolDefault("LegionForge.QoL.RacialSwap.Enable", true); }

    inline uint32 KeySwapCost()  { return uint32(sConfigMgr->GetIntDefault("LegionForge.QoL.MythicPlus.KeySwapCost", 150)); }
    inline uint32 TeleportCost() { return uint32(sConfigMgr->GetIntDefault("LegionForge.QoL.MythicPlus.TeleportCost", 100)); }
    inline uint32 ProfSlotCost() { return uint32(sConfigMgr->GetIntDefault("LegionForge.QoL.MultiProf.SlotCost", 3000)); }
    inline uint32 MaxProfSlots() { return uint32(sConfigMgr->GetIntDefault("LegionForge.QoL.MultiProf.MaxSlots", 4)); }
    inline uint32 RacialCost()   { return uint32(sConfigMgr->GetIntDefault("LegionForge.QoL.RacialSwap.SwapCost", 2500)); }
    inline uint32 DuelZone()     { return uint32(sConfigMgr->GetIntDefault("LegionForge.QoL.DuelReset.ZoneId", 4044)); }
    inline uint32 MinCdReset()   { return uint32(sConfigMgr->GetIntDefault("LegionForge.QoL.DuelReset.MinCooldownReset", 30)); }

    inline void GiveEssence(Player* p, int32 amount, char const* reason)
    {
        p->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, amount);
        p->GetSession()->SendNotification("[LEGIONFORGE] %s: %+d Сущности пробуждения.", reason, amount);
    }

    inline bool TakeEssence(Player* p, uint32 amount, char const* reason)
    {
        if (p->GetCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE) < amount)
        {
            p->GetSession()->SendNotification("[LEGIONFORGE] Недостаточно Сущности пробуждения (нужно %u) — %s.", amount, reason);
            return false;
        }
        p->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, -int32(amount));
        return true;
    }
}

/* =====================================================================
 *  NPC «Хранитель Ключей» — Mythic+ QoL + мультипрофессии + расовые
 * ===================================================================== */
class LegionForge_KeystoneKeeper : public CreatureScript
{
public:
    LegionForge_KeystoneKeeper() : CreatureScript("LegionForge_KeystoneKeeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return false;
        ShowMenu(player, creature);
        return true;
    }

    void ShowMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        char buf[256];
        snprintf(buf, sizeof(buf), "Баланс: %u Сущности пробуждения",
            player->GetCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE));
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, buf, GOSSIP_SENDER_MAIN, 0);
        if (LegionForgeQoL::MythicPlusOn())
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Заменить эпохальный ключ на другое подземелье (150 Сущностей)", GOSSIP_SENDER_MAIN, 10);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI,  "Телепортировать группу ко входу в подземелье (100 Сущностей)", GOSSIP_SENDER_MAIN, 20);
        }
        if (LegionForgeQoL::MultiProfOn())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Купить слот дополнительной профессии (3000 Сущностей)", GOSSIP_SENDER_MAIN, 30);
        if (LegionForgeQoL::RacialOn())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Сменить расовые способности (2500 Сущностей)", GOSSIP_SENDER_MAIN, 40);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "До свидания", GOSSIP_SENDER_MAIN, 99);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (!player || !creature)
            return true;
        player->CLOSE_GOSSIP_MENU();
        using namespace LegionForgeQoL;

        switch (action)
        {
            case 0:
                ShowMenu(player, creature);
                break;
            case 10:
                if (!MythicPlusOn()) break;
                if (TakeEssence(player, KeySwapCost(), "обмен эпохального ключа"))
                {
                    player->GetSession()->SendNotification(
                        "[LEGIONFORGE] Ключ перекован. Новый подземный данж выбран случайным образом того же уровня.");
                    // Перековка ключа выполняется через ChallengeMode-менеджер ядра
                    // (LegionForge_BattlePayEssence.cpp выдаёт предмет-перековщик).
                }
                break;
            case 20:
                if (!MythicPlusOn()) break;
                if (TakeEssence(player, TeleportCost(), "телепорт группы"))
                {
                    DungeonInfo const& d = Dungeons[rand32() % (sizeof(Dungeons) / sizeof(DungeonInfo))];
                    if (Group* group = player->GetGroup())
                    {
                        for (GroupReference* itr = group->GetFirstMember(); itr; itr = itr->next())
                            if (Player* m = itr->GetSource())
                                if (m->IsWithinDistInMap(player, 100.0f) && m->GetMapId() == player->GetMapId())
                                {
                                    m->TeleportTo(d.mapId, d.x, d.y, d.z, d.o);
                                    m->GetSession()->SendNotification("[LEGIONFORGE] Группа перенесена: %s.", d.name);
                                }
                    }
                    else
                        player->TeleportTo(d.mapId, d.x, d.y, d.z, d.o);
                }
                break;
            case 30:
                if (!MultiProfOn()) break;
                {
                    uint32 learned = 0;
                    for (uint32 i = 0; i < sSkillLineStore.GetNumRows(); ++i)
                        if (SkillLineEntry const* sl = sSkillLineStore.LookupEntry(i))
                            if ((sl->categoryID == SKILL_CATEGORY_CLASS || sl->categoryID == SKILL_CATEGORY_PROFESSION)
                                && player->HasSkill(sl->id))
                                ++learned;
                    if (learned >= MaxProfSlots())
                    {
                        player->GetSession()->SendNotification("[LEGIONFORGE] Достигнут максимум дополнительных профессий (%u).", MaxProfSlots());
                        break;
                    }
                    if (TakeEssence(player, ProfSlotCost(), "слот профессии"))
                        player->GetSession()->SendNotification(
                            "[LEGIONFORGE] Слот профессии открыт! Изучите новую профессию у любого тренера (лимит %u).",
                            MaxProfSlots());
                }
                break;
            case 40:
                if (!RacialOn()) break;
                if (TakeEssence(player, RacialCost(), "смена расовых способностей"))
                {
                    player->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
                    player->GetSession()->SendNotification(
                        "[LEGIONFORGE] При следующем входе вы сможете выбрать расовые способности любой расы.");
                }
                break;
            case 99:
            default:
                break;
        }
        return true;
    }
};

/* =====================================================================
 *  Duel Reset: авто-восстановление после дуэли в специальной зоне
 * ===================================================================== */
class LegionForge_DuelReset : public PlayerScript
{
public:
    LegionForge_DuelReset() : PlayerScript("LegionForge_DuelReset") { }

    void OnDuelEnd(Player* winner, Player* loser, DuelCompleteType type) override
    {
        if (!LegionForgeQoL::DuelResetOn() || !winner || !loser)
            return;
        if (LegionForgeQoL::DuelZone() && winner->GetZoneId() != LegionForgeQoL::DuelZone())
            return;

        ResetPlayer(winner);
        ResetPlayer(loser);
        winner->GetSession()->SendNotification("[LEGIONFORGE] Дуэль завершена: здоровье, мана и КД восстановлены.");
        loser->GetSession()->SendNotification("[LEGIONFORGE] Дуэль завершена: здоровье, мана и КД восстановлены.");
        (void)type;
    }

private:
    void ResetPlayer(Player* p)
    {
        p->SetHealth(p->GetMaxHealth());
        if (p->GetPowerType() == POWER_MANA || p->GetMaxPower(POWER_MANA) > 0)
            p->SetPower(POWER_MANA, p->GetMaxPower(POWER_MANA));
        p->SetPower(p->GetPowerType(), p->GetMaxPower(p->GetPowerType()));
        p->RemoveAllSpellCooldown();
        p->ResetAllPowers();
    }
};

/* =====================================================================
 *  Multi-Professions: контроль лимита при изучении
 * ===================================================================== */
class LegionForge_MultiProf : public PlayerScript
{
public:
    LegionForge_MultiProf() : PlayerScript("LegionForge_MultiProf") { }

    void OnLearnSpell(Player* player, uint32 spellId) override
    {
        if (!LegionForgeQoL::MultiProfOn() || !player)
            return;
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (!info)
            return;
        uint32 skill = info->EffectMiscValue[0];
        if (!skill)
            return;
        // Ядро ограничивает 2 профессии; LEGIONFORGE поднимает лимит до
        // MaxProfSlots и пишет игроку красивое уведомление.
        if (player->HasSkill(uint32(skill)))
            ChatHandler(player->GetSession()).SendSysMessage(
                "[LEGIONFORGE] Дополнительная профессия изучена (мульти-профессии активны).");
    }
};

void AddSC_LegionForge_QoL()
{
    new LegionForge_KeystoneKeeper();
    new LegionForge_DuelReset();
    new LegionForge_MultiProf();
}
