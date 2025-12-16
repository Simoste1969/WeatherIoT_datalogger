#include "config_manager.h"
#include "config.h"
#include <Wire.h>
#include <Preferences.h>

Preferences internalMem;
#define MAGIC_BYTE 0x77 // Cambiato per forzare re-init struttura

bool eepromDetected = false;

void initConfig() {
    // Wire.begin() viene chiamato nel main PRIMA di questo
#ifdef ENABLE_EXT_EEPROM
    // CHECK PRESENZA EEPROM 0x50
    Wire.beginTransmission(I2C_ADDR_EEPROM);
    if (Wire.endTransmission() == 0) {
        Serial.println("[EEPROM] AT24C64 Rilevata!");
        eepromDetected = true;
    } else {
        Serial.println("[EEPROM] ERRORE: Non trovata! Uso Flash Interna.");
        eepromDetected = false;
    }
#endif
}

// Scrittura sicura byte per byte (Lento ma affidabile per evitare Page Boundary issues)
void i2cWriteByte(uint16_t addr, uint8_t data) {
    Wire.beginTransmission(I2C_ADDR_EEPROM);
    Wire.write((int)(addr >> 8));   // MSB
    Wire.write((int)(addr & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(5); // Ritardo fondamentale per ciclo scrittura EEPROM (5ms)
}

uint8_t i2cReadByte(uint16_t addr) {
    Wire.beginTransmission(I2C_ADDR_EEPROM);
    Wire.write((int)(addr >> 8));
    Wire.write((int)(addr & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(I2C_ADDR_EEPROM, 1);
    if (Wire.available()) return Wire.read();
    return 0xFF;
}

void saveSystemConfig(const SystemConfig &cfg) {
    SystemConfig safeCfg = cfg;
    safeCfg.magic = MAGIC_BYTE;
    
    #ifdef ENABLE_EXT_EEPROM
    if(eepromDetected) {
        uint8_t* p = (uint8_t*)&safeCfg;
        for(size_t i=0; i<sizeof(SystemConfig); i++) {
            i2cWriteByte(i, p[i]);
        }
        Serial.println("[EEPROM] Salvataggio completato.");
        return;
    }
    #endif

    // Fallback Flash Interna
    internalMem.begin("sys-conf", false);
    internalMem.putBytes("data", &safeCfg, sizeof(SystemConfig));
    internalMem.end();
    Serial.println("[FLASH] Salvataggio completato.");
}

bool loadSystemConfig(SystemConfig &cfg) {
    #ifdef ENABLE_EXT_EEPROM
    if(eepromDetected) {
        uint8_t* p = (uint8_t*)&cfg;
        for(size_t i=0; i<sizeof(SystemConfig); i++) {
            p[i] = i2cReadByte(i);
        }
        return (cfg.magic == MAGIC_BYTE);
    }
    #endif

    internalMem.begin("sys-conf", true);
    size_t len = internalMem.getBytes("data", &cfg, sizeof(SystemConfig));
    internalMem.end();
    return (len == sizeof(SystemConfig) && cfg.magic == MAGIC_BYTE);
}

void wipeSystemConfig() {
    SystemConfig empty;
    memset(&empty, 0, sizeof(SystemConfig));
    empty.humThreshMin = 50.0;
    empty.humThreshMax = 70.0;
    empty.nmosAutoMode = false;
    saveSystemConfig(empty);
    Serial.println("[CONFIG] Factory Reset Eseguito.");
}