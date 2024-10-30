#include "ESP32LCD.h"

// Constructor
ESP32LCD::ESP32LCD(RTCDateTime& DayTime, LiquidCrystal deviceLCD) :
    Button_UP(36, BUTTON_ANALOG, 1000, 2200),
    Button_DN(36, BUTTON_ANALOG, 1000, 470),
    Button_OK(36, BUTTON_ANALOG, 1000, 0),
    display_index(0),
    Cursor_line(0),
    display_set(0),
    Cursor_index(100),
    DayTime(DayTime),
    deviceLCD(deviceLCD)
{
}

// Initialize the LCD
void ESP32LCD::begin() {
    deviceLCD.begin(20, 4);  // Initialize 20x4 LCD
    deviceLCD.setCursor(0, 0);
    deviceLCD.print("Program starting");
}

// Set the current date and time
void ESP32LCD::setDayTime(RTCDateTime& DayTime) {
    this->DayTime = DayTime;
}

// Function to calculate the time since the last button press in milliseconds
unsigned long ESP32LCD::button_old_pulse_time() {
    unsigned long re = millis() - Button_OK.time_refresh;
    if (millis() - Button_UP.time_refresh < re) re = millis() - Button_UP.time_refresh;
    if (millis() - Button_DN.time_refresh < re) re = millis() - Button_DN.time_refresh;
    return re;
}

// Function to automatically return to the main screen when buttons are not pressed for 5 minutes
void ESP32LCD::auto_back_home() {
    if (button_old_pulse_time() > 300000) {  // 5 minutes = 300,000 milliseconds
        display_index = 0;  // Return to main screen
        Cursor_line   = 0;  // Reset cursor line
        display_set   = 0;  // Exit setup mode
    }
}

// Function to handle screen navigation
void ESP32LCD::display_index_function() {
    if (display_index == 0) {  // If on the home screen
        if ((Button_OK.IsFalling()) || (Button_UP.IsFalling()) || (Button_DN.IsFalling())) {
            display_index = 1;  // Enter the settings selection screen
            Cursor_line   = 0;  // Set cursor to the first line
            deviceLCD.clear();
        }
    } else {  // If in the settings selection screen
        if (Button_UP.IsFalling()) {
            if (Cursor_line == 0)
                display_index = 0;  // Move screen up if on the top line
            else
                Cursor_line--;  // Move cursor up
        }
        if (Button_DN.IsFalling()) {
            if (Cursor_line < 2) Cursor_line++;  // Move cursor down
        }
        if (Button_OK.IsFalling()) {
            display_set = 1;  // Enter setup mode
            deviceLCD.clear();
        }
    }
}

// Function to display content based on the current screen index
void ESP32LCD::display_print_index(const SettingsData& settings) {
    char s[32];  // Buffer for string manipulation
    if (display_index == 0) {  // Main screen
        deviceLCD.setCursor(0, 1);
        sprintf(s, "     %02u/%02u/%04u     ", DayTime.day, DayTime.month, DayTime.year);
        deviceLCD.print(s);

        deviceLCD.setCursor(0, 0);
        sprintf(s, "      %02u:%02u:%02u      ", DayTime.hour, DayTime.minute, DayTime.second);
        deviceLCD.print(s);

        deviceLCD.setCursor(0, 3);
        deviceLCD.print("tell:  +84915999472 ");

        deviceLCD.setCursor(0, 2);
        deviceLCD.print("                    ");
    } else {
        int HourStart   = settings.hour_on;
        int MinuteStart = settings.minute_on;
        int HourEnd     = settings.hour_off;
        int MinuteEnd   = settings.minute_off;

        deviceLCD.setCursor(1, 0);
        sprintf(s, " Time On:   %02u:%02u ", HourStart, MinuteStart);
        deviceLCD.print(s);

        deviceLCD.setCursor(1, 1);
        sprintf(s, " Time Off:  %02u:%02u ", HourEnd, MinuteEnd);
        deviceLCD.print(s);

        deviceLCD.setCursor(1, 2);
        deviceLCD.print(" WIFI information ");

        // Display cursor indicators
        for (int i = 0; i < 3; i++) {
            if (Cursor_line == i) {
                deviceLCD.setCursor(0, i);
                deviceLCD.print(">");
                deviceLCD.setCursor(19, i);
                deviceLCD.print("<");
            } else {
                deviceLCD.setCursor(0, i);
                deviceLCD.print(" ");
                deviceLCD.setCursor(19, i);
                deviceLCD.print(" ");
            }
        }

        deviceLCD.setCursor(0, 3);
        deviceLCD.print("                    ");
    }
}

