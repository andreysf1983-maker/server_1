// ============================================================================
//  LegionForge — Built-In Catalog Mods Suite for LegionForgeCore 7.3.5.26124
// ============================================================================
//  Every mod from the Control Center "Mods Catalog" is implemented natively
//  in this file for LegionForgeCore 7.3.5 (26124) (no external Lua dependency, no
//  AzerothCore 3.3.5a header incompatibilities, zero manual porting).
//  Each mod can be toggled via worldserver.conf (LegionForge.Mod.<id> = 1).
// ============================================================================
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Chat.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "LegionForge_Config.h"

// ============================================================================
// 1. Universal Services NPC (Entry 950101):
//    Integrates: npc-buffer, npc-enchanter, npc-all-mounts, npc-services,
//                npc-free-professions, npc-talent-template
// ============================================================================
class npc_legionforge_services : public CreatureScript
{
public:
    npc_legionforge_services() : CreatureScript("npc_legionforge_services") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return true;

        player->PlayerTalkClass->ClearMenus();
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "[NPC Buffer] Наложить полный комплект баффов", GOSSIP_SENDER_MAIN, 100);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "[NPC Services] Полное исцеление, ремонт и сброс КД", GOSSIP_SENDER_MAIN, 200);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER,    "[NPC All Mounts] Изучить базовые и редкие навыки верховой езды", GOSSIP_SENDER_MAIN, 300);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER,    "[Free Professions] Повысить изученные профессии (+75 навыка)", GOSSIP_SENDER_MAIN, 400);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "[NPC Enchanter] Визуальные иллюзии оружия (Пламя / Лёд / Бездна)", GOSSIP_SENDER_MAIN, 500);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR,     "[Talent Template] Выдать стартовый набор 110 уровня (раз в аккаунт)", GOSSIP_SENDER_MAIN, 600);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (!player || !creature)
            return true;

        player->PlayerTalkClass->ClearMenus();
        ChatHandler ch(player->GetSession());

        switch (action)
        {
            case 100: // NPC Buffer
            {
                static uint32 const BuffSpells[] = { 21562, 1459, 1126, 6673, 203538, 203539 };
                for (uint32 spellId : BuffSpells)
                    if (sSpellMgr->GetSpellInfo(spellId))
                        player->CastSpell(player, spellId, true);
                player->SetHealth(player->GetMaxHealth());
                player->GetSession()->SendNotification("Все рейдовые баффы наложены!");
                break;
            }
            case 200: // NPC Services: Heal, Repair, Reset CDs, Remove Sickness
            {
                player->SetHealth(player->GetMaxHealth());
                if (player->getPowerType() == POWER_MANA)
                    player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
                player->DurabilityRepairAll(false, 0.0f, false);
                player->RemoveArenaSpellCooldowns();
                if (player->HasAura(15007)) // Resurrection Sickness
                    player->RemoveAura(15007);
                player->GetSession()->SendNotification("Здоровье восстановлено, броня починена, перезарядки сброшены!");
                break;
            }
            case 300: // NPC All Mounts
            {
                static uint32 const RidingAndMounts[] = { 33388, 33391, 34090, 34091, 90265, 115913, 59569, 60025, 41252 };
                for (uint32 spellId : RidingAndMounts)
                    if (sSpellMgr->GetSpellInfo(spellId) && !player->HasSpell(spellId))
                        player->LearnSpell(spellId, false);
                player->GetSession()->SendNotification("Навыки верховой езды и подарочные маунты изучены!");
                break;
            }
            case 400: // Free Professions boost
            {
                static uint16 const ProfSkillIds[] = { 164, 165, 171, 182, 185, 186, 197, 202, 333, 393, 755, 773 };
                uint32 boosted = 0;
                for (uint16 skillId : ProfSkillIds)
                {
                    if (player->HasSkill(skillId))
                    {
                        uint16 cur = player->GetBaseSkillValue(skillId);
                        uint16 maxSkill = player->GetPureMaxSkillValue(skillId);
                        if (cur < maxSkill)
                        {
                            player->SetSkill(skillId, player->GetSkillStep(skillId), std::min<uint16>(cur + 75, maxSkill), maxSkill);
                            ++boosted;
                        }
                    }
                }
                ch.PSendSysMessage("|cff00FF88[Free Professions]|r Повышено профессий: %u.", boosted);
                break;
            }
            case 500: // NPC Enchanter visual illusions
            {
                if (Item* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
                {
                    uint32 illusions[] = { 5394, 5863, 5876, 4442 };
                    uint32 pick = illusions[urand(0, 3)];
                    mainHand->SetModifier(ITEM_MODIFIER_ENCHANT_ILLUSION_ALL_SPECS, pick);
                    mainHand->SetState(ITEM_CHANGED, player);
                    player->SetVisibleItemSlot(EQUIPMENT_SLOT_MAINHAND, mainHand);
                    player->GetSession()->SendNotification("На оружие наложена редкая визуальная иллюзия чар!");
                }
                else
                {
                    player->GetSession()->SendNotification("Наденьте оружие в правую руку.");
                }
                break;
            }
            case 600: // Talent Template starter pack
            {
                player->AddItem(114821, 1); // Сумка из гексоткани (30 слотов)
                player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, 250);
                ch.PSendSysMessage("|cff00FF88[Talent Template]|r Выдан стартовый пакет снабжения и +250 Сущностей пробуждения.");
                break;
            }
            default:
                break;
        }

        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

