#ifndef SETTINGS_H
#define SETTINGS_H

// ================= CAMERA MODEL =================
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif

// ================= HOTSPOT =================
const char* AP_SSID = "We Are Engineers";
const char* AP_PASSWORD = "12341234";

// ================= LED SETTINGS =================
#define LED_PIN 4

// ================= DEFAULT VALUES =================
#define DEFAULT_BLINK_BAUD 300
#define DEFAULT_PAUSE_MS 500
#define DEFAULT_CAMERA_QUALITY 30
#define MAX_CODES 4

// ================= SERVER PORTS =================
#define STREAM_PORT 81
#define CODE_PORT 80
#define STREAM_CTRL_PORT 32769
#define CODE_CTRL_PORT 32768

// ================= PREFERENCES =================
#define CODES_NAMESPACE "codes"

// ================= HTTP BOUNDARY =================
#define PART_BOUNDARY "123456789000000000000987654321"

#endif