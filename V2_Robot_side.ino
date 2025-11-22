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

// ===== Robot azonosító =====
#define ROBOT_ID 69

// ===== CRC ellenőrzés beállításai =====
#define CRC_POLYNOMIAL 0x1021
#define CRC_INITIAL_VALUE 0xFFFF
#define CRC_FINAL_XOR_VALUE 0x0000  // CRC végső XOR értéke

// ===== Motor vezérlő pin definíciók =====
#define LEFT_MOTOR_FORWARD_PIN 32
#define LEFT_MOTOR_REVERSE_PIN 27
#define RIGHT_MOTOR_FORWARD_PIN 25
#define RIGHT_MOTOR_REVERSE_PIN 26

// ===== PWM beállítások =====
#define PWM_FREQUENCY 1000    // 1 kHz PWM frekvencia
#define PWM_RESOLUTION 8      // 8 bites PWM felbontás (0-255)

// ===== Sebesség szintek =====
int motorSpeedLevels[3] = {255, 120, 60};  // Sebesség szintek
int currentSpeedLevelIndex = 0;        // Jelenlegi sebesség szint indexe
bool previousSpeedButtonState = false; // Előző sebesség gomb állapota

// ===== LoRa ReInit =====
const int maxRetries = 5;

// ===== Biztonsági beállítások (Failsafe) =====
unsigned long lastReceivedPacketTime = 0;        // Utolsó csomag érkezésének ideje
const unsigned long FAILSAFE_TIMEOUT_MS = 300;   // 300 ms után leállítja a motort

// ===== CRC számoló objektum =====
CRC16 crcCalculator(CRC_POLYNOMIAL, CRC_INITIAL_VALUE, CRC_FINAL_XOR_VALUE, true, true);

