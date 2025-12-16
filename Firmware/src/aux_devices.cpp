#include "aux_devices.h"
#include "config.h"

// Parametri NTC (Beta 3950, 100k)
const float VCC = 3.3;
const float R_DIV = 100000.0; 
const float BETA = 3950.0;    
const float T0 = 298.15;      
const float R0 = 100000.0;    

static bool nmosState = false;

void setupAuxDevices() {
#ifdef ENABLE_NMOS_LOAD
    pinMode(PIN_NMOS, OUTPUT);
    digitalWrite(PIN_NMOS, LOW);
#endif
#ifdef ENABLE_NTC_SENSOR
    pinMode(PIN_NTC, INPUT);
    analogReadResolution(12);
#endif
}

float readNTCTemperature() {
#ifdef ENABLE_NTC_SENSOR
    int adc = analogRead(PIN_NTC);
    if(adc == 0 || adc >= 4095) return -999.0;
    float v_out = (adc * VCC) / 4095.0;
    float r_ntc = (VCC * R_DIV / v_out) - R_DIV;
    float tempK = 1.0 / ( (1.0/T0) + (log(r_ntc/R0) / BETA) );
    return tempK - 273.15;
#else
    return 0.0;
#endif
}

void setNMOSState(bool state) {
#ifdef ENABLE_NMOS_LOAD
    digitalWrite(PIN_NMOS, state ? HIGH : LOW);
    nmosState = state;
#endif
}

bool getNMOSState() { return nmosState; }