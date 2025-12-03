#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include <Arduino.h>
#include "settings.h"

Preferences preferences;
String codes[MAX_CODES];
int activeCode = -1;
bool shouldBlink = false;
bool externalTriggerEnabled = false;
int BLINK_BAUD = DEFAULT_BLINK_BAUD;
int PAUSE_BETWEEN_CODES = DEFAULT_PAUSE_MS;
int cameraQuality = DEFAULT_CAMERA_QUALITY;
unsigned long bitDelay = 1000000UL / DEFAULT_BLINK_BAUD;

// Serial Log Buffer
#define LOG_BUFFER_SIZE 100
String logBuffer[LOG_BUFFER_SIZE];
int logWriteIndex = 0;
int logReadIndex = 0;
int logCount = 0;

void addLog(String message) {
  // Add to circular buffer
  logBuffer[logWriteIndex] = message;
  logWriteIndex = (logWriteIndex + 1) % LOG_BUFFER_SIZE;
  
  if(logCount < LOG_BUFFER_SIZE) {
    logCount++;
  } else {
    logReadIndex = (logReadIndex + 1) % LOG_BUFFER_SIZE;
  }
  
  // Also print to Serial
  Serial.println(message);
}

String getLogHTML() {
  String html = "";
  int count = logCount;
  int idx = logReadIndex;
  
  for(int i = 0; i < count; i++) {
    String line = logBuffer[idx];
    // Escape HTML characters
    line.replace("&", "&amp;");
    line.replace("<", "&lt;");
    line.replace(">", "&gt;");
    line.replace("\"", "&quot;");
    
    html += "<div class='log-line'>" + line + "</div>";
    idx = (idx + 1) % LOG_BUFFER_SIZE;
  }
  
  return html;
}

void loadCodes() {
  preferences.begin(CODES_NAMESPACE, false);
  
  for (int i = 0; i < MAX_CODES; i++) {
    String key = "code" + String(i);
    codes[i] = preferences.getString(key.c_str(), "");
  }
  
  activeCode = preferences.getInt("active", -1);
  shouldBlink = preferences.getBool("blinking", false);
  externalTriggerEnabled = preferences.getBool("extTrigger", false);
  BLINK_BAUD = preferences.getInt("baud", DEFAULT_BLINK_BAUD);
  PAUSE_BETWEEN_CODES = preferences.getInt("pause", DEFAULT_PAUSE_MS);
  cameraQuality = preferences.getInt("camquality", DEFAULT_CAMERA_QUALITY);
  
  preferences.end();
  
  bitDelay = 1000000UL / BLINK_BAUD;
  
  addLog("✅ Kódok betöltve:");
  addLog("   Aktív kód: " + String(activeCode));
  addLog("   Villogás: " + String(shouldBlink ? "BE" : "KI"));
  addLog("   Külső trigger: " + String(externalTriggerEnabled ? "BE" : "KI"));
  addLog("   Baud: " + String(BLINK_BAUD));
}

void saveCodes() {
  preferences.begin(CODES_NAMESPACE, false);
  
  for (int i = 0; i < MAX_CODES; i++) {
    String key = "code" + String(i);
    preferences.putString(key.c_str(), codes[i]);
  }
  
  preferences.putInt("active", activeCode);
  preferences.putBool("blinking", shouldBlink);
  preferences.putBool("extTrigger", externalTriggerEnabled);
  preferences.putInt("baud", BLINK_BAUD);
  preferences.putInt("pause", PAUSE_BETWEEN_CODES);
  preferences.putInt("camquality", cameraQuality);
  
  preferences.end();
  
  addLog("💾 Kódok mentve");
}

#endif