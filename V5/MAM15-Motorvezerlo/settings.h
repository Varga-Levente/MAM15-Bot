#ifndef SETTINGS_H
#define SETTINGS_H

// ===== DEBUG BEÁLLÍTÁSOK =====
struct DebugSettings {
  static const bool GLOBAL_DEBUG = true;
  static const bool LOG_SYSTEM = true;
  static const bool LOG_MOTOR = true;
  static const bool LOG_COMMUNICATION = true;
  static const bool LOG_LORA = true;
  static const bool LOG_ESPNOW = true;
  static const bool LOG_LANDING = true;
  static const bool LOG_FAILSAFE = true;
  static const bool LOG_HEALTH = true;
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

// ===== ROBOT BEÁLLÍTÁSOK =====
struct RobotSettings {
  static const int ROBOT_ID = 69;
};

// ===== ESP-NOW BEÁLLÍTÁSOK =====
struct ESPNowSettings {
  static constexpr uint8_t LANDOLO_MAC[6] = {0x1C, 0xDB, 0xD4, 0xD4, 0x0F, 0x80};
};

// ===== CRC ELLENŐRZÉS BEÁLLÍTÁSAI =====
struct CRCSettings {
  static const uint16_t POLYNOMIAL = 0x1021;
  static const uint16_t INITIAL_VALUE = 0xFFFF;
  static const uint16_t FINAL_XOR_VALUE = 0x0000;
};

// ===== MOTOR PIN DEFINÍCIÓK =====
struct MotorPins {
  static const int LEFT_FORWARD = 32;
  static const int LEFT_REVERSE = 27;
  static const int RIGHT_FORWARD = 25;
  static const int RIGHT_REVERSE = 26;
};

// ===== PWM BEÁLLÍTÁSOK =====
struct PWMSettings {
  static const int FREQUENCY = 1000;
  static const int RESOLUTION = 8;
};

// ===== SEBESSÉG SZINTEK =====
struct SpeedSettings {
  static constexpr int LEVELS[3] = {255, 120, 90};
  static const int DEFAULT_LEVEL = 0;
};

// ===== BIZTONSÁGI BEÁLLÍTÁSOK =====
struct SafetySettings {
  static const unsigned long FAILSAFE_TIMEOUT_MS = 300;
};

// ===== LoRa HEALTH MONITOR BEÁLLÍTÁSOK =====
struct HealthSettings {
  static const unsigned long CHECK_INTERVAL_MS = 5000;
  static const unsigned long RECONNECT_TIMEOUT_MS = 10000;
  static const int MAX_RESTART_COUNT = 3;
};

// ===== CSOMAG BEÁLLÍTÁSOK =====
struct PacketSettings {
  static const int EXPECTED_SIZE = 6;  // Robot ID + Motor + Speed + Landing + CRC(2)
  static const int DATA_SIZE = 4;      // CRC nélkül
};

#endif