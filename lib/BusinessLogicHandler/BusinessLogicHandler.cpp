#include "BusinessLogicHandler.h"
#include <ArduinoJson.h>
#include <time.h>
#include <Arduino.h>                                               // thư viện hàm arduino
#include          <Udp.h>                                                   // thư viện UDP
#include          <WiFiUdp.h>                                               // thư viện UDP
#include          "NTPClient.h"  
#include "TinyGPS.h"                                                                          // thư viện sử lý dữ liệu GPS
#include          "Modbus.h"                                                // thư viện giao tiếp modbus
#include "button.h"                                                                           // file lưu các hàm sử lý button
#include <LiquidCrystal.h> // thư viện LCD

// Khai báo Real Time Clock
WiFiUDP           ntpUDP;                                                   // sử dụng giao thức UDP
NTPClient         timeClient(ntpUDP, "europe.pool.ntp.org", 7 * 3600, 60000); // khai báo server thời gian thực
RTCDateTime       DayTime;    
GPS_time gps;                                                                                 // khởi tạo thư viện GPS

// Khai báo Modbus
Modbus            modbus(Serial2);                                          // kết nối modbus RTU với serial 2

// Khai báo các nút nhấn
Button Button_UP(36, BUTTON_ANALOG, 1000, 2200);                                              // nút up
Button Button_DN(36, BUTTON_ANALOG, 1000, 470);                                               // nút down
Button Button_OK(36, BUTTON_ANALOG, 1000, 0);     

// Khai báo LCD
LiquidCrystal lcd(15 /*rs*/, 2 /*en*/, 0/*d4*/, 4 /*d5*/, 5 /*d6*/, 19 /*d7*/);

#ifndef       LED_BUILTIN                                               // nếu chân đèn báo chưa được định nghĩa
#define       LED_BUILTIN               27                               // đèn báo trạng thái
#endif                                                                      //

#define       LED_BUILTIN_ON_STATE      1                               // mức logic đền báo trạng thái sáng
#define       FLASH_ACTIVE_LED          digitalWrite(LED_BUILTIN, LED_BUILTIN_ON_STATE); // thay thế chớp led
#define       PR_LED                    18                               // đèn báo trạng thái
#define       OUTPUT_CRT                13                               // đèn báo trạng thái
#define       PWM_AUTO_RESET            33                               // đèn báo trạng thái
#define       RS485_SERIAL              16
#define       TX_PIN                    16                              // chân TX của serial 2 kết nối các thiết bị ngoại vi
#define       RX_PIN                    17                              // chân RX của serial 2 kết nối các thiết bị ngoại vi
#define       GPS_SERIAL                25
#define       GPS_TX_PIN                25                              // chân TX của serial 2 kết nối các thiết bị ngoại vi
#define       GPS_RX_PIN                26                             // chân RX của serial 2 kết nối các thiết bị ngoại vi

byte          Serial2_Using;
unsigned long Autochange = 0;

#include "power_meter.h"                                                    // file chương trình
#include "printLCD.h" 

// Constructor
BusinessLogicHandler::BusinessLogicHandler(PubSubClient& client, const String& mac) 
    : mqttClient(client), macAddress(mac), deviceState(false) {
    // Initialize topics
    statusTopic = "unit/" + mac + "/status";
    commandTopic = "unit/" + mac + "/command";

    // Initialize pins
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);  // Turn off LED

    pinMode(PR_LED, OUTPUT); 
    digitalWrite(PR_LED, HIGH);

    pinMode(OUTPUT_CRT, OUTPUT); 
    digitalWrite(OUTPUT_CRT, LOW);

    pinMode(PWM_AUTO_RESET, OUTPUT); 
    digitalWrite(PWM_AUTO_RESET, LOW);

    // Initialize LCD
    lcd.begin(20, 4);
    lcd.setCursor(0, 0);
    lcd.print("Program starting");

    // Initialize NTP Client
    timeClient.begin();

    // Initialize GPS
    Serial2_Using = GPS_SERIAL;
    Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

    // Initialize Modbus
    modbus.init();

    // Initialize buttons
    // Button initialization is done via constructors

    // Initialize other components
    power_meter_begin();

    // Initialize member variables
    scheduledHourOn = 0;
    scheduledMinuteOn = 0;
    scheduledHourOff = 0;
    scheduledMinuteOff = 0;
}

void FLASH_ACTIVE_led(unsigned long t_on, unsigned long T) {
    #if defined(ESP32)
    static unsigned long t;
    if (millis() - t > T * 10 + t_on) {
        t = millis();
    } else if (millis() - t >= T * 10) {
        FLASH_ACTIVE_LED
    } else if (millis() - t >  T - t_on) {
        FLASH_ACTIVE_LED
        t = millis() - T * 10;
    }
    #endif  // ESP32
}

void GPS_read() {
  static unsigned long timer;
  if (millis() % 30000ul < 15000ul) return;
  if (millis() < timer)             return;
  timer = millis() + 1000;

  if (Serial2_Using != GPS_SERIAL) {
    Serial2.end();
    while (Serial2.available()) Serial2.read();
    delay(100);
    Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial2_Using = GPS_SERIAL;
  } else {
    while (Serial2.available()) gps.encode(Serial2.read());                                       // nếu có dữ liệu từ Serial tách dữ liệu lấy thời gian
  }
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
    
    if (commandType == "REBOOT") {
        Serial.println("Rebooting device...");
        ESP.restart();
    } else if (commandType == "TOGGLE") {
        // Convert payload to string
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
    } else {
        Serial.print("Unknown toggle state: ");
        Serial.println(state);
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
