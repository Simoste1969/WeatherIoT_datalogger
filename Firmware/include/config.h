#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h> // Necessario per String

// =========================================================
//            ABILITAZIONE MODULI HARDWARE
// =========================================================
#define ENABLE_RGB_LED
#define ENABLE_LCD_DISPLAY
#define ENABLE_EXT_EEPROM
#define ENABLE_NTC_SENSOR   
#define ENABLE_NMOS_LOAD    

// =========================================================

// --- Struttura Dati Condivisa (Spostata qui per visibilità globale) ---
struct DataPoint { 
    String time; 
    float temp; 
    float hum; 
};

// --- WiFi Default ---
#define AP_SSID "WeatherIoT_AP"

// --- Pinout ---
#define I2C_SDA 8
#define I2C_SCL 9
#define SD_MISO 13
#define SD_MOSI 11
#define SD_SCK  12
#define SD_CS   21
#define LED_PIN  39  
#define PIN_NTC  5   
#define PIN_NMOS 38  

// --- Indirizzi ---
#define I2C_ADDR_LCD    0x27 
#define I2C_ADDR_BME    0x77 
#define I2C_ADDR_EEPROM 0x50 

// --- Settings ---
#define LCD_COLS 20
#define LCD_ROWS 4
#define NUM_LEDS 1

#endif