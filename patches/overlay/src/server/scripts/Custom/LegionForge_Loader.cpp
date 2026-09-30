// ============================================================================
//  LEGIONFORGE — Master Custom Script Loader
//  Target: LegionForgeCore 7.3.5 · Client Build 7.3.5.26124
// ============================================================================
//  Вызывается из ScriptLoader.cpp :: AddCustomScripts().
//  Все кастомные системы платформы регистрируются здесь — добавление нового
//  модуля = один файл в src/server/scripts/Custom/ + одна строка ниже.
// ============================================================================

// --- Базовые системы LEGIONFORGE (экономика, апгрейд, магазин, NPC) --------
void AddSC_LegionForge_ItemTome();
void AddSC_LegionForge_ItemUpgrade();
void AddSC_LegionForge_VendorNPC();
void AddSC_LegionForge_OnlineReward();
void AddSC_LegionForge_BrokenQuests();
void AddSC_LegionForge_WorldBoss();
void AddSC_LegionForge_PlayerBots();
void AddSC_LegionForge_CatalogMods();

// --- Расширенные модули платформы (v3.1) -----------------------------------
void AddSC_LegionForge_TransmogFreedom();     // свободная трансмогрификация
void AddSC_LegionForge_LegacySpells();        // фолианты забытых способностей
void AddSC_LegionForge_Crossfaction();        // межфракционная игра
void AddSC_LegionForge_QoL();                 // Mythic+ QoL / Duel Reset / MultiProf / RacialSwap
void AddSC_LegionForge_Prestige();            // престиж и Hardcore
void AddSC_LegionForge_WorldBosses();         // 100 кастомных мировых боссов
void AddSC_LegionForge_Bots();                // умные ИИ-боты и живой чат
void AddSC_LegionForge_BattlePayEssence();    // магазин кнопки «W» за Сущность

void AddSC_LegionForge_Custom()
{
    // Базовые системы
    AddSC_LegionForge_ItemTome();
    AddSC_LegionForge_ItemUpgrade();
    AddSC_LegionForge_VendorNPC();
    AddSC_LegionForge_OnlineReward();
    AddSC_LegionForge_BrokenQuests();
    AddSC_LegionForge_WorldBoss();
    AddSC_LegionForge_PlayerBots();
    AddSC_LegionForge_CatalogMods();

    // Расширенные модули v3.1
    AddSC_LegionForge_TransmogFreedom();
    AddSC_LegionForge_LegacySpells();
    AddSC_LegionForge_Crossfaction();
    AddSC_LegionForge_QoL();
    AddSC_LegionForge_Prestige();
    AddSC_LegionForge_WorldBosses();
    AddSC_LegionForge_Bots();
    AddSC_LegionForge_BattlePayEssence();
}
