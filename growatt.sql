CREATE TABLE growatt (
  id INT PRIMARY KEY AUTO_INCREMENT,
  inverter_id INT NOT NULL,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

  battery_voltage FLOAT,
  battery_soc INT,
  output_current FLOAT,
  inverter_current FLOAT,
  inverter_temp FLOAT,
  fan_speed_1 INT,
  fan_speed_2 INT,
  pv_input_power FLOAT,
  grid_voltage FLOAT,
  line_frequency FLOAT,
  output_voltage FLOAT,
  output_frequency FLOAT,
  ac_charge_current FLOAT,
  solar_buck1_current FLOAT,
  solar_buck2_current FLOAT,
  total_solar_charge_current FLOAT
);
