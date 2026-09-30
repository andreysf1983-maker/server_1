-- ============================================================================
--  LEGIONFORGE :: custom_legionforge_characters.sql   (база characters)
--  Персональные данные модулей: Престиж, Hardcore, статистика Сущности
-- ============================================================================
SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS `custom_legionforge_player` (
  `Guid` int(10) unsigned NOT NULL,
  `PrestigeRank` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Hardcore` tinyint(1) NOT NULL DEFAULT 0,
  `HardcoreDeaths` int(10) unsigned NOT NULL DEFAULT 0,
  `TotalEssence` bigint(20) unsigned NOT NULL DEFAULT 0,
  `OnlineHours` int(10) unsigned NOT NULL DEFAULT 0,
  `ProfSlots` tinyint(3) unsigned NOT NULL DEFAULT 2,
  `RacialSwapped` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `UpdatedAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`Guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Журнал выдачи Сущности пробуждения (для веб-панели и анти-инфляции)
CREATE TABLE IF NOT EXISTS `custom_legionforge_essence_log` (
  `Id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Guid` int(10) unsigned NOT NULL,
  `Amount` int(11) NOT NULL,
  `Reason` varchar(64) NOT NULL DEFAULT '',
  `CreatedAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`Id`),
  KEY `k_guid` (`Guid`),
  KEY `k_created` (`CreatedAt`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
