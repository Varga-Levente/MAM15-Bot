#include <SPI.h>
#include <LoRa.h>
#include <CRC.h>  // Rob Tillaart CRC könyvtár

// ===== LoRa kommunikációs beállítások =====
#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RESET_PIN 14
#define LORA_DIO0_PIN 2
#define LORA_FREQUENCY 433E6  // 433 MHz-es sáv

// ===== Robot azonosító =====
#define ROBOT_ID 69

// ===== CRC ellenőrzés beállításai =====
#define CRC_POLYNOMIAL 0x1021
#define CRC_INITIAL_VALUE 0xFFFF

// ===== Motor vezérlő pin definíciók =====
#define LEFT_MOTOR_FORWARD_PIN 12
#define LEFT_MOTOR_REVERSE_PIN 13
#define RIGHT_MOTOR_FORWARD_PIN 27
#define RIGHT_MOTOR_REVERSE_PIN 26

// ===== PWM beállítások =====
#define PWM_FREQUENCY 1000    // 1 kHz PWM frekvencia
#define PWM_RESOLUTION 8      // 8 bites PWM felbontás (0-255)

// ===== Sebesség szintek =====
int motorSpeedLevels[2] = {120, 255};  // Alacsony és maximális sebesség
int currentSpeedLevelIndex = 0;        // Jelenlegi sebesség szint indexe
bool previousSpeedButtonState = false; // Előző sebesség gomb állapota

// ===== Biztonsági beállítások (Failsafe) =====
unsigned long lastReceivedPacketTime = 0;        // Utolsó csomag érkezésének ideje
const unsigned long FAILSAFE_TIMEOUT_MS = 300;   // 300 ms után leállítja a motort

