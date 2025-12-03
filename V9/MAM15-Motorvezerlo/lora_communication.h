#ifndef LORA_COMMUNICATION_H
#define LORA_COMMUNICATION_H

#include <LoRa.h>
#include <SPI.h>
#include "settings.h"

enum LoRaState {
  LORA_OK,
  LORA_DISCONNECTED,
  LORA_RECONNECTING
};

class LoRaCommunication {
private:
  LoRaState currentState;
  unsigned long lastHealthCheck;
  unsigned long lastReceivedPacket;
  unsigned long stateChangeTime;
  int restartCount;
  bool moduleHealthy;

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_LORA
      Serial.println(message);
    #endif
  }

  void logHealth(const char* message) {
    #if DEBUG_ENABLED && DEBUG_HEALTH
      Serial.println(message);
    #endif
  }

public:
  LoRaCommunication() 
    : currentState(LORA_OK)
    , lastHealthCheck(0)
    , lastReceivedPacket(0)
    , stateChangeTime(0)
    , restartCount(0)
    , moduleHealthy(true) {}

  bool init() {
    LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
    
    if (!LoRa.begin(LORA_FREQUENCY)) {
      #if DEBUG_ENABLED && DEBUG_LORA
        Serial.println("❌ LoRa inicializálás sikertelen!");
      #endif
      return false;
    }
    
    pinMode(LORA_RESET_PIN, OUTPUT);
    digitalWrite(LORA_RESET_PIN, HIGH);
    
    log("✅ LoRa inicializálás sikeres");
    
    lastHealthCheck = millis();
    lastReceivedPacket = millis();
    
    return true;
  }

  bool restart() {
    #if DEBUG_ENABLED && DEBUG_LORA
      Serial.println("🔄 LoRa modul újraindítása...");
    #endif
    
    LoRa.end();
    delay(100);
    digitalWrite(LORA_RESET_PIN, LOW);
    delay(10);
    digitalWrite(LORA_RESET_PIN, HIGH);
    delay(50);
    
    bool success = LoRa.begin(LORA_FREQUENCY);
    if (success) {
      restartCount++;
      #if DEBUG_ENABLED && DEBUG_LORA
        Serial.print("✅ LoRa modul újraindítva (");
        Serial.print(restartCount);
        Serial.println(". alkalommal)");
      #endif
    } else {
      log("❌ LoRa modul újraindítása sikertelen!");
    }
    return success;
  }

  void checkHealth() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastHealthCheck < LORA_HEALTH_CHECK_INTERVAL) {
      return;
    }
    
    lastHealthCheck = currentTime;
    
    bool loraWorking = (currentTime - lastReceivedPacket) < LORA_HEALTH_CHECK_INTERVAL;
    
    if (!loraWorking && moduleHealthy) {
      logHealth("⚠️ LoRa: Nincs csomag 5 másodperc alatt - újracsatlakozás...");
      moduleHealthy = false;
      currentState = LORA_RECONNECTING;
      stateChangeTime = currentTime;
    }
    
    if (currentState == LORA_RECONNECTING) {
      if (currentTime - stateChangeTime > LORA_RECONNECT_TIMEOUT) {
        logHealth("❌ LoRa újracsatlakozási időtúllépés!");
        currentState = LORA_DISCONNECTED;
      } else {
        if (restart()) {
          logHealth("✅ LoRa modul sikeresen újracsatlakozott!");
          moduleHealthy = true;
          currentState = LORA_OK;
          lastReceivedPacket = currentTime;
        } else {
          delay(1000);
        }
      }
    }
  }

  int parsePacket() {
    return LoRa.parsePacket();
  }

  byte read() {
    return LoRa.read();
  }

  void updateReceivedTime() {
    lastReceivedPacket = millis();
  }

  unsigned long getLastReceivedTime() const {
    return lastReceivedPacket;
  }

  LoRaState getState() const {
    return currentState;
  }

  bool isHealthy() const {
    return moduleHealthy;
  }
};

#endif