#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_BAND 433E6

// Motorok PWM csatornák (ESP32 LEDC)
#define MOTOR_LEFT_FORWARD 32
#define MOTOR_LEFT_BACK    33
#define MOTOR_RIGHT_FORWARD 25
#define MOTOR_RIGHT_BACK    26

// PWM csatorna és frekvencia
#define PWM_FREQ 1000
#define PWM_RES 8 // 0-255
#define PWM_CHANNEL_LF 0
#define PWM_CHANNEL_LB 1
#define PWM_CHANNEL_RF 2
#define PWM_CHANNEL_RB 3

// Aktuális sebesség
byte motorSpeed = 150; // alap lassú (0-255)

void setup() {
  Serial.begin(115200);

  // Motor kimenetek
  pinMode(MOTOR_LEFT_FORWARD, OUTPUT);
  pinMode(MOTOR_LEFT_BACK, OUTPUT);
  pinMode(MOTOR_RIGHT_FORWARD, OUTPUT);
  pinMode(MOTOR_RIGHT_BACK, OUTPUT);

  // PWM beállítás
  ledcSetup(PWM_CHANNEL_LF, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_LB, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_RF, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_RB, PWM_FREQ, PWM_RES);

  ledcAttachPin(MOTOR_LEFT_FORWARD, PWM_CHANNEL_LF);
  ledcAttachPin(MOTOR_LEFT_BACK, PWM_CHANNEL_LB);
  ledcAttachPin(MOTOR_RIGHT_FORWARD, PWM_CHANNEL_RF);
  ledcAttachPin(MOTOR_RIGHT_BACK, PWM_CHANNEL_RB);

  // LoRa inicializálás
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa init OK!");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    byte data = LoRa.read();
    Serial.print("Received: ");
    Serial.println(data, BIN);

    // Sebesség frissítés
    if (data & 0b00010000) motorSpeed = 255; // gyors
    else if (data & 0b00100000) motorSpeed = 150; // lassú

    // Irány
    if (data & 0b00000001) moveForward();
    else if (data & 0b00000010) moveBackward();
    else if (data & 0b00000100) turnLeft();
    else if (data & 0b00001000) turnRight();
    else stopMotors();
  }
}

// =====================
// Motor vezérlő függvények PWM-mel
// =====================
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