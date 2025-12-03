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

void handleCodes(httpd_req_t *req){
  if(req->method == HTTP_GET){
    String html = codesPage();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
  }
  else if(req->method == HTTP_POST){
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
              Serial.println("✅ Új kód mentve: " + newCode);
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

void handleActivate(httpd_req_t *req){
  char body[32];
  int len = httpd_req_recv(req, body, sizeof(body)-1);

  if(len > 0){
    body[len] = 0;
    String arg = body;

    int idx = arg.indexOf("id=");
    if(idx >= 0){
      int id = arg.substring(idx+3).toInt();

      if(id >= 0 && id < MAX_CODES && codes[id].length() == 3){
        activeCode = id;
        shouldBlink = true;
        saveCodes();
        Serial.printf("➡ Aktív kód: %s (villogás indult)\n", codes[id].c_str());
      }
    }
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

void handleStop(httpd_req_t *req){
  shouldBlink = false;
  digitalWrite(LED_PIN, LOW);
  saveCodes();
  Serial.println("⏸ Villogás leállítva");

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

void handleStart(httpd_req_t *req){
  char body[32];
  int len = httpd_req_recv(req, body, sizeof(body)-1);

  if(len > 0){
    body[len] = 0;
    String arg = body;

    int idx = arg.indexOf("id=");
    if(idx >= 0){
      int id = arg.substring(idx+3).toInt();

      if(id >= 0 && id < MAX_CODES && codes[id].length() == 3 && activeCode == id){
        shouldBlink = true;
        saveCodes();
        Serial.printf("▶ Villogás indítva: %s\n", codes[id].c_str());
      }
    }
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

void handleDelete(httpd_req_t *req){
  char buf[16];
  int len = httpd_req_recv(req, buf, sizeof(buf)-1);
  if(len > 0){
    buf[len] = 0;
    String arg = buf;

    int idx = arg.indexOf("id=");
    if(idx >= 0){
      int id = arg.substring(idx+3).toInt();
      if(id >= 0 && id < MAX_CODES){
        codes[id] = "";
        if(activeCode == id) {
          activeCode = -1;
          shouldBlink = false;
        }
        saveCodes();
        Serial.printf("🗑 Törölve: index %d\n", id);
      }
    }
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

void handleSettings(httpd_req_t *req){
  char body[64];
  int len = httpd_req_recv(req, body, sizeof(body)-1);
  if(len > 0){
    body[len] = 0;
    String arg = body;

    int baudIdx = arg.indexOf("baud=");
    int pauseIdx = arg.indexOf("pause=");

    if(baudIdx >= 0){
      int ampPos = arg.indexOf('&', baudIdx);
      String baudStr = (ampPos > 0) ? arg.substring(baudIdx+5, ampPos) : arg.substring(baudIdx+5);
      BLINK_BAUD = baudStr.toInt();
      bitDelay = 1000000UL / BLINK_BAUD;
      Serial.printf("⚙ Baud rate frissítve: %d\n", BLINK_BAUD);
    }

    if(pauseIdx >= 0){
      String pauseStr = arg.substring(pauseIdx+6);
      PAUSE_BETWEEN_CODES = pauseStr.toInt();
      Serial.printf("⚙ Szünet frissítve: %d ms\n", PAUSE_BETWEEN_CODES);
    }

    saveCodes();
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

void handleCamera(httpd_req_t *req){
  char body[32];
  int len = httpd_req_recv(req, body, sizeof(body)-1);
  if(len > 0){
    body[len] = 0;
    String arg = body;

    int qualityIdx = arg.indexOf("quality=");
    if(qualityIdx >= 0){
      String qualityStr = arg.substring(qualityIdx+8);
      cameraQuality = qualityStr.toInt();
      
      sensor_t * s = esp_camera_sensor_get();
      if(s){
        s->set_quality(s, cameraQuality);
        Serial.printf("📷 Kamera minőség frissítve: %d\n", cameraQuality);
      }
      
      saveCodes();
    }
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/codes");
  httpd_resp_send(req, NULL, 0);
}

#endif