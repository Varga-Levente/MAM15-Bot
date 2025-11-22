#include "esp_camera.h"
#include <WiFi.h>
#include <Preferences.h>
#include "esp_http_server.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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
const char* ap_ssid = "We Are Engineers";
const char* ap_password = "12341234";

// === LED BLINK SETTINGS ===
int LED_PIN = 13;          // ESP32-CAM beépített vakuvillogó LED (GPIO 4)
int BLINK_BAUD = 25;     // 300 (Ez kell versenyen) 10-15 (Szemmel is olvasható)
unsigned long bitDelay = 1000000UL / BLINK_BAUD;  // microseconds

// ================= CODES STORAGE =================
Preferences preferences;
const char* codes_ns = "codes";
const int max_codes = 4;
String codes[max_codes];
int activeCode = -1;

// ================= HTTP SERVER =================
httpd_handle_t stream_httpd = NULL;
httpd_handle_t code_httpd = NULL;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ================= UTILS =================
void loadCodes() {
  preferences.begin(codes_ns, false);
  for (int i = 0; i < max_codes; i++) {
    String key = "code" + String(i);  // <-- így kell
    codes[i] = preferences.getString(key.c_str(), "");
  }
  activeCode = preferences.getInt("active", -1);
  preferences.end();
}

void saveCodes() {
  preferences.begin(codes_ns, false);
  for (int i = 0; i < max_codes; i++) {
    String key = "code" + String(i);  // <-- így kell
    preferences.putString(key.c_str(), codes[i]);
  }
  preferences.putInt("active", activeCode);
  preferences.end();
}

void blinkCode(String code) {
  if (code.length() != 3) return;

  pinMode(LED_PIN, OUTPUT);

  for (int i = 0; i < code.length(); i++) {
    char c = code[i];

    // START BIT (LOW)
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(bitDelay);

    // 8 DATA BIT LSB FIRST
    for (int b = 0; b < 8; b++) {
      bool bit = (c >> b) & 1;
      digitalWrite(LED_PIN, bit ? HIGH : LOW);
      delayMicroseconds(bitDelay);
    }

    // STOP BIT (HIGH)
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(bitDelay);
  }

  // LED OFF
  digitalWrite(LED_PIN, LOW);
}

String codesPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Kódok kezelése</title></head><body>";
  html += "<h2>Kód hozzáadása (max 4 db)</h2>";
  html += "<form method='POST' action='/codes'>";
  html += "Kód (3 karakter): <input type='text' name='newcode' maxlength='3' minlength='3' required>";
  html += "<input type='submit' value='Mentés'></form>";

  html += "<h3>Mentett kódok</h3><ul style='list-style:none;padding-left:0;'>";

  for(int i=0; i < max_codes; i++) {
    if(codes[i].length() == 3) {
      html += "<li style='margin:4px 0;'><b>" + codes[i] + "</b>";

      if(activeCode == i){
        // Aktív kód jelzése és TESZT gomb
        html += " <span style='color:green;'>(Aktív)</span>";
        html += " <form style='display:inline' method='POST' action='/blink'><input type='submit' value='TESZT'></form>";
      } else {
        // Nem aktív kód: Aktiválás gomb
        html += " <form style='display:inline' method='POST' action='/activate'>";
        html += "  <input type='hidden' name='id' value='"+String(i)+"'>";
        html += "  <input type='submit' value='Aktivál'>";
        html += "</form>";
      }

      // Törlés gomb minden kódnál
      html += " <form style='display:inline' method='POST' action='/delete'>";
      html += "  <input type='hidden' name='id' value='"+String(i)+"'>";
      html += "  <input type='submit' value='Törlés'>";
      html += "</form>";

      html += "</li>";
    }
  }

  html += "</ul></body></html>";
  return html;
}

// ================= HTTP HANDLERS =================
static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char part_buf[64];

  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  while(true){
    fb = esp_camera_fb_get();
    if(!fb){
      Serial.println("Camera capture failed");
      continue;
    }

    if(fb->format != PIXFORMAT_JPEG){
      frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if(!_jpg_buf){
        Serial.println("JPEG conversion failed");
        continue;
      }
    } else {
      _jpg_buf = fb->buf;
      _jpg_buf_len = fb->len;
    }

    int hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, _jpg_buf_len);
    httpd_resp_send_chunk(req, part_buf, hlen);
    httpd_resp_send_chunk(req, (const char*)_jpg_buf, _jpg_buf_len);
    httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(req->handle == NULL) break; // client disconnected
  }
  return ESP_OK;
}

static void handleCodes(httpd_req_t *req){
  if(req->method == HTTP_GET){
    String html = codesPage();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
  }else if(req->method == HTTP_POST){
    char body[32];
    int len = httpd_req_recv(req, body, sizeof(body)-1);
    if(len > 0){
      body[len] = 0;
      String arg = body;

      int idx = arg.indexOf("newcode=");
      if(idx >= 0){
        String newCode = arg.substring(idx + 8);
        newCode.trim();

        if(newCode.length() == 3){
          bool exists = false;
          for(int i=0;i<max_codes;i++) if(codes[i] == newCode) exists = true;

          if(!exists){
            int freeIdx = -1;
            for(int i=0;i<max_codes;i++){
              if(codes[i].length() != 3){
                freeIdx = i;
                break;
              }
            }

            if(freeIdx >= 0){
              codes[freeIdx] = newCode;
              saveCodes();
              Serial.println("Új kód mentve: " + newCode);
            } else {
              Serial.println("⚠ Tele a kódtár");
            }
          }
        }
      }
    }

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/codes");
    httpd_resp_send(req, NULL, 0);
  }
}

