// ============================================================================
//  LegionForge — Unified Configuration & Built-in Mod Registry
//  Target: LegionForgeCore 7.3.5 (26124) · Client Build 7.3.5.26124
// ============================================================================
//  This header is pre-integrated into both custom/src/ and
//  patches/overlay/src/server/scripts/Custom/ so that START.bat compiles all
//  custom systems and catalog mods automatically with zero manual porting.
// ============================================================================
#ifndef LEGIONFORGE_CONFIG_H
#define LEGIONFORGE_CONFIG_H

#include "Common.h"
#include "Config.h"
#include "SharedDefines.h"

namespace LegionForge
{
    // ------------------------------------------------------------------------
    // Core Economy & Item Upgrade Constants
    // ------------------------------------------------------------------------
    enum Constants : uint32
    {
        CURRENCY_WAKENING_ESSENCE   = 1533,   // Сущность пробуждения (Legion 7.3.5)
        CURRENCY_ORDER_RESOURCES    = 1220,   // Ресурсы оплота

        DEFAULT_ONLINE_REWARD       = 50,     // +50 Сущностей каждый час
        DEFAULT_ONLINE_INTERVAL_MIN = 60,     // 60 минут

        LEGENDARY_ILVL_CAP          = 1200,   // Максимальный уровень легендарок
        LEGENDARY_UPGRADE_STEP      = 5,      // +5 ilvl за 1 Концентрат силы
        ENDGAME_ILVL_MIN            = 985,    // Минимальный уровень для закалки
        ENDGAME_ILVL_CAP            = 1000,   // Уровень после Эссенции закалки

        PRICE_CONCENTRATE           = 800,    // Цена Концентрата силы (+5 ilvl леге)
        PRICE_ENDGAME_TEMPER        = 1500,   // Цена Эссенции закалки (985 -> 1000)
        PRICE_LEGACY_TOME           = 5000,   // Цена тома забытой способности

        ITEM_CONCENTRATE_CUSTOM     = 950090, // Кастомный ID Концентрата силы
        ITEM_ENDGAME_CUSTOM         = 950091, // Кастомный ID Эссенции закалки
        ITEM_MORPHING_FLASK_CUSTOM  = 950092, // Кастомный ID Колбы метаморфоз

        NPC_VENDOR_COLLECTOR        = 950100, // Каль'тарас Хранитель Арсенала
        NPC_UNIVERSAL_SERVICES      = 950101, // Сервисный НПС (баффер, чары, маунты, профессии)
        NPC_COSMETIC_MORPHER        = 950102, // Мастер иллюзий и морфа
        NPC_EXCHANGE_BM_LOTTERY     = 950103, // Обменник валют, Чёрный рынок и Лотерея
        NPC_COMPANION_BOT           = 950104, // ИИ-напарник (Playerbot AI Companion)

        WORLDBOSS_ENTRY_FIRST       = 900001,
        WORLDBOSS_ENTRY_LAST        = 900070
    };

    // ------------------------------------------------------------------------
    // 44 Restored Legacy Abilities (Class-checked)
    // ------------------------------------------------------------------------
    struct LegacySpellEntry
    {
        uint32 tomeItemId;   // Custom item ID (950001..950044)
        uint8  classId;      // Required WoW class (CLASS_WARRIOR..CLASS_DEMON_HUNTER)
        uint32 spellId;      // Restored SpellID in 7.3.5.26124
        char const* nameRu;  // Russian ability name for gossip/notifications
    };

