#include <SPI.h>
#include "settings.h"
#include "button_handler.h"
#include "communication.h"

// ===== GLOBÁLIS OBJEKTUMOK =====
ButtonHandler buttonHandler;
Communication communication;

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  // Soros kommunikáció indítása
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
    Serial.begin(115200);
    delay(100);
    Serial.println("╔═══════════════════════════════════════════╗");
    Serial.println("║   🎮 TÁVIRÁNYÍTÓ RENDSZER INDÍTÁSA 🎮     ║");
    Serial.println("║     (Landoló vezérléssel)                 ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.println();
  }

  // Gombok inicializálása
  buttonHandler.init();

  // LoRa kommunikáció inicializálása
  if (!communication.init()) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
      Serial.println("❌ KRITIKUS HIBA: A rendszer nem indítható!");
    }
    while (1) {
      delay(1000);
    }
  }

  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_SYSTEM) {
    Serial.println("✅ Távirányító készen áll!");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
  }
}

// =============================== FŐ PROGRAMHURÖK =================================
void loop() {
  // Gombok beolvasása és parancsok generálása
  byte motorCommand = buttonHandler.readMotorCommands();
  bool speedFlag = buttonHandler.getSpeedChangeFlag();
  bool landingFlag = buttonHandler.getLandingToggleFlag();

  // Adat csomag küldése LoRa-n keresztül
  communication.sendPacket(
    RobotSettings::TARGET_ROBOT_ID,
    motorCommand,
    speedFlag,
    landingFlag
  );

  // Késleltetés a következő ciklusig
  delay(TimingSettings::LOOP_DELAY_MS);
}