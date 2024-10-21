#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "OTAHandler.h"

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

// Global variables
String macAddress;
String commandTopic;

void setup() {
    Serial.begin(115200);
    Serial.println("Booting...");

    // Connect to Wi-Fi
    setup_wifi();

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

    // Your business logic code
    // e.g., read sensors, process data, perform tasks

    // Example: Delay to simulate work
    delay(1000);
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
        // Process the business logic message
        Serial.println("Main - Processing business logic command...");
        // Add your business logic processing here
    }
}

// Utility function to get formatted MAC address
String getFormattedMAC() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    return mac;
}