static void handleActivate(httpd_req_t *req){
  char body[32];
  int len = httpd_req_recv(req, body, sizeof(body)-1);

  if(len > 0){
    body[len] = 0;
    String arg = body;

    int idx = arg.indexOf("id=");
    if(idx >= 0){
      int id = arg.substring(idx+3).toInt();

      if(id >= 0 && id < max_codes && codes[id].length() == 3){
        activeCode = id;

        preferences.begin("codeStore", false);
        preferences.putInt("active", activeCode);
        preferences.end();

        Serial.printf("➡ Aktív kód átállítva: %s (index: %d)\n", codes[id].c_str(), id);
      }
    }
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

static void handleDelete(httpd_req_t *req){
  char buf[16];
  int len = httpd_req_recv(req, buf, sizeof(buf)-1);
  if(len > 0){
    buf[len] = 0;
    String arg = buf;

    int idx = arg.indexOf("id=");
    if(idx >= 0){
      int id = arg.substring(idx+3).toInt();
      if(id >= 0 && id < max_codes){
        codes[id] = "";           // törlés
        if(activeCode == id) activeCode = -1;  // ha aktív volt, inaktiváljuk
        saveCodes();
        Serial.printf("Törölve: index %d\n", id);
      }
    }
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

static void handleBlink(httpd_req_t *req){
  if (activeCode >= 0 && activeCode < max_codes) {
    blinkCode(codes[activeCode]);
  }
  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

// ================= START SERVERS =================
void startCameraServer() {
    // -----------------------------
    // 1️⃣ Stream szerver 81-en
    // -----------------------------
    httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
    stream_config.server_port = 81;
    stream_config.ctrl_port = 32769; // külön ctrl port, hogy ne ütközzön
    if (httpd_start(&stream_httpd, &stream_config) == ESP_OK) {
        httpd_uri_t stream_uri = {
            .uri       = "/stream",
            .method    = HTTP_GET,
            .handler   = stream_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    } else {
        Serial.println("Stream server indítása sikertelen!");
    }

    // -----------------------------
    // 2️⃣ Kódkezelő szerver 80-on
    // -----------------------------
    httpd_config_t code_config = HTTPD_DEFAULT_CONFIG();
    code_config.server_port = 80;
    code_config.ctrl_port = 32768; // alap ctrl_port
    if (httpd_start(&code_httpd, &code_config) == ESP_OK) {

        // GET /codes
        httpd_uri_t codes_get_uri = {
            .uri      = "/codes",
            .method   = HTTP_GET,
            .handler  = [](httpd_req_t* req) -> esp_err_t {
                handleCodes(req);
                return ESP_OK;
            },
            .user_ctx = NULL
        };
        httpd_register_uri_handler(code_httpd, &codes_get_uri);

        // POST /codes
        httpd_uri_t codes_post_uri = {
            .uri      = "/codes",
            .method   = HTTP_POST,
            .handler  = [](httpd_req_t* req) -> esp_err_t {
                handleCodes(req);
                return ESP_OK;
            },
            .user_ctx = NULL
        };
        httpd_register_uri_handler(code_httpd, &codes_post_uri);

        // POST /activate
        httpd_uri_t activate_uri = {
            .uri      = "/activate",
            .method   = HTTP_POST,
            .handler  = [](httpd_req_t* req) -> esp_err_t {
                handleActivate(req);
                return ESP_OK;
            },
            .user_ctx = NULL
        };
        httpd_register_uri_handler(code_httpd, &activate_uri);

        // POST /delete
        httpd_uri_t delete_uri = {
            .uri      = "/delete",
            .method   = HTTP_POST,
            .handler  = [](httpd_req_t* req) -> esp_err_t {
                handleDelete(req);
                return ESP_OK;
            },
            .user_ctx = NULL
        };
        httpd_register_uri_handler(code_httpd, &delete_uri);

        httpd_uri_t blink_uri = {
          .uri      = "/blink",
          .method   = HTTP_POST,
          .handler  = [](httpd_req_t* req) -> esp_err_t {
              handleBlink(req);
              return ESP_OK;
          },
          .user_ctx = NULL
        };
        httpd_register_uri_handler(code_httpd, &blink_uri);

    } else {
        Serial.println("Code server indítása sikertelen!");
    }
}

// ================= SETUP =================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout
  Serial.begin(115200);

  // Camera init
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if(err != ESP_OK){
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  // Wi-Fi hotspot
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Hotspot IP: "); Serial.println(WiFi.softAPIP());

  // Load codes
  loadCodes();

  // Start servers
  startCameraServer();
}

void loop() {
  // nothing, HTTP server handles everything
}