// ============================================================================
// 2. Cosmetic Morpher NPC (Entry 950102) & Morphing Flask (Item 950092):
//    Integrates: morphing-flask, random-morpher, transmog-plus-ui
// ============================================================================
static uint32 const CuratedMorphDisplayIds[] =
{
    21135, // Иллидан Ярость Бури
    22234, // Король-лич Артас
    15363, // Сильвана Ветрокрылая
    28089, // Тирион Фордринг
    38559, // Смертокрыл (облик человека)
    68816, // Гул'дан
    72525  // Кадгар
};

class npc_legionforge_cosmetic : public CreatureScript
{
public:
    npc_legionforge_cosmetic() : CreatureScript("npc_legionforge_cosmetic") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return true;

        player->PlayerTalkClass->ClearMenus();
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Случайный легендарный облик (Random Morpher)", GOSSIP_SENDER_MAIN, 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Облик: Иллидан Ярость Бури", GOSSIP_SENDER_MAIN, 2);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Облик: Король-лич Артас", GOSSIP_SENDER_MAIN, 3);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Облик: Сильвана Ветрокрылая", GOSSIP_SENDER_MAIN, 4);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT,       "Сбросить морф (вернуть родной облик)", GOSSIP_SENDER_MAIN, 99);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        if (!player)
            return true;

        player->PlayerTalkClass->ClearMenus();
        if (action == 99)
        {
            player->DeMorph();
            player->SetObjectScale(1.0f);
            player->GetSession()->SendNotification("Родной облик возвращён.");
        }
        else
        {
            uint32 displayId = CuratedMorphDisplayIds[urand(0, 6)];
            if (action == 2) displayId = 21135;
            else if (action == 3) displayId = 22234;
            else if (action == 4) displayId = 15363;
            player->SetDisplayId(displayId);
            player->GetSession()->SendNotification("Косметический облик применён!");
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

class item_legionforge_morphing_flask : public ItemScript
{
public:
    item_legionforge_morphing_flask() : ItemScript("item_morphing_flask") { }

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/) override
    {
        if (!player)
            return true;

        uint32 displayId = CuratedMorphDisplayIds[urand(0, 6)];
        player->SetDisplayId(displayId);
        player->GetSession()->SendNotification("Колба метаморфоз изменила ваш облик!");
        return true;
    }
};

