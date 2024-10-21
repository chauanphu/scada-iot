#ifndef BUSINESSLOGICHANDLER_H
#define BUSINESSLOGICHANDLER_H

#include <ArduinoJson.h>
#include <PubSubClient.h>

class BusinessLogicHandler {
public:
    // Constructor
    BusinessLogicHandler(PubSubClient& client, const String& mac);

    // Public methods
    void handleCommand(const String& command);
    String getStatus();

private:
    // Private methods for internal logic
    void handleToggle(const String& state);
    void handleSchedule(int hourOn, int minuteOn, int hourOff, int minuteOff);

    // Member variables
    PubSubClient& mqttClient;
    String macAddress;
    String statusTopic;
    String commandTopic;

    // State variables
    bool deviceState;  // ON/OFF state
    int scheduledHourOn;
    int scheduledMinuteOn;
    int scheduledHourOff;
    int scheduledMinuteOff;
};

#endif