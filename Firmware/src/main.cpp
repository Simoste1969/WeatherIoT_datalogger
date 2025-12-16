#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "config.h"
#include "config_manager.h"
#include "led_manager.h"
#include "display_manager.h"
#include "sd_logger.h"
#include "network_manager.h"
#include "aux_devices.h"

Adafruit_BME680 bme;
RTC_DS3231 rtc;
SystemConfig sysConfig;

unsigned long lastFast=0, lastCloud=0, lastHist=0, lastSerial=0, lastWifiCheck=0;
bool bmeOk = false;

// ... [Funzioni checkSerialConfig, printDataToSerial, sendTS, calcAQ INVARIATE, lasciale come prima] ...
// (Per brevità le ometto ma sono necessarie)
void checkSerialConfig() { if (Serial.available()) { String i=Serial.readStringUntil('\n'); i.trim(); if(i.length()>0){ int sIdx=i.indexOf(':'); if(sIdx>0){ String s=i.substring(0,sIdx); String p=i.substring(sIdx+1); memset(sysConfig.ssid,0,32); memset(sysConfig.pass,0,64); strncpy(sysConfig.ssid,s.c_str(),31); strncpy(sysConfig.pass,p.c_str(),63); saveSystemConfig(sysConfig); delay(1000); ESP.restart(); }}}}
void printDataToSerial(String tStr,float t,float h,float p,float g,float ntc){
#ifdef ENABLE_SERIAL_LOGGING
Serial.printf("[%s] T:%.1f H:%.0f NMOS:%d\n", tStr.c_str(), t, h, getNMOSState());
#endif
}
void sendTS(String k, float t, float h) { if(WiFi.status()==WL_CONNECTED && k.length()>5){ HTTPClient http; http.begin("http://api.thingspeak.com/update?api_key="+k+"&field1="+String(t,1)+"&field2="+String(h,1)); http.GET(); http.end(); }}
float calcAQ(float r) { return (r>50000)?100:((r<5000)?0:(r-5000)/45000.0*100.0); }

void setup() {
  delay(2000); Serial.begin(115200);

  setupLED(); setupAuxDevices(); Wire.begin(I2C_SDA, I2C_SCL); setupDisplay();
  
  initConfig();
  if(!loadSystemConfig(sysConfig)) { wipeSystemConfig(); memset(&sysConfig,0,sizeof(SystemConfig)); sysConfig.humThreshMin=50.0; sysConfig.humThreshMax=70.0; }

  setupSD();
  if(bme.begin(I2C_ADDR_BME)) { bmeOk=true; bme.setTemperatureOversampling(BME680_OS_8X); bme.setHumidityOversampling(BME680_OS_2X); bme.setPressureOversampling(BME680_OS_4X); bme.setIIRFilterSize(BME680_FILTER_SIZE_3); bme.setGasHeater(320, 150); }

  rtc.begin();
  setupNetwork(sysConfig);
  if(!isApMode() && WiFi.status()==WL_CONNECTED) configTime(3600, 3600, "pool.ntp.org");
  
  // CARICAMENTO STORICO DALLA SD
  initHistory(); 

  showBootScreen(isApMode(), isSDWorking());
}

