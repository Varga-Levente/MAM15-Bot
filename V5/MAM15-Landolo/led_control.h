#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include "settings.h"

class LedControl {
private:
  unsigned long blinkStart;
  bool isBlinking;

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_LED
      Serial.println(message);
    #endif
  }

public:
  LedControl() : blinkStart(0), isBlinking(false) {}

  void init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    log("✅ LED pin inicializálva");
  }

  void startBlink() {
    isBlinking = true;
    blinkStart = millis();
    log("💡 LED villogás elindítva");
  }

  void stopBlink() {
    isBlinking = false;
    digitalWrite(LED_PIN, LOW);
    log("💡 LED villogás leállítva");
  }

  bool update() {
    if (!isBlinking) return false;
    
    unsigned long elapsed = millis() - blinkStart;
    
    if (elapsed < LED_BLINK_DURATION) {
      // Gyors villogás
      unsigned long phase = elapsed % (LED_BLINK_ON_TIME + LED_BLINK_OFF_TIME);
      digitalWrite(LED_PIN, phase < LED_BLINK_ON_TIME ? HIGH : LOW);
      return false; // Még villog
    } else {
      // Villogás vége
      stopBlink();
      
      #if DEBUG_ENABLED && DEBUG_LED
        Serial.println("\n💤 ═════════════════════════════════");
        Serial.println("💤 VILLOGÁS VÉGE - SERVÓK NYITVA MARADNAK!");
        Serial.println("💤 ═════════════════════════════════\n");
      #endif
      
      return true; // Villogás befejeződött
    }
  }

  bool getIsBlinking() const {
    return isBlinking;
  }

  void turnOff() {
    digitalWrite(LED_PIN, LOW);
  }
};

#endif