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

class BusinessLogicHandler {
public:
    // Constructor
    BusinessLogicHandler();

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
    void handleAuto(const String& state);
    
    void updateGPS();
    void updateScheduling();
    // Hardware components
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    // GPS data
    float gpsLatitude;
    float gpsLongitude;

    SettingsData settings;
    PowerMeterData powerMeterData;
    
    // State variables
    bool deviceState;  // ON/OFF state
    bool isAuto;       // Auto mode state
    
    // Time variables
    RTCDateTime DayTime;
    // Other private variables and methods...
};

#endif