// ESP32 Growatt Reader - Full Register Reader with Updated Registers
#include <ModbusMaster.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define MAX485_1_DE_RE 4
#define MAX485_2_DE_RE 5
#define RXD2 16
#define TXD2 17
#define RXD1 13
#define TXD1 14

BluetoothSerial SerialBT;
Preferences preferences;
ModbusMaster node1;
ModbusMaster node2;

String ssid, password, serverUrl;

void preTransmission1() { digitalWrite(MAX485_1_DE_RE, HIGH); }
void postTransmission1() { digitalWrite(MAX485_1_DE_RE, LOW); }
void preTransmission2() { digitalWrite(MAX485_2_DE_RE, HIGH); }
void postTransmission2() { digitalWrite(MAX485_2_DE_RE, LOW); }

struct RegisterDef {
  uint16_t address;
  const char* field;
  float scale;
  uint8_t length;
};

// Precise Input Register List
RegisterDef inputRegs[] = {
	{0, "system_status", 1.0f, 1},				// System run state (0-12)
	{1, "pv1_voltage", 0.1f, 1},    			// PV1 voltage
	{2, "pv2_voltage", 0.1f, 1},				// PV2 voltage
	{3, "pv1_charge_power", 0.1f, 2},   // PV1 charge power (32-bit, high+low)
	{5, "pv2_charge_power", 0.1f, 2},   // PV2 charge power (32-bit, high+low)
	{7, "buck1_current", 0.1f, 1},				// Buck1 current
	{8, "buck2_current", 0.1f, 1},				// Buck2 current
	{9,  "output_active_power",        0.1f, 2}, // Output active power (32-bit, W)
	{11, "output_apparent_power",      0.1f, 2}, // Output apparent power (32-bit, VA)
	{13, "ac_charge_power",            0.1f, 2}, // AC charge watt (32-bit, W)
	{15, "ac_charge_apparent_power",   0.1f, 2}, // AC charge apparent power (32-bit, VA)
	{17, "battery_voltage", 0.01f, 1},            // Battery voltage (M3)
	{18, "battery_soc", 1.0f, 1},                 // Battery state of charge (0–100)
	{19, "bus_voltage", 0.1f, 1},                 // Bus voltage
	{20, "grid_voltage", 0.1f, 1},                // AC input voltage
	{21, "line_frequency", 0.01f, 1},             // AC input frequency
	{22, "output_voltage", 0.1f, 1},              // AC output voltage
	{23, "output_frequency", 0.01f, 1},           // AC output frequency
	{24, "dc_output_voltage", 0.1f, 1},            // Output DC voltage
	{25, "inverter_temp", 0.1f, 1},                // Inverter temperature
	{26, "dcdc_temp", 0.1f, 1},                    // DC-DC temperature
	{27, "load_percent", 0.1f, 1},                 // Load percent (0–1000)
	{28, "battery_port_voltage", 0.01f, 1},        // Battery-port volt (DSP)
	{29, "battery_bus_voltage", 0.01f, 1},         // Battery-bus volt (DSP)
	{30, "work_time_total", 0.5f, 2}, 			// Work time total (high + low), in 0.5s units
	{32, "buck1_temp", 0.1f, 1},                   // Buck1 temperature
	{33, "buck2_temp", 0.1f, 1},                   // Buck2 temperature
	{34, "output_current", 0.1f, 1},               // Output current
	{35, "inverter_current", 0.1f, 1},             // Inverter current
	{36, "ac_input_power", 0.1f, 2},        // AC input watt (32-bit)
	{38, "ac_input_va", 0.1f, 2},           // AC input apparent power (32-bit)
	{40, "fault_bit", 1.0f, 1},                    // Fault bit
	{41, "warning_bit", 1.0f, 1},                  // Warning bit
	{42, "fault_value", 1.0f, 1},                  // Fault value (more details elsewhere)
	{43, "warning_value", 1.0f, 1},                // Warning value (more details elsewhere)
	{44, "device_type_code", 1.0f, 1},             // Device type code (DTC)
	{45, "check_step", 1.0f, 1},                   // Product check step
	{46, "production_line_mode", 1.0f, 1},         // Production line mode
	{47, "constant_power_ok_flag", 1.0f, 1},       // Constant power OK flag
	{48, "pv1_energy_today", 0.1f, 2},   // Combined from 48 (H) and 49 (L)
	{50, "pv1_energy_total", 0.1f, 2},   // 50 + 51
	{52, "pv2_energy_today", 0.1f, 2},   // 52 + 53
	{54, "pv2_energy_total", 0.1f, 2},   // 54 + 55
	{56, "ac_charge_energy_today", 0.1f, 2},     // AC charge energy today (kWh)
	{58, "ac_charge_energy_total", 0.1f, 2},     // AC charge energy total (kWh)
	{60, "bat_discharge_energy_today", 0.1f, 2}, // Battery discharge energy today (kWh)
	{62, "bat_discharge_energy_total", 0.1f, 2}, // Battery discharge energy total (kWh)
	{64, "ac_discharge_energy_today", 0.1f, 2},  // AC discharge energy today (kWh)
	{66, "ac_discharge_energy_total", 0.1f, 2},  // AC discharge energy total (kWh)
	{68, "ac_charge_current", 0.1f, 1},              // AC charge battery current
	{69, "ac_discharge_watt", 0.1f, 2},   // AC discharge watt (W)
	{71, "ac_discharge_va", 0.1f, 2},     // AC discharge apparent power (VA)
	{73, "bat_discharge_watt", 0.1f, 2},  // Battery discharge watt (W)
	{75, "bat_discharge_va", 0.1f, 2},    // Battery discharge apparent power (VA)
	{77, "battery_watt", 0.1f, 2},              // Battery watt (signed int 32) (net charge/discharge)
	{80, "bat_overcharge_flag", 1.0f, 1}, 			// Battery Over Charge Flag (0: Not over, 1: Over)
	{81,  "mppt_fan_speed", 1.0f, 1},               // Fan speed of MPPT Charger (%)
	{82,  "inv_fan_speed", 1.0f, 1},                // Fan speed of Inverter (%)
	
	// BMS (Battery Management System) Registers
	{90,  "bms_status", 1.0f, 1},                   // Status from BMS
	{91,  "bms_error", 1.0f, 1},                    // Error info from BMS
	{92,  "bms_warn_info", 1.0f, 1},                // Warning info from BMS
	{93,  "bms_soc", 1.0f, 1},                      // SOC from BMS
	{94,  "bms_battery_volt", 0.1f, 1},             // Battery voltage from BMS
	{95,  "bms_battery_curr", 0.1f, 1},             // Battery current from BMS
	{96,  "bms_battery_temp", 0.1f, 1},             // Battery temperature from BMS
	{97,  "bms_max_curr", 0.1f, 1},                 // Max charge/discharge current
	{98,  "bms_constant_volt", 0.1f, 1},            // CV voltage from BMS
	{99,  "bms_info", 1.0f, 1},                     // BMS Information
	{100, "bms_pack_info", 1.0f, 1},                // Pack Information
	{101, "bms_using_cap", 1.0f, 1},                // Using Cap info
	{102, "bms_cell1_volt", 0.01f, 1},              // Cell1 Voltage from BMS
	{117, "bms_cell16_volt", 0.01f, 1},            // Cell16 Voltage from BMS2
	
	// BMS2 (Secondary Battery System)
	{118, "bms2_status", 1.0f, 1},                  // Status from BMS2
	{119, "bms2_error", 1.0f, 1},                   // Error from BMS2
	{120, "bms2_warn_info", 1.0f, 1},               // Warning from BMS2
	{121, "bms2_soc", 1.0f, 1},                     // SOC from BMS2
	{122, "bms2_battery_volt", 0.1f, 1},            // Voltage from BMS2
	{123, "bms2_battery_curr", 0.1f, 1},            // Current from BMS2
	{124, "bms2_battery_temp", 0.1f, 1},            // Temp from BMS2
	{125, "bms2_max_curr", 0.1f, 1},                // Max current from BMS2
	{126, "bms2_constant_volt", 0.1f, 1},           // CV from BMS2
	{127, "bms2_info", 1.0f, 1},                    // Info from BMS2
	{128, "bms2_pack_info", 1.0f, 1},               // Pack Info from BMS2
	{129, "bms2_using_cap", 1.0f, 1},               // Cap Usage from BMS2
	{130, "bms2_cell1_volt", 0.01f, 1},             // Cell1 Voltage from BMS2
	{145, "bms2_cell16_volt", 0.01f, 1},            // Cell16 Voltage from BMS2
	{180, "solar1_status", 1.0f, 1},         // Solar Charger1 Status
	{181, "solar1_fault_code", 1.0f, 1},     // Solar Charger1 Fault Code
	{182, "solar1_warning_code", 1.0f, 1},   // Solar Charger1 Warning Code
	{183, "solar1_bat_voltage", 0.01f, 1},
	{184, "solar1_pv1_voltage", 0.1f, 1},
	{185, "solar1_pv2_voltage", 0.1f, 1},
	{186, "solar1_buck1_current", 0.1f, 1},
	{187, "solar1_buck2_current", 0.1f, 1},
	{188, "solar1_pv1_charge_power", 0.1f, 2},  // Solar Charger1 PV1 charge power (32-bit: High + Low)
	{190, "solar1_pv2_charge_power", 0.1f, 2},  // Solar Charger1 PV2 charge power (32-bit: High + Low)
	{192, "solar1_hs1_temp", 0.1f, 1},
	{193, "solar1_hs2_temp", 0.1f, 1},
	{194, "solar1_epv1_today", 0.1f, 1},  // Solar Charger1 PV1 Energy kWh today
	{195, "solar1_epv2_today", 0.1f, 1},  // Solar Charger1 PV2 Energy kWh today
	{196, "solar1_epv1_total", 0.1f, 2},  // Solar Charger1 PV1 Energy total (32-bit)
	{198, "solar1_epv2_total", 0.1f, 2},  // Solar Charger1 PV2 Energy total (32-bit)
	{200, "solar2_status", 1.0f, 1},
	{201, "solar2_fault_code", 1.0f, 1},
	{202, "solar2_warning_code", 1.0f, 1},
	{203, "solar2_bat_voltage", 0.01f, 1},
	{204, "solar2_pv1_voltage", 0.1f, 1},
	{205, "solar2_pv2_voltage", 0.1f, 1},
	{206, "solar2_buck1_current", 0.1f, 1},
	{207, "solar2_buck2_current", 0.1f, 1},
	{208, "solar2_pv1_charge_power", 0.1f, 2},  // Solar Charger2 PV1 charge power (32-bit)
	{210, "solar2_pv2_charge_power", 0.1f, 2},  // Solar Charger2 PV2 charge power (32-bit)
	{212, "solar2_hs1_temp", 0.1f, 1},
	{213, "solar2_hs2_temp", 0.1f, 1},
	{214, "solar2_epv1_today", 0.1f, 1},
	{215, "solar2_epv2_today", 0.1f, 1},
	{216, "solar2_epv1_total", 0.1f, 2},  // Solar Charger2 PV1 Energy total (32-bit)
	{218, "solar2_epv2_total", 0.1f, 2},  // Solar Charger2 PV2 Energy total (32-bit)
	{220, "solar_connect_ok_flag", 1.0f, 1},
	{221, "solar_batvolt_consist_flag", 1.0f, 1},
	{222, "solar_type_switch_state", 1.0f, 1},
	{223, "solar_mode_switch_state", 1.0f, 1},
	{224, "solar_address_switch_state", 1.0f, 1},
	{360, "bms_gauge_rm", 1.0f, 1},
	{361, "bms_gauge_fcc", 1.0f, 1},
	{362, "bms_fw", 1.0f, 1},
	{363, "bms_delta_volt", 1.0f, 1},
	{364, "bms_cycle_count", 1.0f, 1},
	{365, "bms_soh", 1.0f, 1},
	{366, "bms_ic_current", 1.0f, 1},
	{367, "bms_mcu_version", 1.0f, 1},
	{368, "bms_gauge_version", 1.0f, 1},
	{369, "bms_fr_version", 1.0f, 2},     // BMS Gauge FR Version (32-bit, L+H)
	{371, "bms2_gauge_rm", 1.0f, 1},
	{372, "bms2_gauge_fcc", 1.0f, 1},
	{373, "bms2_fw", 1.0f, 1},
	{374, "bms2_delta_volt", 1.0f, 1},
	{375, "bms2_cycle_count", 1.0f, 1},
	{376, "bms2_soh", 1.0f, 1},
	{377, "bms2_ic_current", 1.0f, 1},
	{378, "bms2_mcu_version", 1.0f, 1},
	{379, "bms2_gauge_version", 1.0f, 1},
	{380, "bms2_fr_version", 1.0f, 2},    // BMS2 Gauge FR Version (32-bit, L+H)
};

