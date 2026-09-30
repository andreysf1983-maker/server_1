-- ============================================================================
--  LEGIONFORGE :: custom_legionforge_auth.sql   (база auth)
--  Жёсткая привязка реалма к клиенту 7.3.5 Build 26124
-- ============================================================================
SET NAMES utf8mb4;

-- Реалм: имя, адреса, порты и ОБЯЗАТЕЛЬНО gamebuild = 26124
UPDATE `realmlist` SET
  `name` = 'LEGIONFORGE | 7.3.5 | x1 | Трансмог-свобода',
  `gamebuild` = 26124,
  `flag` = 2,
  `timezone` = 1,
  `allowedSecurityLevel` = 0,
  `population` = 0.0
WHERE `id` = 1;

-- Если таблица пуста (свежая установка) — создаём реалм заново
INSERT IGNORE INTO `realmlist`
  (`id`, `name`, `address`, `localAddress`, `localSubnetMask`, `port`, `gamePort`, `icon`, `flag`,
   `timezone`, `allowedSecurityLevel`, `population`, `gamebuild`, `Region`, `Battlegroup`)
VALUES
  (1, 'LEGIONFORGE | 7.3.5 | x1 | Трансмог-свобода', '127.0.0.1', '127.0.0.1', '255.255.255.0', 1119, 8085,
   0, 2, 1, 0, 0.0, 26124, 1, 1);

-- Контроль: результат должен быть 26124
-- SELECT id, name, gamebuild FROM realmlist;
