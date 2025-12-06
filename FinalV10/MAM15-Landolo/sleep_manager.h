#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include "settings.h"

class SleepManager {
private:
  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_SLEEP
      Serial.println(message);
    #endif
  }

public:
  void initWakeupButton() {
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    log("✅ Wakeup gomb inicializálva");
  }

  void enterDeepSleep() {
    log("🛬 Servók NYITVA maradnak (90°)");
    
    delay(SLEEP_ENTER_DELAY);
    
    // GPIO wakeup konfiguráció
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable((gpio_num_t)RESET_BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    
    log("😴 Deep sleep indítása...");
    
    // Deep sleep start
    esp_deep_sleep_start();
  }

  void printBootInfo(int bootCount) {
    #if DEBUG_ENABLED && DEBUG_BOOT
      Serial.println("\n\n════════════════════════════════════");
      Serial.println("🛬 LANDOLÓ - DEEP SLEEP VERZIÓ");
      Serial.println("════════════════════════════════════");
      Serial.print("🔄 Boot count: ");
      Serial.println(bootCount);
      
      if (bootCount > 1) {
        Serial.println("\n🔒 RESET GOMB FELÉBRESZTÉSE");
        Serial.println("🔒 Servók LEZÁRVA (0°)");
      }
      
      Serial.println("════════════════════════════════════\n");
    #endif
  }
};

#endif