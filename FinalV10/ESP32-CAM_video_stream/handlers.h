#ifndef HANDLERS_H
#define HANDLERS_H

#include "esp_http_server.h"
#include "storage.h"
#include "camera.h"
#include "blink.h"
#include "settings.h"
#include <WiFi.h>

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

String codesPage();

esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char part_buf[64];
  static int failCount = 0;

  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  while(true){
      int sock = httpd_req_to_sockfd(req);
      if(sock < 0){
          Serial.println("Client disconnected.");
          break;
      }

      fb = esp_camera_fb_get();
      if(!fb){
          failCount++;
          Serial.println("Camera capture failed");
          delay(10);
          if(failCount > 3){
              Serial.println("⚠ Többszörös capture fail, újra inicializáljuk a kamerát...");
              reinitCamera();
              failCount = 0;
          }
          continue;
      }
      failCount = 0;

      if(fb->format != PIXFORMAT_JPEG){
          if(!frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len)){
              Serial.println("JPEG conversion failed");
              esp_camera_fb_return(fb);
              delay(10);
              continue;
          }
          esp_camera_fb_return(fb);
          fb = NULL;
      } else {
          _jpg_buf = fb->buf;
          _jpg_buf_len = fb->len;
      }

      int hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, _jpg_buf_len);
      if(httpd_resp_send_chunk(req, part_buf, hlen) != ESP_OK) break;
      if(httpd_resp_send_chunk(req, (const char*)_jpg_buf, _jpg_buf_len) != ESP_OK) break;
      if(httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)) != ESP_OK) break;

      if(fb){
          esp_camera_fb_return(fb);
          fb = NULL;
          _jpg_buf = NULL;
      } else if(_jpg_buf){
          free(_jpg_buf);
          _jpg_buf = NULL;
      }

      delay(2);
  }

  return ESP_OK;
}

// CONSOLIDATED: Handle all code operations (add/delete)
void handleCodes(httpd_req_t *req){
  if(req->method == HTTP_GET){
    String html = codesPage();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
  }
  else if(req->method == HTTP_POST){
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body)-1);
    if(len > 0){
      body[len] = 0;
      String arg = body;

      // Add new code
      int newIdx = arg.indexOf("newcode=");
      if(newIdx >= 0){
        String newCode = arg.substring(newIdx + 8);
        int ampPos = newCode.indexOf('&');
        if(ampPos > 0) newCode = newCode.substring(0, ampPos);
        newCode.trim();

        if(newCode.length() == 3){
          bool exists = false;
          for(int i=0;i<MAX_CODES;i++) if(codes[i] == newCode) exists = true;

          if(!exists){
            int freeIdx = -1;
            for(int i=0;i<MAX_CODES;i++){
              if(codes[i].length() != 3){
                freeIdx = i;
                break;
              }
            }

            if(freeIdx >= 0){
              codes[freeIdx] = newCode;
              saveCodes();
              addLog("✅ Új kód mentve: " + newCode);
            } else {
              addLog("⚠ Tele a kódtár");
            }
          }
        }
      }

      // Delete code
      int delIdx = arg.indexOf("delete=");
      if(delIdx >= 0){
        String idStr = arg.substring(delIdx + 7);
        int ampPos = idStr.indexOf('&');
        if(ampPos > 0) idStr = idStr.substring(0, ampPos);
        int id = idStr.toInt();
        
        if(id >= 0 && id < MAX_CODES){
          codes[id] = "";
          if(activeCode == id) {
            activeCode = -1;
            shouldBlink = false;
          }
          saveCodes();
          addLog("🗑 Törölve: index " + String(id));
        }
      }
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "OK", 2);
  }
}

