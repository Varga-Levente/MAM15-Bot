#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_http_server.h"
#include "handlers.h"
#include "storage.h"
#include "settings.h"
#include <WiFi.h>

httpd_handle_t stream_httpd = NULL;
httpd_handle_t code_httpd = NULL;

String codesPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Kódok kezelése</title>";
  html += "<style>body{font-family:Arial;margin:20px;} input[type=text],input[type=number]{padding:5px;margin:5px;} ";
  html += "input[type=submit]{padding:8px 15px;margin:5px;cursor:pointer;background:#4CAF50;color:white;border:none;border-radius:4px;} ";
  html += "input[type=submit]:hover{background:#45a049;} .section{border:1px solid #ddd;padding:15px;margin:10px 0;border-radius:5px;background:#f9f9f9;} ";
  html += ".active-indicator{color:green;font-weight:bold;} ";
  html += ".toggle-btn{padding:10px 20px;font-size:16px;font-weight:bold;border-radius:5px;border:2px solid;cursor:pointer;} ";
  html += ".toggle-on{background:#4CAF50;color:white;border-color:#388E3C;} ";
  html += ".toggle-off{background:#f44336;color:white;border-color:#d32f2f;}</style>";
  html += "</head><body>";
  
  html += "<h1>🎥 ESP32-CAM Kódvillogtató</h1>";
  
  // External Trigger Toggle Section
  html += "<div class='section'><h2>🔌 Külső Trigger (Pin 12)</h2>";
  html += "<p>Ha engedélyezve van, a villogást a 12-es pin állapota vezérli (HIGH = villog, LOW = nem villog).<br>";
  html += "Ha le van tiltva, a villogást a webes kezelőfelületről irányíthatod.</p>";
  html += "<form method='POST' action='/settings'>";
  html += "Külső trigger mód: <select name='extmode'>";
  html += "<option value='0'" + String(externalTriggerEnabled ? "" : " selected") + ">Webes vezérlés</option>";
  html += "<option value='1'" + String(externalTriggerEnabled ? " selected" : "") + ">Pin 12 vezérlés</option>";
  html += "</select> ";
  html += "<input type='submit' value='Alkalmazás'>";
  html += "</form>";
  
  // Show current pin state if external trigger is enabled
  if(externalTriggerEnabled) {
    html += "<p style='margin-top:10px;font-weight:bold;color:#2196F3;'>📍 A 12-es pin vezérli a villogást</p>";
  }
  
  html += "</div>";
  
  html += "<div class='section'><h2>📷 Kamera minőség</h2>";
  html += "<form method='POST' action='/camera'>";
  html += "JPEG minőség (0-63, alacsonyabb = jobb): <input type='number' name='quality' value='" + String(cameraQuality) + "' min='0' max='63'>";
  html += "<input type='submit' value='Alkalmazás'></form></div>";
  
  html += "<div class='section'><h2>⚡ Villogás beállítások</h2>";
  html += "<form method='POST' action='/settings'>";
  html += "Baud rate: <input type='number' name='baud' value='" + String(BLINK_BAUD) + "' min='10' max='1000'> Hz<br>";
  html += "Szünet két kód között: <input type='number' name='pause' value='" + String(PAUSE_BETWEEN_CODES) + "' min='100' max='5000'> ms<br>";
  html += "<input type='submit' value='Alkalmazás'></form></div>";
  
  html += "<div class='section'><h2>➕ Kód hozzáadása (max 4 db)</h2>";
  html += "<form method='POST' action='/codes'>";
  html += "Kód (3 karakter): <input type='text' name='newcode' maxlength='3' minlength='3' required>";
  html += "<input type='submit' value='Mentés'></form></div>";

  html += "<div class='section'><h3>💾 Mentett kódok</h3>";
  
  // Show info about control mode
  if(externalTriggerEnabled) {
    html += "<p style='background:#FFF3CD;padding:10px;border-radius:5px;border-left:4px solid #FFC107;'>";
    html += "⚠️ <strong>Külső trigger aktív:</strong> A villogást a 12-es pin vezérli. Az alábbi Start/Stop gombok nem működnek ebben a módban.";
    html += "</p>";
  }
  
  html += "<ul style='list-style:none;padding-left:0;'>";

  for(int i=0; i < MAX_CODES; i++) {
    if(codes[i].length() == 3) {
      html += "<li style='margin:10px 0;padding:10px;border:1px solid #ddd;border-radius:4px;background:white;display:flex;align-items:center;gap:10px;'>";
      html += "<b style='font-size:18px;min-width:50px;'>" + codes[i] + "</b>";

      if(activeCode == i && shouldBlink && !externalTriggerEnabled){
        html += "<span class='active-indicator' style='min-width:150px;'>● AKTÍV (villog)</span>";
        html += "<form style='display:inline;margin:0;' method='POST' action='/stop'>";
        html += "  <input type='submit' value='⸮ Stop' style='background:#ff9800;'>";
        html += "</form>";
      } else if(activeCode == i && !shouldBlink && !externalTriggerEnabled) {
        html += "<span style='color:#999;min-width:150px;'>○ AKTÍV (leállítva)</span>";
        html += "<form style='display:inline;margin:0;' method='POST' action='/start'>";
        html += "  <input type='hidden' name='id' value='"+String(i)+"'>";
        html += "  <input type='submit' value='▶ Start' style='background:#4CAF50;'>";
        html += "</form>";
      } else if(activeCode == i && externalTriggerEnabled) {
        html += "<span style='color:#2196F3;min-width:150px;'>🔌 AKTÍV (külső trigger)</span>";
      } else {
        html += "<span style='min-width:150px;'></span>";
        if(!externalTriggerEnabled) {
          html += "<form style='display:inline;margin:0;' method='POST' action='/activate'>";
          html += "  <input type='hidden' name='id' value='"+String(i)+"'>";
          html += "  <input type='submit' value='▶ Aktivál'>";
          html += "</form>";
        }
      }

      html += "<form style='display:inline;margin:0;margin-left:auto;' method='POST' action='/delete'>";
      html += "  <input type='hidden' name='id' value='"+String(i)+"'>";
      html += "  <input type='submit' value='🗑 Törlés' style='background:#f44336;'>";
      html += "</form>";

      html += "</li>";
    }
  }

  html += "</ul></div>";
  
  html += "<div style='margin-top:20px;padding:10px;background:#e3f2fd;border-radius:5px;'>";
  html += "<p><b>📡 Stream URL:</b> <a href='http://" + WiFi.softAPIP().toString() + ":81/stream' target='_blank'>";
  html += "http://" + WiFi.softAPIP().toString() + ":81/stream</a></p>";
  html += "</div>";
  
  html += "</body></html>";
  return html;
}

