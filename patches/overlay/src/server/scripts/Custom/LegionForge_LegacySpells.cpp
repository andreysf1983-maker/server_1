/*
 * LEGIONFORGE — Legacy Spells System («Фолианты Древних Знаний»)
 * ---------------------------------------------------------------------
 * Возвращает культовые способности WotLK / Cataclysm / MoP / WoD, которые
 * физически присутствуют в клиентских Spell.db2 билда 7.3.5.26124.
 *
 *   - строгая проверка класса (getClass()) — воин не прочтёт том охотника;
 *   - проверка на дубликат (HasSpell) — том не ломает существующие таланты;
 *   - способность добавляется в общую книгу заклинаний (вкладка «Общее»);
 *   - список томов берётся из БД (custom_legionforge_tomes), при пустой
 *     таблице используется встроенный пул из 40 способностей.
 *
 * Конфиг: LegionForge.Legacy.Enable         = 1
 *         LegionForge.Legacy.TomeCost       = 5000
 *         LegionForge.Legacy.AnnounceWorld  = 0
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Log.h"
#include "LegionForge_Config.h"

namespace LegionForgeLegacy
{
    struct TomeEntry
    {
        uint32 ItemId    = 0;
        uint8  ClassId   = 0;      // 0 = любой класс
        uint32 SpellId   = 0;
        std::string NameRu;
    };

    std::vector<TomeEntry> Tomes;

    struct StaticTome { uint8 cls; uint32 spell; char const* name; };
    static StaticTome const StaticPool[] =
    {
        // ДРУИД
        { CLASS_DRUID,          1126,   "Знак дикой природы (легендарная «Лапка»)" },
        { CLASS_DRUID,           467,   "Шипы" },
        { CLASS_DRUID,        106951,   "Симбиоз (адаптированный)" },
        { CLASS_DRUID,          5570,   "Рой насекомых" },
        { CLASS_DRUID,        132158,   "Природная стремительность" },
        { CLASS_DRUID,          5229,   "Ярость зверя (эпоха WotLK)" },
        { CLASS_DRUID,        102351,   "Обновление (Cenarion Ward)" },
        { CLASS_DRUID,          33891,  "Древо жизни (форма)" },
        // ЧЕРНОКНИЖНИК
        { CLASS_WARLOCK,      103958,   "Метаморфоза (Демонология)" },
        { CLASS_WARLOCK,        6353,   "Ожог души" },
        { CLASS_WARLOCK,       48181,   "Тёмная стая (Haunt)" },
        { CLASS_WARLOCK,        6789,   "Смертельный смерч (Mortal Coil)" },
        { CLASS_WARLOCK,       48018,   "Демонический круг: призыв" },
        { CLASS_WARLOCK,       48020,   "Демонический круг: телепорт" },
        { CLASS_WARLOCK,       30283,   "Теневая ярость (Shadowfury)" },
        // ОХОТНИК
        { CLASS_HUNTER,        53209,   "Выстрел химеры" },
        { CLASS_HUNTER,        63468,   "Разрывной выстрел (версия WotLK)" },
        { CLASS_HUNTER,        34477,   "Перенаправление (Misdirection)" },
        { CLASS_HUNTER,       109306,   "Дух стаи" },
        // ВОИН
        { CLASS_WARRIOR,        7402,   "Удар героя (классический)" },
        { CLASS_WARRIOR,       46924,   "Смертельное спокойствие" },
        { CLASS_WARRIOR,        3411,   "Вмешательство" },
        { CLASS_WARRIOR,      152278,   "Стойка гладиатора" },
        // ПАЛАДИН
        { CLASS_PALADIN,         879,   "Экзорцизм" },
        { CLASS_PALADIN,       31801,   "Печать правды" },
        { CLASS_PALADIN,       20154,   "Печать праведности" },
        { CLASS_PALADIN,       31884,   "Гнев карателя (классический)" },
        { CLASS_PALADIN,       53385,   "Божественная буря" },
        // МАГ
        { CLASS_MAGE,          44614,   "Стрела ледяного огня (Frostfire Bolt)" },
        { CLASS_MAGE,          44572,   "Глубокая заморозка (Deep Freeze)" },
        { CLASS_MAGE,         108978,   "Путешествие во времени (Alter Time)" },
        { CLASS_MAGE,         116011,   "Руна мощи" },
        // ЖРЕЦ
        { CLASS_PRIEST,         2944,   "Всепожирающая чума" },
        { CLASS_PRIEST,        73413,   "Внутренний огонь (Inner Will)" },
        { CLASS_PRIEST,       120517,   "Каскад" },
        // РАЗБОЙНИК
        { CLASS_ROGUE,         51690,   "Череда убийств (Killing Spree)" },
        { CLASS_ROGUE,         74001,   "Теневые клинки (Shadow Blades)" },
        { CLASS_ROGUE,          8647,   "Ослабление доспеха" },
        // ШАМАН
        { CLASS_SHAMAN,         8143,   "Тотем трепета" },
        { CLASS_SHAMAN,        85101,   "Тотем неистовства ветра" },
        { CLASS_SHAMAN,         5213,   "Щит воды (усиленный)" },
        { CLASS_SHAMAN,        58875,   "Духовное путешествие" },
        // РЫЦАРЬ СМЕРТИ
        { CLASS_DEATH_KNIGHT,  50842,   "Вскипание крови (старое)" },
        { CLASS_DEATH_KNIGHT,  49222,   "Костяной щит" },
        { CLASS_DEATH_KNIGHT,  46584,   "Смертельный союз" },
        { CLASS_DEATH_KNIGHT,  49039,   "Нечестивое бешенство" },
        { CLASS_DEATH_KNIGHT,  48263,   "Стойка крови" },
        { CLASS_DEATH_KNIGHT,  48266,   "Стойка льда" },
        { CLASS_DEATH_KNIGHT,  48265,   "Стойка нечестивости" },
        // ДЕМОН-ОХОТНИК
        { CLASS_DEMON_HUNTER, 205604,   "Печать огня (Havoc legacy)" },
        { CLASS_DEMON_HUNTER, 205629,   "Демоническая ярость (legacy)" }
    };

    inline void LoadFromDatabase()
    {
        Tomes.clear();
        if (QueryResult result = WorldDatabase.Query(
            "SELECT ItemId, ClassId, SpellId, NameRu FROM custom_legionforge_tomes WHERE Enabled = 1 ORDER BY ItemId"))
        {
            do
            {
                Field* f = result->Fetch();
                TomeEntry e;
                e.ItemId  = f[0].GetUInt32();
                e.ClassId = f[1].GetUInt8();
                e.SpellId = f[2].GetUInt32();
                e.NameRu  = f[3].GetString();
                Tomes.push_back(e);
            } while (result->NextRow());
        }

        if (Tomes.empty())
        {
            uint32 idx = 0;
            for (auto const& s : StaticPool)
            {
                TomeEntry e;
                e.ItemId  = 950200 + idx;
                e.ClassId = s.cls;
                e.SpellId = s.spell;
                e.NameRu  = s.name;
                Tomes.push_back(e);
                ++idx;
            }
            LOG_INFO("server.loading", "[LEGIONFORGE][Legacy] таблица custom_legionforge_tomes пуста — встроенный пул: %u способностей.", idx);
        }
        else
            LOG_INFO("server.loading", "[LEGIONFORGE][Legacy] загружено фолиантов из БД: %u", uint32(Tomes.size()));
    }

    inline TomeEntry const* FindByItem(uint32 itemId)
    {
        for (auto const& t : Tomes)
            if (t.ItemId == itemId)
                return &t;
        return nullptr;
    }
}

/* =====================================================================
 *  ItemScript: единый обработчик всех фолиантов
 * ===================================================================== */
