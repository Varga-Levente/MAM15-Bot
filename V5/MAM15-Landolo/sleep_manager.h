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
    #if DEBUG_ENABLED && DEBUG_SLEEP
      Serial.println("\n💤 ═════════════════════════════════");
      Serial.println("💤 DEEP SLEEP MÓDBA LÉPÉS...");
      Serial.println("💤 Servók NYITVA maradnak (LANDOLO AKTÍV)");
      Serial.println("💤 Reset gombbal való felébresztésre várva");
      Serial.println("💤 ═════════════════════════════════\n");
      Serial.flush();
    #endif
    
    delay(SLEEP_ENTER_DELAY);
    
    // GPIO wakeup konfiguráció
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable((gpio_num_t)RESET_BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    
    log("😴 Deep sleep indítása...");
    
    // Deep sleep start
    esp_deep_sleep_start();
  }

  int getBootCount() {
    static RTC_DATA_ATTR int bootCount = 0;
    return bootCount;
  }

  void incrementBootCount() {
    static RTC_DATA_ATTR int bootCount = 0;
    bootCount++;
  }

  void printBootInfo(int bootCount) {
    #if DEBUG_ENABLED && DEBUG_BOOT
      Serial.println("\n════════════════════════════════════");
      Serial.println("🛬 LANDOLÓ - DEEP SLEEP VERZIÓ");
      Serial.println("════════════════════════════════════");
      Serial.printf("🔄 Boot count: %d\n", bootCount);
      
      if (bootCount > 1) {
        Serial.println("\n🔒 RESET GOMB FELÉBRESZTÉSE");
        Serial.println("🔒 Servók LEZÁRVA (0°)");
      }
      
      Serial.println("════════════════════════════════════\n");
    #endif
  }
};

#endif