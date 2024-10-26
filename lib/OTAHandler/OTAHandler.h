#ifndef OTAHANDLER_H
#define OTAHANDLER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <PubSubClient.h>

class OTAHandler {
public:
    OTAHandler(PubSubClient& client);
    void setupOTA();
    void performOTA();
    void handleOtaMessage(const String& message);

private:
    PubSubClient& mqttClient;
    const int maxRetries = 10; // Maximum number of OTA retry attempts
};

#endif