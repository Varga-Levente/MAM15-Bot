#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

// ======= ROBOT CÍMZÉS + TITKOSÍTÁS =======
#define ROBOT_ID 0x69
#define SECRET_KEY 0b11011010

// Irány gombok
#define BTN_FORWARD 25
#define BTN_BACK    26
#define BTN_LEFT    32
#define BTN_RIGHT   33

// Sebesség gombok
#define BTN_FAST    27
#define BTN_SLOW    13

void setup() {
  Serial.begin(115200);

  pinMode(BTN_FORWARD, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_FAST, INPUT_PULLUP);
  pinMode(BTN_SLOW, INPUT_PULLUP);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa OK");
}

void loop() {
  byte data = 0;

  if (digitalRead(BTN_FORWARD) == LOW) data |= 0b00000001;
  if (digitalRead(BTN_BACK)    == LOW) data |= 0b00000010;
  if (digitalRead(BTN_LEFT)    == LOW) data |= 0b00000100;
  if (digitalRead(BTN_RIGHT)   == LOW) data |= 0b00001000;

  if (digitalRead(BTN_FAST) == LOW) data |= 0b00010000;
  if (digitalRead(BTN_SLOW) == LOW) data |= 0b00100000;

  // Titkosítás
  byte encrypted = data ^ SECRET_KEY;

  LoRa.beginPacket();
  LoRa.write(ROBOT_ID);      // Cél robot
  LoRa.write(encrypted);     // Titkosított parancs
  LoRa.endPacket();

  Serial.print("Sent raw: ");
  Serial.print(data, BIN);
  Serial.print("  encrypted: ");
  Serial.println(encrypted, BIN);

  delay(50);

}


