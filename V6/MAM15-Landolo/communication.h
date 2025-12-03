#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <WiFi.h>
#include <esp_now.h>
#include "settings.h"

// Callback függvény pointer típusa
typedef void (*CommandCallback)(byte);

class Communication {
private:
  CommandCallback callback;
  
  static Communication* instance;
  
  static void staticOnDataReceived(const esp_now_recv_info_t *recv_info, 
                                   const uint8_t *incomingData, int len) {
    if (instance) {
      instance->onDataReceived(recv_info, incomingData, len);
    }
  }

  void onDataReceived(const esp_now_recv_info_t *recv_info, 
                     const uint8_t *incomingData, int len) {
    if (len != 1) {
      #if DEBUG_ENABLED && DEBUG_COMM
        Serial.print("⚠️ Érvénytelen üzenet hossz: ");
        Serial.println(len);
      #endif
      return;
    }
    
    byte cmd = incomingData[0];
    
    #if DEBUG_ENABLED && DEBUG_COMM
      Serial.println("\n📡 ═════════════════════════════════");
      Serial.print("📡 PARANCS ÉRKEZETT: ");
      Serial.println(cmd);
      Serial.println("📡 ═════════════════════════════════");
    #endif
    
    if (callback) {
      callback(cmd);
    }
  }

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_COMM
      Serial.println(message);
    #endif
  }

public:
  Communication() : callback(nullptr) {
    instance = this;
  }

  bool init(CommandCallback cb) {
    callback = cb;
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    log("🌐 WiFi mód beállítva: STA");
    
    if (esp_now_init() != ESP_OK) {
      #if DEBUG_ENABLED && DEBUG_COMM
        Serial.println("❌ ESP-NOW inicializálás sikertelen!");
      #endif
      return false;
    }
    
    esp_now_register_recv_cb(staticOnDataReceived);
    
    log("✅ ESP-NOW inicializálva");
    log("✅ Callback regisztrálva");
    
    return true;
  }

  void disconnect() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    #if DEBUG_ENABLED && DEBUG_COMM
      Serial.println("🌐 WiFi kikapcsolva");
    #endif
  }
};

// Static instance pointer inicializálása
Communication* Communication::instance = nullptr;

#endif