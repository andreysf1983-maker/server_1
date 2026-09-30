/*
 * LEGIONFORGE — автономная серверная платформа World of Warcraft: Legion
 * ----------------------------------------------------------------------------
 * Файл      : LegionForgeVersion.h
 * Назначение: Единая точка правды о версии ядра, клиентском билде и бренде.
 *             Подключается из Common/Define.h, поэтому доступен во всех
 *             модулях ядра (bnetserver, worldserver, common, scripts).
 * ----------------------------------------------------------------------------
 * ВАЖНО: клиентский билд жёстко зафиксирован — 7.3.5 (26124).
 *        Любые попытки собрать сервер под другой билд должны приводить к
 *        ошибке компиляции (см. static_assert ниже).
 */

#ifndef LEGIONFORGE_VERSION_H
#define LEGIONFORGE_VERSION_H

// ---------------------------------------------------------------------------
// Брендинг проекта
// ---------------------------------------------------------------------------
#define LEGIONFORGE_BRAND           "LEGIONFORGE"
#define LEGIONFORGE_CORE_NAME       "LegionForgeCore"
#define LEGIONFORGE_URL             "https://legionforge.gg"
#define LEGIONFORGE_DISCORD         "discord.gg/legionforge"

// ---------------------------------------------------------------------------
// Версия платформы (SemVer: MAJOR.MINOR.PATCH)
// ---------------------------------------------------------------------------
#define LEGIONFORGE_VERSION_MAJOR   3
#define LEGIONFORGE_VERSION_MINOR   1
#define LEGIONFORGE_VERSION_PATCH   0
#define LEGIONFORGE_VERSION_STRING  "3.1.0"
#define LEGIONFORGE_REVISION        "LEGIONFORGE-3.1.0-26124"

// ---------------------------------------------------------------------------
// Клиент / сервер: 7.3.5 Build 26124
// ---------------------------------------------------------------------------
#define LEGIONFORGE_EXPANSION       6                   // EXPANSION_LEGION
#define LEGIONFORGE_EXPANSION_NAME  "Legion 7.3.5"
#define LEGIONFORGE_CLIENT_BUILD    26124               // <- ЕДИНСТВЕННЫЙ билд
#define LEGIONFORGE_CLIENT_BUILD_MIN 26124
#define LEGIONFORGE_CLIENT_BUILD_MAX 26124
#define LEGIONFORGE_CLIENT_VERSION  "7.3.5"

// DB2 / Hotfixes / BattlePay опкоды соответствуют строго этому билду.
#define LEGIONFORGE_DB2_BUILD       26124
#define LEGIONFORGE_HOTFIX_BUILD    26124
#define LEGIONFORGE_BATTLEPAY_BUILD 26124

// ---------------------------------------------------------------------------
// Экономические константы платформы (дублируются в LegionForge_Config.h,
// но здесь они нужны для кода ядра, не связанного со скриптами).
// ---------------------------------------------------------------------------
#define LEGIONFORGE_CURRENCY_ESSENCE 1533   // Сущность пробуждения (Wakening Essence)

// ---------------------------------------------------------------------------
// Страховка: если кто-то попытается пересобрать ядро под другой билд —
// компилятор остановит сборку внятным сообщением.
// ---------------------------------------------------------------------------
static_assert(LEGIONFORGE_CLIENT_BUILD == 26124,
    "LEGIONFORGE supports ONLY WoW 7.3.5 client build 26124. "
    "Do not change LEGIONFORGE_CLIENT_BUILD.");

#endif // LEGIONFORGE_VERSION_H