// Function to set the "Time On" value
void ESP32LCD::setTimeOn(SettingsData& settings) {
    int HourStart   = settings.hour_on;
    int MinuteStart = settings.minute_on;

    if (Cursor_index == 100) {  // First time entering setup
        Cursor_index  = 0;
    }

    if (Button_OK.IsFalling()) {
        if (Cursor_index < 1) {
            Cursor_index++;
        } else {
            Cursor_index = 100;
            display_set  = 0;
            // eepromUpdate();  // Uncomment if EEPROM update is needed
        }
    }

    if (Button_DN.IsFallingContinuous()) {
        switch (Cursor_index) {
            case 0: HourStart   = (HourStart   + 23) % 24; break;
            case 1: MinuteStart = (MinuteStart + 59) % 60; break;
        }
    }

    if (Button_UP.IsFallingContinuous()) {
        switch (Cursor_index) {
            case 0: HourStart   = (HourStart   + 1) % 24; break;
            case 1: MinuteStart = (MinuteStart + 1) % 60; break;
        }
    }

    char s[32];

    deviceLCD.setCursor(0, 0);
    deviceLCD.print("Set up time on      ");

    deviceLCD.setCursor(0, 1);
    if ((millis() % 1000 < 300) && (button_old_pulse_time() > 500)) {
        switch (Cursor_index) {
            case 0: sprintf(s, "       __:%02u        ", MinuteStart); break;
            case 1: sprintf(s, "       %02u:__        ", HourStart);   break;
        }
    } else {
        sprintf(s, "       %02u:%02u        ", HourStart, MinuteStart);
    }
    deviceLCD.print(s);

    deviceLCD.setCursor(0, 2);
    deviceLCD.print("                    ");
    deviceLCD.setCursor(0, 3);
    deviceLCD.print("                    ");

    // Update settings
    settings.hour_on   = HourStart;
    settings.minute_on = MinuteStart;
}

// Function to set the "Time Off" value
void ESP32LCD::setTimeOff(SettingsData& settings) {
    int HourEnd   = settings.hour_off;
    int MinuteEnd = settings.minute_off;

    if (Cursor_index == 100) {  // First time entering setup
        Cursor_index  = 0;
    }

    if (Button_OK.IsFalling()) {
        if (Cursor_index < 1) {
            Cursor_index++;
        } else {
            Cursor_index = 100;
            display_set = 0;
            // eepromUpdate();  // Uncomment if EEPROM update is needed
        }
    }

    if (Button_DN.IsFallingContinuous()) {
        switch (Cursor_index) {
            case 0: HourEnd   = (HourEnd   + 23) % 24; break;
            case 1: MinuteEnd = (MinuteEnd + 59) % 60; break;
        }
    }

    if (Button_UP.IsFallingContinuous()) {
        switch (Cursor_index) {
            case 0: HourEnd   = (HourEnd   + 1) % 24; break;
            case 1: MinuteEnd = (MinuteEnd + 1) % 60; break;
        }
    }

    char s[32];

    deviceLCD.setCursor(0, 0);
    deviceLCD.print("Set up time off     ");

    deviceLCD.setCursor(0, 1);
    if ((millis() % 1000 < 300) && (button_old_pulse_time() > 500)) {
        switch (Cursor_index) {
            case 0: sprintf(s, "       __:%02u        ", MinuteEnd); break;
            case 1: sprintf(s, "       %02u:__        ", HourEnd);   break;
        }
    } else {
        sprintf(s, "       %02u:%02u        ", HourEnd, MinuteEnd);
    }
    deviceLCD.print(s);

    deviceLCD.setCursor(0, 2);
    deviceLCD.print("                    ");
    deviceLCD.setCursor(0, 3);
    deviceLCD.print("                    ");

    // Update settings
    settings.hour_off   = HourEnd;
    settings.minute_off = MinuteEnd;
}

// Helper function to convert IP address to string
String ESP32LCD::toStringIp(IPAddress ip) {
    return ip.toString();
}

// Function to show Wi-Fi information
void ESP32LCD::showWifiInformation() {
    if (Button_OK.IsFalling()) {
        display_set = 0;
    }

    if (WiFi.status() == WL_CONNECTED) {
        deviceLCD.setCursor(0, 0);
        deviceLCD.print(WiFi.SSID());
        deviceLCD.print("                    ");
        deviceLCD.setCursor(0, 1);
        deviceLCD.print(toStringIp(WiFi.localIP()));
        deviceLCD.print("                    ");
    } else {
        deviceLCD.setCursor(0, 0);
        deviceLCD.print("WIFI CONNECTION LOST");
        deviceLCD.setCursor(0, 1);
        deviceLCD.print("                    ");
    }

    deviceLCD.setCursor(0, 2);
    deviceLCD.print("                    ");
    deviceLCD.setCursor(0, 3);
    deviceLCD.print("                    ");
}

// Function to select the setup page
void ESP32LCD::display_setup(SettingsData& settings) {
    if (Cursor_line == 0) {
        setTimeOn(settings);
    } else if (Cursor_line == 1) {
        setTimeOff(settings);
    } else {
        showWifiInformation();
    }
}

// Main function to handle LCD display updates
void ESP32LCD::print(SettingsData& settings) {
    auto_back_home();  // Check if need to return to main screen automatically
    if (!display_set) {  // If not in setup mode
        display_index_function();         // Update cursor position
        SettingsData newSetting = new SettingsData();
        newSetting.hour_on = settings.hour_on;
        newSetting.minute_on = settings.minute_on;
        newSetting.hour_off = settings.hour_off;
        newSetting.minute_off = settings.minute_off;
        display_print_index(newSetting);    // Display content based on cursor position
        settings = newSetting;
    } else {  // If in setup mode
        display_setup(settings);          // Display setup screen
    }
}

void ESP32LCD::print(String message) {
    deviceLCD.clear();
    deviceLCD.setCursor(0, 0);
    deviceLCD.print(message);
}