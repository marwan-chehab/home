CREATE TABLE `device_control` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `device_type` ENUM('burner', 'pump', 'heater') NOT NULL,
  `status` ENUM('on', 'off') NOT NULL,
  `stamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  INDEX (`stamp`),
  INDEX (`device_type`)  -- Helps with filtering by device type
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
