
#ifndef TinyGPS_h
#define TinyGPS_h

#if ARDUINO >= 100                                                                                            // nếu là arduino
#include "Arduino.h"                                                                                          // sử dụng thư viện arduino
#else                                                                                                         // còn lại
#include "WProgram.h"                                                                                         // sử dụng thư viện WProgram
#endif                                                                                                        //

#include <stdlib.h>

#define _GPS_VERSION          13 // software version of this library
#define _GPS_MPH_PER_KNOT     1.15077945
#define _GPS_MPS_PER_KNOT     0.51444444
#define _GPS_KMPH_PER_KNOT    1.852
#define _GPS_MILES_PER_METER  0.00062137112
#define _GPS_KM_PER_METER     0.001

#define _GPRMC_TERM           "GPRMC"
#define _GPGGA_TERM           "GPGGA"

#define GPS_INVALID_F_ANGLE     1000.0
#define GPS_INVALID_F_ALTITUDE  1000000.0
#define GPS_INVALID_F_SPEED     -1.0



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
#endif                                                                                                        //





class TinyGPS {
  public:
    enum {
      GPS_INVALID_AGE         = 0xFFFFFFFF,
      GPS_INVALID_ANGLE       = 999999999,
      GPS_INVALID_ALTITUDE    = 999999999,
      GPS_INVALID_DATE        = 0,
      GPS_INVALID_TIME        = 0xFFFFFFFF,
      GPS_INVALID_SPEED       = 999999999,
      GPS_INVALID_FIX_TIME    = 0xFFFFFFFF,
      GPS_INVALID_SATELLITES  = 0xFF,
      GPS_INVALID_HDOP        = 0xFFFFFFFF
    };


    TinyGPS()
      :  _time(GPS_INVALID_TIME)
      ,  _date(GPS_INVALID_DATE)
      ,  _latitude(GPS_INVALID_ANGLE)
      ,  _longitude(GPS_INVALID_ANGLE)
      ,  _altitude(GPS_INVALID_ALTITUDE)
      ,  _speed(GPS_INVALID_SPEED)
      ,  _course(GPS_INVALID_ANGLE)
      ,  _hdop(GPS_INVALID_HDOP)
      ,  _numsats(GPS_INVALID_SATELLITES)
      ,  _last_time_fix(GPS_INVALID_FIX_TIME)
      ,  _last_position_fix(GPS_INVALID_FIX_TIME)
      ,  _parity(0)
      ,  _is_checksum_term(false)
      ,  _sentence_type(_GPS_SENTENCE_OTHER)
      ,  _term_number(0)
      ,  _term_offset(0)
      ,  _gps_data_good(false)
    {
      _term[0] = '\0';
    }

    bool encode(char c) { // process one character received from GPS

      bool valid_sentence = false;

      switch (c) {
        case ',': // term terminators
          _parity ^= c;
        case '\r':
        case '\n':
        case '*':
          if (_term_offset < sizeof(_term)) {
            _term[_term_offset] = 0;
            valid_sentence      = term_complete();
          }
          ++_term_number;
          _term_offset          = 0;
          _is_checksum_term     = c == '*';
          return valid_sentence;

        case '$': // sentence begin
          _term_number          = _term_offset = 0;
          _parity               = 0;
          _sentence_type        = _GPS_SENTENCE_OTHER;
          _is_checksum_term     = false;
          _gps_data_good        = false;
          return valid_sentence;
      }

      // ordinary characters
      if (_term_offset < sizeof(_term) - 1)
        _term[_term_offset++] = c;
      if (!_is_checksum_term)
        _parity ^= c;

      return valid_sentence;
    }


    TinyGPS &operator << (char c) {
      encode(c);
      return *this;
    }



    // lat/long in MILLIONTHs of a degree and age of fix in milliseconds
    // (note: versions 12 and earlier gave this value in 100,000ths of a degree.
    void get_position(long *latitude, long *longitude, unsigned long *fix_age = 0) {
      if (latitude)   *latitude   = _latitude;
      if (longitude)  *longitude  = _longitude;
      if (fix_age)    *fix_age    = _last_position_fix == GPS_INVALID_FIX_TIME ? GPS_INVALID_AGE : millis() - _last_position_fix;
    }


