#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

class MotorController {
private:
  int currentSpeedLevelIndex;
  bool previousSpeedButtonState;

public:
  MotorController();
  
  bool init();
  void control(bool leftForward, bool leftBackward, bool rightForward, bool rightBackward);
  void stopAll();
  bool validateCommand(byte command);
  void handleSpeedChange(bool speedButtonPressed);
  int getCurrentSpeed();
  
private:
  bool setupPWM();
};

#endif