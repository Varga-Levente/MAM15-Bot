#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

// === ROBOT CÍMZÉS + DEKÓDOLÁS ===
#define MY_ROBOT_ID 0x69
#define SECRET_KEY 0b11011010

// ===== MOTOR PINEK =====
#define MOTOR_LEFT_FORWARD   32
#define MOTOR_LEFT_BACK      33
#define MOTOR_RIGHT_FORWARD  25
#define MOTOR_RIGHT_BACK     26

#define PWM_FREQ 1000
#define PWM_RES  8

byte lastCommand = 0;
bool lastSpeedButton = false;
byte motorSpeed = 150; // default low mode

void setup() {
  Serial.begin(115200);

  ledcAttach(MOTOR_LEFT_FORWARD,   PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_LEFT_BACK,      PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_RIGHT_FORWARD,  PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_RIGHT_BACK,     PWM_FREQ, PWM_RES);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa FAILED!");
    while (1);
  }

  Serial.println("🤖 Robot READY!");
}

void loop() {
  if (LoRa.parsePacket() != 3) return;

  byte id     = LoRa.read();
  byte encCmd = LoRa.read();
  byte encChk = LoRa.read();

  if (id != MY_ROBOT_ID) return;

  byte cmd = encCmd ^ SECRET_KEY;
  byte chk = encChk ^ SECRET_KEY;

  if (chk != ((MY_ROBOT_ID + cmd) & 0xFF)) {
    Serial.println("❌ BAD CHECKSUM - IGNORED");
    return;
  }

  // ===== SPEED TOGGLE =====
  bool speedBtn = cmd & 0b00010000;
  if (speedBtn && !lastSpeedButton) {
    motorSpeed = (motorSpeed == 150 ? 255 : 150);
    Serial.print("⚡ SPEED SWITCH → ");
    Serial.println(motorSpeed);
  }
  lastSpeedButton = speedBtn;

  // ===== MOTOR VEZÉRLÉS =====
  executeMotor(
    cmd & 0b00000001,  // left forward
    cmd & 0b00000010,  // left back
    cmd & 0b00000100,  // right forward
    cmd & 0b00001000   // right back
  );
}

void executeMotor(bool LF, bool LB, bool RF, bool RB) {
  ledcWrite(0, LF ? motorSpeed : 0);
  ledcWrite(1, LB ? motorSpeed : 0);
  ledcWrite(2, RF ? motorSpeed : 0);
  ledcWrite(3, RB ? motorSpeed : 0);
}
