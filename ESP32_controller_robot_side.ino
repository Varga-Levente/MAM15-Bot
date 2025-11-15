#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

// ===== ROBOT CÍMZÉS + DEKÓDOLÁS =====
#define MY_ROBOT_ID 0x169
#define SECRET_KEY 0b11011010

// ===== MOTOR PIN-EK =====
#define MOTOR_LEFT_FORWARD 32
#define MOTOR_LEFT_BACK    33
#define MOTOR_RIGHT_FORWARD 25
#define MOTOR_RIGHT_BACK    26

#define PWM_FREQ 1000
#define PWM_RES 8

byte motorSpeed = 150; // alap sebesség

void setup() {
  Serial.begin(115200);

  ledcAttach(MOTOR_LEFT_FORWARD, 0);
  ledcAttach(MOTOR_LEFT_BACK,    1);
  ledcAttach(MOTOR_RIGHT_FORWARD,2);
  ledcAttach(MOTOR_RIGHT_BACK,   3);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa FAILED!");
    while(1);
  }
  Serial.println("LoRa OK");
}

void loop() {
  if (LoRa.parsePacket() >= 2) {
    byte targetId = LoRa.read();
    if (targetId != MY_ROBOT_ID) return;

    byte encrypted = LoRa.read();
    byte data = encrypted ^ SECRET_KEY;

    Serial.print("CMD: ");
    Serial.println(data, BIN);

    if (data & 0b00010000) motorSpeed = 255;
    else if (data & 0b00100000) motorSpeed = 150;

    if (data & 0b00000001) moveForward();
    else if (data & 0b00000010) moveBackward();
    else if (data & 0b00000100) turnLeft();
    else if (data & 0b00001000) turnRight();
    else stopMotors();
  }
}

void moveForward() {
  ledcWrite(0, motorSpeed);  ledcWrite(1, 0);
  ledcWrite(2, motorSpeed);  ledcWrite(3, 0);
}

void moveBackward() {
  ledcWrite(0, 0);  ledcWrite(1, motorSpeed);
  ledcWrite(2, 0);  ledcWrite(3, motorSpeed);
}

void turnLeft() {
  ledcWrite(0, 0);  ledcWrite(1, motorSpeed);
  ledcWrite(2, motorSpeed);  ledcWrite(3, 0);
}

void turnRight() {
  ledcWrite(0, motorSpeed);  ledcWrite(1, 0);
  ledcWrite(2, 0);  ledcWrite(3, motorSpeed);
}

void stopMotors() {
  ledcWrite(0, 0); ledcWrite(1, 0);
  ledcWrite(2, 0); ledcWrite(3, 0);
}
