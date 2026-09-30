// ============================================================================
//  LegionForge — Native Playerbot & AI Companion System (7.3.5.26124)
//  Integrates catalog mods: playerbots, npcbot-extended, trinity-bots
// ============================================================================
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Chat.h"
#include "Creature.h"
#include "Map.h"
#include "Player.h"
#include "LegionForge_Config.h"

enum BotSpellsAndEvents : uint32
{
    SPELL_BOT_STUN          = 853,    // Молот правосудия (контроль/стан)
    SPELL_BOT_ROOT          = 339,    // Гнев деревьев (корни)
    SPELL_BOT_HEAL          = 19750,  // Вспышка Света (лечение лидера)
    SPELL_BOT_STRIKE        = 12294,  // Смертельный удар

    EVENT_BOT_CC            = 1,
    EVENT_BOT_HEAL          = 2,
    EVENT_BOT_STRIKE        = 3,
    EVENT_BOT_CHAT          = 4
};

class npc_legionforge_bot : public CreatureScript
{
public:
    npc_legionforge_bot() : CreatureScript("npc_legionforge_bot") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return true;

        player->PlayerTalkClass->ClearMenus();
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Следовать за мной и ассистировать в бою", GOSSIP_SENDER_MAIN, 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Исцелить меня и обновить строй", GOSSIP_SENDER_MAIN, 2);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Отпустить напарника", GOSSIP_SENDER_MAIN, 3);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (!player || !creature)
            return true;

        player->PlayerTalkClass->ClearMenus();
        if (action == 1)
        {
            creature->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            creature->MonsterWhisper("Принято! Держусь рядом и бью по вашей цели.", player->GetGUID());
        }
        else if (action == 2)
        {
            creature->CastSpell(player, SPELL_BOT_HEAL, true);
            player->SetHealth(player->GetMaxHealth());
            creature->MonsterWhisper("Здоровье восстановлено, командир!", player->GetGUID());
        }
        else if (action == 3)
        {
            creature->MonsterWhisper("До встречи на полях сражений Азерота!", player->GetGUID());
            creature->DespawnOrUnsummon(500);
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    struct npc_legionforge_botAI : public ScriptedAI
    {
        npc_legionforge_botAI(Creature* creature) : ScriptedAI(creature)
        {
            _botName = LegionForge::RussianBotNames[urand(0, LegionForge::RussianBotNamesCount - 1)];
        }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_BOT_CC, urand(8000, 14000));
            _events.ScheduleEvent(EVENT_BOT_HEAL, 6000);
            _events.ScheduleEvent(EVENT_BOT_STRIKE, 5000);
            _events.ScheduleEvent(EVENT_BOT_CHAT, urand(120000, 240000));
        }

        void IsSummonedBy(Unit* summoner) override
        {
            if (!summoner)
                return;

            me->SetLevel(summoner->getLevel());
            me->SetMaxHealth(summoner->GetMaxHealth());
            me->SetHealth(me->GetMaxHealth());
            me->setFaction(summoner->getFaction());
            me->GetMotionMaster()->MoveFollow(summoner, PET_FOLLOW_DIST, frand(0.5f, 2.5f));

            if (Player* owner = summoner->ToPlayer())
            {
                std::ostringstream greet;
                greet << "|cff00CCFF[ИИ-напарник " << _botName << "]|r Вступил в группу! Автоассист и контроль активны.";
                ChatHandler(owner->GetSession()).PSendSysMessage("%s", greet.str().c_str());
            }
        }

        void UpdateAI(uint32 diff) override
        {
            Unit* owner = me->GetCharmerOrOwner();
            if (owner && !me->isInCombat())
            {
                if (Unit* ownerVictim = owner->getVictim())
                    if (me->CanCreatureAttack(ownerVictim))
                        AttackStart(ownerVictim);
            }

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BOT_CHAT:
                    {
                        if (owner && owner->ToPlayer())
                        {
                            char const* phrase = LegionForge::RussianBotPhrases[urand(0, LegionForge::RussianBotPhrasesCount - 1)];
                            ChatHandler(owner->ToPlayer()->GetSession()).PSendSysMessage(
                                "|cff00CCFF[Группа] [%s]:|r %s", _botName.c_str(), phrase);
                        }
                        _events.ScheduleEvent(EVENT_BOT_CHAT, urand(120000, 240000));
                        break;
                    }
                    case EVENT_BOT_HEAL:
                    {
                        if (owner && owner->HealthBelowPct(65))
                            DoCast(owner, SPELL_BOT_HEAL);
                        else if (me->HealthBelowPct(50))
                            DoCast(me, SPELL_BOT_HEAL);
                        _events.ScheduleEvent(EVENT_BOT_HEAL, 7000);
                        break;
                    }
                    case EVENT_BOT_CC:
                    {
                        if (Unit* victim = me->getVictim())
                            DoCast(victim, urand(0, 1) ? SPELL_BOT_STUN : SPELL_BOT_ROOT);
                        _events.ScheduleEvent(EVENT_BOT_CC, urand(14000, 22000));
                        break;
                    }
                    case EVENT_BOT_STRIKE:
                    {
                        if (Unit* victim = me->getVictim())
                            DoCast(victim, SPELL_BOT_STRIKE);
                        _events.ScheduleEvent(EVENT_BOT_STRIKE, 6000);
                        break;
                    }
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        std::string _botName;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_legionforge_botAI(creature);
    }
};