    static LegacySpellEntry const LegacySpells[] =
    {
        // Воин (CLASS_WARRIOR = 1)
        { 950001, CLASS_WARRIOR,      6673,   "Боевой крик" },
        { 950002, CLASS_WARRIOR,      469,    "Командующий крик" },
        { 950003, CLASS_WARRIOR,      12328,  "Размашистые удары" },
        { 950004, CLASS_WARRIOR,      20230,  "Ответный удар" },
        // Паладин (CLASS_PALADIN = 2)
        { 950005, CLASS_PALADIN,      31801,  "Печать правды" },
        { 950006, CLASS_PALADIN,      879,    "Экзорцизм" },
        { 950007, CLASS_PALADIN,      54428,  "Божественная мольба" },
        { 950008, CLASS_PALADIN,      86150,  "Страж древних королей" },
        // Охотник (CLASS_HUNTER = 3)
        { 950009, CLASS_HUNTER,       13159,  "Аспект стаи" },
        { 950010, CLASS_HUNTER,       20043,  "Аспект дикой природы" },
        { 950011, CLASS_HUNTER,       1002,   "Глаза зверя" },
        // Разбойник (CLASS_ROGUE = 4)
        { 950012, CLASS_ROGUE,        73981,  "Перенаправление" },
        { 950013, CLASS_ROGUE,        26679,  "Смертельный бросок" },
        { 950014, CLASS_ROGUE,        8647,   "Снятие доспеха" },
        { 950015, CLASS_ROGUE,        36554,  "Шаг сквозь тень" },
        // Жрец (CLASS_PRIEST = 5)
        { 950016, CLASS_PRIEST,       588,    "Внутренний огонь" },
        { 950017, CLASS_PRIEST,       64901,  "Гимн надежды" },
        { 950018, CLASS_PRIEST,       32546,  "Связующее исцеление" },
        { 950019, CLASS_PRIEST,       724,    "Колодец Света" },
        // Рыцарь смерти (CLASS_DEATH_KNIGHT = 6)
        { 950020, CLASS_DEATH_KNIGHT, 57330,  "Рог Зимы" },
        { 950021, CLASS_DEATH_KNIGHT, 73975,  "Некротический удар" },
        { 950022, CLASS_DEATH_KNIGHT, 50842,  "Мор" },
        { 950023, CLASS_DEATH_KNIGHT, 108199, "Хватка кровожада" },
        // Шаман (CLASS_SHAMAN = 7)
        { 950024, CLASS_SHAMAN,       3599,   "Опаляющий тотем" },
        { 950025, CLASS_SHAMAN,       8190,   "Тотем магмы" },
        { 950026, CLASS_SHAMAN,       52127,  "Водный щит" },
        { 950027, CLASS_SHAMAN,       8024,   "Оружие языка пламени" },
        // Маг (CLASS_MAGE = 8)
        { 950028, CLASS_MAGE,         30482,  "Раскаленный доспех" },
        { 950029, CLASS_MAGE,         1459,   "Чародейская гениальность" },
        { 950030, CLASS_MAGE,         6117,   "Доспех мага" },
        { 950031, CLASS_MAGE,         7302,   "Ледяной доспех" },
        // Чернокнижник (CLASS_WARLOCK = 9)
        { 950032, CLASS_WARLOCK,      103958, "Метаморфоза (Демонология)" },
        { 950033, CLASS_WARLOCK,      113858, "Темная душа: Нестабильность" },
        { 950034, CLASS_WARLOCK,      113860, "Темная душа: Страдание" },
        { 950035, CLASS_WARLOCK,      28176,  "Доспех Скверны" },
        // Монах (CLASS_MONK = 10)
        { 950036, CLASS_MONK,         115921, "Наследие императора" },
        { 950037, CLASS_MONK,         116781, "Наследие белого тигра" },
        { 950038, CLASS_MONK,         122281, "Целебный эликсир" },
        // Друид (CLASS_DRUID = 11)
        { 950039, CLASS_DRUID,        1126,   "Знак дикой природы" },
        { 950040, CLASS_DRUID,        467,    "Шипы" },
        { 950041, CLASS_DRUID,        16689,  "Хватка природы" },
        { 950042, CLASS_DRUID,        62606,  "Дикая защита" },
        // Охотник на демонов (CLASS_DEMON_HUNTER = 12)
        { 950043, CLASS_DEMON_HUNTER, 227827, "Поглощение магии" },
        { 950044, CLASS_DEMON_HUNTER, 217832, "Пленение" }
    };

