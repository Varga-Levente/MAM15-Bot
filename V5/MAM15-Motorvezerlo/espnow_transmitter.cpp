#include "espnow_transmitter.h"
#include "settings.h"

ESPNowTransmitter::ESPNowTransmitter() 
  : previousLandingState(false) {
  memcpy(targetMAC, ESPNowSettings::LANDOLO_MAC, 6);
}

void ESPNowTransmitter::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_ESPNOW) {
    Serial.print("📤 ESP-NOW küldés státusza: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ Sikeres" : "❌ Sikertelen");
  }
}

bool ESPNowTransmitter::init() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_ESPNOW) {
    Serial.print("📍 Motorvezérlő ESP32 MAC: ");
    Serial.println(WiFi.macAddress());
  }
  
  if (esp_now_init() != ESP_OK) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_ESPNOW) {
      Serial.println("❌ ESP-NOW inicializálás sikertelen!");
    }
    return false;
  }
  
  esp_now_register_send_cb(onDataSent);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, targetMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_ESPNOW) {
      Serial.println("❌ ESP-NOW peer hozzáadás sikertelen!");
    }
    return false;
  }
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_ESPNOW) {
    Serial.println("✅ ESP-NOW inicializálva");
    Serial.print("📍 Landoló cél MAC: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", targetMAC[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }
  
  return true;
}

void ESPNowTransmitter::sendLandingCommand(bool landingState) {
  if (landingState == previousLandingState) {
    return;
  }
  
  byte command = landingState ? 1 : 0;
  esp_err_t result = esp_now_send(targetMAC, &command, 1);
  
  previousLandingState = landingState;
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_LANDING) {
    Serial.print("🛬 Landoló parancs: ");
    Serial.print(landingState ? "AKTIVÁLÁS (1)" : "DEAKTIVÁLÁS (0)");
    Serial.print(" - Status: ");
    Serial.println(result == ESP_OK ? "✅ OK" : "❌ Hiba");
  }
}