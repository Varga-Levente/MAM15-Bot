#include "settings.h"
#include "servo_control.h"
#include "led_control.h"
#include "communication.h"
#include "sleep_manager.h"

// ═════════════════════════════════════════════════════════
// GLOBÁLIS OBJEKTUMOK
// ═════════════════════════════════════════════════════════
ServoControl servos;
LedControl led;
Communication comm;
SleepManager sleepMgr;

// ═════════════════════════════════════════════════════════
// ÁLLAPOT VÁLTOZÓK
// ═════════════════════════════════════════════════════════
bool landingActive = false;
RTC_DATA_ATTR int bootCount = 0;

// ═════════════════════════════════════════════════════════
// LANDING AKTIVÁLÁS
// ═════════════════════════════════════════════════════════
void activateLanding() {
  landingActive = true;
  
  // Servók nyitása
  servos.open();
  
  // ACK küldése, hogy a servo kinyílt
  comm.sendAck(ACK_SERVO_OPENED);
  
  // LED villogás indítása
  led.startBlink();
  
  #if DEBUG_ENABLED
    Serial.println("\n🛬 ═════════════════════════════════");
    Serial.println("🛬 LANDOLÓ AKTIVÁLVA!");
    Serial.print("🛬 Servók NYITVA (");
    Serial.print(SERVO_OPEN_POSITION);
    Serial.println("°)");
    Serial.print("🛬 LED villogása: ");
    Serial.print(LED_BLINK_DURATION);
    Serial.println(" ms");
    Serial.println("🛬 ═════════════════════════════════");
  #endif
}

// ═════════════════════════════════════════════════════════
// ESP-NOW PARANCS KEZELŐ
// ═════════════════════════════════════════════════════════
void handleCommand(byte cmd) {
  if (cmd == 1) {
    activateLanding();
  }
}

// ═════════════════════════════════════════════════════════
// LED VILLOGÁS KEZELÉS & DEEP SLEEP
// ═════════════════════════════════════════════════════════
void handleLedBlink() {
  if (!led.getIsBlinking()) return;
  
  bool blinkFinished = led.update();
  
  if (blinkFinished) {
    // LED kikapcsolás
    led.turnOff();
    
    // WiFi kikapcsolás
    comm.disconnect();
    
    delay(WIFI_DISCONNECT_DELAY);
    
    // Deep sleep
    sleepMgr.enterDeepSleep();
  }
}

// ═════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════
void setup() {
  // Soros port inicializálás
  #if DEBUG_ENABLED
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
  #endif
  
  // Boot számláló növelése
  bootCount++;
  
  // Boot info kiírása
  sleepMgr.printBootInfo(bootCount);
  
  // Késleltetés reset után
  if (bootCount > 1) {
    delay(SERVO_INIT_DELAY);
  }
  
  // LED inicializálás
  led.init();
  
  // Wakeup gomb inicializálás
  sleepMgr.initWakeupButton();
  
  #if DEBUG_ENABLED
    Serial.println("✅ GPIO pinok inicializálva");
  #endif
  
  // Servók inicializálása és alappozícióba állítás
  servos.init();
  servos.setToStartPosition();
  
  // ESP-NOW inicializálás
  if (!comm.init(handleCommand)) {
    #if DEBUG_ENABLED
      Serial.println("❌ Kommunikáció inicializálása sikertelen!");
    #endif
    return;
  }
  
  #if DEBUG_ENABLED
    Serial.println("════════════════════════════════════");
    Serial.println("✅ Landoló KÉSZEN - Parancsra vár!");
    Serial.println("════════════════════════════════════\n");
  #endif
}

// ═════════════════════════════════════════════════════════
// LOOP
// ═════════════════════════════════════════════════════════
void loop() {
  handleLedBlink();
  delay(10);
}