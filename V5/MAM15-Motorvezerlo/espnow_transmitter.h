#ifndef ESPNOW_TRANSMITTER_H
#define ESPNOW_TRANSMITTER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

class ESPNowTransmitter {
private:
  bool previousLandingState;
  uint8_t targetMAC[6];

public:
  ESPNowTransmitter();
  
  bool init();
  void sendLandingCommand(bool landingState);
  
private:
  static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
};

#endif