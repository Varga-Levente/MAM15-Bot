#include <SPI.h>
#include <LoRa.h>
#include <CRC.h>

// ===== DEBUG BEÁLLÍTÁS =====
const bool DEBUG = true;

// ===== LoRa kommunikációs beállítások =====
#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RESET_PIN 14
#define LORA_DIO0_PIN 2
#define LORA_FREQUENCY 433E6

// ===== Cél robot azonosítója =====
#define TARGET_ROBOT_ID 69

// ===== CRC ellenőrzés beállításai =====
#define CRC_POLYNOMIAL 0x1021
#define CRC_INITIAL_VALUE 0xFFFF
#define CRC_FINAL_XOR_VALUE 0x0000

// ===== Irányító gombok pin definíciói =====
#define FORWARD_BUTTON_PIN 32
#define BACKWARD_BUTTON_PIN 33  
#define RIGHT_BUTTON_PIN 25
#define LEFT_BUTTON_PIN 26
#define SPEED_CHANGE_BUTTON_PIN 27
#define LANDING_BUTTON_PIN 13  // ÚJ: Landoló gomb

// ===== Gomb állapot változók =====
bool previousSpeedButtonState = false;
bool speedChangeFlag = false;
bool previousLandingButtonState = false;  // ÚJ: Landoló gomb előző állapota
bool landingToggleFlag = false;           // ÚJ: Toggle flag

// ===== CRC számoló objektum =====
CRC16 crcCalculator(CRC_POLYNOMIAL, CRC_INITIAL_VALUE, CRC_FINAL_XOR_VALUE, true, true);

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    Serial.println("🎮 Távirányító indítása (landoló vezérléssel)...");
  }

  // ===== Gomb bemenetek beállítása =====
  pinMode(FORWARD_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BACKWARD_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEED_CHANGE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LANDING_BUTTON_PIN, INPUT_PULLUP);  // ÚJ

  // ===== LoRa kommunikáció inicializálása =====
  LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    if (DEBUG) Serial.println("❌ Hiba: LoRa inicializálás sikertelen!");
    while (1) {
      delay(1000);
    }
  }

  if (DEBUG) Serial.println("✅ Távirányító készen áll - LoRa adó módban...");
}

// =============================== FŐ PROGRAMHURÖK =================================
void loop() {
  byte motorCommandByte = 0;

  // ===== GOMB ÁLLAPOTOK BEOLVASÁSA =====
  if (!digitalRead(FORWARD_BUTTON_PIN)) {
    motorCommandByte |= 0b00000001;
  }
  
  if (!digitalRead(BACKWARD_BUTTON_PIN)) {
    motorCommandByte |= 0b00000010;
  }
  
  if (!digitalRead(RIGHT_BUTTON_PIN)) {
    motorCommandByte |= 0b00000100;
  }
  
  if (!digitalRead(LEFT_BUTTON_PIN)) {
    motorCommandByte |= 0b00001000;
  }

  // ===== SEBESSÉG VÁLTÓ GOMB KEZELÉSE =====
  bool currentSpeedButtonState = !digitalRead(SPEED_CHANGE_BUTTON_PIN);
  
  if (currentSpeedButtonState && !previousSpeedButtonState) {
    speedChangeFlag = true;
  } else {
    speedChangeFlag = false;
  }
  previousSpeedButtonState = currentSpeedButtonState;

  // ===== LANDOLÓ GOMB KEZELÉSE (TOGGLE) =====
  bool currentLandingButtonState = !digitalRead(LANDING_BUTTON_PIN);
  
  // Rising edge észlelés - csak lenyomáskor toggle
  if (currentLandingButtonState && !previousLandingButtonState) {
    landingToggleFlag = !landingToggleFlag;  // Toggle váltás
    if (DEBUG) {
      Serial.print("🛬 Landoló toggle: ");
      Serial.println(landingToggleFlag ? "AKTIVÁLVA" : "DEAKTIVÁLVA");
    }
  }
  previousLandingButtonState = currentLandingButtonState;

  // ===== ADAT CSOMAG ÖSSZEÁLLÍTÁSA =====
  uint8_t transmitPacket[4];  // 3-ról 4-re bővítve!
  transmitPacket[0] = TARGET_ROBOT_ID;
  transmitPacket[1] = motorCommandByte;
  transmitPacket[2] = speedChangeFlag;
  transmitPacket[3] = landingToggleFlag;  // ÚJ: Landoló állapot

  // ===== CRC SZÁMÍTÁSA =====
  crcCalculator.restart();
  crcCalculator.add(transmitPacket, 4);  // Most 4 bájt
  uint16_t packetCRC = crcCalculator.getCRC();

  // ===== LoRa CSOMAG KÜLDÉSE =====
  LoRa.beginPacket();
  LoRa.write(transmitPacket, 4);        // 4 bájt adat
  LoRa.write(packetCRC >> 8);
  LoRa.write(packetCRC & 0xFF);
  LoRa.endPacket();

  delay(60);
}