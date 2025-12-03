#ifndef ESPNOW_COMMUNICATION_H
#define ESPNOW_COMMUNICATION_H

#include <WiFi.h>
#include <esp_now.h>
#include "settings.h"

class ESPNowCommunication {
private:
  uint8_t landoloMAC[6];
  bool previousLandingState;

  static void staticOnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.print("📤 ESP-NOW küldés státusza: ");
      Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ Sikeres" : "❌ Sikertelen");
    #endif
  }

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.println(message);
    #endif
  }

public:
  ESPNowCommunication() : previousLandingState(false) {
    landoloMAC[0] = LANDOLO_MAC_0;
    landoloMAC[1] = LANDOLO_MAC_1;
    landoloMAC[2] = LANDOLO_MAC_2;
    landoloMAC[3] = LANDOLO_MAC_3;
    landoloMAC[4] = LANDOLO_MAC_4;
    landoloMAC[5] = LANDOLO_MAC_5;
  }

  bool init() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.print("📍 Motorvezérlő ESP32 saját MAC: ");
      Serial.println(WiFi.macAddress());
    #endif
    
    if (esp_now_init() != ESP_OK) {
      log("❌ ESP-NOW inicializálás sikertelen!");
      return false;
    }
    
    log("✅ ESP-NOW inicializálva");
    
    esp_now_register_send_cb(staticOnDataSent);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, landoloMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      log("❌ ESP-NOW peer hozzáadás sikertelen!");
      return false;
    }
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.print("📍 Landoló cél MAC: ");
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", landoloMAC[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
    #endif
    
    return true;
  }

  void sendLandingCommand(bool landingState) {
    byte command = landingState ? 1 : 0;
    esp_err_t result = esp_now_send(landoloMAC, &command, 1);
    
    #if DEBUG_ENABLED && DEBUG_LANDING
      Serial.print("🛬 Landoló parancs: ");
      Serial.print(landingState ? "AKTIVÁLÁS (1)" : "DEAKTIVÁLÁS (0)");
      Serial.print(" - Status: ");
      if (result == ESP_OK) {
        Serial.println("✅ OK");
      } else {
        Serial.println("❌ Hiba!");
      }
    #endif
  }

  void handleLandingState(bool currentLandingState) {
    if (currentLandingState != previousLandingState) {
      sendLandingCommand(currentLandingState);
      previousLandingState = currentLandingState;
      
      #if DEBUG_ENABLED && DEBUG_LANDING
        Serial.print("🔄 Landoló állapot változás: ");
        Serial.println(currentLandingState ? "AKTÍV" : "INAKTÍV");
      #endif
    }
  }
};

#endif