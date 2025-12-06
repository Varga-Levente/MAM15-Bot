#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "settings.h"
#include "storage.h"

bool initCamera() {
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
  config.grab_mode = CAMERA_GRAB_LATEST;  // Hozzáadva: mindig a legfrissebb képet használja

  if(psramFound()){
    Serial.println("📦 PSRAM detected");
    config.frame_size = FRAMESIZE_QVGA;  // 320x240 - KISEBB FELBONTÁS
    config.jpeg_quality = cameraQuality;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;  // Hozzáadva: PSRAM használata
  } else {
    Serial.println("⚠ No PSRAM found");
    config.frame_size = FRAMESIZE_QVGA;  // 320x240 - KISEBB FELBONTÁS
    config.jpeg_quality = cameraQuality;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;  // Hozzáadva: DRAM használata
  }

  // PWDN pin reset - néha ez segít
  if(PWDN_GPIO_NUM != -1){
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, LOW);
    delay(10);
    digitalWrite(PWDN_GPIO_NUM, HIGH);
    delay(10);
  }

  // Első próbálkozás normál frekvenciával
  Serial.println("🔍 Kamera inicializálás (20MHz XCLK)...");
  esp_err_t err = esp_camera_init(&config);
  
  // Ha nem sikerült, próbáljuk alacsonyabb frekvenciával
  if(err != ESP_OK){
    Serial.printf("⚠ Első próba sikertelen (0x%x), újrapróbálkozás 10MHz-en...\n", err);
    esp_camera_deinit();
    delay(100);
    
    config.xclk_freq_hz = 10000000;  // 10MHz
    err = esp_camera_init(&config);
  }
  
  // Ha még mindig nem sikerült, próbáljuk még alacsonyabban
  if(err != ESP_OK){
    Serial.printf("⚠ Második próba sikertelen (0x%x), újrapróbálkozás 8MHz-en...\n", err);
    esp_camera_deinit();
    delay(100);
    
    config.xclk_freq_hz = 8000000;  // 8MHz
    err = esp_camera_init(&config);
  }

  if(err != ESP_OK){
    Serial.printf("❌ Kamera init hiba: 0x%x\n", err);
    Serial.println("\n=== HIBAKERESÉSI LÉPÉSEK ===");
    Serial.println("1. Ellenőrizd a kamera szalagkábel csatlakozását");
    Serial.println("2. Tisztítsd meg a kábel érintkezőit");
    Serial.println("3. Próbálj meg másik kamera modult");
    Serial.println("4. Ellenőrizd a tápellátást (min. 5V 2A)");
    Serial.println("============================\n");
    return false;
  }

  // Szenzor információk kiírása
  sensor_t * s = esp_camera_sensor_get();
  if(s){
    Serial.println("✅ Kamera inicializálva");
    Serial.printf("📷 Szenzor PID: 0x%02X\n", s->id.PID);
    Serial.printf("⚙️  XCLK frekvencia: %d Hz\n", config.xclk_freq_hz);
    Serial.printf("🖼️  Frame size: %d\n", config.frame_size);
    Serial.printf("💾 JPEG quality: %d\n", cameraQuality);
    
    // Optimális beállítások
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  }

  return true;
}

void reinitCamera() {
  Serial.println("🔄 Kamera újra inicializálása...");
  esp_camera_deinit();
  delay(200);
  
  if(!initCamera()){
    Serial.println("❌ Kamera újra inicializálás sikertelen!");
  }
}

#endif