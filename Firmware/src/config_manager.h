#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#include <Arduino.h>

struct SystemConfig {
    uint8_t magic;
    char ssid[32];
    char pass[64];
    bool useStaticIP;
    char staticIP[16];
    char gateway[16];
    
    // Cloud & Automation
    bool tsEnabled;
    char tsKey[20];
    bool nmosAutoMode;
    float humThreshMin;
    float humThreshMax;
};

void initConfig(); // Init Wire se necessario
bool loadSystemConfig(SystemConfig &cfg);
void saveSystemConfig(const SystemConfig &cfg);
void wipeSystemConfig();

#endif