    static constexpr uint32 LegacySpellsCount = sizeof(LegacySpells) / sizeof(LegacySpells[0]);

    // ------------------------------------------------------------------------
    // Known Broken Quests in Legion 7.3.5 (Auto-complete + Essence reward)
    // ------------------------------------------------------------------------
    static uint32 const BrokenQuestIds[] =
    {
        38834, // Вступая в бой (Азсуна)
        39864, // Штормхейм: сценарий корабля
        40519, // Легион: вступительный сценарий пристанища
        42233, // Крутогорье: фазовый переход
        43341, // Сурамар: разлом теней
        44184, // Каражан: осколок времени
        46730, // Расколотый берег: штурм
        47221, // Аргус: Крокуун вступление
        48440, // Аргус: Пустоши Анторуса
        48460  // Аргус: Мак'Ари маяк
    };

    static constexpr uint32 BrokenQuestCount = sizeof(BrokenQuestIds) / sizeof(BrokenQuestIds[0]);

    // ------------------------------------------------------------------------
    // 100+ Russian Bot Names for Playerbot AI Companions & PvP Slots
    // ------------------------------------------------------------------------
    static char const* const RussianBotNames[] =
    {
        "Александр", "Дмитрий", "Максим", "Сергей", "Андрей", "Алексей", "Артём", "Илья", "Кирилл", "Михаил",
        "Никита", "Матвей", "Роман", "Егор", "Арсений", "Иван", "Денис", "Евгений", "Даниил", "Тимофей",
        "Владислав", "Игорь", "Глеб", "Марк", "Ярослав", "Богдан", "Олег", "Виктор", "Антон", "Константин",
        "Анна", "Мария", "Елена", "Дарья", "Алина", "Ирина", "Екатерина", "Арина", "Полина", "Ольга",
        "Юлия", "Татьяна", "Наталья", "Виктория", "Ксения", "Светлана", "Валерия", "Алиса", "София", "Вероника",
        "Диана", "Елизавета", "Анастасия", "Кристина", "Варвара", "Милана", "Яна", "Надежда", "Любовь", "Марина",
        "Ярополк", "Святослав", "Радомир", "Добрыня", "Любомир", "Владимир", "Всеволод", "Мстислав", "Ростислав", "Борислав",
        "Станислав", "Вячеслав", "Мирослав", "Бронислав", "Яромир", "Святозар", "Велимир", "Ратибор", "Горыня", "Любава",
        "Злата", "Мирослава", "Ярослава", "Лада", "Ведана", "Снежана", "Забава", "Милица", "Божена", "Весна",
        "Громобой", "Светозар", "Черномор", "Белояр", "Волк", "Сокол", "Кречет", "Витязь", "Варяг", "Русич",
        "Пересвет", "Ослябя", "Ермак", "Коловрат", "Ратник", "Булат", "Скиф", "Сармат", "Берегиня", "Валькирия"
    };

    static constexpr uint32 RussianBotNamesCount = sizeof(RussianBotNames) / sizeof(RussianBotNames[0]);

    static char const* const RussianBotPhrases[] =
    {
        "Держу строй! Ассистирую по цели лидера.",
        "Готов к бою, прикрываю группу контролем!",
        "Отличный темп, продолжаем зачистку!",
        "Если нужна помощь на мировом боссе — зовите!",
        "Контроль наготове, сбиваю касты по фокусу.",
        "Неплохой лут! Копим Сущности пробуждения на апгрейд леги.",
        "Баффы обновлены, можно пуллить следующий пак.",
        "Слава героям Азерота! Идём до победного конца."
    };

    static constexpr uint32 RussianBotPhrasesCount = sizeof(RussianBotPhrases) / sizeof(RussianBotPhrases[0]);
}

#endif // LEGIONFORGE_CONFIG_H