void setup() {
  Serial.begin(115200);
  SerialBT.begin("GrowattReader");

  preferences.begin("growatt", false);
  ssid = preferences.getString("ssid", "Repeater");
  password = preferences.getString("password", "carrotcake");
  serverUrl = preferences.getString("serverUrl", "http://35.181.4.56:5000/growatt");

  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected.");

  pinMode(MAX485_1_DE_RE, OUTPUT); digitalWrite(MAX485_1_DE_RE, LOW);
  pinMode(MAX485_2_DE_RE, OUTPUT); digitalWrite(MAX485_2_DE_RE, LOW);

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);

  node1.begin(1, Serial2);
  node1.preTransmission(preTransmission1);
  node1.postTransmission(postTransmission1);

  node2.begin(2, Serial1);
  node2.preTransmission(preTransmission2);
  node2.postTransmission(postTransmission2);

  SerialBT.println("✅ Bluetooth started. Reading full register set...");
}

void readAndSend(ModbusMaster &node, uint8_t id) {
  StaticJsonDocument<8192> doc;
  doc["inverter_id"] = id;

  for (RegisterDef &reg : inputRegs) {
    float value = 0;
    bool success = false;
    for (int attempt = 0; attempt < 3 && !success; ++attempt) {
      uint8_t result = node.readInputRegisters(reg.address, reg.length);
      delay(50);
      if (result == node.ku8MBSuccess) {
        if (reg.length == 1) {
          value = node.getResponseBuffer(0) * reg.scale;
        } else {
          uint32_t raw = 0;
          for (int i = 0; i < reg.length; i++) {
            raw = (raw << 16) | node.getResponseBuffer(i);
          }
          value = raw * reg.scale;
        }
        doc[reg.field] = value;
        SerialBT.printf("%s: %.2f\n", reg.field, value);
        success = true;
      }
    }
    if (!success) {
      SerialBT.printf("❌ Failed to read %s (reg %u)\n", reg.field, reg.address);
    }
  }

  SerialBT.println("------------------------------");
  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  http.end();
  Serial.printf("[Inverter %d] POST code: %d\n", id, code);
}

void loop() {
  readAndSend(node1, 1);
  delay(3000);
  readAndSend(node2, 2);
  delay(7000);
}
