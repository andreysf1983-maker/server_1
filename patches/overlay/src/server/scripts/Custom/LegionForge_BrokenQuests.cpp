// ============================================================================
//  LegionForge — Broken Quest Auto-Completion & Technical Compensation
//  Target: LegionForgeCore 7.3.5 (26124) · Client Build 7.3.5.26124
// ============================================================================
//  In LegionForgeCore 7.3.5 (26124), PlayerScript has OnUpdate(Player*, uint32),
//  OnLogin(Player*) and OnQuestReward(Player*, Quest const*). We scan the
//  player's active quest log every 3 seconds for any quest ID in
//  LegionForge::BrokenQuestIds (plus a manual .questfix command), immediately
//  completing and rewarding it with a system notification and +25 Essences.
// ============================================================================
#include "ScriptMgr.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestData.h"
#include "WorldSession.h"
#include "LegionForge_Config.h"

class LegionForge_BrokenQuests : public PlayerScript
{
public:
    LegionForge_BrokenQuests() : PlayerScript("legionforge_broken_quests") { }

    void OnUpdate(Player* player, uint32 diff) override
    {
        if (!player || !player->IsInWorld())
            return;

        uint32& timer = _checkTimer[player->GetGUIDLow()];
        if (timer > diff)
        {
            timer -= diff;
            return;
        }
        timer = 3000; // check every 3 seconds

        if (!sConfigMgr->GetBoolDefault("LegionForge.BrokenQuests.Enable", true))
            return;

        for (uint32 i = 0; i < LegionForge::BrokenQuestCount; ++i)
        {
            uint32 questId = LegionForge::BrokenQuestIds[i];
            if (!questId)
                continue;

            if (player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
            {
                if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
                {
                    player->CompleteQuest(questId);
                    player->RewardQuest(quest, 0, player, false);
                    uint32 comp = sConfigMgr->GetIntDefault("LegionForge.BrokenQuests.CompensationEssence", 25);
                    if (comp > 0)
                        player->ModifyCurrency(LegionForge::CURRENCY_WAKENING_ESSENCE, int32(comp));

                    player->GetSession()->SendNotification("Техническая компенсация: проблемный квест #%u выполнен автоматически (+%u Сущностей)!", questId, comp);
                    ChatHandler(player->GetSession()).PSendSysMessage(
                        "|cff00FF88[LegionForge]|r Квест #%u автоматически зачтён системой технической компенсации (+%u Сущностей пробуждения).",
                        questId, comp);
                }
            }
        }
    }

    void OnLogout(Player* player) override
    {
        if (player)
            _checkTimer.erase(player->GetGUIDLow());
    }

private:
    std::unordered_map<uint32, uint32> _checkTimer;
};

void AddSC_LegionForge_BrokenQuests()
{
    new LegionForge_BrokenQuests();
}
