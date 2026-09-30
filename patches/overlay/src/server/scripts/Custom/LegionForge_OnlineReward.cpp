// ============================================================================
//  LegionForge — Online Wakening Essence Reward (+50 every 60 minutes)
//  Target: LegionForgeCore 7.3.5 (26124) · Client Build 7.3.5.26124
// ============================================================================
#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "LegionForge_Config.h"

class LegionForge_OnlineReward : public WorldScript
{
public:
    LegionForge_OnlineReward() : WorldScript("legionforge_online_reward"), _accumMs(0) { }

    void OnConfigLoad(bool /*reload*/) override
    {
        _accumMs = 0;
    }

    void OnUpdate(uint32 diff) override
    {
        if (!sConfigMgr->GetBoolDefault("LegionForge.OnlineBonus.Enable", true))
            return;

        uint32 intervalMinutes = sConfigMgr->GetIntDefault("LegionForge.OnlineBonus.IntervalMinutes", LegionForge::DEFAULT_ONLINE_INTERVAL_MIN);
        if (intervalMinutes == 0)
            intervalMinutes = 60;

        uint32 intervalMs = intervalMinutes * MINUTE * IN_MILLISECONDS;
        _accumMs += diff;
        if (_accumMs < intervalMs)
            return;
        _accumMs -= intervalMs;

        uint32 rewardAmount = sConfigMgr->GetIntDefault("LegionForge.OnlineBonus.Amount", LegionForge::DEFAULT_ONLINE_REWARD);
        uint32 currencyId   = sConfigMgr->GetIntDefault("LegionForge.CurrencyId", LegionForge::CURRENCY_WAKENING_ESSENCE);
        uint32 minLevel     = sConfigMgr->GetIntDefault("LegionForge.OnlineBonus.MinLevel", 10);
        bool skipAfk        = sConfigMgr->GetBoolDefault("LegionForge.OnlineBonus.SkipAfk", true);

        SessionMap const& sessions = sWorld->GetAllSessions();
        for (auto const& pair : sessions)
        {
            if (!pair.second)
                continue;
            Player* player = pair.second->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;
            if (player->getLevel() < minLevel)
                continue;
            if (skipAfk && player->isAFK())
                continue;

            player->ModifyCurrency(currencyId, int32(rewardAmount));
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cff00FF88[LegionForge]|r Вы получили |cffFFD800+%u Сущностей пробуждения|r за активную игру онлайн!",
                rewardAmount);
        }
    }

private:
    uint32 _accumMs;
};

void AddSC_LegionForge_OnlineReward()
{
    new LegionForge_OnlineReward();
}
