#include <SPI.h>
#include <LoRa.h>
#include "CRC16.h"

// ===== LoRa Pin-ek =====
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

// ===== Robot azonosító =====
#define ROBOT_ID 69

// ===== CRC változó paraméterek =====
uint16_t CRC_POLY = 0x1021;
uint16_t CRC_SEED = 0xFFFF;

// ===== MOTOR PIN és LEDC csatorna =====
#define L_FWD 12
#define L_REV 13
#define R_FWD 27
#define R_REV 26

#define CH_L_FWD 0
#define CH_L_REV 1
#define CH_R_FWD 2
#define CH_R_REV 3

// ===== Sebesség szintek =====
int speedLevels[2] = {120, 255};
int currentSpeedIndex = 0;
bool lastSpeedButton = false;

// ===== Failsafe =====
unsigned long lastPacketTime = 0;
const unsigned long FAILSAFE_TIMEOUT = 300;  // ms

// ===== CRC objektum =====
CRC16 crc(CRC_POLY, CRC_SEED, 0, 0, true, true);

// =============================== SETUP =================================
void setup() {
  Serial.begin(115200);

  // LEDC PWM inicializálása
  ledcAttachPin(L_FWD, CH_L_FWD);
  ledcAttachPin(L_REV, CH_L_REV);
  ledcAttachPin(R_FWD, CH_R_FWD);
  ledcAttachPin(R_REV, CH_R_REV);

  ledcSetup(CH_L_FWD, 1000, 8);
  ledcSetup(CH_L_REV, 1000, 8);
  ledcSetup(CH_R_FWD, 1000, 8);
  ledcSetup(CH_R_REV, 1000, 8);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa Init ERROR!");
    while (1);
  }

  Serial.println("🚗 Robot READY - Waiting LoRa...");
}

// =============================== MOTOR VEZÉRLÉS ==============================
void drive(bool lf, bool lb, bool rf, bool rb) {
  int sp = speedLevels[currentSpeedIndex];

  ledcWrite(CH_L_FWD, lf ? sp : 0);
  ledcWrite(CH_L_REV, lb ? sp : 0);
  ledcWrite(CH_R_FWD, rf ? sp : 0);
  ledcWrite(CH_R_REV, rb ? sp : 0);
}

void stopMotors() {
  drive(0, 0, 0, 0);
}

// =============================== LOOP =================================
void loop() {

  // FAILSAFE STOP – ha nincs jel
  if (millis() - lastPacketTime > FAILSAFE_TIMEOUT) {
    stopMotors();
  }

  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;
  if (packetSize != 5) return;

  byte pkt[5];
  for (int i = 0; i < 5; i++) pkt[i] = LoRa.read();

  // CRC ellenőrzés
  uint16_t receivedCRC = (pkt[3] << 8) | pkt[4];

  crc.restart();
  crc.add(pkt, 3);
  uint16_t calcCRC = crc.getCRC();

  if (receivedCRC != calcCRC) {
    Serial.println("❌ BAD CRC!");
    return;
  }

  if (pkt[0] != ROBOT_ID) return; // Nem nekünk szól

  lastPacketTime = millis();  // FAILSAFE reset

  byte cmd   = pkt[1];
  bool spBtn = pkt[2];

  // ===== SEBESSÉG VÁLTÁS =====
  if (spBtn && !lastSpeedButton) {
    currentSpeedIndex = (currentSpeedIndex + 1) % 2;
    Serial.printf("⚡ SPEED CHANGED → %d\n", speedLevels[currentSpeedIndex]);
  }
  lastSpeedButton = spBtn;

  // ===== STOP ha nincs gombnyomás =====
  if (cmd == 0) {
    stopMotors();
    return;
  }

  // ===== MEGHajtás =====
  drive(
    cmd & 0b0001,   // Left forward
    cmd & 0b0010,   // Left backward
    cmd & 0b0100,   // Right forward
    cmd & 0b1000    // Right backward
  );
}
