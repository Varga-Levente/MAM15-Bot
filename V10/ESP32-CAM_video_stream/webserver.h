#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_http_server.h"
#include "handlers.h"
#include "storage.h"
#include "settings.h"
#include <WiFi.h>

httpd_handle_t stream_httpd = NULL;
httpd_handle_t code_httpd = NULL;

// Log handler - returns log HTML
esp_err_t log_handler(httpd_req_t *req) {
  String html = getLogHTML();
  
  httpd_resp_set_type(req, "text/html; charset=UTF-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
  httpd_resp_send(req, html.c_str(), html.length());
  return ESP_OK;
}

// Status handler - returns current system status as JSON
esp_err_t status_handler(httpd_req_t *req) {
  String json = "{";
  json += "\"externalTriggerEnabled\":" + String(externalTriggerEnabled ? "true" : "false") + ",";
  json += "\"cameraQuality\":" + String(cameraQuality) + ",";
  json += "\"blinkBaud\":" + String(BLINK_BAUD) + ",";
  json += "\"pauseBetweenCodes\":" + String(PAUSE_BETWEEN_CODES) + ",";
  json += "\"activeCode\":" + String(activeCode) + ",";
  json += "\"shouldBlink\":" + String(shouldBlink ? "true" : "false") + ",";
  json += "\"codes\":[";
  for(int i = 0; i < MAX_CODES; i++) {
    if(i > 0) json += ",";
    json += "\"" + codes[i] + "\"";
  }
  json += "]}";
  
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, json.c_str(), json.length());
  return ESP_OK;
}

String codesPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32-CAM Control</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: Arial, sans-serif; background: #1a1a1a; color: #fff; overflow: hidden; }";
  html += ".container { display: flex; height: 100vh; }";
  html += ".left-panel { width: 25%; background: #2d2d2d; padding: 20px; overflow-y: auto; border-right: 2px solid #444; }";
  html += ".center-panel { width: 50%; background: #000; display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 20px; position: relative; }";
  html += ".right-panel { width: 25%; background: #1a1a1a; padding: 20px; overflow: hidden; border-left: 2px solid #444; display: flex; flex-direction: column; }";
  html += "h1 { font-size: 20px; margin-bottom: 15px; color: #4CAF50; }";
  html += "h2 { font-size: 16px; margin-top: 15px; margin-bottom: 10px; color: #2196F3; }";
  html += "h3 { font-size: 14px; margin-bottom: 10px; color: #FF9800; }";
  html += ".section { margin-bottom: 20px; padding: 15px; background: #3d3d3d; border-radius: 8px; border-left: 4px solid #4CAF50; }";
  html += "label { display: block; color: #aaa; font-size: 12px; margin-top: 10px; margin-bottom: 3px; font-weight: bold; }";
  html += "input[type=text], input[type=number], select { width: 100%; padding: 8px; margin: 0 0 5px 0; background: #2d2d2d; color: #fff; border: 1px solid #555; border-radius: 4px; }";
  html += "button, input[type=submit], input[type=button] { width: 100%; padding: 10px; margin: 10px 0 0 0; cursor: pointer; background: #4CAF50; color: white; border: none; border-radius: 4px; font-weight: bold; transition: background 0.3s; }";
  html += "button:hover, input[type=submit]:hover, input[type=button]:hover { background: #45a049; }";
  html += "button.red, input[type=submit].red { background: #f44336; }";
  html += "button.red:hover, input[type=submit].red:hover { background: #da190b; }";
  html += "button.orange, input[type=submit].orange { background: #ff9800; }";
  html += "button.orange:hover, input[type=submit].orange:hover { background: #e68900; }";
  html += ".code-item { margin: 8px 0; padding: 12px; background: #2d2d2d; border-radius: 6px; border-left: 3px solid #4CAF50; }";
  html += ".code-item b { color: #4CAF50; font-size: 18px; display: block; margin-bottom: 5px; }";
  html += ".code-item .status { font-size: 12px; color: #aaa; margin-bottom: 8px; }";
  html += ".code-item .status.active { color: #4CAF50; font-weight: bold; }";
  html += ".code-item .status.external { color: #2196F3; font-weight: bold; }";
  html += ".code-item button { width: auto; padding: 6px 12px; margin: 0 5px 5px 0; font-size: 12px; display: inline-block; }";
  html += ".warning-box { background: #FFF3CD; color: #856404; padding: 12px; border-radius: 6px; border-left: 4px solid #FFC107; margin-bottom: 15px; font-size: 13px; }";
  html += ".deactivate-box { background: #ffebee; color: #c62828; padding: 12px; border-radius: 6px; border-left: 4px solid #f44336; margin-bottom: 15px; }";
  html += ".camera-frame { width: 90%; max-width: 800px; aspect-ratio: 4/3; border: 3px solid #444; border-radius: 12px; overflow: hidden; background: #000; }";
  html += ".camera-frame img { width: 100%; height: 100%; object-fit: contain; }";
  html += "#log-container { flex: 1; background: #0d0d0d; border-radius: 8px; border: 1px solid #333; padding: 15px; overflow-y: auto; font-family: 'Courier New', monospace; font-size: 11px; }";
  html += ".log-line { color: #0f0; margin: 2px 0; white-space: pre-wrap; word-wrap: break-word; }";
  html += "::-webkit-scrollbar { width: 8px; }";
  html += "::-webkit-scrollbar-track { background: #1a1a1a; }";
  html += "::-webkit-scrollbar-thumb { background: #4CAF50; border-radius: 4px; }";
  html += "::-webkit-scrollbar-thumb:hover { background: #45a049; }";
  html += "p { font-size: 13px; line-height: 1.6; margin: 8px 0; color: #ccc; }";
  html += ".center-title { position: absolute; top: 20px; color: #2196F3; }";
  html += "</style>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  
  // LEFT PANEL - Settings
  html += "<div class='left-panel' id='left-panel'>";
  html += "<h1>🎥 ESP32-CAM Control</h1>";
  html += "<div id='settings-content'><p style='color:#888;'>Loading...</p></div>";
  html += "</div>";
  
  // CENTER PANEL - Camera Feed
  html += "<div class='center-panel'>";
  html += "<h2 class='center-title'>📡 Camera Feed</h2>";
  html += "<div class='camera-frame'>";
  html += "<img src='http://" + WiFi.softAPIP().toString() + ":81/stream' alt='Camera Stream'>";
  html += "</div>";
  html += "</div>";
  
  // RIGHT PANEL - Serial Log
  html += "<div class='right-panel'>";
  html += "<h1 style='margin-bottom: 10px;'>📊 Serial LIVE Log</h1>";
  html += "<div id='log-container'>Loading...</div>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript for AJAX operations and live updates
  html += "<script>";
  html += "let isScrolledToBottom = true;";
  html += "const logContainer = document.getElementById('log-container');";
  html += "let currentState = {};";
  html += "let lastStateHash = '';";
  html += "";
  html += "logContainer.addEventListener('scroll', function() {";
  html += "  const threshold = 50;";
  html += "  isScrolledToBottom = (logContainer.scrollHeight - logContainer.scrollTop - logContainer.clientHeight) < threshold;";
  html += "});";
  html += "";
  html += "function hashState(state) {";
  html += "  return JSON.stringify(state);";
  html += "}";
  html += "";
  html += "function sendAjax(url, data, callback) {";
  html += "  fetch(url, {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
  html += "    body: data";
  html += "  })";
  html += "  .then(() => { if(callback) callback(); updateStatus(true); })";
  html += "  .catch(err => console.error('Error:', err));";
  html += "}";
  html += "";
  html += "function updateLog() {";
  html += "  fetch('/getlog')";
  html += "    .then(response => response.text())";
  html += "    .then(html => {";
  html += "      logContainer.innerHTML = html;";
  html += "      if(isScrolledToBottom) {";
  html += "        logContainer.scrollTop = logContainer.scrollHeight;";
  html += "      }";
  html += "    })";
  html += "    .catch(err => console.error('Log error:', err));";
  html += "}";
  html += "";
  html += "function updateStatus(forceRender = false) {";
  html += "  fetch('/status')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      const newHash = hashState(data);";
  html += "      if(forceRender || newHash !== lastStateHash) {";
  html += "        lastStateHash = newHash;";
  html += "        currentState = data;";
  html += "        renderSettings(data);";
  html += "      }";
  html += "    })";
  html += "    .catch(err => console.error('Status error:', err));";
  html += "}";
  html += "";
  html += "function renderSettings(s) {";
  html += "  let html = '';";
  html += "";
  html += "  html += '<div class=\"section\" style=\"border-left-color: #f44336;\">';";
  html += "  html += '<h2>🔄 Motor Controller Reset</h2>';";
  html += "  html += '<p style=\"font-size: 12px; color: #aaa; margin-bottom: 10px;\">Pin 2 reset (150ms LOW pulse)</p>';";
  html += "  html += '<button onclick=\"if(confirm(\\'Are you sure you want to reset the motor controller?\\')) sendAjax(\\'/control\\', \\'action=reset\\')\" class=\"red\" style=\"font-size: 16px; font-weight: bold;\">🔄 Reset Motor Controller</button>';";
  html += "  html += '</div>';";
  html += "";
  html += "  html += '<div class=\"section\">';";
  html += "  html += '<h2>📷 Camera Quality</h2>';";
  html += "  html += '<p style=\"font-size: 12px; color: #aaa;\">Lower value = better quality, larger size</p>';";
  html += "  html += '<label>Quality (10-63):</label>';";
  html += "  html += '<input type=\"number\" id=\"quality\" value=\"' + s.cameraQuality + '\" min=\"10\" max=\"63\" placeholder=\"Quality\">';";
  html += "  html += '<button onclick=\"sendAjax(\\'/config\\', \\'quality=\\' + document.getElementById(\\'quality\\').value)\">Apply</button>';";
  html += "  html += '</div>';";
  html += "";
  html += "  html += '<div class=\"section\">';";
  html += "  html += '<h2>🔌 Control Mode</h2>';";
  html += "  html += '<label>Trigger mode:</label>';";
  html += "  html += '<select id=\"extmode\">';";
  html += "  html += '<option value=\"0\" ' + (s.externalTriggerEnabled ? '' : 'selected') + '>Web Control</option>';";
  html += "  html += '<option value=\"1\" ' + (s.externalTriggerEnabled ? 'selected' : '') + '>Pin 12 Control</option>';";
  html += "  html += '</select>';";
  html += "  html += '<button onclick=\"sendAjax(\\'/config\\', \\'extmode=\\' + document.getElementById(\\'extmode\\').value)\">Apply</button>';";
  html += "  html += '</div>';";
  html += "";
  html += "  html += '<div class=\"section\">';";
  html += "  html += '<h2>⚡ Blink Settings</h2>';";
  html += "  html += '<label>Baud Rate (Hz):</label>';";
  html += "  html += '<input type=\"number\" id=\"baud\" value=\"' + s.blinkBaud + '\" min=\"10\" max=\"1000\" placeholder=\"Baud Rate\">';";
  html += "  html += '<label>Pause Between Codes (ms):</label>';";
  html += "  html += '<input type=\"number\" id=\"pause\" value=\"' + s.pauseBetweenCodes + '\" min=\"100\" max=\"5000\" placeholder=\"Pause\">';";
  html += "  html += '<button onclick=\"sendAjax(\\'/config\\', \\'baud=\\' + document.getElementById(\\'baud\\').value + \\'&pause=\\' + document.getElementById(\\'pause\\').value)\">Apply</button>';";
  html += "  html += '</div>';";
  html += "";
  html += "  html += '<div class=\"section\">';";
  html += "  html += '<h2>➕ Add New Code</h2>';";
  html += "  html += '<label>3-character code:</label>';";
  html += "  html += '<input type=\"text\" id=\"newcode\" maxlength=\"3\" minlength=\"3\" placeholder=\"3 characters\">';";
  html += "  html += '<button onclick=\"addCode()\">Save Code</button>';";
  html += "  html += '</div>';";
  html += "";
  html += "  html += '<div class=\"section\">';";
  html += "  html += '<h2>💾 Saved Codes</h2>';";
  html += "";
  html += "  if(s.externalTriggerEnabled) {";
  html += "    html += '<div class=\"warning-box\">⚠️ Pin 12 control is active</div>';";
  html += "  }";
  html += "";
  html += "  if(s.activeCode >= 0 && s.activeCode < 4 && s.codes[s.activeCode].length === 3) {";
  html += "    html += '<div class=\"deactivate-box\">';";
  html += "    html += '<strong>Active Code:</strong> ' + s.codes[s.activeCode];";
  html += "    html += '<button onclick=\"sendAjax(\\'/control\\', \\'action=deactivate\\')\" class=\"red\">❌ Deactivate</button>';";
  html += "    html += '</div>';";
  html += "  }";
  html += "";
  html += "  for(let i = 0; i < 4; i++) {";
  html += "    if(s.codes[i].length === 3) {";
  html += "      html += '<div class=\"code-item\">';";
  html += "      html += '<b>' + s.codes[i] + '</b>';";
  html += "";
  html += "      if(s.activeCode === i && s.shouldBlink && !s.externalTriggerEnabled) {";
  html += "        html += '<div class=\"status active\">● ACTIVE (blinking)</div>';";
  html += "        html += '<button onclick=\"sendAjax(\\'/control\\', \\'action=stop\\')\" class=\"orange\">⏸ Stop</button>';";
  html += "      } else if(s.activeCode === i && !s.shouldBlink && !s.externalTriggerEnabled) {";
  html += "        html += '<div class=\"status\">○ ACTIVE (stopped)</div>';";
  html += "        html += '<button onclick=\"sendAjax(\\'/control\\', \\'action=start&id=' + i + '\\')\" >▶ Start</button>';";
  html += "      } else if(s.activeCode === i && s.externalTriggerEnabled) {";
  html += "        html += '<div class=\"status external\">🔌 ACTIVE (pin controlled)</div>';";
  html += "      } else {";
  html += "        if(!s.externalTriggerEnabled) {";
  html += "          html += '<button onclick=\"sendAjax(\\'/control\\', \\'action=activate&id=' + i + '\\')\" >▶ Activate</button>';";
  html += "        }";
  html += "      }";
  html += "";
  html += "      html += '<button onclick=\"sendAjax(\\'/codes\\', \\'delete=' + i + '\\')\" class=\"red\">🗑 Delete</button>';";
  html += "      html += '</div>';";
  html += "    }";
  html += "  }";
  html += "";
  html += "  html += '</div>';";
  html += "";
  html += "  document.getElementById('settings-content').innerHTML = html;";
  html += "}";
  html += "";
  html += "function addCode() {";
  html += "  const code = document.getElementById('newcode').value;";
  html += "  if(code.length === 3) {";
  html += "    sendAjax('/codes', 'newcode=' + code, () => {";
  html += "      document.getElementById('newcode').value = '';";
  html += "    });";
  html += "  }";
  html += "}";
  html += "";
  html += "updateLog();";
  html += "updateStatus();";
  html += "setInterval(updateLog, 500);";
  html += "setInterval(updateStatus, 2000);";
  html += "console.log('Page loaded, timers started');";
  html += "</script>";
  
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
        addLog("📡 Stream szerver indítva (port 81)");
    }

    httpd_config_t code_config = HTTPD_DEFAULT_CONFIG();
    code_config.server_port = CODE_PORT;
    code_config.ctrl_port = CODE_CTRL_PORT;
    
    if (httpd_start(&code_httpd, &code_config) == ESP_OK) {
        // 1. Main page (GET)
        httpd_uri_t codes_get = { 
            .uri = "/codes", 
            .method = HTTP_GET, 
            .handler = [](httpd_req_t* req) -> esp_err_t { handleCodes(req); return ESP_OK; }, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(code_httpd, &codes_get);

        // 2. Code operations - add/delete (POST)
        httpd_uri_t codes_post = { 
            .uri = "/codes", 
            .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleCodes(req); return ESP_OK; }, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(code_httpd, &codes_post);

        // 3. Control operations - activate/deactivate/start/stop (POST)
        httpd_uri_t control_post = { 
            .uri = "/control", 
            .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleControl(req); return ESP_OK; }, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(code_httpd, &control_post);

        // 4. Configuration - settings/camera/external trigger (POST)
        httpd_uri_t config_post = { 
            .uri = "/config", 
            .method = HTTP_POST,
            .handler = [](httpd_req_t* req) -> esp_err_t { handleConfig(req); return ESP_OK; }, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(code_httpd, &config_post);

        // 5. Log endpoint (GET)
        httpd_uri_t log_get = { 
            .uri = "/getlog", 
            .method = HTTP_GET,
            .handler = log_handler, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(code_httpd, &log_get);

        // 6. Status endpoint (GET)
        httpd_uri_t status_get = { 
            .uri = "/status", 
            .method = HTTP_GET,
            .handler = status_handler, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(code_httpd, &status_get);

        addLog("🌐 Kód kezelő szerver indítva (port 80)");
        addLog("📊 6 végpont regisztrálva");
    }
}

#endif