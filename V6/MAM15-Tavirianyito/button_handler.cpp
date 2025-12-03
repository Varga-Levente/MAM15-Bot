#include "button_handler.h"
#include "settings.h"

ButtonHandler::ButtonHandler() 
  : previousSpeedButtonState(false),
    speedChangeFlag(false),
    previousLandingButtonState(false),
    landingToggleFlag(false) {
}

void ButtonHandler::init() {
  pinMode(ButtonPins::FORWARD, INPUT_PULLUP);
  pinMode(ButtonPins::BACKWARD, INPUT_PULLUP);
  pinMode(ButtonPins::RIGHT, INPUT_PULLUP);
  pinMode(ButtonPins::LEFT, INPUT_PULLUP);
  pinMode(ButtonPins::SPEED_CHANGE, INPUT_PULLUP);
  pinMode(ButtonPins::LANDING, INPUT_PULLUP);

  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_BUTTON) {
    Serial.println("✅ Gombok inicializálva");
  }
}

byte ButtonHandler::readMotorCommands() {
  byte motorCommandByte = 0;

  if (!digitalRead(ButtonPins::FORWARD)) {
    motorCommandByte |= 0b00000001;
  }
  
  if (!digitalRead(ButtonPins::BACKWARD)) {
    motorCommandByte |= 0b00000010;
  }
  
  if (!digitalRead(ButtonPins::RIGHT)) {
    motorCommandByte |= 0b00000100;
  }
  
  if (!digitalRead(ButtonPins::LEFT)) {
    motorCommandByte |= 0b00001000;
  }

  handleSpeedButton();
  handleLandingButton();

  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_MOTOR && motorCommandByte != 0) {
    Serial.print("🎮 Motor parancs: 0b");
    Serial.println(motorCommandByte, BIN);
  }

  return motorCommandByte;
}

void ButtonHandler::handleSpeedButton() {
  bool currentSpeedButtonState = !digitalRead(ButtonPins::SPEED_CHANGE);
  
  if (currentSpeedButtonState && !previousSpeedButtonState) {
    speedChangeFlag = true;
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_BUTTON) {
      Serial.println("⚡ Sebesség váltás: AKTIVÁLVA");
    }
  } else {
    speedChangeFlag = false;
  }
  previousSpeedButtonState = currentSpeedButtonState;
}

void ButtonHandler::handleLandingButton() {
  bool currentLandingButtonState = !digitalRead(ButtonPins::LANDING);
  
  // Rising edge észlelés - csak lenyomáskor toggle
  if (currentLandingButtonState && !previousLandingButtonState) {
    landingToggleFlag = !landingToggleFlag;
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_LANDING) {
      Serial.print("🛬 Landoló toggle: ");
      Serial.println(landingToggleFlag ? "AKTIVÁLVA" : "DEAKTIVÁLVA");
    }
  }
  previousLandingButtonState = currentLandingButtonState;
}

bool ButtonHandler::getSpeedChangeFlag() {
  return speedChangeFlag;
}

bool ButtonHandler::getLandingToggleFlag() {
  return landingToggleFlag;
}