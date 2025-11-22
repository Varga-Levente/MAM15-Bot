#include <SPI.h>
#include <LoRa.h>
#include <CRC.h>  // Rob Tillaart CRC könyvtár

// ===== DEBUG BEÁLLÍTÁS =====
const bool DEBUG = true;  // true = Serial kiírás engedélyezve, false = nincs kiírás

// ===== LoRa kommunikációs beállítások =====
#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RESET_PIN 14
#define LORA_DIO0_PIN 2
#define LORA_FREQUENCY 433E6  // 433 MHz-es sáv

// ===== Cél robot azonosítója =====
#define TARGET_ROBOT_ID 69

// ===== CRC ellenőrzés beállításai =====
#define CRC_POLYNOMIAL 0x1021
#define CRC_INITIAL_VALUE 0xFFFF
#define CRC_FINAL_XOR_VALUE 0x0000  // CRC végső XOR értéke

// ===== Irányító gombok pin definíciói =====
#define FORWARD_BUTTON_PIN 32
#define BACKWARD_BUTTON_PIN 33  
#define RIGHT_BUTTON_PIN 25
#define LEFT_BUTTON_PIN 26
#define SPEED_CHANGE_BUTTON_PIN 27

// ===== Gomb állapot változók =====
bool previousSpeedButtonState = false;  // Előző sebesség gomb állapot
bool speedChangeFlag = false;           // Sebesség váltás jelző

// ===== CRC számoló objektum - JAVÍTOTT KONSTRUKTOR =====
// Paraméterek: polynomial, initial, xorOut, reverseIn, reverseOut
CRC16 crcCalculator(CRC_POLYNOMIAL, CRC_INITIAL_VALUE, CRC_FINAL_XOR_VALUE, true, true);

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    Serial.println("🎮 Távirányító indítása...");
  }

  // ===== Gomb bemenetek beállítása =====
  // Minden gomb bemenet felhúzó ellenállással (INPUT_PULLUP)
  pinMode(FORWARD_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BACKWARD_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEED_CHANGE_BUTTON_PIN, INPUT_PULLUP);

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

// =============================== FŰ PROGRAMHURK =================================
void loop() {
  byte motorCommandByte = 0; // Motor parancsok bitmezője

  // ===== GOMB ÁLLAPOTOK BEOLVASÁSA ÉS PARANCCSÁ ALAKÍTÁSA =====
  // Minden gomb aktív alacsony (LOW), mert PULLUP bemenetek
  
  // Előre gomb - Bal motor előre (bit 0)
  if (!digitalRead(FORWARD_BUTTON_PIN)) {
    motorCommandByte |= 0b00000001;
  }
  
  // Hátra gomb - Bal motor hátra (bit 1)  
  if (!digitalRead(BACKWARD_BUTTON_PIN)) {
    motorCommandByte |= 0b00000010;
  }
  
  // Jobbra gomb - Jobb motor előre (bit 2)
  if (!digitalRead(RIGHT_BUTTON_PIN)) {
    motorCommandByte |= 0b00000100;
  }
  
  // Balra gomb - Jobb motor hátra (bit 3)
  if (!digitalRead(LEFT_BUTTON_PIN)) {
    motorCommandByte |= 0b00001000;
  }

  // ===== SEBESSÉG VÁLTÓ GOMB KEZELÉSE =====
  bool currentSpeedButtonState = !digitalRead(SPEED_CHANGE_BUTTON_PIN);
  
  // Rising edge észlelés - csak a gomb lenyomásának elején
  if (currentSpeedButtonState && !previousSpeedButtonState) {
    speedChangeFlag = true;
  } else {
    speedChangeFlag = false;
  }
  previousSpeedButtonState = currentSpeedButtonState;

  // ===== ADAT CSOMAG ÖSSZEÁLLÍTÁSA =====
  uint8_t transmitPacket[3];
  transmitPacket[0] = TARGET_ROBOT_ID;    // Cél robot ID
  transmitPacket[1] = motorCommandByte;   // Motor parancsok
  transmitPacket[2] = speedChangeFlag;    // Sebesség váltás jelző

  // ===== CRC SZÁMÍTÁSA =====
  crcCalculator.restart(); // CRC számoló alapállapotba
  crcCalculator.add(transmitPacket, 3); // Mind a 3 bájt hozzáadása
  uint16_t packetCRC = crcCalculator.getCRC(); // CRC kiszámítása

  // ===== LoRa CSOMAG KÜLDÉSE =====
  LoRa.beginPacket();
  LoRa.write(transmitPacket, 3);        // 3 bájt adat
  LoRa.write(packetCRC >> 8);           // CRC magas byte
  LoRa.write(packetCRC & 0xFF);         // CRC alacsony byte
  LoRa.endPacket();

  // ===== RÖVID KÉSLELTETÉS A KÖVETKEZŐ KÜLDÉS ELŐTT =====
  delay(60); // 60 ms késleltetés a következő csomag küldése előtt
}
