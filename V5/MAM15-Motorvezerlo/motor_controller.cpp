#include "motor_controller.h"
#include "settings.h"

MotorController::MotorController() 
  : currentSpeedLevelIndex(SpeedSettings::DEFAULT_LEVEL),
    previousSpeedButtonState(false) {
}

bool MotorController::init() {
  return setupPWM();
}

bool MotorController::setupPWM() {
  bool success = true;
  
  if (!ledcAttach(MotorPins::LEFT_FORWARD, PWMSettings::FREQUENCY, PWMSettings::RESOLUTION)) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
      Serial.println("❌ Hiba: Bal motor előre PWM inicializálás sikertelen!");
    }
    success = false;
  }
  
  if (!ledcAttach(MotorPins::LEFT_REVERSE, PWMSettings::FREQUENCY, PWMSettings::RESOLUTION)) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
      Serial.println("❌ Hiba: Bal motor hátra PWM inicializálás sikertelen!");
    }
    success = false;
  }
  
  if (!ledcAttach(MotorPins::RIGHT_FORWARD, PWMSettings::FREQUENCY, PWMSettings::RESOLUTION)) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
      Serial.println("❌ Hiba: Jobb motor előre PWM inicializálás sikertelen!");
    }
    success = false;
  }
  
  if (!ledcAttach(MotorPins::RIGHT_REVERSE, PWMSettings::FREQUENCY, PWMSettings::RESOLUTION)) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
      Serial.println("❌ Hiba: Jobb motor hátra PWM inicializálás sikertelen!");
    }
    success = false;
  }
  
  if (success && DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
    Serial.println("✅ PWM inicializálás sikeres");
  }
  
  return success;
}

void MotorController::control(bool leftForward, bool leftBackward, bool rightForward, bool rightBackward) {
  int currentSpeed = SpeedSettings::LEVELS[currentSpeedLevelIndex];
  
  ledcWrite(MotorPins::LEFT_FORWARD, leftForward ? currentSpeed : 0);
  ledcWrite(MotorPins::LEFT_REVERSE, leftBackward ? currentSpeed : 0);
  ledcWrite(MotorPins::RIGHT_FORWARD, rightForward ? currentSpeed : 0);
  ledcWrite(MotorPins::RIGHT_REVERSE, rightBackward ? currentSpeed : 0);
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
    if (leftForward || leftBackward || rightForward || rightBackward) {
      Serial.printf("🚗 Motor: L(%s) R(%s) @ %d\n",
        leftForward ? "↑" : (leftBackward ? "↓" : "○"),
        rightForward ? "↑" : (rightBackward ? "↓" : "○"),
        currentSpeed);
    }
  }
}

void MotorController::stopAll() {
  control(false, false, false, false);
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
    Serial.println("🛑 Motorok leállítva");
  }
}

bool MotorController::validateCommand(byte command) {
  bool leftConflict = (command & 0b0001) && (command & 0b0010);
  bool rightConflict = (command & 0b0100) && (command & 0b1000);
  
  if (leftConflict) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
      Serial.println("❌ ÉRVÉNYTELEN: Bal motor egyszerre előre és hátra!");
    }
    return false;
  }
  
  if (rightConflict) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
      Serial.println("❌ ÉRVÉNYTELEN: Jobb motor egyszerre előre és hátra!");
    }
    return false;
  }
  
  return true;
}

void MotorController::handleSpeedChange(bool speedButtonPressed) {
  if (speedButtonPressed && !previousSpeedButtonState) {
    int oldIndex = currentSpeedLevelIndex;
    currentSpeedLevelIndex = (currentSpeedLevelIndex + 1) % 3;
    
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR) {
      Serial.printf("⚡ Sebesség váltás: %d → %d\n",
        SpeedSettings::LEVELS[oldIndex],
        SpeedSettings::LEVELS[currentSpeedLevelIndex]);
    }
  }
  previousSpeedButtonState = speedButtonPressed;
}

int MotorController::getCurrentSpeed() {
  return SpeedSettings::LEVELS[currentSpeedLevelIndex];
}