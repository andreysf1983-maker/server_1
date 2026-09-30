/*
 * LEGIONFORGE — Custom World Bosses (100 уникальных мировых мини-боссов)
 * ---------------------------------------------------------------------
 * Полностью data-driven: все 100 боссов описаны в таблице
 * `custom_legionforge_worldboss` (sql/custom/custom_legionforge.sql),
 * а один универсальный C++-скрипт выдаёт им тактики:
 *   - фазы по здоровью (Phase1Pct / Phase2Pct);
 *   - «лужи» (AOE) по случайной цели;
 *   - призыв слуг с интервалом;
 *   - энрейдж через 8 минут боя;
 *   - анонс вступления в бой и убийства на весь сервер;
 *   - награда 250..500 Сущности пробуждения каждому участнику группы;
 *   - динамический масштаб HP/урона под количество игроков (2..10).
 *
 * Конфиг: LegionForge.WorldBoss.Enable           = 1
 *         LegionForge.WorldBoss.AnnounceSpawn    = 1
 *         LegionForge.WorldBoss.AnnounceKill     = 1
 *         LegionForge.WorldBoss.PeriodicAnnounce = 1
 *         LegionForge.WorldBoss.RespawnMinutes   = 180
 *         LegionForge.WorldBoss.ScalePerPlayer   = 0.18
 */
#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "TemporarySummon.h"
#include "Log.h"
#include "LegionForge_Config.h"

namespace LegionForgeWorldBoss
{
    struct BossData
    {
        uint32 Entry = 0;
        std::string NameRu;
        uint8  Phase1Pct = 70;
        uint8  Phase2Pct = 35;
        uint32 SpellMain = 0;
        uint32 SpellAoe = 0;
        uint32 SpellSummon = 0;
        uint32 SummonEntry = 0;
        uint32 SpellEnrage = 0;
        uint32 EssenceMin = 250;
        uint32 EssenceMax = 500;
    };

    std::unordered_map<uint32, BossData> Bosses;

    inline bool Enabled() { return sConfigMgr->GetBoolDefault("LegionForge.WorldBoss.Enable", true); }
    inline bool AnnounceSpawn() { return sConfigMgr->GetBoolDefault("LegionForge.WorldBoss.AnnounceSpawn", true); }
    inline bool AnnounceKill()  { return sConfigMgr->GetBoolDefault("LegionForge.WorldBoss.AnnounceKill", true); }
    inline uint32 RespawnMinutes() { return uint32(sConfigMgr->GetIntDefault("LegionForge.WorldBoss.RespawnMinutes", 180)); }
    inline float ScalePerPlayer() { return sConfigMgr->GetFloatDefault("LegionForge.WorldBoss.ScalePerPlayer", 0.18f); }

    inline void Announce(char const* fmt, ...)
    {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        sWorld->SendServerMessage(SERVER_MSG_STRING, buf);
    }

    inline void LoadBosses()
    {
        Bosses.clear();
        if (QueryResult result = WorldDatabase.Query(
            "SELECT Entry, NameRu, Phase1Pct, Phase2Pct, SpellMain, SpellAoe, SpellSummon, SummonEntry, "
            "SpellEnrage, EssenceMin, EssenceMax FROM custom_legionforge_worldboss WHERE Enabled = 1"))
        {
            do
            {
                Field* f = result->Fetch();
                BossData b;
                b.Entry = f[0].GetUInt32();  b.NameRu = f[1].GetString();
                b.Phase1Pct = f[2].GetUInt8(); b.Phase2Pct = f[3].GetUInt8();
                b.SpellMain = f[4].GetUInt32(); b.SpellAoe = f[5].GetUInt32();
                b.SpellSummon = f[6].GetUInt32(); b.SummonEntry = f[7].GetUInt32();
                b.SpellEnrage = f[8].GetUInt32();
                b.EssenceMin = f[9].GetUInt32(); b.EssenceMax = f[10].GetUInt32();
                Bosses[b.Entry] = b;
            } while (result->NextRow());
        }
        LOG_INFO("server.loading", "[LEGIONFORGE][WorldBoss] загружено мировых боссов: %u", uint32(Bosses.size()));
    }

    inline BossData const* Get(uint32 entry)
    {
        auto itr = Bosses.find(entry);
        return itr == Bosses.end() ? nullptr : &itr->second;
    }
}

/* =====================================================================
 *  Универсальный AI мирового босса LEGIONFORGE
 * ===================================================================== */
