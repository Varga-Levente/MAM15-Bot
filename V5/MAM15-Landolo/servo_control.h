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
      Serial.print("🔒 Servók ZÁRVA (");
      Serial.print(SERVO_CLOSED_POSITION);
      Serial.println("°)");
    #endif
  }

  void open() {
    servo1.write(SERVO_OPEN_POSITION);
    servo2.write(SERVO_OPEN_POSITION);
    isOpen = true;
    
    #if DEBUG_ENABLED && DEBUG_SERVO
      Serial.print("🛬 Servók NYITVA (");
      Serial.print(SERVO_OPEN_POSITION);
      Serial.println("°)");
    #endif
  }

  void setToStartPosition() {
    // Első boot: mindig zárva
    close();
    log("✅ Servók alaphelyzetben (zárva)");
  }

  bool getIsOpen() const {
    return isOpen;
  }

  void printStatus() {
    #if DEBUG_ENABLED && DEBUG_SERVO
      Serial.print("🛬 Servók maradnak: ");
      Serial.println(isOpen ? "NYITVA" : "ZÁRVA");
    #endif
  }
};

#endif