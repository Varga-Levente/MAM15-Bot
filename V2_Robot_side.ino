#include <SPI.h>
#include <LoRa.h>

// ---- CRC beállítások ----
uint16_t CRC_SEED = 0x1D0F;
uint16_t CRC_POLY = 0x1021;

// ---- Motor PWM ----
#define L_FWD 12
#define L_REV 13
#define R_FWD 27
#define R_REV 26
#define PWM_FREQ 8000
#define PWM_RES 8

int speedLevels[2] = {120, 255};
int currentSpeedIndex = 0;
bool lastSpeedButton = false;

#define ROBOT_ID 69

// ---- Lora ----
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

// ==== CRC ====
uint16_t calcCRC(uint8_t *data, uint8_t len) {
  uint16_t crc = CRC_SEED;

  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++)
      crc = (crc & 0x8000) ? (crc << 1) ^ CRC_POLY : (crc << 1);
  }

  return crc;
}

// ==== Motor vezérlés ====
void drive(bool lf, bool lb, bool rf, bool rb) {
  int spd = speedLevels[currentSpeedIndex];

  ledcWrite(0, lf ? spd : 0);
  ledcWrite(1, lb ? spd : 0);
  ledcWrite(2, rf ? spd : 0);
  ledcWrite(3, rb ? spd : 0);
}

void setup() {
  Serial.begin(115200);

  // PWM csatornák
  ledcAttachPin(L_FWD, 0);
  ledcAttachPin(L_REV, 1);
  ledcAttachPin(R_FWD, 2);
  ledcAttachPin(R_REV, 3);
  for (int i = 0; i < 4; i++)
    ledcSetup(i, PWM_FREQ, PWM_RES);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  LoRa.begin(LORA_BAND);

  Serial.println("Robot ready");
}

void loop() {
  int size = LoRa.parsePacket();
  if (size != 5) return;

  uint8_t pkt[3];
  pkt[0] = LoRa.read();
  pkt[1] = LoRa.read();
  pkt[2] = LoRa.read();
  uint16_t rxCrc = (LoRa.read() << 8) | LoRa.read();

  if (pkt[0] != ROBOT_ID) return;

  uint16_t calc = calcCRC(pkt, 3);
  if (rxCrc != calc) {
    Serial.println("CRC FAIL!");
    return;
  }

  bool speedBtn = pkt[2];

  if (speedBtn && !lastSpeedButton) {
    currentSpeedIndex = (currentSpeedIndex + 1) % 2;
    Serial.printf("Speed now: %d\n", speedLevels[currentSpeedIndex]);
  }
  lastSpeedButton = speedBtn;

  drive(
    pkt[1] & 1,
    pkt[1] & 2,
    pkt[1] & 4,
    pkt[1] & 8
  );
}
