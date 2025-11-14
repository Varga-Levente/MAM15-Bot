#include <Arduino.h>

// ---- XBee UART ----
#define XBEE_RX 16
#define XBEE_TX 17

// ---- Motor sebességek (állítható!) ----
int motorSpeedSlow = 120;
int motorSpeedFast = 255;

// ---- Aktív sebesség mód ----
int currentSpeedMode = 0;

// ---- Irány ----
String currentDirection = "STOP";

// ---- Motor PWM kimenetek ----
#define MOTOR_L1 4
#define MOTOR_L2 5
#define MOTOR_R1 18
#define MOTOR_R2 19

void setMotors(int l, int r) {
  ledcWrite(0, abs(l));
  ledcWrite(1, abs(r));
}

void motorStop() {
  setMotors(0, 0);
}

void motorForward(int speed) {
  setMotors(speed, speed);
}

void motorBack(int speed) {
  setMotors(-speed, -speed);
}

void motorLeft(int speed) {
  setMotors(-speed, speed);
}

void motorRight(int speed) {
  setMotors(speed, -speed);
}

void applyDirection() {
  int speed = currentSpeedMode == 0 ? motorSpeedSlow : motorSpeedFast;

  if (currentDirection == "FWD") motorForward(speed);
  else if (currentDirection == "BACK") motorBack(speed);
  else if (currentDirection == "LEFT") motorLeft(speed);
  else if (currentDirection == "RIGHT") motorRight(speed);
  else motorStop();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, XBEE_RX, XBEE_TX);

  // PWM motor vezérlés
  ledcAttachPin(MOTOR_L1, 0);
  ledcAttachPin(MOTOR_L2, 1);
  ledcAttachPin(MOTOR_R1, 2);
  ledcAttachPin(MOTOR_R2, 3);

  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcSetup(2, 1000, 8);
  ledcSetup(3, 1000, 8);

  Serial.println("Receiver ready!");
}

void loop() {
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();

    Serial.println("[XBee RX] " + msg);

    if (msg.startsWith("DIR:")) {
      currentDirection = msg.substring(4);
      applyDirection();
    }

    if (msg.startsWith("SPEED:")) {
      currentSpeedMode = msg.substring(6).toInt();
      Serial.println("Speed mode changed: " + String(currentSpeedMode));
    }
  }
}