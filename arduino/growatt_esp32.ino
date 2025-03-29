// ESP32 Growatt Reader - Full Register Reader with Retries and Delays
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

// Extensive Input Register List from Modbus Manual (Pages 13–19)
RegisterDef inputRegs[] = {
  {0, "system_status", 1.0f, 1},
  {1, "pv1_voltage", 0.1f, 1},
  {2, "pv2_voltage", 0.1f, 1},
  {3, "pv_input_power_high", 1.0f, 2},
  {17, "battery_voltage", 0.01f, 1},
  {18, "battery_soc", 1.0f, 1},
  {19, "bus_voltage", 0.1f, 1},
  {20, "grid_voltage", 0.1f, 1},
  {21, "line_frequency", 0.01f, 1},
  {22, "output_voltage", 0.1f, 1},
  {23, "output_frequency", 0.01f, 1},
  {24, "dc_output_voltage", 0.1f, 1},
  {25, "inverter_temp", 0.1f, 1},
  {26, "dcdc_temp", 0.1f, 1},
  {27, "load_percent", 1.0f, 1},
  {28, "battery_port_voltage", 0.1f, 1},
  {29, "battery_bus_voltage", 0.1f, 1},
  {30, "work_time_low", 1.0f, 1},
  {31, "work_time_high", 1.0f, 1},
  {32, "buck1_temp", 0.1f, 1},
  {33, "buck2_temp", 0.1f, 1},
  {34, "output_current", 0.1f, 1},
  {35, "inverter_current", 0.1f, 1},
  {36, "ac_input_watt_high", 1.0f, 2},
  {40, "fault_bit", 1.0f, 1},
  {41, "warning_bit", 1.0f, 1},
  {44, "device_type_code", 1.0f, 1},
  {68, "ac_charge_current", 0.1f, 1},
  {77, "battery_watt_charge", 1.0f, 1},
  {78, "battery_watt_discharge", 1.0f, 1},
  {81, "fan_speed_1", 1.0f, 1},
  {82, "fan_speed_2", 1.0f, 1},
  {186, "solar_buck1_current", 0.1f, 1},
  {187, "solar_buck2_current", 0.1f, 1},
  {188, "total_solar_charge_current", 0.1f, 2},
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
  StaticJsonDocument<2048> doc;
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