    // date as ddmmyy, time as hhmmsscc, and age in milliseconds
    void get_datetime(unsigned long *date, unsigned long *time, unsigned long *age = 0) {
      if (date) *date = _date;
      if (time) *time = _time;
      if (age)  *age  = _last_time_fix == GPS_INVALID_FIX_TIME ? GPS_INVALID_AGE : millis() - _last_time_fix;
    }

    // signed altitude in centimeters (from GPGGA sentence)
    inline long altitude() {
      return _altitude;
    }

    // course in last full GPRMC sentence in 100th of a degree
    inline unsigned long course() {
      return _course;
    }

    // speed in last full GPRMC sentence in 100ths of a knot
    inline unsigned long speed() {
      return _speed;
    }

    // satellites used in last full GPGGA sentence
    inline unsigned short satellites() {
      return _numsats;
    }

    // horizontal dilution of precision in 100ths
    inline unsigned long hdop() {
      return _hdop;
    }




    void f_get_position(float *latitude, float *longitude, unsigned long *fix_age = 0) {
      long lat, lon;
      get_position(&lat, &lon, fix_age);
      *latitude   = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lat / 1000000.0);
      *longitude  = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lon / 1000000.0);
    }


    void crack_datetime(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *hundredths = 0, unsigned long *age = 0) {
      unsigned long date, time;
      get_datetime(&date, &time, age);
      if (year) {
        *year = date % 100;
        *year += *year > 80 ? 1900 : 2000;
      }
      if (month)      *month      = (date /     100) % 100;
      if (day)        *day        =  date /   10000;
      if (hour)       *hour       =  time / 1000000;
      if (minute)     *minute     = (time /   10000) % 100;
      if (second)     *second     = (time /     100) % 100;
      if (hundredths) *hundredths =  time %     100;
    }


    float f_altitude() {
      return _altitude == GPS_INVALID_ALTITUDE ? GPS_INVALID_F_ALTITUDE : _altitude / 100.0;
    }


    float f_course() {
      return _course == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : _course / 100.0;
    }


    float f_speed_knots() {
      return _speed == GPS_INVALID_SPEED ? GPS_INVALID_F_SPEED : _speed / 100.0;
    }


    float f_speed_mph() {
      float sk = f_speed_knots();
      return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPH_PER_KNOT * sk;
    }


    float f_speed_mps() {
      float sk = f_speed_knots();
      return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPS_PER_KNOT * sk;
    }


    float f_speed_kmph() {
      float sk = f_speed_knots();
      return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_KMPH_PER_KNOT * sk;
    }

    static int library_version() {
      return _GPS_VERSION;
    }

    static float distance_between (float lat1, float long1, float lat2, float long2) {
      // returns distance in meters between two positions, both specified
      // as signed decimal-degrees latitude and longitude. Uses great-circle
      // distance computation for hypothetical sphere of radius 6372795 meters.
      // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
      // Courtesy of Maarten Lamers
      float delta   = radians(long1 - long2);
      float sdlong  = sin(delta);
      float cdlong  = cos(delta);
      lat1          = radians(lat1);
      lat2          = radians(lat2);
      float slat1   = sin(lat1);
      float clat1   = cos(lat1);
      float slat2   = sin(lat2);
      float clat2   = cos(lat2);
      delta         = (clat1 * slat2) - (slat1 * clat2 * cdlong);
      delta         = sq(delta);
      delta        += sq(clat2 * sdlong);
      delta         = sqrt(delta);
      float denom   = (slat1 * slat2) + (clat1 * clat2 * cdlong);
      delta         = atan2(delta, denom);
      return delta * 6372795;
    }


    static float course_to (float lat1, float long1, float lat2, float long2) {
      // returns course in degrees (North=0, West=270) from position 1 to position 2,
      // both specified as signed decimal-degrees latitude and longitude.
      // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
      // Courtesy of Maarten Lamers
      float dlon  = radians(long2 - long1);
      lat1        = radians(lat1);
      lat2        = radians(lat2);
      float a1    = sin(dlon) * cos(lat2);
      float a2    = sin(lat1) * cos(lat2) * cos(dlon);
      a2          = cos(lat1) * sin(lat2) - a2;
      a2          = atan2(a1, a2);
      if (a2 < 0.0) {
        a2 += TWO_PI;
      }
      return degrees(a2);
    }


    static const char *cardinal (float course) {
      static const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};

      int direction = (int)((course + 11.25f) / 22.5f);
      return directions[direction % 16];
    }
    
  private:
    enum {_GPS_SENTENCE_GPGGA, _GPS_SENTENCE_GPRMC, _GPS_SENTENCE_OTHER};

    // properties
    unsigned long   _time,              _new_time;
    unsigned long   _date,              _new_date;
    long            _latitude,          _new_latitude;
    long            _longitude,         _new_longitude;
    long            _altitude,          _new_altitude;
    unsigned long   _speed,             _new_speed;
    unsigned long   _course,            _new_course;
    unsigned long   _hdop,              _new_hdop;
    unsigned short  _numsats,           _new_numsats;

    unsigned long   _last_time_fix,     _new_time_fix;
    unsigned long   _last_position_fix, _new_position_fix;

    // parsing state variables
    byte _parity;
    bool _is_checksum_term;
    char _term[15];
    byte _sentence_type;
    byte _term_number;
    byte _term_offset;
    bool _gps_data_good;


    // internal utilities
    int from_hex(char a) {
      if (a >= 'A' && a <= 'F')
        return a - 'A' + 10;
      else if (a >= 'a' && a <= 'f')
        return a - 'a' + 10;
      else
        return a - '0';
    }


    unsigned long parse_decimal() {
      char *p = _term;
      bool isneg = *p == '-';
      if (isneg) ++p;
      unsigned long ret = 100UL * gpsatol(p);
      while (gpsisdigit(*p)) ++p;
      if (*p == '.') {
        if (gpsisdigit(p[1])) {
          ret += 10 * (p[1] - '0');
          if (gpsisdigit(p[2]))
            ret += p[2] - '0';
        }
      }
      return isneg ? -ret : ret;
    }


    // Parse a string in the form ddmm.mmmmmmm...
    unsigned long parse_degrees() {
      char *p;
      unsigned long left_of_decimal = gpsatol(_term);
      unsigned long hundred1000ths_of_minute = (left_of_decimal % 100UL) * 100000UL;
      for (p = _term; gpsisdigit(*p); ++p);
      if (*p == '.') {
        unsigned long mult = 10000;
        while (gpsisdigit(*++p)) {
          hundred1000ths_of_minute += mult * (*p - '0');
          mult /= 10;
        }
      }
      return (left_of_decimal / 100) * 1000000 + (hundred1000ths_of_minute + 3) / 6;
    }

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)


    // Processes a just-completed term
    // Returns true if new sentence has just passed checksum test and is validated
    bool term_complete() {
      if (_is_checksum_term) {
        byte checksum = 16 * from_hex(_term[0]) + from_hex(_term[1]);
        if (checksum == _parity) {
          if (_gps_data_good) {
            _last_time_fix      = _new_time_fix;
            _last_position_fix  = _new_position_fix;

            switch (_sentence_type) {
              case _GPS_SENTENCE_GPRMC:
                _time      = _new_time;
                _date      = _new_date;
                _latitude  = _new_latitude;
                _longitude = _new_longitude;
                _speed     = _new_speed;
                _course    = _new_course;
                break;
              case _GPS_SENTENCE_GPGGA:
                _altitude  = _new_altitude;
                _time      = _new_time;
                _latitude  = _new_latitude;
                _longitude = _new_longitude;
                _numsats   = _new_numsats;
                _hdop      = _new_hdop;
                break;
            }

            return true;
          }
        }

        return false;
      }

      // the first term determines the sentence type
      if (_term_number == 0) {
        if (!gpsstrcmp(_term, _GPRMC_TERM))
          _sentence_type = _GPS_SENTENCE_GPRMC;
        else if (!gpsstrcmp(_term, _GPGGA_TERM))
          _sentence_type = _GPS_SENTENCE_GPGGA;
        else
          _sentence_type = _GPS_SENTENCE_OTHER;
        return false;
      }

      if (_sentence_type != _GPS_SENTENCE_OTHER && _term[0])
        switch (COMBINE(_sentence_type, _term_number)) {
          case COMBINE(_GPS_SENTENCE_GPRMC, 1): // Time in both sentences
          case COMBINE(_GPS_SENTENCE_GPGGA, 1):
            _new_time     = parse_decimal();
            _new_time_fix = millis();
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 2): // GPRMC validity
            _gps_data_good = _term[0] == 'A';
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 3): // Latitude
          case COMBINE(_GPS_SENTENCE_GPGGA, 2):
            _new_latitude     = parse_degrees();
            _new_position_fix = millis();
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 4): // N/S
          case COMBINE(_GPS_SENTENCE_GPGGA, 3):
            if (_term[0] == 'S')
              _new_latitude = -_new_latitude;
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 5): // Longitude
          case COMBINE(_GPS_SENTENCE_GPGGA, 4):
            _new_longitude = parse_degrees();
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 6): // E/W
          case COMBINE(_GPS_SENTENCE_GPGGA, 5):
            if (_term[0] == 'W')
              _new_longitude = -_new_longitude;
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 7): // Speed (GPRMC)
            _new_speed = parse_decimal();
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 8): // Course (GPRMC)
            _new_course = parse_decimal();
            break;
          case COMBINE(_GPS_SENTENCE_GPRMC, 9): // Date (GPRMC)
            _new_date = gpsatol(_term);
            break;
          case COMBINE(_GPS_SENTENCE_GPGGA, 6): // Fix data (GPGGA)
            _gps_data_good = _term[0] > '0';
            break;
          case COMBINE(_GPS_SENTENCE_GPGGA, 7): // Satellites used (GPGGA)
            _new_numsats = (unsigned char)atoi(_term);
            break;
          case COMBINE(_GPS_SENTENCE_GPGGA, 8): // HDOP
            _new_hdop = parse_decimal();
            break;
          case COMBINE(_GPS_SENTENCE_GPGGA, 9): // Altitude (GPGGA)
            _new_altitude = parse_decimal();
            break;
        }

      return false;
    }


    bool gpsisdigit(char c) {
      return c >= '0' && c <= '9';
    }


    long gpsatol(const char *str) {
      long ret = 0;
      while (gpsisdigit(*str))
        ret = 10 * ret + *str++ - '0';
      return ret;
    }


    int gpsstrcmp(const char *str1, const char *str2) {
      while (*str1 && *str1 == *str2)
        ++str1, ++str2;
      return *str1;
    }


};