class command_legionforge_bot : public CommandScript
{
public:
    command_legionforge_bot() : CommandScript("command_legionforge_bot") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> botSubTable =
        {
            { "add",    SEC_PLAYER, false, &HandleAddBotCommand,    "" },
            { "dismiss",SEC_PLAYER, false, &HandleDismissBotCommand,"" },
            { "names",  SEC_PLAYER, false, &HandleNamesCommand,     "" }
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "bot", SEC_PLAYER, false, nullptr, "", botSubTable }
        };
        return commandTable;
    }

    static bool HandleAddBotCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return true;

        if (!sConfigMgr->GetBoolDefault("LegionForge.Playerbots.Enable", true))
        {
            handler->PSendSysMessage("|cffFF4444[LegionForge]|r Модуль ИИ-напарников отключён в конфигурации.");
            return true;
        }

        if (TempSummon* bot = player->SummonCreature(
            LegionForge::NPC_COMPANION_BOT,
            player->GetPositionX() + frand(-2.5f, 2.5f),
            player->GetPositionY() + frand(-2.5f, 2.5f),
            player->GetPositionZ(),
            player->GetOrientation(),
            TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
            3600 * IN_MILLISECONDS))
        {
            bot->SetCreatorGUID(player->GetGUID());
            bot->SetOwnerGUID(player->GetGUID());
        }
        return true;
    }

    static bool HandleDismissBotCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return true;

        std::list<Creature*> bots;
        player->GetCreatureListWithEntryInGrid(bots, LegionForge::NPC_COMPANION_BOT, 100.0f);
        uint32 count = 0;
        for (Creature* c : bots)
        {
            if (c && c->GetOwnerGUID() == player->GetGUID())
            {
                c->DespawnOrUnsummon();
                ++count;
            }
        }
        handler->PSendSysMessage("|cff00CCFF[LegionForge]|r Отпущено ИИ-напарников: %u.", count);
        return true;
    }

    static bool HandleNamesCommand(ChatHandler* handler, const char* /*args*/)
    {
        handler->PSendSysMessage("|cff00CCFF[LegionForge]|r В пуле Playerbot AI загружено %u русских имён и %u боевых фраз.",
            LegionForge::RussianBotNamesCount, LegionForge::RussianBotPhrasesCount);
        return true;
    }
};

class player_legionforge_bg_bot_filler : public PlayerScript
{
public:
    player_legionforge_bg_bot_filler() : PlayerScript("player_legionforge_bg_bot_filler") { }

    void OnMapChanged(Player* player) override
    {
        if (!player || !sConfigMgr->GetBoolDefault("LegionForge.Playerbots.FillPvPSlots", true))
            return;

        Map* map = player->GetMap();
        if (!map || (!map->IsBattleground() && !map->IsBattleArena()))
            return;

        // Spawn 2 AI combat companions for the player inside under-populated BGs/Arenas
        for (uint8 i = 0; i < 2; ++i)
        {
            if (TempSummon* bot = player->SummonCreature(
                LegionForge::NPC_COMPANION_BOT,
                player->GetPositionX() + frand(-3.0f, 3.0f),
                player->GetPositionY() + frand(-3.0f, 3.0f),
                player->GetPositionZ(),
                player->GetOrientation(),
                TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                1800 * IN_MILLISECONDS))
            {
                bot->SetCreatorGUID(player->GetGUID());
                bot->SetOwnerGUID(player->GetGUID());
            }
        }
    }
};

void AddSC_LegionForge_PlayerBots()
{
    new npc_legionforge_bot();
    new command_legionforge_bot();
    new player_legionforge_bg_bot_filler();
}
