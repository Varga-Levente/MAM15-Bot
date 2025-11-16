#include <SPI.h>
#include <LoRa.h>

// ---- CRC beállítások ----
uint16_t CRC_POLY = 0x1021;
uint16_t CRC_SEED = 0xFFFF;

// ---- Lora beállítások ----
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

#define ROBOT_ID 69

// ---- Gombok ----
#define BTN_FWD 32
#define BTN_BACK 33
#define BTN_LEFT 25
#define BTN_RIGHT 26
#define BTN_SPEED 27

bool lastSpeedBtn = false;
bool speedFlag = false;

// ==== CRC számoló ====
uint16_t calcCRC(uint8_t *data, uint8_t len) {
  uint16_t crc = CRC_SEED;

  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++)
      crc = (crc & 0x8000) ? (crc << 1) ^ CRC_POLY : (crc << 1);
  }

  return crc;
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_FWD, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SPEED, INPUT_PULLUP);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  LoRa.begin(LORA_BAND);
  Serial.println("TX Ready");
}

void loop() {
  byte cmd = 0;

  if (!digitalRead(BTN_FWD))  cmd |= 0b00000001;
  if (!digitalRead(BTN_BACK)) cmd |= 0b00000010;
  if (!digitalRead(BTN_RIGHT)) cmd |= 0b00000100;
  if (!digitalRead(BTN_LEFT))  cmd |= 0b00001000;

  bool speedBtn = !digitalRead(BTN_SPEED);

  if (speedBtn && !lastSpeedBtn)
    speedFlag = true;
  else
    speedFlag = false;

  lastSpeedBtn = speedBtn;

  uint8_t pkt[3];
  pkt[0] = ROBOT_ID;
  pkt[1] = cmd;
  pkt[2] = speedFlag;

  uint16_t crc = calcCRC(pkt, 3);

  LoRa.beginPacket();
  LoRa.write(pkt, 3);
  LoRa.write(crc >> 8);
  LoRa.write(crc & 0xFF);
  LoRa.endPacket();

  delay(60);
}