#if !defined(ARDUINO)
// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round
#endif

#endif







#ifndef GPS_time_h                                                                                            // 
#define GPS_time_h                                                                                            // đánh dấu đẫ đọc

#define UUNIXDATE_BASE 946684800                                                                              // thời gian bù trừ từ ngày 1/1/1970 đến ngày 1/1/2000
class GPS_time: public TinyGPS {                                                                              // thư viện GPS time được sử dụng hàm của TinyGPS
  public:                                                                                                     // biến mảng toàn cục



    RTCDateTime getDateTime(void) {                                                                           // hàm đọc thời gian
      RTCDateTime new_time;                                                                                   // tạo mảng lưu thòi gian
      crack_datetime(&new_time.year, &new_time.month, &new_time.day, &new_time.hour, &new_time.minute, &new_time.second);// đọc thời gian từ GPS
      unsigned long new_time_unixtime = ConverterDateTimeToUnixtime(new_time) + 7ul * 3600ul;                 // chuyển thành dạng thời gian unixtime + zone


      if (new_time_unixtime > old_read_unixtime) {                                                            // nếu thời gian mới đọc được lớn hơn thời gian đọc lần trước
        Time_long         = new_time_unixtime;                                                                // cập nhật thời gian mới
        Time_read         = millis();                                                                         // cập nhật thời gian đọc
        if (Time_long_offset == 0) {                                                                          // nếu chưa có dữ liệu
          Time_long_offset = new_time_unixtime;                                                               // cập nhật thời gian mới
          Time_read_offset = millis();                                                                        // cập nhật thời gian đọc
        } else if ((Time_long - Time_long_offset > 60) && (Time_long - Time_long_offset < 14ul * 24ul * 3600ul)) {// nếu khoảng cách giữa hai thời gian kiểm tra đủ lâu
          Time_scale = double(Time_long - Time_long_offset) / (double(Time_read - Time_read_offset) / 1000.0); // tính sai số thạch anh
        }                                                                                                     //
      }                                                                                                       //

      old_read_unixtime = new_time_unixtime;                                                                  // đánh dấu đã đọc
      if (Time_read == 0) return ConverterDateTimeToRTCDateTime(0, 0, 0, 0, 0, 0);                            // nêu chưa từng có dữ liệu trả về thời gian bằng 0
      if (Time_read > millis()) Time_read = millis();                                                         // nếu bị tràn
      if ((millis() - Time_read)* Time_scale >= 60000) {
        Time_read += int(30000 / Time_scale);
        Time_long += 30;
      }

   //   lcd.setCursor(0, 1);                                                                      // đặt con trỏ
   //   lcd.print(30000 / Time_scale, 6);                                                           // hiển thị số đt
      return ConverterUnixtimeToDateTime(Time_long + int((millis() - Time_read) / 1000.0 * Time_scale)); // trả thời gian được tính toán
    }                                                                                                         //



    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////converter time

