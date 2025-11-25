#include <SPI.h>
#include "settings.h"
#include "motor_controller.h"
#include "lora_receiver.h"
#include "espnow_transmitter.h"

// ===== GLOBÁLIS OBJEKTUMOK =====
MotorController motorController;
LoRaReceiver loraReceiver;
ESPNowTransmitter espnowTransmitter;

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
    Serial.begin(115200);
    delay(100);
    
    for (int i = 0; i < 5; i++) {
      Serial.println();
    }
    
    Serial.println("╔════════════════════════════════════════════╗");
    Serial.println("║   🤖 MOTORVEZÉRLŐ ROBOT INDÍTÁSA 🤖     ║");
    Serial.println("║     (LoRa vevő + ESP-NOW adó)             ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.println();
  }
  
  // ===== Motor inicializálás =====
  if (!motorController.init()) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
      Serial.println("❌ KRITIKUS HIBA: Motor inicializálás sikertelen!");
    }
    while (1) delay(1000);
  }
  
  // ===== LoRa vevő inicializálás =====
  if (!loraReceiver.init()) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
      Serial.println("❌ KRITIKUS HIBA: LoRa inicializálás sikertelen!");
    }
    while (1) delay(1000);
  }
  
  // ===== ESP-NOW inicializálás =====
  if (!espnowTransmitter.init()) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
      Serial.println("❌ KRITIKUS HIBA: ESP-NOW inicializálás sikertelen!");
    }
    while (1) delay(1000);
  }
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
    Serial.println("════════════════════════════════════════════");
    Serial.println("✅ Motorvezérlő robot készen áll!");
    Serial.println("════════════════════════════════════════════");
    Serial.println();
  }
}

// =============================== FŐ PROGRAMHURÖK =================================
void loop() {
  // ===== Health check =====
  loraReceiver.checkHealth();
  
  // ===== Failsafe: motorok leállítása ha nincs kapcsolat =====
  unsigned long timeSinceLastPacket = millis() - loraReceiver.getLastPacketTime();
  if (timeSinceLastPacket > SafetySettings::FAILSAFE_TIMEOUT_MS) {
    motorController.stopAll();
    
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_FAILSAFE) {
      static unsigned long lastFailsafeLog = 0;
      if (millis() - lastFailsafeLog > 1000) {
        Serial.println("⚠️ FAILSAFE: Nincs kapcsolat - motorok leállítva");
        lastFailsafeLog = millis();
      }
    }
    return;
  }
  
  // ===== Csomag fogadása csak ha a LoRa健康 =====
  if (!loraReceiver.isHealthy()) {
    return;
  }
  
  byte receivedPacket[PacketSettings::EXPECTED_SIZE];
  if (!loraReceiver.receivePacket(receivedPacket, PacketSettings::EXPECTED_SIZE)) {
    return;
  }
  
  // ===== Robot ID ellenőrzés =====
  if (receivedPacket[0] != RobotSettings::ROBOT_ID) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_COMMUNICATION) {
      Serial.printf("⚠️ Csomag másik robotnak: %d\n", receivedPacket[0]);
    }
    return;
  }
  
  // ===== Adatok kinyerése =====
  byte motorCommand = receivedPacket[1];
  bool speedButtonPressed = receivedPacket[2];
  bool landingState = receivedPacket[3];
  
  // ===== Landoló parancs továbbítása =====
  espnowTransmitter.sendLandingCommand(landingState);
  
  // ===== Sebesség váltás =====
  motorController.handleSpeedChange(speedButtonPressed);
  
  // ===== Motor vezérlés =====
  if (!motorController.validateCommand(motorCommand)) {
    motorController.stopAll();
    return;
  }
  
  if (motorCommand == 0) {
    motorController.stopAll();
  } else {
    motorController.control(
      motorCommand & 0b0001,  // Bal előre
      motorCommand & 0b0010,  // Bal hátra
      motorCommand & 0b0100,  // Jobb előre
      motorCommand & 0b1000   // Jobb hátra
    );
  }
}