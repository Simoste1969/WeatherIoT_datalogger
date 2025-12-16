#ifndef AUX_DEVICES_H
#define AUX_DEVICES_H
#include <Arduino.h>

void setupAuxDevices();
float readNTCTemperature(); 
void setNMOSState(bool state);
bool getNMOSState();

#endif