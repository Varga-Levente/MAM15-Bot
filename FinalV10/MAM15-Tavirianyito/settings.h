#ifndef SETTINGS_H
#define SETTINGS_H

// ===== DEBUG BEÁLLÍTÁSOK =====
struct DebugSettings {
  static const bool GLOBAL_DEBUG = true;      // Főkapcsoló
  static const bool LOG_SYSTEM = true;        // Rendszer üzenetek
  static const bool LOG_MOTOR = true;         // Motor parancsok
  static const bool LOG_COMMUNICATION = true; // LoRa üzenetek
  static const bool LOG_BUTTON = true;        // Gomb események
  static const bool LOG_LANDING = true;       // Landoló állapot
};

// ===== LoRa KOMMUNIKÁCIÓS BEÁLLÍTÁSOK =====
struct LoRaSettings {
  static const int SCK_PIN = 18;
  static const int MISO_PIN = 19;
  static const int MOSI_PIN = 23;
  static const int SS_PIN = 5;
  static const int RESET_PIN = 14;
  static const int DIO0_PIN = 2;
  static const long FREQUENCY = 433E6;
};

// ===== CÉL ROBOT BEÁLLÍTÁSOK =====
struct RobotSettings {
  static const int TARGET_ROBOT_ID = 69;
};

// ===== CRC ELLENŐRZÉS BEÁLLÍTÁSAI =====
struct CRCSettings {
  static const uint16_t POLYNOMIAL = 0x1021;
  static const uint16_t INITIAL_VALUE = 0xFFFF;
  static const uint16_t FINAL_XOR_VALUE = 0x0000;
};

// ===== GOMB PIN DEFINÍCIÓK =====
struct ButtonPins {
  static const int FORWARD = 32;
  static const int BACKWARD = 33;
  static const int RIGHT = 25;
  static const int LEFT = 26;
  static const int SPEED_CHANGE = 27;
  static const int LANDING = 13;
};

// ===== IDŐZÍTÉS BEÁLLÍTÁSOK =====
struct TimingSettings {
  static const int LOOP_DELAY_MS = 60;
};

// ===== CSOMAG BEÁLLÍTÁSOK =====
struct PacketSettings {
  static const int PACKET_SIZE = 4;  // Robot ID + Motor Command + Speed Flag + Landing Flag
  static const int CRC_SIZE = 2;
};

#endif