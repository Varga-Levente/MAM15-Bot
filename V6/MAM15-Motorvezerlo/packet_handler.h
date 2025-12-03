#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <CRC16.h>
#include "settings.h"

struct PacketData {
  byte robotId;
  byte motorCommand;
  bool speedButtonPressed;
  bool landingState;
  uint16_t crc;
  bool valid;
};

class PacketHandler {
private:
  CRC16 crcCalculator;

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_CRC
      Serial.println(message);
    #endif
  }

public:
  PacketHandler() 
    : crcCalculator(CRC_POLYNOMIAL, CRC_INITIAL_VALUE, CRC_FINAL_XOR_VALUE, true, true) {}

  bool validatePacketSize(int packetSize) {
    if (packetSize != PACKET_SIZE) {
      #if DEBUG_ENABLED && DEBUG_LORA
        Serial.print("⚠️ Hibás csomag méret! Várt: ");
        Serial.print(PACKET_SIZE);
        Serial.print(", Kapott: ");
        Serial.println(packetSize);
      #endif
      return false;
    }
    return true;
  }

  PacketData parsePacket(byte* receivedPacket) {
    PacketData data;
    data.valid = false;
    
    // CRC ellenőrzés
    uint16_t receivedCRC = (receivedPacket[4] << 8) | receivedPacket[5];
    crcCalculator.restart();
    crcCalculator.add(receivedPacket, 4);
    uint16_t calculatedCRC = crcCalculator.getCRC();
    
    if (receivedCRC != calculatedCRC) {
      log("❌ Hibás CRC - csomag elvetve!");
      return data;
    }
    
    // Robot ID ellenőrzés
    if (receivedPacket[0] != ROBOT_ID) {
      #if DEBUG_ENABLED && DEBUG_LORA
        Serial.print("⚠️ Csomag másik robotnak: ");
        Serial.println(receivedPacket[0]);
      #endif
      return data;
    }
    
    // Adatok kinyerése
    data.robotId = receivedPacket[0];
    data.motorCommand = receivedPacket[1];
    data.speedButtonPressed = receivedPacket[2];
    data.landingState = receivedPacket[3];
    data.crc = receivedCRC;
    data.valid = true;
    
    return data;
  }
};

#endif