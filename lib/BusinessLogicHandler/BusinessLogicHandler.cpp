#include "BusinessLogicHandler.h"
#include <ArduinoJson.h>
#include <time.h>
#include <Arduino.h>
#include <Udp.h>
#include <LiquidCrystal.h>
#include <WiFiUdp.h>
#include "secrets.h"
#include <Preferences.h>

// Constants and definitions
#define LED_BUILTIN               27
#define LED_BUILTIN_ON_STATE      1
#define FLASH_ACTIVE_LED          digitalWrite(LED_BUILTIN, LED_BUILTIN_ON_STATE)
#define PR_LED                    18
#define OUTPUT_CRT                13
#define PWM_AUTO_RESET            33
#define RS485_SERIAL              16
#define TX_PIN                    16
#define RX_PIN                    17
#define GPS_SERIAL                25
#define GPS_TX_PIN                25
#define GPS_RX_PIN                26

byte Serial2_Using;
Preferences preferences;

// #include "NTPClient.h"
#include "TinyGPS.h"
#include "Modbus.h"
#include "button.h"
// Instantiate hardware components
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7 * 3600, 60000);  // Adjust timezone as needed
GPS_time gps;
Modbus modbus(Serial2);  // Using Serial2 for Modbus and GPS
// Button Button_UP(36, BUTTON_ANALOG, 1000, 2200);
// Button Button_DN(36, BUTTON_ANALOG, 1000, 470);
// Button Button_OK(36, BUTTON_ANALOG, 1000, 0);
LiquidCrystal glcd(15 /*rs*/, 2 /*en*/, 0/*d4*/, 4 /*d5*/, 5 /*d6*/, 19 /*d7*/);

#include "ESP32LCD.h"
#include "power_meter.h"
// #include "printLCD.h"

#include "BusinessLogicHandler.h"

// Constructor
BusinessLogicHandler::BusinessLogicHandler()
    : timeClient(ntpUDP, "europe.pool.ntp.org", 7 * 3600, 60000),
      settings({0, 0, 0, 0}),
      isAlive("1"),
      deviceLCD(DayTime, glcd),
      deviceState(false),
      gpsLatitude(0.0),
      gpsLongitude(0.0),
      isAuto(true)
      {
    // Initialize devices
    initializeDevices();
}

void BusinessLogicHandler::initializeDevices() {
    // Initialize pins
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    pinMode(PR_LED, OUTPUT);
    digitalWrite(PR_LED, HIGH);

    pinMode(OUTPUT_CRT, OUTPUT);
    digitalWrite(OUTPUT_CRT, LOW);

    pinMode(PWM_AUTO_RESET, OUTPUT);
    digitalWrite(PWM_AUTO_RESET, LOW);
    // Initialize LCD

    // Initialize NTP Client
    timeClient.begin();

    // Initialize GPS (using HardwareSerial)
    Serial1.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

    // Initialize Modbus (using HardwareSerial) 
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    power_meter_begin();

    deviceLCD.begin();

    if (preferences.begin("settings", false)) {
        settings.hour_on = preferences.getUInt("hour_on", 0);
        settings.minute_on = preferences.getUInt("minute_on", 0);
        settings.hour_off = preferences.getUInt("hour_off", 0);
        settings.minute_off = preferences.getUInt("minute_off", 0);

        preferences.end();
    }
}

// Public methods

void BusinessLogicHandler::handleCommand(const String& command) {
    StaticJsonDocument<256> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, command);

    if (error) {
        Serial.println("Failed to parse command JSON.");
        return;
    }

    String commandType = jsonDoc["command"];

    if (commandType == "REBOOT") {
        Serial.println("Rebooting device...");
        ESP.restart();
    } else if (commandType == "TOGGLE") {
        String payloadStr = jsonDoc["payload"];
        handleToggle(payloadStr);
    } else if (commandType == "SCHEDULE") {
        JsonObject payload = jsonDoc["payload"];
        int hourOn = payload["hour_on"];
        int minuteOn = payload["minute_on"];
        int hourOff = payload["hour_off"];
        int minuteOff = payload["minute_off"];
        handleSchedule(hourOn, minuteOn, hourOff, minuteOff);
    } else if (commandType == "AUTO") {
        String payloadStr = jsonDoc["payload"];
        handleAuto(payloadStr);
    } else {
        Serial.print("Unknown command type: ");
        Serial.println(commandType);
    }
}

String cutShort(double value) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    return String(buffer);
}

String BusinessLogicHandler::getStatus() {
    StaticJsonDocument<1024> jsonDoc;

    // Include time
    jsonDoc["time"] = DayTime.unixtime;

    // Include device state
    jsonDoc["toggle"] = deviceState ? 1 : 0;
    jsonDoc["auto"] = isAuto ? 1 : 0;
    // Include GPS data
    jsonDoc["gps_log"] = gpsLongitude;
    jsonDoc["gps_lat"] = gpsLatitude;

    // Include power meter data
    jsonDoc["voltage"] = cutShort(powerMeterData.voltage);
    jsonDoc["current"] = cutShort(powerMeterData.current);
    jsonDoc["power"] = cutShort(powerMeterData.power);
    jsonDoc["power_factor"] = cutShort(powerMeterData.power_factor);
    jsonDoc["frequency"] = cutShort(powerMeterData.frequency);
    jsonDoc["total_energy"] = cutShort(powerMeterData.total_energy);
    jsonDoc["frequency"] = cutShort(powerMeterData.frequency);
    jsonDoc["total_energy"] = cutShort(powerMeterData.total_energy);

    // Include schedule
    jsonDoc["hour_on"] = settings.hour_on;
    jsonDoc["minute_on"] = settings.minute_on;
    jsonDoc["hour_off"] = settings.hour_off;
    jsonDoc["minute_off"] = settings.minute_off;
    
    String status;
    serializeJson(jsonDoc, status);
    return status;
}

