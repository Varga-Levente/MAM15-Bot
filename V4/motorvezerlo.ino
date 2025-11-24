#include <SPI.h>
#include <LoRa.h>
#include <CRC.h>  // Rob Tillaart CRC könyvtár
#include <esp_now.h>
#include <WiFi.h>

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

// ===== Robot azonosító =====
#define ROBOT_ID 69

// ===== ESP-NOW beállítások =====
uint8_t landoloMAC[] = {0x1C, 0xDB, 0xD4, 0xD4, 0x0F, 0x80}; // Landoló MAC: 1C:DB:D4:D4:0F:80

// ===== CRC ellenőrzés beállításai =====
#define CRC_POLYNOMIAL 0x1021
#define CRC_INITIAL_VALUE 0xFFFF
#define CRC_FINAL_XOR_VALUE 0x0000

// ===== Motor vezérlő pin definíciók =====
#define LEFT_MOTOR_FORWARD_PIN 32
#define LEFT_MOTOR_REVERSE_PIN 27
#define RIGHT_MOTOR_FORWARD_PIN 25
#define RIGHT_MOTOR_REVERSE_PIN 26

// ===== PWM beállítások =====
#define PWM_FREQUENCY 1000
#define PWM_RESOLUTION 8

// ===== Sebesség szintek =====
int motorSpeedLevels[3] = {255, 120, 60};
int currentSpeedLevelIndex = 0;
bool previousSpeedButtonState = false;

// ===== Landoló állapot kezelés =====
bool previousLandingState = false;
bool currentLandingState = false;

// ===== Biztonsági beállítások (Failsafe) =====
unsigned long lastReceivedPacketTime = 0;
const unsigned long FAILSAFE_TIMEOUT_MS = 300;

// ===== LoRa HEALTH MONITOR =====
unsigned long lastLoRaHealthCheck = 0;
const unsigned long LORA_HEALTH_CHECK_INTERVAL = 5000;
bool loraModuleHealthy = true;
int loraRestartCount = 0;
const int MAX_LORA_RESTARTS = 3;

// ===== LoRa MODUL ÁLLAPOTVÁLTÁS DETEKCIÓ =====
enum LoRaState {
  LORA_OK,
  LORA_DISCONNECTED,
  LORA_RECONNECTING
};
LoRaState currentLoRaState = LORA_OK;
unsigned long loraStateChangeTime = 0;
const unsigned long LORA_RECONNECT_TIMEOUT = 10000;

// ===== CRC számoló objektum =====
CRC16 crcCalculator(CRC_POLYNOMIAL, CRC_INITIAL_VALUE, CRC_FINAL_XOR_VALUE, true, true);

// ===== ESP-NOW callback függvény =====
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (DEBUG) {
    Serial.print("📤 ESP-NOW küldés státusza: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ Sikeres" : "❌ Sikertelen");
  }
}

bool restartLoRa() {
  if (DEBUG) Serial.println("🔄 LoRa modul újraindítása...");
  
  LoRa.end();
  delay(100);
  
  digitalWrite(LORA_RESET_PIN, LOW);
  delay(10);
  digitalWrite(LORA_RESET_PIN, HIGH);
  delay(50);
  
  bool success = LoRa.begin(LORA_FREQUENCY);
  
  if (success) {
    loraRestartCount++;
    if (DEBUG) Serial.printf("✅ LoRa modul újraindítva (%d. alkalommal)\n", loraRestartCount);
  } else {
    if (DEBUG) Serial.println("❌ LoRa modul újraindítása sikertelen!");
  }
  
  return success;
}

void checkLoRaHealth() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastLoRaHealthCheck < LORA_HEALTH_CHECK_INTERVAL) {
    return;
  }
  lastLoRaHealthCheck = currentTime;
  
  bool loraWorking = false;
  
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    loraWorking = true;
  }
  
  if (!loraWorking && loraModuleHealthy) {
    if (DEBUG) Serial.println("⚠️  LoRa modul nem válaszol - újracsatlakozás indítása...");
    loraModuleHealthy = false;
    currentLoRaState = LORA_RECONNECTING;
    loraStateChangeTime = currentTime;
  }
  
  if (currentLoRaState == LORA_RECONNECTING) {
    if (currentTime - loraStateChangeTime > LORA_RECONNECT_TIMEOUT) {
      if (DEBUG) Serial.println("❌ LoRa újracsatlakozási időtúllépés!");
      currentLoRaState = LORA_DISCONNECTED;
    } else {
      if (restartLoRa()) {
        if (DEBUG) Serial.println("✅ LoRa modul sikeresen újracsatlakozott!");
        loraModuleHealthy = true;
        currentLoRaState = LORA_OK;
      } else {
        delay(1000);
      }
    }
  }
  
  if (loraRestartCount >= MAX_LORA_RESTARTS) {
    if (DEBUG) Serial.println("🛑 CRITICAL: Túl sok LoRa újraindítás - kézi beavatkozás szükséges!");
  }
}

