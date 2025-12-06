#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "settings.h"

class MotorControl {
private:
  int speedLevels[3];
  int currentSpeedLevelIndex;
  bool previousSpeedButtonState;

  void log(const char* message) {
    #if DEBUG_ENABLED && DEBUG_MOTOR
      Serial.println(message);
    #endif
  }

public:
  MotorControl() : currentSpeedLevelIndex(0), previousSpeedButtonState(false) {
    speedLevels[0] = SPEED_LEVEL_1;
    speedLevels[1] = SPEED_LEVEL_2;
    speedLevels[2] = SPEED_LEVEL_3;
  }

  bool init() {
    bool pwmSetupSuccessful = true;
    
    if (!ledcAttach(LEFT_MOTOR_FORWARD_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
      #if DEBUG_ENABLED && DEBUG_MOTOR
        Serial.println("❌ Bal motor előre PWM inicializálás sikertelen!");
      #endif
      pwmSetupSuccessful = false;
    }
    
    if (!ledcAttach(LEFT_MOTOR_REVERSE_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
      #if DEBUG_ENABLED && DEBUG_MOTOR
        Serial.println("❌ Bal motor hátra PWM inicializálás sikertelen!");
      #endif
      pwmSetupSuccessful = false;
    }
    
    if (!ledcAttach(RIGHT_MOTOR_FORWARD_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
      #if DEBUG_ENABLED && DEBUG_MOTOR
        Serial.println("❌ Jobb motor előre PWM inicializálás sikertelen!");
      #endif
      pwmSetupSuccessful = false;
    }
    
    if (!ledcAttach(RIGHT_MOTOR_REVERSE_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
      #if DEBUG_ENABLED && DEBUG_MOTOR
        Serial.println("❌ Jobb motor hátra PWM inicializálás sikertelen!");
      #endif
      pwmSetupSuccessful = false;
    }
    
    if (pwmSetupSuccessful) {
      log("✅ PWM inicializálás sikeres");
    }
    
    return pwmSetupSuccessful;
  }

  void control(bool leftForward, bool leftBackward, bool rightForward, bool rightBackward) {
    int currentSpeed = speedLevels[currentSpeedLevelIndex];
    
    ledcWrite(LEFT_MOTOR_FORWARD_PIN, leftForward ? currentSpeed : 0);
    ledcWrite(LEFT_MOTOR_REVERSE_PIN, leftBackward ? currentSpeed : 0);
    ledcWrite(RIGHT_MOTOR_FORWARD_PIN, rightForward ? currentSpeed : 0);
    ledcWrite(RIGHT_MOTOR_REVERSE_PIN, rightBackward ? currentSpeed : 0);
  }

  void stop() {
    control(false, false, false, false);
    
    #if DEBUG_ENABLED && DEBUG_MOTOR
      Serial.println("🛑 Motorok leállítva");
    #endif
  }

  bool validateCommand(byte command) {
    bool leftConflict = (command & 0b0001) && (command & 0b0010);
    bool rightConflict = (command & 0b0100) && (command & 0b1000);
    
    if (leftConflict) {
      #if DEBUG_ENABLED && DEBUG_MOTOR
        Serial.println("❌ ÉRVÉNYTELEN: Bal motor egyszerre előre és hátra!");
      #endif
      return false;
    }
    
    if (rightConflict) {
      #if DEBUG_ENABLED && DEBUG_MOTOR
        Serial.println("❌ ÉRVÉNYTELEN: Jobb motor egyszerre előre és hátra!");
      #endif
      return false;
    }
    
    return true;
  }

  void handleSpeedButton(bool speedButtonPressed) {
    if (speedButtonPressed && !previousSpeedButtonState) {
      int previousSpeed = speedLevels[currentSpeedLevelIndex];
      currentSpeedLevelIndex = (currentSpeedLevelIndex + 1) % 3;
      
      #if DEBUG_ENABLED && DEBUG_SPEED
        Serial.print("⚡ Sebesség váltás: ");
        Serial.print(previousSpeed);
        Serial.print(" → ");
        Serial.println(speedLevels[currentSpeedLevelIndex]);
      #endif
    }
    previousSpeedButtonState = speedButtonPressed;
  }

  void executeCommand(byte motorCommand) {
    if (!validateCommand(motorCommand)) {
      stop();
      return;
    }
    
    if (motorCommand == 0) {
      stop();
    } else {
      control(
        motorCommand & 0b0001,
        motorCommand & 0b0010,
        motorCommand & 0b0100,
        motorCommand & 0b1000
      );
    }
  }
};

#endif