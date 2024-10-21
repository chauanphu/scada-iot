#include "BusinessLogicHandler.h"
#include <ArduinoJson.h>
#include <time.h>

// Constructor
BusinessLogicHandler::BusinessLogicHandler(PubSubClient& client, const String& mac) 
    : mqttClient(client), macAddress(mac), deviceState(false) {
    // Initialize topics
    statusTopic = "unit/" + mac + "/status";
    commandTopic = "unit/" + mac + "/command";
}

// Method to handle incoming commands
void BusinessLogicHandler::handleCommand(const String& command) {
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, command);

    if (error) {
        Serial.println("Failed to parse command JSON.");
        return;
    }

    String commandType = jsonDoc["command"];
    JsonObject payload = jsonDoc["payload"];

    if (commandType == "REBOOT") {
        Serial.println("Rebooting device...");
        ESP.restart();
    } else if (commandType == "TOGGLE") {
        String toggleValue = payload["toggle"];
        handleToggle(toggleValue);
    } else if (commandType == "SCHEDULE") {
        int hourOn = payload["hour_on"];
        int minuteOn = payload["minute_on"];
        int hourOff = payload["hour_off"];
        int minuteOff = payload["minute_off"];
        handleSchedule(hourOn, minuteOn, hourOff, minuteOff);
    }
}

// Method to get the device status in JSON format
String BusinessLogicHandler::getStatus() {
    JsonDocument jsonDoc;

    time_t now;
    time(&now);

    jsonDoc["time"] = now;
    jsonDoc["toggle"] = deviceState ? 1 : 0;  // Example toggle state
    jsonDoc["gps_log"] = "10.8455953";
    jsonDoc["gps_lat"] = "106.6099666";
    jsonDoc["voltage"] = 220.5;
    jsonDoc["current"] = 0.04;
    jsonDoc["power"] = 105;
    jsonDoc["power_factor"] = 0.98;
    jsonDoc["frequency"] = 50;
    jsonDoc["total_energy"] = 10;

    String status;
    serializeJson(jsonDoc, status);
    return status;
}

// Method to handle toggle command
void BusinessLogicHandler::handleToggle(const String& state) {
    if (state == "on") {
        Serial.println("Toggling device ON...");
        deviceState = true;
        // ...
    } else if (state == "off") {
        Serial.println("Toggling device OFF...");
        deviceState = false;
        // ...
    }
}

// Method to handle schedule command
void BusinessLogicHandler::handleSchedule(int hourOn, int minuteOn, int hourOff, int minuteOff) {
    Serial.printf("Scheduling ON: %02d:%02d, OFF: %02d:%02d\n", hourOn, minuteOn, hourOff, minuteOff);
    scheduledHourOn = hourOn;
    scheduledMinuteOn = minuteOn;
    scheduledHourOff = hourOff;
    scheduledMinuteOff = minuteOff;

    // ...
    
}
