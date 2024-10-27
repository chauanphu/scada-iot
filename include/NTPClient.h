#pragma once                                                                // chỉ đọc một lần


#include "Arduino.h"                                                        // thư viện arduino
#include <Udp.h>                                                            // thư viện UDP

#define SEVENZYYEARS            2208988800UL                                //
#define NTP_PACKET_SIZE         48                                          //
#define NTP_DEFAULT_LOCAL_PORT  1337                                        //

const uint8_t daysArray[]    PROGMEM = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };// số ngày có trong tháng
const uint8_t dowArray[]     PROGMEM = { 0,   3,  2,  5,  0,  3,  5,  1,  4,  6,  2,  4 };// mảng chuyển đổi thứ của tuần
// const char*   strMonth[]     PROGMEM = {"Unknown", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
// const char*   strDayOfWeek[] PROGMEM = {"Unknown", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};



#ifndef __RTCDateTime__                                                                                       // nếu mảng lưu thời gian chưa được tạo
#define __RTCDateTime__                                                                                       // đánh dấu đã tạo
struct RTCDateTime {                                                        // mảng dữ liệu lưu thời gian
  uint16_t year      = 0;                                                   // năm
  uint8_t  month     = 0;                                                   // tháng
  uint8_t  day       = 0;                                                   // ngày
  uint8_t  hour      = 0;                                                   // giờ
  uint8_t  minute    = 0;                                                   // phút
  uint8_t  second    = 0;                                                   // giây
  uint8_t  dayOfWeek = 0;                                                   // thứ
  uint32_t unixtime  = 0;                                                   // thời gian dài tính bằng giây từ ngày 1/1/1970
};                                                                          //
#endif                                                                                                        //

class NTPClient {                                                           //
  public:
    NTPClient(UDP& udp) {
      this->_udp            = &udp;
    }
    NTPClient(UDP& udp, long timeOffset) {
      this->_udp            = &udp;
      this->_timeOffset     = timeOffset;
    }
    NTPClient(UDP& udp, const char* poolServerName) {
      this->_udp            = &udp;
      this->_poolServerName = poolServerName;
    }
    NTPClient(UDP& udp, const char* poolServerName, long timeOffset) {
      this->_udp            = &udp;
      this->_timeOffset     = timeOffset;
      this->_poolServerName = poolServerName;
    }
    NTPClient(UDP& udp, const char* poolServerName, long timeOffset, unsigned long updateInterval) {
      this->_udp            = &udp;
      this->_timeOffset     = timeOffset;
      this->_poolServerName = poolServerName;
      this->_updateInterval = updateInterval;
    }
    NTPClient(UDP& udp, IPAddress poolServerIP) {
      this->_udp            = &udp;
      this->_poolServerIP   = poolServerIP;
      this->_poolServerName = NULL;
    }
    NTPClient(UDP& udp, IPAddress poolServerIP, long timeOffset) {
      this->_udp            = &udp;
      this->_timeOffset     = timeOffset;
      this->_poolServerIP   = poolServerIP;
      this->_poolServerName = NULL;
    }
    NTPClient(UDP& udp, IPAddress poolServerIP, long timeOffset, unsigned long updateInterval) {
      this->_udp            = &udp;
      this->_timeOffset     = timeOffset;
      this->_poolServerIP   = poolServerIP;
      this->_poolServerName = NULL;
      this->_updateInterval = updateInterval;
    }
    void setPoolServerName(const char* poolServerName) {//       Set time server name
      this->_poolServerName = poolServerName;
    }



    void setRandomPort(unsigned int minValue = 49152, unsigned int maxValue = 65535) {//      Set random local port
      randomSeed(analogRead(0));
      this->_port = random(minValue, maxValue);
    }

    void begin() {                                            // Starts the underlying UDP client with the default local port
      this->begin(NTP_DEFAULT_LOCAL_PORT);
    }

    void begin(unsigned int port) {//Starts the underlying UDP client with the specified local port
      this->_port = port;
      this->_udp->begin(this->_port);
      this->_udpSetup = true;
    }

