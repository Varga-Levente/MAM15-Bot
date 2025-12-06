#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WiFi.h>

#include "settings.h"
#include "storage.h"
#include "camera.h"
#include "blink.h"
#include "webserver.h"

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.println("\n\n🚀 ESP32-CAM Kódvillogtató indítása... (V10-Final)");
  
  // Initialize motor controller reset pin (default HIGH)
  pinMode(EXTERNAL_CONTROLLER_RESET_PIN, OUTPUT);
  digitalWrite(EXTERNAL_CONTROLLER_RESET_PIN, HIGH);
  Serial.println("✅ Motorvezérlő reset pin inicializálva (pin " + String(EXTERNAL_CONTROLLER_RESET_PIN) + " HIGH)");
  
  loadCodes();
  
  if(!initCamera()) {
    addLog("❌ Kamera inicializálás sikertelen!");
    return;
  }
  
  // Configure static IP for Access Point (NO DHCP)
  if (!WiFi.softAPConfig(LOCAL_IP, GATEWAY, SUBNET)) {
    addLog("❌ Statikus IP konfiguráció sikertelen!");
    return;
  }
  
  // Start Access Point
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  addLog("📶 Hotspot név: " + String(AP_SSID));
  addLog("🔑 Jelszó: " + String(AP_PASSWORD));
  addLog("🌐 Fix IP cím: " + WiFi.softAPIP().toString());
  addLog("   (DHCP kikapcsolva - fix IP használata)");
  
  startCameraServer();
  startBlinkTask();
  
  addLog("");
  addLog("✅ Rendszer kész!");
  addLog("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  addLog("📱 Kezelőfelület: http://" + WiFi.softAPIP().toString() + "/codes");
  addLog("📡 Video stream: http://" + WiFi.softAPIP().toString() + ":81/stream");
  addLog("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

void loop() {
  delay(1000);
}