class LegionForge_TomeItem : public ItemScript
{
public:
    LegionForge_TomeItem() : ItemScript("LegionForge_TomeItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        using namespace LegionForgeLegacy;
        if (!player || !item)
            return false;

        TomeEntry const* tome = FindByItem(item->GetEntry());
        if (!tome)
            return false;   // не наш том — работает стандартная логика предмета

        if (tome->ClassId != 0 && player->GetClass() != tome->ClassId)
        {
            player->GetSession()->SendNotification(
                "[LEGIONFORGE] Этот фолиант хранит знания другого класса. Вашему персонажу он не подойдёт.");
            return true;    // предмет не расходуется
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(tome->SpellId);
        if (!spellInfo)
        {
            player->GetSession()->SendNotification(
                "[LEGIONFORGE] Заклинание %u отсутствует в данных клиента 26124.", tome->SpellId);
            return true;
        }

        if (player->HasSpell(tome->SpellId))
        {
            player->GetSession()->SendNotification(
                "[LEGIONFORGE] Вы уже владеете способностью «%s».", tome->NameRu.c_str());
            return true;
        }

        player->learnSpell(tome->SpellId);
        player->DestroyItemCount(item->GetEntry(), 1, true);

        player->GetSession()->SendNotification(
            "[LEGIONFORGE] Древнее знание «%s» возвращено в вашу книгу заклинаний (вкладка «Общее»).",
            tome->NameRu.c_str());

        if (sConfigMgr->GetBoolDefault("LegionForge.Legacy.AnnounceWorld", false))
        {
            char buf[512];
            snprintf(buf, sizeof(buf),
                "|cff00ccff[LEGIONFORGE]|r %s вернул(а) забытую способность: |cffffd100%s|r!",
                player->GetName().c_str(), tome->NameRu.c_str());
            sWorld->SendServerMessage(SERVER_MSG_STRING, buf);
        }
        return true;
    }
};

/* =====================================================================
 *  WorldScript: загрузка таблицы томов
 * ===================================================================== */
class LegionForge_LegacyWorld : public WorldScript
{
public:
    LegionForge_LegacyWorld() : WorldScript("LegionForge_LegacyWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        if (sConfigMgr->GetBoolDefault("LegionForge.Legacy.Enable", true))
            LegionForgeLegacy::LoadFromDatabase();
    }

