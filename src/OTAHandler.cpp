#include "OTAHandler.h"
#include "secrets.h"

OTAHandler::OTAHandler(PubSubClient& client) : mqttClient(client) {
    // Constructor
    this->mqttClient = client;
}

void OTAHandler::setupOTA() {
    // Subscribe to the firmware update topic
    bool subscriptionResult = mqttClient.subscribe(MQTT_FIRMWARE_UPDATE_TOPIC);
    if (subscriptionResult) {
        Serial.print("OTAHandler - Subscribed to OTA topic: ");
        Serial.println(MQTT_FIRMWARE_UPDATE_TOPIC);
    } else {
        Serial.print("OTAHandler - Failed to subscribe to OTA topic: ");
        Serial.println(MQTT_FIRMWARE_UPDATE_TOPIC);
        Serial.print("OTAHandler - MQTT client state: ");
        Serial.println(mqttClient.state());
    }
}

void OTAHandler::handleOtaMessage(const String& message) {
    if (message == "update_firmware") {
        Serial.println("OTAHandler - Initiating OTA update...");
        performOTA();
    }
}

void OTAHandler::performOTA() {
    WiFiClient client;
    HTTPClient http;

    Serial.println("OTAHandler - Starting OTA...");

    int retryCount = 0;
    bool otaSuccess = false;

    while (retryCount < maxRetries && !otaSuccess) {
        retryCount++;
        Serial.print("OTAHandler - Attempt ");
        Serial.print(retryCount);
        Serial.println(" to download firmware...");

        http.begin(client, FIRMWARE_URL);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            int contentLength = http.getSize();
            bool canBegin = Update.begin(contentLength);

            if (canBegin) {
                Serial.println("OTAHandler - Begin OTA update...");
                int written = Update.writeStream(http.getStream());

                if (written == contentLength) {
                    Serial.println("OTAHandler - Written " + String(written) + " bytes successfully");
                    otaSuccess = true;
                } else {
                    Serial.println("OTAHandler - Written only: " + String(written) + "/" + String(contentLength));
                }

                if (Update.end()) {
                    Serial.println("OTAHandler - OTA done!");
                    if (Update.isFinished()) {
                        Serial.println("OTAHandler - Update successfully completed. Rebooting...");
                        ESP.restart();
                    } else {
                        Serial.println("OTAHandler - Update not finished? Something went wrong!");
                    }
                } else {
                    Serial.println("OTAHandler - Error #: " + String(Update.getError()));
                }
            } else {
                Serial.println("OTAHandler - Not enough space to begin OTA");
            }
        } else {
            Serial.println("OTAHandler - Firmware download failed, HTTP error: " + String(httpCode));
        }

        http.end();

        if (!otaSuccess) {
            Serial.println("OTAHandler - Retrying OTA in 5 seconds...");
            delay(5000);
        }
    }

    if (!otaSuccess) {
        Serial.println("OTAHandler - Failed to perform OTA after " + String(maxRetries) + " attempts.");
    }
}
