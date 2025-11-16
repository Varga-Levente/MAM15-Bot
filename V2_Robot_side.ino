#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define LORA_BAND 433E6

#define MY_ROBOT_ID 0x69
#define SECRET_KEY 0b11011010

#define MOTOR_LEFT_FORWARD   32
#define MOTOR_LEFT_BACK      33
#define MOTOR_RIGHT_FORWARD  25
#define MOTOR_RIGHT_BACK     26

#define PWM_FREQ 1000
#define PWM_RES  8

// ====== 3 ÁLLÍTHATÓ SEBESSÉG ======
byte speeds[3] = {120, 180, 255};
byte speedIndex = 0;
byte motorSpeed = speeds[0];

// ====== ÁLLAPOTVÁLTOZÓK ======
bool lastSpeedButton = false;

// ====== CHECKSUM ======
byte calcChecksum(byte id, byte cmd) {
  return (id + cmd) & 0xFF;     // 🔄 BÁRMI MÁSRA CSERÉLHETŐ
}

void setup() {
  Serial.begin(115200);

  ledcAttach(MOTOR_LEFT_FORWARD,   PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_LEFT_BACK,      PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_RIGHT_FORWARD,  PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_RIGHT_BACK,     PWM_FREQ, PWM_RES);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  LoRa.begin(LORA_BAND);
  Serial.println("🤖 ROBOT READY");
}

void loop() {
  if (LoRa.parsePacket() != 3) return;

  byte id     = LoRa.read();
  byte encCmd = LoRa.read();
  byte encChk = LoRa.read();

  if (id != MY_ROBOT_ID) return;

  byte cmd = encCmd ^ SECRET_KEY;
  byte chk = encChk ^ SECRET_KEY;

  if (chk != calcChecksum(MY_ROBOT_ID, cmd)) {
    Serial.println("❌ BAD CHECKSUM - IGNORED");
    return;
  }

  // ======== SPEED TOGGLE ========
  bool speedBtn = cmd & 0b00010000;

  if (speedBtn && !lastSpeedButton) {
    speedIndex = (speedIndex + 1) % 3;
    motorSpeed = speeds[speedIndex];

    Serial.print("⚡ SPEED → ");
    Serial.println(motorSpeed);
  }
  lastSpeedButton = speedBtn;

  // ===== MOVE =====
  drive(
    cmd & 0b00000001,  // LF
    cmd & 0b00000010,  // LB
    cmd & 0b00000100,  // RF
    cmd & 0b00001000   // RB
  );
}

void drive(bool LF, bool LB, bool RF, bool RB) {
  ledcWrite(0, LF ? motorSpeed : 0);
  ledcWrite(1, LB ? motorSpeed : 0);
  ledcWrite(2, RF ? motorSpeed : 0);
  ledcWrite(3, RB ? motorSpeed : 0);
}
