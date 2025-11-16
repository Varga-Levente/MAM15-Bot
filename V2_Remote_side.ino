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

#define ROBOT_ID 1

#define BTN_LF  32
#define BTN_LB  33
#define BTN_RF  25
#define BTN_RB  26
#define BTN_SPEED 27

void setup() {
  pinMode(BTN_LF, INPUT_PULLUP);
  pinMode(BTN_LB, INPUT_PULLUP);
  pinMode(BTN_RF, INPUT_PULLUP);
  pinMode(BTN_RB, INPUT_PULLUP);
  pinMode(BTN_SPEED, INPUT_PULLUP);

  Serial.begin(115200);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa failed!");
    while (1);
  }
  Serial.println("TX Ready");
}

void loop() {
  byte cmd = 0;

  if (!digitalRead(BTN_LF)) cmd |= 0b00000001;
  if (!digitalRead(BTN_LB)) cmd |= 0b00000010;
  if (!digitalRead(BTN_RF)) cmd |= 0b00000100;
  if (!digitalRead(BTN_RB)) cmd |= 0b00001000;

  byte speedBtn = (!digitalRead(BTN_SPEED) ? 1 : 0);

  byte packet[3];
  packet[0] = ROBOT_ID;
  packet[1] = cmd;
  packet[2] = speedBtn;

  uint16_t crc = CRC16.ccitt(packet, 3);

  LoRa.beginPacket();
  LoRa.write(packet, 3);
  LoRa.write((crc >> 8) & 0xFF);
  LoRa.write(crc & 0xFF);
  LoRa.endPacket();

  delay(50);
}
