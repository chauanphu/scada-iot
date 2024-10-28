#ifndef MODBUS_H
#define MODBUS_H

#include <Arduino.h>
#include <Stream.h>
#include <vector>
using namespace std;

#define Coil_Register       0x01
#define Discret_Register    0x02
#define Holding_Register    0x03
#define Input_Register      0x04

class Modbus {
  private:
    /* data */
    bool      log           = false;
    int       mode_         = -1;
    uint32_t  timeout_      = 100;
    Stream*   s ;
    byte      rawRx[512];
    int       lenRx         = 0;
    byte      dataRx[512];
    int       datalen       = 0;
    int       SlaveID       = 0x01;
    byte      txout[9]      = {0, 0, 0, 0, 0, 0, 0, 0, 0};

  public:
    Modbus() {
      this->s     = NULL;
      this->mode_ = -1;
    }
    Modbus(Stream &st) {
      this->s = &st;
    }
    bool init(int mode = -1, bool en_log = false) {
      this->mode_ =  mode;
      this->log   =  en_log;
      if (mode_ != -1) {
        pinMode(mode_, OUTPUT);
        digitalWrite(mode_, 0);
      }
      return true;
    }

    void setTimeout(uint16_t timeout) {
      timeout_ = timeout;
    }

    int coilRead(int address) {
      return coilRead(SlaveID, address);
    }

    int coilRead(int slaveId, int address) {
      if (requestFrom(slaveId, Coil_Register, address, 1)) {
        byte x = byteRead(0);
        return bitRead(x, 0);
      } else {
        return -1;
      }
    }

    // Read Discret Register    0x02
    int discreteInputRead(int address) {
      return discreteInputRead(SlaveID, address);
    }

    int discreteInputRead(int slaveId, int address) {
      if (requestFrom(slaveId, Discret_Register, address, 1)) {
        byte x = byteRead(0);
        return bitRead(x, 0);
      } else {
        return -1;
      }
    }

    int ReadDiscretReg(int address) {
      return ReadDiscretReg(1, address, 1);
    }

    int ReadDiscretReg(int slaveId, int address) {
      return ReadDiscretReg(slaveId, address, 1);
    }

    int ReadDiscretReg(int slaveId, int address, int nbit) {
      if (requestFrom(slaveId, Discret_Register, address, nbit)) {
        return byteRead(0);
      } else {
        return -1;
      }
    }

    // Read Holding Register    0x03
    long holdingRegisterRead(int address) {
      return holdingRegisterRead(SlaveID, address, 1);
    }

    long holdingRegisterRead(int slaveId, int address, int block) {
      if (block > 2) block = 2;
      if (requestFrom(SlaveID, Holding_Register, address, block)) {
        if (block == 2) {
          return (blockRead(0) << 16 | blockRead(1));
        } else {
          return blockRead(0);
        }
      } else {
        return -1;
      }
    }

    int ReadHoldingReg(int address) {
      return 0;
    }

    int ReadHoldingReg(int slaveId, int address) {
      return 0;
    }

    int ReadHoldingReg(int slaveId, int address, int nbyte) {
      return 0;
    }
    // Read Input Register      0x04
    long inputRegisterRead(int address) {
      return inputRegisterRead(SlaveID , address, 1);
    }

    long inputRegisterRead(int slaveId, int address, int block) {
      if (block > 2) block = 2;
      if (requestFrom(slaveId, Input_Register, address, block)) {
        if (block == 2) {
          return (blockRead(0) << 16 | blockRead(1));
        } else {
          return blockRead(0);
        }
      } else {
        return -1;
      }
    }

    int ReadInputReg(int address) {
      return 0;
    }

    int ReadInputReg(int slaveId, int address) {
      return 0;
    }

    int ReadInputReg(int slaveId, int address, int nbyte) {
      return 0;
    }

    int coilWrite(int address, uint8_t value);
    int coilWrite(int slaveId, int address, uint8_t value);
    int holdingRegisterWrite(int address, uint16_t value);
    int holdingRegisterWrite(int slaveId, int address, uint16_t value);

    void RxRaw(byte *raw, uint8_t &rlen) {
      for (int i = 0; i < lenRx; i++)
        raw[i] = rawRx[i];
      rlen = this->lenRx;
    }

    void TxRaw(byte *raw, uint8_t &rlen) {
      for (int i = 0; i < 8; i++)
        raw[i] = txout[i];
      rlen = 8;
    }

