#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "config_manager.h" 
#include "config.h" // Per DataPoint
#include <vector>

void setupNetwork(SystemConfig &cfg);
void handleClient();
void updateSensorData(float temp, float hum, float pres, float gasPct, float ntcTemp, bool nmosSt, String timeStr, int rssi, bool sdOk, float sdUsed, float sdTot);
void logToHistory(String time, float temp, float hum);
void initHistory(); // Richiama caricamento SD
bool isApMode();

#endif