// =============================== ALAPBEÁLLÍTÁS =================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    Serial.println("🤖 Robot indítása...");
  }

  // ===== PWM INICIALIZÁLÁSA =====
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

  pinMode(LORA_RESET_PIN, OUTPUT);
  digitalWrite(LORA_RESET_PIN, HIGH);

  // ===== ESP-NOW INICIALIZÁLÁS =====
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    if (DEBUG) Serial.println("❌ ESP-NOW inicializálás sikertelen!");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  
  // Peer konfigurálás
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, landoloMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    if (DEBUG) Serial.println("❌ ESP-NOW peer hozzáadás sikertelen!");
    return;
  }

  if (DEBUG) {
    Serial.println("✅ ESP-NOW inicializálva");
    Serial.print("📍 Motorvezérlő ESP MAC címe: ");
    Serial.println(WiFi.macAddress());
    Serial.print("📍 Cél landoló MAC címe: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", landoloMAC[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }

  if (DEBUG) Serial.println("✅ Robot készen áll - LoRa vevő + ESP-NOW adó módban...");
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

void printButtonStates(byte command) {
  if (!DEBUG) return;
  
  Serial.print("🎮 Gomb állapotok: [");
  
  for (int i = 7; i >= 0; i--) {
    Serial.print(bitRead(command, i));
    if (i == 4) Serial.print(" ");
  }
  Serial.print("] - ");
  
  bool anyButtonPressed = false;
  
  if (command & 0b0001) {
    Serial.print("BAL_ELŐRE ");
    anyButtonPressed = true;
  }
  if (command & 0b0010) {
    Serial.print("BAL_HÁTRA ");
    anyButtonPressed = true;
  }
  if (command & 0b0100) {
    Serial.print("JOBB_ELŐRE ");
    anyButtonPressed = true;
  }
  if (command & 0b1000) {
    Serial.print("JOBB_HÁTRA ");
    anyButtonPressed = true;
  }
  
  if (!anyButtonPressed) {
    Serial.print("NINCS GOMB NYOMVA");
  }
  
  Serial.println();
}

bool validateCommand(byte command) {
  bool leftConflict = (command & 0b0001) && (command & 0b0010);
  bool rightConflict = (command & 0b0100) && (command & 0b1000);
  
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

void sendLandingCommand(bool landingState) {
  byte command = landingState ? 1 : 0;
  
  esp_err_t result = esp_now_send(landoloMAC, &command, 1);
  
  if (DEBUG) {
    Serial.print("🛬 Landoló parancs küldése: ");
    Serial.print(landingState ? "AKTIVÁLÁS (1)" : "DEAKTIVÁLÁS (0)");
    Serial.print(" - Státusz: ");
    Serial.println(result == ESP_OK ? "✅ Küldve" : "❌ Hiba");
  }
}

// =============================== FŰ PROGRAMHURK =================================
void loop() {
  checkLoRaHealth();
  
  if (millis() - lastReceivedPacketTime > FAILSAFE_TIMEOUT_MS) {
    stopAllMotors();
    lastReceivedPacketTime = millis() - FAILSAFE_TIMEOUT_MS + 1000;
  }

  if (currentLoRaState == LORA_OK) {
    int receivedPacketSize = LoRa.parsePacket();
    if (!receivedPacketSize) return;
    
    if (receivedPacketSize != 6) {
      if (DEBUG) {
        Serial.print("⚠️  Figyelmeztetés: Hibás csomag méret! Várt: 6, Kapott: ");
        Serial.println(receivedPacketSize);
      }
      return;
    }

    byte receivedPacket[6];
    for (int byteIndex = 0; byteIndex < 6; byteIndex++) {
      receivedPacket[byteIndex] = LoRa.read();
    }

    // ===== CRC ELLENŐRZÉS =====
    uint16_t receivedCRC = (receivedPacket[4] << 8) | receivedPacket[5];
    
    crcCalculator.restart();
    crcCalculator.add(receivedPacket, 4);
    uint16_t calculatedCRC = crcCalculator.getCRC();

    if (receivedCRC != calculatedCRC) {
      if (DEBUG) Serial.println("❌ Hibás CRC - csomag elvetve!");
      return;
    }

    if (receivedPacket[0] != ROBOT_ID) {
      if (DEBUG) {
        Serial.print("⚠️  Csomag másik robotnak: ");
        Serial.println(receivedPacket[0]);
      }
      return;
    }

    lastReceivedPacketTime = millis();

    // ===== CSOMAG ADATAINAK KINYERÉSE =====
    byte motorCommand = receivedPacket[1];
    bool speedButtonPressed = receivedPacket[2];
    currentLandingState = receivedPacket[3];

    // ===== LANDOLÓ PARANCS TOVÁBBÍTÁSA =====
    if (currentLandingState != previousLandingState) {
      sendLandingCommand(currentLandingState);
      previousLandingState = currentLandingState;
      
      if (DEBUG) {
        Serial.print("🔄 Landoló állapot változás: ");
        Serial.println(currentLandingState ? "AKTÍV" : "INAKTÍV");
      }
    }

    // ===== SEBESSÉG VÁLTÁS KEZELÉSE =====
    if (speedButtonPressed && !previousSpeedButtonState) {
      currentSpeedLevelIndex = (currentSpeedLevelIndex + 1) % 3;
      if (DEBUG) {
        Serial.printf("⚡ Sebesség váltás: %d → %d\n", 
                      motorSpeedLevels[(currentSpeedLevelIndex + 2) % 3], 
                      motorSpeedLevels[currentSpeedLevelIndex]);
      }
    }
    previousSpeedButtonState = speedButtonPressed;

    printButtonStates(motorCommand);

    if (!validateCommand(motorCommand)) {
      if (DEBUG) Serial.println("🛑 Motorok leállítva érvénytelen parancs miatt");
      stopAllMotors();
      return;
    }

    if (motorCommand == 0) {
      stopAllMotors();
      if (DEBUG) Serial.println("🛑 Minden motor leállítva");
    } else {
      controlMotors(
        motorCommand & 0b0001,
        motorCommand & 0b0010,
        motorCommand & 0b0100,
        motorCommand & 0b1000
      );
      
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
    
    if (DEBUG) Serial.println("---");
  }
}