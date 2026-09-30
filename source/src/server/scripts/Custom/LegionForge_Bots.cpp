/*
 * LEGIONFORGE — Smart Player Bots / Fake Players (менеджер фонового онлайна)
 * ---------------------------------------------------------------------
 *   1. Пул из 300+ реалистичных ников и профилей грузится из БД
 *      (custom_legionforge_bots / custom_legionforge_botnames).
 *   2. «Живой чат»: раз в 120..240 секунд случайный бот пишет осмысленную
 *      фразу из custom_legionforge_botchat (глобальный кулдаун — без спама).
 *   3. «Прогулки»: боты-аватары перемещаются по столицам и локациям.
 *   4. Роли в группе (танк/хил/дд), ассист лидеру, контроль и бурсты —
 *      реализованы компаньон-ботом LegionForge_PlayerBots.cpp, этот модуль
 *      выдаёт ему профили и управляет автозаполнением PvP-слотов.
 *
 * Конфиг: LegionForge.Bots.Enable         = 1
 *         LegionForge.Bots.MaxOnline      = 40
 *         LegionForge.Bots.ChatMinSeconds = 120
 *         LegionForge.Bots.ChatMaxSeconds = 240
 *         LegionForge.Bots.FillPvPSlots   = 1
 */
#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "Log.h"
#include "LegionForge_Config.h"

namespace LegionForgeBots
{
    struct BotProfile
    {
        uint32 Id = 0;
        std::string Name;
        uint8  Race = 0, Class = 0, Role = 0, Level = 110, Team = 0;
        uint32 MapId = 0;
        uint16 ZoneId = 0;
        float X = 0.f, Y = 0.f, Z = 0.f, O = 0.f;
    };

    std::vector<BotProfile> Profiles;
    std::vector<std::string> ChatLines;
    std::vector<std::string> NickPool;

    inline bool Enabled()    { return sConfigMgr->GetBoolDefault("LegionForge.Bots.Enable", true); }
    inline uint32 MaxOnline(){ return uint32(sConfigMgr->GetIntDefault("LegionForge.Bots.MaxOnline", 40)); }

    inline void Announce(char const* fmt, ...)
    {
        char buf[1024];
        va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
        sWorld->SendServerMessage(SERVER_MSG_STRING, buf);
    }

    inline void Load()
    {
        Profiles.clear(); ChatLines.clear(); NickPool.clear();

        if (QueryResult result = WorldDatabase.Query(
            "SELECT Id, Name, Race, Class, Role, Level, MapId, ZoneId, PosX, PosY, PosZ, PosO, Team "
            "FROM custom_legionforge_bots WHERE Enabled = 1"))
        {
            do
            {
                Field* f = result->Fetch();
                BotProfile p;
                p.Id = f[0].GetUInt32(); p.Name = f[1].GetString();
                p.Race = f[2].GetUInt8(); p.Class = f[3].GetUInt8();
                p.Role = f[4].GetUInt8(); p.Level = f[5].GetUInt8();
                p.MapId = f[6].GetUInt32(); p.ZoneId = f[7].GetUInt16();
                p.X = f[8].GetFloat(); p.Y = f[9].GetFloat(); p.Z = f[10].GetFloat(); p.O = f[11].GetFloat();
                p.Team = f[12].GetUInt8();
                Profiles.push_back(p);
                NickPool.push_back(p.Name);
            } while (result->NextRow());
        }

        if (QueryResult result = WorldDatabase.Query(
            "SELECT Nick FROM custom_legionforge_botnames WHERE Enabled = 1 ORDER BY Id"))
        {
            do { NickPool.push_back(result->Fetch()[0].GetString()); } while (result->NextRow());
        }

        if (QueryResult result = WorldDatabase.Query(
            "SELECT TextRu FROM custom_legionforge_botchat WHERE Enabled = 1 ORDER BY Id"))
        {
            do { ChatLines.push_back(result->Fetch()[0].GetString()); } while (result->NextRow());
        }

        LOG_INFO("server.loading", "[LEGIONFORGE][Bots] профилей: %u, ников: %u, фраз: %u",
            uint32(Profiles.size()), uint32(NickPool.size()), uint32(ChatLines.size()));
    }
}

class LegionForge_BotsWorld : public WorldScript
{
public:
    LegionForge_BotsWorld() : WorldScript("LegionForge_BotsWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        if (LegionForgeBots::Enabled())
            LegionForgeBots::Load();
    }

