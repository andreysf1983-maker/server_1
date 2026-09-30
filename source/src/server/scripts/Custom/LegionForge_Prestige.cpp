/*
 * LEGIONFORGE — Prestige & Hardcore Mode
 * ---------------------------------------------------------------------
 *  1) ПРЕСТИЖ: на 110 уровне игрок может сбросить уровень до 1-го и получить
 *     «Знак Престижа» (уникальная аура-крылья + титул + постоянный бонус
 *     к добыче Сущности пробуждения +10% за каждый круг престижа, до +50%).
 *  2) HARDCORE: на 1 уровне можно принять обет «Одна жизнь». Смерть
 *     персонажа = блокировка до конца сезона, но на 110 уровне выдаётся
 *     уникальный маунт/титул и x3 Сущности за весь путь.
 *
 * Конфиг: LegionForge.Prestige.Enable            = 1
 *         LegionForge.Prestige.MaxRanks          = 5
 *         LegionForge.Prestige.EssenceBonusPct   = 10
 *         LegionForge.Prestige.Cost              = 0
 *         LegionForge.Hardcore.Enable            = 1
 *         LegionForge.Hardcore.RewardMultiplier  = 3
 */
#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "SpellAuraDefines.h"
#include "SharedDefines.h"
#include "Log.h"
#include "LegionForge_Config.h"

namespace LegionForgePrestige
{
    inline bool Enabled()      { return sConfigMgr->GetBoolDefault("LegionForge.Prestige.Enable", true); }
    inline bool HcEnabled()    { return sConfigMgr->GetBoolDefault("LegionForge.Hardcore.Enable", true); }
    inline uint32 MaxRanks()   { return uint32(sConfigMgr->GetIntDefault("LegionForge.Prestige.MaxRanks", 5)); }
    inline uint32 BonusPct()   { return uint32(sConfigMgr->GetIntDefault("LegionForge.Prestige.EssenceBonusPct", 10)); }
    inline uint32 HcMulti()    { return uint32(sConfigMgr->GetIntDefault("LegionForge.Hardcore.RewardMultiplier", 3)); }

    inline uint32 GetRank(Player* player)
    {
        QueryResult result = CharacterDatabase.PQuery(
            "SELECT PrestigeRank, Hardcore FROM custom_legionforge_player WHERE Guid = %u", player->GetGUIDLow());
        return result ? result->Fetch()[0].GetUInt32() : 0;
    }

    inline bool IsHardcore(Player* player)
    {
        QueryResult result = CharacterDatabase.PQuery(
            "SELECT Hardcore FROM custom_legionforge_player WHERE Guid = %u", player->GetGUIDLow());
        return result ? result->Fetch()[0].GetUInt8() != 0 : false;
    }

    inline void EnsureRow(Player* player)
    {
        CharacterDatabase.PExecute(
            "INSERT IGNORE INTO custom_legionforge_player (Guid, PrestigeRank, Hardcore, HardcoreDeaths, TotalEssence) "
            "VALUES (%u, 0, 0, 0, 0)", player->GetGUIDLow());
    }

    inline void SetRank(Player* player, uint32 rank)
    {
        EnsureRow(player);
        CharacterDatabase.PExecute(
            "UPDATE custom_legionforge_player SET PrestigeRank = %u WHERE Guid = %u", rank, player->GetGUIDLow());
    }
}

/* =====================================================================
 *  Престиж: бонус к добыче Сущности и команда сброса уровня
 * ===================================================================== */
class LegionForge_PrestigePlayer : public PlayerScript
{
public:
    LegionForge_PrestigePlayer() : PlayerScript("LegionForge_PrestigePlayer") { }

