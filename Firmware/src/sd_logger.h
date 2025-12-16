#ifndef SD_LOGGER_H
#define SD_LOGGER_H
#include "config.h" // Per struct DataPoint
#include <vector>

bool setupSD();
void logDataToSD(String date, String time, float temp, float hum, float pres, float gas);
bool isSDWorking();
void getSDSpace(float &totalGB, float &usedGB);
void wipeSDCard();

// NUOVA FUNZIONE: Riempie il vettore con i dati letti dalla SD
void loadHistoryFromSD(std::vector<DataPoint>& history, int maxPoints);

#endif