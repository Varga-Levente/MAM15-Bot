#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

// ===== DEBUG BEÁLLÍTÁS =====
const bool DEBUG = true;

// ===== SERVO BEÁLLÍTÁSOK =====
#define SERVO1_PIN 12  // Első servo pin (módosítsd szükség szerint)
#define SERVO2_PIN 13  // Második servo pin (módosítsd szükség szerint)

Servo servo1;
Servo servo2;

// ===== LANDOLÓ ÁLLAPOT =====
bool landingActive = false;

// ===== MOTORVEZÉRLŐ MAC CÍME (opcionális, ha szűrni szeretnéd) =====
uint8_t motorControllerMAC[] = {0xF0, 0x24, 0xF9, 0x0E, 0x6D, 0xE4};

// ===== ESPNow FOGADÁSI CALLBACK =====
void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len != 1) {
    if (DEBUG) Serial.println("⚠️  Hibás csomag méret!");
    return;
  }

  // MAC cím ellenőrzés (opcionális biztonsági szint)
  if (memcmp(mac, motorControllerMAC, 6) != 0) {
    if (DEBUG) {
      Serial.print("⚠️  Ismeretlen küldő MAC: ");
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
    }
    return;
  }

  byte command = incomingData[0];
  
  if (DEBUG) {
    Serial.print("📥 ESPNow parancs fogadva: ");
    Serial.println(command);
  }

  // Parancs feldolgozása
  if (command == 1 && !landingActive) {
    // AKTIVÁLÁS
    landingActive = true;
    activateLanding();
  } else if (command == 0 && landingActive) {
    // DEAKTIVÁLÁS
    landingActive = false;
    deactivateLanding();
  }
}

// ===== LANDOLÓ AKTIVÁLÁS =====
void activateLanding() {
  if (DEBUG) Serial.println("🛬 LANDOLÓ AKTIVÁLVA - Servók 90°-ra mozgatása");
  
  servo1.write(90);
  servo2.write(90);
  
  if (DEBUG) Serial.println("✅ Servók pozícióban");
}

// ===== LANDOLÓ DEAKTIVÁLÁS =====
void deactivateLanding() {
  if (DEBUG) Serial.println("🔼 LANDOLÓ DEAKTIVÁLVA - Servók 0°-ra visszaállítása");
  
  servo1.write(0);
  servo2.write(0);
  
  if (DEBUG) Serial.println("✅ Servók alaphelyzetben");
}

// ===== SETUP =====
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    Serial.println("🛬 Landoló egység indítása...");
  }

  // ===== SERVO INICIALIZÁLÁS =====
  // ESP32Servo könyvtár automatikusan használja a PWM channel-eket
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  
  // Servók alaphelyzetbe állítása
  servo1.write(0);
  servo2.write(0);
  
  if (DEBUG) Serial.println("✅ Servók inicializálva és alaphelyzetben");

  // ===== WiFi STATION MODE =====
  WiFi.mode(WIFI_STA);
  
  if (DEBUG) {
    Serial.print("📍 Landoló ESP MAC címe: ");
    Serial.println(WiFi.macAddress());
  }

  // ===== ESPNow INICIALIZÁLÁS =====
  if (esp_now_init() != ESP_OK) {
    if (DEBUG) Serial.println("❌ ESPNow inicializálás sikertelen!");
    return;
  }
  
  if (DEBUG) Serial.println("✅ ESPNow inicializálva");

  // ===== FOGADÁSI CALLBACK REGISZTRÁLÁSA =====
  esp_now_register_recv_cb(onDataReceived);
  
  if (DEBUG) Serial.println("✅ Landoló egység készen áll - ESPNow fogadó módban...");
}

// ===== LOOP =====
void loop() {
  // ESPNow automatikusan kezeli a fogadást a callback-en keresztül
  // Itt egyéb feladatokat végezhetsz, ha szükséges
  
  delay(10);  // Kis késleltetés az ESP stabilitása érdekében
}