    uint32_t ConverterDateTimeToUnixtime(RTCDateTime dt) {                                                    // hàm chuyển về Unixtime
      return ConverterDateTimeToUnixtime(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);           // hàm chuyển về Unixtime
    }

    uint32_t ConverterDateTimeToUnixtime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) { // hàm chuyển về Unixtime
      int32_t days = day - 1;                                                                                 // trừ đi một ngày
      for (int i = 1; i < month; ++i) {                                                                       // lặp các thàng
        if (i == 2)  {                                                                                        // nếu là thàng 2
          days += (28 + isLeapYear(year));                                                                    // có 28 hoặc 29 ngày
        } else if (i == 4 || i == 6 || i == 9 || i == 11) {                                                   // nếu là thàng 4,6,9,11
          days += 30;                                                                                         // có 30 ngày
        } else {                                                                                              // các thàng còn lại
          days += 31;                                                                                         // 31 ngày
        }                                                                                                     //
      }                                                                                                       //
      for (int i = 2000; i < year; ++i) {                                                                     // lặp các năm từ năm 2000
        days += (365 + isLeapYear(i));                                                                        // tính số ngày từng năm
      }                                                                                                       //

      return UUNIXDATE_BASE + (((days * 24 + hour) * 60 + minute) * 60) + second;                             // chuyển về Unixtime
    }                                                                                                         //

    RTCDateTime ConverterDateTimeToRTCDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) { // hàm chuyển kiểu dữ liệu
      RTCDateTime t;                                                                                          // biến đệm lưu trữ
      t.year    = year;                                                                                       // lưu
      t.month   = month;                                                                                      // lưu
      t.day     = day;                                                                                        // lưu
      t.hour    = hour;                                                                                       // lưu
      t.minute  = minute;                                                                                     // lưu
      t.second  = second;                                                                                     // lưu
      return t;                                                                                               // trả nề kết quả
    }                                                                                                         //



    RTCDateTime ConverterUnixtimeToDateTime(uint32_t ut) {                                                    // hàm chuyển Unixtime về Date Time
      RTCDateTime t;                                                                                          // biến đệm lưu dữ liệu

      t.unixtime = ut;                                                                                        // lưu Unixtime
      t.dayOfWeek = ((ut / 86400) + 4) % 7;                                                                   // tính thứ của tuần
      uint32_t tt = ut - UUNIXDATE_BASE;                                                                      // chuyển Unixtime từ 1970 về 2000
      t.second = tt % 60;                                                                                     // tính số giây
      tt /= 60;                                                                                               // chia 60
      t.minute = tt % 60;                                                                                     // tính số phút
      tt /= 60;                                                                                               // chia 60
      t.hour = tt % 24;                                                                                       // tính số giờ
      int16_t days = tt / 24;                                                                                 // tính tổng số ngày
      bool leap;                                                                                              // biến đệm kiểm tra năm nhuận
      t.year = 2000;                                                                                          // năm bắt đầu từ năm 2000
      while (true) {                                                                                          // lặp đến khi đủ điều kiện
        leap = isLeapYear(t.year);                                                                            // tính xem có phải năm nhuận không
        if (days < 365 + leap) {                                                                              // nếu số ngày còn lại nhỏ hơn một năm
          break;                                                                                              // thoát while
        }                                                                                                     //
        days -= (365 + leap);                                                                                 // trừ đi số ngày của năm
        ++t.year;                                                                                             // cộng năm thêm 1
      }                                                                                                       //

      for (t.month = 1; t.month < 12; ++t.month) {                                                            // lặp các tháng
        int8_t daysPerMonth = 31;                                                                             // mặc định là 31 ngày
        if (t.month == 2)  {                                                                                  // nếu là thàng 2
          daysPerMonth = leap ? 29 : 28;                                                                      // có 28 hoặc 29 ngày
        } else if (t.month == 4 || t.month == 6 || t.month == 9 || t.month == 11) {                           // nếu là thàng 4,6,9,11
          daysPerMonth = 30;                                                                                  // có 30 ngày
        }                                                                                                     //
        if (days < daysPerMonth) {                                                                            // nếu số ngày còn lại nhỏ hơn một thàng
          break;                                                                                              // thoát for
        }                                                                                                     //
        days -= daysPerMonth;                                                                                 // trừ đi số ngày của tháng
      }                                                                                                       //
      t.day = days + 1;                                                                                       // tính ngày của tháng

      return t;                                                                                               // trả về kết quả
    }                                                                                                         //



    const char *strDayOfWeek(uint8_t dayOfWeek) {
      switch (dayOfWeek) {
        case 1:   return "Monday";    break;
        case 2:   return "Tuesday";   break;
        case 3:   return "Wednesday"; break;
        case 4:   return "Thursday";  break;
        case 5:   return "Friday";    break;
        case 6:   return "Saturday";  break;
        case 7:   return "Sunday";    break;
        default:  return "Unknown";
      }
    }



    const char *strMonth(uint8_t month) {
      switch (month) {
        case 1:   return "January";   break;
        case 2:   return "February";  break;
        case 3:   return "March";     break;
        case 4:   return "April";     break;
        case 5:   return "May";       break;
        case 6:   return "June";      break;
        case 7:   return "July";      break;
        case 8:   return "August";    break;
        case 9:   return "September"; break;
        case 10:  return "October";   break;
        case 11:  return "November";  break;
        case 12:  return "December";  break;
        default:  return "Unknown";
      }
    }

    bool isLeapYear(uint16_t year) {
      return  ((year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 1 : 0);
    }

    byte isERRO() {
      if (Time_read == 0) return 2;
      return 0;
    }



    double        Time_scale       = 1.0;
  private:
    unsigned long Time_read         = 0;
    unsigned long Time_long         = 0;
    unsigned long old_read_unixtime = 0xFFFFFFFF;

    unsigned long Time_read_offset  = 0;
    unsigned long Time_long_offset  = 0;

};

#endif
