#ifndef button_BaoTran97_h
#define button_BaoTran97_h

#include <Arduino.h>

#define BUTTON_ANALOG  1
#define BUTTON_DIGITAL 0

class Button {
  public:
    int           type_button = BUTTON_DIGITAL;
    unsigned int  max_analog  = 4095;
    int           pin;
    int           val_reference;
    double        delta;
    boolean       old_stt;
    boolean       read_return;
    unsigned long time_Falling;
    unsigned long time_Rising;
    unsigned long time_refresh;
    unsigned long time_true;
    unsigned long time_false;

    Button(long _pin, int _type = BUTTON_DIGITAL, unsigned long _high_resistor = 0, unsigned long _low_resistor = 0, double _delta = 20) {
      pin             = _pin;
      type_button     = _type;

      if (type_button == BUTTON_DIGITAL) {
        pinMode(pin, INPUT_PULLUP);
        val_reference = 0;
      } else {
        val_reference = (double(max_analog) * double(_low_resistor)) / double(_high_resistor + _low_resistor);
      }
      delta = max_analog * _delta / 100.0;
    }

    boolean read(unsigned long _time_delay = 100) {
      int val;
      if (type_button == BUTTON_DIGITAL) {
        val = digitalRead(pin) * max_analog;
      } else {
        val = analogRead(pin);
      }

      if (abs(val - val_reference) <= delta) {
        if (time_true <= time_false) time_true = millis();
      } else {
        if (time_false <= time_true) time_false = millis();
      }

      if (time_true  > millis()) time_true = 0;
      if (time_false > millis()) time_false = 0;

      if ((time_true > time_false) && (millis() - time_true  > _time_delay)) read_return = 0;
      if ((time_false > time_true) && (millis() - time_false > _time_delay)) read_return = 1;

      return read_return;
    }

    boolean IsFalling(unsigned long _time_delay = 50) {
      boolean new_stt = read();
      boolean falling = (old_stt) && (!new_stt);
      boolean Rising  = (!old_stt) && (new_stt);
      old_stt         = new_stt;

      if (Rising) time_Rising = millis();

      if (falling) {
        time_Falling  = millis();
        time_refresh  = millis();
        if (millis() - time_Rising < _time_delay) time_Rising = 0;
      }

      if ((millis() - time_Falling >= _time_delay) && (time_Rising) && (!new_stt)) {
        time_Rising = 0;
        return 1;
      }

      return 0;
    }

    boolean IsFallingContinuous(unsigned long _time_refresh = 100, unsigned long _time_delay = 500) {
      if (IsFalling()) return 1;
      if (read())      return 0;

      if ((millis() - time_Falling >= _time_delay) && (millis() - time_refresh >= _time_refresh)) {
        time_refresh = millis();
        return 1;
      }

      return 0;
    }

    boolean IsLow() {
      return !read();
    }

    boolean IsHigh() {
      return read();
    }
};

#endif