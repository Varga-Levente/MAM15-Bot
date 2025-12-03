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
  Serial.println("\n\n🚀 ESP32-CAM Kódvillogtató indítása...");

  // REMOVE ME (Controller ESP Reset)
  digitalWrite(EXTERNAL_CONTROLLER_RESET_PIN, HIGH);
  
  loadCodes();
  
  if(!initCamera()) {
    Serial.println("❌ Kamera inicializálás sikertelen!");
    return;
  }
  
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("📶 Hotspot név: "); Serial.println(AP_SSID);
  Serial.print("🔑 Jelszó: "); Serial.println(AP_PASSWORD);
  Serial.print("🌐 IP cím: "); Serial.println(WiFi.softAPIP());
  
  startCameraServer();
  startBlinkTask();
  
  Serial.println("\n✅ Rendszer kész!");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.printf("📱 Kezelőfelület: http://%s/codes\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("📡 Video stream: http://%s:81/stream\n", WiFi.softAPIP().toString().c_str());
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  delay(1000);
}