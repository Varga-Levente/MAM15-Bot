#include "esp_camera.h"
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

// SD pin definíciók (ESP32-CAM AI Thinker)
#define SD_CS 13
#define SPI_MOSI 15
#define SPI_MISO 2
#define SPI_SCK 14

// Kamera pin-ek (AI Thinker)
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

// HTTP server
#include "esp_http_server.h"
httpd_handle_t stream_httpd = NULL;

// Kamera konfigurációs struktúra
struct CamConfig {
  String ssid;
  String password;
  int frame_size;
  int jpeg_quality;
};

CamConfig camConfig;

// ------------------ CONFIG BETÖLTÉS ------------------
bool loadConfigFromSD(){
    Serial.println("SD inicializálása...");
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    delay(100);
    if(!SD.begin(SD_CS)){
        Serial.println("SD init sikertelen!");
        return false;
    }

    if(!SD.exists("/config.cfg")){
        Serial.println("config.cfg nincs jelen!");
        return false;
    }

    File cfgFile = SD.open("/config.cfg");
    if(!cfgFile){
        Serial.println("config.cfg megnyitás sikertelen!");
        return false;
    }

    size_t size = cfgFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    cfgFile.readBytes(buf.get(), size);
    cfgFile.close();

    DynamicJsonDocument doc(1024);
    auto error = deserializeJson(doc, buf.get());
    if(error){
        Serial.println("JSON parsing sikertelen!");
        return false;
    }

    camConfig.ssid = doc["wifi"]["ssid"] | "VLevente";
    camConfig.password = doc["wifi"]["password"] | "12341234";
    String frame_size_str = doc["camera"]["frame_size"] | "VGA";
    camConfig.jpeg_quality = doc["camera"]["jpeg_quality"] | 30;

    if(frame_size_str == "QVGA") camConfig.frame_size = FRAMESIZE_QVGA;
    else if(frame_size_str == "VGA") camConfig.frame_size = FRAMESIZE_VGA;
    else if(frame_size_str == "SVGA") camConfig.frame_size = FRAMESIZE_SVGA;
    else camConfig.frame_size = FRAMESIZE_VGA;

    Serial.println("Konfiguráció betöltve:");
    Serial.println("SSID: " + camConfig.ssid);
    Serial.println("Frame size: " + frame_size_str);
    Serial.println("JPEG quality: " + String(camConfig.jpeg_quality));

    return true;
}

// ------------------ STREAM HANDLER ------------------
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) { res = ESP_FAIL; break; }
        else{
            if(fb->format != PIXFORMAT_JPEG){
                bool jpeg_converted = frame2jpg(fb, camConfig.jpeg_quality, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if(!jpeg_converted){ res = ESP_FAIL; break; }
            } else{
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }

        size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
        res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        if(res != ESP_OK) break;
        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        if(res != ESP_OK) break;
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if(res != ESP_OK) break;

        if(fb) esp_camera_fb_return(fb);
        if(_jpg_buf && fb==NULL){ free(_jpg_buf); _jpg_buf=NULL; }
    }
    return res;
}

// ------------------ CHECK HANDLER ------------------
static esp_err_t check_handler(httpd_req_t *req){
    const char* resp_str = "OK";
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, resp_str, strlen(resp_str));
}

// ------------------ CAMERA SERVER ------------------
void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t stream_uri = { .uri="/stream", .method=HTTP_GET, .handler=stream_handler, .user_ctx=NULL };
    httpd_uri_t check_uri = { .uri="/check", .method=HTTP_GET, .handler=check_handler, .user_ctx=NULL };

    if(httpd_start(&stream_httpd, &config) == ESP_OK){
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        httpd_register_uri_handler(stream_httpd, &check_uri);
    }
}

// ------------------ SETUP ------------------
void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-CAM indítás...");

    if(!loadConfigFromSD()){
        Serial.println("Alapértelmezett konfigurációt használunk.");
        camConfig.ssid = "VLevente";
        camConfig.password = "12341234";
        camConfig.frame_size = FRAMESIZE_VGA;
        camConfig.jpeg_quality = 30;
    }

    // Kamera konfiguráció
    camera_config_t cam_config;
    cam_config.ledc_channel = LEDC_CHANNEL_0;
    cam_config.ledc_timer = LEDC_TIMER_0;
    cam_config.pin_d0 = Y2_GPIO_NUM;
    cam_config.pin_d1 = Y3_GPIO_NUM;
    cam_config.pin_d2 = Y4_GPIO_NUM;
    cam_config.pin_d3 = Y5_GPIO_NUM;
    cam_config.pin_d4 = Y6_GPIO_NUM;
    cam_config.pin_d5 = Y7_GPIO_NUM;
    cam_config.pin_d6 = Y8_GPIO_NUM;
    cam_config.pin_d7 = Y9_GPIO_NUM;
    cam_config.pin_xclk = XCLK_GPIO_NUM;
    cam_config.pin_pclk = PCLK_GPIO_NUM;
    cam_config.pin_vsync = VSYNC_GPIO_NUM;
    cam_config.pin_href = HREF_GPIO_NUM;
    cam_config.pin_sscb_sda = SIOD_GPIO_NUM;
    cam_config.pin_sscb_scl = SIOC_GPIO_NUM;
    cam_config.pin_pwdn = PWDN_GPIO_NUM;
    cam_config.pin_reset = RESET_GPIO_NUM;
    cam_config.xclk_freq_hz = 20000000;
    cam_config.pixel_format = PIXFORMAT_JPEG;
    cam_config.frame_size = camConfig.frame_size;
    cam_config.jpeg_quality = camConfig.jpeg_quality;
    cam_config.fb_count = psramFound() ? 2 : 1;

    if(esp_camera_init(&cam_config) != ESP_OK){
        Serial.println("Kamera inicializálás sikertelen!");
        return;
    }
    Serial.println("Kamera inicializálva.");

    // WiFi
    Serial.println("WiFi csatlakozás: " + camConfig.ssid);
    WiFi.begin(camConfig.ssid.c_str(), camConfig.password.c_str());
    int timeout = 0;
    while(WiFi.status() != WL_CONNECTED && timeout++ < 20){
        delay(500);
        Serial.print(".");
    }
    if(WiFi.status() == WL_CONNECTED){
        Serial.println("\nWiFi csatlakoztatva: " + WiFi.localIP().toString());
        startCameraServer();
        Serial.println("Server fut. Stream: http://" + WiFi.localIP().toString() + "/stream");
        Serial.println("Health check: http://" + WiFi.localIP().toString() + "/check");
    } else {
        Serial.println("\nWiFi csatlakozás sikertelen!");
    }
}

void loop() {
    delay(1000);
}