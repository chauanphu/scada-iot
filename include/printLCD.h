#include <WiFi.h>  // WiFi library
#include <Arduino.h>
#include "button.h"
#include "types.h"

Button Button_UP(36, BUTTON_ANALOG, 1000, 2200);
Button Button_DN(36, BUTTON_ANALOG, 1000, 470);
Button Button_OK(36, BUTTON_ANALOG, 1000, 0);

// Global variables
byte display_index;     // Variable to store selected screen index
byte Cursor_line;       // Variable to store selected line on the screen
byte display_set;       // Variable to indicate display mode
byte Cursor_index = 100;  // Variable to store cursor position

// Function to calculate the time since the last button press in milliseconds
unsigned long button_old_pluse_time() {
  unsigned long re = millis() - Button_OK.time_refresh;
  if (millis() - Button_UP.time_refresh < re) re = millis() - Button_UP.time_refresh;
  if (millis() - Button_DN.time_refresh < re) re = millis() - Button_DN.time_refresh;
  return re;
}

// Function to automatically return to the main screen when buttons are not pressed for 5 minutes
void LCD_auto_back_home() {
  if (button_old_pluse_time() > 300000) {  // 5 minutes = 300,000 milliseconds
    display_index = 0;  // Return to main screen
    Cursor_line   = 0;  // Reset cursor line
    display_set   = 0;  // Exit setup mode
  }
}

// Function to handle screen navigation
void LCD_display_index() {
  if (display_index == 0) {  // If on the home screen
    if ((Button_OK.IsFalling()) || (Button_UP.IsFalling()) || (Button_DN.IsFalling())) {
      display_index = 1;  // Enter the settings selection screen
      Cursor_line   = 0;  // Set cursor to the first line
      lcd.clear();
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
      lcd.clear();
    }
  }
}

// Function to display content based on the current screen index
void LCD_display_print_index(const SettingsData& settings) {
  char s[32];  // Buffer for string manipulation
  if (display_index == 0) {  // Main screen
    lcd.setCursor(0, 1);
    sprintf(s, "     %02u/%02u/%04u     ", DayTime.day, DayTime.month, DayTime.year);
    lcd.print(s);

    lcd.setCursor(0, 0);
    sprintf(s, "      %02u:%02u:%02u      ", DayTime.hour, DayTime.minute, DayTime.second);
    lcd.print(s);

    lcd.setCursor(0, 3);
    lcd.print("tell:  +84915999472 ");

    lcd.setCursor(0, 2);
    lcd.print("                    ");
  } else {
    int HourStart   = settings.hour_on;
    int MinuteStart = settings.minute_on;
    int HourEnd     = settings.hour_off;
    int MinuteEnd   = settings.minute_off;

    lcd.setCursor(1, 0);
    sprintf(s, " Time On:   %02u:%02u ", HourStart, MinuteStart);
    lcd.print(s);

    lcd.setCursor(1, 1);
    sprintf(s, " Time Off:  %02u:%02u ", HourEnd, MinuteEnd);
    lcd.print(s);

    lcd.setCursor(1, 2);
    lcd.print(" WIFI information ");

    // Display cursor indicators
    for (int i = 0; i < 3; i++) {
      if (Cursor_line == i) {
        lcd.setCursor(0, i);
        lcd.print(">");
        lcd.setCursor(19, i);
        lcd.print("<");
      } else {
        lcd.setCursor(0, i);
        lcd.print(" ");
        lcd.setCursor(19, i);
        lcd.print(" ");
      }
    }

    lcd.setCursor(0, 3);
    lcd.print("                    ");
  }
}

// Function to set the "Time On" value
void SetTimeOn(SettingsData& settings) {
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

  lcd.setCursor(0, 0);
  lcd.print("Set up time on      ");

  lcd.setCursor(0, 1);
  if ((millis() % 1000 < 300) && (button_old_pluse_time() > 500)) {
    switch (Cursor_index) {
      case 0: sprintf(s, "       __:%02u        ", MinuteStart); break;
      case 1: sprintf(s, "       %02u:__        ", HourStart);   break;
    }
  } else {
    sprintf(s, "       %02u:%02u        ", HourStart, MinuteStart);
  }
  lcd.print(s);

  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");

  // Update settings
  settings.hour_on   = HourStart;
  settings.minute_on = MinuteStart;
}

// Function to set the "Time Off" value
void SetTimeOff(SettingsData& settings) {
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

  lcd.setCursor(0, 0);
  lcd.print("Set up time off     ");

  lcd.setCursor(0, 1);
  if ((millis() % 1000 < 300) && (button_old_pluse_time() > 500)) {
    switch (Cursor_index) {
      case 0: sprintf(s, "       __:%02u        ", MinuteEnd); break;
      case 1: sprintf(s, "       %02u:__        ", HourEnd);   break;
    }
  } else {
    sprintf(s, "       %02u:%02u        ", HourEnd, MinuteEnd);
  }
  lcd.print(s);

  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");

  // Update settings
  settings.hour_off   = HourEnd;
  settings.minute_off = MinuteEnd;
}


String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

// Function to show Wi-Fi information
void ShowWifiInformation() {
  if (Button_OK.IsFalling()) {
    display_set = 0;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print(WiFi.SSID());
    lcd.print("                    ");
    lcd.setCursor(0, 1);
    lcd.print(toStringIp(WiFi.localIP()));
    lcd.print("                    ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WIFI CONNECTION LOST");
    lcd.setCursor(0, 1);
    lcd.print("                    ");
  }

  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
}

// Function to select the setup page
void LCD_display_setup(SettingsData& settings) {
  if (Cursor_line == 0) {
    SetTimeOn(settings);
  } else if (Cursor_line == 1) {
    SetTimeOff(settings);
  } else {
    ShowWifiInformation();
  }
}

// Main function to handle LCD display updates
void LCD_print(SettingsData& settings) {
  LCD_auto_back_home();  // Check if need to return to main screen automatically

  if (!display_set) {  // If not in setup mode
    LCD_display_index();  // Update cursor position
    LCD_display_print_index(settings);  // Display content based on cursor position
  } else {  // If in setup mode
    LCD_display_setup(settings);  // Display setup screen
  }
}

// Function to initialize the LCD
void LCD_begin() {
  lcd.begin(20, 4);  // Initialize 20x4 LCD
  lcd.setCursor(0, 0);
  lcd.print("Program starting");
}
