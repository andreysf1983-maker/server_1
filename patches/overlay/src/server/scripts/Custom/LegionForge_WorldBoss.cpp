// ============================================================================
//  LegionForge — 60 World Mini-Bosses AI, Dynamic Scaling & Kill Announcer
//  Target: LegionForgeCore 7.3.5 (26124) · Client Build 7.3.5.26124
// ============================================================================
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Chat.h"
#include "Creature.h"
#include "Group.h"
#include "Player.h"
#include "World.h"
#include "LegionForge_Config.h"

enum LegionForgeWorldBossSpells : uint32
{
    SPELL_BOSS_ENRAGE    = 26662,
    SPELL_BOSS_CLEAVE    = 15496,
    EVENT_BOSS_CLEAVE    = 1,
    EVENT_BOSS_ENRAGE    = 2
};

class npc_world_boss_generic : public CreatureScript
{
public:
    npc_world_boss_generic() : CreatureScript("npc_world_boss_generic") { }

    struct npc_world_boss_genericAI : public ScriptedAI
    {
        npc_world_boss_genericAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _enraged = false;
        }

        void EnterCombat(Unit* who) override
        {
            ScriptedAI::EnterCombat(who);
            _events.ScheduleEvent(EVENT_BOSS_CLEAVE, 8000);
            _events.ScheduleEvent(EVENT_BOSS_ENRAGE, 150000);
        }

        void JustDied(Unit* killer) override
        {
            ScriptedAI::JustDied(killer);

            if (!killer)
                return;

            Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (!player)
                return;

            uint32 essenceBonus = sConfigMgr->GetIntDefault("LegionForge.WorldBoss.KillEssenceReward", 150);
            uint32 currencyId = sConfigMgr->GetIntDefault("LegionForge.CurrencyId", LegionForge::CURRENCY_WAKENING_ESSENCE);

            if (Group* grp = player->GetGroup())
            {
                for (GroupReference* itr = grp->GetFirstMember(); itr != nullptr; itr = itr->next())
                {
                    if (Player* member = itr->getSource())
                    {
                        if (member->IsInMap(me) && member->GetDistance(me) <= 150.0f)
                        {
                            member->ModifyCurrency(currencyId, int32(essenceBonus));
                            ChatHandler(member->GetSession()).PSendSysMessage(
                                "|cff00FF88[Мировой босс]|r Побеждён |cffFFD800[%s]|r! Вы получили +%u Сущностей пробуждения.",
                                me->GetName().c_str(), essenceBonus);
                        }
                    }
                }
            }
            else
            {
                player->ModifyCurrency(currencyId, int32(essenceBonus));
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "|cff00FF88[Мировой босс]|r Побеждён |cffFFD800[%s]|r! Вы получили +%u Сущностей пробуждения.",
                    me->GetName().c_str(), essenceBonus);
            }

            if (sConfigMgr->GetBoolDefault("LegionForge.WorldBoss.AnnounceKill", true))
            {
                std::ostringstream ss;
                ss << "|cffFF8000[Мировые боссы Азерота]|r Герой |cff4CFF00" << player->GetName()
                   << "|r и его союзники сокрушили мини-босса |cffFFD800[" << me->GetName() << "]|r!";
                sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BOSS_CLEAVE:
                        DoCastVictim(SPELL_BOSS_CLEAVE);
                        _events.ScheduleEvent(EVENT_BOSS_CLEAVE, 10000);
                        break;
                    case EVENT_BOSS_ENRAGE:
                        if (!_enraged)
                        {
                            DoCast(me, SPELL_BOSS_ENRAGE);
                            _enraged = true;
                        }
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _enraged = false;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_world_boss_genericAI(creature);
    }
};

void AddSC_LegionForge_WorldBoss()
{
    new npc_world_boss_generic();
}
