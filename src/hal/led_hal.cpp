#include "led_hal.h"
#include "../config.h"
#include <FastLED.h>

void FastLedHal::init(CRGB* leds, uint16_t count) {
    FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, count);
    FastLED.setBrightness(128);
}

void FastLedHal::show() {
    FastLED.show();
}

void FastLedHal::setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
}
