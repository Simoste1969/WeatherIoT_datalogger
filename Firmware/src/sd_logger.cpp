#include "sd_logger.h"
#include "led_manager.h"
#include <SPI.h>
#include <SD.h>
#include <FS.h>

bool sdStatus = false;

bool setupSD() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI)) { sdStatus = false; return false; }
    if (SD.cardType() == CARD_NONE) { sdStatus = false; return false; }

    if (!SD.exists("/datalog.csv")) {
        File file = SD.open("/datalog.csv", FILE_WRITE);
        if (file) { file.println("Date,Time,Temp,Hum,Pres,Gas"); file.close(); }
    }
    sdStatus = true;
    return true;
}

void logDataToSD(String date, String time, float temp, float hum, float pres, float gas) {
    if (!sdStatus) { flashColor(255, 100, 0, 2); return; }
    File file = SD.open("/datalog.csv", FILE_APPEND);
    if (file) {
        file.printf("%s,%s,%.2f,%.2f,%.2f,%.0f\n", date.c_str(), time.c_str(), temp, hum, pres, gas);
        file.close();
    } else {
        sdStatus = false; flashColor(255, 100, 0, 2);
    }
}

// --- NUOVO: CARICAMENTO STORICO ---
void loadHistoryFromSD(std::vector<DataPoint>& history, int maxPoints) {
    if (!sdStatus || !SD.exists("/datalog.csv")) return;

    File file = SD.open("/datalog.csv", FILE_READ);
    if (!file) return;

    Serial.println("[SD] Caricamento storico in corso...");
    history.clear();

    // Salta Header
    if (file.available()) file.readStringUntil('\n');

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        // Parsing CSV: Date,Time,Temp,Hum...
        // Esempio: 2023-10-10,12:00,24.5,60.0,1013,5000
        
        int idx1 = line.indexOf(','); // Dopo Date
        int idx2 = line.indexOf(',', idx1 + 1); // Dopo Time
        int idx3 = line.indexOf(',', idx2 + 1); // Dopo Temp
        int idx4 = line.indexOf(',', idx3 + 1); // Dopo Hum

        if (idx1 > 0 && idx2 > 0 && idx3 > 0 && idx4 > 0) {
            String timeStr = line.substring(idx1 + 1, idx2);
            String tempStr = line.substring(idx2 + 1, idx3);
            String humStr  = line.substring(idx3 + 1, idx4);

            DataPoint dp;
            dp.time = timeStr;
            dp.temp = tempStr.toFloat();
            dp.hum  = humStr.toFloat();

            // Aggiungi al vettore (logica circolare se pieno)
            if (history.size() >= maxPoints) history.erase(history.begin());
            history.push_back(dp);
        }
    }
    file.close();
    Serial.printf("[SD] Caricati %d punti nello storico.\n", history.size());
}

bool isSDWorking() { return sdStatus; }

void getSDSpace(float &totalGB, float &usedGB) {
    if (!sdStatus) { totalGB=0; usedGB=0; return; }
    totalGB = (float)SD.totalBytes() / 1073741824.0;
    usedGB = (float)SD.usedBytes() / 1073741824.0;
}

void wipeSDCard() {
    if(sdStatus && SD.exists("/datalog.csv")) {
        SD.remove("/datalog.csv");
        File file = SD.open("/datalog.csv", FILE_WRITE);
        if (file) { file.println("Date,Time,Temp,Hum,Pres,Gas"); file.close(); }
    }
}