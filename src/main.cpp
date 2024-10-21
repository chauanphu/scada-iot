#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // JSON library for handling status in JSON format
#include "secrets.h"
#include "OTAHandler.h"
#include <time.h>  // For time management

// Global objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Instantiate OTAHandler with the existing mqttClient
OTAHandler otaHandler(mqttClient);

// Function prototypes
void setup_wifi();
bool connectToMQTT();
String getFormattedMAC();
void mqttCallback(char* topic, byte* payload, unsigned int length);

// Abstract business logic function prototypes
String get_status();
void handle_command(const String& command);

// Global variables
String macAddress;
String commandTopic;
String statusTopic;

void setup() {
    Serial.begin(115200);
    Serial.println("Booting...");

    // Connect to Wi-Fi
    setup_wifi();
    configTime(25200, 0, "pool.ntp.org", "time.nist.gov");  // 25200 seconds is UTC+7

    // Wait for time to be set
    Serial.println("Waiting for time synchronization...");
    while (!time(nullptr)) {
        delay(500);
    }
    Serial.println("Time synchronized.");
    // Set MQTT server and callback function
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    // Connect to MQTT broker
    if (connectToMQTT()) {
        // Initialize OTA functionality (subscribe to OTA topic)
        otaHandler.setupOTA();
    } else {
        Serial.println("Failed to connect to MQTT broker in setup.");
    }

    // Additional setup code if needed
}

void loop() {
    // Reconnect to Wi-Fi if disconnected
    if (WiFi.status() != WL_CONNECTED) {
        setup_wifi();
    }

    // Reconnect to MQTT if disconnected
    if (!mqttClient.connected()) {
        if (connectToMQTT()) {
            // Resubscribe to necessary topics
            mqttClient.subscribe(commandTopic.c_str());
            otaHandler.setupOTA(); // Resubscribe to OTA topic
        }
    }
    
    mqttClient.loop();

    // Business logic: Publish device status at regular intervals
    static unsigned long lastStatusPublish = 0;
    unsigned long now = millis();
    if (now - lastStatusPublish > status_interval) { // Publish status every 5 seconds
        String status = get_status();
        mqttClient.publish(statusTopic.c_str(), status.c_str(), true);
        Serial.println("Status published.");
        lastStatusPublish = now;
    }
}

// Function to connect to Wi-Fi
void setup_wifi() {
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int max_attempts = 20;
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < max_attempts) {
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi connected");
        // Get and format MAC address
        macAddress = getFormattedMAC();
        commandTopic = MQTT_COMMAND_TOPIC_PREFIX + macAddress + MQTT_COMMAND_TOPIC_SUFFIX;
        statusTopic = MQTT_STATUS_TOPIC_PREFIX + macAddress + MQTT_STATUS_TOPIC_SUFFIX;
        Serial.print("MAC Address: ");
        Serial.println(macAddress);
    } else {
        Serial.println("\nFailed to connect to Wi-Fi");
        // Implement retry logic or enter deep sleep
    }
}

// Function to connect to MQTT broker with Last Will and Testament
bool connectToMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT broker at ");
        Serial.print(MQTT_SERVER);
        Serial.print(":");
        Serial.println(MQTT_PORT);

        String clientId = "ESP32Client-" + macAddress;

        // Define Last Will and Testament
        String aliveTopic = MQTT_ALIVE_TOPIC_PREFIX + macAddress + MQTT_ALIVE_TOPIC_SUFFIX;

        const char* willMessage = "0";
        int willQoS = 1;
        bool willRetain = true;

        // Attempt to connect with LWT
        if (mqttClient.connect(clientId.c_str(),
                               NULL, NULL,          // Username and password if required
                               aliveTopic.c_str(),
                               willQoS,
                               willRetain,
                               willMessage)) {
            Serial.println("Connected to MQTT broker");

            // Publish alive message upon connection
            String alivePublishTopic = aliveTopic;
            mqttClient.publish(alivePublishTopic.c_str(), "1", true);
            Serial.print("Published to: ");
            Serial.println(alivePublishTopic);
            Serial.println("Message: 1");

            // Subscribe to business logic topic
            mqttClient.subscribe(commandTopic.c_str());
            Serial.print("Subscribed to: ");
            Serial.println(commandTopic);
        } else {
            Serial.print("Failed to connect to MQTT, rc=");
            Serial.print(mqttClient.state());
            Serial.println(". Trying again in 5 seconds...");
            delay(5000);
        }
    }
    return mqttClient.connected();
}

// MQTT callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Main - Message arrived on topic: ");
    Serial.println(topicStr);
    Serial.print("Main - Message: ");
    Serial.println(message);

    // Handle OTA messages
    if (topicStr == MQTT_FIRMWARE_UPDATE_TOPIC) {
        otaHandler.handleOtaMessage(message);
    }
    // Handle business logic messages
    else if (topicStr == commandTopic) {
        Serial.println("Main - Processing business logic command...");
        handle_command(message);  // Handle the command logic
    }
}

// Utility function to get formatted MAC address
String getFormattedMAC() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    return mac;
}

// Abstract function to get the current status in JSON format
String get_status() {
    // Example: Collect and format status data
    DynamicJsonDocument jsonDoc(200); 

    // Get current time (in seconds since Jan 1, 1970)
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Populate the JSON document with required data
    jsonDoc["time"] = now;  // Add real UTC+7 time in seconds
    jsonDoc["toggle"] = 0;  // Example toggle value (customize as needed)
    jsonDoc["gps_log"] = "10.8455953";  // Example GPS longitude (replace with actual value)
    jsonDoc["gps_lat"] = "106.6099666";  // Example GPS latitude (replace with actual value)
    jsonDoc["voltage"] = 220.5;  // Example voltage value
    jsonDoc["current"] = 0.04;  // Example current value
    jsonDoc["power"] = 105;  // Example power value
    jsonDoc["power_factor"] = 0.98;  // Example power factor
    jsonDoc["frequency"] = 50;  // Example frequency in Hz
    jsonDoc["total_energy"] = 10;  // Example total energy consumption

    // Serialize the JSON document to a string
    String status;
    serializeJson(jsonDoc, status);
    return status;
}

// Abstract function to handle incoming commands
void handle_command(const String& command) {
    Serial.print("Handling command: ");

    // Parse the command using ArduinoJson
    StaticJsonDocument<200> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, command);

    if (error) {
        Serial.println("Failed to parse command JSON.");
        return;
    }

    // Get the command type and payload
    String commandType = jsonDoc["command"];
    JsonObject payload = jsonDoc["payload"];

    // Process the command based on type
    if (commandType == "REBOOT") {
        Serial.println("Rebooting device...");
        ESP.restart();  // Reboot the ESP32
    }
    else if (commandType == "TOGGLE") {
        String toggleValue = payload["toggle"];
        if (toggleValue == "on") {
            Serial.println("Toggling device ON...");
            // Add code to turn device ON
        }
        else if (toggleValue == "off") {
            Serial.println("Toggling device OFF...");
            // Add code to turn device OFF
        }
    }
    else if (commandType == "SCHEDULE") {
        int hour_on = payload["hour_on"];
        int minute_on = payload["minute_on"];
        int hour_off = payload["hour_off"];
        int minute_off = payload["minute_off"];

        Serial.println("Scheduling device ON and OFF times...");
        Serial.printf("ON: %02d:%02d, OFF: %02d:%02d\n", hour_on, minute_on, hour_off, minute_off);

        // Add logic to handle scheduling
        // You can store these times in variables and use them in your loop
    }
    else {
        Serial.println("Unknown command.");
    }
}