// Define a data structure to hold settings
#ifndef SETTINGS_DATA_H
#define SETTINGS_DATA_H

struct SettingsData {
  int hour_on;
  int minute_on;
  int hour_off;
  int minute_off;
};

#endif // SETTINGS_DATA_H

// Define a data structure to hold power meter readings
#ifndef POWER_METER_DATA_H
#define POWER_METER_DATA_H

struct PowerMeterData {
  double total_energy;
  double total_energy_reverse;
  double total_energy_forward;
  double voltage;
  double current;
  double power;
  double power_factor;
  double frequency;
};

#endif // POWER_METER_DATA_H