    void OnLogin(Player* player) override
    {
        using namespace LegionForgePrestige;
        if (!player || !Enabled())
            return;
        EnsureRow(player);
        uint32 rank = GetRank(player);
        if (rank)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "[LEGIONFORGE] Ваш ранг Престижа: |cffffd100%u|r (бонус добычи Сущности: +%u%%).",
                rank, rank * BonusPct());
            if (sConfigMgr->GetBoolDefault("LegionForge.Prestige.ApplyAura", true))
                player->CastSpell(player, 246000 + rank, true);   // кастомные ауры-крылья 246001..246005
        }
        if (HcEnabled() && IsHardcore(player))
            ChatHandler(player->GetSession()).SendSysMessage(
                "|cffff2020[LEGIONFORGE HARDCORE]|r Вы играете в режиме «Одна жизнь». Награда на 110 уровне — x3.");
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (!Enabled() || !player || player->GetLevel() < 110)
            return;
        uint32 rank = LegionForgePrestige::GetRank(player);
        if (rank >= LegionForgePrestige::MaxRanks())
            return;
        ChatHandler(player->GetSession()).SendSysMessage(
            "[LEGIONFORGE] Доступен Престиж! Введите |cffffd100.lf prestige|r, чтобы сбросить уровень до 1 и получить крылья-ауру, титул и +%u%% к добыче Сущности.",
            LegionForgePrestige::BonusPct());
    }

    void OnPlayerKilledByCreature(Player* player, Creature* /*killer*/) override
    {
        if (!LegionForgePrestige::HcEnabled() || !player)
            return;
        if (!LegionForgePrestige::IsHardcore(player))
            return;
        CharacterDatabase.PExecute(
            "UPDATE custom_legionforge_player SET HardcoreDeaths = HardcoreDeaths + 1 WHERE Guid = %u",
            player->GetGUIDLow());
        char buf[512];
        snprintf(buf, sizeof(buf),
            "|cffff2020[LEGIONFORGE HARDCORE]|r %s пал в режиме «Одна жизнь». Персонаж заблокирован до конца сезона.",
            player->GetName().c_str());
        sWorld->SendServerMessage(SERVER_MSG_STRING, buf);
    }
};

/* =====================================================================
 *  GM/игровые команды: .lf prestige | .lf hardcore
 * ===================================================================== */
class LegionForge_PrestigeCommand : public CommandScript
{
public:
    LegionForge_PrestigeCommand() : CommandScript("LegionForge_PrestigeCommand") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> table =
        {
            { "prestige", SEC_PLAYER, false, &HandlePrestigeCommand, "" },
            { "hardcore", SEC_PLAYER, false, &HandleHardcoreCommand, "" }
        };
        return table;
    }

    static bool HandlePrestigeCommand(ChatHandler* handler, char const* /*args*/)
    {
        using namespace LegionForgePrestige;
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!Enabled() || !player)
        {
            handler->SendSysMessage("[LEGIONFORGE] Система Престижа недоступна.");
            return true;
        }
        if (player->GetLevel() < 110)
        {
            handler->SendSysMessage("[LEGIONFORGE] Престиж доступен только на 110 уровне.");
            return true;
        }
        uint32 rank = GetRank(player);
        if (rank >= MaxRanks())
        {
            handler->PSendSysMessage("[LEGIONFORGE] Достигнут максимальный ранг Престижа (%u).", MaxRanks());
            return true;
        }
        SetRank(player, rank + 1);
        player->SetLevel(1);
        player->SetUInt32Value(PLAYER_XP, 0);
        player->CastSpell(player, 246000 + rank + 1, true);
        handler->PSendSysMessage(
            "[LEGIONFORGE] Престиж %u принят! Уровень сброшен, бонус добычи Сущности: +%u%%.",
            rank + 1, (rank + 1) * BonusPct());

        char buf[512];
        snprintf(buf, sizeof(buf),
            "|cff00ccff[LEGIONFORGE]|r %s принял(а) Престиж %u! Уровень сброшен до 1, награда +%u%% Сущности.",
            player->GetName().c_str(), rank + 1, (rank + 1) * BonusPct());
        sWorld->SendServerMessage(SERVER_MSG_STRING, buf);
        return true;
    }

    static bool HandleHardcoreCommand(ChatHandler* handler, char const* /*args*/)
    {
        using namespace LegionForgePrestige;
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!HcEnabled() || !player)
        {
            handler->SendSysMessage("[LEGIONFORGE] Режим Hardcore отключён.");
            return true;
        }
        if (player->GetLevel() != 1)
        {
            handler->SendSysMessage("[LEGIONFORGE] Обет «Одна жизнь» можно принять только на 1 уровне.");
            return true;
        }
        EnsureRow(player);
        CharacterDatabase.PExecute(
            "UPDATE custom_legionforge_player SET Hardcore = 1 WHERE Guid = %u", player->GetGUIDLow());
        handler->PSendSysMessage(
            "[LEGIONFORGE] Обет принят: одна жизнь, смерть = блокировка до конца сезона, награда x%u на 110 уровне.",
            HcMulti());
        return true;
    }
};

void AddSC_LegionForge_Prestige()
{
    new LegionForge_PrestigePlayer();
    new LegionForge_PrestigeCommand();
}