// ===== LoRa újrainicializáló függvény =====
bool ensureLoRaInitialized() {
  int retries = 0;
  while (!LoRa.begin(LORA_FREQUENCY) && retries < maxRetries) {
    if (DEBUG) Serial.println("⚠️ LoRa modul nem elérhető, újrapróbálkozás...");
    retries++;
    delay(500);
  }

  if (retries == maxRetries) {
    if (DEBUG) Serial.println("❌ LoRa modul újrainicializálás sikertelen!");
    return false;
  }

  if (DEBUG) Serial.println("✅ LoRa újrainicializálva");
  return true;
}

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    Serial.println("🤖 Robot indítása...");
  }

  // ===== PWM INICIALIZÁLÁSA AZ ÚJ LEDC API-VAL =====
  bool pwmSetupSuccessful = true;
  
  if (!ledcAttach(LEFT_MOTOR_FORWARD_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    if (DEBUG) Serial.println("❌ Hiba: Bal motor előre PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }
  
  if (!ledcAttach(LEFT_MOTOR_REVERSE_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    if (DEBUG) Serial.println("❌ Hiba: Bal motor hátra PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }
  
  if (!ledcAttach(RIGHT_MOTOR_FORWARD_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    if (DEBUG) Serial.println("❌ Hiba: Jobb motor előre PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }
  
  if (!ledcAttach(RIGHT_MOTOR_REVERSE_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    if (DEBUG) Serial.println("❌ Hiba: Jobb motor hátra PWM inicializálás sikertelen!");
    pwmSetupSuccessful = false;
  }

  if (!pwmSetupSuccessful) {
    if (DEBUG) Serial.println("❌ Kritikus hiba: PWM inicializálás sikertelen! A rendszer leáll.");
    while (1) {
      delay(1000);
    }
  }

  if (DEBUG) Serial.println("✅ PWM inicializálás sikeres");

  // ===== LoRa kommunikáció inicializálása =====
  LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    if (DEBUG) Serial.println("❌ Hiba: LoRa inicializálás sikertelen!");
    while (1) {
      delay(1000);
    }
  }

  if (DEBUG) Serial.println("✅ Robot készen áll - LoRa vevő módban...");
}

// =============================== MOTOR VEZÉRLŐ FÜGGVÉNYEK ==============================
void controlMotors(bool leftForward, bool leftBackward, bool rightForward, bool rightBackward) {
  int currentSpeed = motorSpeedLevels[currentSpeedLevelIndex];
  
  ledcWrite(LEFT_MOTOR_FORWARD_PIN, leftForward ? currentSpeed : 0);
  ledcWrite(LEFT_MOTOR_REVERSE_PIN, leftBackward ? currentSpeed : 0);
  ledcWrite(RIGHT_MOTOR_FORWARD_PIN, rightForward ? currentSpeed : 0);
  ledcWrite(RIGHT_MOTOR_REVERSE_PIN, rightBackward ? currentSpeed : 0);
}

void stopAllMotors() {
  controlMotors(false, false, false, false);
}

/**
 * Gomb állapotok kiírása a kapott parancs byte alapján
 * @param command - A motor parancs byte (8 bites)
 */
void printButtonStates(byte command) {
  if (!DEBUG) return;
  
  Serial.print("🎮 Gomb állapotok: [");
  
  // Bitminta kiírása
  for (int i = 7; i >= 0; i--) {
    Serial.print(bitRead(command, i));
    if (i == 4) Serial.print(" "); // Szóköz a jobb/bal motorok között
  }
  Serial.print("] - ");
  
  // Gombok szöveges értelmezése
  bool anyButtonPressed = false;
  
  if (command & 0b0001) { // Bal motor előre
    Serial.print("BAL_ELŐRE ");
    anyButtonPressed = true;
  }
  if (command & 0b0010) { // Bal motor hátra
    Serial.print("BAL_HÁTRA ");
    anyButtonPressed = true;
  }
  if (command & 0b0100) { // Jobb motor előre
    Serial.print("JOBB_ELŐRE ");
    anyButtonPressed = true;
  }
  if (command & 0b1000) { // Jobb motor hátra
    Serial.print("JOBB_HÁTRA ");
    anyButtonPressed = true;
  }
  
  if (!anyButtonPressed) {
    Serial.print("NINCS GOMB NYOMVA");
  }
  
  Serial.println();
}

/**
 * Ellenőrzi az érvénytelen gomb kombinációkat
 * @param command - A motor parancs byte
 * @return true - ha érvényes a kombináció, false - ha érvénytelen
 */
bool validateCommand(byte command) {
  // Ellenőrizzük, hogy ugyanaz a motor ne menjen egyszerre előre és hátra
  bool leftConflict = (command & 0b0001) && (command & 0b0010);  // Bal előre + hátra
  bool rightConflict = (command & 0b0100) && (command & 0b1000); // Jobb előre + hátra
  
  if (leftConflict) {
    if (DEBUG) Serial.println("❌ ÉRVÉNYTELEN: Bal motor egyszerre előre és hátra!");
    return false;
  }
  
  if (rightConflict) {
    if (DEBUG) Serial.println("❌ ÉRVÉNYTELEN: Jobb motor egyszerre előre és hátra!");
    return false;
  }
  
  return true;
}

// =============================== FŰ PROGRAMHURK =================================
void loop() {
  // Ellenőrizzük, hogy a LoRa aktív-e
  if (LoRa.parsePacket() == 0 && !ensureLoRaInitialized()) {
    // Ha nem tudjuk újrainicializálni, várunk egy kicsit
    delay(500);
    return;
  }

  // ===== BIZTONSÁGI LEÁLLÍTÁS (Failsafe) =====
  if (millis() - lastReceivedPacketTime > FAILSAFE_TIMEOUT_MS) {
    stopAllMotors();
    lastReceivedPacketTime = millis() - FAILSAFE_TIMEOUT_MS + 1000;
  }

  // ===== LoRa CSOMAG FELDOLGOZÁSA =====
  int receivedPacketSize = LoRa.parsePacket();
  if (!receivedPacketSize) return;
  
  if (receivedPacketSize != 5) {
    if (DEBUG) Serial.println("⚠️  Figyelmeztetés: Hibás csomag méret!");
    return;
  }

  byte receivedPacket[5];
  for (int byteIndex = 0; byteIndex < 5; byteIndex++) {
    receivedPacket[byteIndex] = LoRa.read();
  }

  // ===== CRC ELLENŐRZÉS =====
  uint16_t receivedCRC = (receivedPacket[3] << 8) | receivedPacket[4];
  
  crcCalculator.restart();
  crcCalculator.add(receivedPacket, 3);
  uint16_t calculatedCRC = crcCalculator.getCRC();

  if (receivedCRC != calculatedCRC) {
    if (DEBUG) Serial.println("❌ Hibás CRC - csomag elvetve!");
    return;
  }

  // ===== ROBOT AZONOSÍTÓ ELLENŐRZÉSE =====
  if (receivedPacket[0] != ROBOT_ID) {
    return;
  }

  lastReceivedPacketTime = millis();

  // ===== CSOMAG ADATAINAK KINYERÉSE =====
  byte motorCommand = receivedPacket[1];
  bool speedButtonPressed = receivedPacket[2];

  // ===== SEBESSÉG VÁLTÁS KEZELÉSE =====
  if (speedButtonPressed && !previousSpeedButtonState) {
    currentSpeedLevelIndex = (currentSpeedLevelIndex + 1) % 3;
    if (DEBUG) {
      Serial.printf("⚡ Sebesség váltás: %d → %d\n", 
                    motorSpeedLevels[(currentSpeedLevelIndex + 1) % 3], 
                    motorSpeedLevels[currentSpeedLevelIndex]);
    }
  }
  previousSpeedButtonState = speedButtonPressed;

  // ===== GOMB ÁLLAPOTOK KIÍRÁSA =====
  printButtonStates(motorCommand);

  // ===== PARANCS ÉRVÉNYESSÉGÉNEK ELLENŐRZÉSE =====
  if (!validateCommand(motorCommand)) {
    if (DEBUG) Serial.println("🛑 Motorok leállítva érvénytelen parancs miatt");
    stopAllMotors();
    return;
  }

  // ===== MOTOROK VEZÉRLÉSE =====
  if (motorCommand == 0) {
    stopAllMotors();
    if (DEBUG) Serial.println("🛑 Minden motor leállítva");
  } else {
    // Motor parancsok végrehajtása bitenkénti ellenőrzéssel
    controlMotors(
      motorCommand & 0b0001,   // Bal motor előre (LSB)
      motorCommand & 0b0010,   // Bal motor hátra
      motorCommand & 0b0100,   // Jobb motor előre
      motorCommand & 0b1000    // Jobb motor hátra (MSB)
    );
    
    // Mozgás irányának kiírása
    if (DEBUG) {
      Serial.print("🚗 Mozgás: ");
      if ((motorCommand & 0b0001) && (motorCommand & 0b0100)) {
        Serial.println("EGYENESEN ELŐRE");
      } else if ((motorCommand & 0b0010) && (motorCommand & 0b1000)) {
        Serial.println("EGYENESEN HÁTRA");
      } else if (motorCommand & 0b0001) {
        Serial.println("BALRA FORDUL");
      } else if (motorCommand & 0b0100) {
        Serial.println("JOBBRA FORDUL");
      } else if (motorCommand & 0b0010) {
        Serial.println("BALRA HÁTRA");
      } else if (motorCommand & 0b1000) {
        Serial.println("JOBBRA HÁTRA");
      }
    }
  }
  
  if (DEBUG) Serial.println("---"); // Elválasztó a következő csomaghoz
}