void loop() {
  handleClient(); checkSerialConfig();
  unsigned long now = millis();

  if(!isApMode() && WiFi.status()!=WL_CONNECTED && (now-lastWifiCheck>=30000)) { lastWifiCheck=now; WiFi.reconnect(); }

  // LOGICA LED AGGIORNATA
  if(isApMode()) { 
      if((now/1000)%2==0) setApModeColor(); else clearLed(); 
  } else {
      // Priorità al Carico Attivo (VIOLA)
      if (getNMOSState()) {
          // Viola (R+B)
          flashColor(180, 0, 255, 1); // Solido "viola" non è nativo, usiamo un trick o impostiamo colore fisso
          // Meglio usare setPixelColor diretto per colore fisso:
          // In led_manager non abbiamo una funzione "setCustomColor", usiamo flashColor breve o modifichiamo led_manager. 
          // Per semplicità uso flashColor simulando colore fisso ripetuto o aggiungiamo la logica.
          // LOGICA MIGLIORE:
      } else {
          setStatusColor(WiFi.status() == WL_CONNECTED);
      }
  }
  
  // FIX LED LOGIC (Inserisci questo blocco nel loop per gestire correttamente il Viola)
  // Per fare il viola fisso senza flash, serve modificare led_manager o usare un trucco.
  // Modifico led_manager.cpp per esporre una funzione 'setPurple()' o uso un comando diretto qui se pixel pubblico.
  // Dato che pixels è statico in led_manager, usiamo setStatusColor modificata o gestiamo qui la logica.
  
  /* PER RENDERE IL LED VIOLA FISSO CON LE FUNZIONI ESISTENTI:
     Non posso settarlo fisso da qui facilmente senza esporre 'pixels'.
     Tuttavia, ho 'flashColor'.
     Facciamo così:
  */
  if (!isApMode()) {
     if (getNMOSState()) {
         // Se NMOS ON -> Imposta colore Viola (usiamo flashColor con count 0 se possibile o modifichiamo led_manager)
         // Modifica rapida: in led_manager.cpp aggiungi void setPurple() { pixels.setPixelColor(0, pixels.Color(200,0,255)); pixels.show(); }
         // Ma non posso modificare led_manager ora senza rimandarti il file.
         // RIMANDO led_manager.h/.cpp RAPIDO sotto per aggiungere setPurple()
     } else {
         setStatusColor(WiFi.status() == WL_CONNECTED);
     }
  }

  // LOGICA AUTOMAZIONE
  if(sysConfig.nmosAutoMode && bmeOk) {
      if (bme.humidity > sysConfig.humThreshMax) setNMOSState(true);
      else if (bme.humidity < sysConfig.humThreshMin) setNMOSState(false);
  }

  if(now - lastFast >= 2000) {
    lastFast = now;
    float t=0,h=0,p=0,g=0,aq=0;
    if(bmeOk && bme.performReading()) { t=bme.temperature; h=bme.humidity; p=bme.pressure/100.0; g=bme.gas_resistance; aq=calcAQ(g); }
    float ntc = readNTCTemperature();
    DateTime dt = rtc.now(); char tb[10]; sprintf(tb,"%02d:%02d:%02d",dt.hour(),dt.minute(),dt.second()); String tStr=String(tb);
    float sT=0,sU=0; getSDSpace(sT,sU);
    String ipStr = isApMode()?"192.168.4.1":WiFi.localIP().toString();
    updateDisplay(tStr,t,h,aq,ipStr,isApMode(),WiFi.RSSI(),isSDWorking(),sU,sT);
    updateSensorData(t,h,p,aq,ntc,getNMOSState(),tStr,WiFi.RSSI(),isSDWorking(),sU,sT);
    
    // GESTIONE LED VIOLA NEL LOOP LENTO (così non fluttua)
    if(!isApMode()) {
        if(getNMOSState()) flashColor(200, 0, 255, 1); // Viola lampeggiante breve ogni 2s per indicare carico
        // Nota: lampeggia sopra il verde/rosso. Va bene.
    }
    
    if(now-lastSerial>=15000){ lastSerial=now; printDataToSerial(tStr,t,h,p,g,ntc); }
  }

  if(!isApMode() && sysConfig.tsEnabled && (now-lastCloud>=60000)) { lastCloud=now; if(bmeOk) sendTS(String(sysConfig.tsKey), bme.temperature, bme.humidity); }

  if(now-lastHist>=300000) {
    lastHist=now; 
    if(WiFi.status()!=WL_CONNECTED) flashColor(255,0,0,3);
    DateTime dt=rtc.now(); char tb[10]; sprintf(tb,"%02d:%02d",dt.hour(),dt.minute()); char db[12]; sprintf(db,"%04d-%02d-%02d",dt.year(),dt.month(),dt.day());
    float t=0,h=0,p=0,g=0; if(bmeOk){t=bme.temperature;h=bme.humidity;p=bme.pressure/100.0;g=bme.gas_resistance;}
    logToHistory(String(tb),t,h);
    logDataToSD(String(db),String(tb),t,h,p,g);
  }
}