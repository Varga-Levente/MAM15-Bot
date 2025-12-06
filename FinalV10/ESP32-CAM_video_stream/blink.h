#ifndef BLINK_H
#define BLINK_H

#include <Arduino.h>
#include "settings.h"
#include "storage.h"

TaskHandle_t blinkTaskHandle = NULL;

void blinkCode(String code) {
  if (code.length() != 3) return;

  unsigned long bitDuration = 1000000UL / BLINK_BAUD;

  for (int i = 0; i < code.length(); i++) {
    char c = code[i];
    
    // START BIT (HIGH) - Inverted UART
    unsigned long startTime = micros();
    digitalWrite(LED_PIN, HIGH);
    while(micros() - startTime < bitDuration);
    
    // 8 DATA BITS (LSB FIRST, INVERTED)
    for (int b = 0; b < 8; b++) {
      startTime = micros();
      bool bit = (c >> b) & 1;
      digitalWrite(LED_PIN, bit ? LOW : HIGH);  // INVERTED!
      while(micros() - startTime < bitDuration);
    }
    
    // STOP BIT (LOW)
    startTime = micros();
    digitalWrite(LED_PIN, LOW);
    while(micros() - startTime < bitDuration);
  }
  
  digitalWrite(LED_PIN, LOW);
}

void blinkTask(void* parameter) {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  pinMode(EXTERNAL_TRIGGER_PIN, INPUT);
  
  Serial.println("⚡ Villogó task elindult");
  Serial.printf("📡 Protocol: Inverted UART 8N1 @ %d baud\n", BLINK_BAUD);
  
  while(true) {
    bool shouldBlinkNow = false;
    
    if (externalTriggerEnabled) {
      int triggerState = digitalRead(EXTERNAL_TRIGGER_PIN);
      shouldBlinkNow = (triggerState == HIGH);
    } else {
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
    4096,  // Normal stack size
    NULL,
    1,
    &blinkTaskHandle,
    0
  );
}

#endif