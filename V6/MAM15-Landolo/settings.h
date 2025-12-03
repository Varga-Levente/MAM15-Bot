#ifndef SETTINGS_H
#define SETTINGS_H

// ═════════════════════════════════════════════════════════
// HARDWARE PIN DEFINÍCIÓK
// ═════════════════════════════════════════════════════════
#define SERVO1_PIN 2
#define SERVO2_PIN 3
#define LED_PIN 8
#define RESET_BUTTON_PIN 9

// ═════════════════════════════════════════════════════════
// SERVO POZÍCIÓK (fokban)
// ═════════════════════════════════════════════════════════
#define SERVO_OPEN_POSITION 175    // Nyitott állapot
#define SERVO_CLOSED_POSITION 5    // Zárt állapot

// ═════════════════════════════════════════════════════════
// LED BEÁLLÍTÁSOK
// ═════════════════════════════════════════════════════════
#define LED_BLINK_DURATION 3000    // LED villogás időtartama (ms)
#define LED_BLINK_ON_TIME 100      // LED bekapcsolva (ms)
#define LED_BLINK_OFF_TIME 100     // LED kikapcsolva (ms)

// ═════════════════════════════════════════════════════════
// DEBUG BEÁLLÍTÁSOK - KOMPONENSENKÉNT
// ═════════════════════════════════════════════════════════
#define DEBUG_ENABLED true         // Fő debug kapcsoló

#define DEBUG_SERVO true           // Servo műveletek logolása
#define DEBUG_LED true             // LED műveletek logolása
#define DEBUG_COMM true            // Kommunikáció logolása
#define DEBUG_SLEEP true           // Sleep műveletek logolása
#define DEBUG_BOOT true            // Boot információk logolása

// ═════════════════════════════════════════════════════════
// EGYÉB BEÁLLÍTÁSOK
// ═════════════════════════════════════════════════════════
#define SERIAL_BAUD_RATE 115200
#define SERVO_INIT_DELAY 1000      // Delay servo reset után (ms)
#define SLEEP_ENTER_DELAY 500      // Delay deep sleep előtt (ms)
#define WIFI_DISCONNECT_DELAY 100  // Delay WiFi kikapcsolás után (ms)

#endif