    //Read multiple coils, discrete inputs, holding registers, or input register values.
    //int requestFrom(int type, int address, int nb, byte *ret,int len);
    int requestFrom(int slaveId, int type, int address, int nb) {

      SlaveID   = slaveId;
      txout[0]  = slaveId;
      txout[1]  = type;
      txout[2]  = address >> 8;
      txout[3]  = address;
      txout[4]  = nb >> 8;
      txout[5]  = nb;
      int crc   = this->CheckCRC(txout, 6);
      txout[6]  = crc ;
      txout[7]  = crc >> 8;


      if (log) {
        Serial.print("TX: ");
        //        cmd.print("TX: ");
        for (int i = 0; i < 8; i++) {
          Serial.printf("%02X ", txout[i] );
          //          cmd.printf("%02X ", txout[i] );
        }
        Serial.print("\t");
        //        cmd.print("\t");
      }

      if (mode_ != -1) digitalWrite(mode_, 1);
      delay(1);
      this->s->write(txout, 8);
      this->s->flush();
      if (mode_ != -1) digitalWrite(mode_, 0);
      delay(1);
      uint32_t  t       = millis();
      lenRx             = 0;
      datalen           = 0;
      int       ll      = 0;
      int       rx;
      byte      found   = 0;

      while ((millis() - t) < timeout_) {
        if (this->s->available()) {
          rx  = this->s->read();
          t   = millis();

          if (found == 0) {
            if (txout[ll] == rx) {
              ll++;
            } else {
              ll = 0;
            }
            if (ll == 2) {
              found = 1;
            }
          } else if (found == 1) {
            rawRx[0]  = txout[0];
            rawRx[1]  = txout[1];
            rawRx[2]  = rx;
            lenRx     = 3;
            found     = 2;
          } else if (found == 2) {
            this->rawRx[lenRx++] =  rx;
            if (lenRx >= rawRx[2] + 5) {
              break;
            }
          }
        }
      }
      if (log) {
        Serial.print("RX: ");
        //        cmd.print("RX: ");
        for (int i = 0; i < lenRx; i++) {
          Serial.printf("%02X ", rawRx[i] );
          //          cmd.printf("%02X ", rawRx[i] );
        }
        Serial.println();
        //        cmd.println();
      }
      if (lenRx > 2) {
        int crc1 = rawRx[lenRx - 1] << 8 | rawRx[lenRx - 2];
        int crc2 = CheckCRC(rawRx, lenRx - 2);

        if (crc1 == crc2) {
          datalen = rawRx[2];
          return datalen;
        } else {
          return -1;
        }
      } else {
        return -1;
      }
    }

    // Read Coil Register       0x01
    int ReadCoilReg(int address) {
      return ReadCoilReg(1,  address, 1);
    }

    int ReadCoilReg(int slaveId, int address) {
      return ReadCoilReg(slaveId, address, 1);
    }

    int ReadCoilReg(int slaveId, int address, int nbit) {
      if (requestFrom(slaveId, Coil_Register, address, nbit)) {
        return byteRead(0);
      } else {
        return -1;
      }
    }

    byte byteRead(int index) {
      return rawRx[index + 3];
    }

    int blockRead(int index) {
      return  ((dataRx[index * 2] << 8) | dataRx[index * 2 + 1]);
    }

    int8_t uint8(int address) {
      return rawRx[address * 2 + 3];
    }

    uint16_t uint16(int address) {
      int address_ = (address) * 2 + 3;
      return (rawRx[address_] << 8 | rawRx[address_ + 1]);
    }

    int16_t int16(int address) {
      int address_ = (address) * 2 + 3;
      return (rawRx[address_] << 8 | rawRx[address_ + 1]);
    }

    uint32_t uint32(int address, bool byteHL = true) {
      uint32_t val ;
      if (byteHL) val = uint16(address)     << 16 | uint16(address + 1);
      else        val = uint16(address + 1) << 16 | uint16(address);
      return val;
    }

    int32_t int32(int address, bool byteHL = true) {
      int32_t val ;
      if (byteHL) val = int16(address)     << 16 | int16(address + 1);
      else        val = int16(address + 1) << 16 | int16(address);
      return val;
    }
    int CheckCRC(byte *buf, int len) {
      int           nominal   = 0xA001;
      int           crc       = 0xFFFF;
      unsigned char pos;
      unsigned char i;

      for ( pos = 0; pos < len; pos++) {
        crc ^= (unsigned int)buf[pos];          // XOR byte into least sig. byte of crc

        for (i = 8; i != 0; i--) {        // Loop over each bit
          if ((crc & 0x0001) != 0) {      // If the LSB is set
            crc >>= 1;                    // Shift right and XOR 0xA001
            crc ^= nominal;
          }
          else                            // Else LSB is not set
            crc >>= 1;                    // Just shift right
        }
      }
      // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
      return crc;
    }
};
#endif