void BusinessLogicHandler::handleToggle(const String& state) {
    if (state == "on") {
        Serial.println("Toggling device ON...");
        deviceState = true;
        isAuto = false;
    } else if (state == "off") {
        Serial.println("Toggling device OFF...");
        deviceState = false;
        isAuto = false;
    } else {
        Serial.print("Unknown toggle state: ");
        Serial.println(state);
    }
    // Update the OUTPUT_CRT pin based on deviceState
    digitalWrite(OUTPUT_CRT, deviceState ? HIGH : LOW);
}

void BusinessLogicHandler::handleSchedule(int hourOn, int minuteOn, int hourOff, int minuteOff) {
    Serial.printf("Scheduling ON: %02d:%02d, OFF: %02d:%02d\n", hourOn, minuteOn, hourOff, minuteOff);
    settings.hour_on = hourOn;
    settings.minute_on = minuteOn;
    settings.hour_off = hourOff;
    settings.minute_off = minuteOff;

    if (preferences.begin("settings", false)) {
        preferences.putInt("hour_on", hourOn);
        preferences.putInt("minute_on", minuteOn);
        preferences.putInt("hour_off", hourOff);
        preferences.putInt("minute_off", minuteOff);
                
        preferences.end(); // Ensure to close preferences after writing
    }
}

void BusinessLogicHandler::handleAuto(const String& state) {
    if (state == "on") {
        Serial.println("Auto mode ON...");
        isAuto = true;
    } else if (state == "off") {
        Serial.println("Auto mode OFF...");
        isAuto = false;
    } else {
        Serial.print("Unknown auto state: ");
        Serial.println(state);
    }
}

void BusinessLogicHandler::update() {
    timeClient.update();

    unsigned long currentMillis = millis();
    // Update time from NTP client periodically
    // static unsigned long lastTimeUpdate = 0;
    // if (currentMillis - lastTimeUpdate >= status_interval) { // Update every minute
    //     lastTimeUpdate = currentMillis;
    //     DayTime = timeClient.getDateTime();
    //     deviceLCD.setDayTime(DayTime);
    // }

    // FLASH_ACTIVE_led(10, 1000);
    digitalWrite(PR_LED, millis() % 1000 < 500);
    digitalWrite(PWM_AUTO_RESET, !digitalRead(PWM_AUTO_RESET));

    // Update GPS data
    updateGPS();
    deviceLCD.setDayTime(DayTime);

    // Handle scheduling
    updateScheduling();
    digitalWrite(OUTPUT_CRT, deviceState ? HIGH : LOW);
    // Update LCD display
    // Intialize new settings
    
    deviceLCD.print(settings);
    digitalWrite(LED_BUILTIN, !LED_BUILTIN_ON_STATE);
      
    // Read power meter data
    power_meter_read(powerMeterData);
}

void processGPSData() {
    while (Serial1.available()) {
        char c = Serial1.read();
        gps.encode(c);
    }
}

void BusinessLogicHandler::updateGPS() {
    RTCDateTime DayTime_net = timeClient.getDateTime();                                       // lưu thời gian vào biến DayTime
    processGPSData();
    RTCDateTime DayTime_gps = gps.getDateTime(); 

    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    if (flat != GPS_INVALID_F_ANGLE) gpsLatitude = flat;
    if (flon != GPS_INVALID_F_ANGLE) gpsLongitude = flon;

    if (DayTime_net.unixtime > DayTime_gps.unixtime) DayTime = DayTime_net;
    else DayTime = DayTime_gps;   
}
// Implement LCD_print() and any other required methods

void BusinessLogicHandler::updateScheduling() {
    // Get current time in seconds since midnight
    int currentSeconds = DayTime.hour * 3600 + DayTime.minute * 60 + DayTime.second;
    int onTimeSeconds = settings.hour_on * 3600 + settings.minute_on * 60;
    int offTimeSeconds = settings.hour_off * 3600 + settings.minute_off * 60;

    // Handle device state based on schedule
    if ((onTimeSeconds != offTimeSeconds) && isAuto) { // Valid schedule and auto mode
        if ((currentSeconds >= onTimeSeconds && currentSeconds < offTimeSeconds) ||
            (offTimeSeconds < onTimeSeconds && (currentSeconds >= onTimeSeconds || currentSeconds < offTimeSeconds))) {
            deviceState = true;
        } else {
            deviceState = false;
        }
    }

    // Update output
    digitalWrite(OUTPUT_CRT, deviceState ? HIGH : LOW);
}

// Add any additional methods required for your application