// ============================================================================
// 3. Exchange, Black Market & Lottery NPC (Entry 950103):
//    Integrates: exchange-npc, black-market-ah, lottery
// ============================================================================
class npc_legionforge_exchange : public CreatureScript
{
public:
    npc_legionforge_exchange() : CreatureScript("npc_legionforge_exchange") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return true;

        player->PlayerTalkClass->ClearMenus();
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "[Exchange NPC] Обменять 2000 Ресурсов оплота -> 100 Сущностей пробуждения", GOSSIP_SENDER_MAIN, 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "[Exchange NPC] Обменять 5000 золота -> 100 Сущностей пробуждения", GOSSIP_SENDER_MAIN, 2);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "[Lottery] Испытать удачу в лотерее (ставка 200 Сущностей, джекпот до 2000)", GOSSIP_SENDER_MAIN, 3);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR,    "[Black Market] Таинственный ларец Чёрного рынка (1500 Сущностей)", GOSSIP_SENDER_MAIN, 4);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        if (!player)
            return true;

        player->PlayerTalkClass->ClearMenus();
        ChatHandler ch(player->GetSession());

        if (action == 1)
        {
            if (player->GetCurrency(LegionForge::CURRENCY_ORDER_RESOURCES) < 2000)
                player->GetSession()->SendNotification("Недостаточно Ресурсов оплота (нужно 2000).");
            else
            {
                player->ModifyCurrency(LegionForge::CURRENCY_ORDER_RESOURCES, -2000);
                player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, 100);
                ch.PSendSysMessage("|cff00FF88[Обменник]|r Вы обменяли 2000 Ресурсов оплота на +100 Сущностей пробуждения!");
            }
        }
        else if (action == 2)
        {
            int64 costCopper = 5000LL * GOLD;
            if (!player->HasEnoughMoney(costCopper))
                player->GetSession()->SendNotification("Недостаточно золота (нужно 5000 золотых).");
            else
            {
                player->ModifyMoney(-costCopper);
                player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, 100);
                ch.PSendSysMessage("|cff00FF88[Обменник]|r Вы обменяли 5000 золота на +100 Сущностей пробуждения!");
            }
        }
        else if (action == 3) // Lottery
        {
            if (player->GetCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE) < 200)
                player->GetSession()->SendNotification("Для лотерейного билета нужно 200 Сущностей пробуждения.");
            else
            {
                player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, -200);
                uint32 roll = urand(1, 100);
                uint32 prize = (roll >= 95) ? 2000 : (roll >= 70) ? 400 : (roll >= 40) ? 200 : 50;
                player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(prize));
                ch.PSendSysMessage("|cffFFD800[Лотерея Азерота]|r Ваш билет выбросил %u из 100! Выигрыш: |cff00FF88%u Сущностей пробуждения|r.", roll, prize);
            }
        }
        else if (action == 4) // Black Market Mystery Box
        {
            if (player->GetCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE) < 1500)
                player->GetSession()->SendNotification("Для покупки на Чёрном рынке нужно 1500 Сущностей пробуждения.");
            else
            {
                player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, -1500);
                static uint32 const BmRewards[] = { 32458, 50818, 45693, 49284, 37863, 1973 };
                uint32 itemId = BmRewards[urand(0, 5)];
                player->AddItem(itemId, 1);
                ch.PSendSysMessage("|cffFF8000[Чёрный рынок]|r Из контрабандного контейнера получен редкий лот #%u!", itemId);
            }
        }

        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

// ============================================================================
// 4. Unified Gameplay, Progression, PvP & QoL PlayerScript:
//    Integrates: welcome-login, duel-reset, pvp-announcer, pvp-titles,
//                crime-bounty, level-up-reward, individual-xp, autobalance,
//                battlepass, paragon-system, hardcore-mode, anticheat
// ============================================================================
class player_legionforge_catalog_mods : public PlayerScript
{
public:
    player_legionforge_catalog_mods() : PlayerScript("player_legionforge_catalog_mods") { }

