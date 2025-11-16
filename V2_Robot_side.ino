#include <SPI.h>
#include <LoRa.h>
#include "CRC16.h"

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

#define ROBOT_ID 69

// ===== MOTOR PINEK =====
#define L_FWD 12
#define L_REV 13
#define R_FWD 27
#define R_REV 26

// ===== Sebességek =====
int speedLevels[2] = {120, 255}; // lassú, gyors
int currentSpeedIndex = 0;

bool lastSpeedButton = false;

void setup() {
  Serial.begin(115200);

  ledcAttachPin(L_FWD, 0); // channel 0
  ledcAttachPin(L_REV, 1);
  ledcAttachPin(R_FWD, 2);
  ledcAttachPin(R_REV, 3);

  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcSetup(2, 1000, 8);
  ledcSetup(3, 1000, 8);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa error!");
    while (1);
  }

  Serial.println("Robot RX Ready");
}

void drive(int lf, int lb, int rf, int rb) {
  ledcWrite(0, lf ? speedLevels[currentSpeedIndex] : 0);
  ledcWrite(1, lb ? speedLevels[currentSpeedIndex] : 0);
  ledcWrite(2, rf ? speedLevels[currentSpeedIndex] : 0);
  ledcWrite(3, rb ? speedLevels[currentSpeedIndex] : 0);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  if (packetSize != 5) return; // 3 byte + 2 byte CRC

  byte pkt[5];
  for (int i = 0; i < 5; i++) pkt[i] = LoRa.read();

  uint16_t receivedCrc = (pkt[3] << 8) | pkt[4];
  uint16_t calcCrc = CRC16.ccitt(pkt, 3);

  if (receivedCrc != calcCrc) {
    Serial.println("CRC FAIL!");
    return;
  }

  if (pkt[0] != ROBOT_ID) return; // csak saját ID

  byte cmd = pkt[1];
  bool speedBtn = pkt[2];

  // sebesség váltás, csak a gomb lenyomásának éle
  if (speedBtn && !lastSpeedButton) {
    currentSpeedIndex = (currentSpeedIndex + 1) % 2;
    Serial.printf("Speed changed → %d\n", speedLevels[currentSpeedIndex]);
  }
  lastSpeedButton = speedBtn;

  drive(
    cmd & 0b00000001,
    cmd & 0b00000010,
    cmd & 0b00000100,
    cmd & 0b00001000
  );
}
