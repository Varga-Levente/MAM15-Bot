#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <ESP32Servo.h>
#include "settings.h"

class ServoControl {
private:
  Servo servo1;
  Servo servo2;
  bool isOpen;

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_SERVO
      Serial.println(message);
    #endif
  }

public:
  ServoControl() : isOpen(false) {}

  void init() {
    servo1.attach(SERVO1_PIN);
    servo2.attach(SERVO2_PIN);
    log("✅ Servók inicializálva");
  }

  void close() {
    servo1.write(SERVO_CLOSED_POSITION);
    servo2.write(SERVO_CLOSED_POSITION);
    isOpen = false;
    
    #if DEBUG_ENABLED && DEBUG_SERVO
      Serial.println("🔒 Servók ZÁRVA (" + String(SERVO_CLOSED_POSITION) + "°)");
    #endif
  }

  void open() {
    servo1.write(SERVO_OPEN_POSITION);
    servo2.write(SERVO_OPEN_POSITION);
    isOpen = true;
    
    #if DEBUG_ENABLED && DEBUG_SERVO
      Serial.println("🛬 Servók NYITVA (" + String(SERVO_OPEN_POSITION) + "°)");
    #endif
  }

  void setToBootPosition(int bootCount) {
    if (bootCount == 1) {
      close();
      log("✅ Servók alaphelyzetben (zárva)");
    } else {
      open();
      log("🔒 Servók LEZÁRVA (Reset után)");
    }
  }

  bool getIsOpen() const {
    return isOpen;
  }

  void printStatus() {
    #if DEBUG_ENABLED && DEBUG_SERVO
      Serial.println("🛬 Servók maradnak: " + String(isOpen ? "NYITVA" : "ZÁRVA"));
    #endif
  }
};

#endif