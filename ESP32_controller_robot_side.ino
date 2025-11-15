#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 15
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

// ======= ROBOT CÍMZÉS + TITKOSÍTÁS =======
#define MY_ROBOT_ID 0x169
#define SECRET_KEY 0b11011010

// Motorok
#define MOTOR_LEFT_FORWARD 32
#define MOTOR_LEFT_BACK    33
#define MOTOR_RIGHT_FORWARD 25
#define MOTOR_RIGHT_BACK    26

#define PWM_FREQ 1000
#define PWM_RES 8
#define PWM_CHANNEL_LF 0
#define PWM_CHANNEL_LB 1
#define PWM_CHANNEL_RF 2
#define PWM_CHANNEL_RB 3

byte motorSpeed = 150;

void setup() {
  Serial.begin(115200);

  ledcAttach(MOTOR_LEFT_FORWARD, PWM_CHANNEL_LF);
  ledcAttach(MOTOR_LEFT_BACK, PWM_CHANNEL_LB);
  ledcAttach(MOTOR_RIGHT_FORWARD, PWM_CHANNEL_RF);
  ledcAttach(MOTOR_RIGHT_BACK, PWM_CHANNEL_RB);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa OK");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize >= 2) {
    byte targetId = LoRa.read();
    if (targetId != MY_ROBOT_ID) {
      Serial.println("Not my ID, ignoring!");
      return;
    }

    byte encrypted = LoRa.read();
    byte data = encrypted ^ SECRET_KEY;  // VISSZAFEJTÉS

    Serial.print("Decoded command: ");
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
  ledcWrite(PWM_CHANNEL_LF, motorSpeed);
  ledcWrite(PWM_CHANNEL_LB, 0);
  ledcWrite(PWM_CHANNEL_RF, motorSpeed);
  ledcWrite(PWM_CHANNEL_RB, 0);
}

void moveBackward() {
  ledcWrite(PWM_CHANNEL_LF, 0);
  ledcWrite(PWM_CHANNEL_LB, motorSpeed);
  ledcWrite(PWM_CHANNEL_RF, 0);
  ledcWrite(PWM_CHANNEL_RB, motorSpeed);
}

void turnLeft() {
  ledcWrite(PWM_CHANNEL_LF, 0);
  ledcWrite(PWM_CHANNEL_LB, motorSpeed);
  ledcWrite(PWM_CHANNEL_RF, motorSpeed);
  ledcWrite(PWM_CHANNEL_RB, 0);
}

void turnRight() {
  ledcWrite(PWM_CHANNEL_LF, motorSpeed);
  ledcWrite(PWM_CHANNEL_LB, 0);
  ledcWrite(PWM_CHANNEL_RF, 0);
  ledcWrite(PWM_CHANNEL_RB, motorSpeed);
}

void stopMotors() {
  ledcWrite(PWM_CHANNEL_LF, 0);
  ledcWrite(PWM_CHANNEL_LB, 0);
  ledcWrite(PWM_CHANNEL_RF, 0);
  ledcWrite(PWM_CHANNEL_RB, 0);

}
