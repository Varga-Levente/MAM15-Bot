#include "settings.h"
#include "motor_control.h"
#include "lora_communication.h"
#include "espnow_communication.h"
#include "packet_handler.h"
#include "failsafe.h"

// ═════════════════════════════════════════════════════════
// GLOBÁLIS OBJEKTUMOK
// ═════════════════════════════════════════════════════════
MotorControl motors;
LoRaCommunication lora;
ESPNowCommunication espnow;
PacketHandler packetHandler;
Failsafe failsafe;

// ═════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════
void setup() {
  #if DEBUG_ENABLED
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    for (int i = 0; i < 5; i++) {
      Serial.println();
    }
    
    Serial.println("════════════════════════════════════");
    Serial.println("🤖 MOTORVEZÉRLŐ ROBOT INDÍTÁSA");
    Serial.println("════════════════════════════════════");
  #endif
  
  // ===== PWM INICIALIZÁLÁSA =====
  if (!motors.init()) {
    #if DEBUG_ENABLED
      Serial.println("❌ Kritikus hiba: PWM inicializálás sikertelen!");
      Serial.println("A rendszer leáll.");
    #endif
    while (1) {
      delay(1000);
    }
  }
  
  delay(100);
  
  // ===== LoRa INICIALIZÁLÁSA =====
  if (!lora.init()) {
    #if DEBUG_ENABLED
      Serial.println("❌ Kritikus hiba: LoRa inicializálás sikertelen!");
      Serial.println("A rendszer leáll.");
    #endif
    while (1) {
      delay(1000);
    }
  }
  
  delay(100);
  
  // ===== ESP-NOW INICIALIZÁLÁSA =====
  if (!espnow.init()) {
    #if DEBUG_ENABLED
      Serial.println("⚠️ Figyelmeztetés: ESP-NOW inicializálás sikertelen!");
      Serial.println("A landoló parancsok nem működnek.");
    #endif
  }
  
  // ===== FAILSAFE INICIALIZÁLÁSA =====
  failsafe.init();
  
  #if DEBUG_ENABLED
    Serial.println("════════════════════════════════════");
    Serial.println("✅ Motorvezérlő KÉSZEN");
    Serial.println("✅ LoRa vevő + ESP-NOW adó aktív");
    Serial.println("✅ LED_FLASH_PIN toggle funkció készen");
    Serial.println("════════════════════════════════════");
  #endif
}

// ═════════════════════════════════════════════════════════
// LOOP
// ═════════════════════════════════════════════════════════
void loop() {
  // ===== LORA HEALTH CHECK =====
  lora.checkHealth();
  
  // ===== CSAK AKKOR OLVAS, HA LORA OK =====
  if (lora.getState() != LORA_OK) {
    // Ha LoRa nem OK, motorok leállítása
    motors.stop();
    return;
  }
  
  // ===== CSOMAG FOGADÁS =====
  int receivedPacketSize = lora.parsePacket();
  if (!receivedPacketSize) {
    // Nincs csomag - failsafe ellenőrzés
    if (failsafe.check()) {
      motors.stop();
    }
    return;
  }
  
  // ===== CSOMAG ÉRKEZETT =====
  // Időzítők frissítése
  lora.updateReceivedTime();
  failsafe.reset();
  
  // ===== CSOMAG MÉRET ELLENŐRZÉS =====
  if (!packetHandler.validatePacketSize(receivedPacketSize)) {
    return;
  }
  
  // ===== CSOMAG OLVASÁSA =====
  byte receivedPacket[PACKET_SIZE];
  for (int i = 0; i < PACKET_SIZE; i++) {
    receivedPacket[i] = lora.read();
  }
  
  // ===== CSOMAG FELDOLGOZÁSA =====
  PacketData data = packetHandler.parsePacket(receivedPacket);
  
  if (!data.valid) {
    return;
  }
  
  // ═════════════════════════════════════════════════════════
  // LANDOLÓ GOMB KEZELÉSE
  // ═════════════════════════════════════════════════════════
  // Az ESP-NOW osztály automatikusan kezeli a funkciót:
  //   - Ha ESP-NOW aktív: landoló parancs küldése
  //   - Ha ESP-NOW inaktív: PIN22 toggle
  espnow.handleLandingState(data.landingState);
  
  // ===== SEBESSÉG VÁLTÁS KEZELÉSE =====
  motors.handleSpeedButton(data.speedButtonPressed);
  
  // ===== MOTOR PARANCS VÉGREHAJTÁSA =====
  motors.executeCommand(data.motorCommand);
}