#ifndef SETTINGS_H
#define SETTINGS_H

// ═════════════════════════════════════════════════════════
// DEBUG BEÁLLÍTÁSOK - KOMPONENSENKÉNT
// ═════════════════════════════════════════════════════════
#define DEBUG_ENABLED true         // Fő debug kapcsoló

#define DEBUG_LORA true            // LoRa kommunikáció logolása
#define DEBUG_MOTOR true           // Motor műveletek logolása
#define DEBUG_ESPNOW true          // ESP-NOW kommunikáció logolása
#define DEBUG_CRC true             // CRC ellenőrzés logolása
#define DEBUG_LANDING true         // Landoló parancsok logolása
#define DEBUG_SPEED true           // Sebesség váltás logolása
#define DEBUG_FAILSAFE true        // Failsafe események logolása
#define DEBUG_HEALTH true          // Health check logolása

// ═════════════════════════════════════════════════════════
// ROBOT AZONOSÍTÓ
// ═════════════════════════════════════════════════════════
#define ROBOT_ID 69

// ═════════════════════════════════════════════════════════
// LORA KOMMUNIKÁCIÓS BEÁLLÍTÁSOK
// ═════════════════════════════════════════════════════════
#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RESET_PIN 14
#define LORA_DIO0_PIN 2
#define LORA_FREQUENCY 433E6

// ═════════════════════════════════════════════════════════
// LORA HEALTH MONITOR BEÁLLÍTÁSOK
// ═════════════════════════════════════════════════════════
#define LORA_HEALTH_CHECK_INTERVAL 5000     // Health check időköz (ms)
#define LORA_RECONNECT_TIMEOUT 10000        // Újracsatlakozási timeout (ms)
#define MAX_LORA_RESTARTS 3                 // Max újraindítási kísérletek

// ═════════════════════════════════════════════════════════
// ESP-NOW BEÁLLÍTÁSOK - LANDOLÓ MAC CÍME (1C:DB:D4:D5:D0:28)
// ═════════════════════════════════════════════════════════
#define LANDOLO_MAC_0 0x1C
#define LANDOLO_MAC_1 0xDB
#define LANDOLO_MAC_2 0xD4
#define LANDOLO_MAC_3 0xD5
#define LANDOLO_MAC_4 0xD0
#define LANDOLO_MAC_5 0x28

// ═════════════════════════════════════════════════════════
// CRC ELLENŐRZÉS BEÁLLÍTÁSAI
// ═════════════════════════════════════════════════════════
#define CRC_POLYNOMIAL 0x1021
#define CRC_INITIAL_VALUE 0xFFFF
#define CRC_FINAL_XOR_VALUE 0x0000

// ═════════════════════════════════════════════════════════
// MOTOR VEZÉRLŐ PIN DEFINÍCIÓK
// ═════════════════════════════════════════════════════════
#define LEFT_MOTOR_FORWARD_PIN 32
#define LEFT_MOTOR_REVERSE_PIN 27
#define RIGHT_MOTOR_FORWARD_PIN 25
#define RIGHT_MOTOR_REVERSE_PIN 26

// ═════════════════════════════════════════════════════════
// PWM BEÁLLÍTÁSOK
// ═════════════════════════════════════════════════════════
#define PWM_FREQUENCY 50
#define PWM_RESOLUTION 8

// ═════════════════════════════════════════════════════════
// SEBESSÉG SZINTEK (0-255)
// ═════════════════════════════════════════════════════════
#define SPEED_LEVEL_1 255          // Maximális sebesség
#define SPEED_LEVEL_2 120          // Közepes sebesség
#define SPEED_LEVEL_3 90           // Lassú sebesség

// ═════════════════════════════════════════════════════════
// BIZTONSÁGI BEÁLLÍTÁSOK (FAILSAFE)
// ═════════════════════════════════════════════════════════
#define FAILSAFE_TIMEOUT_MS 300    // Failsafe timeout (ms)

// ═════════════════════════════════════════════════════════
// EGYÉB BEÁLLÍTÁSOK
// ═════════════════════════════════════════════════════════
#define SERIAL_BAUD_RATE 115200
#define PACKET_SIZE 6              // LoRa csomag mérete


#endif
