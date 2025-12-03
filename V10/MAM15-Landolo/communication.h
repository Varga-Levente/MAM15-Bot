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
  uint8_t senderMacAddress[6];
  bool hasSenderAddress;
  
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
    
    // Küldő MAC címének mentése
    memcpy(senderMacAddress, recv_info->src_addr, 6);
    hasSenderAddress = true;
    
    #if DEBUG_ENABLED && DEBUG_COMM
      Serial.print("📡 Küldő MAC: ");
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", senderMacAddress[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
    #endif
    
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
  Communication() : callback(nullptr), hasSenderAddress(false) {
    instance = this;
    memset(senderMacAddress, 0, 6);
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

  bool sendAck(byte ackCode) {
    if (!hasSenderAddress) {
      #if DEBUG_ENABLED && DEBUG_COMM
        Serial.println("⚠️ Nincs küldő cím, nem lehet választ küldeni!");
      #endif
      return false;
    }
    
    // Peer hozzáadása
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, senderMacAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    // Ellenőrizzük, hogy már hozzá van-e adva
    if (!esp_now_is_peer_exist(senderMacAddress)) {
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        #if DEBUG_ENABLED && DEBUG_COMM
          Serial.println("❌ Peer hozzáadása sikertelen!");
        #endif
        return false;
      }
    }
    
    // ACK küldése
    esp_err_t result = esp_now_send(senderMacAddress, &ackCode, 1);
    
    #if DEBUG_ENABLED && DEBUG_COMM
      if (result == ESP_OK) {
        Serial.println("\n📤 ═════════════════════════════════");
        Serial.print("📤 ACK ELKÜLDVE: ");
        Serial.println(ackCode);
        Serial.println("📤 ═════════════════════════════════");
      } else {
        Serial.print("❌ ACK küldés sikertelen! Hiba: ");
        Serial.println(result);
      }
    #endif
    
    return (result == ESP_OK);
  }

  void disconnect() {
    // ESP-NOW deinicializálás
    esp_now_deinit();
    
    #if DEBUG_ENABLED && DEBUG_COMM
      Serial.println("📡 ESP-NOW deinicializálva");
    #endif
    
    // WiFi kikapcsolás
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