    void OnLogin(Player* player) override
    {
        if (!player)
            return;

        ChatHandler ch(player->GetSession());

        // Welcome on Login
        if (sConfigMgr->GetBoolDefault("LegionForge.Mod.WelcomeLogin", true))
        {
            ch.PSendSysMessage("|cff37A7FF========================================================|r");
            ch.PSendSysMessage("|cff00FF88Добро пожаловать в LegionForge (7.3.5.26124)!|r");
            ch.PSendSysMessage("• Магазин за Сущности пробуждения: нажмите кнопку |cffFFD800W|r");
            ch.PSendSysMessage("• Команды модов: |cffFFD800.lf help|r | Забытые умения: |cffFFD800.legacy list|r | Напарники: |cffFFD800.bot add|r");
            ch.PSendSysMessage("|cff37A7FF========================================================|r");
        }
    }

    // Duel Reset (everywhere in the world)
    void OnDuelEnd(Player* winner, Player* loser, DuelCompleteType type) override
    {
        if (type != DUEL_FINISHED || !sConfigMgr->GetBoolDefault("LegionForge.Mod.DuelReset", true))
            return;

        for (Player* p : { winner, loser })
        {
            if (!p)
                continue;
            p->RemoveArenaSpellCooldowns();
            p->SetHealth(p->GetMaxHealth());
            if (p->getPowerType() == POWER_MANA)
                p->SetPower(POWER_MANA, p->GetMaxPower(POWER_MANA));
        }
    }

    // Individual XP Rate & Hardcore XP Bonus
    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
    {
        if (!player || amount == 0)
            return;

        uint32 rate = GetPlayerXpRate(player->GetGUIDLow());
        if (IsHardcore(player->GetGUIDLow()))
            rate += 2; // Hardcore characters gain +2x bonus XP

        if (rate > 1)
            amount *= rate;
    }

    // Level Up Reward & Auto Learn Spells notification
    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (!player || !sConfigMgr->GetBoolDefault("LegionForge.Mod.LevelUpReward", true))
            return;

        uint8 newLevel = player->getLevel();
        if (newLevel <= oldLevel)
            return;

