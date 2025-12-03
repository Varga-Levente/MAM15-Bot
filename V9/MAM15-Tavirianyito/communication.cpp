#include "communication.h"
#include "settings.h"

Communication::Communication() {
  crcCalculator = new CRC16(
    CRCSettings::POLYNOMIAL,
    CRCSettings::INITIAL_VALUE,
    CRCSettings::FINAL_XOR_VALUE,
    true,
    true
  );
}

Communication::~Communication() {
  delete crcCalculator;
}

bool Communication::init() {
  LoRa.setPins(
    LoRaSettings::SS_PIN,
    LoRaSettings::RESET_PIN,
    LoRaSettings::DIO0_PIN
  );
  
  if (!LoRa.begin(LoRaSettings::FREQUENCY)) {
    if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_COMMUNICATION) {
      Serial.println("❌ Hiba: LoRa inicializálás sikertelen!");
    }
    return false;
  }

  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_COMMUNICATION) {
    Serial.println("✅ LoRa adó mód aktiválva");
  }
  
  return true;
}

void Communication::sendPacket(uint8_t robotId, byte motorCommand, bool speedFlag, bool landingFlag) {
  // Adat csomag összeállítása
  uint8_t transmitPacket[PacketSettings::PACKET_SIZE];
  transmitPacket[0] = robotId;
  transmitPacket[1] = motorCommand;
  transmitPacket[2] = speedFlag;
  transmitPacket[3] = landingFlag;

  // CRC számítása
  uint16_t packetCRC = calculateCRC(transmitPacket, PacketSettings::PACKET_SIZE);

  // LoRa csomag küldése
  LoRa.beginPacket();
  LoRa.write(transmitPacket, PacketSettings::PACKET_SIZE);
  LoRa.write(packetCRC >> 8);
  LoRa.write(packetCRC & 0xFF);
  LoRa.endPacket();

  if (DebugSettings::GLOBAL_DEBUG && DebugSettings::LOG_COMMUNICATION && motorCommand != 0) {
    Serial.print("📡 Csomag elküldve - ID: ");
    Serial.print(robotId);
    Serial.print(" | Motor: 0b");
    Serial.print(motorCommand, BIN);
    Serial.print(" | Sebesség: ");
    Serial.print(speedFlag);
    Serial.print(" | Landoló: ");
    Serial.print(landingFlag);
    Serial.print(" | CRC: 0x");
    Serial.println(packetCRC, HEX);
  }
}

uint16_t Communication::calculateCRC(uint8_t* data, size_t length) {
  crcCalculator->restart();
  crcCalculator->add(data, length);
  return crcCalculator->getCRC();
}