// CONSOLIDATED: Handle all control operations (activate/deactivate/start/stop/reset)
void handleControl(httpd_req_t *req){
  char body[64];
  int len = httpd_req_recv(req, body, sizeof(body)-1);

  if(len > 0){
    body[len] = 0;
    String arg = body;

    // Parse action parameter
    int actionIdx = arg.indexOf("action=");
    if(actionIdx >= 0){
      String action = arg.substring(actionIdx + 7);
      int ampPos = action.indexOf('&');
      if(ampPos > 0) action = action.substring(0, ampPos);

      // Parse id parameter if exists
      int id = -1;
      int idIdx = arg.indexOf("id=");
      if(idIdx >= 0){
        String idStr = arg.substring(idIdx + 3);
        int ampPos2 = idStr.indexOf('&');
        if(ampPos2 > 0) idStr = idStr.substring(0, ampPos2);
        id = idStr.toInt();
      }

      // Execute action
      if(action == "activate"){
        if(id >= 0 && id < MAX_CODES && codes[id].length() == 3){
          activeCode = id;
          shouldBlink = true;
          saveCodes();
          addLog("➡ Aktív kód: " + codes[id] + " (villogás indult)");
        }
      }
      else if(action == "deactivate"){
        activeCode = -1;
        shouldBlink = false;
        digitalWrite(LED_PIN, LOW);
        saveCodes();
        addLog("❌ Aktív kód inaktiválva");
      }
      else if(action == "start"){
        if(id >= 0 && id < MAX_CODES && codes[id].length() == 3 && activeCode == id){
          shouldBlink = true;
          saveCodes();
          addLog("▶ Villogás indítva: " + codes[id]);
        }
      }
      else if(action == "stop"){
        shouldBlink = false;
        digitalWrite(LED_PIN, LOW);
        saveCodes();
        addLog("⏸ Villogás leállítva");
      }
      else if(action == "reset"){
        addLog("🔄 Motorvezérlő reset indítása...");
        digitalWrite(EXTERNAL_CONTROLLER_RESET_PIN, LOW);
        addLog("⬇️ Reset pin LOW");
        delay(RESTART_WAIT_TIME);
        digitalWrite(EXTERNAL_CONTROLLER_RESET_PIN, HIGH);
        addLog("⬆️ Reset pin HIGH");
        addLog("✅ Motorvezérlő resetelve");
      }
    }
  }

  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, "OK", 2);
}

// CONSOLIDATED: Handle all configuration (settings + camera + external trigger)
void handleConfig(httpd_req_t *req){
  char body[128];
  int len = httpd_req_recv(req, body, sizeof(body)-1);
  if(len > 0){
    body[len] = 0;
    String arg = body;

    // Baud rate
    int baudIdx = arg.indexOf("baud=");
    if(baudIdx >= 0){
      String baudStr = arg.substring(baudIdx + 5);
      int ampPos = baudStr.indexOf('&');
      if(ampPos > 0) baudStr = baudStr.substring(0, ampPos);
      BLINK_BAUD = baudStr.toInt();
      bitDelay = 1000000UL / BLINK_BAUD;
      addLog("⚙ Baud rate frissítve: " + String(BLINK_BAUD));
    }

    // Pause between codes
    int pauseIdx = arg.indexOf("pause=");
    if(pauseIdx >= 0){
      String pauseStr = arg.substring(pauseIdx + 6);
      int ampPos = pauseStr.indexOf('&');
      if(ampPos > 0) pauseStr = pauseStr.substring(0, ampPos);
      PAUSE_BETWEEN_CODES = pauseStr.toInt();
      addLog("⚙ Szünet frissítve: " + String(PAUSE_BETWEEN_CODES) + " ms");
    }

    // External trigger mode
    int extIdx = arg.indexOf("extmode=");
    if(extIdx >= 0){
      String extStr = arg.substring(extIdx + 8);
      int ampPos = extStr.indexOf('&');
      if(ampPos > 0) extStr = extStr.substring(0, ampPos);
      int mode = extStr.toInt();
      externalTriggerEnabled = (mode == 1);
      addLog("🔌 Külső trigger: " + String(externalTriggerEnabled ? "ENGEDÉLYEZVE" : "LETILTVA"));
    }

    // Camera quality
    int qualityIdx = arg.indexOf("quality=");
    if(qualityIdx >= 0){
      String qualityStr = arg.substring(qualityIdx + 8);
      int ampPos = qualityStr.indexOf('&');
      if(ampPos > 0) qualityStr = qualityStr.substring(0, ampPos);
      cameraQuality = qualityStr.toInt();
      
      sensor_t * s = esp_camera_sensor_get();
      if(s){
        s->set_quality(s, cameraQuality);
        addLog("📷 Kamera minőség frissítve: " + String(cameraQuality));
      }
    }

    saveCodes();
  }

  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, "OK", 2);
}

#endif