// ===== CRC számoló objektum =====
CRC16 crcCalculator(CRC_POLYNOMIAL, CRC_INITIAL_VALUE, 0x0000, 0x0000, true, true);

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  Serial.begin(115200);
  Serial.println("🤖 Robot indítása...");

  // ===== PWM INICIALIZÁLÁSA AZ ÚJ LEDC API-VAL =====
  // Minden motor pin-hez PWM csatorna társítása automatikus csatorna kiválasztással
  
  bool pwmSetupSuccessful = true;
  
  // Bal motor előre PWM beállítása
  if (!ledcAttach(LEFT_MOTOR_FORWARD_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Serial.println("❌ Hiba: Bal motor előre PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }
  
  // Bal motor hátra PWM beállítása
  if (!ledcAttach(LEFT_MOTOR_REVERSE_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Serial.println("❌ Hiba: Bal motor hátra PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }
  
  // Jobb motor előre PWM beállítása
  if (!ledcAttach(RIGHT_MOTOR_FORWARD_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Serial.println("❌ Hiba: Jobb motor előre PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }
  
  // Jobb motor hátra PWM beállítása
  if (!ledcAttach(RIGHT_MOTOR_REVERSE_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Serial.println("❌ Hiba: Jobb motor hátra PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }

  // Ha valamelyik PWM beállítás sikertelen, hibaüzenet és leállás
  if (!pwmSetupSuccessful) {
    Serial.println("❌ Kritikus hiba: PWM inicializálás sikertelen! A rendszer leáll.");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("✅ PWM inicializálás sikeres");

  // ===== LoRa kommunikáció inicializálása =====
  LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("❌ Hiba: LoRa inicializálás sikertelen!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("✅ Robot készen áll - LoRa vevő módban...");
}

// =============================== MOTOR VEZÉRLŐ FÜGGVÉNYEK ==============================

/**
 * Motorok vezérlése a megadott irányok szerint
 * @param leftForward - Bal motor előre irány
 * @param leftBackward - Bal motor hátra irány  
 * @param rightForward - Jobb motor előre irány
 * @param rightBackward - Jobb motor hátra irány
 */
void controlMotors(bool leftForward, bool leftBackward, bool rightForward, bool rightBackward) {
  int currentSpeed = motorSpeedLevels[currentSpeedLevelIndex];
  
  // Motorok PWM jeleinek beállítása - most már közvetlenül a pin-ekre írunk
  // Az új LEDC API automatikusan kezeli a csatornákat a pin-ek mögött
  ledcWrite(LEFT_MOTOR_FORWARD_PIN, leftForward ? currentSpeed : 0);
  ledcWrite(LEFT_MOTOR_REVERSE_PIN, leftBackward ? currentSpeed : 0);
  ledcWrite(RIGHT_MOTOR_FORWARD_PIN, rightForward ? currentSpeed : 0);
  ledcWrite(RIGHT_MOTOR_REVERSE_PIN, rightBackward ? currentSpeed : 0);
}

/**
 * Minden motor azonnali leállítása
 */
void stopAllMotors() {
  controlMotors(false, false, false, false);
  Serial.println("🛑 Minden motor leállítva");
}

// =============================== FŰ PROGRAMHURK =================================
void loop() {
  // ===== BIZTONSÁGI LEÁLLÍTÁS (Failsafe) =====
  // Ha túl sok idő telt el az utolsó érvényes csomag óta, motorok leállítása
  if (millis() - lastReceivedPacketTime > FAILSAFE_TIMEOUT_MS) {
    stopAllMotors();
    // Biztonsági időzítő alaphelyzetbe állítása, hogy ne folyamatosan írja ki az üzenetet
    lastReceivedPacketTime = millis() - FAILSAFE_TIMEOUT_MS + 1000; // 1 másodperc múlva újra
  }

  // ===== LoRa CSOMAG FELDOLGOZÁSA =====
  int receivedPacketSize = LoRa.parsePacket();
  if (!receivedPacketSize) return; // Nincs csomag, kilépés
  
  // Csomag méret ellenőrzése (3 adat bájt + 2 CRC bájt = 5 bájt)
  if (receivedPacketSize != 5) {
    Serial.println("⚠️  Figyelmeztetés: Hibás csomag méret!");
    return;
  }

  // Csomag adatainak beolvasása
  byte receivedPacket[5];
  for (int byteIndex = 0; byteIndex < 5; byteIndex++) {
    receivedPacket[byteIndex] = LoRa.read();
  }

  // ===== CRC ELLENŐRZÉS =====
  uint16_t receivedCRC = (receivedPacket[3] << 8) | receivedPacket[4]; // CRC kinyerése
  
  crcCalculator.restart(); // CRC számoló alapállapotba
  crcCalculator.add(receivedPacket, 3); // Első 3 bájt hozzáadása
  uint16_t calculatedCRC = crcCalculator.getCRC(); // CRC kiszámítása

  // CRC ellenőrzése
  if (receivedCRC != calculatedCRC) {
    Serial.println("❌ Hibás CRC - csomag elvetve!");
    return;
  }

  // ===== ROBOT AZONOSÍTÓ ELLENŐRZÉSE =====
  if (receivedPacket[0] != ROBOT_ID) {
    return; // A csomag nem ehhez a robothoz tartozik
  }

  lastReceivedPacketTime = millis(); // Biztonsági időzítő alaphelyzetbe

  // ===== CSOMAG ADATAINAK KINYERÉSE =====
  byte motorCommand = receivedPacket[1];     // Motor parancs bitmező
  bool speedButtonPressed = receivedPacket[2]; // Sebesség váltó gomb állapota

  // ===== SEBESSÉG VÁLTÁS KEZELÉSE =====
  // Csak a gomb lenyomásának elején vált sebességet (rising edge detection)
  if (speedButtonPressed && !previousSpeedButtonState) {
    currentSpeedLevelIndex = (currentSpeedLevelIndex + 1) % 2;
    Serial.printf("⚡ Sebesség váltás: %d → %d\n", 
                  motorSpeedLevels[(currentSpeedLevelIndex + 1) % 2], 
                  motorSpeedLevels[currentSpeedLevelIndex]);
  }
  previousSpeedButtonState = speedButtonPressed;

  // ===== MOTOROK VEZÉRLÉSE =====
  if (motorCommand == 0) {
    // Nincs gombnyomás - motorok leállítása
    stopAllMotors();
  } else {
    // Motor parancsok végrehajtása bitenkénti ellenőrzéssel
    controlMotors(
      motorCommand & 0b0001,   // Bal motor előre (LSB)
      motorCommand & 0b0010,   // Bal motor hátra
      motorCommand & 0b0100,   // Jobb motor előre
      motorCommand & 0b1000    // Jobb motor hátra (MSB)
    );
  }
}
