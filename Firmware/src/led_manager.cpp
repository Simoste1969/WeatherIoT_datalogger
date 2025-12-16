#include "led_manager.h"
#include "config.h"

#ifdef ENABLE_RGB_LED
#include <Adafruit_NeoPixel.h>
static Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

void setupLED() {
#ifdef ENABLE_RGB_LED
    pixels.begin(); pixels.setBrightness(40); pixels.show();
#endif
}

void setStatusColor(bool wifiConnected) {
#ifdef ENABLE_RGB_LED
    // Verde = Online, Rosso = Offline
    if(wifiConnected) pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    else pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
#endif
}

void setApModeColor() {
#ifdef ENABLE_RGB_LED
    pixels.setPixelColor(0, pixels.Color(255, 180, 0)); // Giallo
    pixels.show();
#endif
}

void flashColor(uint8_t r, uint8_t g, uint8_t b, int count) {
#ifdef ENABLE_RGB_LED
    for(int i=0; i<count; i++) {
        pixels.setPixelColor(0, pixels.Color(r,g,b)); pixels.show(); delay(150);
        pixels.setPixelColor(0, 0); pixels.show(); if(i<count-1) delay(150);
    }
#endif
}

void clearLed() {
#ifdef ENABLE_RGB_LED
    pixels.clear(); pixels.show();
#endif
}