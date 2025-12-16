#ifndef LED_MANAGER_H
#define LED_MANAGER_H
#include <Arduino.h>

void setupLED();
void setStatusColor(bool wifiConnected);
void setApModeColor();
void flashColor(uint8_t r, uint8_t g, uint8_t b, int count);
void clearLed();

#endif