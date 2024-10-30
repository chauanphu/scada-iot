#ifndef BUSINESSLOGICHANDLER_H
#define BUSINESSLOGICHANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <LiquidCrystal.h>
#include "types.h"
#include "ESP32LCD.h"

// #include "TinyGPS.h"
// #include "Modbus.h"

// #include "TinyGPS.h"
// #include "Modbus.h"
// #include "button.h"
// #include "printLCD.h"

class BusinessLogicHandler {
public:
    // Constructor
    BusinessLogicHandler(PubSubClient& client, const String& mac);

    // Public methods
    void handleCommand(const String& command);
    String getStatus();
    String isAlive;
    ESP32LCD deviceLCD;
    void update(); // Method to be called in main loop
    void initializeDevices(); // Device initialization

private:
    // Private methods for internal logic
    void handleToggle(const String& state);
    void handleSchedule(int hourOn, int minuteOn, int hourOff, int minuteOff);
    void updateGPS();
    void updateScheduling();
    void updateButtons();
    // Hardware components
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    // TinyGPS gps;
    // Modbus modbus;
    // Button buttonUp;
    // Button buttonDn;
    // Button buttonOk;

    // GPS data
    float gpsLatitude;
    float gpsLongitude;

    // Member variables
    PubSubClient& mqttClient;
    String macAddress;
    String statusTopic;
    String commandTopic;
    SettingsData settings;
    PowerMeterData powerMeterData;
    
    // State variables
    bool deviceState;  // ON/OFF state

    // Time variables
    RTCDateTime DayTime;
    // Other private variables and methods...
};

#endif