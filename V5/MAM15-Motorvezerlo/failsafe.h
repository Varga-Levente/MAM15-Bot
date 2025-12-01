#ifndef FAILSAFE_H
#define FAILSAFE_H

#include <Arduino.h>
#include "settings.h"

class Failsafe {
private:
  unsigned long lastSafeTime;
  bool failsafeActive;

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_FAILSAFE
      Serial.println(message);
    #endif
  }

public:
  Failsafe() : lastSafeTime(0), failsafeActive(false) {}

  void reset() {
    lastSafeTime = millis();
    if (failsafeActive) {
      failsafeActive = false;
      log("✅ Failsafe deaktiválva - normál működés");
    }
  }

  bool check() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastSafeTime > FAILSAFE_TIMEOUT_MS) {
      if (!failsafeActive) {
        log("⚠️ FAILSAFE AKTIVÁLVA - Nincs kommunikáció!");
        failsafeActive = true;
      }
      
      // Időzítő reset (de failsafe aktív marad)
      lastSafeTime = currentTime - FAILSAFE_TIMEOUT_MS + 1000;
      return true;
    }
    
    return false;
  }

  bool isActive() const {
    return failsafeActive;
  }

  void init() {
    lastSafeTime = millis();
    failsafeActive = false;
  }
};

#endif