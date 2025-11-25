#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include <LoRa.h>
#include <CRC.h>

class Communication {
private:
  CRC16* crcCalculator;

public:
  Communication();
  ~Communication();
  
  bool init();
  void sendPacket(uint8_t robotId, byte motorCommand, bool speedFlag, bool landingFlag);
  
private:
  uint16_t calculateCRC(uint8_t* data, size_t length);
};

#endif