void startCameraServer() {
    httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
    stream_config.server_port = STREAM_PORT;
    stream_config.ctrl_port = STREAM_CTRL_PORT;
    
    if (httpd_start(&stream_httpd, &stream_config) == ESP_OK) {
        httpd_uri_t stream_uri = {
            .uri       = "/stream",
            .method    = HTTP_GET,
            .handler   = stream_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("📡 Stream szerver indítva (port 81)");
    }

    httpd_config_t code_config = HTTPD_DEFAULT_CONFIG();
    code_config.server_port = CODE_PORT;
    code_config.ctrl_port = CODE_CTRL_PORT;
    
    if (httpd_start(&code_httpd, &code_config) == ESP_OK) {
        httpd_uri_t codes_get = { .uri = "/codes", .method = HTTP_GET, 
            .handler = [](httpd_req_t* req) -> esp_err_t { handleCodes(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &codes_get);

        httpd_uri_t codes_post = { .uri = "/codes", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleCodes(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &codes_post);

        httpd_uri_t activate = { .uri = "/activate", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleActivate(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &activate);

        httpd_uri_t stop = { .uri = "/stop", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleStop(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &stop);

        httpd_uri_t start = { .uri = "/start", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleStart(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &start);

        httpd_uri_t del = { .uri = "/delete", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleDelete(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &del);

        httpd_uri_t sett = { .uri = "/settings", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleSettings(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &sett);

        httpd_uri_t cam = { .uri = "/camera", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleCamera(req); return ESP_OK; }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &cam);

        httpd_uri_t extrig = { .uri = "/toggletrigger", .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { return handleExternalTrigger(req); }, .user_ctx = NULL };
        httpd_register_uri_handler(code_httpd, &extrig);
        Serial.println("✅ /toggletrigger endpoint regisztrálva");

        Serial.println("🌐 Kód kezelő szerver indítva (port 80)");
    }
}

#endif