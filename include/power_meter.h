#include <Arduino.h>
#include "WiFi.h"
#include "Modbus.h"
#include "types.h"

// Initialize Modbus communication
void power_meter_begin() {
  modbus.init();
  // Initialize other required variables if needed
}

// Function to read data from the power meter
bool power_meter_read(PowerMeterData &data) {
  static unsigned long timer;
  static byte mun_erro;

  // Timing logic to control read frequency
  if (millis() % 30000ul > 15000ul) return false;
  if (millis() < timer)             return false;
  timer = millis() + 100;

  // Check and configure Serial2 if necessary
  if (Serial2_Using != RS485_SERIAL) {
    Serial2.end();
    while (Serial2.available()) Serial2.read();
    delay(100);
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial2_Using = RS485_SERIAL;
  } else {
    // Request data from the Modbus device
    if (modbus.requestFrom(0x01, 0x04, 0x00, 60) > 0) {
      double voltage = modbus.uint16(0) / 10.0;
      if (voltage > 0) {
        // Populate the data structure with readings
        data.total_energy         = modbus.uint32(29) / 100.0;
        data.total_energy_reverse = modbus.uint32(39) / 100.0;
        data.total_energy_forward = modbus.uint32(49) / 100.0;
        data.voltage              = voltage;
        data.current              = modbus.int16(3)  / 100.0;
        data.power                = modbus.int16(8)  / 1.0;
        data.power_factor         = modbus.int16(20) / 1000.0;
        data.frequency            = modbus.int16(26) / 100.0;

        timer = millis() + 1000;
        mun_erro = 0;
        return true;  // Data read successfully
      } else {
        // Invalid voltage reading
        return false;
      }
    } else if (mun_erro > 100) {
      // Exceeded error threshold
      mun_erro++;
      return false;
    } else {
      mun_erro++;
      return false;
    }
  }
  return false;  // Default to false if conditions are not met
}

// Main loop function to be called repeatedly
void power_meter_loop() {
  PowerMeterData data;
  if (power_meter_read(data)) {
    // Use the data as needed
    // For example, print the readings
    Serial.println("Power Meter Readings:");
    Serial.print("Total Energy: "); Serial.println(data.total_energy);
    Serial.print("Voltage: "); Serial.println(data.voltage);
    Serial.print("Current: "); Serial.println(data.current);
    Serial.print("Power: "); Serial.println(data.power);
    Serial.print("Power Factor: "); Serial.println(data.power_factor);
    Serial.print("Frequency: "); Serial.println(data.frequency);
    // Add more processing as needed
  } else {
    // Handle the case when data is not available
    // For example, log an error or retry
  }
}