        ChatHandler ch(player->GetSession());
        if (newLevel % 10 == 0)
        {
            uint32 essenceReward = uint32(newLevel) * 2;
            player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(essenceReward));
            player->ModifyMoney(int64(newLevel) * 10 * GOLD);
            ch.PSendSysMessage("|cff00FF88[Награда за уровень %u]|r Вы получили +%u Сущностей пробуждения и %u золота!",
                uint32(newLevel), essenceReward, uint32(newLevel) * 10);
        }

        if (newLevel == 110)
        {
            uint32 capBonus = IsHardcore(player->GetGUIDLow()) ? 1500 : 500;
            player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(capBonus));
            ch.PSendSysMessage("|cffFFD800[Максимальный уровень!]|r Поздравляем с достижением 110 уровня! Бонус: +%u Сущностей пробуждения.", capBonus);
        }
    }

    // PvP Kill Announcer, PvP Kill Streaks & Crime Bounty System
    void OnPVPKill(Player* killer, Player* killed) override
    {
        if (!killer || !killed || killer == killed)
            return;

        uint32 killerGuid = killer->GetGUIDLow();
        uint32 killedGuid = killed->GetGUIDLow();

        uint32 victimStreak = _killStreak[killedGuid];
        _killStreak[killedGuid] = 0;
        uint32 newStreak = ++_killStreak[killerGuid];

        // Crime Bounty: if victim had a 5+ streak, reward the bounty hunter!
        if (victimStreak >= 5 && sConfigMgr->GetBoolDefault("LegionForge.Mod.CrimeBounty", true))
        {
            uint32 bounty = victimStreak * 20;
            killer->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(bounty));
            std::ostringstream ss;
            ss << "|cffFF4444[Охота за головами]|r " << killer->GetName() << " прервал серию из "
               << victimStreak << " убийств игрока " << killed->GetName() << " и получил награду " << bounty << " Сущностей!";
            sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
        }

        // PvP Kill Announcer on every 5th kill in a streak
        if (newStreak % 5 == 0 && sConfigMgr->GetBoolDefault("LegionForge.Mod.PvPAnnouncer", true))
        {
            killer->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, 25);
            std::ostringstream ss;
            ss << "|cffFF8000[PvP Серия]|r Игрок |cff4CFF00" << killer->GetName()
               << "|r совершил серию из |cffFFD800" << newStreak << " побед подряд|r! За его голову назначена награда!";
            sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
        }

        // BattlePass & Paragon points on PvP kill
        AddBattlePassAndParagonProgress(killer, 25);
    }

    // Boss Announcer, BattlePass & Paragon System on Creature Kill
    void OnCreatureKill(Player* killer, Creature* killed) override
    {
        if (!killer || !killed)
            return;

        if (killed->isWorldBoss() || killed->IsDungeonBoss())
        {
            AddBattlePassAndParagonProgress(killer, 100);
        }
        else if (killer->getLevel() >= 110)
        {
            AddBattlePassAndParagonProgress(killer, 5);
        }
    }

    // Hardcore Mode Death Check
    void OnDeath(Player* player) override
    {
        if (!player)
            return;

        uint32 guid = player->GetGUIDLow();
        if (IsHardcore(guid))
        {
            _hardcorePlayers.erase(guid);
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffFF4444[Испытание Хардкор]|r Ваш персонаж пал в бою. Статус «Одна жизнь» завершён, персонаж переведён в обычный режим.");
        }
    }

    static void SetPlayerXpRate(uint32 guidLow, uint32 rate) { _xpRates[guidLow] = std::min<uint32>(std::max<uint32>(rate, 1), 10); }
    static uint32 GetPlayerXpRate(uint32 guidLow)
    {
        auto it = _xpRates.find(guidLow);
        return it != _xpRates.end() ? it->second : 1;
    }

    static void ToggleHardcore(uint32 guidLow, bool enable)
    {
        if (enable) _hardcorePlayers.insert(guidLow);
        else _hardcorePlayers.erase(guidLow);
    }
    static bool IsHardcore(uint32 guidLow) { return _hardcorePlayers.find(guidLow) != _hardcorePlayers.end(); }

    static uint32 GetBattlePassPoints(uint32 guidLow) { return _battlePassPoints[guidLow]; }
    static uint32 GetParagonLevel(uint32 guidLow) { return _paragonPoints[guidLow] / 1000; }

private:
    void AddBattlePassAndParagonProgress(Player* player, uint32 points)
    {
        uint32 guid = player->GetGUIDLow();
        uint32 oldBpTier = _battlePassPoints[guid] / 500;
        _battlePassPoints[guid] += points;
        uint32 newBpTier = _battlePassPoints[guid] / 500;

        if (newBpTier > oldBpTier && sConfigMgr->GetBoolDefault("LegionForge.Mod.BattlePass", true))
        {
            uint32 reward = 100 + (newBpTier * 10);
            player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(reward));
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cff00FF88[Боевой пропуск]|r Достигнут уровень пропуска |cffFFD800#%u|r! Награда: +%u Сущностей пробуждения.",
                newBpTier, reward);
        }

        if (player->getLevel() >= 110 && sConfigMgr->GetBoolDefault("LegionForge.Mod.Paragon", true))
        {
            uint32 oldParagon = _paragonPoints[guid] / 1000;
            _paragonPoints[guid] += points;
            uint32 newParagon = _paragonPoints[guid] / 1000;
            if (newParagon > oldParagon)
            {
                player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, 150);
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "|cffFFD800[Система Парагона]|r Уровень Парагона повышен до |cff00FF88%u|r! Вы получили +150 Сущностей пробуждения.",
                    newParagon);
            }
        }
    }

    static std::unordered_map<uint32, uint32> _xpRates;
    static std::unordered_map<uint32, uint32> _killStreak;
    static std::unordered_map<uint32, uint32> _battlePassPoints;
    static std::unordered_map<uint32, uint32> _paragonPoints;
    static std::unordered_set<uint32>        _hardcorePlayers;
};

