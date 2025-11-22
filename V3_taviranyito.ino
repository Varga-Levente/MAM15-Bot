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

// ===== DEBOUNCE BEÁLLÍTÁSOK =====
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;  // 50 ms debounce idő

// ===== KÜLDÉSI BEÁLLÍTÁSOK =====
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 30;  // 30 ms küldési intervallum (33 Hz)

// ===== CRC számoló objektum =====
CRC16 crcCalculator(CRC_POLYNOMIAL, CRC_INITIAL_VALUE, CRC_FINAL_XOR_VALUE, true, true);

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    Serial.println("🎮 Távirányító indítása...");
  }

  // ===== Gomb bemenetek beállítása =====
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

  // ===== LoRa BEÁLLÍTÁSOK OPTIMALIZÁLÁSA =====
  LoRa.setTxPower(20);  // Max teljesítmény (20 dBm)
  LoRa.setSpreadingFactor(7);  // Alacsony spreading factor gyorsabb átvitelhez
  LoRa.setSignalBandwidth(125E3);  // Szabvány sávszélesség
  
  if (DEBUG) Serial.println("✅ Távirányító készen áll - LoRa adó módban...");
}

/**
 * Debounce-olással ellátott gomb állapot olvasás
 */
bool readDebouncedButton(int pin) {
  static bool lastStableState[6] = {false}; // Minden pinhez tároljuk az állapotot
  static unsigned long lastDebounceTimes[6] = {0};
  
  bool currentState = !digitalRead(pin); // Aktív alacsony, ezért invertáljuk
  int pinIndex = pin - 25; // Pin index számítás (25-33 közöttiek)
  
  if (currentState != lastStableState[pinIndex]) {
    lastDebounceTimes[pinIndex] = millis();
  }
  
  if ((millis() - lastDebounceTimes[pinIndex]) > DEBOUNCE_DELAY) {
    if (currentState != lastStableState[pinIndex]) {
      lastStableState[pinIndex] = currentState;
    }
  }
  
  return lastStableState[pinIndex];
}

// =============================== FŰ PROGRAMHURK =================================
void loop() {
  unsigned long currentTime = millis();
  
  // ===== IDŐZÍTETT KÜLDÉS =====
  if (currentTime - lastSendTime < SEND_INTERVAL) {
    return; // Várunk a következő küldési intervallumig
  }
  lastSendTime = currentTime;

  byte motorCommandByte = 0; // Motor parancsok bitmezője

  // ===== GOMB ÁLLAPOTOK BEOLVASÁSA DEBOUNCE-OLVA =====
  // Előre gomb - Bal motor előre (bit 0)
  if (readDebouncedButton(FORWARD_BUTTON_PIN)) {
    motorCommandByte |= 0b00000001;
  }
  
  // Hátra gomb - Bal motor hátra (bit 1)  
  if (readDebouncedButton(BACKWARD_BUTTON_PIN)) {
    motorCommandByte |= 0b00000010;
  }
  
  // Jobbra gomb - Jobb motor előre (bit 2)
  if (readDebouncedButton(RIGHT_BUTTON_PIN)) {
    motorCommandByte |= 0b00000100;
  }
  
  // Balra gomb - Jobb motor hátra (bit 3)
  if (readDebouncedButton(LEFT_BUTTON_PIN)) {
    motorCommandByte |= 0b00001000;
  }

  // ===== SEBESSÉG VÁLTÓ GOMB KEZELÉSE DEBOUNCE-OLVA =====
  bool currentSpeedButtonState = readDebouncedButton(SPEED_CHANGE_BUTTON_PIN);
  
  // Rising edge észlelés - csak a gomb lenyomásának elején
  if (currentSpeedButtonState && !previousSpeedButtonState) {
    speedChangeFlag = true;
    if (DEBUG) Serial.println("⚡ Sebesség váltás kérése");
  } else {
    speedChangeFlag = false;
  }
  previousSpeedButtonState = currentSpeedButtonState;

  // ===== DEBUG INFORMÁCIÓ =====
  if (DEBUG) {
    static byte lastMotorCommand = 0;
    if (motorCommandByte != lastMotorCommand) {
      Serial.printf("🎮 Motor parancs: 0x%02X - ", motorCommandByte);
      if (motorCommandByte & 0b0001) Serial.print("ELŐRE ");
      if (motorCommandByte & 0b0010) Serial.print("HÁTRA ");
      if (motorCommandByte & 0b0100) Serial.print("JOBBRA ");
      if (motorCommandByte & 0b1000) Serial.print("BALRA ");
      if (motorCommandByte == 0) Serial.print("STOP");
      Serial.println();
      lastMotorCommand = motorCommandByte;
    }
  }

  // ===== ADAT CSOMAG ÖSSZEÁLLÍTÁSA =====
  uint8_t transmitPacket[3];
  transmitPacket[0] = TARGET_ROBOT_ID;    // Cél robot ID
  transmitPacket[1] = motorCommandByte;   // Motor parancsok
  transmitPacket[2] = speedChangeFlag;    // Sebesség váltás jelző

  // ===== CRC SZÁMÍTÁSA =====
  crcCalculator.restart();
  crcCalculator.add(transmitPacket, 3);
  uint16_t packetCRC = crcCalculator.getCRC();

  // ===== LoRa CSOMAG KÜLDÉSE =====
  LoRa.beginPacket();
  LoRa.write(transmitPacket, 3);        // 3 bájt adat
  LoRa.write(packetCRC >> 8);           // CRC magas byte
  LoRa.write(packetCRC & 0xFF);         // CRC alacsony byte
  
  if (LoRa.endPacket()) {
    // Sikeres küldés
    if (DEBUG) {
      static unsigned long packetCount = 0;
      packetCount++;
      if (packetCount % 50 == 0) { // Minden 50. csomagnál
        Serial.printf("📡 %lu csomag sikeresen elküldve\n", packetCount);
      }
    }
  } else {
    if (DEBUG) Serial.println("❌ Küldési hiba!");
  }
}
