#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H
#include <Arduino.h>

void setupDisplay();
void showBootScreen(bool isApMode, bool sdStatus);
void updateDisplay(String timeStr, float temp, float hum, float airQuality, String ip, bool isApMode, int wifiRSSI, bool sdStatus, float sdUsed, float sdTotal);

#endif