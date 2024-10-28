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

#ifndef STATUS_H
#define STATUS_H

enum PowerMeterResponse {
  POWERON, // Read successful
  LOSTPOWER, // Voltage is 0,
  TIMEOUT, // Timeout occurred, mun_error > 100
  TIMECOUNT, // increase mun_error
  CONFIGURED // Configuration successful
};

#endif // STATUS_H

#ifndef __RTCDateTime__                                                                                       // nếu mảng lưu thời gian chưa được tạo
#define __RTCDateTime__                                                                                       // đánh dấu đã tạo
struct RTCDateTime {                                                                                          // mảng dữ liệu lưu thời gian
  uint16_t year     = 0;                                                                                      // năm
  uint8_t month     = 0;                                                                                      // tháng
  uint8_t day       = 0;                                                                                      // ngày
  uint8_t hour      = 0;                                                                                      // giờ
  uint8_t minute    = 0;                                                                                      // phút
  uint8_t second    = 0;                                                                                      // giây
  uint8_t dayOfWeek = 0;                                                                                      // thứ
  uint32_t unixtime = 0;                                                                                      // thời gian dài tính bằng giây từ ngày 1/1/1970
};                                                                                                            //
#endif