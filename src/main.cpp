#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // JSON library for handling status in JSON format
#include "secrets.h"     // Must define: WIFI_SSID, WIFI_PASSWORD, MQTT_SERVER, MQTT_PORT, MQTT_COMMAND_TOPIC_PREFIX, MQTT_COMMAND_TOPIC_SUFFIX, MQTT_STATUS_TOPIC_PREFIX, MQTT_STATUS_TOPIC_SUFFIX, MQTT_FIRMWARE_UPDATE_TOPIC, etc.
#include "OTAHandler.h"
#include "BusinessLogicHandler.h" // Business logic for the device
#include <time.h>  // For time management

// Global objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Instantiate OTAHandler with the existing mqttClient
OTAHandler otaHandler(mqttClient);

// Instantiate BusinessLogicHandler
BusinessLogicHandler businessLogicHandler;

// Function prototypes
void setup_wifi();
bool connectToMQTT();
String getFormattedMAC();
void mqttCallback(char* topic, byte* payload, unsigned int length);

// Global variables for topics and MAC address
String macAddress;
String commandTopic;
String statusTopic;
String aliveTopic;

void setup() {
    // Initialize network interface
    esp_netif_init();

    Serial.begin(115200);
    Serial.println("Booting...");

    // Connect to Wi-Fi
    setup_wifi();
    
    // Set time for NTP synchronization (adjust timezone offset as needed)
    configTime(25200, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time synchronization...");

    // After Wi-Fi is connected, set up topics using the formatted MAC address.
    // (Assumes your secrets.h defines the topic prefixes and suffixes.)
    if (WiFi.status() == WL_CONNECTED) {
        macAddress = getFormattedMAC();
        commandTopic = String(MQTT_COMMAND_TOPIC_PREFIX) + macAddress + String(MQTT_COMMAND_TOPIC_SUFFIX);
        statusTopic  = String(MQTT_STATUS_TOPIC_PREFIX)  + macAddress + String(MQTT_STATUS_TOPIC_SUFFIX);
        Serial.print("MAC Address: ");
        Serial.println(macAddress);
    }

    // Set MQTT server and callback function
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(512);

    // Connect to MQTT broker
    if (connectToMQTT()) {
        Serial.println("MQTT connection established.");
        // Initialize OTA functionality (subscribe to OTA topic, etc.)
        otaHandler.setupOTA();
    } else {
        Serial.println("Failed to connect to MQTT broker in setup.");
    }
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

    // Call update method in BusinessLogicHandler
    businessLogicHandler.update();

    // Publish device status at regular intervals
    static unsigned long lastStatusPublish = 0;
    unsigned long now = millis();
    if (now - lastStatusPublish > status_interval) {
        String status = businessLogicHandler.getStatus();
        bool success = mqttClient.publish(statusTopic.c_str(), status.c_str());
        if (success) {
            Serial.print("Published status: ");
            Serial.println(status);
        } else {
            Serial.println("Failed to publish status.");
        }
        lastStatusPublish = now;
    }

    delay(10);  // Small delay to avoid busy looping
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
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to Wi-Fi");
    }
}

// Function to connect to MQTT broker with LWT if needed
bool connectToMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT broker at ");
        Serial.print(MQTT_SERVER);
        Serial.print(":");
        Serial.println(MQTT_PORT);

        // Generate a random client ID
        String clientId = macAddress + "-" + String(random(0, 10000));

        // Optionally define Last Will and Testament here
        aliveTopic = String(MQTT_ALIVE_TOPIC_PREFIX) + macAddress + String(MQTT_ALIVE_TOPIC_SUFFIX);
        const char* willMessage = "0";
        int willQoS = 1;
        bool willRetain = true;

        if (mqttClient.connect(clientId.c_str(),
                               NULL, NULL, // Username and password if needed
                               aliveTopic.c_str(),
                               willQoS,
                               willRetain,
                               willMessage)) {
            Serial.println("Connected to MQTT broker.");
            // Optionally subscribe to command topic
            mqttClient.subscribe(commandTopic.c_str());
        } else {
            Serial.print("MQTT connect failed, state=");
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

    Serial.print("MQTT message arrived on topic: ");
    Serial.println(topicStr);
    Serial.print("Message: ");
    Serial.println(message);

    // Handle OTA messages if the topic matches
    if (topicStr == String(MQTT_FIRMWARE_UPDATE_TOPIC)) {
        otaHandler.handleOtaMessage(message);
    }
    // Handle business logic commands if needed
    else if (topicStr == commandTopic) {
        businessLogicHandler.handleCommand(message);
    }
}

// Utility function to get formatted MAC address (remove colons and convert to lowercase)
String getFormattedMAC() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    return mac;
}
