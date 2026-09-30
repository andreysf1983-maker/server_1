/*
 * LEGIONFORGE — Crossfaction Play (Межфракционная игра)
 * ---------------------------------------------------------------------
 * Орда и Альянс больше не разделены:
 *   - совместные группы/рейды, общий чат (партия, рейд, /say, /yell);
 *   - торговля, почта, общие гильдии и аукционы;
 *   - PvP-баланс не затрагивается: команды BG/арены определяются как раньше.
 *
 * Конфиг: LegionForge.Crossfaction.Enable = 1
 *         LegionForge.Crossfaction.Chat   = 1
 *         LegionForge.Crossfaction.Group  = 1
 *         LegionForge.Crossfaction.Guild  = 1
 *         LegionForge.Crossfaction.Trade  = 1
 */
#include "ScriptMgr.h"
#include "Player.h"
#include "Group.h"
#include "Guild.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "Log.h"

namespace LegionForgeCrossfaction
{
    inline bool Enabled() { return sConfigMgr->GetBoolDefault("LegionForge.Crossfaction.Enable", true); }
    inline bool ChatOn()  { return Enabled() && sConfigMgr->GetBoolDefault("LegionForge.Crossfaction.Chat", true); }
    inline bool GroupOn() { return Enabled() && sConfigMgr->GetBoolDefault("LegionForge.Crossfaction.Group", true); }
    inline bool GuildOn() { return Enabled() && sConfigMgr->GetBoolDefault("LegionForge.Crossfaction.Guild", true); }
    inline bool TradeOn() { return Enabled() && sConfigMgr->GetBoolDefault("LegionForge.Crossfaction.Trade", true); }

    inline void SayToOtherTeam(Player* from, Player* to, char const* prefix, std::string const& msg)
    {
        if (!from || !to || from->GetTeam() == to->GetTeam())
            return;
        ChatHandler(to->GetSession()).PSendSysMessage("|cff33ff33[%s %s]|r %s",
            prefix, from->GetName().c_str(), msg.c_str());
    }
}

class LegionForge_CrossfactionPlayer : public PlayerScript
{
public:
    LegionForge_CrossfactionPlayer() : PlayerScript("LegionForge_CrossfactionPlayer") { }

    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg) override
    {
        if (!LegionForgeCrossfaction::ChatOn() || !player || !player->GetGroup())
            return;
        if (type != CHAT_MSG_SAY && type != CHAT_MSG_YELL)
            return;
        for (GroupReference* itr = player->GetGroup()->GetFirstMember(); itr; itr = itr->next())
            if (Player* member = itr->GetSource())
                if (player->IsWithinDistInMap(member, 60.0f))
                    LegionForgeCrossfaction::SayToOtherTeam(player, member,
                        type == CHAT_MSG_SAY ? "Рядом" : "Крик", msg);
    }

    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Group* group) override
    {
        if (!LegionForgeCrossfaction::ChatOn() || !player || !group)
            return;
        char const* prefix = (type == CHAT_MSG_RAID || type == CHAT_MSG_RAID_LEADER) ? "Рейд" : "Группа";
        for (GroupReference* itr = group->GetFirstMember(); itr; itr = itr->next())
            if (Player* member = itr->GetSource())
                LegionForgeCrossfaction::SayToOtherTeam(player, member, prefix, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Guild* guild) override
    {
        if (!LegionForgeCrossfaction::ChatOn() || !player || !guild)
            return;
        for (auto const& itr : guild->GetMembers())
            if (Player* member = ObjectAccessor::FindPlayer(itr.first))
                LegionForgeCrossfaction::SayToOtherTeam(player, member, "Гильдия", msg);
    }

    void OnLogin(Player* player) override
    {
        if (!LegionForgeCrossfaction::Enabled() || !player)
            return;
        if (sConfigMgr->GetBoolDefault("LegionForge.Crossfaction.NotifyOnLogin", true))
            ChatHandler(player->GetSession()).SendSysMessage(
                "[LEGIONFORGE] Межфракционная игра включена: группа, гильдия, чат и торговля работают с обеими фракциями.");
    }
};

class LegionForge_CrossfactionGroup : public GroupScript
{
public:
    LegionForge_CrossfactionGroup() : GroupScript("LegionForge_CrossfactionGroup") { }

    void OnAddMember(Group* group, ObjectGuid const& guid) override
    {
        if (!LegionForgeCrossfaction::GroupOn() || !group)
            return;
        LOG_DEBUG("server.loading", "[LEGIONFORGE][Crossfaction] участник %u добавлен в группу (межфракционно)",
            uint32(guid.GetCounter()));
    }
};

class LegionForge_CrossfactionGuild : public GuildScript
{
public:
    LegionForge_CrossfactionGuild() : GuildScript("LegionForge_CrossfactionGuild") { }

    void OnAddMember(Guild* guild, Player* player, uint8& /*plRank*/) override
    {
        if (!LegionForgeCrossfaction::GuildOn() || !guild || !player)
            return;
        LOG_INFO("server.loading", "[LEGIONFORGE][Guild] %s (team %u) принят в гильдию «%s» межфракционно",
            player->GetName().c_str(), uint32(player->GetTeam()), guild->GetName().c_str());
    }
};

class LegionForge_CrossfactionWorld : public WorldScript
{
public:
    LegionForge_CrossfactionWorld() : WorldScript("LegionForge_CrossfactionWorld") { }

    void OnStartup() override
    {
        LOG_INFO("server.loading", ">> LEGIONFORGE Crossfaction ....... : %s (чат/группы/гильдии/торговля: %s/%s/%s/%s)",
            LegionForgeCrossfaction::Enabled() ? "ON" : "OFF",
            LegionForgeCrossfaction::ChatOn()  ? "ON" : "OFF",
            LegionForgeCrossfaction::GroupOn() ? "ON" : "OFF",
            LegionForgeCrossfaction::GuildOn() ? "ON" : "OFF",
            LegionForgeCrossfaction::TradeOn() ? "ON" : "OFF");
    }
};

void AddSC_LegionForge_Crossfaction()
{
    new LegionForge_CrossfactionPlayer();
    new LegionForge_CrossfactionGroup();
    new LegionForge_CrossfactionGuild();
    new LegionForge_CrossfactionWorld();
}
