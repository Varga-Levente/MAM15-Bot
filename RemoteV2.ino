#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

// ===== ROBOT CÍMZÉS + TITKOSÍTÁS =====
#define ROBOT_ID 0x69
#define SECRET_KEY 0b11011010

// ===== GOMBOK =====
#define BTN_LEFT_FWD     25
#define BTN_LEFT_BACK    26
#define BTN_RIGHT_FWD    32
#define BTN_RIGHT_BACK   33
#define BTN_SPEED        27   // toggle request
#define BTN_UNUSED       13   // későbbi funkció

void setup() {
  Serial.begin(115200);

  pinMode(BTN_LEFT_FWD,   INPUT_PULLUP);
  pinMode(BTN_LEFT_BACK,  INPUT_PULLUP);
  pinMode(BTN_RIGHT_FWD,  INPUT_PULLUP);
  pinMode(BTN_RIGHT_BACK, INPUT_PULLUP);
  pinMode(BTN_SPEED,      INPUT_PULLUP);
  pinMode(BTN_UNUSED,     INPUT_PULLUP);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("🚀 Remote Ready!");
}

void loop() {
  byte cmd = 0;

  if (digitalRead(BTN_LEFT_FWD)   == LOW) cmd |= 0b00000001;
  if (digitalRead(BTN_LEFT_BACK)  == LOW) cmd |= 0b00000010;
  if (digitalRead(BTN_RIGHT_FWD)  == LOW) cmd |= 0b00000100;
  if (digitalRead(BTN_RIGHT_BACK) == LOW) cmd |= 0b00001000;

  if (digitalRead(BTN_SPEED) == LOW) cmd |= 0b00010000; // speed toggle request

  byte checksum = (ROBOT_ID + cmd) & 0xFF;

  byte encCmd = cmd ^ SECRET_KEY;
  byte encChk = checksum ^ SECRET_KEY;

  LoRa.beginPacket();
  LoRa.write(ROBOT_ID);
  LoRa.write(encCmd);
  LoRa.write(encChk);
  LoRa.endPacket();

  Serial.print("CMD RAW: ");
  Serial.print(cmd, BIN);
  Serial.print(" ENC: ");
  Serial.print(encCmd, BIN);
  Serial.print(" CHK: ");
  Serial.println(encChk, BIN);

  delay(50);
}
