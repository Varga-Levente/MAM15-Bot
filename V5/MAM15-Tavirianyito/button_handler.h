#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

class ButtonHandler {
private:
  // Gomb állapot változók
  bool previousSpeedButtonState;
  bool speedChangeFlag;
  bool previousLandingButtonState;
  bool landingToggleFlag;

public:
  ButtonHandler();
  
  void init();
  byte readMotorCommands();
  bool getSpeedChangeFlag();
  bool getLandingToggleFlag();
  
private:
  void handleSpeedButton();
  void handleLandingButton();
};

#endif