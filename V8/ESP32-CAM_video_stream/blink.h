#ifndef BLINK_H
#define BLINK_H

#include <Arduino.h>
#include "settings.h"
#include "storage.h"

TaskHandle_t blinkTaskHandle = NULL;

void blinkCode(String code) {
  if (code.length() != 3) return;

  for (int i = 0; i < code.length(); i++) {
    char c = code[i];

    // START BIT (LOW)
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(bitDelay);

    // 8 DATA BIT LSB FIRST
    for (int b = 0; b < 8; b++) {
      bool bit = (c >> b) & 1;
      digitalWrite(LED_PIN, bit ? HIGH : LOW);
      delayMicroseconds(bitDelay);
    }

    // STOP BIT (HIGH)
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(bitDelay);
  }

  // LED OFF
  digitalWrite(LED_PIN, LOW);
}

void blinkTask(void* parameter) {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize external trigger pin
  pinMode(EXTERNAL_TRIGGER_PIN, INPUT);
  
  Serial.println("⚡ Villogó task elindult");
  
  while(true) {
    // Determine if we should blink based on web control and/or external trigger
    bool shouldBlinkNow = false;
    
    if (externalTriggerEnabled) {
      // If external trigger is enabled, check pin 12
      int triggerState = digitalRead(EXTERNAL_TRIGGER_PIN);
      shouldBlinkNow = (triggerState == HIGH);
    } else {
      // If external trigger is disabled, use web control
      shouldBlinkNow = shouldBlink;
    }
    
    if (shouldBlinkNow && activeCode >= 0 && activeCode < MAX_CODES) {
      String code = codes[activeCode];
      if (code.length() == 3) {
        blinkCode(code);
        delay(PAUSE_BETWEEN_CODES);
      } else {
        delay(100);
      }
    } else {
      delay(100);
    }
  }
}

void startBlinkTask() {
  xTaskCreatePinnedToCore(
    blinkTask,
    "BlinkTask",
    4096,
    NULL,
    1,
    &blinkTaskHandle,
    0
  );
}

#endif