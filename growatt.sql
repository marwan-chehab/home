DROP TABLE IF EXISTS `growatt`;

CREATE TABLE `growatt` (
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `inverter_id` INT(11) NOT NULL,
  `timestamp` DATETIME DEFAULT CURRENT_TIMESTAMP,

  -- Voltage & Power
  `battery_voltage` FLOAT DEFAULT NULL,
  `pv1_voltage` FLOAT DEFAULT NULL,
  `pv2_voltage` FLOAT DEFAULT NULL,
  `bus_voltage` FLOAT DEFAULT NULL,
  `grid_voltage` FLOAT DEFAULT NULL,
  `output_voltage` FLOAT DEFAULT NULL,
  `dc_output_voltage` FLOAT DEFAULT NULL,

  -- Current
  `output_current` FLOAT DEFAULT NULL,
  `inverter_current` FLOAT DEFAULT NULL,
  `ac_charge_current` FLOAT DEFAULT NULL,
  `solar_buck1_current` FLOAT DEFAULT NULL,
  `solar_buck2_current` FLOAT DEFAULT NULL,
  `total_solar_charge_current` FLOAT DEFAULT NULL,

  -- Frequency
  `line_frequency` FLOAT DEFAULT NULL,
  `output_frequency` FLOAT DEFAULT NULL,

  -- Temperature
  `inverter_temp` FLOAT DEFAULT NULL,
  `dcdc_temp` FLOAT DEFAULT NULL,
  `buck1_temp` FLOAT DEFAULT NULL,
  `buck2_temp` FLOAT DEFAULT NULL,

  -- Fan & Load
  `fan_speed_1` INT DEFAULT NULL,
  `fan_speed_2` INT DEFAULT NULL,
  `load_percent` FLOAT DEFAULT NULL,

  -- PV Input Power
  `pv_input_power_high` FLOAT DEFAULT NULL,

  -- Battery Info
  `battery_soc` INT DEFAULT NULL,
  `battery_port_voltage` FLOAT DEFAULT NULL,
  `battery_bus_voltage` FLOAT DEFAULT NULL,
  `battery_watt_charge` FLOAT DEFAULT NULL,
  `battery_watt_discharge` FLOAT DEFAULT NULL,

  -- Misc
  `work_time_low` FLOAT DEFAULT NULL,
  `work_time_high` FLOAT DEFAULT NULL,
  `device_type_code` INT DEFAULT NULL,
  `system_status` INT DEFAULT NULL,
  `fault_bit` INT DEFAULT NULL,
  `warning_bit` INT DEFAULT NULL,

  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
