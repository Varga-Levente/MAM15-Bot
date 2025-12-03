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
  bool espnowPermanentlyDisabled;
  
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
    
    // ACK_SERVO_OPENED = 100
    if (ackCode == 200) {
      #if DEBUG_ENABLED && DEBUG_LANDING
        Serial.println("\n✅ ╔═══════════════════════════════╗");
        Serial.println("✅ LANDOLÓ VISSZAIGAZOLÁS:");
        Serial.println("✅ Servo sikeresen kinyílt!");
        Serial.println("✅ ESP-NOW VÉGLEGESEN leállítása...");
        Serial.println("✅ PIN22 LED vezérlés továbbra is aktív");
        Serial.println("✅ ╚═══════════════════════════════╝\n");
      #endif
      
      // ESP-NOW VÉGLEGESEN leállítása
      shutdownPermanently();
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
    , espnowPermanentlyDisabled(false) {
    instance = this;
    landoloMAC[0] = LANDOLO_MAC_0;
    landoloMAC[1] = LANDOLO_MAC_1;
    landoloMAC[2] = LANDOLO_MAC_2;
    landoloMAC[3] = LANDOLO_MAC_3;
    landoloMAC[4] = LANDOLO_MAC_4;
    landoloMAC[5] = LANDOLO_MAC_5;
  }

  bool init() {
    // LED_FLASH_PIN inicializálása kimenetként (LOW = alapértelmezett)
    pinMode(LED_FLASH_PIN, OUTPUT);
    digitalWrite(LED_FLASH_PIN, LOW);
    
    #if DEBUG_ENABLED && DEBUG_LED_FLASH
      Serial.println("🔦 LED_FLASH_PIN (22) inicializálva: LOW");
    #endif
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.print("🔐 Motorvezérlő ESP32 saját MAC: ");
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
      Serial.print("🔐 Landoló cél MAC: ");
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", landoloMAC[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
    #endif
    
    espnowActive = true;
    espnowPermanentlyDisabled = false;
    return true;
  }

  void sendLandingCommand(bool landingState) {
    if (!espnowActive || espnowPermanentlyDisabled) {
      #if DEBUG_ENABLED && DEBUG_ESPNOW
        if (espnowPermanentlyDisabled) {
          Serial.println("⚠️ ESP-NOW véglegesen letiltva (ACK után)");
        } else {
          Serial.println("⚠️ ESP-NOW nem aktív, parancs nem küldhető!");
        }
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

  void handleLandingState(bool currentLandingState) {
    // Csak akkor reagálunk, ha az állapot megváltozott
    if (currentLandingState == previousLandingState) {
      return;
    }
    
    // ══════════════════════════════════════════════════════
    // KÉT FÜGGETLEN FUNKCIÓ:
    // 1. ESP-NOW: Csak első alkalommal küld, ACK után letiltva
    // 2. PIN22 LED: Mindig követi a landingState-et
    // ══════════════════════════════════════════════════════
    
    // 1️⃣ ESP-NOW parancs küldése (csak ha még aktív és nem letiltva)
    if (espnowActive && !espnowPermanentlyDisabled) {
      sendLandingCommand(currentLandingState);
      
      #if DEBUG_ENABLED && DEBUG_LANDING
        Serial.print("🔄 Landoló állapot változás: ");
        Serial.println(currentLandingState ? "AKTÍV" : "INAKTÍV");
      #endif
    }
    
    // 2️⃣ PIN22 LED vezérlése (MINDIG, ESP-NOW állapottól függetlenül)
    digitalWrite(LED_FLASH_PIN, currentLandingState ? HIGH : LOW);
    
    #if DEBUG_ENABLED && DEBUG_LED_FLASH
      Serial.println("\n🔦 ╔═══════════════════════════════╗");
      Serial.print("🔦 PIN22 LED: ");
      Serial.println(currentLandingState ? "HIGH (ON)" : "LOW (OFF)");
      Serial.println("🔦 ╚═══════════════════════════════╝\n");
    #endif
    
    // Állapot mentése
    previousLandingState = currentLandingState;
  }

  void shutdownPermanently() {
    if (!espnowActive) {
      return;
    }
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.println("\n🔌 ╔═══════════════════════════════╗");
      Serial.println("🔌 ESP-NOW VÉGLEGES LEÁLLÍTÁS");
      Serial.println("🔌 Újraindításig nem aktiválható!");
      Serial.println("🔌 ╚═══════════════════════════════╝");
    #endif
    
    esp_now_deinit();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    espnowActive = false;
    espnowPermanentlyDisabled = true;
    
    #if DEBUG_ENABLED && DEBUG_ESPNOW
      Serial.println("✅ ESP-NOW leállítva");
      Serial.println("✅ WiFi kikapcsolva");
      Serial.println("✅ Motorvezérlő tisztán LoRa módban");
      Serial.println("✅ PIN22 LED vezérlés AKTÍV marad\n");
    #endif
  }

  bool isActive() const {
    return espnowActive;
  }
  
  bool isPermanentlyDisabled() const {
    return espnowPermanentlyDisabled;
  }
};

// Static instance pointer inicializálása
ESPNowCommunication* ESPNowCommunication::instance = nullptr;

#endif