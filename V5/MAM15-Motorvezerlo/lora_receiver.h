#ifndef LORA_RECEIVER_H
#define LORA_RECEIVER_H

#include <Arduino.h>
#include <LoRa.h>
#include <CRC16.h>

enum LoRaState {
  LORA_OK,
  LORA_DISCONNECTED,
  LORA_RECONNECTING
};

class LoRaReceiver {
private:
  CRC16* crcCalculator;
  unsigned long lastReceivedPacketTime;
  unsigned long lastHealthCheck;
  unsigned long stateChangeTime;
  
  LoRaState currentState;
  bool moduleHealthy;
  int restartCount;

public:
  LoRaReceiver();
  ~LoRaReceiver();
  
  bool init();
  void checkHealth();
  bool receivePacket(byte* packet, int expectedSize);
  bool isHealthy();
  unsigned long getLastPacketTime();
  
private:
  bool restartModule();
  bool validateCRC(byte* packet, int dataSize);
};

#endif