class LegionForge_WorldBossAI : public ScriptedAI
{
public:
    explicit LegionForge_WorldBossAI(Creature* creature) : ScriptedAI(creature), _data(nullptr) { }

    void Reset() override
    {
        _phase = 0;
        _enraged = false;
        _scaled = false;
        _mainTimer = 8000;
        _aoeTimer = 12000;
        _summonTimer = 15000;
        _combatStart = 0;
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void InitializeAI() override
    {
        _data = LegionForgeWorldBoss::Get(me->GetEntry());
        ScriptedAI::InitializeAI();
    }

    void EnterCombat(Unit* who) override
    {
        if (!who)
            return;
        _combatStart = getMSTime();
        me->SetInCombatWithZone();
        ApplyScaling(true);

        if (LegionForgeWorldBoss::AnnounceSpawn())
            LegionForgeWorldBoss::Announce(
                "|cffff2020[LEGIONFORGE]|r Мировой босс |cffffd100%s|r вступил в бой! Расчёт на группу из 2-10 игроков.",
                _data && !_data->NameRu.empty() ? _data->NameRu.c_str() : me->GetName().c_str());
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim() || !_data)
            return;

        if (_mainTimer <= diff)
        {
            if (_data->SpellMain)
                DoCastVictim(_data->SpellMain);
            _mainTimer = 8000 + rand32() % 4000;
        }
        else _mainTimer -= diff;

        if (_aoeTimer <= diff)
        {
            if (_data->SpellAoe)
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    DoCast(target, _data->SpellAoe);
            _aoeTimer = 14000 + rand32() % 8000;
        }
        else _aoeTimer -= diff;

        if (_summonTimer <= diff)
        {
            if (_data->SpellSummon)
                DoCast(me, _data->SpellSummon);
            else if (_data->SummonEntry)
                DoSpawnCreature(_data->SummonEntry, 3.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
            _summonTimer = 25000 + rand32() % 15000;
        }
        else _summonTimer -= diff;

        float hp = HealthPct(me);
        if (_phase == 0 && hp <= float(_data->Phase1Pct))
        {
            _phase = 1;
            Talk(0);
        }
        else if (_phase == 1 && hp <= float(_data->Phase2Pct))
        {
            _phase = 2;
            Talk(1);
        }

        if (!_enraged && _data->SpellEnrage && getMSTimeDiffToNow(_combatStart) > 8 * MINUTE * IN_MILLISECONDS)
        {
            _enraged = true;
            DoCast(me, _data->SpellEnrage);
            Talk(2);
        }

        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* killer) override
    {
        if (!_data)
            return;

        uint32 essence = _data->EssenceMin;
        if (_data->EssenceMax > _data->EssenceMin)
            essence = _data->EssenceMin + rand32() % (_data->EssenceMax - _data->EssenceMin + 1);

        uint32 rewarded = 0;
        Player* killerPlayer = killer ? killer->ToPlayer() : nullptr;
        Group* group = killerPlayer ? killerPlayer->GetGroup() : nullptr;

        if (group)
        {
            for (GroupReference* itr = group->GetFirstMember(); itr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || !member->IsAtGroupRewardDistance(me))
                    continue;
                member->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(essence));
                member->GetSession()->SendNotification(
                    "[LEGIONFORGE] Награда за мирового босса: +%u Сущности пробуждения.", essence);
                ++rewarded;
            }
        }
        else if (killerPlayer)
        {
            killerPlayer->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(essence));
            killerPlayer->GetSession()->SendNotification(
                "[LEGIONFORGE] Награда за мирового босса: +%u Сущности пробуждения.", essence);
            ++rewarded;
        }

        if (LegionForgeWorldBoss::AnnounceKill())
            LegionForgeWorldBoss::Announce(
                "|cffffd100[LEGIONFORGE]|r Мировой босс |cffff2020%s|r повержен! Награду получили %u игроков (+%u Сущности).",
                _data->NameRu.empty() ? me->GetName().c_str() : _data->NameRu.c_str(), rewarded, essence);

        me->SetRespawnTime(LegionForgeWorldBoss::RespawnMinutes() * MINUTE * IN_MILLISECONDS);
    }

