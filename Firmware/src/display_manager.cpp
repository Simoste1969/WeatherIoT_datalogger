#include "display_manager.h"
#include "config.h"
#include <Wire.h>

#ifdef ENABLE_LCD_DISPLAY
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(I2C_ADDR_LCD, LCD_COLS, LCD_ROWS);

// Icone Personalizzate
byte wifi1[8] = {0,0,0,0,0,0,0,24};
byte wifi2[8] = {0,0,0,0,0,6,6,30};
byte wifi3[8] = {0,0,1,1,7,7,15,31};
byte sdErr[8] = {31,17,27,14,27,17,17,31};
byte sdLvl0[8] = {31,17,17,17,17,17,17,31}; // Vuota/Piena dati
byte sdLvl1[8] = {31,17,17,17,17,31,31,31};
byte sdLvl2[8] = {31,17,17,31,31,31,31,31};
byte sdLvl3[8] = {31,31,31,31,31,31,31,31}; // Piena/Libera
#endif

void setupDisplay() {
#ifdef ENABLE_LCD_DISPLAY
  Wire.beginTransmission(I2C_ADDR_LCD);
  if (Wire.endTransmission() == 0) {
      lcd.init(); lcd.backlight();
      lcd.createChar(0, wifi1); lcd.createChar(1, wifi2); lcd.createChar(2, wifi3);
      lcd.createChar(3, sdErr); lcd.createChar(4, sdLvl0); lcd.createChar(5, sdLvl1); lcd.createChar(6, sdLvl2); lcd.createChar(7, sdLvl3);
      lcd.setCursor(0, 0); lcd.print("Display Ready");
  } else {
      Serial.println("[LCD] Errore: Display non trovato!");
  }
#endif
}

void showBootScreen(bool isApMode, bool sdStatus) {
#ifdef ENABLE_LCD_DISPLAY
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("WeatherIoT Log V1.0");
  lcd.setCursor(0,1); lcd.print("--------------------");
  lcd.setCursor(0,2); lcd.print(isApMode ? "WiFi: AP Mode" : "WiFi: Client Mode");
  lcd.setCursor(0,3); lcd.print("SD: "); lcd.print(sdStatus ? "Presente" : "ASSENTE!");
  delay(3000); lcd.clear();
#endif
}

void updateDisplay(String timeStr, float temp, float hum, float airQuality, String ip, bool isApMode, int wifiRSSI, bool sdStatus, float sdUsed, float sdTotal) {
#ifdef ENABLE_LCD_DISPLAY
  lcd.setCursor(0,0); lcd.print(timeStr);
  
  // Icone Stato (Col 18 e 19)
  lcd.setCursor(18,0);
  if(isApMode) lcd.write(2); // Max signal
  else {
      if (wifiRSSI >= -60) lcd.write(2);
      else if (wifiRSSI >= -85) lcd.write(1);
      else if (wifiRSSI == 0) lcd.print("x");
      else lcd.write(0);
  }

  lcd.setCursor(19,0);
  if (!sdStatus || sdTotal == 0) lcd.write(3); // Errore
  else {
      float freePct = ((sdTotal - sdUsed) / sdTotal) * 100.0;
      if (freePct > 80) lcd.write(7);
      else if (freePct > 50) lcd.write(6);
      else if (freePct > 20) lcd.write(5);
      else lcd.write(4);
  }

  lcd.setCursor(0,1); lcd.print("Temp: "); lcd.print(temp, 1); lcd.print((char)223); lcd.print("C     ");
  lcd.setCursor(0,2); lcd.print("Hum:"); lcd.print(hum, 0); lcd.print("% "); lcd.print("AirQ:"); lcd.print(airQuality, 0); lcd.print("% ");
  lcd.setCursor(0,3);
  if (isApMode) lcd.print("AP: 192.168.4.1     ");
  else { lcd.print("IP: "); lcd.print(ip); int l = 4 + ip.length(); for(int i=l; i<20; i++) lcd.print(" "); }
#endif
}