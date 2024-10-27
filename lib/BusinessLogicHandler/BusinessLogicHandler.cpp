#include "BusinessLogicHandler.h"
#include <ArduinoJson.h>
#include <time.h>
#include <Arduino.h>
#include <Udp.h>
#include <LiquidCrystal.h>
#include <WiFiUdp.h>
#include "secrets.h"

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

// #include "NTPClient.h"
#include "TinyGPS.h"
#include "Modbus.h"
#include "button.h"
// Instantiate hardware components
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7 * 3600, 60000);  // Adjust timezone as needed
TinyGPS gps;
Modbus modbus(Serial2);  // Using Serial2 for Modbus and GPS
// Button Button_UP(36, BUTTON_ANALOG, 1000, 2200);
// Button Button_DN(36, BUTTON_ANALOG, 1000, 470);
// Button Button_OK(36, BUTTON_ANALOG, 1000, 0);
LiquidCrystal lcd(15 /*rs*/, 2 /*en*/, 0/*d4*/, 4 /*d5*/, 5 /*d6*/, 19 /*d7*/);
RTCDateTime DayTime;

#include "power_meter.h"
#include "printLCD.h"

#include "BusinessLogicHandler.h"

// Constructor
BusinessLogicHandler::BusinessLogicHandler(PubSubClient& client, const String& mac)
    : mqttClient(client),
      macAddress(mac),
      timeClient(ntpUDP, "europe.pool.ntp.org", 7 * 3600, 60000),
      settings({0, 0, 0, 0}),
      isAlive("0"),

    //   buttonUp(36, BUTTON_ANALOG, 1000, 2200),
    //   buttonDn(36, BUTTON_ANALOG, 1000, 470),
    //   buttonOk(36, BUTTON_ANALOG, 1000, 0),
      lcd(15, 2, 0, 4, 5, 19),
      deviceState(false),
      scheduledHourOn(0),
      scheduledMinuteOn(0),
      scheduledHourOff(0),
      scheduledMinuteOff(0),
      gpsLatitude(0.0),
      gpsLongitude(0.0)
      {
    // Initialize topics
    statusTopic = "unit/" + macAddress + "/status";
    commandTopic = "unit/" + macAddress + "/command";

    // Subscribe to command topic
    mqttClient.subscribe(commandTopic.c_str());

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
    LCD_begin();

    // Initialize NTP Client
    timeClient.begin();

    // Initialize GPS (using HardwareSerial)
    Serial1.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

    // Initialize Modbus (using HardwareSerial)
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

    // Initialize other components
    if (power_meter_read(powerMeterData) == PowerMeterResponse::TIMEOUT) isAlive = "0";
    else isAlive = "1";
    // Any additional setup...
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
    } else {
        Serial.print("Unknown command type: ");
        Serial.println(commandType);
    }
}

String BusinessLogicHandler::getStatus() {
    StaticJsonDocument<1024> jsonDoc;

    // Include time
    jsonDoc["time"] = DayTime.unixtime;

    // Include device state
    jsonDoc["toggle"] = deviceState ? 1 : 0;

    // Include GPS data
    jsonDoc["gps_log"] = gpsLongitude;
    jsonDoc["gps_lat"] = gpsLatitude;

    // Include power meter data
    jsonDoc["voltage"] = powerMeterData.voltage;
    jsonDoc["current"] = powerMeterData.current;
    jsonDoc["power"] = powerMeterData.power;
    jsonDoc["power_factor"] = powerMeterData.power_factor;
    jsonDoc["frequency"] = powerMeterData.frequency;
    jsonDoc["total_energy"] = powerMeterData.total_energy;

    String status;
    serializeJson(jsonDoc, status);
    return status;
}

void BusinessLogicHandler::handleToggle(const String& state) {
    if (state == "on") {
        Serial.println("Toggling device ON...");
        deviceState = true;
    } else if (state == "off") {
        Serial.println("Toggling device OFF...");
        deviceState = false;
    } else {
        Serial.print("Unknown toggle state: ");
        Serial.println(state);
    }
    // Update the OUTPUT_CRT pin based on deviceState
    digitalWrite(OUTPUT_CRT, deviceState ? HIGH : LOW);
}

void BusinessLogicHandler::handleSchedule(int hourOn, int minuteOn, int hourOff, int minuteOff) {
    Serial.printf("Scheduling ON: %02d:%02d, OFF: %02d:%02d\n", hourOn, minuteOn, hourOff, minuteOff);
    scheduledHourOn = hourOn;
    scheduledMinuteOn = minuteOn;
    scheduledHourOff = hourOff;
    scheduledMinuteOff = minuteOff;
}

void BusinessLogicHandler::update() {
    unsigned long currentMillis = millis();

    // Update time from NTP client periodically
    static unsigned long lastTimeUpdate = 0;
    if (currentMillis - lastTimeUpdate >= status_interval) { // Update every minute
        timeClient.update();
        lastTimeUpdate = currentMillis;
        DayTime = timeClient.getDateTime();
    }

    // Update GPS data
    updateGPS();

    // Handle scheduling
    updateScheduling();

    // Read buttons
    // updateButtons();

    // Update LCD display
    // Intialize new settings
    
    LCD_print(settings);

    // Read power meter data
    if (!power_meter_read(powerMeterData)) {

    }

    // Other periodic tasks
}

void BusinessLogicHandler::updateGPS() {
    while (Serial1.available()) {
        char c = Serial1.read();
        gps.encode(c);
    }
    // Get GPS data
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    if (flat != GPS_INVALID_F_ANGLE) {
        gpsLatitude = flat;
    }
    if (flon != GPS_INVALID_F_ANGLE) {
        gpsLongitude = flon;
    }
}
// Implement LCD_print() and any other required methods

void BusinessLogicHandler::updateScheduling() {
    // Get current time in seconds since midnight
    int currentSeconds = DayTime.hour * 3600 + DayTime.minute * 60 + DayTime.second;
    int onTimeSeconds = scheduledHourOn * 3600 + scheduledMinuteOn * 60;
    int offTimeSeconds = scheduledHourOff * 3600 + scheduledMinuteOff * 60;

    // Handle device state based on schedule
    if (onTimeSeconds != offTimeSeconds) { // Valid schedule
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