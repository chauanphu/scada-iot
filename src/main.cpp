#include <Arduino.h>

void setup() {
  // Initialize serial communication at 9600 bits per second
  Serial.begin(9600);
}

void loop() {
  // Print "Hello" to the serial monitor
  Serial.println("Hello");
  
  // Wait for 2 seconds
  delay(2000);
}
// No additional code needed for this functionality