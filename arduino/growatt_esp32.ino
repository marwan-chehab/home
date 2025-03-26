// ESP32 Growatt Reader + WiFi POST to Flask API
#include <ModbusMaster.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// MAX485 control pins
#define MAX485_1_DE_RE 4
#define MAX485_2_DE_RE 5

// UART pins for RS485
#define RXD2 16  // Master RX
#define TXD2 17  // Master TX
#define RXD1 13  // Slave RX
#define TXD1 14  // Slave TX

BluetoothSerial SerialBT;
Preferences preferences;
ModbusMaster node1;
ModbusMaster node2;

String ssid, password, serverUrl;

void preTransmission1() { digitalWrite(MAX485_1_DE_RE, HIGH); }
void postTransmission1() { digitalWrite(MAX485_1_DE_RE, LOW); }
void preTransmission2() { digitalWrite(MAX485_2_DE_RE, HIGH); }
void postTransmission2() { digitalWrite(MAX485_2_DE_RE, LOW); }

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

  SerialBT.println("✅ Bluetooth started. Reading both inverters...");
}

void readAndSend(ModbusMaster &node, uint8_t id, float &outputCurrent, float &invCurrent) {
  uint8_t result;
  StaticJsonDocument<512> doc;
  doc["inverter_id"] = id;

  SerialBT.printf("🔷 Inverter ID %u\n", id);

  auto tryRead = [&](uint16_t reg, const char* name, float scale = 1.0f) {
    for (int attempt = 0; attempt < 3; attempt++) {
      result = node.readInputRegisters(reg, 1);
      delay(50);
      if (result == node.ku8MBSuccess) {
        float value = node.getResponseBuffer(0) * scale;
        doc[name] = value;
        SerialBT.printf("%s: %.2f\n", name, value);
        return value;
      }
    }
    SerialBT.printf("❌ Failed to read register %u (%s)\n", reg, name);
    return 0.0f;
  };

  doc["battery_voltage"] = tryRead(17, "Battery Voltage", 0.01f);
  doc["battery_soc"] = tryRead(18, "Battery SOC");
  outputCurrent = tryRead(34, "Output Current", 0.1f);
  doc["output_current"] = outputCurrent;
  invCurrent = tryRead(35, "Inverter Current", 0.1f);
  doc["inverter_current"] = invCurrent;
  doc["inverter_temp"] = tryRead(25, "Inverter Temp", 0.1f);
  doc["fan_speed_1"] = tryRead(81, "Fan Speed 1");
  doc["fan_speed_2"] = tryRead(82, "Fan Speed 2");

  result = node.readInputRegisters(3, 2);
  delay(50);
  if (result == node.ku8MBSuccess) {
    uint32_t pv = ((uint32_t)node.getResponseBuffer(0) << 16 | node.getResponseBuffer(1));
    doc["pv_input_power"] = pv * 0.1f;
    SerialBT.printf("PV Input Power: %.1f W\n", pv * 0.1f);
  } else {
    SerialBT.println("❌ Failed to read registers 3-4 (PV Input Power)");
  }

  doc["grid_voltage"] = tryRead(20, "Grid Voltage", 0.1f);
  doc["line_frequency"] = tryRead(21, "Line Frequency", 0.01f);
  doc["output_voltage"] = tryRead(22, "Output Voltage", 0.1f);
  doc["output_frequency"] = tryRead(23, "Output Frequency", 0.01f);
  doc["ac_charge_current"] = tryRead(68, "AC Charge Current", 0.1f);

  result = node.readInputRegisters(186, 2);
  delay(50);
  if (result == node.ku8MBSuccess) {
    float buck1 = node.getResponseBuffer(0) * 0.1f;
    float buck2 = node.getResponseBuffer(1) * 0.1f;
    doc["solar_buck1_current"] = buck1;
    doc["solar_buck2_current"] = buck2;
    doc["total_solar_charge_current"] = buck1 + buck2;
    SerialBT.printf("Solar Buck1 Current: %.1f A\n", buck1);
    SerialBT.printf("Solar Buck2 Current: %.1f A\n", buck2);
    SerialBT.printf("Total Solar Charge Current: %.1f A\n", buck1 + buck2);
  } else {
    SerialBT.println("❌ Failed to read registers 186-187 (Solar Charge Currents)");
  }

  SerialBT.println("------------------------------");

  String json;
  serializeJson(doc, json);

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(json);
  http.end();
  Serial.printf("[Inverter %d] POST code: %d\n", id, code);
}

void loop() {
  float outputCurrent1 = 0, invCurrent1 = 0;
  float outputCurrent2 = 0, invCurrent2 = 0;

  readAndSend(node1, 1, outputCurrent1, invCurrent1);
  delay(3000);
  readAndSend(node2, 2, outputCurrent2, invCurrent2);
  delay(3000);

  float totalOut = outputCurrent1 + outputCurrent2;
  float totalInv = invCurrent1 + invCurrent2;

  SerialBT.printf("🔆 Total Output Current: %.1f A\n", totalOut);
  SerialBT.printf("🔆 Total Inverter Current: %.1f A\n", totalInv);
  SerialBT.println("==================================\n");

  delay(5000);
}
