#ifndef LCD_H
#define LCD_H

#include <WiFi.h>
#include <Arduino.h>
#include <LiquidCrystal.h>
#include "button.h"
#include "types.h"
#include <PubSubClient.h>
#include "NTPClient.h"

class ESP32LCD {
public:
    // Constructor
    ESP32LCD(RTCDateTime& DayTime, LiquidCrystal deviceLCD);

    // Initialize the LCD
    void begin();

    // Main function to handle LCD display updates
    void print(SettingsData& settings);

    void print(String message);

    void setDayTime(RTCDateTime& DayTime);

private:
    // Buttons
    Button Button_UP;
    Button Button_DN;
    Button Button_OK;

    // Display variables
    byte display_index;     // Variable to store selected screen index
    byte Cursor_line;       // Variable to store selected line on the screen
    byte display_set;       // Variable to indicate display mode
    byte Cursor_index;      // Variable to store cursor position
    RTCDateTime DayTime;    // Variable to store current date and time

    // LCD object
    LiquidCrystal deviceLCD;
    
    // Functions
    unsigned long button_old_pulse_time();
    void auto_back_home();
    void display_index_function();
    void display_print_index(const SettingsData& settings);
    void setTimeOn(SettingsData& settings);
    void setTimeOff(SettingsData& settings);
    void showWifiInformation();
    void display_setup(SettingsData& settings);
    String toStringIp(IPAddress ip);
};

#endif // LCD_H