    void OnStartup() override
    {
        LOG_INFO("server.loading", ">> LEGIONFORGE Legacy Spells ...... : %s (томов: %u)",
            sConfigMgr->GetBoolDefault("LegionForge.Legacy.Enable", true) ? "ON" : "OFF",
            uint32(LegionForgeLegacy::Tomes.size()));
    }
};

/* =====================================================================
 *  GM-команды: .lf legacy reload | list | learn <spellId>
 * ===================================================================== */
class LegionForge_LegacyCommand : public CommandScript
{
public:
    LegionForge_LegacyCommand() : CommandScript("LegionForge_LegacyCommand") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> legacyTable =
        {
            { "reload", SEC_ADMINISTRATOR, true,  &HandleReloadCommand, "" },
            { "list",   SEC_ADMINISTRATOR, false, &HandleListCommand,   "" },
            { "learn",  SEC_ADMINISTRATOR, false, &HandleLearnCommand,  "" }
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "legacy", SEC_ADMINISTRATOR, false, nullptr, "", legacyTable }
        };
        return commandTable;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        LegionForgeLegacy::LoadFromDatabase();
        handler->SendSysMessage("[LEGIONFORGE] Список фолиантов перезагружен.");
        return true;
    }

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        for (auto const& t : LegionForgeLegacy::Tomes)
            handler->PSendSysMessage("item %u -> spell %u (class %u) %s",
                t.ItemId, t.SpellId, t.ClassId, t.NameRu.c_str());
        return true;
    }

    static bool HandleLearnCommand(ChatHandler* handler, char const* args)
    {
        uint32 spellId = args ? uint32(atoi(args)) : 0;
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!spellId || !player)
        {
            handler->SendSysMessage("Использование: .lf legacy learn <spellId>");
            return true;
        }
        player->learnSpell(spellId);
        handler->PSendSysMessage("[LEGIONFORGE] Заклинание %u выдано персонажу %s.", spellId, player->GetName().c_str());
        return true;
    }
};

void AddSC_LegionForge_LegacySpells()
{
    new LegionForge_TomeItem();
    new LegionForge_LegacyWorld();
    new LegionForge_LegacyCommand();
}