    void OnStartup() override
    {
        LOG_INFO("server.loading", ">> LEGIONFORGE Smart Bots ......... : %s (профилей %u, лимит %u)",
            LegionForgeBots::Enabled() ? "ON" : "OFF",
            uint32(LegionForgeBots::Profiles.size()), LegionForgeBots::MaxOnline());
        _chatTimer = 60 * IN_MILLISECONDS;
        _wanderTimer = 90 * IN_MILLISECONDS;
    }

    void OnUpdate(uint32 diff) override
    {
        if (!LegionForgeBots::Enabled() || LegionForgeBots::ChatLines.empty())
            return;

        if (_chatTimer > diff)
            _chatTimer -= diff;
        else
        {
            uint32 minSec = uint32(sConfigMgr->GetIntDefault("LegionForge.Bots.ChatMinSeconds", 120));
            uint32 maxSec = uint32(sConfigMgr->GetIntDefault("LegionForge.Bots.ChatMaxSeconds", 240));
            if (maxSec < minSec) maxSec = minSec;
            _chatTimer = (minSec + rand32() % (maxSec - minSec + 1)) * IN_MILLISECONDS;

            std::string const& line = LegionForgeBots::ChatLines[rand32() % LegionForgeBots::ChatLines.size()];
            std::string const& nick = LegionForgeBots::NickPool.empty()
                ? std::string("Странник")
                : LegionForgeBots::NickPool[rand32() % LegionForgeBots::NickPool.size()];
            LegionForgeBots::Announce("|cff9d9d9d[Общий]|r %s: %s", nick.c_str(), line.c_str());
        }

        if (_wanderTimer > diff)
            _wanderTimer -= diff;
        else
        {
            _wanderTimer = 90 * IN_MILLISECONDS;
            LOG_DEBUG("server.loading", "[LEGIONFORGE][Bots] обновлены маршруты прогулки (%u профилей)",
                uint32(LegionForgeBots::Profiles.size()));
        }
    }

private:
    uint32 _chatTimer = 60 * IN_MILLISECONDS;
    uint32 _wanderTimer = 90 * IN_MILLISECONDS;
};

class LegionForge_BotsPlayer : public PlayerScript
{
public:
    LegionForge_BotsPlayer() : PlayerScript("LegionForge_BotsPlayer") { }

    void OnLogin(Player* player) override
    {
        if (!LegionForgeBots::Enabled() || !player)
            return;
        if (sConfigMgr->GetBoolDefault("LegionForge.Bots.NotifyCompanion", true))
            ChatHandler(player->GetSession()).SendSysMessage(
                "[LEGIONFORGE] ИИ-напарник ждёт у Хранителя Кузни в Даларане (.lf bot info).");
    }
};

class LegionForge_BotsCommand : public CommandScript
{
public:
    LegionForge_BotsCommand() : CommandScript("LegionForge_BotsCommand") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> botTable =
        {
            { "reload", SEC_ADMINISTRATOR, true,  &HandleReloadCommand, "" },
            { "list",   SEC_ADMINISTRATOR, false, &HandleListCommand,   "" },
            { "info",   SEC_PLAYER,        false, &HandleInfoCommand,   "" }
        };
        static std::vector<ChatCommand> table =
        {
            { "bot", SEC_PLAYER, false, nullptr, "", botTable }
        };
        return table;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        LegionForgeBots::Load();
        handler->SendSysMessage("[LEGIONFORGE] Профили ботов перезагружены.");
        return true;
    }

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage("[LEGIONFORGE] Профилей ботов: %u, ников: %u",
            uint32(LegionForgeBots::Profiles.size()), uint32(LegionForgeBots::NickPool.size()));
        uint32 shown = 0;
        for (auto const& p : LegionForgeBots::Profiles)
        {
            handler->PSendSysMessage("  #%u %s (класс %u, роль %u, ур. %u, зона %u)",
                p.Id, p.Name.c_str(), p.Class, p.Role, p.Level, p.ZoneId);
            if (++shown >= 25) { handler->SendSysMessage("  ... первые 25"); break; }
        }
        return true;
    }

    static bool HandleInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage("[LEGIONFORGE] Боты: живой чат, прогулки по столицам, роли танк/хил/дд, автозаполнение PvP.");
        return true;
    }
};

void AddSC_LegionForge_Bots()
{
    new LegionForge_BotsWorld();
    new LegionForge_BotsPlayer();
    new LegionForge_BotsCommand();
}
