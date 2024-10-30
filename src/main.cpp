#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // JSON library for handling status in JSON format
#include "secrets.h"
#include "OTAHandler.h"
#include <time.h>  // For time management
#include "BusinessLogicHandler.h"

// Global objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Instantiate OTAHandler with the existing mqttClient
OTAHandler otaHandler(mqttClient);

// Instantiate BusinessLogicHandler
BusinessLogicHandler* businessLogicHandler;

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
String aliveTopic;

void setup() {
    Serial.begin(115200);
    Serial.println("Booting...");

    // Connect to Wi-Fi
    setup_wifi();
    configTime(25200, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time synchronization...");

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
    // Instantiate business logic handler after Wi-Fi is connected
    macAddress = getFormattedMAC();
    businessLogicHandler = new BusinessLogicHandler(mqttClient, macAddress);
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
    businessLogicHandler->update();  // Call update method in BusinessLogicHandler
    // Business logic: Publish device status at regular intervals
    static unsigned long lastStatusPublish = 0;
    unsigned long now = millis();
    if (now - lastStatusPublish > status_interval) { // Publish status every 5 seconds
        String status = businessLogicHandler->getStatus();  // Use getStatus from BusinessLogicHandler
        mqttClient.publish(statusTopic.c_str(), status.c_str());
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

        srand(time(0));  // Seed the random number generator
        int randomId = rand();
        String clientId = "ESP32Client-" + String(randomId);

        // Define Last Will and Testament
        aliveTopic = MQTT_ALIVE_TOPIC_PREFIX + macAddress + MQTT_ALIVE_TOPIC_SUFFIX;

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
            mqttClient.publish(aliveTopic.c_str(), "1", true);
            Serial.print("Published to: ");
            Serial.println(aliveTopic);
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
    if (businessLogicHandler == nullptr) {
        Serial.println("Business logic handler not initialized yet.");
        return;
    }
    
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
        businessLogicHandler->deviceLCD.print("OTA updating...");
        otaHandler.handleOtaMessage(message);
    }
    // Handle business logic messages
    else if (topicStr == commandTopic) {
        Serial.println("Main - Processing business logic command...");
        businessLogicHandler->handleCommand(message);  // Handle the command logic
    }
}

// Utility function to get formatted MAC address
String getFormattedMAC() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    return mac;
}