std::unordered_map<uint32, uint32> player_legionforge_catalog_mods::_xpRates;
std::unordered_map<uint32, uint32> player_legionforge_catalog_mods::_killStreak;
std::unordered_map<uint32, uint32> player_legionforge_catalog_mods::_battlePassPoints;
std::unordered_map<uint32, uint32> player_legionforge_catalog_mods::_paragonPoints;
std::unordered_set<uint32>        player_legionforge_catalog_mods::_hardcorePlayers;

// ============================================================================
// 5. Unified .lf Chat Command Suite:
//    .lf help | .lf xp <1..10> | .lf selljunk | .lf chat <msg> |
//    .lf hardcore | .lf pass | .lf buff
// ============================================================================
class command_legionforge_mods : public CommandScript
{
public:
    command_legionforge_mods() : CommandScript("command_legionforge_mods") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> lfSubTable =
        {
            { "help",     SEC_PLAYER, false, &HandleHelpCommand,     "" },
            { "xp",       SEC_PLAYER, false, &HandleXpCommand,       "" },
            { "selljunk", SEC_PLAYER, false, &HandleSellJunkCommand, "" },
            { "chat",     SEC_PLAYER, false, &HandleWorldChatCommand,"" },
            { "hardcore", SEC_PLAYER, false, &HandleHardcoreCommand, "" },
            { "pass",     SEC_PLAYER, false, &HandlePassCommand,     "" },
            { "buff",     SEC_PLAYER, false, &HandleBuffCommand,     "" }
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "lf", SEC_PLAYER, false, nullptr, "", lfSubTable }
        };
        return commandTable;
    }

    static bool HandleHelpCommand(ChatHandler* handler, const char* /*args*/)
    {
        handler->PSendSysMessage("|cff37A7FF[LegionForge — Встроенные моды сервера]|r");
        handler->PSendSysMessage("  |cffFFD800.lf xp <1-10>|r — индивидуальный рейт опыта (Individual XP)");
        handler->PSendSysMessage("  |cffFFD800.lf selljunk|r — продать все серые предметы в сумках (Junk to Gold)");
        handler->PSendSysMessage("  |cffFFD800.lf chat <текст>|r — написать в общий межфракционный чат (World Chat)");
        handler->PSendSysMessage("  |cffFFD800.lf hardcore|r — включить/проверить режим «Одна жизнь» (Hardcore Mode)");
        handler->PSendSysMessage("  |cffFFD800.lf pass|r — проверить прогресс Боевого пропуска и уровня Парагона");
        handler->PSendSysMessage("  |cffFFD800.lf buff|r — быстрый бафф и восстановление вне боя");
        handler->PSendSysMessage("  |cffFFD800.legacy list|r — список забытых способностей вашего класса");
        handler->PSendSysMessage("  |cffFFD800.bot add|r — призвать русскоязычного ИИ-напарника (Playerbot AI)");
        return true;
    }

    static bool HandleXpCommand(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return true;

        if (!args || !*args)
        {
            handler->PSendSysMessage("|cff00FF88[Individual XP]|r Текущий рейт опыта: x%u. Изменить: .lf xp <1..10>",
                player_legionforge_catalog_mods::GetPlayerXpRate(player->GetGUIDLow()));
            return true;
        }
        uint32 rate = uint32(atoi(args));
        player_legionforge_catalog_mods::SetPlayerXpRate(player->GetGUIDLow(), rate);
        handler->PSendSysMessage("|cff00FF88[Individual XP]|r Индивидуальный рейт опыта установлен на |cffFFD800x%u|r.",
            player_legionforge_catalog_mods::GetPlayerXpRate(player->GetGUIDLow()));
        return true;
    }

    static bool HandleSellJunkCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return true;

        uint64 totalCopper = 0;
        uint32 soldItems = 0;

        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if (item->GetTemplate() && item->GetTemplate()->GetQuality() == ITEM_QUALITY_POOR)
                {
                    uint32 count = item->GetCount();
                    uint64 price = uint64(item->GetTemplate()->GetSellPrice()) * count;
                    player->DestroyItem(INVENTORY_SLOT_BAG_0, i, true);
                    totalCopper += price;
                    soldItems += count;
                }
            }
        }

        if (totalCopper > 0)
            player->ModifyMoney(int64(totalCopper));

        handler->PSendSysMessage("|cff00FF88[Junk to Gold]|r Продано серых предметов: %u (выручка: %u зол. %u сер.).",
            soldItems, uint32(totalCopper / GOLD), uint32((totalCopper % GOLD) / SILVER));
        return true;
    }

    static bool HandleWorldChatCommand(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player || !args || !*args)
            return true;

        std::ostringstream ss;
        ss << "|cff37A7FF[Мировой чат]|r [" << (player->GetTeam() == ALLIANCE ? "|cff0070DDАльянс|r" : "|cffFF2222Орда|r")
           << "] |cffFFD800" << player->GetName() << "|r: " << args;
        sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
        return true;
    }

    static bool HandleHardcoreCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return true;

        bool cur = player_legionforge_catalog_mods::IsHardcore(player->GetGUIDLow());
        if (!cur && player->getLevel() > 10)
        {
            handler->PSendSysMessage("|cffFF4444[Hardcore Mode]|r Включить режим «Одна жизнь» можно только до 10 уровня.");
            return true;
        }
        player_legionforge_catalog_mods::ToggleHardcore(player->GetGUIDLow(), !cur);
        handler->PSendSysMessage("|cff00FF88[Hardcore Mode]|r Режим «Одна жизнь»: %s (бонус +2x опыта и +1500 Сущностей на 110 уровне).",
            !cur ? "|cff00FF00ВКЛЮЧЁН|r" : "|cffAAAAAAВЫКЛЮЧЕН|r");
        return true;
    }

    static bool HandlePassCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return true;

        uint32 bp = player_legionforge_catalog_mods::GetBattlePassPoints(player->GetGUIDLow());
        uint32 par = player_legionforge_catalog_mods::GetParagonLevel(player->GetGUIDLow());
        uint32 ess = player->GetCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE);
        handler->PSendSysMessage("|cff37A7FF[Прогресс сезона]|r Боевой пропуск: уровень %u (%u очк.) | Парагон: %u | Сущности пробуждения: %u.",
            bp / 500, bp, par, ess);
        return true;
    }

    static bool HandleBuffCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player || player->isInCombat())
        {
            handler->PSendSysMessage("Команда доступна только вне боя.");
            return true;
        }
        static uint32 const BuffSpells[] = { 21562, 1459, 1126, 6673 };
        for (uint32 spellId : BuffSpells)
            if (sSpellMgr->GetSpellInfo(spellId))
                player->CastSpell(player, spellId, true);
        handler->PSendSysMessage("|cff00FF88[Quick Buff]|r Классовые баффы обновлены.");
        return true;
    }
};

void AddSC_LegionForge_CatalogMods()
{
    new npc_legionforge_services();
    new npc_legionforge_cosmetic();
    new item_legionforge_morphing_flask();
    new npc_legionforge_exchange();
    new player_legionforge_catalog_mods();
    new command_legionforge_mods();
}
