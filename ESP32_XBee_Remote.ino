#include <Arduino.h>

// ---- XBee UART ----
#define XBEE_RX 16
#define XBEE_TX 17

// ---- Irány gombok ----
#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_LEFT   25
#define BTN_RIGHT  26

// ---- Sebesség mód gombok ----
#define BTN_SLOW   27
#define BTN_FAST   14

// ---- Küldendő adatok ----
int speedMode = 0; // 0 = lassú, 1 = gyors

void sendCommand(const String &cmd) {
  Serial2.println(cmd);
  Serial.println("[XBee SEND] " + cmd);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, XBEE_RX, XBEE_TX);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  pinMode(BTN_SLOW, INPUT_PULLUP);
  pinMode(BTN_FAST, INPUT_PULLUP);

  Serial.println("Transmitter ready!");
}

void loop() {

  // ---- Sebesség mód gombok kezelése ----
  if (!digitalRead(BTN_SLOW)) {
    speedMode = 0;
    sendCommand("SPEED:0");
    delay(300);
  }
  if (!digitalRead(BTN_FAST)) {
    speedMode = 1;
    sendCommand("SPEED:1");
    delay(300);
  }

  // ---- Irányok küldése ----
  if (!digitalRead(BTN_UP)) {
    sendCommand("DIR:FWD");
  }
  else if (!digitalRead(BTN_DOWN)) {
    sendCommand("DIR:BACK");
  }
  else if (!digitalRead(BTN_LEFT)) {
    sendCommand("DIR:LEFT");
  }
  else if (!digitalRead(BTN_RIGHT)) {
    sendCommand("DIR:RIGHT");
  }
  else {
    // Ha semmit nem nyom, STOP legyen
    sendCommand("DIR:STOP");
  }

  delay(80);
}