    bool update() {                                                                                                           //  This should be called in the main loop of your application. By default an update from the NTP Server is only made every 60 seconds. This can be configured in the NTPClient constructor.
      if ((millis() - this->_lastUpdate >= this->_updateInterval) || this->_lastUpdate == 0) {                                 // Update after _updateInterval  // Update if there was no update yet.
        if (!this->_udpSetup || this->_port != NTP_DEFAULT_LOCAL_PORT) this->begin(this->_port); // setup the UDP client if needed
        return this->forceUpdate();
      }
      return false;   // return false if update does not occur
    }

    bool forceUpdate() {                                                                      //       This will force the update from the NTP Server.

      // flush any existing packets
      while (this->_udp->parsePacket() != 0)
        this->_udp->flush();

      this->sendNTPPacket();

      // Wait till data is there or timeout...
      byte  timeout = 0;
      int   cb      = 0;
      do {
        delay ( 10 );
        cb = this->_udp->parsePacket();
        if (timeout > 100) return false; // timeout after 1000 ms
        timeout++;
      } while (cb == 0);

      this->_lastUpdate = millis() - (10 * (timeout + 1)); // Account for delay in reading the time

      this->_udp->read(this->_packetBuffer, NTP_PACKET_SIZE);

      unsigned long highWord = word(this->_packetBuffer[40], this->_packetBuffer[41]);
      unsigned long lowWord  = word(this->_packetBuffer[42], this->_packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;

      this->_currentEpoc = secsSince1900 - SEVENZYYEARS;

      return true;  // return true after successful update
    }

    bool isTimeSet() const {//       This allows to check if the NTPClient successfully received a NTP packet and set the time.       @return true if time has been set, else false
      return (this->_lastUpdate != 0); // returns true if the time has been set, else false
    }

    void setTimeOffset(int timeOffset) {//Changes the time offset. Useful for changing timezones dynamically
      this->_timeOffset     = timeOffset;
    }

    void setUpdateInterval(unsigned long updateInterval) {//       Set the update interval to another frequency. E.g. useful when the timeOffset should not be set in the constructor
      this->_updateInterval = updateInterval;
    }












    int getDay() const {
      return (((this->getEpochTime()  / 86400L) + 4 ) % 7); //0 is Sunday
    }
    int getHours() const {
      return ((this->getEpochTime()  % 86400L) / 3600);
    }
    int getMinutes() const {
      return ((this->getEpochTime() % 3600) / 60);
    }
    int getSeconds() const {
      return (this->getEpochTime() % 60);
    }

    RTCDateTime getDateTime(void) {                                                                           // hàm đọc thời gian
      return this->ConverterUnixtimeToDateTime(this->getEpochTime());                                                        // trả thời gian được tính toán
    }

    uint32_t ConverterDateTimeToUnixtime(RTCDateTime dt) {
      return ConverterDateTimeToUnixtime(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    }

    uint32_t ConverterDateTimeToUnixtime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
      return time2long(date2days(year, month, day), hour, minute, second);
    }


    RTCDateTime ConverterUnixtimeToDateTime(uint32_t ut) {
      RTCDateTime t;

      t.second = ut % 60;
      ut /= 60;

      t.minute = ut % 60;
      ut /= 60;

      t.hour = ut % 24;
      uint16_t days = ut / 24;
      uint8_t  leap;

      for (t.year = 0; ; ++t.year) {
        leap = (t.year + 2) % 4 == 0;
        if (days < 365 + leap) break;
        days -= 365 + leap;
      }
      t.year += 1970;

      for (t.month = 1; ; ++t.month) {
        uint8_t daysPerMonth = pgm_read_byte(daysArray + t.month - 1);
        if (leap && t.month == 2) ++daysPerMonth;
        if (days < daysPerMonth) break;
        days -= daysPerMonth;
      }

      t.day = days + 1;

      t.dayOfWeek = dow(t.year, t.month, t.day);
      t.unixtime  = ConverterDateTimeToUnixtime(t);

      return t;
    }



    long time2long(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds) {
      return ((days * 24L + hours) * 60 + minutes) * 60 + seconds;
    }

    uint16_t date2days(uint16_t year, uint8_t month, uint8_t day) {
      year = year - 1970;
      uint16_t days16 = day;
      for (uint8_t i = 1; i < month; ++i)
        days16 += pgm_read_byte(daysArray + i - 1);
      if ((month == 2) && isLeapYear(year)) ++days16;
      return days16 + 365 * year + (year + 3) / 4 - 1;
    }

    uint8_t daysInMonth(uint16_t year, uint8_t month) {
      uint8_t days;
      days = pgm_read_byte(daysArray + month - 1);
      if ((month == 2) && isLeapYear(year)) ++days;
      return days;
    }

    uint16_t dayInYear(uint16_t year, uint8_t month, uint8_t day) {
      uint16_t fromDate;
      uint16_t toDate;
      fromDate  = date2days(year, 1, 1);
      toDate    = date2days(year, month, day);
      return (toDate - fromDate);
    }

    uint8_t dow(uint16_t y, uint8_t m, uint8_t d) {
      uint8_t dow;
      y -= m < 3;
      dow = ((y + y / 4 - y / 100 + y / 400 + pgm_read_byte(dowArray + (m - 1)) + d) % 7);
      if (dow == 0) return 7;
      return dow;
    }

    String getFormattedTime() const {//@return time formatted like `hh:mm:ss`
      unsigned long   rawTime     = this->getEpochTime();
      unsigned long   hours       = (rawTime % 86400L) / 3600;
      String          hoursStr    = hours   < 10 ? "0" + String(hours)   : String(hours);
      unsigned long   minutes     = (rawTime % 3600) / 60;
      String          minuteStr   = minutes < 10 ? "0" + String(minutes) : String(minutes);
      unsigned long   seconds     = rawTime % 60;
      String          secondStr   = seconds < 10 ? "0" + String(seconds) : String(seconds);
      return          hoursStr + ":" + minuteStr + ":" + secondStr;
    }

    unsigned long getEpochTime() const {                                                        //@return time in seconds since Jan. 1, 1970
      return this->_timeOffset + this->_currentEpoc + ((millis() - this->_lastUpdate) / 1000);  // User offset // Epoch returned by the NTP server // Time since last update
    }

    void end() {                                                                                // Stops the underlying UDP client
      this->_udp->stop();
      this->_udpSetup = false;
    }

  private:                                                                                       // mảng lưu dữ liệu

    UDP*          _udp;
    bool          _udpSetup         = false;

    const char*   _poolServerName   = "pool.ntp.org"; // Default time server
    IPAddress     _poolServerIP;
    unsigned int  _port             = NTP_DEFAULT_LOCAL_PORT;
    long          _timeOffset       = 0;

    unsigned long _updateInterval   = 60000;  // In ms

    unsigned long _currentEpoc      = 0;      // In s
    unsigned long _lastUpdate       = 0;      // In ms

    byte          _packetBuffer[NTP_PACKET_SIZE];

    void sendNTPPacket() {
      // set all bytes in the buffer to 0
      memset(this->_packetBuffer, 0, NTP_PACKET_SIZE);
      // Initialize values needed to form NTP request
      this->_packetBuffer[0]    = 0b11100011;   // LI, Version, Mode
      this->_packetBuffer[1]    = 0;     // Stratum, or type of clock
      this->_packetBuffer[2]    = 6;     // Polling Interval
      this->_packetBuffer[3]    = 0xEC;  // Peer Clock Precision
      // 8 bytes of zero for Root Delay & Root Dispersion
      this->_packetBuffer[12]   = 49;
      this->_packetBuffer[13]   = 0x4E;
      this->_packetBuffer[14]   = 49;
      this->_packetBuffer[15]   = 52;

      // all NTP fields have been given values, now
      // you can send a packet requesting a timestamp:
      if  (this->_poolServerName) {
        this->_udp->beginPacket(this->_poolServerName, 123);
      } else {
        this->_udp->beginPacket(this->_poolServerIP, 123);
      }
      this->_udp->write(this->_packetBuffer, NTP_PACKET_SIZE);
      this->_udp->endPacket();
    }


    bool isLeapYear(uint16_t year) {
      return ((year + 2) % 4 == 0);
    }


};
