#include <ModbusMaster.h>
#include "BluetoothSerial.h"

// MAX485 control pins
#define MAX485_1_DE_RE 4
#define MAX485_2_DE_RE 5

// UART pins
#define RXD2 16  // Master RX
#define TXD2 17  // Master TX
#define RXD1 13  // Slave RX
#define TXD1 14  // Slave TX

BluetoothSerial SerialBT;

ModbusMaster node1;  // Inverter 1 (Master)
ModbusMaster node2;  // Inverter 2 (Slave)

// RS485 direction control
void preTransmission1() { digitalWrite(MAX485_1_DE_RE, HIGH); }
void postTransmission1() { digitalWrite(MAX485_1_DE_RE, LOW); }
void preTransmission2() { digitalWrite(MAX485_2_DE_RE, HIGH); }
void postTransmission2() { digitalWrite(MAX485_2_DE_RE, LOW); }

void setup() {
  Serial.begin(115200);
  SerialBT.begin("GrowattReader");

  // Serial ports for RS485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // Inverter 1
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);  // Inverter 2

  pinMode(MAX485_1_DE_RE, OUTPUT); digitalWrite(MAX485_1_DE_RE, LOW);
  pinMode(MAX485_2_DE_RE, OUTPUT); digitalWrite(MAX485_2_DE_RE, LOW);

  // Inverter 1
  node1.begin(1, Serial2);
  node1.preTransmission(preTransmission1);
  node1.postTransmission(postTransmission1);

  // Inverter 2
  node2.begin(2, Serial1);
  node2.preTransmission(preTransmission2);
  node2.postTransmission(postTransmission2);

  SerialBT.println("✅ Bluetooth started. Reading both inverters...");
}

void readInverter(ModbusMaster &node, uint8_t id, float &outputCurrent, float &invCurrent) {
  SerialBT.printf("🔷 Inverter ID %u\n", id);
  uint8_t result;

  // Battery Voltage
  result = node.readInputRegisters(17, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("Battery Voltage: %.2f V\n", node.getResponseBuffer(0) * 0.01);
  else
    SerialBT.println("❌ Failed to read register 17 (Battery Voltage)");

  // Battery SOC
  result = node.readInputRegisters(18, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("Battery SOC: %u %%\n", node.getResponseBuffer(0));
  else
    SerialBT.println("❌ Failed to read register 18 (Battery SOC)");

  // Output Current
  result = node.readInputRegisters(34, 1);
  if (result == node.ku8MBSuccess) {
    outputCurrent = node.getResponseBuffer(0) * 0.1;
    SerialBT.printf("Output Current: %.1f A\n", outputCurrent);
  } else {
    outputCurrent = 0;
    SerialBT.println("❌ Failed to read register 34 (Output Current)");
  }

  // Inverter Current
  result = node.readInputRegisters(35, 1);
  if (result == node.ku8MBSuccess) {
    invCurrent = node.getResponseBuffer(0) * 0.1;
    SerialBT.printf("Inverter Current: %.1f A\n", invCurrent);
  } else {
    invCurrent = 0;
    SerialBT.println("❌ Failed to read register 35 (Inverter Current)");
  }

  // Inverter Temp
  result = node.readInputRegisters(25, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("Inverter Temp: %.1f °C\n", node.getResponseBuffer(0) * 0.1);
  else
    SerialBT.println("❌ Failed to read register 25 (Inverter Temp)");

  // Fan Speeds
  result = node.readInputRegisters(81, 2);
  if (result == node.ku8MBSuccess) {
    SerialBT.printf("Fan Speed 1: %u %%\n", node.getResponseBuffer(0));
    SerialBT.printf("Fan Speed 2: %u %%\n", node.getResponseBuffer(1));
  } else {
    SerialBT.println("❌ Failed to read registers 81-82 (Fan Speeds)");
  }

  // PV Input Power
  result = node.readInputRegisters(3, 2);
  if (result == node.ku8MBSuccess) {
    uint32_t powerRaw = ((uint32_t)node.getResponseBuffer(0) << 16) | node.getResponseBuffer(1);
    SerialBT.printf("PV Input Power: %.1f W\n", powerRaw * 0.1);
  } else {
    SerialBT.println("❌ Failed to read registers 3-4 (PV Input Power)");
  }

  // Grid Voltage
  result = node.readInputRegisters(20, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("Grid Voltage: %.1f V\n", node.getResponseBuffer(0) * 0.1);
  else
    SerialBT.println("❌ Failed to read register 20 (Grid Voltage)");

  // Line Frequency
  result = node.readInputRegisters(21, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("Line Frequency: %.2f Hz\n", node.getResponseBuffer(0) * 0.01);
  else
    SerialBT.println("❌ Failed to read register 21 (Line Frequency)");

  // Output Voltage
  result = node.readInputRegisters(22, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("Output Voltage: %.1f V\n", node.getResponseBuffer(0) * 0.1);
  else
    SerialBT.println("❌ Failed to read register 22 (Output Voltage)");

  // Output Frequency
  result = node.readInputRegisters(23, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("Output Frequency: %.2f Hz\n", node.getResponseBuffer(0) * 0.01);
  else
    SerialBT.println("❌ Failed to read register 23 (Output Frequency)");

  // AC Charge Current
  result = node.readInputRegisters(68, 1);
  if (result == node.ku8MBSuccess)
    SerialBT.printf("AC Charge Current: %.1f A\n", node.getResponseBuffer(0) * 0.1);
  else
    SerialBT.println("❌ Failed to read register 68 (AC Charge Current)");

  // Solar Charge Currents (186 + 187)
  result = node.readInputRegisters(186, 2);
  if (result == node.ku8MBSuccess) {
    float buck1 = node.getResponseBuffer(0) * 0.1;
    float buck2 = node.getResponseBuffer(1) * 0.1;
    SerialBT.printf("Solar Buck1 Current: %.1f A\n", buck1);
    SerialBT.printf("Solar Buck2 Current: %.1f A\n", buck2);
    SerialBT.printf("Total Solar Charge Current: %.1f A\n", buck1 + buck2);
  } else {
    SerialBT.println("❌ Failed to read registers 186-187 (Solar Charge Currents)");
  }

  SerialBT.println("------------------------------");
}

void loop() {
  float outputCurrent1 = 0, invCurrent1 = 0;
  float outputCurrent2 = 0, invCurrent2 = 0;

  readInverter(node1, 1, outputCurrent1, invCurrent1);
  delay(3000);
  readInverter(node2, 2, outputCurrent2, invCurrent2);
  delay(3000);

  float totalOut = outputCurrent1 + outputCurrent2;
  float totalInv = invCurrent1 + invCurrent2;

  SerialBT.printf("🔆 Total Output Current: %.1f A\n", totalOut);
  SerialBT.printf("🔆 Total Inverter Current: %.1f A\n", totalInv);
  SerialBT.println("==================================\n");

  delay(5000);
}