private:
    void ApplyScaling(bool first)
    {
        if (!first && _scaled)
            return;
        _scaled = true;

        uint32 players = uint32(me->getThreatList().size());
        if (players == 0)
            players = 1;
        if (players > 10)
            players = 10;

        float scale = 1.0f + LegionForgeWorldBoss::ScalePerPlayer() * float(players);
        CreatureTemplate const* cTemplate = me->GetCreatureTemplate();
        if (cTemplate && cTemplate->MaxLevelHealth > 0)
        {
            uint32 hp = uint32(float(cTemplate->MaxLevelHealth) * scale);
            me->SetMaxHealth(hp);
            me->SetHealth(hp);
        }
        LOG_DEBUG("server.loading", "[LEGIONFORGE][WorldBoss] %s: целей %u, масштаб %.2f",
            me->GetName().c_str(), players, scale);
    }

    LegionForgeWorldBoss::BossData const* _data;
    uint8  _phase = 0;
    bool   _enraged = false;
    bool   _scaled = false;
    uint32 _combatStart = 0;
    uint32 _mainTimer = 8000;
    uint32 _aoeTimer = 12000;
    uint32 _summonTimer = 15000;
};

class LegionForge_WorldBossCreature : public CreatureScript
{
public:
    LegionForge_WorldBossCreature() : CreatureScript("LegionForge_WorldBossCreature") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return LegionForgeWorldBoss::Get(creature->GetEntry()) ? new LegionForge_WorldBossAI(creature) : nullptr;
    }
};

/* =====================================================================
 *  Менеджер: загрузка данных, периодические анонсы
 * ===================================================================== */
class LegionForge_WorldBossWorld : public WorldScript
{
public:
    LegionForge_WorldBossWorld() : WorldScript("LegionForge_WorldBossWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        if (LegionForgeWorldBoss::Enabled())
            LegionForgeWorldBoss::LoadBosses();
    }

    void OnStartup() override
    {
        LOG_INFO("server.loading", ">> LEGIONFORGE World Bosses ....... : %s (%u боссов, респаун %u мин)",
            LegionForgeWorldBoss::Enabled() ? "ON" : "OFF",
            uint32(LegionForgeWorldBoss::Bosses.size()), LegionForgeWorldBoss::RespawnMinutes());
        _announceTimer = 30 * MINUTE * IN_MILLISECONDS;
    }

    void OnUpdate(uint32 diff) override
    {
        if (!LegionForgeWorldBoss::Enabled())
            return;
        if (_announceTimer > diff)
        {
            _announceTimer -= diff;
            return;
        }
        _announceTimer = 60 * MINUTE * IN_MILLISECONDS;

        if (!sConfigMgr->GetBoolDefault("LegionForge.WorldBoss.PeriodicAnnounce", true))
            return;

        LegionForgeWorldBoss::Announce(
            "|cff00ccff[LEGIONFORGE]|r В Азероте и на Расколотых островах активно %u мировых боссов. "
            "За каждого — от 250 до 500 Сущности пробуждения и шанс на «Концентрат силы Титанов»!",
            uint32(LegionForgeWorldBoss::Bosses.size()));
    }

private:
    uint32 _announceTimer = 0;
};

/* =====================================================================
 *  GM-команды: .lf worldboss reload | list
 * ===================================================================== */
class LegionForge_WorldBossCommand : public CommandScript
{
public:
    LegionForge_WorldBossCommand() : CommandScript("LegionForge_WorldBossCommand") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> wbTable =
        {
            { "reload", SEC_ADMINISTRATOR, true,  &HandleReloadCommand, "" },
            { "list",   SEC_ADMINISTRATOR, false, &HandleListCommand,   "" }
        };
        static std::vector<ChatCommand> table =
        {
            { "worldboss", SEC_ADMINISTRATOR, false, nullptr, "", wbTable }
        };
        return table;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        LegionForgeWorldBoss::LoadBosses();
        handler->SendSysMessage("[LEGIONFORGE] Таблица мировых боссов перезагружена.");
        return true;
    }

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        for (auto const& pair : LegionForgeWorldBoss::Bosses)
            handler->PSendSysMessage("%u - %s (фазы %u%%/%u%%, лут %u-%u)",
                pair.first, pair.second.NameRu.c_str(), pair.second.Phase1Pct, pair.second.Phase2Pct,
                pair.second.EssenceMin, pair.second.EssenceMax);
        return true;
    }
};

void AddSC_LegionForge_WorldBosses()
{
    new LegionForge_WorldBossCreature();
    new LegionForge_WorldBossWorld();
    new LegionForge_WorldBossCommand();
}
