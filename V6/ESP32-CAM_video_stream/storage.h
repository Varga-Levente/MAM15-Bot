#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include <Arduino.h>
#include "settings.h"

Preferences preferences;
String codes[MAX_CODES];
int activeCode = -1;
bool shouldBlink = false;
int BLINK_BAUD = DEFAULT_BLINK_BAUD;
int PAUSE_BETWEEN_CODES = DEFAULT_PAUSE_MS;
int cameraQuality = DEFAULT_CAMERA_QUALITY;
unsigned long bitDelay = 1000000UL / DEFAULT_BLINK_BAUD;

void loadCodes() {
  preferences.begin(CODES_NAMESPACE, false);
  
  for (int i = 0; i < MAX_CODES; i++) {
    String key = "code" + String(i);
    codes[i] = preferences.getString(key.c_str(), "");
  }
  
  activeCode = preferences.getInt("active", -1);
  shouldBlink = preferences.getBool("blinking", false);
  BLINK_BAUD = preferences.getInt("baud", DEFAULT_BLINK_BAUD);
  PAUSE_BETWEEN_CODES = preferences.getInt("pause", DEFAULT_PAUSE_MS);
  cameraQuality = preferences.getInt("camquality", DEFAULT_CAMERA_QUALITY);
  
  preferences.end();
  
  bitDelay = 1000000UL / BLINK_BAUD;
  
  Serial.println("✅ Kódok betöltve:");
  Serial.printf("   Aktív kód: %d\n", activeCode);
  Serial.printf("   Villogás: %s\n", shouldBlink ? "BE" : "KI");
  Serial.printf("   Baud: %d\n", BLINK_BAUD);
}

void saveCodes() {
  preferences.begin(CODES_NAMESPACE, false);
  
  for (int i = 0; i < MAX_CODES; i++) {
    String key = "code" + String(i);
    preferences.putString(key.c_str(), codes[i]);
  }
  
  preferences.putInt("active", activeCode);
  preferences.putBool("blinking", shouldBlink);
  preferences.putInt("baud", BLINK_BAUD);
  preferences.putInt("pause", PAUSE_BETWEEN_CODES);
  preferences.putInt("camquality", cameraQuality);
  
  preferences.end();
  
  Serial.println("💾 Kódok mentve");
}

#endif