#ifndef ESPNOW_COMMUNICATION_H
#define ESPNOW_COMMUNICATION_H

#include <WiFi.h>
#include <esp_now.h>
#include "settings.h"

class ESPNowCommunication {
private:
  uint8_t landoloMAC[6];
  bool previousLandingState;
  bool espnowActive;
  bool previousButtonState;
  bool pin22State;
  
  static ESPNowCommunication* instance;

  static void staticOnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.print("📤 ESP-NOW küldés státusza: ");
      Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ Sikeres" : "❌ Sikertelen");
    #endif
  }

  static void staticOnDataReceived(const esp_now_recv_info_t *recv_info, 
                                   const uint8_t *incomingData, int len) {
    if (instance) {
      instance->onDataReceived(recv_info, incomingData, len);
    }
  }

  void onDataReceived(const esp_now_recv_info_t *recv_info, 
                     const uint8_t *incomingData, int len) {
    if (len != 1) {
      #if DEBUG_ENABLED && DEBUG_ESPNOW
        Serial.print("⚠️ Érvénytelen ACK hossz: ");
        Serial.println(len);
      #endif
      return;
    }
    
    byte ackCode = incomingData[0];
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.println("\n📥 ╔═══════════════════════════════╗");
      Serial.print("📥 LANDOLÓ ACK ÉRKEZETT: ");
      Serial.println(ackCode);
      Serial.println("📥 ╚═══════════════════════════════╝");
    #endif
    
    // ACK_SERVO_OPENED = 100 - Csak logolás, már nem állítjuk le az ESP-NOW-t
    if (ackCode == 100) {
      #if DEBUG_ENABLED && DEBUG_LANDING
        Serial.println("\n✅ ╔═══════════════════════════════╗");
        Serial.println("✅ LANDOLÓ VISSZAIGAZOLÁS:");
        Serial.println("✅ Servo sikeresen kinyílt!");
        Serial.println("✅ ╚═══════════════════════════════╝\n");
      #endif
    }
  }

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.println(message);
    #endif
  }

  void logLedFlash(const char* message) {
    #if DEBUG_ENABLED && DEBUG_LED_FLASH
      Serial.println(message);
    #endif
  }

public:
  ESPNowCommunication() 
    : previousLandingState(false)
    , espnowActive(false)
    , previousButtonState(false)
    , pin22State(false) {
    instance = this;
    landoloMAC[0] = LANDOLO_MAC_0;
    landoloMAC[1] = LANDOLO_MAC_1;
    landoloMAC[2] = LANDOLO_MAC_2;
    landoloMAC[3] = LANDOLO_MAC_3;
    landoloMAC[4] = LANDOLO_MAC_4;
    landoloMAC[5] = LANDOLO_MAC_5;
  }

  bool init() {
    // LED_FLASH_PIN inicializálása kimenetként
    pinMode(LED_FLASH_PIN, OUTPUT);
    digitalWrite(LED_FLASH_PIN, LOW);
    pin22State = false;
    
    #if DEBUG_ENABLED && DEBUG_LED_FLASH
      Serial.println("📍 LED_FLASH_PIN inicializálva (LOW)");
    #endif
    
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
    
    // Callback regisztrálása
    esp_now_register_send_cb(staticOnDataSent);
    esp_now_register_recv_cb(staticOnDataReceived);
    
    // Landoló peer hozzáadása
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
    
    espnowActive = true;
    return true;
  }

  void sendLandingCommand(bool landingState) {
    if (!espnowActive) {
      #if DEBUG_ENABLED && DEBUG_ESPNOW
        Serial.println("⚠️ ESP-NOW nem aktív, parancs nem küldhető!");
      #endif
      return;
    }
    
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

  void handleLandingButton(bool currentButtonState) {
    // Gombnyomás detektálása (rising edge)
    bool buttonPressed = currentButtonState && !previousButtonState;
    previousButtonState = currentButtonState;
    
    if (!buttonPressed) {
      return; // Csak gombnyomásra reagálunk
    }
    
    // ═══════════════════════════════════════════════════════
    // PÁRHUZAMOS MŰKÖDÉS: ESP-NOW + PIN22 TOGGLE
    // ═══════════════════════════════════════════════════════
    
    // 1️⃣ ESP-NOW parancs küldése (ha aktív)
    if (espnowActive) {
      bool newLandingState = !previousLandingState;
      sendLandingCommand(newLandingState);
      previousLandingState = newLandingState;
      
      #if DEBUG_ENABLED && DEBUG_LANDING
        Serial.print("🔄 Landoló állapot váltás: ");
        Serial.println(newLandingState ? "AKTÍV" : "INAKTÍV");
      #endif
    }
    
    // 2️⃣ LED_FLASH_PIN Toggle (MINDIG, ESP-NOW állapottól függetlenül)
    pin22State = !pin22State;
    digitalWrite(LED_FLASH_PIN, pin22State ? HIGH : LOW);
    
    #if DEBUG_ENABLED && DEBUG_LED_FLASH
      Serial.println("\n🔀 ╔═══════════════════════════════╗");
      Serial.print("🔀 LED_FLASH_PIN TOGGLE: ");
      Serial.println(pin22State ? "HIGH" : "LOW");
      Serial.println("🔀 ╚═══════════════════════════════╝\n");
    #endif
  }

  void shutdown() {
    if (!espnowActive) {
      return;
    }
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.println("\n🔌 ╔═══════════════════════════════╗");
      Serial.println("🔌 ESP-NOW LEÁLLÍTÁS");
      Serial.println("🔌 ╚═══════════════════════════════╝");
    #endif
    
    esp_now_deinit();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    espnowActive = false;
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.println("✅ ESP-NOW leállítva");
      Serial.println("✅ WiFi kikapcsolva");
      Serial.println("✅ Motorvezérlő tisztán LoRa módban");
      Serial.println("✅ LED_FLASH_PIN toggle továbbra is működik\n");
    #endif
  }

  bool isActive() const {
    return espnowActive;
  }
  
  bool getPin22State() const {
    return pin22State;
  }
};

// Static instance pointer inicializálása
ESPNowCommunication* ESPNowCommunication::instance = nullptr;

#endif