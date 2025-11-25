#include "lora_receiver.h"
#include "settings.h"

LoRaReceiver::LoRaReceiver() 
  : lastReceivedPacketTime(0),
    lastHealthCheck(0),
    stateChangeTime(0),
    currentState(LORA_OK),
    moduleHealthy(true),
    restartCount(0) {
  
  crcCalculator = new CRC16(
    CRCSettings::POLYNOMIAL,
    CRCSettings::INITIAL_VALUE,
    CRCSettings::FINAL_XOR_VALUE,
    true,
    true
  );
}

LoRaReceiver::~LoRaReceiver() {
  delete crcCalculator;
}

bool LoRaReceiver::init() {
  LoRa.setPins(
    LoRaSettings::SS_PIN,
    LoRaSettings::RESET_PIN,
    LoRaSettings::DIO0_PIN
  );
  
  if (!LoRa.begin(LoRaSettings::FREQUENCY)) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_LORA) {
      Serial.println("❌ Hiba: LoRa inicializálás sikertelen!");
    }
    return false;
  }
  
  pinMode(LoRaSettings::RESET_PIN, OUTPUT);
  digitalWrite(LoRaSettings::RESET_PIN, HIGH);
  
  lastReceivedPacketTime = millis();
  lastHealthCheck = millis();
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_LORA) {
    Serial.println("✅ LoRa vevő inicializálva");
  }
  
  return true;
}

bool LoRaReceiver::restartModule() {
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_HEALTH) {
    Serial.println("🔄 LoRa modul újraindítása...");
  }
  
  LoRa.end();
  delay(100);
  digitalWrite(LoRaSettings::RESET_PIN, LOW);
  delay(10);
  digitalWrite(LoRaSettings::RESET_PIN, HIGH);
  delay(50);
  
  bool success = LoRa.begin(LoRaSettings::FREQUENCY);
  
  if (success) {
    restartCount++;
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_HEALTH) {
      Serial.printf("✅ LoRa modul újraindítva (%d. alkalommal)\n", restartCount);
    }
  } else {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_HEALTH) {
      Serial.println("❌ LoRa modul újraindítása sikertelen!");
    }
  }
  
  return success;
}

void LoRaReceiver::checkHealth() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastHealthCheck < HealthSettings::CHECK_INTERVAL_MS) {
    return;
  }
  
  lastHealthCheck = currentTime;
  
  bool loraWorking = (currentTime - lastReceivedPacketTime) < HealthSettings::CHECK_INTERVAL_MS;
  
  if (!loraWorking && moduleHealthy) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_HEALTH) {
      Serial.println("⚠️ LoRa: Nincs csomag 5 másodperc alatt - újracsatlakozás indítása...");
    }
    moduleHealthy = false;
    currentState = LORA_RECONNECTING;
    stateChangeTime = currentTime;
  }
  
  if (currentState == LORA_RECONNECTING) {
    if (currentTime - stateChangeTime > HealthSettings::RECONNECT_TIMEOUT_MS) {
      if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_HEALTH) {
        Serial.println("❌ LoRa újracsatlakozási időtúllépés!");
      }
      currentState = LORA_DISCONNECTED;
    } else {
      if (restartModule()) {
        if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_HEALTH) {
          Serial.println("✅ LoRa modul sikeresen újracsatlakozott!");
        }
        moduleHealthy = true;
        currentState = LORA_OK;
        lastReceivedPacketTime = currentTime;
      } else {
        delay(1000);
      }
    }
  }
}

bool LoRaReceiver::validateCRC(byte* packet, int dataSize) {
  uint16_t receivedCRC = (packet[dataSize] << 8) | packet[dataSize + 1];
  
  crcCalculator->restart();
  crcCalculator->add(packet, dataSize);
  uint16_t calculatedCRC = crcCalculator->getCRC();
  
  bool valid = (receivedCRC == calculatedCRC);
  
  if (!valid && DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_LORA) {
    Serial.println("❌ Hibás CRC - csomag elvetve!");
  }
  
  return valid;
}

bool LoRaReceiver::receivePacket(byte* packet, int expectedSize) {
  int packetSize = LoRa.parsePacket();
  
  if (!packetSize) {
    return false;
  }
  
  if (packetSize != expectedSize) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_LORA) {
      Serial.printf("⚠️ Hibás csomag méret! Várt: %d, Kapott: %d\n", expectedSize, packetSize);
    }
    return false;
  }
  
  for (int i = 0; i < expectedSize; i++) {
    packet[i] = LoRa.read();
  }
  
  if (!validateCRC(packet, PacketSettings::DATA_SIZE)) {
    return false;
  }
  
  lastReceivedPacketTime = millis();
  
  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_LORA) {
    Serial.println("📥 LoRa csomag fogadva és validálva");
  }
  
  return true;
}

bool LoRaReceiver::isHealthy() {
  return currentState == LORA_OK;
}

unsigned long LoRaReceiver::getLastPacketTime() {
  return lastReceivedPacketTime;
}